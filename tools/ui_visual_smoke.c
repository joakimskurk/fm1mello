#include "mello_ui.h"
#include <stdio.h>
#include <string.h>

static int fills = 0;
static int texts = 0;
static int bounds_error = 0;

static void smoke_fill(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour)
{
    (void)ctx; (void)colour;
    if (x < 0 || y < 0 || w < 0 || h < 0 || x + w > 240 || y + h > 240) {
        fprintf(stderr, "bad fill %d %d %d %d\n", x, y, w, h);
        bounds_error = 1;
    }
    ++fills;
}

static void smoke_text(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large,
                       const char *text_value, uint16_t colour, uint8_t align)
{
    (void)ctx; (void)large; (void)text_value; (void)colour; (void)align;
    if (x < 0 || y < 0 || w < 0 || x + w > 240 || y > 239) {
        fprintf(stderr, "bad text %d %d %d\n", x, y, w);
        bounds_error = 1;
    }
    ++texts;
}

static int render_page(MelloUi *ui, const MelloUiDrawOps *ops, MelloUiPage page)
{
    fills = 0;
    texts = 0;
    bounds_error = 0;
    mello_ui_set_page(ui, page);
    ui->dirty |= MELLO_UI_DIRTY_ALL;
    mello_ui_render(ui, ops);
    if (bounds_error) {
        fprintf(stderr, "page %d exceeded 240x240 bounds\n", (int)page);
        return 1;
    }
    return 0;
}

int main(void)
{
    MelloProject project;
    MelloTransport transport;
    MelloStorage storage;
    MelloUi ui;
    MelloUiDrawOps ops = { smoke_fill, smoke_text, 0, 0, 0 };
    MelloSampleBank sample_bank;
    uint8_t sample_dummy[4][1] = {{0},{0},{0},{0}};
    int16_t encoder_delta[MELLO_UI_ENCODER_COUNT] = {0};
    uint8_t i;

    mello_project_defaults(&project);
    mello_transport_init(&transport, &project, 123u, 0, 0);
    mello_storage_init(&storage, 0, 0);
    mello_ui_init(&ui, &transport, &storage);
    mello_sample_bank_init(&sample_bank, sample_dummy[0], sample_dummy[1], sample_dummy[2], sample_dummy[3], 1u);
    sample_bank.meta[0].loaded = true;
    sample_bank.meta[0].sample_count = MELLO_SAMPLE_SOURCE_RATE;
    memcpy(sample_bank.meta[0].name, "KICK", 5u);
    sample_bank.meta[1].loaded = true;
    sample_bank.meta[1].sample_count = MELLO_SAMPLE_SOURCE_RATE / 2u;
    memcpy(sample_bank.meta[1].name, "SNARE", 6u);
    mello_ui_attach_sample_bank(&ui, &sample_bank);

    if (render_page(&ui, &ops, MELLO_UI_HOME) || fills < 30 || texts < 8) return 2;

    for (i = 0u; i < 4u; ++i) {
        ui.step_subpage = i;
        if (render_page(&ui, &ops, MELLO_UI_SEQ_STEP) || fills < 50 || texts < 12) return 3;
    }

    mello_ui_set_page(&ui, MELLO_UI_SEQ_STEP);
    ui.step_subpage = 3u;
    transport.engine.project.seq_len = 16u;
    encoder_delta[MELLO_UI_ENC_KNOB1] = 1;
    mello_ui_input(&ui, 0u, 0u, 0u, 0u, 0u, 0u, encoder_delta);
    if (transport.engine.project.seq_len != 20u) {
        fprintf(stderr, "STEP length 16 -> 20 failed: %u\n", (unsigned)transport.engine.project.seq_len);
        return 21;
    }
    encoder_delta[MELLO_UI_ENC_KNOB1] = -1;
    mello_ui_input(&ui, 0u, 0u, 0u, 0u, 0u, 0u, encoder_delta);
    if (transport.engine.project.seq_len != 16u) {
        fprintf(stderr, "STEP length 20 -> 16 failed: %u\n", (unsigned)transport.engine.project.seq_len);
        return 22;
    }
    transport.engine.project.seq_len = 17u;
    encoder_delta[MELLO_UI_ENC_KNOB1] = 1;
    mello_ui_input(&ui, 0u, 0u, 0u, 0u, 0u, 0u, encoder_delta);
    if (transport.engine.project.seq_len != 20u) {
        fprintf(stderr, "STEP length legacy 17 -> 20 failed: %u\n", (unsigned)transport.engine.project.seq_len);
        return 23;
    }
    transport.engine.project.seq_len = 17u;
    encoder_delta[MELLO_UI_ENC_KNOB1] = -1;
    mello_ui_input(&ui, 0u, 0u, 0u, 0u, 0u, 0u, encoder_delta);
    if (transport.engine.project.seq_len != 16u) {
        fprintf(stderr, "STEP length legacy 17 -> 16 failed: %u\n", (unsigned)transport.engine.project.seq_len);
        return 24;
    }
    transport.engine.project.seq_len = 4u;
    encoder_delta[MELLO_UI_ENC_KNOB1] = -1;
    mello_ui_input(&ui, 0u, 0u, 0u, 0u, 0u, 0u, encoder_delta);
    if (transport.engine.project.seq_len != 4u) return 25;
    transport.engine.project.seq_len = 32u;
    encoder_delta[MELLO_UI_ENC_KNOB1] = 1;
    mello_ui_input(&ui, 0u, 0u, 0u, 0u, 0u, 0u, encoder_delta);
    if (transport.engine.project.seq_len != 32u) return 26;
    encoder_delta[MELLO_UI_ENC_KNOB1] = 0;

    transport.engine.project.seq_len = 8u;
    ui.step_bank = 0u;
    mello_ui_input(&ui, 0u, 0u, 0u, 0u, 0u, (1u << 14), encoder_delta);
    if (ui.selected_step != 8u || transport.engine.project.seq_len != 10u) {
        fprintf(stderr, "STEP select 9 should extend 8 -> 10: selected=%u len=%u\n",
                (unsigned)ui.selected_step, (unsigned)transport.engine.project.seq_len);
        return 27;
    }
    transport.engine.project.seq_len = 16u;
    ui.step_bank = 1u;
    mello_ui_input(&ui, 0u, 0u, 0u, 0u, 0u, 1u, encoder_delta);
    if (ui.selected_step != 16u || transport.engine.project.seq_len != 20u) {
        fprintf(stderr, "STEP select 17 should extend 16 -> 20: selected=%u len=%u\n",
                (unsigned)ui.selected_step, (unsigned)transport.engine.project.seq_len);
        return 28;
    }
    transport.engine.project.seq_len = 16u;
    ui.step_bank = 0u;

    for (i = 0u; i < 2u; ++i) {
        ui.segment_subpage = i;
        if (render_page(&ui, &ops, MELLO_UI_SEQ_SEGMENT) || fills < 50 || texts < 12) return 4;
    }
    for (i = 0u; i < 3u; ++i) {
        ui.loop_subpage = i;
        if (render_page(&ui, &ops, MELLO_UI_SEQ_LOOP) || fills < 45 || texts < 15) return 5;
    }
    if (render_page(&ui, &ops, MELLO_UI_SEQ_SLOTSEQ) || fills < 90 || texts < 14) return 6;
    if (render_page(&ui, &ops, MELLO_UI_RANDOM) || fills < 30 || texts < 10) return 7;

    transport.engine.randomize.mask = MELLO_RAND_PITCH | MELLO_RAND_GATE;
    mello_ui_input(&ui, 0u, 0u, (1u << MELLO_UI_BTN_OCT_DOWN), 0u, 0u, 0u, encoder_delta);
    if (transport.engine.randomize.mask != 0u) {
        fprintf(stderr, "RANDOM NONE shortcut failed: mask=%u\n", (unsigned)transport.engine.randomize.mask);
        return 19;
    }
    mello_ui_input(&ui, 0u, 0u, (1u << MELLO_UI_BTN_OCT_UP), 0u, 0u, 0u, encoder_delta);
    if (transport.engine.randomize.mask != MELLO_RAND_ALL) {
        fprintf(stderr, "RANDOM ALL shortcut failed: mask=%u\n", (unsigned)transport.engine.randomize.mask);
        return 20;
    }

    if (render_page(&ui, &ops, MELLO_UI_KEY_SCALE) || fills < 60 || texts < 12) return 8;

    for (i = 0u; i < 2u; ++i) {
        ui.slots_subpage = i;
        if (render_page(&ui, &ops, MELLO_UI_SLOTS) || fills < 80 || texts < 25) return 9;
    }
    for (i = 0u; i < 4u; ++i) {
        ui.selected_dot_target = i;
        if (render_page(&ui, &ops, MELLO_UI_DOT) || fills < 50 || texts < 8) return 10;
    }
    for (i = 0u; i < 3u; ++i) {
        ui.tm_subpage = i;
        if (render_page(&ui, &ops, MELLO_UI_TURING) || fills < 30 || texts < 12) return 11;
    }
    for (i = 0u; i < 5u; ++i) {
        ui.sub_subpage = i;
        if (render_page(&ui, &ops, MELLO_UI_SUB) || fills < 35 || texts < 18) return 12;
    }
    for (i = 0u; i < 2u; ++i) {
        ui.global_subpage = i;
        if (render_page(&ui, &ops, MELLO_UI_GLOBAL) || fills < 8 || texts < 14) return 13;
    }
    for (i = 0u; i < 5u; ++i) {
        ui.sound_subpage = i;
        if (render_page(&ui, &ops, MELLO_UI_SOUND_RESERVED) || fills < 35 || texts < 12) return 14;
    }
    for (i = 0u; i < 5u; ++i) {
        ui.fx_subpage = i;
        if (render_page(&ui, &ops, MELLO_UI_AUDIO_FX) || fills < 25 || texts < 16) return 15;
    }
    for (i = 0u; i < 4u; ++i) {
        ui.sample_subpage = i;
        if (render_page(&ui, &ops, MELLO_UI_SAMPLE_RESERVED) || fills < 35 || texts < 18) return 16;
    }

    mello_ui_set_page(&ui, MELLO_UI_HOME);
    ui.dirty |= MELLO_UI_DIRTY_ALL;
    mello_ui_render(&ui, &ops);
    transport.display_step = 1u;
    mello_ui_tick(&ui);
    fills = 0;
    texts = 0;
    bounds_error = 0;
    mello_ui_render(&ui, &ops);

    if (bounds_error) return 17;
    if (fills >= 20 || texts >= 4) {
        fprintf(stderr, "HOME playhead redraw too broad: fills=%d texts=%d\n", fills, texts);
        return 18;
    }
    printf("PASS: all visual-port pages/subpages stay inside 240x240; STEP length parity works; RANDOM ALL/NONE parity works; HOME incremental redraw fills=%d texts=%d\n", fills, texts);
    return 0;
}
