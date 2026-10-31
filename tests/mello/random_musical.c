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
    unsigned strong_on = 0u, strong_n = 0u, weak_on = 0u, weak_n = 0u, mus_motion = 0u, med_motion = 0u, back = 0u;
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
            CHECK(st->pitch >= 0 && st->pitch <= pmax, "seed %u step %u: pitch %d", (unsigned)s, i, st->pitch);
            if (i < 16u && played(i)) {
                uint8_t deg = (uint8_t)(st->pitch % 7);
                if (i % 4u == 0u)
                    CHECK(deg == 0u || deg == 2u || deg == 4u, "seed %u step %u: a strong step on degree %u",
                          (unsigned)s, i, deg);
            }
            if (i < 16u && i % 2u) weak_n++, weak_on += played(i);
            if (i < 16u && i % 4u == 0u) strong_n++, strong_on += played(i);
            if (i >= 16u)
                CHECK(st->pitch == e.project.steps[i - 16u].pitch && st->gate == e.project.steps[i - 16u].gate,
                      "seed %u: step %u is not step %u again", (unsigned)s, i, i - 16u);
        }
        CHECK(played(0u), "seed %u: the first step rests", (unsigned)s);
        back += !memcmp(&e.project.steps[0], &e.project.steps[8], 4u * sizeof(MelloStep));
        last = last_played(16u);
        CHECK(last >= 0 && e.project.steps[last].pitch == home && e.project.steps[last].octave == 0,
              "seed %u: the phrase ends on %d, not the root %u", (unsigned)s, last >= 0 ? e.project.steps[last].pitch : -1,
              (unsigned)home);
        mus_motion += motion(16u);
        roll(s, 16u, MELLO_RAND_DEFAULT, MELLO_RANDOMIZE_MED);
        med_motion += motion(16u);
    }
    printf("300 rolls: the first beat back at step 9 in %u\n", back);
    CHECK(back >= 180u, "the first beat came back at step 9 in only %u rolls of 300", back);
    printf("300 rolls: strong steps play %u%%, weak %u%%; the melody moves %u.%02u scale steps a note (MED %u.%02u)\n",
           strong_on * 100u / strong_n, weak_on * 100u / weak_n, mus_motion / 30000u, mus_motion / 300u % 100u,
           med_motion / 30000u, med_motion / 300u % 100u);
    CHECK(strong_on * 100u / strong_n > 75u && weak_on * 100u / weak_n < 50u, "strong %u/%u, weak %u/%u", strong_on,
          strong_n, weak_on, weak_n);
    CHECK(mus_motion * 10u < med_motion * 6u, "MUSICAL moves %u, MED %u", mus_motion, med_motion);

    { /* the mask: PITCH only leaves the rhythm as it was */
        MelloProject p;
        uint8_t i;
        mello_project_defaults(&p);
        for (i = 0u; i < MELLO_MAX_STEPS; ++i) p.steps[i].gate = (MelloGateMode)(i % 3u), p.steps[i].pulses = 2u;
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

    for (s = 1u; s <= 50u; ++s) { /* 12 steps: cells of 3; 10: one cell, still home at the end */
        static const uint8_t lens[2] = {12u, 10u};
        uint8_t k;
        for (k = 0u; k < 2u; ++k) {
            int last;
            roll(s, lens[k], MELLO_RAND_DEFAULT, MELLO_RANDOMIZE_MUSICAL);
            last = last_played(lens[k]);
            CHECK(last >= 0 && e.project.steps[last].pitch == home, "%u steps, seed %u: ends on %d", lens[k],
                  (unsigned)s, last >= 0 ? e.project.steps[last].pitch : -1);
            if (k == 0u)
                CHECK(!memcmp(&e.project.steps[0], &e.project.steps[6], 3u * sizeof(MelloStep)),
                      "12 steps, seed %u: the A cell does not come back", (unsigned)s);
        }
    }
    puts("12 steps in cells of 3, 10 steps as one cell: both end on the root");

#if MELLO_FEATURE_CHORD_MACHINE
    for (s = 1u; s <= 50u; ++s) {
        roll(s, 16u, MELLO_RAND_DEFAULT | MELLO_RAND_CHORD, MELLO_RANDOMIZE_MUSICAL);
        CHECK(e.chord.length == 4u && e.chord.steps[0].degree == 0u &&
                  e.chord.steps[0].shape == e.chord.steps[3].shape,
              "seed %u: the progression starts on degree %u", (unsigned)s, e.chord.steps[0].degree);
    }
    puts("CHORD: four chords from the root, one shape");
#endif

    if (fails) return 1;
    puts("PASS: MUSICAL rolls a phrase in the bar, the scale and the mask");
    return 0;
}
