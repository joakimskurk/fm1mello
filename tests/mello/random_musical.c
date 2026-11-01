/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* RANDOM: MUSICAL (feedback 2026-10-31: "musical instead of random; I press RANDOM many times and then tweak").
 * Checked over 300 rolls: the bar stays a bar (pulses 1, no skips), pitches stay in the span, strong steps that play
 * are chord tones, the first step always plays, the phrase ends on the root, the A cell comes back (at step 9 in
 * the four-cell forms, most rolls), the loop's second half repeats the first;
 * the melody moves by smaller steps than MED's and plays strong steps more than weak ones; the mask still decides
 * what changes; odd lengths are one cell; UNDO takes a roll back; a CHORD roll starts its progression on the root. */
#include <stdio.h>
#include <stdlib.h>
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

static void roll(uint32_t seed, uint8_t len, uint16_t mask, MelloRandomizeIntensity how)
{
    MelloProject p;
    mello_project_defaults(&p);
    p.seq_len = len;
    mello_engine_init(&e, &p, seed);
    mello_engine_set_randomize_mask(&e, mask);
    mello_engine_randomize_steps_intensity(&e, how);
}

static int played(uint8_t i)
{
    return e.project.steps[i].gate != MELLO_GATE_REST;
}

/* The last step of 0..len-1 that plays. */
static int last_played(uint8_t len)
{
    int i = len;
    while (i-- > 0)
        if (played((uint8_t)i)) return i;
    return -1;
}

/* Mean |pitch step| between notes that play, in hundredths. */
static unsigned motion(uint8_t len)
{
    int prev = -1, i;
    unsigned sum = 0u, k = 0u;
    for (i = 0; i < len; ++i)
        if (played((uint8_t)i)) {
            if (prev >= 0) sum += (unsigned)abs(e.project.steps[i].pitch - prev), ++k;
            prev = e.project.steps[i].pitch;
        }
    return k ? sum * 100u / k : 0u;
}

int main(void)
{
    /* The minor scale: chord tones are degrees 0, 2 and 4; the root's home is the second octave when the span
     * has room. */
    MelloProject d;
    uint8_t pmax, home;
    uint32_t s;
    unsigned strong_on = 0u, strong_n = 0u, weak_on = 0u, weak_n = 0u, mus_motion = 0u, med_motion = 0u,
             back = 0u;
    mello_project_defaults(&d);
    pmax = (uint8_t)(7u * d.pitch_span_oct - 1u);
    home = pmax >= 11u ? 7u : 0u;

    for (s = 1u; s <= 300u; ++s) {
        uint8_t i;
        int last;
        roll(s, 16u, MELLO_RAND_DEFAULT, MELLO_RANDOMIZE_MUSICAL);
        for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
            const MelloStep *st = &e.project.steps[i];
            CHECK(st->pulses == 1u && !st->skip, "seed %u step %u: pulses %u skip %d", (unsigned)s, i,
                  (unsigned)st->pulses, st->skip);
            CHECK(st->pitch >= 0 && st->pitch <= pmax, "seed %u step %u: pitch %d", (unsigned)s, i,
                  st->pitch);
            if (i < 16u && played(i)) {
                uint8_t deg = (uint8_t)(st->pitch % 7);
                if (i % 4u == 0u)
                    CHECK(deg == 0u || deg == 2u || deg == 4u, "seed %u step %u: a strong step on degree %u",
                          (unsigned)s, i, deg);
            }
            if (i < 16u && i % 2u) weak_n++, weak_on += played(i);
            if (i < 16u && i % 4u == 0u) strong_n++, strong_on += played(i);
            if (i >= 16u)
                CHECK(st->pitch == e.project.steps[i - 16u].pitch &&
                          st->gate == e.project.steps[i - 16u].gate,
                      "seed %u: step %u is not step %u again", (unsigned)s, i, i - 16u);
        }
        CHECK(played(0u), "seed %u: the first step rests", (unsigned)s);
        back += !memcmp(&e.project.steps[0], &e.project.steps[8], 4u * sizeof(MelloStep));
        last = last_played(16u);
        CHECK(last >= 0 && e.project.steps[last].pitch == home && e.project.steps[last].octave == 0,
              "seed %u: the phrase ends on %d, not the root %u", (unsigned)s,
              last >= 0 ? e.project.steps[last].pitch : -1, (unsigned)home);
        mus_motion += motion(16u);
        roll(s, 16u, MELLO_RAND_DEFAULT, MELLO_RANDOMIZE_MED);
        med_motion += motion(16u);
    }
    printf("300 rolls: the first beat back at step 9 in %u\n", back);
    /* v2.1: A comes back word for word in two forms of five (feedback: "a looped 8-step"), so sometimes, not mostly;
     * and the second half is never the first half again */
    CHECK(back >= 30u && back <= 150u, "the first beat came back at step 9 in %u rolls of 300", back);
    {
        unsigned same_half = 0u, i;
        for (s = 1u; s <= 300u; ++s) {
            unsigned alike = 1u;
            roll(s, 16u, MELLO_RAND_DEFAULT, MELLO_RANDOMIZE_MUSICAL);
            for (i = 0u; i < 8u && alike; ++i)
                alike = e.project.steps[i].gate == e.project.steps[i + 8u].gate &&
                        (e.project.steps[i].gate == MELLO_GATE_REST ||
                         e.project.steps[i].pitch == e.project.steps[i + 8u].pitch);
            same_half += alike;
        }
        printf("300 rolls: the second half repeats the first in %u\n", same_half);
        CHECK(same_half <= 15u, "the second half repeats the first in %u rolls of 300", same_half);
    }
    { /* MUSICAL v2 (DECISIONS.md "A melody, not a walk"): a phrase uses the range and more than a handful of notes.
       * Over 300 rolls: the mean span (highest to lowest sounding note, octave field counted) and the mean count of
       * distinct notes, and how many rolls reach into the octave above. */
        unsigned span_sum = 0u, distinct_sum = 0u, up = 0u;
        for (s = 1u; s <= 300u; ++s) {
            int lo = 999, hi = -999, i, k, n = 0, seen[64];
            roll(s, 16u, MELLO_RAND_DEFAULT, MELLO_RANDOMIZE_MUSICAL);
            for (i = 0; i < 16; ++i) {
                int v;
                if (!played((uint8_t)i)) continue;
                v = e.project.steps[i].pitch + e.project.steps[i].octave * 7;
                if (v < lo) lo = v;
                if (v > hi) hi = v;
                for (k = 0; k < n && seen[k] != v; ++k) {
                }
                if (k == n) seen[n++] = v;
                if (e.project.steps[i].octave > 0) up = (unsigned)(up | (1u << 31));
            }
            span_sum += (unsigned)(hi - lo);
            distinct_sum += (unsigned)n;
            if (up & (1u << 31)) up = (up & ~(1u << 31)) + 1u;
        }
        printf(
            "300 rolls: a phrase spans %u.%u scale steps and uses %u.%u distinct notes; %u rolls reach the "
            "octave above\n",
            span_sum / 300u, span_sum * 10u / 300u % 10u, distinct_sum / 300u,
            distinct_sum * 10u / 300u % 10u, up);
        CHECK(span_sum >= 300u * 7u,
              "the phrases span only %u.%u scale steps on average: a melody, not a walk", span_sum / 300u,
              span_sum * 10u / 300u % 10u);
        CHECK(distinct_sum * 10u >= 300u * 45u, "the phrases use only %u.%u distinct notes on average",
              distinct_sum / 300u, distinct_sum * 10u / 300u % 10u); /* 4.5: the floor; v2 measures 4.8 */
        CHECK(up >= 60u, "only %u rolls of 300 reach the octave above", up);
    }
    printf("300 rolls: strong steps play %u%%, weak %u%%; the melody moves %u.%02u scale steps a note (MED "
           "%u.%02u)\n",
           strong_on * 100u / strong_n, weak_on * 100u / weak_n, mus_motion / 30000u,
           mus_motion / 300u % 100u, med_motion / 30000u, med_motion / 300u % 100u);
    CHECK(strong_on * 100u / strong_n > 75u && weak_on * 100u / weak_n < 65u, "strong %u/%u, weak %u/%u",
          strong_on, strong_n, weak_on,
          weak_n); /* v2.2's rhythm motifs put more on the weak steps: under 65% */
    CHECK(mus_motion * 10u < med_motion * 6u, "MUSICAL moves %u, MED %u", mus_motion, med_motion);

    { /* the mask: PITCH only leaves the rhythm as it was */
        MelloProject p;
        uint8_t i;
        mello_project_defaults(&p);
        for (i = 0u; i < MELLO_MAX_STEPS; ++i)
            p.steps[i].gate = (MelloGateMode)(i % 3u), p.steps[i].pulses = 2u;
        mello_engine_init(&e, &p, 9u);
        mello_engine_set_randomize_mask(&e, MELLO_RAND_PITCH);
        mello_engine_randomize_steps_intensity(&e, MELLO_RANDOMIZE_MUSICAL);
        for (i = 0u; i < MELLO_MAX_STEPS; ++i)
            CHECK(e.project.steps[i].gate == (MelloGateMode)(i % 3u) && e.project.steps[i].pulses == 2u,
                  "PITCH only changed step %u's rhythm", i);
        CHECK(mello_engine_undo_swap(&e) && !memcmp(&e.project.steps, &p.steps, sizeof p.steps),
              "UNDO did not bring the steps back");
        puts("PITCH only: the rhythm stays; UNDO restores the roll");
    }

    {
        unsigned a_back = 0u;
        for (s = 1u; s <= 50u; ++s) { /* 12 steps: cells of 3; 10: one cell, still home at the end */
            static const uint8_t lens[2] = {12u, 10u};
            uint8_t k;
            for (k = 0u; k < 2u; ++k) {
                int last;
                roll(s, lens[k], MELLO_RAND_DEFAULT, MELLO_RANDOMIZE_MUSICAL);
                last = last_played(lens[k]);
                CHECK(last >= 0 && e.project.steps[last].pitch == home, "%u steps, seed %u: ends on %d",
                      lens[k], (unsigned)s, last >= 0 ? e.project.steps[last].pitch : -1);
                if (k == 0u)
                    a_back += !memcmp(&e.project.steps[0], &e.project.steps[6], 3u * sizeof(MelloStep));
            }
        }
        /* the forms with A back in the third cell are two of five */
        CHECK(a_back >= 5u && a_back <= 30u, "12 steps: the A cell came back in %u rolls of 50", a_back);
        printf("12 steps in cells of 3 (A back in the third cell in %u of 50), 10 steps as one cell: both "
               "end on "
               "the root\n",
               a_back);
    }

#if MELLO_FEATURE_CHORD_MACHINE
    { /* Progressions with a function (DECISIONS.md): 3 to 8 chords on the scale's degrees, no degree twice
       * running, mostly from the tonic and mostly home again, not all triads, not all of length 4, HOLD 8 in
       * place of the chain's default 2 */
        unsigned from_tonic = 0u, home = 0u, not_four = 0u, coloured = 0u;
        for (s = 1u; s <= 100u; ++s) {
            uint8_t k;
            roll(s, 16u, MELLO_RAND_DEFAULT | MELLO_RAND_CHORD, MELLO_RANDOMIZE_MUSICAL);
            CHECK(e.chord.length >= 3u && e.chord.length <= 8u, "seed %u: %u chords", (unsigned)s,
                  e.chord.length);
            for (k = 0u; k < e.chord.length; ++k) {
                CHECK(e.chord.steps[k].degree < 7u, "seed %u: degree %u", (unsigned)s,
                      e.chord.steps[k].degree);
                CHECK(e.chord.steps[k].hold >= 4u, "seed %u: hold %u", (unsigned)s, e.chord.steps[k].hold);
                if (k)
                    CHECK(e.chord.steps[k].degree != e.chord.steps[k - 1u].degree,
                          "seed %u: degree %u twice running", (unsigned)s, e.chord.steps[k].degree);
                if (e.chord.steps[k].shape != MELLO_CHORD_TRIAD) coloured = 1u;
            }
            from_tonic += e.chord.steps[0].degree == 0u;
            home += e.chord.steps[e.chord.length - 1u].degree == 0u;
            not_four += e.chord.length != 4u;
        }
        printf("CHORD: 100 progressions: %u from the tonic, %u home at the end, %u not four chords long\n",
               from_tonic, home, not_four);
        CHECK(from_tonic >= 60u && from_tonic <= 95u, "from the tonic in %u of 100", from_tonic);
        CHECK(home >= 55u && home <= 90u, "home at the end in %u of 100", home);
        CHECK(not_four >= 30u && coloured, "%u not four long, coloured %u", not_four, coloured);
    }
#endif

    if (fails) return 1;
    puts("PASS: MUSICAL rolls a phrase in the bar, the scale and the mask");
    return 0;
}
