/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Record it yourself, easily (DECISIONS "Record it yourself, easily"). On HOME a tap of REC goes straight into KEYS
 * recording the sequence. REC held half a second and let go starts the take from empty. REC held and a white key
 * picks the target (2 BASS, 4 DRUMS: the keys then play the kit), B0 the click. A take that wrote something ends
 * locked against the rolls, and a bass take fits the chords to it, inside the take's one UNDO. The engine: a bar's
 * count-in of clicks before the sequence moves; a take's click on every beat over silent drums. */
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
static uint32_t ms = 1000u;
static int16_t enc[MELLO_UI_ENCODER_COUNT];

static void in(uint32_t pressed, uint32_t released, uint32_t held, uint32_t notes)
{
    ms += 20u;
    mello_ui_input(&u, ms * 1000u, ms, pressed, released, held, notes, enc);
}
static void rec_tap(void)
{
    in(1u << MELLO_UI_BTN_REC, 0u, 1u << MELLO_UI_BTN_REC, 0u);
    in(0u, 1u << MELLO_UI_BTN_REC, 0u, 0u);
}
static void rec_hold(uint32_t hold_ms, uint32_t key)
{
    uint32_t start = ms;
    in(1u << MELLO_UI_BTN_REC, 0u, 1u << MELLO_UI_BTN_REC, 0u);
    while (ms - start < hold_ms) in(0u, 0u, 1u << MELLO_UI_BTN_REC, 0u);
    if (key) {
        mello_ui_set_held_notes(&u, key);
        in(0u, 0u, 1u << MELLO_UI_BTN_REC, key);
        mello_ui_set_held_notes(&u, 0u);
    }
    in(0u, 1u << MELLO_UI_BTN_REC, 0u, 0u);
}
static uint32_t white(uint8_t pos)
{
    uint8_t i;
    for (i = 0u; i < 27u; ++i)
        if (mello_ui_white_key_position(i) == (int8_t)pos) return 1u << i;
    return 0u;
}
static uint32_t black(uint8_t pos)
{
    uint8_t i, n = 0u;
    for (i = 0u; i < 27u; ++i) {
        if (mello_ui_white_key_position(i) >= 0) continue;
        if (n++ == pos) return 1u << i;
    }
    return 0u;
}
static void key(uint8_t note_index)
{
    mello_ui_set_held_notes(&u, 1u << note_index);
    in(0u, 0u, 0u, 1u << note_index);
    mello_ui_set_held_notes(&u, 0u);
    in(0u, 0u, 0u, 0u);
}

static void ui_part(void)
{
    MelloEngine *e;
    uint8_t i, plays = 0u;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 3u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_HOME);
    e = &t.engine;

    /* a tap: KEYS recording the sequence, no EDIT first */
    rec_tap();
    CHECK(u.keys_mode && u.keys_rec && u.keys_rec_target == MELLO_UI_REC_SEQ,
          "a tap of REC on HOME did not record");
    key(7u); /* C4 to step 1 */
    CHECK(e->project.steps[0].gate != MELLO_GATE_REST && u.keys_rec_pos == 1u,
          "the key did not write step 1");

    /* held and let go: from empty; a key writes step 1, the rest stay rests */
    rec_hold(700u, 0u);
    CHECK(u.keys_rec && u.keys_rec_target == MELLO_UI_REC_SEQ, "a hold ended the take");
    key(10u);
    for (i = 0u; i < 16u; ++i) plays += e->project.steps[i].gate != MELLO_GATE_REST;
    CHECK(plays == 1u && e->project.steps[0].gate != MELLO_GATE_REST, "from empty: %u steps play, not 1",
          plays);

    /* held, white key 2: BASS, the same take; a bass line of root and fifth, then REC round to off */
    rec_hold(300u, white(1u));
    CHECK(u.keys_rec_target == MELLO_UI_REC_BASS, "REC + white key 2 did not pick BASS");
    key(7u), key(14u), key(7u), key(14u);
    e->roll_lock = 0u;
#if MELLO_FEATURE_CHORD_MACHINE
    {
        MelloChordConfig c = e->chord;
        c.on = true;
        mello_engine_chord_set_config(e, &c);
    }
#endif
    rec_tap(); /* (CHORD) */
    rec_tap(); /* (DRUMS) */
    rec_tap(); /* off: the take ends */
    CHECK(!u.keys_rec && (e->roll_lock & MELLO_PART_BASS), "the take ended without locking the bass (%u)",
          e->roll_lock);
#if MELLO_FEATURE_CHORD_MACHINE
    CHECK(u.take_fit && e->chord.length == 2u, "the chords were not fitted to the bass (length %u)",
          e->chord.length);
#endif
    CHECK(u.readout == MELLO_UI_READOUT_TAKE, "no TAKE readout");
    CHECK(mello_engine_undo_swap(e) && e->project.steps[0].gate != MELLO_GATE_REST && !e->bass.steps[1].on,
          "one UNDO did not take the take (and the fit) back");

    /* held, B0: the click off; white key 4: DRUMS, the keys play the kit */
    rec_hold(300u, black(0u));
    CHECK(u.rec_click_off, "REC + B0 did not turn the click off");
    rec_hold(300u, white(3u));
    CHECK(u.keys_rec && u.keys_rec_target == MELLO_UI_REC_DRUMS, "REC + white key 4 did not pick DRUMS");
    {
        uint32_t was = e->drums.hits[MELLO_DRUM_SNARE];
        mello_ui_set_held_notes(&u, white(1u));
        in(0u, 0u, 0u, white(1u)); /* white key 2: the snare, at the cursor */
        mello_ui_set_held_notes(&u, 0u);
        CHECK(e->drums.hits[MELLO_DRUM_SNARE] != was, "REC > DRUMS on HOME: white key 2 wrote no snare");
    }
}

static void engine_part(void)
{
    static MelloEngine e;
    MelloProject q;
    uint32_t k;
    unsigned clicks = 0u, notes = 0u, first_note = 0u;
    mello_project_defaults(&q);
    mello_engine_init(&e, &q, 2u);
    e.drums.on = false;
    mello_engine_start(&e);
    e.countin = 16u;
    for (k = 0u; k < 24u; ++k) {
        MelloEvent ev;
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev)) {
            if (ev.type == MELLO_EVENT_DRUM_HIT && k < 16u) clicks++;
            if (ev.type == MELLO_EVENT_NOTE_ON && !notes++) first_note = k;
        }
    }
    CHECK(clicks == 4u, "the count-in clicked %u times, not 4", clicks);
    CHECK(notes && first_note == 16u, "the sequence started at pulse %u, not after the bar's count-in",
          first_note);
    e.click = true, clicks = 0u;
    for (k = 0u; k < 16u; ++k) {
        MelloEvent ev;
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev))
            if (ev.type == MELLO_EVENT_DRUM_HIT) clicks++;
    }
    CHECK(clicks == 4u, "a take's click over silent drums: %u clicks in a bar, not 4", clicks);
}

int main(void)
{
    ui_part();
    engine_part();
    if (fails) return 1;
    puts("PASS: rec_easy");
    return 0;
}
