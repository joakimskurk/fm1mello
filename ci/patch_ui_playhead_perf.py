#!/usr/bin/env python3
from pathlib import Path

C = Path('mello/mello_ui.c')
H = Path('mello/mello_ui.h')
s = C.read_text()
h = H.read_text()

def once(text, old, new, label):
    n = text.count(old)
    if n != 1:
        raise SystemExit(f'FAIL: UI perf {label}: expected one anchor, found {n}')
    return text.replace(old, new, 1)

# One extra dirty bit for tiny playhead-only redraws outside HOME.
h = once(h,
'''    MELLO_UI_DIRTY_HOME_PLAYHEAD = 1u << 6,
    MELLO_UI_DIRTY_ALL    = 0x7Fu
''',
'''    MELLO_UI_DIRTY_HOME_PLAYHEAD = 1u << 6,
    MELLO_UI_DIRTY_SEQ_PLAYHEAD  = 1u << 7,
    MELLO_UI_DIRTY_ALL    = 0xFFu
''', 'dirty bit')

old_tick = '''    uint8_t run, step_changed, other_changed;
    if (!ui || !ui->transport) return;
    e = &ui->transport->engine;
    run = ui->transport->running ? 1u : 0u;
    step_changed = ui->seen_step != ui->transport->display_step;
    other_changed = ui->seen_pulse != ui->transport->display_pulse ||
        ui->seen_segment != ui->transport->display_segment || ui->seen_loop_pos != e->loop_cycle_pos ||
        ui->seen_slot_seq_pos != e->slot_seq_pos || ui->seen_tm_pos != e->tm_active_index ||
        ui->seen_sub_x_pos != e->sub_x_active_index ||
#if MELLO_FEATURE_CHORD_MACHINE
        ui->seen_chord_pos != e->chord_chain_pos || ui->seen_chord_grid != e->chord_grid_active_cell ||
#endif
        ui->seen_running != run;
    if (step_changed || other_changed) {
        if (ui->page == MELLO_UI_HOME && step_changed) {
            ui->home_old_step = ui->seen_step;
            ui->home_new_step = ui->transport->display_step;
            ui->dirty |= MELLO_UI_DIRTY_HOME_PLAYHEAD;
        } else if (ui->page != MELLO_UI_HOME) {
            ui->dirty |= MELLO_UI_DIRTY_BODY;
        }
        ui->seen_step = ui->transport->display_step;
        ui->seen_pulse = ui->transport->display_pulse;
        ui->seen_segment = ui->transport->display_segment;
        ui->seen_loop_pos = e->loop_cycle_pos;
        ui->seen_slot_seq_pos = e->slot_seq_pos;
        ui->seen_tm_pos = e->tm_active_index;
        ui->seen_sub_x_pos = e->sub_x_active_index;
#if MELLO_FEATURE_CHORD_MACHINE
        ui->seen_chord_pos=e->chord_chain_pos;ui->seen_chord_grid=e->chord_grid_active_cell;
#endif
        ui->seen_running = run;
        ui->dirty |= MELLO_UI_DIRTY_STATUS | MELLO_UI_DIRTY_LEDS;
    }
'''
new_tick = '''    uint8_t run, step_changed, other_changed, dynamic_body = 1u;
    if (!ui || !ui->transport) return;
    e = &ui->transport->engine;
    run = ui->transport->running ? 1u : 0u;
    step_changed = ui->seen_step != ui->transport->display_step;
    other_changed = ui->seen_pulse != ui->transport->display_pulse ||
        ui->seen_segment != ui->transport->display_segment || ui->seen_loop_pos != e->loop_cycle_pos ||
        ui->seen_slot_seq_pos != e->slot_seq_pos || ui->seen_tm_pos != e->tm_active_index ||
        ui->seen_sub_x_pos != e->sub_x_active_index ||
#if MELLO_FEATURE_CHORD_MACHINE
        ui->seen_chord_pos != e->chord_chain_pos || ui->seen_chord_grid != e->chord_grid_active_cell ||
#endif
        ui->seen_running != run;

    /* Static/edit pages do not depend on the transport cursor. Do not redraw
       their 142-pixel body merely because playback advanced elsewhere. */
    switch (ui->page) {
    case MELLO_UI_HOME:
    case MELLO_UI_SEQ_STEP:
    case MELLO_UI_RANDOM:
    case MELLO_UI_KEY_SCALE:
    case MELLO_UI_SLOTS:
    case MELLO_UI_DOT:
    case MELLO_UI_GLOBAL:
    case MELLO_UI_SOUND_RESERVED:
    case MELLO_UI_AUDIO_FX:
    case MELLO_UI_SAMPLE_RESERVED:
        dynamic_body = 0u;
        break;
    default:
        break;
    }

    if (step_changed || other_changed) {
        if (step_changed) {
            ui->home_old_step = ui->seen_step;
            ui->home_new_step = ui->transport->display_step;
            if (ui->page == MELLO_UI_HOME) ui->dirty |= MELLO_UI_DIRTY_HOME_PLAYHEAD;
            else if (ui->page == MELLO_UI_SEQ_STEP || ui->page == MELLO_UI_RANDOM)
                ui->dirty |= MELLO_UI_DIRTY_SEQ_PLAYHEAD;
            else if (dynamic_body) ui->dirty |= MELLO_UI_DIRTY_BODY;
        }
        if (other_changed && dynamic_body) ui->dirty |= MELLO_UI_DIRTY_BODY;
        ui->seen_step = ui->transport->display_step;
        ui->seen_pulse = ui->transport->display_pulse;
        ui->seen_segment = ui->transport->display_segment;
        ui->seen_loop_pos = e->loop_cycle_pos;
        ui->seen_slot_seq_pos = e->slot_seq_pos;
        ui->seen_tm_pos = e->tm_active_index;
        ui->seen_sub_x_pos = e->sub_x_active_index;
#if MELLO_FEATURE_CHORD_MACHINE
        ui->seen_chord_pos=e->chord_chain_pos;ui->seen_chord_grid=e->chord_grid_active_cell;
#endif
        ui->seen_running = run;
        ui->dirty |= MELLO_UI_DIRTY_STATUS | MELLO_UI_DIRTY_LEDS;
    }
'''
s = once(s, old_tick, new_tick, 'tick dispatch')

old_strip = '''static void draw_sequence_overview_strip(MelloUi *ui, const MelloUiDrawOps *o, int16_t y, int16_t h)
{
    MelloEngine *e = eng(ui);
    uint8_t i, n;
    if (!e) return;
    n = ui_step_overview_count(&e->project);
    for (i = 0u; i < n; ++i) {
        int16_t x = ui_step_overview_x(&e->project, i), w = ui_step_overview_w(&e->project, i);
        uint16_t c = UI_PANEL2;
        if (i < e->project.seq_len) {
            uint8_t seg = ui_segment_for_step(&e->project, i);
            c = e->project.steps[i].skip ? ui_segment_colour_dim(seg) : ui_segment_colour(seg);
        }
        o->fill(o->ctx, x, y, w, h, i < e->project.seq_len ? c : UI_PANEL);
        if (ui->transport->display_step == i) ui_frame(o, x, y - 1, w, h + 2, UI_WHITE);
        else if (ui->selected_step == i) ui_frame(o, x, y - 1, w, h + 2, UI_SOFT);
    }
}
'''
new_strip = '''static void draw_sequence_overview_cell(MelloUi *ui, const MelloUiDrawOps *o, int16_t y, int16_t h, uint8_t i, uint8_t clear_bg)
{
    MelloEngine *e = eng(ui);
    int16_t x, w;
    uint16_t c = UI_PANEL2;
    if (!e || i >= ui_step_overview_count(&e->project)) return;
    x = ui_step_overview_x(&e->project, i); w = ui_step_overview_w(&e->project, i);
    if (clear_bg) o->fill(o->ctx, x, y - 1, w, h + 2, UI_BLACK);
    if (i < e->project.seq_len) {
        uint8_t seg = ui_segment_for_step(&e->project, i);
        c = e->project.steps[i].skip ? ui_segment_colour_dim(seg) : ui_segment_colour(seg);
    }
    o->fill(o->ctx, x, y, w, h, i < e->project.seq_len ? c : UI_PANEL);
    if (ui->transport->display_step == i) ui_frame(o, x, y - 1, w, h + 2, UI_WHITE);
    else if (ui->selected_step == i) ui_frame(o, x, y - 1, w, h + 2, UI_SOFT);
}

static void draw_sequence_overview_strip(MelloUi *ui, const MelloUiDrawOps *o, int16_t y, int16_t h)
{
    MelloEngine *e = eng(ui);
    uint8_t i, n;
    if (!e) return;
    n = ui_step_overview_count(&e->project);
    for (i = 0u; i < n; ++i) draw_sequence_overview_cell(ui, o, y, h, i, 0u);
}

static void draw_step_overview_cell(MelloUi *ui, const MelloUiDrawOps *o, uint8_t i, uint8_t clear_bg)
{
    MelloEngine *e = eng(ui);
    int16_t x, cw;
    uint16_t c;
    if (!e || i >= ui_step_overview_count(&e->project)) return;
    cw = ui_step_overview_w(&e->project, i);
    x = ui_step_overview_x(&e->project, i);
    if (clear_bg) o->fill(o->ctx, x, 35, cw, 12, UI_BLACK);
    c = i < e->project.seq_len ? UI_PANEL2 : UI_BLACK;
    o->fill(o->ctx, x, 36, cw, 10, c);
    if (i < e->project.seq_len) {
        uint8_t seg = ui_segment_for_step(&e->project, i);
        o->fill(o->ctx, x, 42, cw, 2, e->project.steps[i].skip ? ui_segment_colour_dim(seg) : ui_segment_colour(seg));
    }
    if (i == ui->selected_step) ui_frame(o, x, 35, cw, 12, UI_SOFT);
    if (ui->transport->display_step == i) ui_frame(o, x, 36, cw, 10, UI_WHITE);
}
'''
s = once(s, old_strip, new_strip, 'overview helper')

old_step_loop = '''        /* Full-sequence strip: always show every main step; OCT only changes which 16 keys edit. */
        for (i = 0u; i < vis; ++i) {
            uint8_t si = i; int16_t cw = ui_step_overview_w(&e->project, i);
            uint16_t c = si < e->project.seq_len ? UI_PANEL2 : UI_BLACK;
            x = ui_step_overview_x(&e->project, i);
            o->fill(o->ctx, x, 36, cw, 10, c);
            if (si < e->project.seq_len) { uint8_t seg = ui_segment_for_step(&e->project, si); o->fill(o->ctx, x, 42, cw, 2, e->project.steps[si].skip ? ui_segment_colour_dim(seg) : ui_segment_colour(seg)); }
            if (si == ui->selected_step) ui_frame(o, x, 35, cw, 12, UI_SOFT);
            if (ui->transport->display_step == si) ui_frame(o, x, 36, cw, 10, UI_WHITE);
        }
'''
new_step_loop = '''        /* Full-sequence strip: always show every main step; OCT only changes which 16 keys edit. */
        for (i = 0u; i < vis; ++i) draw_step_overview_cell(ui, o, i, 0u);
'''
s = once(s, old_step_loop, new_step_loop, 'STEP full overview loop')

old_home_ph = '''static void draw_home_playhead(MelloUi *ui, const MelloUiDrawOps *o)
{
    if (ui->home_old_step < MELLO_MAX_STEPS) draw_home_step(ui, o, ui->home_old_step);
    if (ui->home_new_step < MELLO_MAX_STEPS && ui->home_new_step != ui->home_old_step) draw_home_step(ui, o, ui->home_new_step);
}
'''
new_home_ph = '''static void draw_home_playhead(MelloUi *ui, const MelloUiDrawOps *o)
{
    if (ui->home_old_step < MELLO_MAX_STEPS) draw_home_step(ui, o, ui->home_old_step);
    if (ui->home_new_step < MELLO_MAX_STEPS && ui->home_new_step != ui->home_old_step) draw_home_step(ui, o, ui->home_new_step);
}

static void draw_sequence_playhead(MelloUi *ui, const MelloUiDrawOps *o)
{
    uint8_t old_step = ui->home_old_step, new_step = ui->home_new_step;
    if (ui->page == MELLO_UI_SEQ_STEP) {
        if (old_step < MELLO_MAX_STEPS) draw_step_overview_cell(ui, o, old_step, 1u);
        if (new_step < MELLO_MAX_STEPS && new_step != old_step) draw_step_overview_cell(ui, o, new_step, 1u);
    } else if (ui->page == MELLO_UI_RANDOM) {
        if (old_step < MELLO_MAX_STEPS) draw_sequence_overview_cell(ui, o, 166, 4, old_step, 1u);
        if (new_step < MELLO_MAX_STEPS && new_step != old_step) draw_sequence_overview_cell(ui, o, 166, 4, new_step, 1u);
    }
}
'''
s = once(s, old_home_ph, new_home_ph, 'playhead delta helper')

old_render = '''    if (d & MELLO_UI_DIRTY_HEADER) draw_header(ui, ops);
    if (d & MELLO_UI_DIRTY_BODY) draw_body(ui, ops);
    else if ((d & MELLO_UI_DIRTY_HOME_PLAYHEAD) && ui->page == MELLO_UI_HOME) draw_home_playhead(ui, ops);
    if (d & MELLO_UI_DIRTY_FOOTER) draw_footer(ui, ops);
'''
new_render = '''    if (d & MELLO_UI_DIRTY_HEADER) draw_header(ui, ops);
    if (d & MELLO_UI_DIRTY_BODY) draw_body(ui, ops);
    else if ((d & MELLO_UI_DIRTY_HOME_PLAYHEAD) && ui->page == MELLO_UI_HOME) draw_home_playhead(ui, ops);
    else if ((d & MELLO_UI_DIRTY_SEQ_PLAYHEAD) && (ui->page == MELLO_UI_SEQ_STEP || ui->page == MELLO_UI_RANDOM)) draw_sequence_playhead(ui, ops);
    if (d & MELLO_UI_DIRTY_FOOTER) draw_footer(ui, ops);
'''
s = once(s, old_render, new_render, 'render dispatch')

for token in ('MELLO_UI_DIRTY_SEQ_PLAYHEAD','draw_step_overview_cell','draw_sequence_playhead','dynamic_body'):
    if token not in s and token not in h:
        raise SystemExit(f'FAIL: UI perf postcondition missing {token}')

C.write_text(s)
H.write_text(h)
print('PASS: UI playback redraws are page-aware; STEP/RANDOM use two-cell playhead deltas and static pages avoid body redraws')
