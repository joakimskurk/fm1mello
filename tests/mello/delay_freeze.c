/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* FREEZE (FX > DELAY LOOP, ROADMAP H7, DECISIONS "The looping delay"): Clouds' looping delay, frozen, plays a loop of
 * what it holds and records nothing. Here the loop is the delay's time: frozen, the return repeats with that period to
 * the sample (but where a pass fades in), what is sent after does not get in, REV plays it backwards, the seams and
 * the way in and out have no click, a time turned while frozen sizes the next pass, and turned off the tap and the
 * writing come back. Off, nothing changes. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"

#if !defined(MELLO_REVERB_TEST_HOOK) || !MELLO_REVERB_TEST_HOOK
#error "delay_freeze needs MELLO_REVERB_TEST_HOOK=1"
#endif

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

#define FS  44100
#define LEN 11025u /* 250 ms */
static int16_t delay_buf[MELLO_AUDIO_DELAY_SAMPLES], reverb_buf[MELLO_AUDIO_REVERB_SAMPLES];
static MelloAudio a;
static MelloSoundConfig sound;
static MelloFxConfig fx;
static MelloSubConfig sub;
static MelloSampleConfig sample;
static int32_t ret[6 * FS];

static void apply(void)
{
    int32_t out[2];
    mello_audio_set_config(&a, &sound, &fx, &sub, &sample);
    mello_audio_render(&a, out, 1u, 0u, 80u); /* (applies it; the render's own sample is not the probe's) */
}
static double tone(const int32_t *x, uint32_t n, double hz)
{
    double c = 2.0 * cos(6.283185307179586 * hz / FS), s1 = 0.0, s2 = 0.0;
    uint32_t i;
    for (i = 0u; i < n; ++i) {
        double s = x[i] + c * s1 - s2;
        s2 = s1, s1 = s;
    }
    return s1 * s1 + s2 * s2 - c * s1 * s2;
}
static int32_t sine(uint32_t n, double hz, double amp)
{
    return (int32_t)(amp * sin(6.283185307179586 * hz * n / FS));
}
static int32_t step_max(const int32_t *x, uint32_t from, uint32_t to)
{
    int32_t m = 0;
    uint32_t i;
    for (i = from + 1u; i < to; ++i) {
        int32_t d = x[i] - x[i - 1u];
        if (d < 0) d = -d;
        if (d > m) m = d;
    }
    return m;
}
static void start(void)
{
    mello_audio_init(&a, delay_buf, MELLO_AUDIO_DELAY_SAMPLES, reverb_buf, MELLO_AUDIO_REVERB_SAMPLES);
    fx.delay_freeze = 0u;
    apply();
}

int main(void)
{
    uint32_t n;
    int32_t r;
    mello_sound_defaults(&sound);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sample);
    fx.delay_sync = false, fx.delay_time_ms = 250u, fx.delay_lowcut = 0u, fx.delay_tone = 1000u;

    /* frozen: the period, nothing new, no click */
    start();
    CHECK(!a.frz_play, "a new project frozen");
    for (n = 0u; n < FS; ++n) (void)mello_audio_delay_probe(&a, sine(n, 300.0, 8000.0), 0, &r);
    fx.delay_freeze = 1u, apply();
    CHECK(a.frz_play && a.frz_len_n == LEN, "FREEZE: playing %d, a pass of %u (want %u)", a.frz_play,
          a.frz_len_n, LEN);
    for (n = 0u; n < 4u * LEN; ++n) ret[n] = mello_audio_delay_probe(&a, sine(n, 2000.0, 12000.0), 2048, &r);
    {
        uint32_t i, diff = 0u;
        for (i = LEN; i < 3u * LEN; ++i)
            if (i % LEN >= 256u && ret[i] != ret[i + LEN]) diff++;
        CHECK(diff == 0u, "FREEZE: %u samples differ a pass apart (out of the crossfades)", diff);
        CHECK(tone(ret + LEN, 2u * LEN, 300.0) > 1e4 * tone(ret + LEN, 2u * LEN, 2000.0),
              "FREEZE: what was sent after got in (2 kHz %g, 300 Hz %g)", tone(ret + LEN, 2u * LEN, 2000.0),
              tone(ret + LEN, 2u * LEN, 300.0));
        /* a 300 Hz sine at 8000 moves at most 342 a sample; a seam's jump would be up to 16000 */
        CHECK(step_max(ret, 1u, 4u * LEN) < 600, "FREEZE: a step of %d (a click) in the loop",
              step_max(ret, 1u, 4u * LEN));
    }
    /* a time turned while frozen sizes the next pass */
    fx.delay_time_ms = 400u, apply();
    for (n = 0u; n < LEN + 10u; ++n) (void)mello_audio_delay_probe(&a, 0, 2048, &r);
    CHECK(a.frz_len_n == 17640u, "FREEZE: 400 ms turned, the next pass %u samples (want 17640)", a.frz_len_n);
    fx.delay_time_ms = 250u, apply();

    /* off: the tap back, the writing with it, no click */
    for (n = 0u; n < 2u * LEN; ++n) (void)mello_audio_delay_probe(&a, 0, 2048, &r);
    fx.delay_freeze = 0u, apply();
    for (n = 0u; n < 3u * LEN; ++n) ret[n] = mello_audio_delay_probe(&a, sine(n, 1000.0, 8000.0), 0, &r);
    CHECK(!a.frz_play, "FREEZE off: still playing the loop");
    CHECK(step_max(ret, 1u, 3u * LEN) < 1400, "FREEZE off: a step of %d (a click)",
          step_max(ret, 1u, 3u * LEN));
    CHECK(tone(ret + 2u * LEN, LEN, 1000.0) > 1e3 * tone(ret + 2u * LEN, LEN, 300.0),
          "FREEZE off: what is sent does not come back (1 kHz %g, 300 Hz %g)",
          tone(ret + 2u * LEN, LEN, 1000.0), tone(ret + 2u * LEN, LEN, 300.0));

    /* REV: a rising ramp comes back falling */
    {
        double up = 0.0, down = 0.0;
        uint32_t i;
        start();
        for (n = 0u; n < FS; ++n) (void)mello_audio_delay_probe(&a, (int32_t)(n % 20000u) - 10000, 0, &r);
        fx.delay_freeze = 1u, apply();
        for (n = 0u; n < 2u * LEN; ++n) ret[n] = mello_audio_delay_probe(&a, 0, 0, &r);
        for (i = LEN + 1000u; i < LEN + 3000u; ++i) up += ret[i] - ret[i - 1u];
        fx.delay_freeze = 2u, apply();
        for (n = 0u; n < 2u * LEN; ++n) ret[n] = mello_audio_delay_probe(&a, 0, 0, &r);
        for (i = LEN + 1000u; i < LEN + 3000u; ++i) down += ret[i] - ret[i - 1u];
        CHECK(up > 1500.0 && down < -1500.0, "REV: the ramp rose %.0f frozen and %.0f backwards", up, down);
    }

    /* never frozen: the delay as it was, to the sample, beside one that was never told */
    {
        static int32_t plain[2 * FS];
        int same = 1;
        start();
        for (n = 0u; n < 2u * FS; ++n)
            plain[n] = mello_audio_delay_probe(&a, n < 2000u ? sine(n, 500.0, 9000.0) : 0, 2867, &r);
        start();
        fx.delay_freeze = 0u, fx.delay_glide = 0u, apply();
        for (n = 0u; n < 2u * FS; ++n)
            if (mello_audio_delay_probe(&a, n < 2000u ? sine(n, 500.0, 9000.0) : 0, 2867, &r) != plain[n])
                same = 0;
        CHECK(same, "FREEZE never on: the delay changed");
    }
    if (fails) return 1;
    puts("PASS: delay_freeze");
    return 0;
}
