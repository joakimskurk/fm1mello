/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* P1 regression (review 2026-10, P1-1): after a note on the factory patch (reverb on), the audio must
 * report silent again once the tail has died. dinghy.c only autosaves, SAVEs and accepts sample uploads
 * while stopped and silent; a reverb-return DC of about -150 used to keep the tail guard armed forever. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"

#define FRAMES 256u
static int16_t delay_buf[4096];
static int16_t reverb_buf[24576];

/* Seconds from note-off until mello_audio_is_silent(), or -1 if not within 20 s. Also returns the
 * largest |output| over the last 0.5 s of that wait. */
static double silent_after(uint16_t reverb_wet, uint16_t delay_wet, int32_t *end_peak)
{
    static MelloAudio a;
    static int32_t out[FRAMES * 2u];
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    uint32_t blk, us = 0u, off_blk = 2u + 22050u / FRAMES, blocks = off_blk + 20u * 44100u / FRAMES;
    double found = -1.0;

    mello_sound_defaults(&s);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sm);
    fx.reverb_wet = reverb_wet;
    fx.delay_wet = delay_wet;
    mello_audio_init(&a, delay_buf, 4096u, reverb_buf, 24576u);
    mello_audio_set_config(&a, &s, &fx, &sub, &sm);
    *end_peak = 0;
    for (blk = 0u; blk < blocks; ++blk) {
        uint32_t i;
        if (blk == 2u || blk == off_blk) {
            MelloEvent e;
            memset(&e, 0, sizeof e);
            e.type = blk == 2u ? MELLO_EVENT_NOTE_ON : MELLO_EVENT_NOTE_OFF;
            e.note = 48u;
            e.velocity = 110u;
            mello_audio_push_engine_event(&a, &e, us);
        }
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, FRAMES, us, 80u);
        us += FRAMES * 1000000u / 44100u;
        if (blk > off_blk && found < 0.0 && mello_audio_is_silent(&a))
            found = (double)(blk - off_blk) * FRAMES / 44100.0;
        if (blk + 22050u / FRAMES >= blocks)
            for (i = 0u; i < FRAMES * 2u; ++i) {
                int32_t v = out[i] < 0 ? -out[i] : out[i];
                if (v > *end_peak) *end_peak = v;
            }
    }
    return found;
}

int main(void)
{
    static const struct {
        const char *name;
        uint16_t reverb_wet, delay_wet;
        /* The tail guard holds for one reverb decay (factory 6 s) after the last sample above +-8, and the
         * tail itself takes about one decay to get there: 2 x 6 s plus slack. */
        double limit_s;
    } cases[] = {
        {"factory FX (reverb + delay)", 240u, 120u, 13.0},
        {"reverb only", 240u, 0u, 13.0},
        {"delay only", 0u, 120u, 3.0},
    };
    unsigned i, failed = 0u;
    for (i = 0u; i < sizeof cases / sizeof cases[0]; ++i) {
        int32_t peak;
        double t = silent_after(cases[i].reverb_wet, cases[i].delay_wet, &peak);
        int ok = t >= 0.0 && t <= cases[i].limit_s && peak <= 8;
        printf("%-30s silent after %6.2f s (limit %.1f), end peak %5d %s\n", cases[i].name, t,
               cases[i].limit_s, (int)peak, ok ? "ok" : "FAIL");
        if (!ok) failed++;
    }
    if (failed) {
        printf("FAIL: %u case(s) never became silent: autosave and SAVE would never run\n", failed);
        return 1;
    }
    printf("PASS: silent again after the tail\n");
    return 0;
}
