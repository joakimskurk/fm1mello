/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Oliverb in fixed point (firmware/src/mello_oliverb.h, after Parasites' clouds/dsp/fx/oliverb.h) against the same
 * network in floating point, written here as Parasites has it (its Context's reads, writes and allpasses, its random
 * LFOs, its shimmer: MIT, Copyright 2014 Emilie Gillet; the notice in mello_oliverb-LICENSE.txt), on exact memory
 * (Clouds' is 16-bit, cut towards zero: the port keeps 8 bits more), the same scaled lengths and the same random
 * numbers: within -25 dB of each other sample by sample, and their energy within 1 dB. Then what a reverb must do:
 * its tail dies away (under 2 LSB and no DC held, long after the input stops), as long at any level as the floating
 * point's (with Clouds' memory a burst 36 dB down rang a third as long), it stays bounded at a decay past 1 (Parasites'
 * freeze: the soft limit's job), the shimmer moves the tail up (+12 to the octave, +7 to the fifth), and the same seed
 * gives the same samples. At the network's rate, 22.05 kHz. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mello_oliverb.h"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

#define FS 22050
static int16_t SINE[256];
static int16_t ring_a[MELLO_OLIVERB_WORDS], ring_b[MELLO_OLIVERB_WORDS];
static float ring_f[MELLO_OLIVERB_LEN];

/* ---- the floating-point reference: Parasites' Oliverb, step for step */
typedef struct {
    float phase, inc, value, next;
    int up;
} FLfo;
typedef struct {
    float *ring;
    unsigned base[OLV_LINES], wp;
    float acc, prev, lp1, lp2, hp1, hp2, smooth, ps_phase, ps_step;
    unsigned tick;
    FLfo lfo[9];
    uint32_t rng;
    float size, kap, decay, in_gain, lp, hp, mod, slope, ratio, psa;
} FOliverb;
static uint32_t f_rand(FOliverb *o)
{
    return o->rng = o->rng * 1664525u + 1013904223u;
}
static float f_raised_cos(float t)
{
    return (1.0f - cosf(t * 3.14159265f)) * 0.5f;
}
static void f_lfo_inc(FOliverb *o, FLfo *l)
{
    float d = fabsf(l->next - l->value);
    if (d < 1.0f / 32768.0f) d = 1.0f / 32768.0f;
    l->inc = o->slope / d;
    if (l->inc > 1.0f) l->inc = 1.0f;
}
static float f_lfo_next(FOliverb *o, FLfo *l)
{
    l->phase += l->inc;
    if (l->phase > 1.0f) {
        float rnd = 0.3f + 0.7f * (float)((double)f_rand(o) / 4294967296.0);
        l->phase -= 1.0f;
        l->value = l->next;
        l->up = !l->up;
        l->next = l->up ? l->value + (1.0f - l->value) * rnd : l->value - (1.0f + l->value) * rnd;
        f_lfo_inc(o, l);
    }
    return l->value + (l->next - l->value) * f_raised_cos(l->phase);
}
static float *f_at(FOliverb *o, int line, int k)
{
    unsigned i = o->wp + o->base[line] + (unsigned)k;
    return &o->ring[i >= MELLO_OLIVERB_LEN ? i - MELLO_OLIVERB_LEN : i];
}
static float f_get(FOliverb *o, int line, int k)
{
    return *f_at(o, line, k);
}
static void f_put(FOliverb *o, int line, int k,
                  float v) /* (saturated as the port's: 16 bits and 8 under them) */
{
    *f_at(o, line, k) = v > 8388607.0f / 8388608.0f ? 8388607.0f / 8388608.0f : v < -1.0f ? -1.0f : v;
}
static float f_hermite(FOliverb *o, int line, float off)
{
    int n = (int)off;
    float t = off - (float)n, xm1 = f_get(o, line, n - 1), x0 = f_get(o, line, n), x1 = f_get(o, line, n + 1),
          x2 = f_get(o, line, n + 2), c = (x1 - xm1) * 0.5f, v = x0 - x1, w = c + v,
          a = w + v + (x2 - x0) * 0.5f, b_neg = w + a;
    return (((a * t) - b_neg) * t + c) * t + x0;
}
static float f_off(FOliverb *o, int line, float lfo)
{
    float off = (float)(OLV_LEN[line] - 1u) * o->smooth + lfo * o->mod;
    return off<1.0f ? 1.0f : off>(float)(OLV_LEN[line] - 1u) ? (float)(OLV_LEN[line] - 1u) : off;
}
static void f_init(FOliverb *o, float *ring, uint32_t seed)
{
    unsigned i, b = 0;
    memset(o, 0, sizeof *o);
    for (i = 0; i < OLV_LINES; ++i) o->base[i] = b, b += OLV_LEN[i] + 1u;
    o->ring = ring;
    memset(ring, 0, sizeof ring_f);
    o->rng = seed;
    o->slope = 1.0f / 16777216.0f;
    for (i = 0; i < 9; ++i) {
        o->lfo[i].next = (float)((int32_t)(f_rand(o) >> 16) - 32768) / 32768.0f;
        f_lfo_inc(o, &o->lfo[i]);
    }
}
static void f_step(FOliverb *o, float in, float *l, float *r)
{
    float acc, apout, tri, ps_size, phase, half, g_main, g_ps;
    o->wp = o->wp ? o->wp - 1u : MELLO_OLIVERB_LEN - 1u;
    o->smooth += (o->size - o->smooth) * (475.0f / 32768.0f);
    ps_size = 88.0f + (2350.0f - 88.0f) * o->smooth;
    if (!(o->tick++ & 15u)) o->ps_step = (1.0f - o->ratio) / ps_size;
    o->ps_phase += o->ps_step;
    if (o->ps_phase >= 1.0f) o->ps_phase -= 1.0f;
    if (o->ps_phase < 0.0f) o->ps_phase += 1.0f;
    tri = f_raised_cos(2.0f * (o->ps_phase >= 0.5f ? 1.0f - o->ps_phase : o->ps_phase));
    phase = o->ps_phase * ps_size;
    half = phase + ps_size * 0.5f;
    if (half >= ps_size) half -= ps_size;
    {
        float a = f_get(o, OLV_AP1, 27), b = f_get(o, OLV_AP1, 28);
        f_put(o, OLV_AP1, 69, a + (b - a) * 0.5625f);
    }
    acc = in * o->in_gain;
#define F_AP(line, li, g)                                                                                    \
    do {                                                                                                     \
        float pr = f_hermite(o, line, f_off(o, line, (li) < 0 ? 0.0f : f_lfo_next(o, &o->lfo[li])));         \
        acc += pr * (g);                                                                                     \
        f_put(o, line, 0, acc);                                                                              \
        acc = acc * -(g) + pr;                                                                               \
    } while (0)
    F_AP(OLV_AP1, 1, o->kap);
    F_AP(OLV_AP2, 2, o->kap);
    F_AP(OLV_AP3, 3, o->kap);
    F_AP(OLV_AP4, 4, o->kap);
    apout = acc;
    g_main = o->decay * (1.0f - o->psa), g_ps = o->decay * o->psa;
    acc += f_hermite(o, OLV_DEL2, f_off(o, OLV_DEL2, f_lfo_next(o, &o->lfo[5]))) * g_main;
    if (g_ps > 0.0f) {
        acc += f_hermite(o, OLV_DEL2, phase) * tri * g_ps;
        acc += f_hermite(o, OLV_DEL2, half) * (1.0f - tri) * g_ps;
    }
    o->lp1 += o->lp * (acc - o->lp1), acc = o->lp1;
    o->hp1 += o->hp * (acc - o->hp1), acc -= o->hp1;
    acc = acc * (27.0f + acc * acc) / (27.0f + 9.0f * acc * acc);
    F_AP(OLV_DAP1A, 6, -o->kap);
    F_AP(OLV_DAP1B, -1, o->kap);
    f_put(o, OLV_DEL1, 0, acc);
    *l = acc * 2.0f;
    acc = apout;
    acc += f_hermite(o, OLV_DEL1, f_off(o, OLV_DEL1, f_lfo_next(o, &o->lfo[7]))) * g_main;
    if (g_ps > 0.0f) {
        acc += f_hermite(o, OLV_DEL1, phase) * tri * g_ps;
        acc += f_hermite(o, OLV_DEL1, half) * (1.0f - tri) * g_ps;
    }
    o->lp2 += o->lp * (acc - o->lp2), acc = o->lp2;
    o->hp2 += o->hp * (acc - o->hp2), acc -= o->hp2;
    acc = acc * (27.0f + acc * acc) / (27.0f + 9.0f * acc * acc);
    F_AP(OLV_DAP2A, 8, o->kap);
    F_AP(OLV_DAP2B, -1, -o->kap);
    f_put(o, OLV_DEL2, 0, acc);
    *r = acc * 2.0f;
}

/* ---- the settings, in both */
typedef struct {
    float size, kap, decay, in_gain, lp, hp, mod, rate, semis;
} Settings;
static void set_both(MelloOliverb *x, FOliverb *f, const Settings *s)
{
    float slope = s->rate * s->rate, ratio = powf(2.0f, s->semis / 12.0f), lim = 0.7f, sl = 0.4f,
          a = s->semis,
          psa = a < -lim        ? 1.0f
                : a < -lim + sl ? 1.0f - (a + lim) / sl
                : a < lim - sl  ? 0.0f
                : a < lim       ? 1.0f + (a - lim) / sl
                                : 1.0f;
    slope = slope * slope * slope / 200.0f * (32000.0f / FS); /* Parasites' rate, at this rate */
    uint32_t slope_q24 = (uint32_t)(slope * 16777216.0f) ? (uint32_t)(slope * 16777216.0f) : 1u;
    if (x)
        mello_oliverb_set(
            x, (int32_t)(s->size * 65536.0f), (int32_t)(s->kap * 32768.0f), (int32_t)(s->decay * 32768.0f),
            (int32_t)(s->in_gain * 32768.0f), (int32_t)(s->lp * 32768.0f), (int32_t)(s->hp * 32768.0f),
            (int32_t)(s->mod * 65536.0f), slope_q24, (uint32_t)(ratio * 65536.0f), (int32_t)(psa * 32768.0f));
    if (f) {
        unsigned i;
        f->size = (float)(int32_t)(s->size * 65536.0f) / 65536.0f, f->kap = s->kap, f->decay = s->decay;
        f->in_gain = s->in_gain, f->lp = s->lp, f->hp = s->hp, f->mod = s->mod, f->psa = psa;
        f->ratio = (float)(uint32_t)(ratio * 65536.0f) / 65536.0f;
        f->slope = (float)slope_q24 / 16777216.0f; /* (as the fixed point has it) */
        for (i = 0; i < 9; ++i) f_lfo_inc(f, &f->lfo[i]);
    }
}
static uint32_t noise = 12345u;
static int32_t burst(int n) /* white noise for the first 0.15 s, then nothing */
{
    if (n >= FS * 15 / 100) return 0;
    noise = noise * 1103515245u + 12345u;
    return ((int32_t)(noise >> 16) - 32768) >> 2;
}

/* the time the energy left (Schroeder's backward sum, from the burst's end) takes from -5 to -25 dB */
static double t20(double *e, int len)
{
    double sum = 0.0;
    int n, t5 = 0, t25 = 0;
    for (n = len - 1; n >= 0; --n) sum += e[n], e[n] = sum;
    for (n = FS * 15 / 100; n < len && !t25; ++n) {
        double d = 10.0 * log10(e[n] / e[FS * 15 / 100] + 1e-30);
        if (!t5 && d < -5.0) t5 = n;
        if (d < -25.0) t25 = n;
    }
    return (double)(t25 - t5) / FS;
}

int main(void)
{
    static const Settings S[3] = {
        /* Parasites' middle: size 0.5, diffusion 0.625, decay 0.65, input 0.5, open, no swing, no shimmer */
        {0.52f, 0.625f, 0.65f, 0.5f, 0.93f, 0.01f, 0.0f, 0.0f, 0.0f},
        /* swung (100 samples), dark, longer */
        {0.8f, 0.55f, 0.8f, 0.5f, 0.4f, 0.02f, 100.0f, 0.5f, 0.0f},
        /* the shimmer, +12 */
        {0.6f, 0.7f, 0.9f, 0.5f, 0.8f, 0.01f, 30.0f, 0.3f, 12.0f}};
    static MelloOliverb a, b;
    static FOliverb f;
    unsigned k;
    int i, n;
    for (i = 0; i < 256; ++i) SINE[i] = (int16_t)lrint(32767.0 * sin(6.283185307179586 * i / 256.0));

    /* fixed point against floating point, the same random numbers: sample by sample within -25 dB (a wrong tap,
     * sign or line is about 0 dB; the shimmer's window drifts a little apart), and the energy of every tenth of a
     * second within 1 dB while it is above -60 dB */
    for (k = 0u; k < 3u; ++k) {
        double e = 0.0, s = 0.0, we = 0.0, wf = 0.0, top = 0.0, worst = 0.0;
        mello_oliverb_init(&a, ring_a, 0x1234u + k);
        f_init(&f, ring_f, 0x1234u + k);
        set_both(&a, &f, &S[k]);
        noise = 12345u;
        for (n = 0; n < 2 * FS; ++n) {
            int32_t in = burst(n), l, r;
            float fl, fr;
            mello_oliverb_step(&a, in, &l, &r, SINE);
            f_step(&f, (float)in / 32768.0f, &fl, &fr);
            fl *= 32768.0f, fr *= 32768.0f;
            e += (l - fl) * (l - fl) + (r - fr) * (r - fr);
            s += fl * fl + fr * fr;
            we += (double)l * l + (double)r * r, wf += (double)fl * fl + (double)fr * fr;
            if ((n + 1) % (FS / 10) == 0) {
                if (wf > top) top = wf;
                if (wf > top * 1e-6 && we > 0.0) {
                    double d = fabs(10.0 * log10(we / wf));
                    if (d > worst) worst = d;
                }
                we = wf = 0.0;
            }
        }
        CHECK(s > 1e9, "setting %u: the reference is silent (%g)", k, s);
        CHECK(e < s * 3e-3, "setting %u: fixed point %.1f dB from the floating point", k,
              10.0 * log10(e / s));
        CHECK(worst < 1.0, "setting %u: a tenth of a second's energy %.2f dB from the floating point's", k,
              worst);
        printf("setting %u: fixed point %.1f dB from the floating point, its energy within %.2f dB\n", k,
               10.0 * log10(e / s + 1e-30), worst);
    }

    /* the tail dies away: under 1 LSB, and no DC, 12 s after the burst at the middle setting */
    {
        int32_t peak = 0, l = 0, r = 0;
        long dc = 0;
        mello_oliverb_init(&a, ring_a, 7u);
        set_both(&a, 0, &S[0]);
        noise = 12345u;
        for (n = 0; n < 14 * FS; ++n) {
            mello_oliverb_step(&a, burst(n), &l, &r, SINE);
            if (n >= 13 * FS) {
                int32_t m = l < 0 ? -l : l;
                if (m > peak) peak = m;
                dc += l + r;
            }
        }
        CHECK(peak <= 2, "13 s after the burst the tail is still %d", peak);
        CHECK(dc > -2 * FS && dc < 2 * FS, "a DC held in the tail: %ld over a second", dc);
    }

    /* as long at any level: at full level, 18 and 36 dB down, the tail falls from -5 to -25 dB (the energy left,
     * Schroeder's) in the floating point's time, within 3% (its output cut to samples as the port's: below a few
     * samples that cut shortens both alike), at a decay of 0.9 a half. Clouds' 16-bit memory: 9, 33 and 68% short. */
    {
        static const int down[3] = {1, 8, 64};
        static double e1[8 * FS], e2[8 * FS];
        Settings lg = S[0];
        unsigned q;
        lg.decay = 0.9f;
        for (q = 0u; q < 3u; ++q) {
            double t1, t2;
            mello_oliverb_init(&a, ring_a, 5u);
            f_init(&f, ring_f, 5u);
            set_both(&a, &f, &lg);
            noise = 12345u;
            for (n = 0; n < 8 * FS; ++n) {
                int32_t in = burst(n) / down[q], l, r;
                float fl, fr;
                double ql, qr;
                mello_oliverb_step(&a, in, &l, &r, SINE);
                f_step(&f, (float)in / 32768.0f, &fl, &fr);
                ql = (double)(long)(fl * 32768.0f), qr = (double)(long)(fr * 32768.0f);
                e1[n] = (double)l * l + (double)r * r, e2[n] = ql * ql + qr * qr;
            }
            t1 = t20(e1, 8 * FS), t2 = t20(e2, 8 * FS);
            CHECK(t2 > 0.3 && t1 > 0.97 * t2 && t1 < 1.03 * t2,
                  "%d dB down the tail falls 20 dB in %.2f s, the floating point's in %.2f s", 6 * (int)q * 3,
                  t1, t2);
            printf("%2d dB down: the tail's 20 dB in %.2f s (the floating point's %.2f s)\n", 6 * (int)q * 3,
                   t1, t2);
        }
    }

    /* bounded at a decay past 1 (1.3: Parasites' freeze), noise all the way */
    {
        Settings fr = S[1];
        int32_t peak = 0, l, r;
        fr.decay = 1.3f, fr.lp = 1.0f, fr.hp = 0.0f;
        mello_oliverb_init(&a, ring_a, 9u);
        set_both(&a, 0, &fr);
        for (n = 0; n < 10 * FS; ++n) {
            int32_t m;
            noise = noise * 1103515245u + 12345u;
            mello_oliverb_step(&a, ((int32_t)(noise >> 16) - 32768) >> 1, &l, &r, SINE);
            m = l < 0 ? -l : l;
            if (m > peak) peak = m;
        }
        CHECK(peak < 4 * 32768, "decay 1.3: the output reached %d", peak);
        CHECK(peak > 4096, "decay 1.3: the output stayed at %d", peak);
    }

    /* the shimmer: 400 Hz in for half a second; at +12 the tail (0.6 .. 2 s) has far more at 800 Hz than without, and
     * less left at 400 (the feedback is all shifted), at +7 more at 600 Hz */
    {
        static const float semis[3] = {0.0f, 12.0f, 7.0f};
        double p[3][3]; /* [semis][400, 600, 800 Hz] */
        static const double hz[3] = {400.0, 600.0, 800.0};
        int32_t l, r;
        unsigned m, h;
        for (m = 0u; m < 3u; ++m) {
            Settings sh = S[2];
            double c[3], s1[3] = {0, 0, 0}, s2[3] = {0, 0, 0};
            sh.semis = semis[m];
            for (h = 0u; h < 3u; ++h) c[h] = 2.0 * cos(6.283185307 * hz[h] / FS);
            mello_oliverb_init(&a, ring_a, 11u);
            set_both(&a, 0, &sh);
            for (n = 0; n < 2 * FS; ++n) {
                int32_t in = n < FS / 2 ? (int32_t)(8000.0 * sin(6.283185307 * 400.0 * n / FS)) : 0;
                mello_oliverb_step(&a, in, &l, &r, SINE);
                if (n >= FS * 6 / 10) /* Goertzel on the tail */
                    for (h = 0u; h < 3u; ++h) {
                        double s = l + c[h] * s1[h] - s2[h];
                        s2[h] = s1[h], s1[h] = s;
                    }
            }
            for (h = 0u; h < 3u; ++h) p[m][h] = s1[h] * s1[h] + s2[h] * s2[h] - c[h] * s1[h] * s2[h];
        }
        CHECK(p[1][2] > 1000.0 * p[0][2], "+12: 800 Hz %g, without the shimmer %g", p[1][2], p[0][2]);
        CHECK(p[1][0] < 0.5 * p[0][0], "+12: 400 Hz %g left, without %g", p[1][0], p[0][0]);
        CHECK(p[2][1] > 10.0 * p[0][1], "+7: 600 Hz %g, without the shimmer %g", p[2][1], p[0][1]);
    }

    /* the same seed, the same samples */
    {
        int32_t l1, r1, l2, r2;
        int same = 1;
        mello_oliverb_init(&a, ring_a, 21u);
        mello_oliverb_init(&b, ring_b, 21u);
        set_both(&a, 0, &S[1]);
        set_both(&b, 0, &S[1]);
        noise = 12345u;
        for (n = 0; n < FS; ++n) {
            int32_t in = burst(n);
            mello_oliverb_step(&a, in, &l1, &r1, SINE);
            mello_oliverb_step(&b, in, &l2, &r2, SINE);
            if (l1 != l2 || r1 != r2) same = 0;
        }
        CHECK(same, "two of the same seed differ");
    }

    if (fails) return 1;
    puts("PASS: oliverb");
    return 0;
}
