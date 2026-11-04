/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* One part at a time (DECISIONS "One part at a time"). The engine: after a TECHNO roll, each part rolled alone in the
 * style that plays changes that part and nothing else (BASS: the line and its sound, the chords' degrees following
 * it so the bass is still in the chord; DRUMS: the beat, not the kit's sound; KIT: the sound, not the beat; LEAD,
 * CHORDS, SOUND, TEMPO). The UI: RNDMZR held and a white key rolls that part alone, letting go then does nothing
 * more; held half a second with no key, the RANDOM page; a tap, the whole band. */
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

static MelloEngine e, was;
#define SAME(f) (!memcmp(&e.f, &was.f, sizeof e.f))

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
static uint8_t white_note(uint8_t pos)
{
    uint8_t i;
    for (i = 0u; i < 27u; ++i)
        if (mello_ui_white_key_position(i) == (int8_t)pos) return i;
    return 0xFFu;
}

#if MELLO_FEATURE_CHORD_MACHINE
static int clashes(MelloEngine *x)
{
    unsigned p, bad = 0u;
    uint16_t pcs = 0u;
    int bass = -1, notes = 0;
    mello_engine_stop(x);
    mello_engine_start(x);
    for (p = 0u; p < 32u; ++p) {
        MelloEvent ev;
        if (p % 16u == 0u) {
            if (p && bass >= 0 && notes && !(pcs & (1u << bass))) ++bad;
            pcs = 0u, bass = -1, notes = 0;
        }
        (void)mello_engine_advance_pulse(x);
        while (mello_event_pop(x, &ev)) {
            if (ev.type == MELLO_EVENT_CHORD_NOTE_ON) pcs |= (uint16_t)(1u << (ev.note % 12u)), ++notes;
            if (ev.type == MELLO_EVENT_SUB_NOTE_ON && bass < 0) bass = ev.note % 12;
        }
    }
    return (int)bad;
}
#endif

static void engine_part(void)
{
    MelloProject p;
    uint32_t seed;
    for (seed = 1u; seed <= 4u; ++seed) {
        mello_project_defaults(&p);
        mello_engine_init(&e, &p, seed);
        (void)mello_engine_style_roll(&e, MELLO_STYLE_TECHNO);

        was = e;
        CHECK(mello_engine_style_roll_parts(&e, MELLO_STYLE_ANY, MELLO_PART_BASS) == MELLO_STYLE_TECHNO,
              "a part rolled with ANY left the style that plays");
        CHECK(!SAME(bass) && SAME(project.steps) && SAME(drums) && SAME(fx) && SAME(sound) &&
                  e.project.bpm == was.project.bpm,
              "BASS changed more than the bass");
#if MELLO_FEATURE_CHORD_MACHINE
        CHECK(clashes(&e) == 0, "after BASS alone, the bass outside its chords (seed %u)", seed);
#endif
        was = e;
        (void)mello_engine_style_roll_parts(&e, MELLO_STYLE_ANY, MELLO_PART_DRUMS);
        {
            uint8_t v, kit_same = 1u;
            for (v = 0u; v < MELLO_DRUM_VOICES; ++v) kit_same &= !memcmp(&e.drums.voice[v], &was.drums.voice[v], sizeof e.drums.voice[v]);
            CHECK(kit_same && e.drums.drive == was.drums.drive && SAME(bass) && SAME(project.steps),
                  "DRUMS changed the kit's sound or another part");
        }
        was = e;
        (void)mello_engine_style_roll_parts(&e, MELLO_STYLE_ANY, MELLO_PART_KIT);
        CHECK(!memcmp(e.drums.hits, was.drums.hits, sizeof e.drums.hits) &&
                  memcmp(e.drums.voice, was.drums.voice, sizeof e.drums.voice),
              "KIT changed the beat, or not the sound");
        was = e;
        (void)mello_engine_style_roll_parts(&e, MELLO_STYLE_ANY, MELLO_PART_LEAD);
        CHECK(SAME(bass) && SAME(drums) && SAME(fx), "LEAD changed another part");
        was = e;
        (void)mello_engine_style_roll_parts(&e, MELLO_STYLE_ANY, MELLO_PART_SOUND);
        CHECK(SAME(project.steps) && SAME(bass) && SAME(drums) && !(SAME(sound) && SAME(fx)), "SOUND not the sound alone");
        was = e;
        (void)mello_engine_style_roll_parts(&e, MELLO_STYLE_ANY, MELLO_PART_TEMPO);
        CHECK(SAME(project.steps) && SAME(bass) && e.project.bpm >= 124u && e.project.bpm <= 132u,
              "TEMPO: %u BPM, or more than the tempo", e.project.bpm);
#if MELLO_FEATURE_CHORD_MACHINE
        was = e;
        (void)mello_engine_style_roll_parts(&e, MELLO_STYLE_ANY, MELLO_PART_CHORDS);
        CHECK(SAME(bass) && SAME(project.steps) && SAME(drums), "CHORDS changed another part");
#endif
    }
}

static void ui_part(void)
{
    static MelloProject p;
    static MelloTransport t;
    static MelloStorage st;
    static MelloUi u;
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    uint32_t rnd = 1u << black_note(10u), bass_key = 1u << white_note(1u);
    MelloStep lead[MELLO_MAX_STEPS];
    MelloBassConfig b;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 5u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_HOME);

    /* a tap: the whole band */
    mello_ui_set_held_notes(&u, rnd);
    mello_ui_input(&u, 1000000u, 1000u, 0u, 0u, 0u, rnd, enc);
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 1100000u, 1100u, 0u, 0u, 0u, 0u, enc);
    CHECK(t.engine.style_now >= MELLO_STYLE_AMBIENT, "a tap of RNDMZR rolled no style");

    /* held, and BASS: the bass alone; letting go adds nothing */
    memcpy(lead, t.engine.project.steps, sizeof lead);
    b = t.engine.bass;
    mello_ui_set_held_notes(&u, rnd);
    mello_ui_input(&u, 2000000u, 2000u, 0u, 0u, 0u, rnd, enc);
    mello_ui_input(&u, 2300000u, 2300u, 0u, 0u, 0u, 0u, enc);
    CHECK(u.rndmzr_menu, "RNDMZR held did not show the parts");
    mello_ui_set_held_notes(&u, rnd | bass_key);
    mello_ui_input(&u, 2400000u, 2400u, 0u, 0u, 0u, bass_key, enc);
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 2900000u, 2900u, 0u, 0u, 0u, 0u, enc);
    CHECK(memcmp(&b, &t.engine.bass, sizeof b) && !memcmp(lead, t.engine.project.steps, sizeof lead),
          "RNDMZR + BASS did not roll the bass alone");
    CHECK(u.page == MELLO_UI_HOME && !u.rndmzr_menu, "letting go after a part opened a page or kept the menu");

    /* held half a second, no key: the RANDOM page */
    memcpy(lead, t.engine.project.steps, sizeof lead);
    mello_ui_set_held_notes(&u, rnd);
    mello_ui_input(&u, 3000000u, 3000u, 0u, 0u, 0u, rnd, enc);
    mello_ui_input(&u, 3700000u, 3700u, 0u, 0u, 0u, 0u, enc);
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 3800000u, 3800u, 0u, 0u, 0u, 0u, enc);
    CHECK(u.page == MELLO_UI_RANDOM && !memcmp(lead, t.engine.project.steps, sizeof lead),
          "a long hold did not open RANDOM, or rolled");
}

int main(void)
{
    engine_part();
    ui_part();
    if (fails) return 1;
    puts("PASS: roll_parts");
    return 0;
}
