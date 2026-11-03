/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The delay's divisions and its reach (ROADMAP E11 / F9, DECISIONS "A send per part"): 32N, 16D, 2T, 2D and 1 BAR
 * after the HTML's and 16T .. 2N; a time past the line's 1.49 s runs it at half rate and the echo still comes on
 * time (1 BAR at 100 BPM, 2.4 s); past 2.97 s the longest it holds plays. A new project's LO CUT is on (~120 Hz). */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"
#include "mello_transport.h"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

static int16_t db[65536], rb[24576];
static MelloAudio a;
static MelloTransport t;
static void sink(void *ctx, const MelloEvent *e, uint32_t due)
{
    (void)mello_audio_push_engine_event((MelloAudio *)ctx, e, due);
}

/* One lead note at the start (every other step a rest, a sequence of 64 so it does not come round), the delay alone
 * on DIV `div` at `bpm`, feedback 0: the time (ms) of the loudest 10 ms after the note has died away. */
static uint32_t echo_ms(uint8_t div, uint16_t bpm)
{
    MelloProject p;
    MelloFxConfig fx;
    uint32_t ms, i, best_ms = 0u;
    int32_t out[96];
    double win = 0, best = 0;
    mello_project_defaults(&p);
    p.bpm = bpm;
    p.seq_len = 32u;
    for (i = 0; i < MELLO_MAX_STEPS; ++i) p.steps[i].gate = i ? MELLO_GATE_REST : MELLO_GATE_SINGLE;
    p.steps[0].pulses = 1u;
    mello_transport_init(&t, &p, 1u, 0, 0);
    fx = t.engine.fx;
    fx.reverb_wet = 0u;
    fx.delay_wet = 1000u;
    fx.delay_feedback = 0u;
    fx.delay_sync = true;
    fx.delay_div = div;
    mello_engine_fx_set_config(&t.engine, &fx);
    mello_audio_init(&a, db, 65536, rb, 24576);
    mello_audio_set_config(&a, &t.engine.sound, &t.engine.fx, &t.engine.sub, &t.engine.sample);
    mello_transport_set_event_sink(&t, sink, &a);
    mello_transport_start(&t, 0u);
    for (ms = 0; ms < 3600u; ++ms) {
        uint32_t fr = 44u + (ms % 10u == 0u ? 1u : 0u);
        mello_transport_poll_ahead(&t, ms * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, fr, ms * 1000u, t.engine.project.bpm);
        for (i = 0; i < fr * 2u; ++i) win += (double)out[i] * out[i];
        if (ms % 10u == 9u) {
            if (ms > 600u && win > best) best = win, best_ms = ms - 9u;
            win = 0;
        }
    }
    return best_ms;
}

int main(void)
{
    MelloFxConfig d;
    uint32_t e1, e2, e3, e4;
    mello_fx_defaults(&d);
    CHECK(d.delay_lowcut >= 350u && d.delay_lowcut <= 420u, "a new project's LO CUT is not ~120 Hz (%u)",
          d.delay_lowcut);
    /* the divisions' times from a beat of 600 ms (100 BPM) */
    CHECK(mello_delay_div_ms(9u, 600u) == 75u && mello_delay_div_ms(10u, 600u) == 225u &&
              mello_delay_div_ms(11u, 600u) == 800u && mello_delay_div_ms(12u, 600u) == 1800u &&
              mello_delay_div_ms(13u, 600u) == 2400u && mello_delay_div_ms(8u, 600u) == 1200u,
          "the new divisions' times are wrong");

    e1 = echo_ms(8u, 100u);  /* 2N, 1.2 s: the line as it was */
    e2 = echo_ms(12u, 100u); /* 2D, 1.8 s: half rate */
    e3 = echo_ms(13u, 100u); /* 1 BAR, 2.4 s: half rate */
    e4 = echo_ms(13u, 60u);  /* 1 BAR at 60 BPM, 4 s: past the reach, the longest it holds (2.97 s) */
    CHECK(e1 >= 1180u && e1 <= 1260u, "2N at 100 BPM: the echo at %u ms, not 1200", e1);
    CHECK(e2 >= 1780u && e2 <= 1860u, "2D at 100 BPM: the echo at %u ms, not 1800", e2);
    CHECK(e3 >= 2380u && e3 <= 2460u, "1 BAR at 100 BPM: the echo at %u ms, not 2400", e3);
    CHECK(e4 >= 2900u && e4 <= 3020u, "1 BAR at 60 BPM: the echo at %u ms, not the reach (2970)", e4);
    printf("delay_reach: echoes at %u, %u, %u, %u ms\n", e1, e2, e3, e4);
    if (fails) return 1;
    puts("PASS: delay_reach");
    return 0;
}
