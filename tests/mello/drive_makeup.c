/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Distortion keeps its level (DECISIONS "Distortion keeps its level"): a part played alone, 4 s, its RMS with the
 * drive turned up against the same part with none. The kit's DRIVE (0 .. 100%, with and without FOLD), the kick's
 * TONE, the bass's DRIVE: each within 2 dB of where it started. Built with the output's gain at 0 dB
 * (run_mello.sh), so the limiter cannot hide a rise. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"
#include "mello_transport.h"

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
static MelloTransport t;
static void sink(void *c, const MelloEvent *e, uint32_t due)
{
    (void)mello_audio_push_engine_event((MelloAudio *)c, e, due);
}

enum { KIT, KICK, BASS };

/* The part alone (the lead silent, no effects), `amount` on its drive; RMS in dBFS. */
static double rms_db(int part, uint16_t amount, uint16_t fold)
{
    MelloProject p;
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloDrumConfig d;
    uint32_t ms, i;
    int32_t out[96];
    double ss = 0;
    unsigned long n = 0;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 5u, 0, 0);
    s = t.engine.sound;
    s.lead_level = 0u;
    mello_engine_sound_set_config(&t.engine, &s);
    fx = t.engine.fx;
    fx.delay_wet = fx.reverb_wet = 0u;
    mello_engine_fx_set_config(&t.engine, &fx);
    d = t.engine.drums;
    d.on = part != BASS;
    d.length = 16u;
    d.hits[MELLO_DRUM_KICK] = 0x1111u;
    d.hits[MELLO_DRUM_SNARE] = part == KIT ? 0x4444u : 0u;
    d.hits[MELLO_DRUM_HAT] = part == KIT ? 0xAAAAu : 0u;
    d.hits[MELLO_DRUM_GLITCH] = 0u;
    if (part == KIT) d.drive = amount, d.fold = fold;
    if (part == KICK) d.voice[MELLO_DRUM_KICK].tone = amount;
    mello_engine_drums_set_config(&t.engine, &d);
    sub = t.engine.sub;
    sub.on = part == BASS;
    sub.drive = part == BASS ? amount : 0u;
    mello_engine_sub_set_config(&t.engine, &sub);
    if (part == BASS) {
        MelloBassConfig b = t.engine.bass;
        b.on = true;
        mello_engine_bass_set_config(&t.engine, &b);
        mello_engine_bass_roll(&t.engine);
    }
    mello_audio_init(&a, db, 65536, rb, 24576);
    mello_audio_set_config(&a, &t.engine.sound, &t.engine.fx, &t.engine.sub, &t.engine.sample);
    mello_audio_set_drum_config(&a, &t.engine.drums);
    mello_audio_set_bass_config(&a, &t.engine.bass);
    mello_transport_set_event_sink(&t, sink, &a);
    mello_transport_start(&t, 0u);
    for (ms = 0; ms < 4000u; ++ms) {
        uint32_t fr = 44u + (ms % 10u == 0u ? 1u : 0u);
        mello_transport_poll_ahead(&t, ms * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, fr, ms * 1000u, t.engine.project.bpm);
        for (i = 0; i < fr * 2u; ++i) ss += (double)out[i] * out[i], n++;
    }
    return 10.0 * log10(ss / (double)n / (32768.0 * 32768.0) + 1e-20);
}

int main(void)
{
    static const uint16_t amounts[4] = {250u, 500u, 750u, 1000u};
    static const char *names[3] = {"kit DRIVE", "kick TONE", "bass DRIVE"};
    int part, k, f;
    for (part = KIT; part <= BASS; ++part)
        for (f = 0; f < (part == KIT ? 2 : 1); ++f) {
            uint16_t fold = f ? 600u : 0u;
            double base = rms_db(part, 0u, 0u);
            printf("%-10s%s at 0: %6.1f dBFS;", names[part], f ? " +FOLD" : "", base);
            CHECK(base > -60.0, "%s: nothing sounds (%.1f dBFS)", names[part], base);
            for (k = 0; k < 4; ++k) {
                double r = rms_db(part, amounts[k], fold) - base;
                printf(" %u%%: %+.1f", amounts[k] / 10u, r);
                CHECK(r < 2.0 && r > -2.0, "%s%s at %u%%: %+.1f dB against 0", names[part], f ? " with FOLD" : "",
                      amounts[k] / 10u, r);
            }
            printf("\n");
        }
    if (fails) return 1;
    puts("PASS: drive_makeup");
    return 0;
}
