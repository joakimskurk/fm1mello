/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
#include "mello_audio.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if !defined(MELLO_REVERB_TEST_HOOK) || !MELLO_REVERB_TEST_HOOK
#error "WP-E3 smoke requires MELLO_REVERB_TEST_HOOK=1"
#endif

#define RV_TARGET_CAP 24576u
#define IR_N          52000u

static int16_t delay_buf[4096];
static int16_t reverb_buf[RV_TARGET_CAP];

static void setup(MelloAudio *a, uint16_t decay, uint16_t size, uint16_t tone, uint16_t pre)
{
    MelloSoundConfig sound;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sample;
    int32_t out[2] = {0, 0};
    memset(delay_buf, 0, sizeof delay_buf);
    memset(reverb_buf, 0, sizeof reverb_buf);
    mello_sound_defaults(&sound);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sample);
    fx.delay_wet = 0u;
    fx.delay_level = 0u;
    fx.dist_amount = 0u;
    fx.reverb_wet = 1000u;
    fx.reverb_level = 1000u;
    fx.reverb_decay_ms = decay;
    fx.reverb_size = size;
    fx.reverb_tone = tone;
    fx.reverb_pre_ms = pre;
    mello_audio_init(a, delay_buf, 4096u, reverb_buf, RV_TARGET_CAP);
    mello_audio_set_config(a, &sound, &fx, &sub, &sample);
    mello_audio_render(a, out, 1u, 0u, 80u);
}

static double tone_roughness(uint16_t tone)
{
    MelloAudio a;
    double e = 0.0, de = 0.0;
    int32_t prev = 0, l = 0, r = 0;
    uint32_t i;
    setup(&a, 6000u, 500u, tone, 0u);
    for (i = 0u; i < IR_N; ++i) {
        mello_audio_reverb_stereo_probe(&a, i == 0u ? 20000 : 0, &l, &r);
        if (i >= 8000u && i < 40000u) {
            double v = (double)l, d = (double)(l - prev);
            e += v * v;
            de += d * d;
        }
        prev = l;
    }
    return e > 1.0 ? de / e : 0.0;
}

static void structural_and_stereo_contract(void)
{
    MelloAudio a;
    uint32_t i, total = 0u, dense_l = 0u, dense_r = 0u, different = 0u;
    double xx = 0.0, yy = 0.0, xy = 0.0, corr2;
    int32_t l = 0, r = 0;
    setup(&a, 6000u, 500u, 620u, 0u);

    assert(a.reverb_pre_cap >= 2645u); /* the network runs at fs / 2: half the samples */
    assert(a.reverb_ap_cap[0] > 150u && a.reverb_ap_cap[1] > 250u && a.reverb_ap_cap[2] > 450u);
    total = a.reverb_pre_cap + a.reverb_ap_cap[0] + a.reverb_ap_cap[1] + a.reverb_ap_cap[2];
    for (i = 0u; i < 8u; ++i) {
        total += a.reverb_line_cap[i];
        assert(a.reverb_line_delay[i] >= 3u && a.reverb_line_delay[i] < a.reverb_line_cap[i]);
        assert((a.reverb_line_delay[i] & 1u) == 1u);
        if (i) assert(a.reverb_line_delay[i - 1u] < a.reverb_line_delay[i]);
    }
    assert(total <= RV_TARGET_CAP);
    assert(a.reverb_feedback_q15 > 29000u && a.reverb_feedback_q15 < 32768u);

    for (i = 0u; i < IR_N; ++i) {
        mello_audio_reverb_stereo_probe(&a, i == 0u ? 20000 : 0, &l, &r);
        assert(l >= -32767 && l <= 32767 && r >= -32767 && r <= 32767);
        if (i >= 2000u && i < 50000u) {
            double dl = (double)l, dr = (double)r;
            if (l > 1 || l < -1) dense_l++;
            if (r > 1 || r < -1) dense_r++;
            if (l != r) different++;
            xx += dl * dl;
            yy += dr * dr;
            xy += dl * dr;
        }
    }
    assert(dense_l > 30000u && dense_r > 30000u && different > 30000u);
    assert(xx > 1.0 && yy > 1.0);
    corr2 = (xy * xy) / (xx * yy);
    assert(corr2 < 0.04); /* |corr| < .20: real stereo late field, not mono duplication. */
    assert(yy > xx * 0.35 && yy < xx * 2.85); /* neither side may collapse. */
    printf("Reverb V3 stereo: dense=%u/%u corr2=%.6f energyR/L=%.3f layout=%u\n", (unsigned)dense_l,
           (unsigned)dense_r, corr2, yy / xx, (unsigned)total);
}

static int push_note(MelloAudio *a, MelloEventType type, uint32_t due)
{
    MelloEvent e;
    memset(&e, 0, sizeof e);
    e.type = type;
    e.note = 60u;
    e.velocity = 100u;
    return mello_audio_push_engine_event(a, &e, due) ? 1 : 0;
}

static void rendered_stereo_contract(void)
{
    MelloAudio a;
    MelloSoundConfig sound;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sample;
    uint32_t i, diff = 0u;
    int32_t out[2];
    memset(delay_buf, 0, sizeof delay_buf);
    memset(reverb_buf, 0, sizeof reverb_buf);
    mello_sound_defaults(&sound);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sample);
    fx.delay_wet = 0u;
    fx.delay_level = 0u;
    fx.dist_amount = 0u;
    fx.reverb_wet = 1000u;
    fx.reverb_level = 1000u;
    fx.reverb_decay_ms = 6000u;
    fx.reverb_size = 500u;
    fx.reverb_tone = 620u;
    mello_audio_init(&a, delay_buf, 4096u, reverb_buf, RV_TARGET_CAP);
    mello_audio_set_config(&a, &sound, &fx, &sub, &sample);
    assert(push_note(&a, MELLO_EVENT_NOTE_ON, 0u));
    assert(push_note(&a, MELLO_EVENT_NOTE_OFF, 18000u));
    for (i = 0u; i < 18000u; ++i) {
        out[0] = out[1] = 0;
        mello_audio_render(&a, out, 1u, i * 23u, 80u);
        if (i > 3000u && out[0] != out[1]) diff++;
    }
    assert(diff > 5000u);
}

/* A quiet tail rings as long as a loud one and ends in silence (0.5-f118: the lines' 8 bits under each sample, the
 * loops' products towards zero). Before, the rounding held a floor of about 6 samples at a 2.5 s decay: a burst 18 dB
 * down then "fell" 60 dB in 80 s, and nothing ever went silent. A second of noise, down by div; the time its energy
 * left (Schroeder's) takes from -5 to -25 dB, x 3; the last of 24 s's peak. */
static double t60_after(MelloAudio *a, int32_t div, int32_t *last_peak)
{
    static double e[44100u * 24u];
    uint32_t n, seed = 777u, t5 = 0u, t25 = 0u, len = 44100u * 24u;
    double sum = 0.0;
    int32_t l, r;
    *last_peak = 0;
    for (n = 0u; n < len; ++n) {
        int32_t in = 0;
        if (n < 44100u) seed = seed * 1664525u + 1013904223u, in = ((int32_t)(seed >> 20) - 2048) * 2 / div;
        mello_audio_reverb_stereo_probe(a, in, &l, &r);
        e[n] = (double)l * l + (double)r * r;
        if (n >= len - 44100u) {
            int32_t m = l < 0 ? -l : l;
            if (m > *last_peak) *last_peak = m;
        }
    }
    for (n = len; n-- > 44100u;) sum += e[n], e[n] = sum;
    for (n = 44100u; n < len && !t25; ++n) {
        double d = 10.0 * log10(e[n] / e[44100u] + 1e-30);
        if (!t5 && d < -5.0) t5 = n;
        if (d < -25.0) t25 = n;
    }
    return t25 ? 3.0 * (double)(t25 - t5) / 44100.0 : -1.0;
}
static void quiet_tail(void)
{
    static MelloAudio a;
    int32_t pk_loud, pk_quiet;
    double loud, quiet;
    setup(&a, 6000u, 700u, 620u, 12u);
    loud = t60_after(&a, 1, &pk_loud);
    setup(&a, 6000u, 700u, 620u, 12u);
    quiet = t60_after(&a, 8, &pk_quiet);
    printf("V3: 60 dB in %.2f s, 18 dB down in %.2f s; the last second's peak %d and %d\n", loud, quiet,
           pk_loud, pk_quiet);
    assert(loud > 5.4 && loud < 6.6); /* DECAY 6 s: 60 dB in 6 s (DECISIONS "DECAY in seconds") */
    assert(quiet > 0.92 * loud && quiet < 1.08 * loud);
    assert(pk_loud == 0 && pk_quiet == 0);
}

int main(void)
{
    double dark, bright;
    structural_and_stereo_contract();
    /* Phase 2: the loop is flat, as Tone.Reverb's white-noise IR; TONE is the return filter only (revTone). */
    dark = tone_roughness(150u);
    bright = tone_roughness(900u);
    assert(dark > 0.0 && bright > 0.0 && bright < dark * 1.2 && dark < bright * 1.2);
    printf("Reverb feedback tone roughness (flat loop): dark=%.5f bright=%.5f\n", dark, bright);
    rendered_stereo_contract();
    quiet_tail();
    puts("PASS: WP-E3 Reverb V3 dense 8-line stereo FDN contract");
    return 0;
}
