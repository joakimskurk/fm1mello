/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* PERFORM's black keys (DECISIONS "PERFORM's black keys"), on the audio. A take of the default pattern, the effect
 * held from 1.0 s to 2.0 s (amounts as the UI starts them), measured against the same take with nothing held:
 * - nothing held: the very same samples (the effects are out of the path);
 * - TAPE: the last 100 ms held all but silent (stopped); 5 ms after letting go, the take as it was;
 * - STUT: once round, the sound repeats itself at the division's length (1/16 at 120 BPM: 11025 samples a loop);
 * - CRUSH: the samples held in runs; CHOP: near silence in the gate's off part;
 * - PLUCK: less sound late in each note; OCT-: the lead's zero crossings halved;
 * - WARP: not the take (a moving delay), and back to it after letting go.
 * The UI: in PERFORM a black key holds its effect (not the page's action), K1 turns its amount, the strip says it. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mello_ui.h"
#include "mello_audio.h"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

#define N (44100u * 3u)
static int16_t db[65536], rb[24576];
static MelloAudio a;
static MelloTransport t;
static int32_t dry[N], wet[N];
static void sink(void *c, const MelloEvent *ev, uint32_t due)
{
    (void)mello_audio_push_engine_event((MelloAudio *)c, ev, due);
}
static const uint16_t AMT[8] = {400u, 500u, 500u, 600u, 500u, 0u, 0u, 500u};

/* 3 s at 120 BPM, the lead alone (or the band), fx2 held from 1 s to 2 s; the left channel kept */
static void render(uint8_t fx2, bool lead_only, int32_t *keep)
{
    MelloProject p;
    uint32_t ms, i, n = 0u;
    int32_t out[96];
    mello_project_defaults(&p);
    p.bpm = 120u;
    mello_transport_init(&t, &p, 6u, 0, 0);
    if (lead_only) {
        MelloDrumConfig d = t.engine.drums;
        MelloSubConfig s = t.engine.sub;
        d.on = false, s.on = false;
        mello_engine_drums_set_config(&t.engine, &d);
        mello_engine_sub_set_config(&t.engine, &s);
    }
    mello_audio_init(&a, db, 65536, rb, 24576);
    mello_audio_set_config(&a, &t.engine.sound, &t.engine.fx, &t.engine.sub, &t.engine.sample);
    mello_audio_set_drum_config(&a, &t.engine.drums);
    mello_transport_set_event_sink(&t, sink, &a);
    mello_transport_start(&t, 0u);
    for (ms = 0u; ms < 3000u; ++ms) {
        uint32_t fr = 44u + (ms % 10u == 0u ? 1u : 0u);
        if (ms == 1000u) mello_audio_set_perf_fx2(&a, fx2, AMT);
        if (ms == 2000u) mello_audio_set_perf_fx2(&a, 0u, AMT);
        mello_transport_poll_ahead(&t, ms * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, fr, ms * 1000u, 120u);
        for (i = 0u; i < fr && n < N; ++i) keep[n++] = out[2u * i];
    }
}
static double energy(const int32_t *x, uint32_t from, uint32_t to)
{
    double e = 0;
    uint32_t i;
    for (i = from; i < to; ++i) e += (double)x[i] * x[i];
    return e / (double)(to - from) + 1.0;
}
static double diff(const int32_t *x, const int32_t *y, uint32_t from, uint32_t to)
{
    double e = 0;
    uint32_t i;
    for (i = from; i < to; ++i) e += ((double)x[i] - y[i]) * ((double)x[i] - y[i]);
    return e / (double)(to - from) + 1.0;
}
static unsigned crossings(const int32_t *x, uint32_t from, uint32_t to)
{
    unsigned c = 0u;
    uint32_t i;
    for (i = from + 1u; i < to; ++i) c += (x[i - 1u] < 0) != (x[i] < 0);
    return c;
}
#define S(ms) ((uint32_t)(ms) * 441u / 10u)

static void audio_part(void)
{
    double base;
    render(0u, false, dry);
    base = energy(dry, S(1000), S(2000));
    render(0u, false, wet);
    CHECK(!memcmp(dry, wet, sizeof dry), "nothing held: the take is not the same");

    render(MELLO_PERF_FX2_TAPE, false, wet);
    CHECK(energy(wet, S(1900), S(2000)) < base / 300.0, "TAPE: the end of the hold is not stopped (%.0f against %.0f)",
          energy(wet, S(1900), S(2000)), base);
    CHECK(diff(wet, dry, S(2005), S(2200)) < base / 1000.0, "TAPE: 5 ms after letting go it is not the take again");

    render(MELLO_PERF_FX2_STUT, false, wet);
    { /* 1/16 at 120 BPM: 125 ms, 5512 frames at half the rate, 11025 samples a loop */
        uint32_t loop = 5512u * 2u, from = S(1000) + 2u * loop;
        CHECK(diff(wet, wet + loop, from, from + loop / 2u) < energy(wet, from, from + loop / 2u) / 50.0,
              "STUT: the sound does not repeat itself at the division");
    }

    render(MELLO_PERF_FX2_CRUSH, false, wet);
    {
        uint32_t i, runs = 0u;
        for (i = S(1100); i < S(1900); ++i) runs += wet[i] == wet[i - 1u];
        CHECK(runs > (S(1900) - S(1100)) / 2u, "CRUSH: the samples are not held (%u repeats)", runs);
    }

    render(MELLO_PERF_FX2_CHOP, false, wet);
    { /* 1/16 at 120 BPM: 5512 samples, open the first 60% */
        uint32_t len = 5512u, k, off = 0u, on = 0u;
        double eo = 0, en = 0;
        for (k = 0u; k < 5u; ++k) {
            uint32_t s0 = S(1000) + 2u + (k + 2u) * len;
            eo += energy(wet, s0 + len * 7u / 10u, s0 + len * 95u / 100u), off++;
            en += energy(dry, s0 + len * 7u / 10u, s0 + len * 95u / 100u), on++;
        }
        CHECK(eo / off < en / on / 100.0, "CHOP: the gate's off part is not silent (%.0f against %.0f)", eo / off,
              en / on);
    }

    render(MELLO_PERF_FX2_PLUCK, false, wet);
    CHECK(energy(wet, S(1100), S(1900)) < energy(dry, S(1100), S(1900)) / 2.0, "PLUCK: hardly shorter");

    render(0u, true, dry);
    render(MELLO_PERF_FX2_OCTDN, true, wet);
    {
        unsigned cd = crossings(dry, S(1300), S(1900)), cw = crossings(wet, S(1300), S(1900));
        CHECK(cw * 10u < cd * 7u && cw * 10u > cd * 3u, "OCT-: %u zero crossings against %u, not about half", cw, cd);
    }
    render(0u, false, dry);
    render(MELLO_PERF_FX2_WARP, false, wet);
    CHECK(diff(wet, dry, S(1200), S(1900)) > base / 20.0, "WARP: no different from the take");
    CHECK(diff(wet, dry, S(2050), S(2300)) < base / 1000.0, "WARP: not the take again after letting go");
}

static void ui_part(void)
{
    static MelloProject p;
    static MelloTransport tt;
    static MelloStorage st;
    static MelloUi u;
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    uint8_t i, n = 0u, tape = 0xFFu, b0 = 0xFFu;
    int8_t was_root;
    for (i = 0u; i < 27u; ++i)
        if (mello_ui_white_key_position(i) < 0) {
            if (n == 0u) b0 = i;
            if (n == 1u) tape = i;
            n++;
        }
    mello_project_defaults(&p);
    mello_transport_init(&tt, &p, 5u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &tt, &st);
    mello_ui_set_page(&u, MELLO_UI_HOME);
    u.perform = 1u;
    was_root = (int8_t)tt.engine.project.root_note;
    mello_ui_set_held_notes(&u, 1u << b0); /* B0 on HOME is transpose -1; in PERFORM, STUT */
    mello_ui_input(&u, 1000000u, 1000u, 0u, 0u, 0u, 1u << b0, enc);
    mello_ui_input(&u, 1050000u, 1050u, 0u, 0u, 0u, 0u, enc);
    CHECK((tt.engine.perf.fx2 & MELLO_PERF_FX2_STUT) && tt.engine.project.root_note == (uint8_t)was_root,
          "PERFORM: B0 did not hold STUT, or transposed (fx2 %u)", tt.engine.perf.fx2);
    mello_ui_set_held_notes(&u, 1u << tape);
    mello_ui_input(&u, 1100000u, 1100u, 0u, 0u, 0u, 1u << tape, enc);
    enc[MELLO_UI_ENC_KNOB1] = 5;
    mello_ui_input(&u, 1150000u, 1150u, 0u, 0u, 0u, 0u, enc);
    enc[MELLO_UI_ENC_KNOB1] = 0;
    mello_ui_input(&u, 1160000u, 1160u, 0u, 0u, 0u, 0u, enc); /* (the engine has it at the next input) */
    CHECK((tt.engine.perf.fx2 & MELLO_PERF_FX2_TAPE) && u.perf_amt[1] != 500u && tt.engine.perf.fx2_amt[1] == u.perf_amt[1],
          "PERFORM: K1 with TAPE held did not turn TAPE (%u)", u.perf_amt[1]);
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 1200000u, 1200u, 0u, 0u, 0u, 0u, enc);
    CHECK(!tt.engine.perf.fx2, "letting go did not let the effects go (%u)", tt.engine.perf.fx2);
}

int main(void)
{
    audio_part();
    ui_part();
    if (fails) return 1;
    puts("PASS: perform_black");
    return 0;
}
