/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* A chord left hanging (the product owner, 2026-11-12: "en akkord henger når jeg trykker randomize og forblir i
 * bakgrunn til jeg stopper og starter igjen"): HOLD holds its chord until the next one releases it, so when a roll
 * (RNDMZR, a STYLE roll) or UNDO turns the chords to STAB, an ARP, PATTERN or WALK, or off, what HOLD held must be let
 * go then, not at STOP. Here the band plays and rolls into a new style every bar, eight seeds, 48 bars each: a pulse
 * after the chords are no longer HOLD (or off), no chord note the engine struck is still unreleased; in HOLD no more
 * are than one chord's voices. (The audio lets a voice go at its note's release, and kills them all when the chords
 * go off: chord_note_off, chord_kill.) */
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
static MelloProject p;
static int held[128]; /* chord notes struck and not yet released, as the engine sends them */
static void sink(void *ctx, const MelloEvent *e, uint32_t due)
{
    if (e->type == MELLO_EVENT_CHORD_NOTE_ON) held[e->note & 127u]++;
    if (e->type == MELLO_EVENT_CHORD_NOTE_OFF && held[e->note & 127u] > 0) held[e->note & 127u]--;
    (void)mello_audio_push_engine_event((MelloAudio *)ctx, e, due);
}
static int unreleased(void)
{
    int k, n = 0;
    for (k = 0; k < 128; ++k) n += held[k];
    return n;
}
static void sync_audio(void)
{
    MelloEngine *e = &t.engine;
    mello_audio_set_drum_config(&a, &e->drums);
    mello_audio_set_bass_config(&a, &e->bass);
    mello_audio_set_config(&a, &e->sound, &e->fx, &e->sub, &e->sample);
    mello_audio_set_mod_routing(&a, &e->modr);
    mello_audio_set_chord_config(&a, &e->chord);
}

int main(void)
{
    static int32_t out[2 * 64];
    uint32_t seed, ms, worst_off = 0u, worst_hold = 0u, checks = 0u;
    for (seed = 1u; seed <= 8u; ++seed) {
        uint32_t bar_ms = 2000u, next_roll = 500u, settle = 0u, roll_no = 0u;
        memset(held, 0, sizeof held);
        mello_project_defaults(&p);
        p.bpm = 120u;
        mello_transport_init(&t, &p, seed * 7919u, 0, 0);
        mello_transport_set_bpm(&t, 120u);
        mello_audio_init(&a, db, 65536, rb, 24576);
        mello_engine_reseed(&t.engine, mello_seed_of((uint16_t)seed));
        (void)mello_engine_style_roll(&t.engine, MELLO_STYLE_ANY);
        sync_audio();
        mello_transport_set_event_sink(&t, sink, &a);
        mello_transport_start(&t, 0u);
        for (ms = 0u; ms < 48u * bar_ms; ++ms) {
            uint32_t fr = 44u + (ms % 10u == 0u ? 1u : 0u);
            if (ms == next_roll) { /* a roll mid-bar, as a hand would: the band into a new style */
                (void)mello_engine_style_roll(&t.engine, MELLO_STYLE_ANY);
                next_roll += bar_ms + (uint32_t)(roll_no++ % 3u) * 250u;
                settle = ms + 300u; /* (a pulse is 125 ms: the next one has let go) */
            }
            mello_transport_poll_ahead(&t, ms * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
            if (!(ms & 7u)) sync_audio();
            if (ms == settle) {
                const MelloChordConfig *c = &t.engine.chord;
                uint32_t u = (uint32_t)unreleased();
                checks++;
                if (!c->on || c->mode != MELLO_CHORD_HOLD) {
                    if (u > worst_off) worst_off = u;
                } else if (u > worst_hold)
                    worst_hold = u;
            }
            memset(out, 0, sizeof out);
            mello_audio_render(&a, out, fr, ms * 1000u, t.engine.project.bpm);
        }
    }
    CHECK(worst_off == 0u, "a pulse after a roll left HOLD: %u chord notes never released", worst_off);
    CHECK(worst_hold <= 4u, "in HOLD: %u chord notes held (a chord has 4 voices at most)", worst_hold);
    printf("%u rolls checked: unreleased off HOLD %u, in HOLD at most %u\n", checks, worst_off, worst_hold);
    if (fails) return 1;
    puts("PASS: chord_roll_release");
    return 0;
}
