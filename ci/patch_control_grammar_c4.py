#!/usr/bin/env python3
from pathlib import Path
import hashlib

UI = Path('mello/mello_ui.c')
ROTARY = Path('tools/rotary_mapping_parity.c')
PRE_UI_SHA = 'c811c6ee8e5d6422a479452c65e0c4720d93d0b979cba3b4bcd1281f95a162da'


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


actual = sha(UI)
if actual != PRE_UI_SHA:
    raise SystemExit(f'FAIL: WP-C4 UI base hash {actual} != {PRE_UI_SHA}')

s = UI.read_text()

old_page_button = '''static uint8_t page_button(MelloUiPage page)\n{\n    switch (page) {\n    case MELLO_UI_SEQ_STEP:\n    case MELLO_UI_SEQ_SEGMENT:\n    case MELLO_UI_SEQ_LOOP:\n    case MELLO_UI_SEQ_SLOTSEQ: return MELLO_UI_BTN_SEQ;\n    case MELLO_UI_RANDOM: return MELLO_UI_BTN_EDIT;\n    case MELLO_UI_KEY_SCALE: return MELLO_UI_BTN_SEL;\n    case MELLO_UI_SLOTS: return MELLO_UI_BTN_SAVE;\n    case MELLO_UI_DOT: return MELLO_UI_BTN_FX;\n    case MELLO_UI_TURING: return MELLO_UI_BTN_LFO;\n    case MELLO_UI_SUB: return MELLO_UI_BTN_ARP;\n#if MELLO_FEATURE_CHORD_MACHINE\n    case MELLO_UI_CHORD: return MELLO_UI_BTN_ARP;\n#endif\n    case MELLO_UI_GLOBAL: return MELLO_UI_BTN_GLO;\n    case MELLO_UI_SOUND_RESERVED: return MELLO_UI_BTN_ENV;\n    case MELLO_UI_AUDIO_FX: return MELLO_UI_BTN_FX;\n    case MELLO_UI_SAMPLE_RESERVED: return MELLO_UI_BTN_ENV;\n    case MELLO_UI_HOME:\n    default: return MELLO_UI_BTN_HOME;\n    }\n}\n'''
new_page_button = '''static uint8_t page_button(MelloUi *ui)\n{\n    switch (ui->page) {\n    case MELLO_UI_SEQ_STEP:\n    case MELLO_UI_SEQ_SEGMENT:\n    case MELLO_UI_SEQ_LOOP:\n    case MELLO_UI_SEQ_SLOTSEQ: return MELLO_UI_BTN_SEQ;\n    case MELLO_UI_RANDOM: return 0xFFu;\n    case MELLO_UI_KEY_SCALE: return MELLO_UI_BTN_SEL;\n    case MELLO_UI_SLOTS: return MELLO_UI_BTN_SAVE;\n    case MELLO_UI_DOT:\n    case MELLO_UI_TURING: return MELLO_UI_BTN_LFO;\n    case MELLO_UI_SUB: return MELLO_UI_BTN_ARP;\n#if MELLO_FEATURE_CHORD_MACHINE\n    case MELLO_UI_CHORD: return MELLO_UI_BTN_ARP;\n#endif\n    case MELLO_UI_GLOBAL: return MELLO_UI_BTN_GLO;\n    case MELLO_UI_SOUND_RESERVED: return ui->sound_subpage == 2u ? MELLO_UI_BTN_LFO : MELLO_UI_BTN_ENV;\n    case MELLO_UI_AUDIO_FX: return MELLO_UI_BTN_FX;\n    case MELLO_UI_SAMPLE_RESERVED: return MELLO_UI_BTN_ENV;\n    case MELLO_UI_HOME:\n    default: return MELLO_UI_BTN_HOME;\n    }\n}\n'''
if s.count(old_page_button) != 1:
    raise SystemExit(f'FAIL: WP-C4 page_button marker count {s.count(old_page_button)}')
s = s.replace(old_page_button, new_page_button, 1)

old_loop_k4 = '        else if (knob == 3u) mello_engine_loop_cycle_reset_position(e);\n'
new_loop_k4 = '        else if (knob == 3u) return;\n'
if s.count(old_loop_k4) != 1:
    raise SystemExit(f'FAIL: WP-C4 LOOP K4 handler marker count {s.count(old_loop_k4)}')
s = s.replace(old_loop_k4, new_loop_k4, 1)

old_loop_footer = 'l[3]="RESET";copy_s(v[3],"NOW");'
new_loop_footer = 'l[3]="K4";copy_s(v[3],"-");'
if s.count(old_loop_footer) != 1:
    raise SystemExit(f'FAIL: WP-C4 LOOP K4 footer marker count {s.count(old_loop_footer)}')
s = s.replace(old_loop_footer, new_loop_footer, 1)

old_comment = '''    /* Stable contextual verb slots:\n     * B0 state toggle, B1 trigger/reroll/tap, B2 reset, B3 destructive,\n     * B4 selected-position toggle, B5 open RANDOM. */\n'''
new_comment = '''    /* Stable contextual verb slots:\n     * B0 state toggle, B1 trigger/reroll/tap, B2 reset, B3 destructive,\n     * B4 selected-position toggle, B5 open RANDOM, B6/B7 page-specific pair. */\n'''
if s.count(old_comment) != 1:
    raise SystemExit('FAIL: WP-C4 context comment marker')
s = s.replace(old_comment, new_comment, 1)

old_random = '''        } else if (action == 3u) {\n            if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) <= 0) {\n                ui->reset_armed = false;\n                mello_engine_clear_pattern(e);\n                persistent_dirty(ui, now_ms); mark_edit(ui);\n            } else {\n                ui->reset_armed = true;\n                ui->reset_confirm_until_ms = now_ms + 3000u;\n                mark_edit(ui);\n            }\n        }\n        break;\n'''
new_random = '''        } else if (action == 3u) {\n            if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) <= 0) {\n                ui->reset_armed = false;\n                mello_engine_clear_pattern(e);\n                persistent_dirty(ui, now_ms); mark_edit(ui);\n            } else {\n                ui->reset_armed = true;\n                ui->reset_confirm_until_ms = now_ms + 3000u;\n                mark_edit(ui);\n            }\n        } else if (action == 6u) {\n            mello_engine_set_randomize_mask(e, 0u);\n            mark_edit(ui);\n        } else if (action == 7u) {\n            mello_engine_set_randomize_mask(e, MELLO_RAND_ALL);\n            mark_edit(ui);\n        }\n        break;\n'''
if s.count(old_random) != 1:
    raise SystemExit(f'FAIL: WP-C4 RANDOM action marker count {s.count(old_random)}')
s = s.replace(old_random, new_random, 1)

old_turing = '''    case MELLO_UI_TURING:\n        if (action == 1u) { mello_engine_tm_reroll(e); mark_edit(ui); }\n        break;\n'''
new_turing = '''    case MELLO_UI_TURING:\n        if (action == 0u) { MelloTmConfig c=e->tm; c.on=!c.on; mello_engine_tm_set_config(e,&c); mark_edit(ui); }\n        else if (action == 1u) { mello_engine_tm_reroll(e); mark_edit(ui); }\n        break;\n'''
if s.count(old_turing) != 1:
    raise SystemExit(f'FAIL: WP-C4 TURING action marker count {s.count(old_turing)}')
s = s.replace(old_turing, new_turing, 1)

old_global = '''    case MELLO_UI_GLOBAL:\n        if (action == 1u) tap_tempo(ui, now_ms);\n        break;\n'''
new_global = '''    case MELLO_UI_GLOBAL:\n        if (action == 1u) tap_tempo(ui, now_ms);\n        else if (action == 3u) {\n            if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) <= 0) {\n                factory_reset_ui(ui, now_ms);\n            } else {\n                ui->reset_armed = true;\n                ui->reset_confirm_until_ms = now_ms + 3000u;\n                mark_edit(ui);\n            }\n        }\n        break;\n'''
if s.count(old_global) != 1:
    raise SystemExit(f'FAIL: WP-C4 GLOBAL action marker count {s.count(old_global)}')
s = s.replace(old_global, new_global, 1)

old_reset_head = '''    MelloEngine *e = eng(ui);\n    const uint32_t reset_combo = bit_u32(MELLO_UI_BTN_OCT_DOWN) | bit_u32(MELLO_UI_BTN_OCT_UP);\n    if (!e) return;\n\n    if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) > 0) {\n        ui->reset_armed = false;\n        mark_edit(ui);\n    }\n    if (ui->page == MELLO_UI_GLOBAL && (held & reset_combo) == reset_combo && (pressed & reset_combo)) {\n        if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) <= 0) {\n            factory_reset_ui(ui, now_ms);\n            return;\n        }\n        ui->reset_armed = true;\n        ui->reset_confirm_until_ms = now_ms + 3000u;\n        mark_edit(ui);\n        return;\n    } else if (ui->reset_armed && pressed) {\n        ui->reset_armed = false;\n        mark_edit(ui);\n    }\n'''
new_reset_head = '''    MelloEngine *e = eng(ui);\n    if (!e) return;\n\n    if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) > 0) {\n        ui->reset_armed = false;\n        mark_edit(ui);\n    }\n    if (ui->reset_armed && pressed) {\n        ui->reset_armed = false;\n        mark_edit(ui);\n    }\n'''
if s.count(old_reset_head) != 1:
    raise SystemExit(f'FAIL: WP-C4 hidden GLOBAL OCT reset marker count {s.count(old_reset_head)}')
s = s.replace(old_reset_head, new_reset_head, 1)

old_fx = '''    if (pressed & bit_u32(MELLO_UI_BTN_FX)) {\n        if (ui->page == MELLO_UI_DOT) mello_ui_set_page(ui, MELLO_UI_AUDIO_FX);\n        else if (ui->page == MELLO_UI_AUDIO_FX) mello_ui_set_page(ui, MELLO_UI_DOT);\n        else mello_ui_set_page(ui, MELLO_UI_DOT);\n    }\n'''
new_fx = '''    if (pressed & bit_u32(MELLO_UI_BTN_FX)) mello_ui_set_page(ui, MELLO_UI_AUDIO_FX);\n'''
if s.count(old_fx) != 1:
    raise SystemExit(f'FAIL: WP-C4 FX navigation marker count {s.count(old_fx)}')
s = s.replace(old_fx, new_fx, 1)

old_lfo = '''    if (pressed & bit_u32(MELLO_UI_BTN_LFO)) {\n        if (ui->page == MELLO_UI_TURING) { MelloTmConfig c = e->tm; c.on = !c.on; mello_engine_tm_set_config(e, &c); mark_edit(ui); }\n        else mello_ui_set_page(ui, MELLO_UI_TURING);\n    }\n'''
new_lfo = '''    if (pressed & bit_u32(MELLO_UI_BTN_LFO)) {\n        if (ui->page == MELLO_UI_SOUND_RESERVED && ui->sound_subpage == 2u) {\n            mello_ui_set_page(ui, MELLO_UI_DOT);\n        } else if (ui->page == MELLO_UI_DOT) {\n            mello_ui_set_page(ui, MELLO_UI_TURING);\n        } else {\n            ui->sound_subpage = 2u;\n            if (ui->page == MELLO_UI_SOUND_RESERVED) mark_full(ui);\n            else mello_ui_set_page(ui, MELLO_UI_SOUND_RESERVED);\n        }\n    }\n'''
if s.count(old_lfo) != 1:
    raise SystemExit(f'FAIL: WP-C4 LFO/MOD navigation marker count {s.count(old_lfo)}')
s = s.replace(old_lfo, new_lfo, 1)

old_oct_down = '''    if (pressed & bit_u32(MELLO_UI_BTN_OCT_DOWN)) {\n        if (ui->page == MELLO_UI_RANDOM) mello_engine_set_randomize_mask(e, 0u);\n        else if (ui->page == MELLO_UI_HOME || ui->page == MELLO_UI_SEQ_STEP) ui->step_bank = 0u;\n        else if (ui->page == MELLO_UI_SEQ_SLOTSEQ && ui->slot_seq_bank > 0u) --ui->slot_seq_bank;\n        else if (ui->page == MELLO_UI_SLOTS && ui->slot_bank > 0u) --ui->slot_bank;\n        else if (ui->page == MELLO_UI_TURING) ui->tm_bank = 0u;\n        mark_edit(ui);\n    }\n'''
new_oct_down = '''    if (pressed & bit_u32(MELLO_UI_BTN_OCT_DOWN)) {\n        if (ui->page == MELLO_UI_HOME || ui->page == MELLO_UI_SEQ_STEP) { ui->step_bank = 0u; mark_edit(ui); }\n        else if (ui->page == MELLO_UI_SEQ_SLOTSEQ && ui->slot_seq_bank > 0u) { --ui->slot_seq_bank; mark_edit(ui); }\n        else if (ui->page == MELLO_UI_SLOTS && ui->slot_bank > 0u) { --ui->slot_bank; mark_edit(ui); }\n        else if (ui->page == MELLO_UI_TURING) { ui->tm_bank = 0u; mark_edit(ui); }\n    }\n'''
if s.count(old_oct_down) != 1:
    raise SystemExit(f'FAIL: WP-C4 OCT DOWN marker count {s.count(old_oct_down)}')
s = s.replace(old_oct_down, new_oct_down, 1)

old_oct_up = '''    if (pressed & bit_u32(MELLO_UI_BTN_OCT_UP)) {\n        if (ui->page == MELLO_UI_RANDOM) mello_engine_set_randomize_mask(e, MELLO_RAND_ALL);\n        else if (ui->page == MELLO_UI_HOME || ui->page == MELLO_UI_SEQ_STEP) ui->step_bank = 1u;\n        else if (ui->page == MELLO_UI_SEQ_SLOTSEQ && ui->slot_seq_bank < 3u) ++ui->slot_seq_bank;\n        else if (ui->page == MELLO_UI_SLOTS && ui->slot_bank < 3u) ++ui->slot_bank;\n        else if (ui->page == MELLO_UI_TURING) ui->tm_bank = 1u;\n        mark_edit(ui);\n    }\n'''
new_oct_up = '''    if (pressed & bit_u32(MELLO_UI_BTN_OCT_UP)) {\n        if (ui->page == MELLO_UI_HOME || ui->page == MELLO_UI_SEQ_STEP) { ui->step_bank = 1u; mark_edit(ui); }\n        else if (ui->page == MELLO_UI_SEQ_SLOTSEQ && ui->slot_seq_bank < 3u) { ++ui->slot_seq_bank; mark_edit(ui); }\n        else if (ui->page == MELLO_UI_SLOTS && ui->slot_bank < 3u) { ++ui->slot_bank; mark_edit(ui); }\n        else if (ui->page == MELLO_UI_TURING) { ui->tm_bank = 1u; mark_edit(ui); }\n    }\n'''
if s.count(old_oct_up) != 1:
    raise SystemExit(f'FAIL: WP-C4 OCT UP marker count {s.count(old_oct_up)}')
s = s.replace(old_oct_up, new_oct_up, 1)

old_hint = '''static void draw_control_hint(MelloUi *ui, const MelloUiDrawOps *o)\n{\n    char b[48], *p = b; const char *fn = function_name(ui);\n    b[0] = 0;\n    switch (ui->page) {\n    case MELLO_UI_HOME: p=copy_s(p,"KEY STEP | OCT BANK"); break;\n    case MELLO_UI_SEQ_STEP: p=copy_s(p,"KEY STEP | SEL "); p=copy_s(p,fn); p=copy_s(p," | OCT"); break;\n    case MELLO_UI_SEQ_SEGMENT: p=copy_s(p,"KEY SEG | SEL "); p=copy_s(p,fn); break;\n    case MELLO_UI_SEQ_LOOP: p=copy_s(p,"KEY LOOP | SEL "); p=copy_s(p,fn); p=copy_s(p," | RST"); break;\n    case MELLO_UI_SEQ_SLOTSEQ: p=copy_s(p,"KEY STEP | OCT BANK"); break;\n    case MELLO_UI_RANDOM: p=copy_s(p,"KEY TARGET | REC RND | OCT"); break;\n    case MELLO_UI_SLOTS: p=copy_s(p,"KEY SLOT | SEL "); p=copy_s(p,fn); p=copy_s(p," | OCT"); break;\n    case MELLO_UI_DOT: p=copy_s(p,"KEY CELL | SEL "); p=copy_s(p,fn); break;\n    case MELLO_UI_TURING: p=copy_s(p,"KEY BIT | SEL "); p=copy_s(p,fn); p=copy_s(p," | OCT"); break;\n    case MELLO_UI_SUB: p=copy_s(p,"KEY 5SEQ | SEL "); p=copy_s(p,fn); break;\n#if MELLO_FEATURE_CHORD_MACHINE\n    case MELLO_UI_CHORD: p=copy_s(p,ui->chord_subpage==3u?"KEY GRID | SEL ":ui->chord_subpage==4u?"KEY PAT | SEL ":"KEY CHAIN | SEL "); p=copy_s(p,fn); break;\n#endif\n    case MELLO_UI_SAMPLE_RESERVED: p=copy_s(p,"KEY A-D | SEL "); p=copy_s(p,fn); break;\n    case MELLO_UI_GLOBAL: case MELLO_UI_SOUND_RESERVED: case MELLO_UI_AUDIO_FX:\n        p=copy_s(p,"SEL FN "); p=copy_s(p,fn); break;\n    default: break;\n    }\n    o->fill(o->ctx,0,172,240,16,UI_PANEL2);\n    o->text(o->ctx,4,172,232,0,b,UI_SOFT,0);\n}\n'''
new_hint = '''static void draw_control_hint(MelloUi *ui, const MelloUiDrawOps *o)\n{\n    const char *b = "B5:RANDOM";\n    switch (ui->page) {\n    case MELLO_UI_SEQ_STEP: b="B4:SKIP B5:RANDOM"; break;\n    case MELLO_UI_SEQ_LOOP: b="B2:RST B5:RANDOM"; break;\n    case MELLO_UI_RANDOM: b="B1:RND B3:CLR2 B6:NONE B7:ALL"; break;\n    case MELLO_UI_DOT: b="B1:RND B2:RST B5:RANDOM"; break;\n    case MELLO_UI_TURING: b="B0:ON B1:RND B5:RANDOM"; break;\n    case MELLO_UI_SUB: b="B0:ON B4:STEP B5:RANDOM"; break;\n#if MELLO_FEATURE_CHORD_MACHINE\n    case MELLO_UI_CHORD: b="B0:ON B5:RANDOM"; break;\n#endif\n    case MELLO_UI_GLOBAL: b="B1:TAP B3:RST2 B5:RANDOM"; break;\n    case MELLO_UI_SAMPLE_RESERVED: b="B0:ON B5:RANDOM"; break;\n    default: break;\n    }\n    o->fill(o->ctx,0,172,240,16,UI_PANEL2);\n    o->text(o->ctx,4,172,232,0,b,UI_SOFT,0);\n}\n'''
if s.count(old_hint) != 1:
    raise SystemExit(f'FAIL: WP-C4 control-hint marker count {s.count(old_hint)}')
s = s.replace(old_hint, new_hint, 1)

old_leds = '''static uint32_t calc_button_leds(MelloUi *ui)\n{\n    uint32_t m = bit_u32(page_button(ui->page));\n    MelloEngine *e = eng(ui);\n    if (ui->transport && ui->transport->running) m |= bit_u32(MELLO_UI_BTN_PLAY);\n    if (ui->page == MELLO_UI_TURING && e && e->tm.on) m |= bit_u32(MELLO_UI_BTN_LFO);\n    if (ui->page == MELLO_UI_SUB && e && e->sub.on) m |= bit_u32(MELLO_UI_BTN_ARP);\n#if MELLO_FEATURE_CHORD_MACHINE\n    if (ui->page == MELLO_UI_CHORD && e && e->chord.on) m |= bit_u32(MELLO_UI_BTN_ARP);\n#endif\n    return m;\n}\n'''
new_leds = '''static uint32_t calc_button_leds(MelloUi *ui)\n{\n    uint8_t pb = page_button(ui);\n    uint32_t m = pb <= MELLO_UI_BTN_REC ? bit_u32(pb) : 0u;\n    MelloEngine *e = eng(ui);\n    if (ui->transport && ui->transport->running) m |= bit_u32(MELLO_UI_BTN_PLAY);\n    if (ui->page == MELLO_UI_SUB && e && e->sub.on) m |= bit_u32(MELLO_UI_BTN_ARP);\n#if MELLO_FEATURE_CHORD_MACHINE\n    if (ui->page == MELLO_UI_CHORD && e && e->chord.on) m |= bit_u32(MELLO_UI_BTN_ARP);\n#endif\n    return m;\n}\n'''
if s.count(old_leds) != 1:
    raise SystemExit(f'FAIL: WP-C4 LED marker count {s.count(old_leds)}')
s = s.replace(old_leds, new_leds, 1)

for forbidden in (
    'reset_combo',
    'ui->page == MELLO_UI_RANDOM) mello_engine_set_randomize_mask',
    'mello_engine_loop_cycle_reset_position(e);\n    }\n    mark_edit(ui);',
    'if (ui->page == MELLO_UI_TURING) { MelloTmConfig c = e->tm; c.on = !c.on;',
    'case MELLO_UI_RANDOM: return MELLO_UI_BTN_EDIT;',
    'case MELLO_UI_DOT: return MELLO_UI_BTN_FX;',
    'KEY TARGET | REC RND | OCT',
):
    if forbidden in s:
        raise SystemExit(f'FAIL: WP-C4 legacy grammar remains: {forbidden!r}')
for required in (
    'B6/B7 page-specific pair',
    'action == 6u', 'action == 7u',
    'case MELLO_UI_TURING:\n        if (action == 0u)',
    'else if (action == 3u)',
    'ui->sound_subpage = 2u;',
    'B1:RND B3:CLR2 B6:NONE B7:ALL',
    'uint32_t m = pb <= MELLO_UI_BTN_REC ? bit_u32(pb) : 0u;',
):
    if required not in s:
        raise SystemExit(f'FAIL: WP-C4 postcondition marker missing: {required!r}')

UI.write_text(s)

r = ROTARY.read_text()
old_rot = 'C("LOOP LOOP",MELLO_UI_SEQ_LOOP,2,15,8,"LEN","SYNC","ON","RESET"),'
new_rot = 'C("LOOP LOOP",MELLO_UI_SEQ_LOOP,2,7,0,"LEN","SYNC","ON","K4"),'
if r.count(old_rot) != 1:
    raise SystemExit(f'FAIL: WP-C4 rotary LOOP fixture marker count {r.count(old_rot)}')
r = r.replace(old_rot, new_rot, 1)
ROTARY.write_text(r)

print(f'PASS: WP-C4 contextual rail + MOD navigation applied; UI_SHA={sha(UI)}; ROTARY_SHA={sha(ROTARY)}')
