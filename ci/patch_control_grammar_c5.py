#!/usr/bin/env python3
from pathlib import Path
import hashlib

UI = Path('mello/mello_ui.c')
PRE_UI_SHA = 'a3829d2fdc354154021727838b4b17cf8420832f88565a2b2f290b17a06002c5'


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replace_function(src: str, signature: str, replacement: str) -> str:
    if src.count(signature) != 1:
        raise SystemExit(f'FAIL: WP-C5 function marker {signature!r} count {src.count(signature)}')
    start = src.index(signature)
    brace = src.index('{', start)
    depth = 0
    end = None
    for i in range(brace, len(src)):
        if src[i] == '{':
            depth += 1
        elif src[i] == '}':
            depth -= 1
            if depth == 0:
                end = i + 1
                break
    if end is None:
        raise SystemExit(f'FAIL: WP-C5 unterminated function {signature!r}')
    if end < len(src) and src[end] == '\n':
        end += 1
    return src[:start] + replacement + src[end:]


actual = sha(UI)
if actual != PRE_UI_SHA:
    raise SystemExit(f'FAIL: WP-C5 UI base hash {actual} != {PRE_UI_SHA}')

s = UI.read_text()
forward = 'static void handle_context_action(MelloUi *ui, uint32_t now_ms, uint8_t action);\n'
if s.count(forward) != 1:
    raise SystemExit(f'FAIL: WP-C5 forward marker count {s.count(forward)}')

mapping = r'''typedef enum {
    MELLO_CTX_NONE = 0,
    MELLO_CTX_OPEN_RANDOM,
    MELLO_CTX_STEP_SKIP,
    MELLO_CTX_LOOP_RESET,
    MELLO_CTX_RANDOM_REROLL,
    MELLO_CTX_RANDOM_CLEAR,
    MELLO_CTX_RANDOM_NONE,
    MELLO_CTX_RANDOM_ALL,
    MELLO_CTX_DOT_REROLL,
    MELLO_CTX_DOT_RESET,
    MELLO_CTX_TURING_TOGGLE,
    MELLO_CTX_TURING_REROLL,
    MELLO_CTX_SUB_TOGGLE,
    MELLO_CTX_SUB_STEP,
    MELLO_CTX_CHORD_TOGGLE,
    MELLO_CTX_GLOBAL_TAP,
    MELLO_CTX_GLOBAL_RESET,
    MELLO_CTX_SAMPLE_TOGGLE
} MelloContextVerb;

typedef struct {
    MelloContextVerb verb;
    const char *label;
} MelloContextAction;

/* WP-C5 authoritative contextual-action mapping. The physical B0-B7 slot,
 * rendered label and handler verb are defined together here. */
static const MelloContextAction context_actions[MELLO_UI_SAMPLE_RESERVED + 1u][8] = {
    [MELLO_UI_HOME] = { [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"} },
    [MELLO_UI_SEQ_STEP] = { [4] = {MELLO_CTX_STEP_SKIP,"SKIP"}, [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"} },
    [MELLO_UI_SEQ_SEGMENT] = { [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"} },
    [MELLO_UI_SEQ_LOOP] = { [2] = {MELLO_CTX_LOOP_RESET,"RST"}, [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"} },
    [MELLO_UI_SEQ_SLOTSEQ] = { [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"} },
    [MELLO_UI_RANDOM] = {
        [1] = {MELLO_CTX_RANDOM_REROLL,"RND"},
        [3] = {MELLO_CTX_RANDOM_CLEAR,"CLR2"},
        [6] = {MELLO_CTX_RANDOM_NONE,"NONE"},
        [7] = {MELLO_CTX_RANDOM_ALL,"ALL"}
    },
    [MELLO_UI_KEY_SCALE] = { [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"} },
    [MELLO_UI_SLOTS] = { [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"} },
    [MELLO_UI_DOT] = {
        [1] = {MELLO_CTX_DOT_REROLL,"RND"}, [2] = {MELLO_CTX_DOT_RESET,"RST"},
        [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"}
    },
    [MELLO_UI_TURING] = {
        [0] = {MELLO_CTX_TURING_TOGGLE,"ON"}, [1] = {MELLO_CTX_TURING_REROLL,"RND"},
        [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"}
    },
    [MELLO_UI_SUB] = {
        [0] = {MELLO_CTX_SUB_TOGGLE,"ON"}, [4] = {MELLO_CTX_SUB_STEP,"STEP"},
        [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"}
    },
#if MELLO_FEATURE_CHORD_MACHINE
    [MELLO_UI_CHORD] = { [0] = {MELLO_CTX_CHORD_TOGGLE,"ON"}, [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"} },
#endif
    [MELLO_UI_GLOBAL] = {
        [1] = {MELLO_CTX_GLOBAL_TAP,"TAP"}, [3] = {MELLO_CTX_GLOBAL_RESET,"RST2"},
        [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"}
    },
    [MELLO_UI_SOUND_RESERVED] = { [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"} },
    [MELLO_UI_AUDIO_FX] = { [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"} },
    [MELLO_UI_SAMPLE_RESERVED] = {
        [0] = {MELLO_CTX_SAMPLE_TOGGLE,"ON"}, [5] = {MELLO_CTX_OPEN_RANDOM,"RANDOM"}
    }
};

static const MelloContextAction *context_action(MelloUiPage page, uint8_t slot)
{
    if ((uint32_t)page > (uint32_t)MELLO_UI_SAMPLE_RESERVED || slot >= 8u) return 0;
    return &context_actions[(uint32_t)page][slot];
}

static void handle_context_action(MelloUi *ui, uint32_t now_ms, uint8_t action);
'''
s = s.replace(forward, mapping, 1)

handler = r'''static void handle_context_action(MelloUi *ui, uint32_t now_ms, uint8_t action)
{
    MelloEngine *e = eng(ui);
    const MelloContextAction *a = context_action(ui->page, action);
    if (!e || !a || a->verb == MELLO_CTX_NONE) return;

    switch (a->verb) {
    case MELLO_CTX_OPEN_RANDOM:
        mello_ui_set_page(ui, MELLO_UI_RANDOM);
        break;
    case MELLO_CTX_STEP_SKIP: {
        MelloStep *st = &e->project.steps[ui->selected_step];
        st->skip = !st->skip; rebuild(ui); mark_edit(ui);
        break;
    }
    case MELLO_CTX_LOOP_RESET:
        mello_engine_loop_cycle_reset_position(e); mark_edit(ui);
        break;
    case MELLO_CTX_RANDOM_REROLL:
        mello_engine_randomize_steps_intensity(e, ui->random_intensity);
        rebuild(ui); persistent_dirty(ui, now_ms); mark_edit(ui);
        break;
    case MELLO_CTX_RANDOM_CLEAR:
        if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) <= 0) {
            ui->reset_armed = false; mello_engine_clear_pattern(e);
            persistent_dirty(ui, now_ms); mark_edit(ui);
        } else {
            ui->reset_armed = true; ui->reset_confirm_until_ms = now_ms + 3000u; mark_edit(ui);
        }
        break;
    case MELLO_CTX_RANDOM_NONE:
        mello_engine_set_randomize_mask(e, 0u); mark_edit(ui);
        break;
    case MELLO_CTX_RANDOM_ALL:
        mello_engine_set_randomize_mask(e, MELLO_RAND_ALL); mark_edit(ui);
        break;
    case MELLO_CTX_DOT_REROLL:
        mello_engine_dot_randomize_cells(e, (MelloDotTarget)ui->selected_dot_target); mark_edit(ui);
        break;
    case MELLO_CTX_DOT_RESET: {
        MelloDotTarget t = (MelloDotTarget)ui->selected_dot_target;
        MelloDotMatrixConfig c = e->dot[t]; c.reset = !c.reset;
        mello_engine_dot_set_config(e, t, &c); mark_edit(ui);
        break;
    }
    case MELLO_CTX_TURING_TOGGLE: {
        MelloTmConfig c=e->tm; c.on=!c.on; mello_engine_tm_set_config(e,&c); mark_edit(ui);
        break;
    }
    case MELLO_CTX_TURING_REROLL:
        mello_engine_tm_reroll(e); mark_edit(ui);
        break;
    case MELLO_CTX_SUB_TOGGLE: {
        MelloSubConfig c=e->sub; c.on=!c.on; mello_engine_sub_set_config(e,&c); mark_edit(ui);
        break;
    }
    case MELLO_CTX_SUB_STEP:
        if (ui->selected_sub_x_step < MELLO_SUB_X_MAX_STEPS) {
            e->sub.x_steps[ui->selected_sub_x_step].on = !e->sub.x_steps[ui->selected_sub_x_step].on;
            mark_edit(ui);
        }
        break;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_CTX_CHORD_TOGGLE: {
        MelloChordConfig c=e->chord; c.on=!c.on; mello_engine_chord_set_config(e,&c); mark_edit(ui);
        break;
    }
#endif
    case MELLO_CTX_GLOBAL_TAP:
        tap_tempo(ui, now_ms);
        break;
    case MELLO_CTX_GLOBAL_RESET:
        if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) <= 0) {
            factory_reset_ui(ui, now_ms);
        } else {
            ui->reset_armed = true; ui->reset_confirm_until_ms = now_ms + 3000u; mark_edit(ui);
        }
        break;
    case MELLO_CTX_SAMPLE_TOGGLE: {
        MelloSampleConfig c=e->sample; c.on=!c.on; mello_engine_sample_set_config(e,&c); mark_edit(ui);
        break;
    }
    case MELLO_CTX_NONE:
    default:
        break;
    }
}
'''
s = replace_function(s, 'static void handle_context_action(MelloUi *ui, uint32_t now_ms, uint8_t action)\n', handler)

renderer = r'''static void draw_control_hint(MelloUi *ui, const MelloUiDrawOps *o)
{
    char b[64];
    char *p = b;
    uint8_t i;
    b[0] = 0;
    for (i = 0u; i < 8u; ++i) {
        const MelloContextAction *a = context_action(ui->page, i);
        char key[4];
        if (!a || a->verb == MELLO_CTX_NONE || !a->label) continue;
        if (p != b) p = copy_s(p, " ");
        key[0] = 'B'; key[1] = (char)('0' + i); key[2] = ':'; key[3] = 0;
        p = copy_s(p, key);
        p = copy_s(p, a->label);
    }
    o->fill(o->ctx,0,172,240,16,UI_PANEL2);
    o->text(o->ctx,4,172,232,0,b,UI_SOFT,0);
}
'''
s = replace_function(s, 'static void draw_control_hint(MelloUi *ui, const MelloUiDrawOps *o)\n', renderer)

for marker in (
    'WP-C5 authoritative contextual-action mapping',
    'const MelloContextAction *a = context_action(ui->page, action);',
    'const MelloContextAction *a = context_action(ui->page, i);',
):
    if marker not in s:
        raise SystemExit(f'FAIL: WP-C5 missing postcondition {marker!r}')

# No page switch may remain in the contextual handler: page/slot knowledge now
# belongs to the descriptor table. Action implementations dispatch by verb.
start = s.index('static void handle_context_action(MelloUi *ui, uint32_t now_ms, uint8_t action)\n{')
end = s.index('\n}\n', start) + 3
if 'switch (ui->page)' in s[start:end]:
    raise SystemExit('FAIL: WP-C5 handler still duplicates page mapping')

UI.write_text(s)
print(f'PASS: WP-C5 authoritative contextual action mapping applied; UI_SHA={sha(UI)}')
