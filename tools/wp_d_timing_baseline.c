#include "mello_engine.h"
#include "mello_transport.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void drain_events(MelloEngine *e) {
    MelloEvent ev;
    while (mello_event_pop(e, &ev)) { }
}

static MelloProject plain_project(uint8_t len, uint16_t master_len) {
    MelloProject p;
    uint8_t i;
    mello_project_defaults(&p);
    p.seq_len = len;
    p.seg_count = 1u;
    p.seg_repeats[0] = 1u;
    p.seg_direction[0] = MELLO_SEG_FWD;
    p.seg_mode[0] = MELLO_SEG_LINEAR;
    p.seg_skip[0] = false;
    p.master_len = master_len;
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        p.steps[i].pulses = 1u;
        p.steps[i].skip = false;
        p.steps[i].gate = MELLO_GATE_SINGLE;
        p.steps[i].probability = 65535u;
    }
    return p;
}

static void advance_ok(MelloEngine *e, unsigned count) {
    unsigned i;
    for (i = 0u; i < count; ++i) {
        assert(mello_engine_advance_pulse(e));
        drain_events(e);
    }
}

static void test_defaults_and_storage(void) {
    MelloProject p, decoded;
    MelloStoredProject stored;
    MelloLoopCycleConfig lc;
    MelloSlotSeqConfig slots;

    mello_project_defaults(&p);
    assert(p.seq_len == 16u);
    assert(p.clock_div == 1u);
    assert(p.master_len == 0u); /* current FREE/natural representation */

    mello_loop_cycle_defaults(&lc);
    assert(lc.sync);
    assert(lc.length == 4u);
    assert(lc.steps[0].hold == 16u);

    mello_slot_seq_defaults(&slots);
    assert(!slots.active);
    assert(slots.length == 16u);
    assert(slots.steps[0].hold == 16u);

    p.master_len = 128u;
    mello_stored_project_encode(&stored, &p);
    assert(stored.master_len_lo == 128u);
    assert(stored.master_len_hi == 0u);
    memset(&decoded, 0, sizeof(decoded));
    mello_stored_project_decode(&decoded, &stored);
    assert(decoded.master_len == 128u);
}

static void test_main_reset_vs_natural_loop(void) {
    MelloProject p = plain_project(8u, 0u);
    MelloEngine e;

    mello_engine_init(&e, &p, 1u);
    mello_engine_start(&e);
    assert(e.loop_pulse_length == 8u);
    advance_ok(&e, 4u);
    assert(e.loop_count == 0u);
    assert(e.absolute_pulse == 4u);
    assert(e.step_index == 4u);
    advance_ok(&e, 4u);
    assert(e.loop_count == 1u);
    assert(e.absolute_pulse == 8u);
    assert(e.step_index == 0u);

    p = plain_project(8u, 4u);
    mello_engine_init(&e, &p, 2u);
    mello_engine_start(&e);
    assert(e.loop_pulse_length == 4u);
    advance_ok(&e, 4u);
    assert(e.loop_count == 1u);
    assert(e.absolute_pulse == 4u);
    assert(e.step_index == 0u); /* forced main reset, independent of natural length */
}

static void test_loop_cycle_sync_is_separate(void) {
    MelloProject p = plain_project(8u, 4u);
    MelloEngine free_lc, sync_lc;
    MelloLoopCycleStep step;

    mello_engine_init(&free_lc, &p, 3u);
    step = free_lc.loop_cycle.steps[0];
    step.hold = 3u;
    mello_engine_loop_cycle_set_step(&free_lc, 0u, &step);
    mello_engine_loop_cycle_set_step(&free_lc, 1u, &step);
    mello_engine_loop_cycle_set_active(&free_lc, true);
    mello_engine_loop_cycle_set_sync(&free_lc, false);
    mello_engine_start(&free_lc);
    advance_ok(&free_lc, 4u);
    assert(free_lc.loop_count == 1u);
    assert(free_lc.loop_cycle_pos == 1u);
    assert(free_lc.loop_cycle_tick == 1u);

    mello_engine_init(&sync_lc, &p, 3u);
    step = sync_lc.loop_cycle.steps[0];
    step.hold = 3u;
    mello_engine_loop_cycle_set_step(&sync_lc, 0u, &step);
    mello_engine_loop_cycle_set_step(&sync_lc, 1u, &step);
    mello_engine_loop_cycle_set_active(&sync_lc, true);
    mello_engine_loop_cycle_set_sync(&sync_lc, true);
    mello_engine_start(&sync_lc);
    advance_ok(&sync_lc, 4u);
    assert(sync_lc.loop_count == 1u); /* same main reset */
    assert(sync_lc.loop_cycle_pos == 1u);
    assert(sync_lc.loop_cycle_tick == 0u); /* Loop Cycle's own SYNC behavior */
}

static void test_slot_hold_is_separate(void) {
    MelloProject p = plain_project(8u, 4u);
    MelloEngine e;

    mello_engine_init(&e, &p, 4u);
    mello_engine_slot_seq_set_step(&e, 0u, -1, 2u);
    mello_engine_slot_seq_set_active(&e, true);
    mello_engine_start(&e);
    advance_ok(&e, 4u);
    assert(e.loop_count == 1u);
    assert(e.slot_seq_pos == 1u); /* HOLD counter has its own processed-pulse clock */
}

static void test_external_sync_is_transport_only(void) {
    MelloProject p = plain_project(8u, 4u);
    MelloTransport t;
    unsigned i;

    mello_transport_init(&t, &p, 5u, NULL, NULL);
    assert(t.sync_mode == MELLO_SYNC_OFF);
    assert(t.engine.project.master_len == 4u);
    mello_transport_set_sync(&t, MELLO_SYNC_SLAVE, 1000u);
    assert(t.sync_mode == MELLO_SYNC_SLAVE);
    assert(t.engine.project.master_len == 4u);
    assert(!t.running);

    mello_transport_midi_in(&t, 0xFAu, 0u, 0u, 1000u);
    assert(t.running);
    assert(t.engine.absolute_pulse == 1u); /* external Start emits first sequence pulse */
    for (i = 0u; i < 18u; ++i)
        mello_transport_midi_in(&t, 0xF8u, 0u, 0u, 2000u + i * 1000u);
    assert(t.engine.absolute_pulse == 4u);
    assert(t.engine.loop_count == 1u); /* main reset still belongs to engine */
    assert(t.engine.project.master_len == 4u);
}

int main(void) {
    test_defaults_and_storage();
    test_main_reset_vs_natural_loop();
    test_loop_cycle_sync_is_separate();
    test_slot_hold_is_separate();
    test_external_sync_is_transport_only();
    puts("WP-D D1 timing baseline PASS");
    puts("main: master_len=0 natural/free; nonzero processed-pulse reset");
    puts("separate: Loop Cycle SYNC/FREE, Slot Seq HOLD, MIDI OFF/SLAVE/MASTER");
    return 0;
}
