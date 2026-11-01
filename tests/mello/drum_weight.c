/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* A kick with weight (DECISIONS.md): one default kick hit, rendered alone, must carry low end and punch. Measured:
 * the energy at the fundamental and its second and third harmonics (what a small speaker carries), the peak, and how
 * long the hit stays above -20 dB of its peak. The floors are a little under what the 2026-11-01 kick measures, so
 * a thinner kick fails. The snare's body is measured the same way. */
#include "mello_audio.h"
#include "mello_engine.h"
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846 /* -std=c11 on Linux leaves it out */
#endif
#include <stdio.h>
#include <string.h>

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                      \
            printf(__VA_ARGS__);                                                                             \
            printf("\n");                                                                                    \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

static MelloAudio a;
static int16_t db[4096], rb[24576];
static int32_t out[512];
static double mono[44100]; /* a second */

static double goertzel(const double *x, uint32_t n, double f)
{
    double k = 2.0 * cos(2.0 * M_PI * f / 44100.0), s0 = 0.0, s1 = 0.0, s2 = 0.0;
    uint32_t i;
    for (i = 0u; i < n; ++i) {
        s0 = x[i] + k * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    return sqrt(fabs(s1 * s1 + s2 * s2 - k * s1 * s2)) / n;
}

/* One hit of voice v, the kit alone; the mono sum for a second. */
static void hit(uint8_t v)
{
    MelloDrumConfig d;
    MelloEvent ev;
    uint32_t us = 0u, blk, i;
    mello_audio_init(&a, db, 4096u, rb, 24576u);
    mello_drum_defaults(&d);
    d.on = true;
    mello_audio_set_drum_config(&a, &d);
    memset(&ev, 0, sizeof ev);
    ev.type = MELLO_EVENT_DRUM_HIT;
    ev.sample_slot = v;
    ev.note = 64u;
    ev.velocity = 114u;
    for (blk = 0u; blk < 172u; ++blk) { /* 172 x 256 = 44032 frames */
        if (blk == 1u) (void)mello_audio_push_engine_event(&a, &ev, us);
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, 256u, us, 120u);
        for (i = 0u; i < 256u && blk * 256u + i < 44100u; ++i)
            mono[blk * 256u + i] = (out[2u * i] + out[2u * i + 1u]) / 2.0;
        us += 5805u;
    }
}

int main(void)
{
    double low, peak = 0.0, body;
    uint32_t i, above = 0u;
    hit(MELLO_DRUM_KICK);
    for (i = 0u; i < 44032u; ++i)
        if (fabs(mono[i]) > peak) peak = fabs(mono[i]);
    for (i = 0u; i < 44032u; ++i)
        if (fabs(mono[i]) > peak / 10.0) above = i; /* the last sample above -20 dB */
    low = goertzel(mono, 22050u, 59.0) + goertzel(mono, 22050u, 118.0) + goertzel(mono, 22050u, 177.0);
    printf("kick: peak %.0f, above -20 dB for %u ms, low end %.0f (59 + 118 + 177 Hz over half a second)\n",
           peak, above * 1000u / 44100u, low);
    /* the kick of 2026-10-31 measured peak 3411, 70 ms, low end 147; the one with weight 3901, 120 ms, 254 */
    CHECK(peak > 3700.0, "the kick's peak is only %.0f", peak);
    CHECK(above * 1000u / 44100u >= 100u, "the kick is over in %u ms", above * 1000u / 44100u);
    CHECK(low > 220.0, "the kick's low end is only %.0f", low);

    hit(MELLO_DRUM_SNARE);
    body = goertzel(mono, 11025u, 195.0) + goertzel(mono, 11025u, 390.0);
    printf("snare: body %.0f (195 + 390 Hz over a quarter second)\n", body);
    CHECK(body > 60.0, "the snare's body is only %.0f", body);
    printf("PASS: the default kick has weight and punch, the snare a body\n");
    return 0;
}
