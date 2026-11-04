/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The workshop's wave 3 (DECISIONS "Locks", "Three rolls back", "Kept"). A locked part is never rolled: not by a
 * roll of the band, not alone, and locked chords do not follow a new bass. Three rolls back: the band as it was
 * before each of the last three rolls, the tempo kept, and UNDO's swap left so a press goes back to where the player
 * was. The UI: RNDMZR held and B0-B5 lock (and nothing rolls on letting go); RNDMZR + SAVE keeps the band in the
 * first free slot; UNDO held and white key 2 goes back before the roll before last. */
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

static MelloEngine e;

static void engine_part(void)
{
    static MelloDrumConfig drums;
    static MelloStep lead[MELLO_MAX_STEPS], lead1[MELLO_MAX_STEPS];
    MelloProject p;
    uint8_t k;
    mello_project_defaults(&p);
    p.bpm = 99u;
    mello_engine_init(&e, &p, 11u);
    (void)mello_engine_style_roll(&e, MELLO_STYLE_TECHNO);

    /* locked drums and kit: a roll of the band leaves them */
    e.roll_lock = MELLO_PART_DRUMS | MELLO_PART_KIT;
    drums = e.drums;
    memcpy(lead, e.project.steps, sizeof lead);
    for (k = 0u; k < 3u; ++k) (void)mello_engine_style_roll(&e, MELLO_STYLE_ANY);
    CHECK(!memcmp(&drums, &e.drums, sizeof drums), "locked drums were rolled");
    CHECK(memcmp(lead, e.project.steps, sizeof lead), "the lead, not locked, was not rolled");
    (void)mello_engine_style_roll_parts(&e, MELLO_STYLE_ANY, MELLO_PART_DRUMS);
    CHECK(!memcmp(&drums, &e.drums, sizeof drums), "locked drums rolled alone");

    /* locked lead: not by STYLE, not by INT (on the one) */
    e.roll_lock = MELLO_PART_LEAD;
    memcpy(lead, e.project.steps, sizeof lead);
    (void)mello_engine_style_roll(&e, MELLO_STYLE_ANY);
    mello_engine_randomize_at_bar(&e, MELLO_RANDOMIZE_HIGH);
    (void)mello_engine_style_roll(&e, MELLO_STYLE_OFF);
    CHECK(!memcmp(lead, e.project.steps, sizeof lead), "a locked lead was rolled");
#if MELLO_FEATURE_CHORD_MACHINE
    {
        MelloChordConfig c;
        e.roll_lock = MELLO_PART_CHORDS;
        (void)mello_engine_style_roll(&e, MELLO_STYLE_DUB);
        c = e.chord;
        for (k = 0u; k < 3u; ++k) (void)mello_engine_style_roll_parts(&e, MELLO_STYLE_ANY, MELLO_PART_BASS);
        CHECK(!memcmp(&c, &e.chord, sizeof c), "locked chords followed a new bass");
    }
#endif

    /* three rolls back */
    e.roll_lock = 0u;
    memcpy(lead1, e.project.steps, sizeof lead1);            /* before roll A */
    (void)mello_engine_style_roll(&e, MELLO_STYLE_ACID);      /* A */
    memcpy(lead, e.project.steps, sizeof lead);              /* before roll B */
    (void)mello_engine_style_roll(&e, MELLO_STYLE_HOUSE);     /* B */
    (void)mello_engine_style_roll(&e, MELLO_STYLE_DRONE);     /* C */
    CHECK(e.roll_back_n == MELLO_ROLL_BACK, "%u rolls back, not 3", e.roll_back_n);
    e.project.bpm = 131u;
    CHECK(mello_engine_roll_back(&e, 1u) && !memcmp(lead, e.project.steps, sizeof lead), "back 2 is not before B");
    CHECK(e.project.bpm == 131u, "going back changed the tempo to %u", e.project.bpm);
    CHECK(mello_engine_roll_back(&e, 2u) && !memcmp(lead1, e.project.steps, sizeof lead1), "back 3 is not before A");
    CHECK(!mello_engine_roll_back(&e, 3u), "a fourth roll back");
}

static void ui_part(void)
{
    static MelloProject p;
    static MelloTransport t;
    static MelloStorage st;
    static MelloUi u;
    static MelloStep lead[MELLO_MAX_STEPS];
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    uint32_t rnd = 1u << black_note(10u), undo = 1u << black_note(9u), b1 = 1u << black_note(1u),
             w2 = 1u << white_note(1u);
    uint8_t k;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 5u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_HOME);

    /* RNDMZR held, B1: BASS locked; letting go rolls nothing */
    memcpy(lead, t.engine.project.steps, sizeof lead);
    mello_ui_set_held_notes(&u, rnd);
    mello_ui_input(&u, 1000000u, 1000u, 0u, 0u, 0u, rnd, enc);
    mello_ui_set_held_notes(&u, rnd | b1);
    mello_ui_input(&u, 1100000u, 1100u, 0u, 0u, 0u, b1, enc);
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 1200000u, 1200u, 0u, 0u, 0u, 0u, enc);
    CHECK(t.engine.roll_lock == MELLO_PART_BASS, "RNDMZR + B1 locked %u, not BASS", t.engine.roll_lock);
    CHECK(!memcmp(lead, t.engine.project.steps, sizeof lead), "a lock rolled on letting go");

    /* RNDMZR + SAVE: kept in the first free slot */
    mello_ui_set_held_notes(&u, rnd);
    mello_ui_input(&u, 2000000u, 2000u, 0u, 0u, 0u, rnd, enc); /* RNDMZR down, then SAVE */
    mello_ui_input(&u, 2050000u, 2050u, (1u << MELLO_UI_BTN_SAVE), 0u, (1u << MELLO_UI_BTN_SAVE), 0u, enc);
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 2100000u, 2100u, 0u, 0u, 0u, 0u, enc);
    CHECK(t.engine.slots[0].valid && u.kept_slot == 0 && !memcmp(t.engine.slots[0].name, "KEPT 01", 7u),
          "RNDMZR + SAVE kept nothing in slot 1 (%d, valid %d, \"%s\")", u.kept_slot, (int)t.engine.slots[0].valid,
          t.engine.slots[0].name);

    /* three taps of RNDMZR; UNDO held, white key 2: back before the second last */
    for (k = 0u; k < 3u; ++k) {
        if (k == 1u) memcpy(lead, t.engine.project.steps, sizeof lead);
        mello_ui_set_held_notes(&u, rnd);
        mello_ui_input(&u, 3000000u + k * 200000u, 3000u + k * 200u, 0u, 0u, 0u, rnd, enc);
        mello_ui_set_held_notes(&u, 0u);
        mello_ui_input(&u, 3100000u + k * 200000u, 3100u + k * 200u, 0u, 0u, 0u, 0u, enc);
    }
    mello_ui_set_held_notes(&u, undo);
    mello_ui_input(&u, 4000000u, 4000u, 0u, 0u, 0u, undo, enc);
    mello_ui_input(&u, 4300000u, 4300u, 0u, 0u, 0u, 0u, enc);
    CHECK(u.undo_menu, "UNDO held did not show the rolls back");
    mello_ui_set_held_notes(&u, undo | w2);
    mello_ui_input(&u, 4400000u, 4400u, 0u, 0u, 0u, w2, enc);
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 4500000u, 4500u, 0u, 0u, 0u, 0u, enc);
    CHECK(!memcmp(lead, t.engine.project.steps, sizeof lead), "UNDO + 2 is not the band before the second last roll");
    CHECK(!u.undo_menu, "the rolls back still shown after letting go");
}

int main(void)
{
    engine_part();
    ui_part();
    if (fails) return 1;
    puts("PASS: roll_locks");
    return 0;
}
