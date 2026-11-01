/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* ROLL on BASS (DECISIONS.md "A bass line that fits"): a rolled BASS SEQ line is never sporadic (five to eleven of
 * sixteen steps), starts on the root on the one, sits an octave down except for octave jumps, puts roots, fifths or
 * the lead's chord tones on the beats, keeps to tones in the scale, glides only into a step that plays, turns BASS
 * SEQ on, differs from roll to roll, and is undone by UNDO. The lead is MUSICAL's roll, so the beats have chord
 * tones to follow. */
#include "mello_engine.h"
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

static MelloEngine e;

static unsigned hits(void)
{
    unsigned k, n = 0u;
    for (k = 0u; k < 16u; ++k) n += e.bass.steps[k].on;
    return n;
}

int main(void)
{
    MelloProject p;
    MelloBassConfig first;
    uint32_t s;
    unsigned sum = 0u, lead_follow = 0u, beats = 0u, different = 0u;
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 7u);
    mello_engine_randomize_steps_intensity(&e, MELLO_RANDOMIZE_MUSICAL); /* a lead to fit */
    CHECK(!e.bass.on, "BASS SEQ is off to begin with");

    for (s = 0u; s < 200u; ++s) {
        unsigned k, h;
        mello_engine_undo_snapshot(&e);
        mello_engine_bass_roll(&e);
        h = hits();
        sum += h;
        CHECK(e.bass.on, "roll %u: BASS SEQ should be on after a roll", (unsigned)s);
        CHECK(h >= 5u && h <= 11u, "roll %u: %u steps play, wanted 5..11", (unsigned)s, h);
        CHECK(e.bass.steps[0].on && e.bass.steps[0].tone == MELLO_SUB_TONE_ROOT && e.bass.steps[0].accent,
              "roll %u: the one is not an accented root", (unsigned)s);
        for (k = 0u; k < 16u; ++k) {
            const MelloBassStep *b = &e.bass.steps[k];
            CHECK(b->octave == -1 || (b->octave == 0 && b->tone == MELLO_SUB_TONE_ROOT),
                  "roll %u step %u: octave %d tone %u (an octave down, or a root jumped up)", (unsigned)s, k,
                  b->octave, b->tone);
            CHECK(b->tone <= MELLO_SUB_TONE_OCTAVE, "roll %u step %u: tone %u", (unsigned)s, k, b->tone);
            if (b->on && b->glide)
                CHECK(e.bass.steps[(k + 1u) % 16u].on, "roll %u step %u: a glide into a rest", (unsigned)s,
                      k);
            if (b->on && k % 4u == 0u) {
                uint8_t lead = (uint8_t)((e.project.steps[k].pitch < 0 ? 0 : e.project.steps[k].pitch) % 7);
                ++beats;
                if (b->tone == MELLO_SUB_TONE_ROOT || b->tone == MELLO_SUB_TONE_FIFTH)
                    ++lead_follow;
                else {
                    CHECK(b->tone == lead, "roll %u step %u: a beat on tone %u, the lead plays degree %u",
                          (unsigned)s, k, b->tone, lead);
                    ++lead_follow;
                }
            }
        }
        if (s == 0u) first = e.bass;
        if (s && memcmp(e.bass.steps, first.steps, sizeof first.steps)) ++different;
    }
    printf(
        "200 rolls: %u.%u notes a line; %u of %u beats are roots, fifths or the lead's tone; %u lines differ "
        "from the first\n",
        sum / 200u, sum * 10u / 200u % 10u, lead_follow, beats, different);
    CHECK(different >= 190u, "only %u of 199 rolls differ from the first", different);

    { /* UNDO takes a roll back */
        MelloBassConfig before = e.bass;
        mello_engine_undo_snapshot(&e);
        mello_engine_bass_roll(&e);
        CHECK(memcmp(e.bass.steps, before.steps, sizeof before.steps) != 0, "the roll changed nothing");
        CHECK(mello_engine_undo_swap(&e) && !memcmp(e.bass.steps, before.steps, sizeof before.steps),
              "UNDO did not bring the line back");
    }
    printf(
        "PASS: ROLL on BASS gives a line under the sequence: roots and fifths on the beats, an octave down, "
        "never sporadic, undoable\n");
    return 0;
}
