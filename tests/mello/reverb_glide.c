/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The reverb's lines never jump (DECISIONS "The reverb glides"). Heard in the 0.5-f80 demo: clicks going round the
 * reverb. Two causes: the modulation's depth was unsigned, so with its sine below zero a line read at its longest
 * (a jump of ~130 samples, twice a cycle, on every line); and a SIZE changed while the tail rang moved every read at
 * once. A held note into the reverb, SIZE 700 -> 400 halfway: no line's read moves more than 2 samples in any
 * millisecond, each stays within its swing (10 samples) of where SIZE puts it once it has glided there, and the
 * tail's first difference has no lone spikes. */
#include <math.h>
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

static int16_t db[65536], rb[24576];
static MelloAudio a;

int main(void)
{
    MelloSoundConfig s;
    MelloFxConfig f;
    MelloSubConfig sub;
    MelloSampleConfig smp;
    MelloEvent ev;
    uint16_t was[8] = {0};
    uint32_t ms, i, worst = 0u, worst_ms = 0u, far = 0u;
    int32_t out[96], prev = 0, prev2 = 0;
    double e_sum = 0.0, e_max = 0.0;
    uint32_t e_n = 0u;
    mello_sound_defaults(&s);
    memset(&f, 0, sizeof f);
    f.reverb_wet = 800, f.reverb_size = 700, f.reverb_decay_ms = 6000, f.reverb_level = 1000,
    f.reverb_tone = 620;
    f.dist_level = 1000, f.send_rev[0] = 1000;
    mello_sub_defaults(&sub);
    sub.on = false;
    memset(&smp, 0, sizeof smp);
    s.attack_ms = 5u, s.decay_ms = 2000u, s.sustain_permille = 800u, s.release_ms = 200u;
    mello_audio_init(&a, db, 65536, rb, 24576);
    mello_audio_set_config(&a, &s, &f, &sub, &smp);
    memset(&ev, 0, sizeof ev);
    ev.type = MELLO_EVENT_NOTE_ON, ev.note = 57u, ev.velocity = 110u, ev.voice = MELLO_VOICE_LEAD;
    (void)mello_audio_push_engine_event(&a, &ev, 0u);
    for (ms = 0u; ms < 6000u; ++ms) {
        uint32_t fr = 44u + (ms % 10u == 0u ? 1u : 0u);
        if (ms == 2500u) {
            ev.type = MELLO_EVENT_NOTE_OFF, ev.velocity = 0u;
            (void)mello_audio_push_engine_event(&a, &ev, ms * 1000u);
        }
        if (ms == 3000u) { /* SIZE (and DECAY) as a STYLE roll changes them, the tail ringing */
            f.reverb_size = 400u, f.reverb_decay_ms = 3000u;
            mello_audio_set_config(&a, &s, &f, &sub, &smp);
        }
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, fr, ms * 1000u, 120u);
        for (i = 0u; i < 8u; ++i) {
            uint16_t di = a.reverb_mod_di[i];
            uint32_t d = was[i] ? (uint32_t)(di > was[i] ? di - was[i] : was[i] - di) : 0u;
            if (d > worst) worst = d, worst_ms = ms;
            was[i] = di;
            if (ms > 5500u) { /* glided there: within the swing (and a sample) of SIZE's length */
                int32_t off = (int32_t)di - (int32_t)a.reverb_line_delay[i];
                if (off > 12 || off < -12) far++;
            }
        }
        for (i = 0u; i < fr; ++i) { /* the tail after the change: a lone spike in its second difference */
            int32_t x = out[2u * i], e2 = x - 2 * prev + prev2;
            prev2 = prev, prev = x;
            if (ms < 3200u || ms >= 3700u) continue;
            e_sum += (double)e2 * e2, e_n++;
            if (fabs((double)e2) > e_max) e_max = fabs((double)e2);
        }
    }
    CHECK(worst <= 2u, "a reverb line's read jumped %u samples at %u ms", worst, worst_ms);
    CHECK(!far, "%u reads past the swing of where SIZE puts the lines", far);
    CHECK(e_n && e_max < 8.0 * sqrt(e_sum / e_n),
          "a click in the tail after SIZE changed: %.0f against %.0f rms", e_max,
          sqrt(e_sum / (e_n ? e_n : 1u)));
    printf("reverb_glide: the reads move at most %u samples a millisecond; the tail's peak residual %.1fx "
           "its rms\n",
           worst, e_max / sqrt(e_sum / (e_n ? e_n : 1u)));
    if (fails) return 1;
    puts("PASS: reverb_glide");
    return 0;
}
