/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* B5 ROLL (DECISIONS "ROLL on B5"): the product owner (2026-11-12), a roll of the melody alone on a black key, and
 * "roll ... så globalt som mulig der den er tilstede". B5 is ROLL on every page with a part to roll, and rolls that
 * part alone: HOME and the SEQ pages the lead's melody (as RNDMZR held and white key 1), BASS the bass line, CHORD the
 * chords, DRUMS the beat, SOUND the lead's sound; DOT and EVOLVE their own (they were on B1, the bass on B2, the drums
 * on B3: those keys are free now). Here, stopped (a roll lands at once): each page's B5 changes its part and leaves the
 * others as they were; on a page with nothing to roll (GLOBAL, FX, MIX) B5 does nothing. */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "mello_ui.h"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

static MelloProject p;
static MelloTransport t;
static MelloStorage st;
static MelloUi u;
static MelloEngine was;
static uint32_t now_ms = 1000u;

static uint8_t black_note(uint8_t pos)
{
    uint8_t i, n = 0u;
    for (i = 0u; i < 27u; ++i) {
        if (mello_ui_white_key_position(i) >= 0) continue;
        if (n == pos) return i;
        ++n;
    }
    return 0xFFu;
}
static void tap(uint8_t pos)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    uint32_t k = 1u << black_note(pos);
    mello_ui_set_held_notes(&u, k);
    mello_ui_input(&u, now_ms * 1000u, now_ms, 0u, 0u, 0u, k, enc);
    now_ms += 100u;
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, now_ms * 1000u, now_ms, 0u, 0u, 0u, 0u, enc);
    now_ms += 600u;
}
#define SAME(f) (!memcmp(&t.engine.f, &was.f, sizeof was.f))

int main(void)
{
    uint8_t k;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 11u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    u.random_intensity = MELLO_RANDOMIZE_GROOVE;
    mello_ui_set_page(&u, MELLO_UI_HOME);
    tap(9u); /* RNDMZR: a band to roll parts of */
    CHECK(t.engine.style_now >= MELLO_STYLE_AMBIENT, "RNDMZR rolled no style");

    /* HOME: the melody alone */
    was = t.engine;
    tap(5u);
    CHECK(!SAME(project.steps), "HOME B5: the melody did not change");
    CHECK(SAME(bass) && SAME(sub) && SAME(drums) && SAME(sound) && SAME(fx),
          "HOME B5: more than the melody rolled");
#if MELLO_FEATURE_CHORD_MACHINE
    CHECK(SAME(chord), "HOME B5: the chords rolled too");
#endif
    /* the SEQ pages: the same */
    {
        static const MelloUiPage seq[4] = {MELLO_UI_SEQ_STEP, MELLO_UI_SEQ_SEGMENT, MELLO_UI_SEQ_LOOP,
                                           MELLO_UI_SEQ_SLOTSEQ};
        for (k = 0u; k < 4u; ++k) {
            mello_ui_set_page(&u, seq[k]);
            was = t.engine;
            tap(5u);
            CHECK(!SAME(project.steps) && SAME(drums) && SAME(sound), "SEQ page %u B5: not the melody alone",
                  k);
        }
    }
#if MELLO_FEATURE_CHORD_MACHINE
    /* CHORD: the chords alone (a style with chords first) */
    for (k = 0u; k < 12u && !t.engine.chord.on; ++k) {
        mello_ui_set_page(&u, MELLO_UI_HOME);
        tap(9u);
    }
    mello_ui_set_page(&u, MELLO_UI_CHORD);
    { /* (the chords fit what the bass plays, so in some styles a roll of them alone comes out as they were, as
       * RNDMZR held and key 5's does: B5 is that roll, the engine's own, to the byte) */
        static MelloEngine direct;
        direct = t.engine;
        (void)mello_engine_style_roll_parts(&direct, MELLO_STYLE_ANY, MELLO_PART_CHORDS);
        was = t.engine;
        tap(5u);
        CHECK(!memcmp(&direct.chord, &t.engine.chord, sizeof direct.chord) &&
                  !memcmp(&direct.gen_rng, &t.engine.gen_rng, sizeof direct.gen_rng) &&
                  !memcmp(&direct.roll_back, &t.engine.roll_back, sizeof direct.roll_back) &&
                  memcmp(&was.roll_back, &t.engine.roll_back, sizeof was.roll_back),
              "CHORD B5: not the chords' own roll (or none)");
        CHECK(SAME(project.steps) && SAME(drums) && SAME(sound) && SAME(bass),
              "CHORD B5: more than the chords rolled");
    }
#endif
    /* SOUND: the lead's sound alone */
    mello_ui_set_page(&u, MELLO_UI_SOUND_RESERVED);
    was = t.engine;
    tap(5u);
    CHECK(!SAME(sound), "SOUND B5: the sound did not change");
    CHECK(SAME(project.steps) && SAME(drums), "SOUND B5: more than the sound rolled");
    /* DRUMS: the beat */
    mello_ui_set_page(&u, MELLO_UI_DRUMS);
    was = t.engine;
    tap(5u);
    CHECK(!SAME(drums), "DRUMS B5: the drums did not change");
    CHECK(SAME(project.steps) && SAME(sound), "DRUMS B5: more than the drums rolled");
    /* BASS: the line */
    mello_ui_set_page(&u, MELLO_UI_SUB);
    was = t.engine;
    tap(5u);
    CHECK(!SAME(bass), "BASS B5: the bass did not change");
    CHECK(SAME(project.steps) && SAME(drums), "BASS B5: more than the bass rolled");
    /* DOT and EVOLVE: their own */
    mello_ui_set_page(&u, MELLO_UI_DOT);
    was = t.engine;
    tap(5u);
    CHECK(!SAME(dot) && SAME(project.steps), "DOT B5: not the matrix alone");
    mello_ui_set_page(&u, MELLO_UI_TURING);
    was = t.engine;
    tap(5u);
    CHECK(!SAME(tm_float_reg) && SAME(project.steps), "EVOLVE B5: not its figure alone");
    /* nothing to roll: nothing */
    {
        static const MelloUiPage none[3] = {MELLO_UI_GLOBAL, MELLO_UI_AUDIO_FX, MELLO_UI_MIX};
        for (k = 0u; k < 3u; ++k) {
            mello_ui_set_page(&u, none[k]);
            was = t.engine;
            tap(5u);
            CHECK(SAME(project.steps) && SAME(drums) && SAME(sound) && SAME(bass) && SAME(fx),
                  "page %u: B5 changed something", k);
        }
    }
    if (fails) return 1;
    puts("PASS: roll_key");
    return 0;
}
