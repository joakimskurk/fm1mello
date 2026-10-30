#include "mello_ui.h"
#include <stdio.h>
#include <string.h>

static void send_input(MelloUi *ui, uint32_t note_mask, int enc_id, int delta)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    if (enc_id >= 0 && enc_id < (int)MELLO_UI_ENCODER_COUNT) enc[enc_id] = (int16_t)delta;
    mello_ui_input(ui, 1000u, 1u, 0u, 0u, 0u, note_mask, enc);
}

static void key(MelloUi *ui, uint8_t white)
{
    uint8_t note = mello_ui_white_key_note_index(white);
    send_input(ui, 1u << note, -1, 0);
}

static void enc(MelloUi *ui, uint8_t id, int delta)
{
    send_input(ui, 0u, id, delta);
}

static int capture_cycle_notes(int8_t bass_transpose, uint8_t *lead_out, uint8_t *sub_out)
{
    MelloProject p;
    MelloEngine e;
    MelloEvent ev;
    int got_lead = 0, got_sub = 0;
    mello_project_defaults(&p);
    p.steps[0].sub = true;
    p.steps[0].gate = MELLO_GATE_SINGLE;
    mello_engine_init(&e, &p, 77u);
    e.sub.on = true;
    e.sub.mode = MELLO_SUB_MODE_STEP;
    e.loop_cycle.active = false;
    mello_loop_cycle_set_sub(&e.loop_cycle.steps[0], MELLO_SUB_TONE_ROOT, bass_transpose);
    mello_engine_loop_cycle_select(&e, 0u);
    mello_engine_start(&e);
    if (!mello_engine_advance_pulse(&e)) return 0;
    while (mello_event_pop(&e, &ev)) {
        if (!got_lead && ev.type == MELLO_EVENT_NOTE_ON) {
            *lead_out = ev.note;
            got_lead = 1;
        }
        if (!got_sub && ev.type == MELLO_EVENT_SUB_NOTE_ON) {
            *sub_out = ev.note;
            got_sub = 1;
        }
    }
    return got_lead && got_sub;
}

int main(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi ui;
    uint8_t before, bank;
    uint16_t gate;
    int8_t bass_tr;
    uint8_t lead0, sub0, lead7, sub7;

    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 123u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&ui, &t, &st);

    /* HOME uses the white keys as steps and K1-K4 as immediate musical edits. */
    mello_ui_set_page(&ui, MELLO_UI_HOME);
    key(&ui, 5u);
    if (ui.selected_step != 5u) return 1;
    before = (uint8_t)t.engine.project.steps[5].pitch;
    enc(&ui, MELLO_UI_ENC_KNOB1, +1);
    if (t.engine.project.steps[5].pitch == (int8_t)before) return 2;
    enc(&ui, MELLO_UI_ENC_SELECT, +1);
    if (ui.selected_step != 5u) return 3;

    /* STEP selection is keys-only; SELECT changes function layer. */
    mello_ui_set_page(&ui, MELLO_UI_SEQ_STEP);
    ui.step_subpage = 0u;
    key(&ui, 2u);
    if (ui.selected_step != 2u) return 4;
    enc(&ui, MELLO_UI_ENC_SELECT, +1);
    if (ui.selected_step != 2u || ui.step_subpage != 1u) return 5;
    ui.step_subpage = 3u;
    ui.step_bank = 0u;
    before = t.engine.project.pitch_span_oct;
    enc(&ui, MELLO_UI_ENC_KNOB4, +1);
    if (ui.step_bank != 0u || t.engine.project.pitch_span_oct <= before) return 6;

    /* SEGMENT: a step key cuts there and picks the new segment (A 1, B 2-4, C 5-8, D 9-16); K1 edits its repeats. */
    t.engine.project.seg_count = 3u;
    t.engine.project.seg_splits[0] = 4u;
    t.engine.project.seg_splits[1] = 8u;
    mello_ui_set_page(&ui, MELLO_UI_SEQ_SEGMENT);
    key(&ui, 1u);
    if (ui.selected_segment != 1u || t.engine.project.seg_count != 4u || t.engine.project.seg_splits[0] != 1u)
        return 7;
    before = t.engine.project.seg_repeats[1];
    enc(&ui, MELLO_UI_ENC_KNOB1, +1);
    if (ui.selected_segment != 1u || t.engine.project.seg_repeats[1] <= before) return 8;

    /* LOOP CYCLE is direct 1-8 key selection. SELECT walks STEP/HARM/LOOP. */
    mello_ui_set_page(&ui, MELLO_UI_SEQ_LOOP);
    ui.loop_subpage = 0u;
    t.engine.loop_cycle.active = false;
    key(&ui, 6u);
    if (ui.selected_loop_step != 6u || t.engine.loop_cycle_pos != 6u) return 9;
    /* Off-cycle key selection is audition: it changes harmonic context immediately. */
    mello_loop_cycle_set_sub(&t.engine.loop_cycle.steps[6], MELLO_SUB_TONE_FIFTH, +7);
    if (mello_loop_cycle_sub_tone(&t.engine.loop_cycle.steps[6]) != MELLO_SUB_TONE_FIFTH ||
        mello_loop_cycle_sub_transpose(&t.engine.loop_cycle.steps[6]) != 7)
        return 24;
    if (!capture_cycle_notes(0, &lead0, &sub0) || !capture_cycle_notes(7, &lead7, &sub7)) return 27;
    if (lead0 != lead7 || sub7 != (uint8_t)(sub0 + 7u)) return 28;
    enc(&ui, MELLO_UI_ENC_SELECT, +1);
    if (ui.selected_loop_step != 6u || ui.loop_subpage != 1u) return 10;
    bass_tr = mello_loop_cycle_sub_transpose(&t.engine.loop_cycle.steps[6]);
    enc(&ui, MELLO_UI_ENC_KNOB4, +1);
    if (mello_loop_cycle_sub_transpose(&t.engine.loop_cycle.steps[6]) != bass_tr + 1 ||
        !(!t.engine.loop_cycle.active))
        return 25;
    enc(&ui, MELLO_UI_ENC_SELECT, +1);
    if (ui.loop_subpage != 2u) return 11;
    t.engine.loop_cycle.active = true;
    t.engine.loop_cycle.length = 4u;
    t.engine.loop_cycle_pos = 2u;
    key(&ui, 6u);
    if (ui.selected_loop_step != 6u || t.engine.loop_cycle_pos != 2u) return 26;
    t.engine.loop_cycle_pos = 5u;
    t.engine.loop_cycle_tick = 9u;
    enc(&ui, MELLO_UI_ENC_KNOB4, +1);
    if (ui.selected_loop_step != 6u || t.engine.loop_cycle_pos != 5u || t.engine.loop_cycle_tick != 9u)
        return 12;

    /* SLOT SEQ, TURING and SUB positions cannot be scrolled with SELECT anymore. */
    mello_ui_set_page(&ui, MELLO_UI_SEQ_SLOTSEQ);
    key(&ui, 4u);
    if (ui.selected_slot_seq_step != 4u) return 13;
    enc(&ui, MELLO_UI_ENC_SELECT, +1);
    if (ui.selected_slot_seq_step != 4u) return 14;

    mello_ui_set_page(&ui, MELLO_UI_TURING);
    ui.tm_subpage = 0u;
    ui.tm_bank = 0u;
    ui.selected_tm_bit = 0u;
    key(&ui, 7u);
    if (ui.selected_tm_bit != 7u) return 15;
    enc(&ui, MELLO_UI_ENC_SELECT, +1);
    if (ui.selected_tm_bit != 7u || ui.tm_subpage != 1u) return 16;

    mello_ui_set_page(&ui, MELLO_UI_SUB);
    ui.sub_subpage = 2u;
    ui.selected_sub_x_step = 0u;
    key(&ui, 3u);
    if (ui.selected_sub_x_step != 3u) return 17;
    gate = t.engine.sub.gate_amount;
    enc(&ui, MELLO_UI_ENC_KNOB4, +1);
    if (ui.selected_sub_x_step != 3u || t.engine.sub.gate_amount <= gate) return 18;

#if MELLO_FEATURE_CHORD_MACHINE
    /* CHAIN/PATTERN position is keys-only; SELECT changes chord function page. */
    mello_ui_set_page(&ui, MELLO_UI_CHORD);
    {
        MelloChordConfig c = t.engine.chord;
        c.length = 4u;
        mello_engine_chord_set_config(&t.engine, &c);
    }
    ui.chord_subpage = 1u;
    ui.selected_chord_step = 0u;
    key(&ui, 2u);
    if (ui.selected_chord_step != 2u) return 19;
    enc(&ui, MELLO_UI_ENC_SELECT, +1);
    if (ui.selected_chord_step != 2u || ui.chord_subpage != 2u) return 20;
#endif

    /* SAMPLE slots are direct A-D keys; SELECT changes MAIN/PITCH/TRIM/ENV. */
    mello_ui_set_page(&ui, MELLO_UI_SAMPLE_RESERVED);
    ui.sample_subpage = 0u;
    key(&ui, 2u);
    if (t.engine.sample.edit_slot != 2u) return 21;
    enc(&ui, MELLO_UI_ENC_SELECT, +1);
    if (t.engine.sample.edit_slot != 2u || ui.sample_subpage != 1u) return 22;

    /* HOME bank is button-owned; knobs cannot move it. */
    mello_ui_set_page(&ui, MELLO_UI_HOME);
    ui.step_bank = 1u;
    bank = ui.step_bank;
    enc(&ui, MELLO_UI_ENC_KNOB4, +1);
    if (ui.step_bank != bank) return 23;

    puts("PASS: keys own step/cell selection; LOOP OFF keys audition cycle slots; bass transpose is "
         "independent; SELECT owns layers; K1-K4 stay musical");
    return 0;
}
