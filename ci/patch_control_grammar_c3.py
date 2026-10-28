#!/usr/bin/env python3
from pathlib import Path
import hashlib

UI = Path('mello/mello_ui.c')
PRE_UI_SHA = '3da137c8faa5800bcfddd8291176e77718bc320d890f20f9f9d0c9cba4aaffb0'


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


actual = sha(UI)
if actual != PRE_UI_SHA:
    raise SystemExit(f'FAIL: WP-C3 UI base hash {actual} != {PRE_UI_SHA}')

s = UI.read_text()

# Retire the implementation-stage C2 name now that the action rail has more verbs.
if s.count('handle_context_action_c2') != 3:
    raise SystemExit(f'FAIL: WP-C3 expected 3 C2 context-handler references, found {s.count("handle_context_action_c2")}')
s = s.replace('handle_context_action_c2', 'handle_context_action')

# C3 adds B4 = selected-position state toggle and B5 = global RANDOM navigation.
old_switch = '''    /* Stable contextual verb slots introduced by WP-C2:\n     * B0 state toggle, B1 trigger/reroll/tap, B2 reset, B3 destructive. */\n    switch (ui->page) {\n    case MELLO_UI_SEQ_LOOP:\n'''
new_switch = '''    /* Stable contextual verb slots:\n     * B0 state toggle, B1 trigger/reroll/tap, B2 reset, B3 destructive,\n     * B4 selected-position toggle, B5 open RANDOM. */\n    if (action == 5u && ui->page != MELLO_UI_RANDOM) {\n        mello_ui_set_page(ui, MELLO_UI_RANDOM);\n        return;\n    }\n\n    switch (ui->page) {\n    case MELLO_UI_SEQ_STEP:\n        if (action == 4u) {\n            MelloStep *st = &e->project.steps[ui->selected_step];\n            st->skip = !st->skip;\n            rebuild(ui);\n            mark_edit(ui);\n        }\n        break;\n    case MELLO_UI_SEQ_LOOP:\n'''
if s.count(old_switch) != 1:
    raise SystemExit(f'FAIL: WP-C3 context switch marker count {s.count(old_switch)}')
s = s.replace(old_switch, new_switch, 1)

old_dot = '''    case MELLO_UI_DOT:\n        if (action == 1u) { mello_engine_dot_randomize_cells(e, (MelloDotTarget)ui->selected_dot_target); mark_edit(ui); }\n        break;\n'''
new_dot = '''    case MELLO_UI_DOT:\n        if (action == 1u) {\n            mello_engine_dot_randomize_cells(e, (MelloDotTarget)ui->selected_dot_target);\n            mark_edit(ui);\n        } else if (action == 2u) {\n            MelloDotTarget t = (MelloDotTarget)ui->selected_dot_target;\n            MelloDotMatrixConfig c = e->dot[t];\n            c.reset = !c.reset;\n            mello_engine_dot_set_config(e, t, &c);\n            mark_edit(ui);\n        }\n        break;\n'''
if s.count(old_dot) != 1:
    raise SystemExit(f'FAIL: WP-C3 DOT action marker count {s.count(old_dot)}')
s = s.replace(old_dot, new_dot, 1)

old_sub = '''    case MELLO_UI_SUB:\n        if (action == 0u) { MelloSubConfig c=e->sub; c.on=!c.on; mello_engine_sub_set_config(e,&c); mark_edit(ui); }\n        break;\n'''
new_sub = '''    case MELLO_UI_SUB:\n        if (action == 0u) {\n            MelloSubConfig c=e->sub; c.on=!c.on; mello_engine_sub_set_config(e,&c); mark_edit(ui);\n        } else if (action == 4u && ui->selected_sub_x_step < MELLO_SUB_X_MAX_STEPS) {\n            e->sub.x_steps[ui->selected_sub_x_step].on = !e->sub.x_steps[ui->selected_sub_x_step].on;\n            mark_edit(ui);\n        }\n        break;\n'''
if s.count(old_sub) != 1:
    raise SystemExit(f'FAIL: WP-C3 SUB action marker count {s.count(old_sub)}')
s = s.replace(old_sub, new_sub, 1)

# EDIT+white-key was hidden command grammar. White keys now only select position;
# B4 performs the explicit action on the selected position.
old_step = '''    case MELLO_UI_SEQ_STEP:\n        set_selected_step_from_note(ui, note_index);\n        if (held_buttons & bit_u32(MELLO_UI_BTN_EDIT)) {\n            MelloStep *st = &e->project.steps[ui->selected_step];\n            st->skip = !st->skip;\n            rebuild(ui);\n            mark_edit(ui);\n        }\n        break;\n'''
new_step = '''    case MELLO_UI_SEQ_STEP:\n        set_selected_step_from_note(ui, note_index);\n        break;\n'''
if s.count(old_step) != 1:
    raise SystemExit(f'FAIL: WP-C3 STEP hidden EDIT marker count {s.count(old_step)}')
s = s.replace(old_step, new_step, 1)

old_sub_note = '''    case MELLO_UI_SUB:\n        if ((uint8_t)w < MELLO_SUB_X_MAX_STEPS) {\n            ui->selected_sub_x_step = (uint8_t)w;\n            if (held_buttons & bit_u32(MELLO_UI_BTN_EDIT)) {\n                e->sub.x_steps[ui->selected_sub_x_step].on = !e->sub.x_steps[ui->selected_sub_x_step].on;\n            }\n            mark_edit(ui);\n        }\n        break;\n'''
new_sub_note = '''    case MELLO_UI_SUB:\n        if ((uint8_t)w < MELLO_SUB_X_MAX_STEPS) {\n            ui->selected_sub_x_step = (uint8_t)w;\n            mark_edit(ui);\n        }\n        break;\n'''
if s.count(old_sub_note) != 1:
    raise SystemExit(f'FAIL: WP-C3 SUB hidden EDIT marker count {s.count(old_sub_note)}')
s = s.replace(old_sub_note, new_sub_note, 1)

# EDIT remains only where it is genuinely edit/focus/lock state.
old_edit = '''    if (pressed & bit_u32(MELLO_UI_BTN_EDIT)) {\n        if (ui->page == MELLO_UI_DOT) {\n            MelloDotTarget t = (MelloDotTarget)ui->selected_dot_target;\n            MelloDotMatrixConfig c = e->dot[t]; c.reset = !c.reset; mello_engine_dot_set_config(e, t, &c); mark_edit(ui);\n        } else if (ui->page == MELLO_UI_TURING) {\n            MelloTmConfig c = e->tm; c.locked = !c.locked; mello_engine_tm_set_config(e, &c); mark_edit(ui);\n        } else if (ui->page == MELLO_UI_SAMPLE_RESERVED) {\n            MelloSampleConfig c=e->sample; c.per_step=!c.per_step; mello_engine_sample_set_config(e,&c); mark_edit(ui);\n        } else mello_ui_set_page(ui, MELLO_UI_RANDOM);\n    }\n'''
new_edit = '''    if (pressed & bit_u32(MELLO_UI_BTN_EDIT)) {\n        if (ui->page == MELLO_UI_TURING) {\n            MelloTmConfig c = e->tm; c.locked = !c.locked; mello_engine_tm_set_config(e, &c); mark_edit(ui);\n        } else if (ui->page == MELLO_UI_SAMPLE_RESERVED) {\n            MelloSampleConfig c=e->sample; c.per_step=!c.per_step; mello_engine_sample_set_config(e,&c); mark_edit(ui);\n        }\n    }\n'''
if s.count(old_edit) != 1:
    raise SystemExit(f'FAIL: WP-C3 EDIT block marker count {s.count(old_edit)}')
s = s.replace(old_edit, new_edit, 1)

for forbidden in (
    'held_buttons & bit_u32(MELLO_UI_BTN_EDIT)',
    'else mello_ui_set_page(ui, MELLO_UI_RANDOM);',
    'ui->page == MELLO_UI_DOT) {\n            MelloDotTarget t = (MelloDotTarget)ui->selected_dot_target;\n            MelloDotMatrixConfig c = e->dot[t]; c.reset',
):
    if forbidden in s:
        raise SystemExit(f'FAIL: WP-C3 legacy EDIT grammar remains: {forbidden!r}')
for required in (
    'B4 selected-position toggle, B5 open RANDOM',
    'if (action == 5u && ui->page != MELLO_UI_RANDOM)',
    'case MELLO_UI_SEQ_STEP:',
    'action == 4u && ui->selected_sub_x_step < MELLO_SUB_X_MAX_STEPS',
    'else if (action == 2u)',
    'if (ui->page == MELLO_UI_TURING)',
    'ui->page == MELLO_UI_SAMPLE_RESERVED',
):
    if required not in s:
        raise SystemExit(f'FAIL: WP-C3 postcondition marker missing: {required!r}')

UI.write_text(s)
post = sha(UI)
print(f'PASS: WP-C3 EDIT cleanup + selected-position contextual actions applied; UI_SHA={post}')
