/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* RESET resets everything (DECISIONS.md): with RESET set (2 BAR = 32 pulses by default) the drums and the BASS SEQ
 * line start over with the sequence at the reset, however long they are; with RESET on FREE each part runs its own
 * length. A 12-step bass and a 12-step drum pattern under a 16-step lead: after pulse 32 their next step is 1 again
 * with RESET, and 33 mod 12 = 9 with FREE. The transport is polled as the app polls it. */
#include "mello_engine.h"
#include "mello_transport.h"
#include <stdio.h>

static void midi(void *c, uint8_t a, uint8_t b, uint8_t d)
{
    (void)c;
    (void)a;
    (void)b;
    (void)d;
}

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                      \
            printf(__VA_ARGS__);                                                                             \
            printf("\n");                                                                                    \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

static MelloTransport t;

static void run(uint16_t master_len, uint32_t pulses)
{
    MelloProject p;
    MelloDrumConfig d;
    MelloBassConfig b;
    uint32_t pulse_us, us;
    mello_project_defaults(&p);
    p.master_len = master_len;
    mello_transport_init(&t, &p, 3u, midi, NULL);
    d = t.engine.drums;
    d.on = true;
    d.length = 12u;
    mello_engine_drums_set_config(&t.engine, &d);
    b = t.engine.bass;
    b.on = true;
    b.length = 12u;
    mello_engine_bass_set_config(&t.engine, &b);
    pulse_us = 60000000u / t.engine.project.bpm / 4u;
    mello_transport_start(&t, 0u);
    /* up to the middle of pulse `pulses` (0-based): pulses 0..pulses have been counted */
    for (us = 0u; us <= pulses * pulse_us + pulse_us / 2u; us += 1000u)
        mello_transport_poll_ahead(&t, us, MELLO_TRANSPORT_LOOKAHEAD_US);
}

int main(void)
{
    run(32u, 30u); /* a pulse before the period's last: the parts are deep into their third time round */
    CHECK(t.engine.absolute_pulse == 31u, "pulse count %u, wanted 31", (unsigned)t.engine.absolute_pulse);
    CHECK(t.engine.bass_pos == 31u % 12u && t.engine.drum_pos == 31u % 12u,
          "before the reset the bass is at %u and the drums at %u (own clocks, 12 steps)", t.engine.bass_pos,
          t.engine.drum_pos);
    run(32u, 31u); /* the period's last pulse counted: the reset has put every part at its first step */
    CHECK(t.engine.absolute_pulse == 32u, "pulse count %u, wanted 32", (unsigned)t.engine.absolute_pulse);
    CHECK(t.engine.bass_pos == 0u && t.engine.drum_pos == 0u,
          "at the reset the bass is at %u and the drums at %u", t.engine.bass_pos, t.engine.drum_pos);
    run(32u, 32u); /* the first pulse of the next period played: the parts move on from their first step */
    CHECK(t.engine.absolute_pulse == 33u, "pulse count %u, wanted 33", (unsigned)t.engine.absolute_pulse);
    CHECK(t.engine.bass_pos == 1u, "RESET: the bass's next step is %u, wanted 1", t.engine.bass_pos);
    CHECK(t.engine.drum_pos == 1u, "RESET: the drums' next step is %u, wanted 1", t.engine.drum_pos);
    run(0u, 32u); /* FREE: the parts keep their own lengths */
    CHECK(t.engine.bass_pos == 33u % 12u && t.engine.drum_pos == 33u % 12u,
          "FREE: the bass is at %u and the drums at %u, wanted %u (own clocks)", t.engine.bass_pos,
          t.engine.drum_pos, 33u % 12u);
    printf(
        "PASS: at RESET the drums and the bass line start over with the sequence (12 under 16, back to 1 at "
        "pulse 32); FREE leaves them on their own clocks\n");
    return 0;
}
