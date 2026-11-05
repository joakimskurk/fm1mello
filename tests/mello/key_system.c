/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* One key that always holds (ROADMAP E1) and the bass an octave down (E9). The key now is the KEY page's root and
 * scale, the loop step's KEY / SCALE where they are not FOLLOW, ROOT+ moving the tonic. TRANS and BASS TR are
 * semitones whose result is rounded into the key in their direction (+1 in C minor: C to D, Eb to F; -1: D to C),
 * so ROOT+ and TRANS no longer sound the same. Over every scale but CHROMATIC, every root and a spread of ROOT+,
 * TRANS, BASS TR, KEY, SCALE and the loop step's SUB tone, no lead, bass or chord note leaves the key now. CHROMATIC
 * transposes by plain semitones. The bass at OCT -1 sounds two octaves under the root. */
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

typedef struct {
    int8_t root_offset, root_note, scale_set, pitch, sub_tr;
    uint8_t sub_tone;
} Loop;

/* A lead of 16 steps all on `degree` (or walking 0..7 when degree < 0), the bass sequence walking every tone, the
 * chords walking the scale's degrees, and loop step 0 (it applies with the loop cycle off) as given. */
static void start(MelloScale scale, uint8_t root, int8_t degree, const Loop *l, bool bass, bool chords)
{
    MelloProject p;
    MelloLoopCycleStep lc;
    MelloBassConfig b;
    unsigned i;
    mello_project_defaults(&p);
    p.scale = scale;
    p.root_note = root;
    p.seq_len = 16u;
    for (i = 0u; i < 16u; ++i) {
        p.steps[i].pitch = (int8_t)(degree >= 0 ? degree : (int8_t)(i % 8u));
        p.steps[i].octave = 0;
        p.steps[i].gate = MELLO_GATE_SINGLE;
        p.steps[i].pulses = 1u;
        p.steps[i].ratchet = 1u;
        p.steps[i].skip = false;
        p.steps[i].probability = 65535u;
    }
    mello_engine_init(&e, &p, 3u);
    lc = e.loop_cycle.steps[0];
    lc.root_offset = l->root_offset;
    lc.root_note = l->root_note;
    lc.scale_set = l->scale_set;
    lc.pitch = l->pitch;
    mello_loop_cycle_set_sub(&lc, l->sub_tone, l->sub_tr);
    mello_engine_loop_cycle_set_step(&e, 0u, &lc);
    if (bass) {
        mello_bass_defaults(&b);
        b.on = true;
        b.length = 8u;
        for (i = 0u; i < 8u; ++i) {
            b.steps[i].on = true;
            b.steps[i].tone = (uint8_t)(i % 7u);
            b.steps[i].octave = 0;
            b.steps[i].glide = false;
        }
        mello_engine_bass_set_config(&e, &b);
    }
#if MELLO_FEATURE_CHORD_MACHINE
    if (chords) {
        MelloChordConfig c;
        mello_chord_defaults(&c);
        c.on = true;
        c.mode = MELLO_CHORD_HOLD;
        c.chain = MELLO_CHORD_CHAIN_STEPS;
        c.length = 7u;
        for (i = 0u; i < 7u; ++i) {
            c.steps[i].degree = (uint8_t)i;
            c.steps[i].shape = i % 2u ? MELLO_CHORD_SEVENTH : MELLO_CHORD_TRIAD;
            c.steps[i].hold = 2u;
        }
        mello_engine_chord_set_config(&e, &c);
    }
#else
    (void)chords;
#endif
    mello_engine_start(&e);
}

static bool in_key(uint8_t note)
{
    uint8_t kr, n, i, rel;
    MelloScale ks;
    const int8_t *t;
    mello_engine_key_now(&e, &kr, &ks);
    n = mello_scale_tones(ks, &t);
    if (n >= 12u) return true;
    rel = (uint8_t)(((int32_t)note - (int32_t)kr + 120) % 12);
    for (i = 0u; i < n; ++i)
        if (t[i] == (int8_t)rel) return true;
    return false;
}

/* The notes of `pulses` pulses: how many, and how many off the key now. */
static void run(unsigned pulses, unsigned *notes, unsigned *off, uint8_t *first_lead, uint8_t *first_bass)
{
    MelloEvent ev;
    unsigned i;
    *notes = *off = 0u;
    if (first_lead) *first_lead = 0u;
    if (first_bass) *first_bass = 0u;
    for (i = 0u; i < pulses; ++i) {
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev)) {
            bool lead = ev.type == MELLO_EVENT_NOTE_ON, bass = ev.type == MELLO_EVENT_SUB_NOTE_ON;
#if MELLO_FEATURE_CHORD_MACHINE
            bool chord = ev.type == MELLO_EVENT_CHORD_NOTE_ON;
#else
            bool chord = false;
#endif
            if (!lead && !bass && !chord) continue;
            ++*notes;
            if (!in_key(ev.note)) ++*off;
            if (lead && first_lead && !*first_lead) *first_lead = ev.note;
            if (bass && first_bass && !*first_bass) *first_bass = ev.note;
        }
    }
}

static uint8_t lead_note(MelloScale scale, uint8_t root, int8_t degree, int8_t root_offset, int8_t pitch)
{
    Loop l = {root_offset, -1, -1, pitch, 0, MELLO_SUB_TONE_ROOT};
    unsigned n, off;
    uint8_t note;
    start(scale, root, degree, &l, false, false);
    run(4u, &n, &off, &note, NULL);
    return note;
}

int main(void)
{
    static const int8_t offsets[] = {-2, 0, 3}, trans[] = {-5, -1, 0, 1, 2, 7}, sub_trs[] = {-3, 0, 1, 4};
    unsigned total = 0u, off_total = 0u, sc, r, a, b, c;
    uint8_t note, bass_note;

    /* TRANS rounds in its direction: C minor, C4 = 60 */
    CHECK(lead_note(MELLO_SCALE_MINOR, 60u, 0, 0, 1) == 62u, "C +1 in C minor is D (%u)",
          lead_note(MELLO_SCALE_MINOR, 60u, 0, 0, 1));
    CHECK(lead_note(MELLO_SCALE_MINOR, 60u, 2, 0, 1) == 65u, "Eb +1 in C minor is F (%u)",
          lead_note(MELLO_SCALE_MINOR, 60u, 2, 0, 1));
    CHECK(lead_note(MELLO_SCALE_MINOR, 60u, 1, 0, -1) == 60u, "D -1 in C minor is C (%u)",
          lead_note(MELLO_SCALE_MINOR, 60u, 1, 0, -1));
    CHECK(lead_note(MELLO_SCALE_MINOR, 60u, 0, 0, 7) == 67u, "C +7 in C minor is G (%u)",
          lead_note(MELLO_SCALE_MINOR, 60u, 0, 0, 7));
    /* ROOT+ moves the key, TRANS the line within it: D +2 is E in D minor (ROOT+ 2: the second degree of D minor)
     * and F in C minor (TRANS 2: E is not in it) */
    CHECK(lead_note(MELLO_SCALE_MINOR, 60u, 1, 2, 0) == 64u, "ROOT+ 2: the 2nd degree of D minor is E (%u)",
          lead_note(MELLO_SCALE_MINOR, 60u, 1, 2, 0));
    CHECK(lead_note(MELLO_SCALE_MINOR, 60u, 1, 0, 2) == 65u, "TRANS 2: D + 2 in C minor is F (%u)",
          lead_note(MELLO_SCALE_MINOR, 60u, 1, 0, 2));
    /* CHROMATIC: plain semitones */
    CHECK(lead_note(MELLO_SCALE_CHROMATIC, 60u, 0, 0, 1) == 61u, "CHROMATIC: C + 1 is C# (%u)",
          lead_note(MELLO_SCALE_CHROMATIC, 60u, 0, 0, 1));

    /* the key now: the loop step's KEY and ROOT+ where it sets them */
    {
        Loop l = {2, -1, -1, 0, 0, MELLO_SUB_TONE_ROOT};
        uint8_t kr;
        MelloScale ks;
        start(MELLO_SCALE_MINOR, 60u, 0, &l, false, false);
        mello_engine_key_now(&e, &kr, &ks);
        CHECK(kr == 62u && ks == MELLO_SCALE_MINOR, "ROOT+ 2 over C minor: the key now is D minor (%u, %d)",
              kr, (int)ks);
        l.root_offset = 0;
        l.root_note = 9;
        l.scale_set = MELLO_SCALE_DORIAN;
        start(MELLO_SCALE_MINOR, 60u, 0, &l, false, false);
        mello_engine_key_now(&e, &kr, &ks);
        CHECK(kr == 69u && ks == MELLO_SCALE_DORIAN, "KEY A, SCALE DORIAN: the key now is A dorian (%u, %d)",
              kr, (int)ks);
    }

    /* the bass an octave down: OCT -1 (the default), the root tone at octave 0 is two octaves under C4 */
    {
        Loop l = {0, -1, -1, 0, 0, MELLO_SUB_TONE_ROOT};
        unsigned n, off;
        start(MELLO_SCALE_MINOR, 60u, 0, &l, true, false);
        CHECK(e.sub.octave == -1, "the bass's default OCT is -1 (%d)", e.sub.octave);
        run(2u, &n, &off, &note, &bass_note);
        CHECK(bass_note == 36u, "the bass's root at OCT -1 is C2 = 36 (%u)", bass_note);
    }

    /* every scale but CHROMATIC, every root, a spread of the loop step's settings: no note off the key now */
    for (sc = 1u; sc < MELLO_SCALE_COUNT; ++sc)
        for (r = 0u; r < 12u; ++r)
            for (a = 0u; a < sizeof offsets; ++a)
                for (b = 0u; b < sizeof trans; ++b)
                    for (c = 0u; c < sizeof sub_trs; ++c) {
                        Loop l = {offsets[a],
                                  (int8_t)((r + b) % 3u == 0u ? -1 : (int8_t)((r * 5u) % 12u)),
                                  (int8_t)(c % 2u ? -1 : (int8_t)((sc + c) % MELLO_SCALE_COUNT)),
                                  trans[b],
                                  sub_trs[c],
                                  (uint8_t)((a + b + c) % 8u)};
                        unsigned n, off;
                        if (l.scale_set == MELLO_SCALE_CHROMATIC) l.scale_set = -1;
                        start((MelloScale)sc, (uint8_t)(48u + r), -1, &l, true, true);
                        run(32u, &n, &off, NULL, NULL);
                        total += n;
                        off_total += off;
                        if (off)
                            CHECK(0,
                                  "scale %u root %u ROOT+ %d TRANS %d BASS TR %d SUB %u: %u of %u notes off "
                                  "the key",
                                  sc, 48u + r, l.root_offset, l.pitch, l.sub_tr, l.sub_tone, off, n);
                    }
    CHECK(total > 100000u, "the sweep played only %u notes", total);
    printf("key_system: %u notes over the sweep, %u off the key\n", total, off_total);

    if (fails) return 1;
    puts("PASS: key_system");
    return 0;
}
