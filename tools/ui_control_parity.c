#include "mello_ui.h"

#include <stdio.h>

static unsigned midi_calls;
static unsigned event_calls;

static void midi_send(void *ctx, uint8_t status, uint8_t d1, uint8_t d2)
{
    (void)ctx; (void)status; (void)d1; (void)d2;
    ++midi_calls;
}

static void event_sink(void *ctx, const MelloEvent *event, uint32_t due_us)
{
    (void)ctx; (void)event; (void)due_us;
    ++event_calls;
}

static int canonical_step(const MelloStep *s, uint8_t index)
{
    return s->pitch == (int8_t)(index % 7u) && s->pulses == 1u && s->ratchet == 1u &&
           s->gate == MELLO_GATE_SINGLE && s->octave == 0 && !s->slide && !s->skip &&
           !s->accent && !s->sub && s->sample_slot == 1u && s->probability == 65535u;
}

static int canonical_loop_step(const MelloLoopCycleStep *s)
{
    return s->hold == 16u && s->root_offset == 0 && s->root_note == -1 && s->scale_set == -1 &&
           s->octave == 0 && s->pitch == 0 && s->sub_tone == MELLO_SUB_TONE_ROOT;
}

static void input(MelloUi *ui, uint32_t ms, uint32_t pressed, uint32_t held)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    mello_ui_input(ui, ms * 1000u, ms, pressed, 0u, held, 0u, enc);
}

int main(void)
{
    MelloProject project;
    MelloTransport transport;
    MelloStorage storage;
    MelloUi ui;
    MelloSampleBank bank;
    uint8_t dummy[4][1] = {{0},{0},{0},{0}};
    uint8_t i;

    mello_project_defaults(&project);
    mello_transport_init(&transport, &project, 123u, midi_send, (void *)0x1234);
    mello_transport_set_event_sink(&transport, event_sink, (void *)0x5678);
    mello_storage_init(&storage, 0, 0);
    mello_ui_init(&ui, &transport, &storage);
    mello_sample_bank_init(&bank, dummy[0], dummy[1], dummy[2], dummy[3], 1u);
    mello_ui_attach_sample_bank(&ui, &bank);

    /* Source CLEAR: main steps + Loop Cycle step records/position only. */
    transport.engine.project.seq_len = 20u;
    transport.engine.project.bpm = 137u;
    transport.engine.project.root_note = 67u;
    transport.engine.project.scale = MELLO_SCALE_DORIAN;
    transport.engine.project.seg_count = 3u;
    transport.engine.project.master_len = 64u;
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        transport.engine.project.steps[i].pitch = 9;
        transport.engine.project.steps[i].pulses = 4u;
        transport.engine.project.steps[i].accent = true;
        transport.engine.project.steps[i].sample_slot = 3u;
    }
    transport.engine.loop_cycle.active = true;
    transport.engine.loop_cycle.sync = false;
    transport.engine.loop_cycle.length = 6u;
    for (i = 0u; i < MELLO_MAX_LOOP_CYCLE_STEPS; ++i) {
        transport.engine.loop_cycle.steps[i].hold = 32u;
        transport.engine.loop_cycle.steps[i].root_offset = 7;
        transport.engine.loop_cycle.steps[i].pitch = -5;
    }
    transport.engine.loop_cycle_pos = 4u;
    transport.engine.loop_cycle_tick = 9u;
    transport.engine.loop_cycle_count = 12u;
    transport.engine.randomize.mask = MELLO_RAND_PITCH | MELLO_RAND_GATE;
    transport.engine.tm.on = true;
    transport.engine.dot[0].cells = 0xA55Au;
    transport.engine.slot_seq.active = true;
    transport.engine.slot_seq.length = 8u;
    transport.engine.slot_seq.steps[0].slot = 3;
    if (!mello_engine_save_slot(&transport.engine, 2u, "KEEP")) return 1;

    mello_ui_set_page(&ui, MELLO_UI_RANDOM);
    input(&ui, 100u, (1u << MELLO_UI_BTN_REC), (1u << MELLO_UI_BTN_EDIT));
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        if (!canonical_step(&transport.engine.project.steps[i], i)) {
            fprintf(stderr, "CLEAR step %u failed\n", (unsigned)i);
            return 2;
        }
    }
    for (i = 0u; i < MELLO_MAX_LOOP_CYCLE_STEPS; ++i) {
        if (!canonical_loop_step(&transport.engine.loop_cycle.steps[i])) {
            fprintf(stderr, "CLEAR loop %u failed\n", (unsigned)i);
            return 3;
        }
    }
    if (transport.engine.loop_cycle_pos || transport.engine.loop_cycle_tick || transport.engine.loop_cycle_count) return 4;
    if (!transport.engine.loop_cycle.active || transport.engine.loop_cycle.sync || transport.engine.loop_cycle.length != 6u) return 5;
    if (transport.engine.project.seq_len != 20u || transport.engine.project.bpm != 137u ||
        transport.engine.project.root_note != 67u || transport.engine.project.scale != MELLO_SCALE_DORIAN ||
        transport.engine.project.seg_count != 3u || transport.engine.project.master_len != 64u) return 6;
    if (transport.engine.randomize.mask != (MELLO_RAND_PITCH | MELLO_RAND_GATE) ||
        !transport.engine.tm.on || transport.engine.dot[0].cells != 0xA55Au) return 7;
    if (!transport.engine.slot_seq.active || transport.engine.slot_seq.length != 8u ||
        transport.engine.slot_seq.steps[0].slot != 3 || !mello_engine_slot_valid(&transport.engine, 2u)) return 8;
    if (!storage.dirty) return 9;

    /* Exact source TAP: average adjacent intervals, latest eight taps, 3 s freshness. */
    storage.dirty = false;
    mello_ui_set_page(&ui, MELLO_UI_GLOBAL);
    input(&ui, 1000u, (1u << MELLO_UI_BTN_REC), (1u << MELLO_UI_BTN_REC));
    if (transport.engine.project.bpm != 137u || ui.tap_count != 1u) return 10;
    input(&ui, 1500u, (1u << MELLO_UI_BTN_REC), (1u << MELLO_UI_BTN_REC));
    if (transport.engine.project.bpm != 120u || ui.tap_count != 2u || !storage.dirty) return 11;
    input(&ui, 2000u, (1u << MELLO_UI_BTN_REC), (1u << MELLO_UI_BTN_REC));
    input(&ui, 2500u, (1u << MELLO_UI_BTN_REC), (1u << MELLO_UI_BTN_REC));
    if (transport.engine.project.bpm != 120u || ui.tap_count != 4u) return 12;
    input(&ui, 6000u, (1u << MELLO_UI_BTN_REC), (1u << MELLO_UI_BTN_REC));
    if (ui.tap_count != 1u || transport.engine.project.bpm != 120u) return 13;
    input(&ui, 6600u, (1u << MELLO_UI_BTN_REC), (1u << MELLO_UI_BTN_REC));
    if (transport.engine.project.bpm != 100u) return 14;

    /* RESET ALL: one chord arms, a second chord within 3 s confirms. */
    transport.engine.project.seq_len = 24u;
    transport.engine.project.bpm = 199u;
    transport.engine.project.root_note = 72u;
    transport.engine.sound.master_level = 123u;
    transport.engine.slot_seq.active = true;
    (void)mello_engine_save_slot(&transport.engine, 5u, "WIPE");
    mello_transport_set_sync(&transport, MELLO_SYNC_MASTER, 7000000u);
    mello_transport_set_midi_channel(&transport, 9u);
    mello_transport_set_midi_sub_channel(&transport, 10u);
    mello_transport_set_midi_note_out(&transport, true);
    mello_transport_start(&transport, 7000000u);
    storage.dirty = false;
    storage.manual_pending = false;
    mello_ui_set_page(&ui, MELLO_UI_GLOBAL);

    input(&ui, 7100u, (1u << MELLO_UI_BTN_OCT_DOWN),
          (1u << MELLO_UI_BTN_OCT_DOWN) | (1u << MELLO_UI_BTN_OCT_UP));
    if (!ui.reset_armed || transport.engine.project.seq_len != 24u || !transport.running) return 15;
    input(&ui, 8000u, (1u << MELLO_UI_BTN_OCT_UP),
          (1u << MELLO_UI_BTN_OCT_DOWN) | (1u << MELLO_UI_BTN_OCT_UP));
    if (transport.running || ui.page != MELLO_UI_HOME || ui.reset_armed) return 16;
    if (transport.engine.project.seq_len != 16u || transport.engine.project.bpm != 80u ||
        transport.engine.project.root_note != 60u || transport.engine.project.scale != MELLO_SCALE_MINOR) return 17;
    if (transport.sync_mode != MELLO_SYNC_OFF || transport.midi_channel != 0u ||
        transport.midi_sub_channel != 1u || transport.midi_note_out_enabled) return 18;
    if (transport.engine.slot_seq.active || mello_engine_slot_valid(&transport.engine, 5u) ||
        transport.engine.randomize.mask != MELLO_RAND_ALL) return 19;
    if (transport.midi_send != midi_send || transport.midi_ctx != (void *)0x1234 ||
        transport.event_sink != event_sink || transport.event_sink_ctx != (void *)0x5678) return 20;
    if (ui.sample_bank != &bank || !storage.dirty || !storage.manual_pending) return 21;

    /* An expired confirmation chord re-arms instead of resetting. */
    transport.engine.project.bpm = 111u;
    mello_ui_set_page(&ui, MELLO_UI_GLOBAL);
    input(&ui, 10000u, (1u << MELLO_UI_BTN_OCT_DOWN),
          (1u << MELLO_UI_BTN_OCT_DOWN) | (1u << MELLO_UI_BTN_OCT_UP));
    if (!ui.reset_armed) return 22;
    input(&ui, 13101u, (1u << MELLO_UI_BTN_OCT_UP),
          (1u << MELLO_UI_BTN_OCT_DOWN) | (1u << MELLO_UI_BTN_OCT_UP));
    if (!ui.reset_armed || transport.engine.project.bpm != 111u) return 23;

    puts("PASS: source-parity CLEAR, TAP tempo, and confirmed RESET ALL behavior");
    return 0;
}
