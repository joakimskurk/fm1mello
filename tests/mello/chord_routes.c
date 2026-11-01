/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The chords move too (DECISIONS.md): C.CUT and C.LEVEL are modulation destinations for EVOLVE's routes (and DOT's
 * MOD lanes). A route on C.CUT with a negative depth closes the chords' filter as the register rises (the inversion
 * asked for), and a route on C.LEVEL at -100% with the register high takes the chords out of the mix. The chord
 * voice sounds three notes with the lead silent, so what is measured is the chords. */
#include "mello_audio.h"
#include "mello_engine.h"
#include <math.h>
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
static MelloEngine e;
static int16_t db[4096], rb[24576];
static int32_t out[512];

/* The chords on with three notes, the routes given, EVOLVE's value held at tm: rendered a while; the RMS back. */
static double sound(const MelloModRouting *m, int16_t tm, int32_t *f_q15)
{
    static const uint8_t notes[3] = {60u, 63u, 67u};
    MelloChordConfig c;
    uint32_t us = 0u, blk;
    double sum = 0.0;
    uint32_t n = 0u;
    uint8_t k;
    mello_audio_init(&a, db, 4096u, rb, 24576u);
    mello_chord_defaults(&c);
    c.on = true;
    c.level = 700u;
    mello_audio_set_chord_config(&a, &c);
    mello_audio_set_mod_routing(&a, m);
    for (blk = 0u; blk < 60u; ++blk) {
        if (blk == 2u)
            for (k = 0u; k < 3u; ++k) {
                MelloEvent ev;
                memset(&ev, 0, sizeof ev);
                ev.type = MELLO_EVENT_CHORD_NOTE_ON;
                ev.note = notes[k];
                ev.velocity = 100u;
                ev.ratchet_index = k;
                (void)mello_audio_push_engine_event(&a, &ev, us);
            }
        a.tm_value = tm; /* EVOLVE's register, as the engine's events would set it */
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, 256u, us, 120u);
        if (blk >= 30u) {
            uint32_t i;
            for (i = 0u; i < 512u; ++i) sum += (double)out[i] * out[i];
            n += 512u;
        }
        us += 5805u;
    }
    if (f_q15) *f_q15 = a.chord.f_q15;
    return sqrt(sum / n);
}

int main(void)
{
    MelloProject p;
    MelloModRouting plain, inverted, silent;
    int32_t f_plain, f_closed;
    double rms_plain, rms_closed, rms_silent;
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 1u);
    plain = e.modr;
    inverted = plain; /* A: the lead's CUTOFF up, B: the chords' CUT down, as the register rises */
    inverted.tm_dest[0] = MELLO_DEST_CUTOFF;
    inverted.tm_depth[0] = 1000;
    inverted.tm_dest[1] = MELLO_DEST_CHORD_CUT;
    inverted.tm_depth[1] = -1000;
    silent = plain;
    silent.tm_dest[0] = MELLO_DEST_CHORD_LEVEL;
    silent.tm_depth[0] = -1000;

    rms_plain = sound(&plain, 0, &f_plain);
    CHECK(rms_plain > 200.0, "the chords make no sound (rms %.0f)", rms_plain);
    rms_closed = sound(&inverted, 24000, &f_closed);
    CHECK(a.chord_mod_cut < -600, "C.CUT at -100%% with the register at 24000 should pull hard: %d",
          a.chord_mod_cut);
    CHECK(f_closed < f_plain, "the chords' filter did not close (%d vs %d)", (int)f_closed, (int)f_plain);
    CHECK(rms_closed < rms_plain, "a closed filter should leave less sound (%.0f vs %.0f)", rms_closed,
          rms_plain);
    rms_silent = sound(&silent, 32000, 0);
    CHECK(a.chord_mod_level < -900, "C.LEVEL at -100%% with the register high: %d", a.chord_mod_level);
    CHECK(rms_silent < rms_plain / 10.0, "C.LEVEL at -100%% should all but silence the chords (%.0f vs %.0f)",
          rms_silent, rms_plain);
    printf("PASS: chords rms %.0f; with EVOLVE on C.CUT inverted the filter goes %d -> %d and the rms to "
           "%.0f; on "
           "C.LEVEL -100%% the rms is %.0f\n",
           rms_plain, (int)f_plain, (int)f_closed, rms_closed, rms_silent);
    return 0;
}
