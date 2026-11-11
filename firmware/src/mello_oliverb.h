// SPDX-License-Identifier: MIT
//
// Copyright 2014 Emilie Gillet.
// Oliverb: Matthias Puech, in Parasites (github.com/mqtthiqs/parasites, clouds/dsp/fx/oliverb.h), an alternative
// firmware for Mutable Instruments Clouds.
// This fixed-point C port for SkurkyMello (no floating point, no divide but where the settings change):
// Copyright (C) 2026 joakimskurk, under the same terms.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// -----------------------------------------------------------------------------
//
// Oliverb: the Griesinger topology of Dattorro's paper (four allpass diffusers on the input, then a loop of two
// halves, each two allpasses and a delay), the diffusers' and the loop's delays swung by nine random LFOs, and a
// pitch shifter in the loop's feedback (the shimmer). As Parasites' at 32 kHz, here at the reverb network's 22.05
// kHz (the render's half rate): every delay scaled to the same time, the LFOs to the same rate. The arithmetic:
// samples as the render's (32768 = 1.0), gains Q15 (above 1.0 allowed: decay goes to 1.3), every product towards
// zero (so no error in the loop holds itself up), the accumulator and the one-poles with 8 bits under a sample (so a
// quiet tail is not cut short, nor stopped in a dead band), SoftLimit from a table, the Hermite reads in 64 bits.
#ifndef MELLO_OLIVERB_H
#define MELLO_OLIVERB_H
#include <stdint.h>

#define MELLO_OLIVERB_RING 16384u /* its delay memory, samples (a power of two): 10329 used */

/* the lines, Parasites' lengths x 22050 / 32000: ap1..ap4, dap1a, dap1b, del1, dap2a, dap2b, del2 */
enum {
    OLV_AP1,
    OLV_AP2,
    OLV_AP3,
    OLV_AP4,
    OLV_DAP1A,
    OLV_DAP1B,
    OLV_DEL1,
    OLV_DAP2A,
    OLV_DAP2B,
    OLV_DEL2,
    OLV_LINES
};
static const uint16_t OLV_LEN[OLV_LINES] = {78u, 112u, 166u, 275u, 863u, 1198u, 2350u, 1043u, 939u, 3295u};

typedef struct {         /* Parasites' RandomOscillator: a raised-cosine glide to the next random value */
    uint32_t phase, inc; /* Q24 of a glide */
    int32_t value, next; /* Q15 */
    uint8_t up;
} MelloOliverbLfo;

typedef struct {
    int16_t *ring;
    uint16_t base[OLV_LINES];
    uint32_t wp; /* the write pointer: one down each step, so offset k of a line is k steps back */
    int32_t lp1, lp2, hp1, hp2; /* the accumulator's units */
    int32_t smooth;             /* SIZE as it glides, Q24 (in Q16 it stopped short of SIZE) */
    uint32_t ps_phase;          /* the shimmer's window, Q24 of its length */
    int32_t ps_step;            /* its move a step, Q24 (worked out every 16 steps) */
    uint8_t tick;
    MelloOliverbLfo lfo[9];
    uint32_t rng; /* stmlib's Random: the same LCG */
    /* the settings (mello_oliverb_set) */
    int32_t size_q16, kap, decay, in_gain, lp, hp, psa; /* Q15 */
    int32_t mod_q16;                                    /* the swing, samples Q16 */
    uint32_t slope_q24;                                 /* the LFOs' rate */
    uint32_t ratio_q16;                                 /* the shimmer's pitch ratio */
} MelloOliverb;

static uint32_t olv_rand(MelloOliverb *o)
{
    return o->rng = o->rng * 1664525u + 1013904223u;
}
/* a x b, Q15, towards zero (as the render's mul_q12): a floor holds -1 LSB in a loop for ever, and the tail never
 * ends (DECISIONS "Room to run"); towards zero every loop's error makes it smaller */
static int32_t olv_q15(int32_t a, int32_t b)
{
    return (int32_t)(((int64_t)a * b) / 32768);
}
/* sin^2 (pi t / 2) = (1 - cos (pi t)) / 2, t Q16 (0 .. 65536): Parasites' raised cosine and grain window */
static int32_t olv_raised_cos(int32_t t_q16, const int16_t sine_q15[256])
{
    uint32_t p = (uint32_t)t_q16 * 128u + (64u << 16); /* pi t + pi / 2 in 256ths of a turn, Q16 */
    int32_t i = (int32_t)((p >> 16) & 255u), f = (int32_t)(p & 0xFFFFu) >> 1, s0 = sine_q15[i],
            s1 = sine_q15[(i + 1) & 255];
    int32_t c = s0 + (int32_t)(((int64_t)(s1 - s0) * f) >> 15);
    return (32768 - c) >> 1;
}
static void olv_lfo_inc(MelloOliverb *o,
                        MelloOliverbLfo *l) /* slope / |next - value| (the divide: a glide's start) */
{
    int32_t d = l->next - l->value;
    uint32_t inc;
    if (d < 0) d = -d;
    if (d < 1) d = 1;
    inc = (uint32_t)(((uint64_t)o->slope_q24 << 15) / (uint32_t)d);
    l->inc = inc > (1u << 24) ? (1u << 24) : inc;
}
static int32_t olv_lfo_next(MelloOliverb *o, MelloOliverbLfo *l, const int16_t sine_q15[256])
{
    l->phase += l->inc;
    if (l->phase > (1u << 24)) {
        /* the next glide: from where it is to a random point at least 30% of the way to the end it heads for */
        int32_t rnd = 9830 + (int32_t)(((uint64_t)olv_rand(o) * 22938u) >> 32); /* 0.3 .. 1, Q15 */
        l->phase -= 1u << 24;
        l->value = l->next;
        l->up = !l->up;
        l->next =
            l->up ? l->value + olv_q15(32768 - l->value, rnd) : l->value - olv_q15(32768 + l->value, rnd);
        olv_lfo_inc(o, l);
    }
    return l->value + olv_q15(l->next - l->value, olv_raised_cos((int32_t)(l->phase >> 8), sine_q15));
}
/* SoftLimit, x (27 + x^2) / (27 + 9 x^2), from a table over 0 .. 4 (x / 64, Q13): 1.0 at 3, 1.006 at 4 and held
 * there (tools: x = i / 64, round(y x 8192)) */
static const int16_t OLV_SOFT[257] = {
    0,    128,  256,  384,  511,  639,  766,  893,  1019, 1145, 1271, 1396, 1520, 1644, 1767, 1889, 2011,
    2132, 2251, 2370, 2488, 2605, 2721, 2836, 2950, 3062, 3174, 3284, 3393, 3501, 3607, 3712, 3816, 3918,
    4019, 4119, 4217, 4314, 4409, 4503, 4596, 4687, 4776, 4864, 4951, 5036, 5119, 5201, 5282, 5361, 5438,
    5514, 5589, 5662, 5734, 5804, 5873, 5940, 6006, 6070, 6133, 6195, 6255, 6314, 6372, 6428, 6483, 6536,
    6589, 6640, 6689, 6738, 6785, 6832, 6877, 6920, 6963, 7005, 7045, 7084, 7123, 7160, 7196, 7232, 7266,
    7299, 7331, 7363, 7393, 7423, 7452, 7480, 7507, 7533, 7558, 7583, 7607, 7630, 7652, 7674, 7695, 7715,
    7735, 7754, 7773, 7790, 7807, 7824, 7840, 7856, 7870, 7885, 7899, 7912, 7925, 7938, 7949, 7961, 7972,
    7983, 7993, 8003, 8012, 8022, 8030, 8039, 8047, 8055, 8062, 8069, 8076, 8082, 8089, 8095, 8100, 8106,
    8111, 8116, 8121, 8125, 8129, 8133, 8137, 8141, 8144, 8148, 8151, 8154, 8157, 8159, 8162, 8164, 8166,
    8168, 8170, 8172, 8174, 8175, 8177, 8178, 8180, 8181, 8182, 8183, 8184, 8185, 8186, 8186, 8187, 8188,
    8188, 8189, 8189, 8190, 8190, 8190, 8191, 8191, 8191, 8191, 8191, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
    8192, 8193, 8193, 8193, 8193, 8193, 8193, 8194, 8194, 8194, 8195, 8195, 8195, 8196, 8196, 8197, 8197,
    8198, 8198, 8199, 8199, 8200, 8201, 8202, 8202, 8203, 8204, 8205, 8206, 8207, 8208, 8209, 8210, 8211,
    8212, 8213, 8215, 8216, 8217, 8218, 8220, 8221, 8223, 8224, 8226, 8227, 8229, 8231, 8233, 8234, 8236,
    8238, 8240,
};
static int32_t olv_soft_limit(int32_t x) /* the accumulator's units in and out (OLV_ONE = 1.0) */
{
    uint32_t a = (uint32_t)(x < 0 ? -x : x), i = a >> 17, f = a & 0x1FFFFu; /* the table's step, 1/64: 2^17 */
    int32_t y;
    if (i >= 256u)
        y = OLV_SOFT[256] * 1024;
    else
        y = OLV_SOFT[i] * 1024 + (int32_t)(((int64_t)(OLV_SOFT[i + 1] - OLV_SOFT[i]) * 1024 * f) >> 17);
    return x < 0 ? -y : y; /* (Q13 x 1024: the accumulator's) */
}

#define OLV_AT(o, line, k) (o)->ring[((o)->wp + (o)->base[line] + (uint32_t)(k)) & (MELLO_OLIVERB_RING - 1u)]
/* The accumulator carries 8 bits under a sample (OLV_ONE = 1.0 = 32768 x 256), as Parasites' floats carry theirs, so
 * a quiet tail is not cut short at every product: only the memory is 16-bit, as Clouds'. */
#define OLV_ONE               (32768 * 256)
#define OLV_RD(o, line, k)    ((int32_t)OLV_AT(o, line, k) * 256)
#define OLV_WR(o, line, k, v) (OLV_AT(o, line, k) = olv_sat16((v) / 256))
/* Hermite between the line's samples at an offset of Q16 samples (it reads one before and two after, as Clouds') */
static int32_t olv_hermite(const MelloOliverb *o, uint8_t line, int32_t off_q16)
{
    int32_t n = off_q16 >> 16, t = (off_q16 & 0xFFFF) >> 1; /* Q15 */
    int32_t xm1 = OLV_RD(o, line, n - 1), x0 = OLV_RD(o, line, n), x1 = OLV_RD(o, line, n + 1),
            x2 = OLV_RD(o, line, n + 2);
    int32_t c = (x1 - xm1) / 2, v = x0 - x1, w = c + v, a = w + v + (x2 - x0) / 2, b_neg = w + a;
    int64_t y = ((int64_t)a * t) / 32768;
    y = ((y - b_neg) * t) / 32768;
    y = ((y + c) * t) / 32768;
    return (int32_t)y + x0;
}
static int32_t olv_linear(const MelloOliverb *o, uint8_t line, int32_t off_q16)
{
    int32_t n = off_q16 >> 16, f = (off_q16 & 0xFFFF) >> 1, a = OLV_RD(o, line, n),
            b = OLV_RD(o, line, n + 1);
    return a + olv_q15(b - a, f);
}
static int16_t olv_sat16(int32_t x)
{
    return (int16_t)(x > 32767 ? 32767 : x < -32768 ? -32768 : x);
}
/* a line's swung offset: (length - 1) x SIZE + lfo x the swing, within 1 .. length - 1 */
static int32_t olv_offset(const MelloOliverb *o, uint8_t line, int32_t lfo)
{
    int32_t hi = (int32_t)(OLV_LEN[line] - 1u) << 16,
            off = (int32_t)((((int64_t)(OLV_LEN[line] - 1u) * o->smooth) >> 8) +
                            (((int64_t)lfo * o->mod_q16) >> 15));
    return off < (1 << 16) ? (1 << 16) : off > hi ? hi : off;
}

/* ring: MELLO_OLIVERB_RING samples. The settings after: mello_oliverb_set. */
static void mello_oliverb_init(MelloOliverb *o, int16_t *ring, uint32_t seed)
{
    uint8_t i;
    uint16_t b = 0u;
    for (i = 0u; i < OLV_LINES; ++i) o->base[i] = b, b = (uint16_t)(b + OLV_LEN[i] + 1u);
    o->ring = ring;
    {
        uint32_t k;
        for (k = 0u; k < MELLO_OLIVERB_RING; ++k) ring[k] = 0;
    }
    o->wp = 0u;
    o->lp1 = o->lp2 = o->hp1 = o->hp2 = 0;
    o->ps_phase = 0u, o->ps_step = 0, o->tick = 0u;
    o->rng = seed ? seed : 0x4F4C4956u; /* "OLIV" */
    o->slope_q24 = 1u;
    for (i = 0u; i < 9u; ++i) {
        MelloOliverbLfo *l = &o->lfo[i];
        l->phase = 0u, l->value = 0, l->up = 0u;
        l->next = (int32_t)(olv_rand(o) >> 16) - 32768; /* -1 .. 1 */
        olv_lfo_inc(o, l);
    }
    o->smooth = 0; /* (as Parasites': SIZE glides up from nothing) */
}
/* Parasites' settings, ours in: size, diffusion (kap), decay (the loop's gain each half, Q15, up to 1.3), the input's
 * gain, the loop's low-pass and high-pass (one-pole coefficients at this rate, Q15), the swing (samples Q16), the
 * LFOs' slope (Q24 a step), the shimmer's ratio (Q16) and amount (Q15). Out of the audio path: a divide each LFO. */
static void mello_oliverb_set(MelloOliverb *o, int32_t size_q16, int32_t kap, int32_t decay, int32_t in_gain,
                              int32_t lp, int32_t hp, int32_t mod_q16, uint32_t slope_q24, uint32_t ratio_q16,
                              int32_t psa)
{
    uint8_t i;
    o->size_q16 = size_q16, o->kap = kap, o->decay = decay, o->in_gain = in_gain, o->lp = lp, o->hp = hp;
    o->mod_q16 = mod_q16, o->ratio_q16 = ratio_q16, o->psa = psa;
    if (slope_q24 != o->slope_q24) {
        o->slope_q24 = slope_q24 ? slope_q24 : 1u;
        for (i = 0u; i < 9u; ++i) olv_lfo_inc(o, &o->lfo[i]);
    }
}

/* One step of the network (22.05 kHz): in, the input (mono, samples); *l, *r out. */
static void mello_oliverb_step(MelloOliverb *o, int32_t in, int32_t *l, int32_t *r,
                               const int16_t sine_q15[256])
{
    int32_t acc, prev, apout, tri, ps_size, phase, half, g_main, g_ps;
    const int32_t kap = o->kap;
    o->wp = (o->wp - 1u) & (MELLO_OLIVERB_RING - 1u);
    o->smooth +=
        (int32_t)(((int64_t)(o->size_q16 * 256 - o->smooth) * 475) >> 15); /* 0.01 at 32 kHz: 0.0145 */

    /* the shimmer's window: its length from SIZE (Parasites' 128 .. 3410 samples, scaled), its move worked out
     * every 16 steps (a divide), a raised cosine across it */
    ps_size = (88 << 16) + (int32_t)(((int64_t)(2350 - 88) * o->smooth) >> 8); /* Q16 samples */
    if (!(o->tick++ & 15u)) {
        int64_t num = ((int64_t)65536 - (int64_t)o->ratio_q16) * 16777216; /* (1 - ratio) / ps_size, Q24 */
        o->ps_step = (int32_t)(num / ps_size);
    }
    o->ps_phase = (o->ps_phase + (uint32_t)o->ps_step) & 0xFFFFFFu;
    {
        uint32_t ph = o->ps_phase;                                             /* Q24 */
        int32_t t = (int32_t)((ph >= (1u << 23) ? (1u << 24) - ph : ph) >> 7); /* 2 (ph or 1 - ph), Q16 */
        tri = olv_raised_cos(t, sine_q15);
        phase = (int32_t)(((int64_t)ph * ps_size) >> 24);
        half = phase + ps_size / 2;
        if (half >= ps_size) half -= ps_size;
    }

    /* AP1 smeared inside: its tap at 40 (10 + 60 x the engine's LFO, which nothing starts: 0.5) into it at 100 */
    acc = olv_linear(o, OLV_AP1, 1806336); /* 27.56 steps */
    OLV_WR(o, OLV_AP1, 69, acc);

    acc = olv_q15(in * 256, o->in_gain);
    /* the four diffusers */
#define OLV_AP(line, lfo_i, g)                                                                               \
    do {                                                                                                     \
        prev = olv_hermite(o, line, olv_offset(o, line, olv_lfo_next(o, &o->lfo[lfo_i], sine_q15)));         \
        acc += olv_q15(prev, (g));                                                                           \
        OLV_WR(o, line, 0, acc);                                                                             \
        acc = olv_q15(acc, -(g)) + prev;                                                                     \
    } while (0)
#define OLV_AP_FIXED(line, g)                                                                                \
    do {                                                                                                     \
        prev = olv_hermite(o, line, olv_offset(o, line, 0));                                                 \
        acc += olv_q15(prev, (g));                                                                           \
        OLV_WR(o, line, 0, acc);                                                                             \
        acc = olv_q15(acc, -(g)) + prev;                                                                     \
    } while (0)
    OLV_AP(OLV_AP1, 1, kap);
    OLV_AP(OLV_AP2, 2, kap);
    OLV_AP(OLV_AP3, 3, kap);
    OLV_AP(OLV_AP4, 4, kap);
    apout = acc;

    g_main = olv_q15(o->decay, 32768 - o->psa);
    g_ps = olv_q15(o->decay, o->psa);
    /* the first half: del2, swung, and its pitch-shifted feedback */
    acc += olv_q15(olv_hermite(o, OLV_DEL2, olv_offset(o, OLV_DEL2, olv_lfo_next(o, &o->lfo[5], sine_q15))),
                   g_main);
    if (g_ps) {
        acc += olv_q15(olv_hermite(o, OLV_DEL2, phase), olv_q15(tri, g_ps));
        acc += olv_q15(olv_hermite(o, OLV_DEL2, half), olv_q15(32768 - tri, g_ps));
    }
    o->lp1 += olv_q15(acc - o->lp1, o->lp);
    acc = o->lp1;
    o->hp1 += olv_q15(acc - o->hp1, o->hp);
    acc -= o->hp1;
    acc = olv_soft_limit(acc);
    OLV_AP(OLV_DAP1A, 6, -kap);
    OLV_AP_FIXED(OLV_DAP1B, kap);
    OLV_WR(o, OLV_DEL1, 0, acc);
    *l = acc / 128; /* x 2, in samples */

    /* the second half: del1, the same */
    acc = apout;
    acc += olv_q15(olv_hermite(o, OLV_DEL1, olv_offset(o, OLV_DEL1, olv_lfo_next(o, &o->lfo[7], sine_q15))),
                   g_main);
    if (g_ps) {
        acc += olv_q15(olv_hermite(o, OLV_DEL1, phase), olv_q15(tri, g_ps));
        acc += olv_q15(olv_hermite(o, OLV_DEL1, half), olv_q15(32768 - tri, g_ps));
    }
    o->lp2 += olv_q15(acc - o->lp2, o->lp);
    acc = o->lp2;
    o->hp2 += olv_q15(acc - o->hp2, o->hp);
    acc -= o->hp2;
    acc = olv_soft_limit(acc);
    OLV_AP(OLV_DAP2A, 8, kap);
    OLV_AP_FIXED(OLV_DAP2B, -kap);
    OLV_WR(o, OLV_DEL2, 0, acc);
    *r = acc / 128;
#undef OLV_AP
#undef OLV_AP_FIXED
}
#endif
