#include "mello_engine.h"
#include "mello_transport.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void drain_events(MelloEngine *e)
{
    MelloEvent ev;
    while (mello_event_pop(e, &ev)) { }
}

static MelloProject plain_project(uint8_t div, uint16_t master_len)
{
    MelloProject p;
    uint8_t i;
    mello_project_defaults(&p);
    p.seq_len = 7u;
    p.seg_count = 1u;
    p.seg_repeats[0] = 1u;
    p.seg_direction[0] = MELLO_SEG_FWD;
    p.seg_mode[0] = MELLO_SEG_LINEAR;
    p.seg_skip[0] = false;
    p.clock_div = div;
    p.master_len = master_len;
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        p.steps[i].pulses = 1u;
        p.steps[i].skip = false;
        p.steps[i].gate = MELLO_GATE_SINGLE;
        p.steps[i].probability = 65535u;
    }
    return p;
}

static void advance_ok(MelloEngine *e, unsigned count)
{
    while (count-- > 0u) {
        assert(mello_engine_advance_pulse(e));
        drain_events(e);
    }
}

static void test_default_is_two_bars(void)
{
    MelloProject p;
    mello_project_defaults(&p);
    assert(p.master_len == 32u);
    assert(p.clock_div == 1u);
}

static void test_bar_matrix_is_musical(void)
{
    static const uint16_t lengths[] = {16u, 32u, 64u, 128u, 256u};
    static const uint8_t divs[] = {1u, 2u, 4u};
    unsigned i, j;

    for (i = 0u; i < sizeof(lengths) / sizeof(lengths[0]); ++i) {
        for (j = 0u; j < sizeof(divs) / sizeof(divs[0]); ++j) {
            MelloProject p = plain_project(divs[j], lengths[i]);
            MelloEngine e;
            mello_engine_init(&e, &p, 11u + i * 3u + j);
            assert(mello_engine_rebuild_sequence(&e));

            /* Runtime is processed pulses, but musical duration is invariant. */
            assert(e.loop_pulse_length == (uint32_t)(lengths[i] / divs[j]));
            assert(e.loop_pulse_length * (uint32_t)p.clock_div == (uint32_t)lengths[i]);

            mello_engine_start(&e);
            advance_ok(&e, (unsigned)e.loop_pulse_length);
            assert(e.loop_count == 1u);
        }
    }
}

static void test_free_uses_natural_length(void)
{
    MelloProject p = plain_project(4u, 0u);
    MelloEngine e;
    mello_engine_init(&e, &p, 22u);
    assert(mello_engine_rebuild_sequence(&e));
    assert(e.loop_pulse_length == 7u);
    mello_engine_start(&e);
    advance_ok(&e, 7u);
    assert(e.loop_count == 1u);
}

static void test_storage_shape_is_unchanged(void)
{
    MelloProject p, decoded;
    MelloStoredProject stored;
    mello_project_defaults(&p);
    p.master_len = 128u;
    p.clock_div = 4u;

    mello_stored_project_encode(&stored, &p);
    assert(stored.master_len_lo == 128u);
    assert(stored.master_len_hi == 0u);

    memset(&decoded, 0, sizeof(decoded));
    mello_stored_project_decode(&decoded, &stored);
    assert(decoded.master_len == 128u);
    assert(decoded.clock_div == 4u);
}

static void test_other_timing_domains_remain_separate(void)
{
    MelloProject p = plain_project(2u, 32u);
    MelloEngine e;
    MelloTransport t;
    MelloLoopCycleStep step;

    mello_engine_init(&e, &p, 33u);
    step = e.loop_cycle.steps[0];
    step.hold = 3u;
    mello_engine_loop_cycle_set_step(&e, 0u, &step);
    mello_engine_loop_cycle_set_active(&e, true);
    mello_engine_loop_cycle_set_sync(&e, false);
    mello_engine_slot_seq_set_step(&e, 0u, -1, 2u);
    mello_engine_slot_seq_set_active(&e, true);
    mello_engine_start(&e);
    advance_ok(&e, 16u);
    assert(e.loop_count == 1u);
    assert(e.project.master_len == 32u);

    mello_transport_init(&t, &p, 44u, NULL, NULL);
    mello_transport_set_sync(&t, MELLO_SYNC_SLAVE, 1000u);
    assert(t.sync_mode == MELLO_SYNC_SLAVE);
    assert(t.engine.project.master_len == 32u);
    mello_transport_midi_in(&t, 0xFAu, 0u, 0u, 1000u);
    assert(t.running);
    assert(t.engine.loop_pulse_length == 16u);
}

int main(void)
{
    test_default_is_two_bars();
    test_bar_matrix_is_musical();
    test_free_uses_natural_length();
    test_storage_shape_is_unchanged();
    test_other_timing_domains_remain_separate();
    puts("WP-D D2 timing model PASS");
    puts("BAR matrix: 1/2/4/8/16 x DIV 1/2/4 remains musical; default=2 BAR; FREE natural");
    return 0;
}
