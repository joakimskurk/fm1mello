/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* GLIDE (FX > DELAY LOOP, ROADMAP H7, DECISIONS "The looping delay"): a new delay time is reached as Clouds' looping
 * delay reaches it, a tape's glide. At 0 the time ramps in 50 ms as before (the HTML's rampTo); turned up, it glides
 * with GLIDE's time constant (63% of the way in it), never past the new time, and gets there to the sample (a 3 s
 * glide too: its step keeps what it leaves under a unit); turned off while it glides, the ramp takes it from where it
 * is, with no jump, and arrives in 50 ms. In silence a new time is there at once. (A note held all through: the render
 * skips its samples in silence.) */
#include <stdio.h>
#include <string.h>

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

#define FS 44100
static int16_t delay_buf[MELLO_AUDIO_DELAY_SAMPLES];
static MelloAudio a;
static MelloSoundConfig sound;
static MelloFxConfig fx;
static MelloSubConfig sub;
static MelloSampleConfig sample;

static void apply(void)
{
    mello_audio_set_config(&a, &sound, &fx, &sub, &sample);
}
/* n samples of silence, a block at a time; the time the delay reads at each, in Q8, into t (when given) */
static int32_t run(uint32_t n, int32_t *lo, int32_t *hi)
{
    static int32_t out[2 * 64];
    int32_t mn = 0x7FFFFFFF, mx = -0x7FFFFFFF - 1;
    while (n) {
        uint32_t k = n > 64u ? 64u : n;
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, k, 0u, 80u);
        {
            int32_t v = a.dly_glide_k ? a.dly_glide_q8 : a.rp_delay.cur;
            if (v < mn) mn = v;
            if (v > mx) mx = v;
        }
        n -= k;
    }
    if (lo) *lo = mn;
    if (hi) *hi = mx;
    return a.dly_glide_k ? a.dly_glide_q8 : a.rp_delay.cur;
}

int main(void)
{
    int32_t from, to, now, lo, hi;
    double frac;
    mello_sound_defaults(&sound);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sample);
    fx.delay_sync = false, fx.delay_time_ms = 250u;
    sound.sustain_permille = 1000u;
    mello_audio_init(&a, delay_buf, MELLO_AUDIO_DELAY_SAMPLES, 0, 0u);
    apply();
    {
        MelloEvent e;
        memset(&e, 0, sizeof e);
        e.type = MELLO_EVENT_NOTE_ON, e.note = 57u, e.velocity = 110u;
        mello_audio_push_engine_event(&a, &e, 0u);
    }
    from = run(FS / 10u, 0, 0);
    CHECK(a.dly_glide_k == 0u && from == (int32_t)(11025u << 8), "a new project: no glide (step %u), at %d",
          a.dly_glide_k, from >> 8);

    /* GLIDE 0: the ramp, there in 50 ms */
    fx.delay_time_ms = 500u, apply();
    now = run(FS * 50u / 1000u + 64u, 0, 0);
    CHECK(now == (int32_t)(22050u << 8), "GLIDE 0: 50 ms after 250 -> 500 ms the time is %d samples",
          now >> 8);
    fx.delay_time_ms = 250u, apply();
    from = run(FS / 10u, 0, 0);

    /* GLIDE 50%: 300 ms; 63% of the way in 300 ms, never past, to the sample at last */
    CHECK(mello_delay_glide_ms(500u) == 300u && mello_delay_glide_ms(0u) == 0u &&
              mello_delay_glide_ms(1000u) == 3000u,
          "GLIDE's times: %u, %u, %u ms", mello_delay_glide_ms(0u), mello_delay_glide_ms(500u),
          mello_delay_glide_ms(1000u));
    fx.delay_glide = 500u, apply();
    run(64u, 0, 0);
    CHECK(a.dly_glide_k == 167772160u / (300u * 441u), "GLIDE 50%%: its step %u", a.dly_glide_k);
    fx.delay_time_ms = 500u, apply();
    to = 22050 << 8;
    now = run(FS * 300u / 1000u, &lo, &hi);
    frac = (double)(now - from) / (double)(to - from);
    CHECK(frac > 0.62 && frac < 0.645, "GLIDE 50%%: %.3f of the way in 300 ms (want 0.632)", frac);
    now = run(FS * 6u, &lo, &hi);
    CHECK(hi <= to, "GLIDE 50%%: past the time, %d", hi >> 8);
    CHECK(now == to, "GLIDE 50%%: 6 s on, %d Q8 short", to - now);

    /* GLIDE 100%: 3 s, to the sample */
    fx.delay_glide = 1000u, fx.delay_time_ms = 1000u, apply();
    to = 44100 << 8;
    now = run(FS * 3u, 0, 0);
    frac = (double)(now - (22050 << 8)) / (double)(to - (22050 << 8));
    CHECK(frac > 0.62 && frac < 0.645, "GLIDE 100%%: %.3f of the way in 3 s", frac);
    now = run(FS * 60u, &lo, &hi);
    CHECK(now == to && hi <= to, "GLIDE 100%%: a minute on, %d Q8 short (past by %d)", to - now, hi - to);

    /* turned off while it glides: the ramp from there, no jump, there in 50 ms */
    fx.delay_time_ms = 250u, apply();
    run(FS / 2u, 0, 0);
    {
        int32_t at = a.dly_glide_q8, next;
        fx.delay_glide = 0u, apply();
        run(1u, 0, 0);
        next = a.rp_delay.cur;
        CHECK(a.dly_glide_k == 0u, "GLIDE off: still a step %u", a.dly_glide_k);
        { /* one sample on: the ramp's one step (50 ms from there), no more */
            int32_t step = (at - (int32_t)(11025u << 8)) / 2205 + 512;
            CHECK(at - next >= 0 && at - next <= step, "GLIDE off: the time jumped from %d to %d", at >> 8,
                  next >> 8);
        }
        now = run(FS * 50u / 1000u + 64u, 0, 0);
        CHECK(now == (int32_t)(11025u << 8), "GLIDE off: 50 ms on, at %d samples", now >> 8);
    }
    CHECK(!a.silent, "the note stopped sounding");

    /* in silence: at once */
    fx.delay_glide = 1000u, apply();
    run(64u, 0, 0);
    mello_audio_all_notes_off(&a);
    run(FS * 20u, 0, 0);
    CHECK(a.silent, "not silent 20 s after the note");
    fx.delay_time_ms = 600u, apply();
    now = run(64u, 0, 0);
    CHECK(now == (int32_t)(26460u << 8), "in silence the glide is at %d samples, not 26460", now >> 8);
    if (fails) return 1;
    puts("PASS: delay_glide");
    return 0;
}
