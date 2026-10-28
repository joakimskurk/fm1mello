#!/usr/bin/env python3
from pathlib import Path

PARITY = Path('tools/ui_control_parity.c')
GRAMMAR = Path('tools/control_grammar_smoke.c')
s = PARITY.read_text()

old_input = '''static void input(MelloUi *ui, uint32_t ms, uint32_t pressed, uint32_t held)\n{\n    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};\n    mello_ui_input(ui, ms * 1000u, ms, pressed, 0u, held, 0u, enc);\n}\n\n'''
if s.count(old_input) != 1:
    raise SystemExit(f'FAIL: WP-C4 parity input helper marker count {s.count(old_input)}')
s = s.replace(old_input, '', 1)

old_reset = '''    /* RESET ALL remains on its legacy confirmed OCT chord until WP-C4. */\n    transport.engine.project.seq_len = 24u;\n    transport.engine.project.bpm = 199u;\n    transport.engine.project.root_note = 72u;\n    transport.engine.sound.master_level = 123u;\n    transport.engine.slot_seq.active = true;\n    (void)mello_engine_save_slot(&transport.engine, 5u, "WIPE");\n    mello_transport_set_sync(&transport, MELLO_SYNC_MASTER, 7000000u);\n    mello_transport_set_midi_channel(&transport, 9u);\n    mello_transport_set_midi_sub_channel(&transport, 10u);\n    mello_transport_set_midi_note_out(&transport, true);\n    mello_transport_start(&transport, 7000000u);\n    storage.dirty = false;\n    storage.manual_pending = false;\n    mello_ui_set_page(&ui, MELLO_UI_GLOBAL);\n\n    input(&ui, 7100u, (1u << MELLO_UI_BTN_OCT_DOWN),\n          (1u << MELLO_UI_BTN_OCT_DOWN) | (1u << MELLO_UI_BTN_OCT_UP));\n    if (!ui.reset_armed || transport.engine.project.seq_len != 24u || !transport.running) return 16;\n    input(&ui, 8000u, (1u << MELLO_UI_BTN_OCT_UP),\n          (1u << MELLO_UI_BTN_OCT_DOWN) | (1u << MELLO_UI_BTN_OCT_UP));\n'''
new_reset = '''    /* RESET ALL moved to contextual B3 with the same two-press confirmation. */\n    transport.engine.project.seq_len = 24u;\n    transport.engine.project.bpm = 199u;\n    transport.engine.project.root_note = 72u;\n    transport.engine.sound.master_level = 123u;\n    transport.engine.slot_seq.active = true;\n    (void)mello_engine_save_slot(&transport.engine, 5u, "WIPE");\n    mello_transport_set_sync(&transport, MELLO_SYNC_MASTER, 7000000u);\n    mello_transport_set_midi_channel(&transport, 9u);\n    mello_transport_set_midi_sub_channel(&transport, 10u);\n    mello_transport_set_midi_note_out(&transport, true);\n    mello_transport_start(&transport, 7000000u);\n    storage.dirty = false;\n    storage.manual_pending = false;\n    mello_ui_set_page(&ui, MELLO_UI_GLOBAL);\n\n    black_input(&ui, 7100u, 3u);\n    if (!ui.reset_armed || transport.engine.project.seq_len != 24u || !transport.running) return 16;\n    black_input(&ui, 8000u, 3u);\n'''
if s.count(old_reset) != 1:
    raise SystemExit(f'FAIL: WP-C4 parity RESET block marker count {s.count(old_reset)}')
s = s.replace(old_reset, new_reset, 1)

old_expired = '''    /* An expired confirmation chord re-arms instead of resetting. */\n    transport.engine.project.bpm = 111u;\n    mello_ui_set_page(&ui, MELLO_UI_GLOBAL);\n    input(&ui, 10000u, (1u << MELLO_UI_BTN_OCT_DOWN),\n          (1u << MELLO_UI_BTN_OCT_DOWN) | (1u << MELLO_UI_BTN_OCT_UP));\n    if (!ui.reset_armed) return 23;\n    input(&ui, 13101u, (1u << MELLO_UI_BTN_OCT_UP),\n          (1u << MELLO_UI_BTN_OCT_DOWN) | (1u << MELLO_UI_BTN_OCT_UP));\n    if (!ui.reset_armed || transport.engine.project.bpm != 111u) return 24;\n\n    puts("PASS: CLEAR/TAP semantics survive WP-C2 contextual migration; confirmed RESET ALL legacy path remains protected");\n'''
new_expired = '''    /* An expired B3 confirmation re-arms instead of resetting. */\n    transport.engine.project.bpm = 111u;\n    mello_ui_set_page(&ui, MELLO_UI_GLOBAL);\n    black_input(&ui, 10000u, 3u);\n    if (!ui.reset_armed) return 23;\n    black_input(&ui, 13101u, 3u);\n    if (!ui.reset_armed || transport.engine.project.bpm != 111u) return 24;\n\n    puts("PASS: CLEAR/TAP/RESET ALL semantics survive contextual migration without hidden OCT reset grammar");\n'''
if s.count(old_expired) != 1:
    raise SystemExit(f'FAIL: WP-C4 parity expired-confirmation marker count {s.count(old_expired)}')
s = s.replace(old_expired, new_expired, 1)

if 'RESET ALL remains on its legacy confirmed OCT chord' in s or 'static void input(' in s:
    raise SystemExit('FAIL: WP-C4 stale OCT reset parity remains')
PARITY.write_text(s)

g = GRAMMAR.read_text()
old_loop = '''    t.engine.loop_cycle_pos = 5u;\n    t.engine.loop_cycle_tick = 9u;\n    enc(&ui, MELLO_UI_ENC_KNOB4, +1);\n    if (ui.selected_loop_step != 6u || t.engine.loop_cycle_pos != 0u || t.engine.loop_cycle_tick != 0u) return 12;\n'''
new_loop = '''    t.engine.loop_cycle_pos = 5u;\n    t.engine.loop_cycle_tick = 9u;\n    enc(&ui, MELLO_UI_ENC_KNOB4, +1);\n    if (ui.selected_loop_step != 6u || t.engine.loop_cycle_pos != 5u || t.engine.loop_cycle_tick != 9u) return 12;\n'''
if g.count(old_loop) != 1:
    raise SystemExit(f'FAIL: WP-C4 grammar LOOP K4 marker count {g.count(old_loop)}')
g = g.replace(old_loop, new_loop, 1)
GRAMMAR.write_text(g)

print('PASS: WP-C4 generated control tests migrated to B3 RESET ALL and inert LOOP K4')
