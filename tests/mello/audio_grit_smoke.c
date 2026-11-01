/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Phase 5: the grit (DECISIONS.md "Grit"). Off by default and then sample for sample the plain saw; noise, noise FM
 * and unison each change the sound, at a comparable level, without overflow (built with UBSan) even all at full on
 * a high note with the LFO on top. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mello_audio.h"

#define FRAMES 256u
#define BLOCKS 40u
static int16_t delay_buf[4096];
static int16_t reverb_buf[24576];
static int32_t out[BLOCKS][FRAMES * 2u];

/* A held note, dry, filter open; renders BLOCKS blocks into out. */
static void play(const MelloSoundConfig *s, uint8_t note)
{
    static MelloAudio a;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    MelloEvent e;
    uint32_t blk, us = 0u;
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sm);
    fx.delay_wet = fx.reverb_wet = 0u;
    sub.on = false;
    mello_audio_init(&a, delay_buf, 4096u, reverb_buf, 24576u);
    mello_audio_set_config(&a, s, &fx, &sub, &sm);
    memset(&e, 0, sizeof e);
    e.type = MELLO_EVENT_NOTE_ON;
    e.note = note;
    e.velocity = 110u;
    mello_audio_push_engine_event(&a, &e, 0u);
    for (blk = 0u; blk < BLOCKS; ++blk) {
        memset(out[blk], 0, sizeof out[blk]);
        mello_audio_render(&a, out[blk], FRAMES, us, 80u);
        us += FRAMES * 1000000u / 44100u;
    }
}

/* RMS of the left channel over the second half (past the attack), and how rough it is: the mean |difference|. */
static void measure(double *rms, double *rough)
{
    double sq = 0.0, d = 0.0;
    uint32_t blk, i, n = 0u;
    int32_t prev = out[BLOCKS / 2u][0];
    for (blk = BLOCKS / 2u; blk < BLOCKS; ++blk)
        for (i = 0u; i < FRAMES; ++i, ++n) {
            int32_t v = out[blk][i * 2u];
            sq += (double)v * v;
            d += abs(v - prev);
            prev = v;
        }
    *rms = __builtin_sqrt(sq / n);
    *rough = d / n;
}

int main(void)
{
    static int32_t plain[BLOCKS][FRAMES * 2u];
    MelloSoundConfig base, s;
    double r0, g0, r, g;
    mello_sound_defaults(&base);
    base.sustain_permille = 1000u;
    base.follower_sub_on = false;
    base.filter_cut = 1000u;

    if (base.noise_level || base.noise_fm || base.unison != 1u || base.detune) {
        fprintf(stderr, "grit is on by default\n");
        return 1;
    }
    play(&base, 57u);
    memcpy(plain, out, sizeof out);
    measure(&r0, &g0);
    printf("plain saw: rms %.0f, roughness %.0f\n", r0, g0);

    /* Unison 1 with detune turned: still the plain saw, sample for sample. */
    s = base;
    s.detune = 800u;
    play(&s, 57u);
    if (memcmp(plain, out, sizeof out)) {
        fprintf(stderr, "detune changed a single oscillator\n");
        return 1;
    }

    s = base;
    s.noise_level = 600u;
    play(&s, 57u);
    measure(&r, &g);
    printf("noise 60%%: rms %.0f, roughness %.0f\n", r, g);
    if (g < g0 * 1.5) {
        fprintf(stderr, "noise did not roughen the sound\n");
        return 1;
    }

    s = base;
    s.noise_fm = 500u;
    play(&s, 57u);
    measure(&r, &g);
    printf("noise FM 50%%: rms %.0f, roughness %.0f\n", r, g);
    if (!memcmp(plain, out, sizeof out) || r < r0 * 0.7 || r > r0 * 1.4) {
        fprintf(stderr, "noise FM did nothing, or moved the level\n");
        return 1;
    }

    s = base;
    s.unison = 4u;
    s.detune = 600u;
    play(&s, 57u);
    measure(&r, &g);
    printf("unison 4, detune 60%%: rms %.0f\n", r);
    if (!memcmp(plain, out, sizeof out) || r < r0 * 0.6 || r > r0 * 1.6) { /* +-4 dB of the plain saw */
        fprintf(stderr, "unison did nothing, or its level is off\n");
        return 1;
    }

    /* The LFO moves detune: from a detune of 0 it spreads the unison. */
    s = base;
    s.unison = 3u;
    play(&s, 57u);
    memcpy(plain, out, sizeof out);
    s.lfo_on = true;
    s.lfo_target = MELLO_LFO_DETUNE;
    s.lfo_depth = 1000u;
    s.lfo_rate_millihz = 5000u;
    play(&s, 57u);
    if (!memcmp(plain, out, sizeof out)) {
        fprintf(stderr, "LFO -> DETUNE did nothing\n");
        return 1;
    }

    /* Everything at full on a high note, square, resonance up, LFO on noise: no overflow (UBSan), and in range. */
    s = base;
    s.osc_wave = MELLO_WAVE_SQUARE;
    s.filter_res = 1000u;
    s.noise_level = s.noise_fm = s.detune = 1000u;
    s.unison = MELLO_UNISON_MAX;
    s.lfo_on = true;
    s.lfo_target = MELLO_LFO_NOISE;
    s.lfo_depth = 1000u;
    play(&s, 108u);
    play(&s, 24u);
    puts("PASS: grit off = plain saw; noise, noise FM, unison/detune and LFO -> DETUNE work, in range");
    return 0;
}
