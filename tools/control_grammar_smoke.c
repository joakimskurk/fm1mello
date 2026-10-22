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

int main(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi ui;
    uint8_t before, bank;
    uint16_t gate;

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

    /* SEGMENT selected index stays on keys; STRUCT K3 edits repeats. */
    t.engine.project.seg_count = 3u;
    ui.segment_subpage = 1u;
    mello_ui_set_page(&ui, MELLO_UI_SEQ_SEGMENT);
    key(&ui, 1u);
    if (ui.selected_segment != 1u) return 7;
    before = t.engine.project.seg_repeats[1];
    enc(&ui, MELLO_UI_ENC_KNOB3, +1);
    if (ui.selected_segment != 1u || t.engine.project.seg_repeats[1] <= before) return 8;

    /* LOOP CYCLE is direct 1-8 key selection. SELECT walks STEP/HARM/LOOP. */
    mello_ui_set_page(&ui, MELLO_UI_SEQ_LOOP);
    ui.loop_subpage = 0u;
    key(&ui, 6u);
    if (ui.selected_loop_step != 6u) return 9;
    enc(&ui, MELLO_UI_ENC_SELECT, +1);
    if (ui.selected_loop_step != 6u || ui.loop_subpage != 1u) return 10;
    enc(&ui, MELLO_UI_ENC_SELECT, +1);
    if (ui.loop_subpage != 2u) return 11;
    t.engine.loop_cycle_pos = 5u;
    t.engine.loop_cycle_tick = 9u;
    enc(&ui, MELLO_UI_ENC_KNOB4, +1);
    if (ui.selected_loop_step != 6u || t.engine.loop_cycle_pos != 0u || t.engine.loop_cycle_tick != 0u) return 12;

    /* SLOT SEQ, TURING and SUB positions cannot be scrolled with SELECT anymore. */
    mello_ui_set_page(&ui, MELLO_UI_SEQ_SLOTSEQ);
    key(&ui, 4u);
    if (ui.selected_slot_seq_step != 4u) return 13;
    enc(&ui, MELLO_UI_ENC_SELECT, +1);
    if (ui.selected_slot_seq_step != 4u) return 14;

    mello_ui_set_page(&ui, MELLO_UI_TURING);
    ui.tm_subpage = 0u; ui.tm_bank = 0u; ui.selected_tm_bit = 0u;
    key(&ui, 7u);
    if (ui.selected_tm_bit != 7u) return 15;
    enc(&ui, MELLO_UI_ENC_SELECT, +1);
    if (ui.selected_tm_bit != 7u || ui.tm_subpage != 1u) return 16;

    mello_ui_set_page(&ui, MELLO_UI_SUB);
    ui.sub_subpage = 2u; ui.selected_sub_x_step = 0u;
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
    ui.chord_subpage = 1u; ui.selected_chord_step = 0u;
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

    puts("PASS: keys own step/cell selection; SELECT owns function layers; K1-K4 stay musical");
    return 0;
}
