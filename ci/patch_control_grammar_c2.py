#!/usr/bin/env python3
from pathlib import Path
import hashlib

UI = Path('mello/mello_ui.c')
PRE_UI_SHA = 'b5824ffc4341f69547a63d5e0f281a2c19db920334b0cc5b8829bef844c7911d'
POST_UI_SHA = '3da137c8faa5800bcfddd8291176e77718bc320d890f20f9f9d0c9cba4aaffb0'


def sha(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


if sha(UI) != PRE_UI_SHA:
    raise SystemExit(f'FAIL: WP-C2 UI base hash {sha(UI)} != {PRE_UI_SHA}')

s = UI.read_text()

replacements = (
    ('    case MELLO_UI_SAMPLE_RESERVED: return MELLO_UI_BTN_REC;\n',
     '    case MELLO_UI_SAMPLE_RESERVED: return MELLO_UI_BTN_ENV;\n'),
    ('    if (pressed & bit_u32(MELLO_UI_BTN_ENV)) mello_ui_set_page(ui, MELLO_UI_SOUND_RESERVED);\n',
     '    if (pressed & bit_u32(MELLO_UI_BTN_ENV)) {\n'
     '        if (ui->page == MELLO_UI_SOUND_RESERVED) mello_ui_set_page(ui, MELLO_UI_SAMPLE_RESERVED);\n'
     '        else if (ui->page == MELLO_UI_SAMPLE_RESERVED) mello_ui_set_page(ui, MELLO_UI_SOUND_RESERVED);\n'
     '        else mello_ui_set_page(ui, MELLO_UI_SOUND_RESERVED);\n'
     '    }\n'),
)
for old, new in replacements:
    if s.count(old) != 1:
        raise SystemExit(f'FAIL: WP-C2 expected exactly one marker {old!r}, found {s.count(old)}')
    s = s.replace(old, new, 1)

note_marker = 'static void handle_note_press(MelloUi *ui, uint32_t now_ms, uint32_t held_buttons, uint8_t note_index)\n'
if s.count(note_marker) != 1:
    raise SystemExit('FAIL: WP-C2 note handler marker')
pre_note = '''static int8_t black_key_position(uint8_t note_index)\n{
    uint8_t i;\n    int8_t pos = 0;\n    for (i = 0u; i < 27u; ++i) {\n        if (mello_ui_white_key_position(i) >= 0) continue;\n        if (i == note_index) return pos;\n        ++pos;\n    }\n    return -1;\n}\n\nstatic void handle_context_action_c2(MelloUi *ui, uint32_t now_ms, uint8_t action);\n\n'''
s = s.replace(note_marker, pre_note + note_marker, 1)

manual_marker = '''    if (ui->page == MELLO_UI_SEQ_STEP && (held_buttons & bit_u32(MELLO_UI_BTN_SEL))) {\n        set_pitch_from_key(ui, note_index);\n        return;\n    }\n\n    w = mello_ui_white_key_position(note_index);\n'''
manual_new = '''    if (ui->page == MELLO_UI_SEQ_STEP && (held_buttons & bit_u32(MELLO_UI_BTN_SEL))) {\n        set_pitch_from_key(ui, note_index);\n        return;\n    }\n\n    {\n        int8_t b = black_key_position(note_index);\n        if (b >= 0) {\n            handle_context_action_c2(ui, now_ms, (uint8_t)b);\n            return;\n        }\n    }\n\n    w = mello_ui_white_key_position(note_index);\n'''
if s.count(manual_marker) != 1:
    raise SystemExit('FAIL: WP-C2 manual-note insertion marker')
s = s.replace(manual_marker, manual_new, 1)

buttons_marker = 'static void handle_buttons(MelloUi *ui, uint32_t now_us, uint32_t now_ms, uint32_t pressed, uint32_t held)\n'
if s.count(buttons_marker) != 1:
    raise SystemExit('FAIL: WP-C2 button handler marker')
context_impl = '''static void handle_context_action_c2(MelloUi *ui, uint32_t now_ms, uint8_t action)\n{
    MelloEngine *e = eng(ui);\n    if (!e) return;\n\n    /* Stable contextual verb slots introduced by WP-C2:\n     * B0 state toggle, B1 trigger/reroll/tap, B2 reset, B3 destructive. */\n    switch (ui->page) {\n    case MELLO_UI_SEQ_LOOP:\n        if (action == 2u) { mello_engine_loop_cycle_reset_position(e); mark_edit(ui); }\n        break;\n    case MELLO_UI_RANDOM:\n        if (action == 1u) {\n            mello_engine_randomize_steps_intensity(e, ui->random_intensity);\n            rebuild(ui); persistent_dirty(ui, now_ms); mark_edit(ui);\n        } else if (action == 3u) {\n            if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) <= 0) {\n                ui->reset_armed = false;\n                mello_engine_clear_pattern(e);\n                persistent_dirty(ui, now_ms); mark_edit(ui);\n            } else {\n                ui->reset_armed = true;\n                ui->reset_confirm_until_ms = now_ms + 3000u;\n                mark_edit(ui);\n            }\n        }\n        break;\n    case MELLO_UI_DOT:\n        if (action == 1u) { mello_engine_dot_randomize_cells(e, (MelloDotTarget)ui->selected_dot_target); mark_edit(ui); }\n        break;\n    case MELLO_UI_TURING:\n        if (action == 1u) { mello_engine_tm_reroll(e); mark_edit(ui); }\n        break;\n    case MELLO_UI_SUB:\n        if (action == 0u) { MelloSubConfig c=e->sub; c.on=!c.on; mello_engine_sub_set_config(e,&c); mark_edit(ui); }\n        break;\n#if MELLO_FEATURE_CHORD_MACHINE\n    case MELLO_UI_CHORD:\n        if (action == 0u) { MelloChordConfig c=e->chord; c.on=!c.on; mello_engine_chord_set_config(e,&c); mark_edit(ui); }\n        break;\n#endif\n    case MELLO_UI_GLOBAL:\n        if (action == 1u) tap_tempo(ui, now_ms);\n        break;\n    case MELLO_UI_SAMPLE_RESERVED:\n        if (action == 0u) { MelloSampleConfig c=e->sample; c.on=!c.on; mello_engine_sample_set_config(e,&c); mark_edit(ui); }\n        break;\n    default:\n        break;\n    }\n}\n\n'''
s = s.replace(buttons_marker, context_impl + buttons_marker, 1)

rec_block = '''    if (pressed & bit_u32(MELLO_UI_BTN_REC)) {\n        if (ui->page == MELLO_UI_RANDOM && (held & bit_u32(MELLO_UI_BTN_EDIT))) { mello_engine_clear_pattern(e); persistent_dirty(ui, now_ms); mark_edit(ui); }\n        else if (ui->page == MELLO_UI_RANDOM) { mello_engine_randomize_steps_intensity(e, ui->random_intensity); rebuild(ui); persistent_dirty(ui, now_ms); mark_edit(ui); }\n        else if (ui->page == MELLO_UI_GLOBAL) { tap_tempo(ui, now_ms); }\n        else if (ui->page == MELLO_UI_SEQ_LOOP) { mello_engine_loop_cycle_reset_position(e); mark_edit(ui); }\n        else if (ui->page == MELLO_UI_DOT) { mello_engine_dot_randomize_cells(e, (MelloDotTarget)ui->selected_dot_target); mark_edit(ui); }\n        else if (ui->page == MELLO_UI_TURING) { mello_engine_tm_reroll(e); mark_edit(ui); }\n        else if (ui->page == MELLO_UI_SUB) { MelloSubConfig c=e->sub;c.on=!c.on;mello_engine_sub_set_config(e,&c);mark_edit(ui); }\n#if MELLO_FEATURE_CHORD_MACHINE\n        else if (ui->page == MELLO_UI_CHORD) { MelloChordConfig c=e->chord;c.on=!c.on;mello_engine_chord_set_config(e,&c);mark_edit(ui); }\n#endif\n        else if (ui->page == MELLO_UI_SAMPLE_RESERVED) { MelloSampleConfig c=e->sample; c.on=!c.on; mello_engine_sample_set_config(e,&c); mark_edit(ui); }\n        else mello_ui_set_page(ui, MELLO_UI_SAMPLE_RESERVED);\n    }\n'''
if s.count(rec_block) != 1:
    raise SystemExit(f'FAIL: WP-C2 REC block marker count {s.count(rec_block)}')
s = s.replace(rec_block, '    /* REC is reserved for actual record/capture. No capture path exists yet. */\n', 1)

UI.write_text(s)
post = sha(UI)
for marker in (
    'handle_context_action_c2',
    'REC is reserved for actual record/capture',
    'case MELLO_UI_SAMPLE_RESERVED: return MELLO_UI_BTN_ENV;',
    'if (ui->page == MELLO_UI_SOUND_RESERVED) mello_ui_set_page(ui, MELLO_UI_SAMPLE_RESERVED);',
):
    if marker not in s:
        raise SystemExit(f'FAIL: WP-C2 postcondition marker {marker!r}')
if 'if (pressed & bit_u32(MELLO_UI_BTN_REC))' in s:
    raise SystemExit('FAIL: WP-C2 legacy REC handler remains')
if post != POST_UI_SHA:
    raise SystemExit(f'FAIL: WP-C2 final UI hash {post} != {POST_UI_SHA}')
print(f'PASS: WP-C2 REC cleanup + contextual action migration applied; UI_SHA={post}')
