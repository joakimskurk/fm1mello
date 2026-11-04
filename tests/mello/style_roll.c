/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* STYLE (DECISIONS "STYLE"): RNDMZR rolls the band in a style. Each style has its marks: AMBIENT a drone under a
 * slow pad and no kick (DRONE no drums, JUNGLE fast); DUB, TECHNO, HOUSE and DEEP a kick on every beat and chords
 * stabbed off the beat; ACID a 303 bass on nearly every sixteenth with glides, no chords; MINIMAL few notes; ELECTRO no chords, hats on every sixteenth; IDM rolls; TRIPHOP and BREAKS
 * held chords. Where chords and a bass line play together, the bass's note is in the chord (the chords are read
 * from it, as FIT does). ANY never rolls the style it rolled before and goes round many; UNDO takes a roll back
 * whole (the sound, the effects and the sub too); with RANDOM's mask empty the lead's steps are left alone. */
#include <stdio.h>
#include <string.h>

#include "mello_engine.h"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

static MelloEngine e;

static void fresh(uint32_t seed)
{
    MelloProject p;
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, seed);
}

#if MELLO_FEATURE_CHORD_MACHINE
/* 32 pulses; each half bar the bass's first note must be among the chord notes heard in it (with the chords on) */
static int clashes(void)
{
    unsigned p, bad = 0u;
    uint16_t pcs = 0u;
    int bass = -1, chord_notes = 0;
    mello_engine_stop(&e);
    mello_engine_start(&e);
    for (p = 0u; p < 32u; ++p) {
        MelloEvent ev;
        if (p % 16u == 0u) {
            if (p && bass >= 0 && chord_notes && !(pcs & (1u << bass))) ++bad;
            pcs = 0u, bass = -1, chord_notes = 0;
        }
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev)) {
            if (ev.type == MELLO_EVENT_CHORD_NOTE_ON) pcs |= (uint16_t)(1u << (ev.note % 12u)), ++chord_notes;
            if (ev.type == MELLO_EVENT_SUB_NOTE_ON && bass < 0) bass = ev.note % 12;
        }
    }
    if (bass >= 0 && chord_notes && !(pcs & (1u << bass))) ++bad;
    return (int)bad;
}
#endif

int main(void)
{
    uint8_t st, i, seen[MELLO_STYLE_COUNT];
    uint32_t seed;
    for (seed = 1u; seed <= 6u; ++seed)
        for (st = MELLO_STYLE_AMBIENT; st < MELLO_STYLE_COUNT; ++st) {
            const char *nm = mello_style_name(st);
            unsigned plays = 0u, rolls = 0u;
            fresh(seed);
            CHECK(mello_engine_style_roll(&e, st) == st && e.style_now == st, "%s: not rolled", nm);
            for (i = 0u; i < 16u; ++i) {
                const MelloStep *x = &e.project.steps[i];
                plays += x->gate != MELLO_GATE_REST && !x->skip;
                rolls += x->ratchet >= 3u;
            }
            for (i = 0u; i < 16u; ++i) rolls += e.drums.lock[MELLO_DRUM_HAT][i].ratchet >= 2u;
            switch (st) {
            case MELLO_STYLE_AMBIENT:
            case MELLO_STYLE_DRONE:
            case MELLO_STYLE_JUNGLE:
                CHECK(e.sub.mode == MELLO_SUB_MODE_DRONE && !e.bass.on, "%s: no drone", nm);
                if (st == MELLO_STYLE_AMBIENT) CHECK(e.drums.hits[MELLO_DRUM_KICK] == 0u, "AMBIENT: a kick");
                if (st == MELLO_STYLE_DRONE) CHECK(!e.drums.on, "DRONE: drums");
                if (st == MELLO_STYLE_JUNGLE) CHECK(e.project.bpm >= 164u, "JUNGLE at %u BPM", e.project.bpm);
#if MELLO_FEATURE_CHORD_MACHINE
                CHECK(e.chord.on && e.chord.poly && e.chord.mode == MELLO_CHORD_HOLD && e.chord.sub_root &&
                          e.chord.attack_ms >= 400u,
                      "%s: no slow pad over the drone", nm);
#endif
                break;
            case MELLO_STYLE_DUB:
            case MELLO_STYLE_TECHNO:
            case MELLO_STYLE_HOUSE:
            case MELLO_STYLE_DEEP:
                CHECK(e.drums.hits[MELLO_DRUM_KICK] == 0x1111u, "%s: not four on the floor", nm);
#if MELLO_FEATURE_CHORD_MACHINE
                CHECK(e.chord.on && e.chord.trigger == MELLO_CHORD_TRIGGER_GRID && e.chord.mode == MELLO_CHORD_STAB &&
                          e.chord.grid_cells && (e.chord.grid_cells & 0x1111u) == 0u && e.chord.grid_amount == 1000u,
                      "%s: the chords not stabbed off the beat", nm);
#endif
                break;
            case MELLO_STYLE_ACID: { /* the 303 is the bass: nearly every sixteenth, glides, the filter's squelch */
                unsigned notes = 0u, glides = 0u;
                for (i = 0u; i < 16u; ++i) notes += e.bass.steps[i].on, glides += e.bass.steps[i].on && e.bass.steps[i].glide;
                CHECK(e.bass.on && notes >= 9u && glides >= 1u && e.bass.res >= 600u && e.bass.env >= 600u,
                      "ACID: the bass is not a 303 (%u notes, %u glides, RES %u, ENV %u)", notes, glides, e.bass.res,
                      e.bass.env);
#if MELLO_FEATURE_CHORD_MACHINE
                CHECK(!e.chord.on, "ACID: chords on");
#endif
                break;
            }
            case MELLO_STYLE_ELECTRO:
                CHECK(e.drums.hits[MELLO_DRUM_HAT] == 0xFFFFu, "ELECTRO: the hats not on every sixteenth");
                break;
            case MELLO_STYLE_MINIMAL:
                CHECK(e.drums.hits[MELLO_DRUM_KICK] == 0x1111u && plays <= 9u, "MINIMAL: %u notes", plays);
                break;
            case MELLO_STYLE_IDM: CHECK(rolls >= 1u, "IDM: no rolls"); break;
            default: /* TRIPHOP, BREAKS, OUTRUN */
#if MELLO_FEATURE_CHORD_MACHINE
                CHECK(e.chord.on && e.chord.mode == MELLO_CHORD_HOLD, "%s: no held chords", nm);
                if (st == MELLO_STYLE_OUTRUN) CHECK(e.chord.steps[0].shape == MELLO_CHORD_TRIAD, "OUTRUN: not triads");
#endif
                break;
            }
#if MELLO_FEATURE_CHORD_MACHINE
            if (e.chord.on && e.bass.on && st != MELLO_STYLE_IDM) {
                int bad = clashes();
                CHECK(bad == 0, "%s (seed %u): the bass outside the chord in %d bars", nm, seed, bad);
            }
#endif
        }

    /* ANY: never the same twice running, and many of them */
    fresh(7u);
    memset(seen, 0, sizeof seen);
    {
        uint8_t last = 0u;
        for (i = 0u; i < 40u; ++i) {
            uint8_t r = mello_engine_style_roll(&e, MELLO_STYLE_ANY);
            CHECK(r >= MELLO_STYLE_AMBIENT && r != last, "ANY rolled %u after %u", r, last);
            seen[r % MELLO_STYLE_COUNT] = 1u, last = r;
        }
        for (i = 0u, st = 0u; i < MELLO_STYLE_COUNT; ++i) st += seen[i];
        CHECK(st >= 11u, "ANY went round only %u styles in 40 rolls", st);
    }

    /* UNDO takes a roll back whole */
    {
        static MelloEngine before;
        fresh(8u);
        before = e;
        (void)mello_engine_style_roll(&e, MELLO_STYLE_DUB);
        CHECK(mello_engine_undo_swap(&e), "no UNDO after a STYLE roll");
        CHECK(!memcmp(&e.project.steps, &before.project.steps, sizeof e.project.steps) &&
                  !memcmp(&e.fx, &before.fx, sizeof e.fx) && !memcmp(&e.sub, &before.sub, sizeof e.sub) &&
                  !memcmp(&e.drums, &before.drums, sizeof e.drums) && !memcmp(&e.bass, &before.bass, sizeof e.bass) &&
                  !memcmp(&e.sound, &before.sound, sizeof e.sound),
              "UNDO did not take the whole roll back");
    }

    /* the mask empty: the lead's steps stay */
    {
        MelloStep steps[MELLO_MAX_STEPS];
        fresh(9u);
        mello_engine_set_randomize_mask(&e, 0u);
        memcpy(steps, e.project.steps, sizeof steps);
        (void)mello_engine_style_roll(&e, MELLO_STYLE_TRIPHOP);
        CHECK(!memcmp(steps, e.project.steps, sizeof steps), "a STYLE roll changed the lead with the mask empty");
    }

    if (fails) return 1;
    puts("PASS: style_roll");
    return 0;
}
