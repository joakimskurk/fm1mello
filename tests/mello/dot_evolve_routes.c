/* Phase 5: DOT and EVOLVE (DECISIONS.md "DOT and EVOLVE routes").
 * - EVOLVE mutates at the loop's one by default, not on every step; LOOP off gives the old per-step mutation.
 * - DOT's EVOLVE moves lit cells at the loop's one only, and keeps how many are lit.
 * - MOD 1 / MOD 2 send their depth on lit cells, 0 on unlit ones, and nothing on pulses they do not read (FOLLOW).
 * - In mello_audio a lane is the HTML's filter dot by default (20 ms up, 140 ms down, +24% cutoff); GATE holds lit
 *   cells in a row and releases on an unlit one; HIT retriggers from where it is (no snap to 0). */
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fputc('\n', stderr);                                                                             \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

static MelloEngine e;
static MelloEvent last_pulse;

static void pulse(void)
{
    MelloEvent ev;
    (void)mello_engine_advance_pulse(&e);
    while (mello_event_pop(&e, &ev))
        if (ev.type == MELLO_EVENT_PULSE) last_pulse = ev;
}
static unsigned bits(uint16_t v)
{
    unsigned n = 0u;
    for (; v; v &= (uint16_t)(v - 1u)) ++n;
    return n;
}

static int engine_part(void)
{
    MelloProject p;
    MelloTmConfig tm;
    MelloModRouting m;
    MelloDotMatrixConfig d;
    uint16_t before[MELLO_TM_MAX_STEPS], cells0;
    uint32_t loop, i;
    bool moved = false;

    mello_project_defaults(&p);
    p.steps[1].pulses = 2u; /* step 2 has a pulse that is not a step start */
    mello_engine_init(&e, &p, 0x2468ACE1u);
    CHECK(e.modr.tm_on_loop, "EVOLVE does not wait for the loop's one by default");
    tm = e.tm;
    tm.on = true;
    tm.mutation_prob = 500u;
    mello_engine_tm_set_config(&e, &tm);
    mello_dot_defaults_for(&d, MELLO_DOT_MOD1);
    d.cells = 0x0F0Fu;
    mello_engine_dot_set_config(&e, MELLO_DOT_MOD1, &d);
    mello_dot_defaults_for(&d, MELLO_DOT_MOD2);
    d.cells = 0x0001u;
    d.follow = true;
    d.amount = 700u;
    mello_engine_dot_set_config(&e, MELLO_DOT_MOD2, &d);
    m = e.modr;
    m.dot_evolve[MELLO_DOT_MOD1] = 1000u;
    mello_engine_mod_routing_set(&e, &m);
    mello_engine_start(&e);

    pulse(); /* the first pulse: cell 0 is lit in both */
    loop = e.loop_pulse_length;
    printf("loop: %u pulses\n", loop);
    CHECK(loop > 4u, "no loop length");
    CHECK(last_pulse.dot_mod[0] == 240u && last_pulse.dot_mod[1] == 700u,
          "lit cells did not send their depth (%u, %u)", last_pulse.dot_mod[0], last_pulse.dot_mod[1]);
    CHECK(last_pulse.tm_value != 0 || e.tm_float_reg[0] == 32768u, "EVOLVE's value is not in the pulse");
    pulse(); /* step 2, its first pulse: MOD 1's cell 1 lit, MOD 2's cell 1 unlit */
    CHECK(last_pulse.dot_mod[0] == 240u && last_pulse.dot_mod[1] == 0u, "an unlit cell did not send 0");
    pulse(); /* step 2's second pulse: MOD 2 follows steps, so it does not read */
    CHECK(last_pulse.dot_mod[1] == MELLO_DOT_NOT_READ, "a FOLLOW lane read inside a step");

    memcpy(before, e.tm_float_reg, sizeof before);
    cells0 = e.dot[MELLO_DOT_MOD1].cells;
    for (i = 3u; i < loop - 1u; ++i) pulse(); /* up to the loop's last pulse (the one before the wrap) */
    CHECK(!memcmp(before, e.tm_float_reg, sizeof before), "EVOLVE mutated inside the loop");
    CHECK(e.dot[MELLO_DOT_MOD1].cells == cells0, "DOT evolved inside the loop");
    for (i = 0u; i < 6u * loop; ++i) { /* six wraps */
        pulse();
        CHECK(bits(e.dot[MELLO_DOT_MOD1].cells) == 8u, "DOT's evolve changed how many cells are lit");
        if (e.dot[MELLO_DOT_MOD1].cells != cells0) moved = true;
    }
    CHECK(memcmp(before, e.tm_float_reg, sizeof before), "EVOLVE never mutated at the loop's one");
    CHECK(moved, "DOT's evolve never moved a cell");

    m = e.modr; /* LOOP off: the old way, during the loop */
    m.tm_on_loop = false;
    mello_engine_mod_routing_set(&e, &m);
    memcpy(before, e.tm_float_reg, sizeof before);
    for (i = 0u; i < 3u; ++i) pulse();
    CHECK(memcmp(before, e.tm_float_reg, sizeof before), "LOOP off did not mutate on the steps");
    return 0;
}

static MelloAudio a;
static void lane_pulse(uint16_t lv, uint32_t t_us)
{
    MelloEvent ev;
    memset(&ev, 0, sizeof ev);
    ev.type = MELLO_EVENT_PULSE;
    ev.dot_mod[0] = lv;
    ev.dot_mod[1] = MELLO_DOT_NOT_READ;
    (void)mello_audio_push_engine_event(&a, &ev, t_us);
}
static uint32_t now_us;
static void run_ms(uint32_t ms) /* renders ms, 1 ms a block */
{
    int32_t out[2 * 44];
    while (ms--) {
        mello_audio_render(&a, out, 44u, now_us, 120u);
        now_us += 1000u;
    }
}
static int32_t lane_v(void)
{
    return a.dot_lane[0].v >> 16;
}

static int audio_part(void)
{
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    MelloModRouting m;
    int32_t v;
    mello_sound_defaults(&s);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sm);
    mello_audio_init(&a, 0, 0u, 0, 0u);
    mello_audio_set_config(&a, &s, &fx, &sub, &sm);
    mello_mod_routing_defaults(&m);
    mello_audio_set_mod_routing(&a, &m);
    run_ms(2u);

    lane_pulse(240u, now_us); /* HIT, the HTML's filter dot */
    run_ms(21u);
    v = lane_v();
    CHECK(v >= 225 && v <= 240, "HIT did not reach its depth in 20 ms (%d)", (int)v);
    run_ms(70u);
    v = lane_v();
    CHECK(v > 90 && v < 150, "HIT's release is not linear over 140 ms (%d halfway)", (int)v);
    lane_pulse(240u, now_us); /* a hit while the last one falls: up from where it is */
    run_ms(1u);
    CHECK(lane_v() >= v, "HIT restarted from 0 (%d after %d)", (int)lane_v(), (int)v);
    lane_pulse(0u, now_us); /* an unlit cell does not cut a hit */
    run_ms(1u);
    CHECK(lane_v() > 100, "an unlit cell cut the hit");
    run_ms(200u);
    CHECK(lane_v() == 0 && a.dot_lane[0].stage == 0u, "HIT did not end");

    m.dot_gate[0] = true; /* GATE: holds until an unlit cell */
    m.dot_attack_ms[0] = 0u;
    m.dot_release_ms[0] = 300u;
    mello_audio_set_mod_routing(&a, &m);
    run_ms(1u);
    lane_pulse(500u, now_us);
    run_ms(150u);
    lane_pulse(500u, now_us);
    lane_pulse(MELLO_DOT_NOT_READ, now_us);
    run_ms(150u);
    CHECK(lane_v() == 500, "GATE did not hold (%d)", (int)lane_v());
    lane_pulse(0u, now_us);
    run_ms(150u);
    v = lane_v();
    CHECK(v > 200 && v < 300, "GATE's release is off (%d halfway)", (int)v);
    run_ms(200u);
    CHECK(lane_v() == 0, "GATE did not release");

    /* EVOLVE's value moves its route: DECAY up, longer decay for this pulse's notes. */
    {
        MelloEvent ev;
        m.tm_dest[0] = MELLO_DEST_DECAY;
        m.tm_depth[0] = 1000;
        mello_audio_set_mod_routing(&a, &m);
        run_ms(1u);
        memset(&ev, 0, sizeof ev);
        ev.type = MELLO_EVENT_PULSE;
        ev.dot_mod[0] = ev.dot_mod[1] = MELLO_DOT_NOT_READ;
        ev.tm_value = 32767;
        (void)mello_audio_push_engine_event(&a, &ev, now_us);
        run_ms(1u);
        CHECK(a.effective_decay_ms > s.decay_ms * 3u, "EVOLVE -> DECAY did not lengthen the decay (%u)",
              a.effective_decay_ms);
        ev.tm_value = 0;
        (void)mello_audio_push_engine_event(&a, &ev, now_us);
        run_ms(1u);
        CHECK(a.effective_decay_ms == 0u, "the decay stayed modulated after the value went to 0");
    }
    return 0;
}

int main(void)
{
    if (engine_part() || audio_part()) return 1;
    puts("PASS: EVOLVE and DOT evolve at the loop's one; MOD lanes send depth, HIT and GATE shapes, EVOLVE "
         "-> DECAY");
    return 0;
}
