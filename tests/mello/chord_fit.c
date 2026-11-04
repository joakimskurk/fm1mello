/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* FIT and the one-chord start (DECISIONS "FIT"). A new project's chords are one chord, the tonic. FIT over a bass
 * line (root on the one, the fifth on the third beat, the fourth after) makes a chord every half bar on the bass's
 * note: played, every chord holds the pitch class the bass plays under it, in every one of the four variants
 * (triads, sevenths, colour, the chord whose third the note is), which FIT goes through when pressed again over the
 * same bass; another bass starts over at the triads. With the bass off it reads the lead. SUB ROOT goes off; UNDO
 * takes it back. */
#include <stdio.h>
#include <string.h>

#include "mello_engine.h"

#if MELLO_FEATURE_CHORD_MACHINE
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

/* 32 pulses (two rounds of a 16-step bass); per half bar, the chord notes' pitch classes and the bass's first
 * note: every bass pitch class must be among the chord's. Returns the number of half bars where it is not. */
static int clashes(void)
{
    unsigned p, bad = 0u;
    uint16_t chord_pcs = 0u;
    int bass_pc = -1;
    mello_engine_stop(&e);
    mello_engine_start(&e);
    for (p = 0u; p < 32u; ++p) {
        MelloEvent ev;
        if (p % 8u == 0u) {
            if (p && bass_pc >= 0 && !(chord_pcs & (1u << bass_pc))) ++bad;
            chord_pcs = 0u, bass_pc = -1;
        }
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev)) {
            if (ev.type == MELLO_EVENT_CHORD_NOTE_ON) chord_pcs |= (uint16_t)(1u << (ev.note % 12u));
            if (ev.type == MELLO_EVENT_SUB_NOTE_ON && bass_pc < 0) bass_pc = ev.note % 12;
        }
    }
    if (bass_pc >= 0 && !(chord_pcs & (1u << bass_pc))) ++bad;
    return (int)bad;
}

int main(void)
{
    MelloProject p;
    MelloBassConfig b;
    MelloSubConfig s;
    MelloChordConfig c;
    uint8_t v, first_degree;

    mello_project_defaults(&p); /* C minor */
    mello_engine_init(&e, &p, 9u);
    CHECK(e.chord.length == 1u && e.chord.steps[0].degree == 0u, "a new project's chords are not one tonic chord");

    s = e.sub;
    s.on = true;
    mello_engine_sub_set_config(&e, &s);
    b = e.bass;
    b.on = true;
    b.length = 16u;
    memset(b.steps, 0, sizeof b.steps);
    b.steps[0].on = true, b.steps[0].tone = MELLO_SUB_TONE_ROOT;
    b.steps[4].on = true, b.steps[4].tone = MELLO_SUB_TONE_ROOT;
    b.steps[8].on = true, b.steps[8].tone = MELLO_SUB_TONE_FIFTH;
    b.steps[12].on = true, b.steps[12].tone = MELLO_SUB_TONE_FOURTH;
    mello_engine_bass_set_config(&e, &b);
    c = e.chord;
    c.sub_root = true;
    mello_engine_chord_set_config(&e, &c);

    mello_engine_chord_fit(&e);
    CHECK(e.chord.on && !e.chord.sub_root && e.chord.length == 2u, "FIT: on %d, SUB ROOT %d, %u chords",
          (int)e.chord.on, (int)e.chord.sub_root, e.chord.length);
    CHECK(e.chord.steps[0].degree == 0u && e.chord.steps[1].degree == 4u, "FIT read degrees %u, %u, not 0, 4",
          e.chord.steps[0].degree, e.chord.steps[1].degree);
    first_degree = e.chord.steps[1].degree;
    for (v = 0u; v < 4u; ++v) {
        int bad = clashes();
        CHECK(bad == 0, "variant %u: the bass outside the chord in %d half bars", v, bad);
        if (v == 1u) CHECK(e.chord.steps[0].shape == MELLO_CHORD_SEVENTH, "the second press is not the sevenths");
        if (v == 3u)
            CHECK(e.chord.steps[1].degree == (uint8_t)(first_degree - 2u) && e.chord.steps[1].inversion == 1u,
                  "the fourth press is not the chord whose third the note is");
        mello_engine_chord_fit(&e);
    }
    CHECK(e.chord.steps[0].shape == MELLO_CHORD_TRIAD && e.chord.steps[1].degree == first_degree,
          "the fifth press did not come round to the triads");

    /* another bass: the triads again */
    mello_engine_chord_fit(&e); /* (now on the sevenths) */
    b = e.bass;
    b.steps[8].tone = MELLO_SUB_TONE_SIXTH;
    mello_engine_bass_set_config(&e, &b);
    mello_engine_chord_fit(&e);
    CHECK(e.chord.steps[1].degree == 5u && e.chord.steps[1].shape == MELLO_CHORD_TRIAD,
          "another bass did not start at the triads (degree %u)", e.chord.steps[1].degree);
    CHECK(clashes() == 0, "the new bass is outside its chords");
    CHECK(mello_engine_undo_swap(&e) && e.chord.steps[1].degree == 4u && e.chord.steps[1].shape == MELLO_CHORD_SEVENTH,
          "UNDO did not go back to the chords before the last FIT");
    (void)mello_engine_undo_swap(&e); /* (and forward again) */

    /* the bass off: the lead */
    b = e.bass;
    b.on = false;
    mello_engine_bass_set_config(&e, &b);
    {
        uint8_t i;
        for (i = 0u; i < 16u; ++i) e.project.steps[i].gate = MELLO_GATE_REST;
        e.project.steps[0].gate = MELLO_GATE_SINGLE, e.project.steps[0].pitch = 2;
        e.project.steps[8].gate = MELLO_GATE_SINGLE, e.project.steps[8].pitch = 10; /* degree 3, an octave up */
        e.project.seq_len = 16u;
    }
    mello_engine_chord_fit(&e);
    CHECK(e.chord.steps[0].degree == 2u && e.chord.steps[1].degree == 3u, "with the bass off FIT read %u, %u",
          e.chord.steps[0].degree, e.chord.steps[1].degree);

    if (fails) return 1;
    puts("PASS: chord_fit");
    return 0;
}
#else
int main(void)
{
    puts("PASS: chord_fit (no Chord Machine)");
    return 0;
}
#endif
