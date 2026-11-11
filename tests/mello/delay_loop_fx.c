/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* DIFFUSE and PITCH (FX > DELAY LOOP, ROADMAP H7, DECISIONS "The looping delay"): Clouds' diffuser and pitch shifter
 * on the delay's repeats, in its loop. Their memory is the reverb's after what either reverb uses (V3 and Oliverb at
 * every PRE end before it); off on a new project; PITCH +12 puts the first echo an octave up and the second two; -7 a
 * fifth down; DIFFUSE spreads a click's echo over tens of milliseconds at about its energy; with FDBK at 95% and
 * every one up the repeats stay bounded and die away to nothing (an allpass's output cut a product at a time went
 * round for ever at -12); and with a smaller reverb memory there is none, and the delay is as it was. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"

#if !defined(MELLO_REVERB_TEST_HOOK) || !MELLO_REVERB_TEST_HOOK
#error "delay_loop_fx needs MELLO_REVERB_TEST_HOOK=1"
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

#define FS 44100
static int16_t delay_buf[MELLO_AUDIO_DELAY_SAMPLES], reverb_buf[MELLO_AUDIO_REVERB_SAMPLES];
static MelloAudio a;
static MelloSoundConfig sound;
static MelloFxConfig fx;
static MelloSubConfig sub;
static MelloSampleConfig sample;

static int no_run(void (*fn)(uint32_t), uint32_t arg)
{
    (void)fn, (void)arg;
    return 0;
}
static int no_wait(void)
{
    return 0;
}
static void no_sync(void) {}
static const MelloAudioCores CORES = {no_run, no_wait, no_sync, 0};

static void start(uint32_t reverb_words)
{
    int32_t out[2];
    mello_audio_init(&a, delay_buf, MELLO_AUDIO_DELAY_SAMPLES, reverb_buf, reverb_words);
    mello_audio_set_config(&a, &sound, &fx, &sub, &sample);
    mello_audio_render(&a, out, 1u, 0u, 80u);
}

/* the energy at hz over n samples of x (Goertzel) */
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

static int32_t ret[3 * FS];

/* a 0.1 s sine at 400 Hz into the delay (200 ms, FDBK 50%), its return for 0.6 s; each echo's energy at 400, 800,
 * 1600, 267 Hz */
static void echoes(int8_t pitch, double e[2][4])
{
    static const double hz[4] = {400.0, 800.0, 1600.0, 400.0 * 0.6674};
    uint32_t n, k;
    int32_t r;
    fx.delay_pitch = pitch, fx.delay_diffuse = 0u;
    start(MELLO_AUDIO_REVERB_SAMPLES);
    for (n = 0u; n < 3u * FS / 5u; ++n) {
        int32_t in = n < FS / 10u ? (int32_t)(8000.0 * sin(6.283185307179586 * 400.0 * n / FS)) : 0;
        ret[n] = mello_audio_delay_probe(&a, in, 2048, &r);
    }
    for (k = 0u; k < 4u; ++k) {
        /* the first echo 0.2 .. 0.3 s, the second 0.4 .. 0.5 s (a window inside each, past the shifter's grains) */
        e[0][k] = tone(ret + FS / 5u + FS / 50u, FS / 20u, hz[k]);
        e[1][k] = tone(ret + 2u * FS / 5u + FS / 50u, FS / 20u, hz[k]);
    }
}

int main(void)
{
    mello_sound_defaults(&sound);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sample);
    fx.delay_sync = false, fx.delay_time_ms = 200u, fx.delay_lowcut = 0u, fx.delay_tone = 1000u;

    /* the memory: after either reverb's, and off by default */
    {
        uint16_t pre;
        start(MELLO_AUDIO_REVERB_SAMPLES);
        CHECK(a.dlyfx == reverb_buf + MELLO_AUDIO_DLYFX_AT && a.reverb_room == MELLO_AUDIO_DLYFX_AT,
              "the delay's memory at %ld, the reverbs' room %u", (long)(a.dlyfx ? a.dlyfx - reverb_buf : -1),
              a.reverb_room);
        CHECK(!a.dlyfx_diffuse && !a.dlyfx_shift, "a new project's DIFFUSE or PITCH on");
        for (pre = 0u; pre <= 120u; pre += 30u) {
            uint32_t end;
            fx.reverb_pre_ms = pre;
            start(MELLO_AUDIO_REVERB_SAMPLES);
            end = a.reverb_line_offset[7] + a.reverb_line_cap[7];
            CHECK(a.reverb_pre_cap && end + (end + 1u) / 2u <= MELLO_AUDIO_DLYFX_AT,
                  "PRE %u: V3 ends at %u, past %u", pre, end + (end + 1u) / 2u, MELLO_AUDIO_DLYFX_AT);
            mello_audio_set_cores(&a, &CORES);
            mello_audio_reverb_choose_probe(&a);
            CHECK(a.oliverb_on && a.reverb_pre_cap + MELLO_OLIVERB_WORDS <= MELLO_AUDIO_DLYFX_AT,
                  "PRE %u: Oliverb ends at %u, past %u", pre, a.reverb_pre_cap + MELLO_OLIVERB_WORDS,
                  MELLO_AUDIO_DLYFX_AT);
        }
        fx.reverb_pre_ms = 12u;
    }

    /* PITCH: +12 an octave a repeat, -7 a fifth down */
    {
        double off[2][4], up[2][4], dn[2][4];
        echoes(0, off);
        echoes(12, up);
        echoes(-7, dn);
        CHECK(off[0][0] > 100.0 * off[0][1] && off[1][0] > 100.0 * off[1][1],
              "PITCH off: the echoes not at 400 Hz (%g / %g)", off[0][0], off[0][1]);
        CHECK(up[0][1] > 30.0 * up[0][0], "PITCH +12: the first echo's 800 Hz %g, 400 Hz %g", up[0][1],
              up[0][0]);
        CHECK(up[1][2] > 30.0 * up[1][1] && up[1][2] > 30.0 * up[1][0],
              "PITCH +12: the second echo's 1600 Hz %g, 800 Hz %g, 400 Hz %g", up[1][2], up[1][1], up[1][0]);
        CHECK(dn[0][3] > 30.0 * dn[0][0], "PITCH -7: the first echo's 267 Hz %g, 400 Hz %g", dn[0][3],
              dn[0][0]);
        printf("PITCH +12: the first echo 800 Hz %.1f dB over 400; the second 1600 Hz %.1f dB over 800\n",
               10.0 * log10(up[0][1] / up[0][0]), 10.0 * log10(up[1][2] / up[1][1]));
    }

    /* DIFFUSE: a click's echo spread over tens of ms, its energy about the same */
    {
        static const uint16_t amt[2] = {0u, 1000u};
        double energy[2], peak[2], spread[2];
        unsigned m;
        uint32_t n;
        int32_t r;
        for (m = 0u; m < 2u; ++m) {
            double e = 0.0, acc = 0.0, pk = 0.0;
            uint32_t t90 = 0u;
            fx.delay_pitch = 0, fx.delay_diffuse = amt[m];
            start(MELLO_AUDIO_REVERB_SAMPLES);
            for (n = 0u; n < FS / 2u; ++n) ret[n] = mello_audio_delay_probe(&a, n == 0u ? 20000 : 0, 0, &r);
            for (n = 0u; n < FS / 2u; ++n) e += (double)ret[n] * ret[n];
            for (n = 0u; n < FS / 2u && !t90; ++n) {
                acc += (double)ret[n] * ret[n];
                if (fabs((double)ret[n]) > pk) pk = fabs((double)ret[n]);
                if (acc >= 0.9 * e) t90 = n;
            }
            for (; n < FS / 2u; ++n)
                if (fabs((double)ret[n]) > pk) pk = fabs((double)ret[n]);
            energy[m] = e, peak[m] = pk;
            spread[m] = 1000.0 * (double)(t90 - 8820u) / FS; /* (the echo at 200 ms) */
        }
        CHECK(spread[0] < 1.0 && spread[1] > 20.0,
              "DIFFUSE: 90%% of the echo in %.1f ms (off) and %.1f ms (100%%)", spread[0], spread[1]);
        CHECK(energy[1] > 0.7 * energy[0] && energy[1] < 1.3 * energy[0],
              "DIFFUSE 100%%: the echo's energy %.2f of without", energy[1] / energy[0]);
        CHECK(peak[1] < 0.5 * peak[0], "DIFFUSE 100%%: its peak %.0f, without %.0f", peak[1], peak[0]);
        printf("DIFFUSE 100%%: a click's echo over %.1f ms, its energy %.2f, its peak %.2f of without\n",
               spread[1], energy[1] / energy[0], peak[1] / peak[0]);
    }

    /* every one up at FDBK 95%: bounded, and to nothing */
    {
        static const int8_t ps[3] = {12, -12, 24};
        unsigned q;
        for (q = 0u; q < 3u; ++q) {
            uint32_t n, seed = 99u;
            int32_t r, l, peak = 0, last = 0;
            fx.delay_pitch = ps[q], fx.delay_diffuse = 1000u;
            start(MELLO_AUDIO_REVERB_SAMPLES);
            for (n = 0u; n < 40u * FS; ++n) {
                int32_t in = 0, m;
                if (n < FS / 2u)
                    seed = seed * 1664525u + 1013904223u, in = ((int32_t)(seed >> 20) - 2048) * 8;
                l = mello_audio_delay_probe(&a, in, 3891, &r);
                m = l < 0 ? -l : l;
                if (m > peak) peak = m;
                if (n >= 39u * FS && m > last) last = m;
            }
            /* (the line is 16-bit, a full one clips; what comes back of it may go a little past, not on) */
            CHECK(peak < 2 * 32767, "PITCH %+d, DIFFUSE 100%%, FDBK 95%%: the return reached %d", ps[q],
                  peak);
            CHECK(last == 0, "PITCH %+d, DIFFUSE 100%%, FDBK 95%%: 39 s on, still %d", ps[q], last);
        }
    }

    /* a smaller reverb memory: none, and the delay as it was */
    {
        uint32_t n;
        int32_t r, x, y;
        int same = 1;
        static int32_t plain[FS / 2u];
        fx.delay_pitch = 0, fx.delay_diffuse = 0u;
        start(4096u);
        for (n = 0u; n < FS / 2u; ++n) plain[n] = mello_audio_delay_probe(&a, n < 100u ? 9000 : 0, 2048, &r);
        fx.delay_pitch = 12, fx.delay_diffuse = 1000u;
        start(4096u);
        CHECK(!a.dlyfx && !a.dlyfx_shift && !a.dlyfx_diffuse, "a smaller reverb memory: the delay's taken");
        for (n = 0u; n < FS / 2u; ++n) {
            x = mello_audio_delay_probe(&a, n < 100u ? 9000 : 0, 2048, &r);
            y = plain[n];
            if (x != y) same = 0;
        }
        CHECK(same, "a smaller reverb memory: DIFFUSE and PITCH changed the delay");
    }
    if (fails) return 1;
    puts("PASS: delay_loop_fx");
    return 0;
}
