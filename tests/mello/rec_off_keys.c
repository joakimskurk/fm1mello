/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* REC round to off on HOME leaves KEYS again when REC brought it there (DECISIONS "Record it yourself, easily").
 * The product owner, 2026-11-10, after a take on HOME: the drums locked (fine), "but the bass does not seem to change,
 * nor the chords". The take had ended with REC round to off, but KEYS stayed on: every key a note, so RNDMZR (B9)
 * played one (or, playing, moved the root) and rolled nothing, and B7 UNLOCK could not be reached. Now REC round to
 * off goes back where REC found it: out of KEYS when REC's tap brought it in, still in KEYS when EDIT did. */
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
static MelloUi u;
static uint32_t now_ms = 1000u;
static int16_t enc[MELLO_UI_ENCODER_COUNT];

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
static void idle(uint32_t ms)
{
    uint32_t end = now_ms + ms;
    for (; now_ms < end; now_ms += 10u) mello_ui_input(&u, now_ms * 1000u, now_ms, 0u, 0u, 0u, 0u, enc);
}
static void tap(uint8_t note)
{
    mello_ui_set_held_notes(&u, 1u << note);
    mello_ui_input(&u, now_ms * 1000u, now_ms, 0u, 0u, 0u, 1u << note, enc);
    now_ms += 60u;
    mello_ui_set_held_notes(&u, 0u);
    idle(200u);
}
static void button(uint8_t b)
{
    uint32_t m = 1u << b;
    mello_ui_input(&u, now_ms * 1000u, now_ms, m, 0u, m, 0u, enc);
    now_ms += 60u;
    mello_ui_input(&u, now_ms * 1000u, now_ms, 0u, m, 0u, 0u, enc);
    idle(200u);
}
static void fresh(void)
{
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 7u, 0, 0);
    mello_ui_init(&u, &t, 0);
    u.random_intensity = MELLO_RANDOMIZE_GROOVE;
}
/* REC round from SEQ to off, a key on SEQ (the take writes the lead) */
static void take_round(void)
{
    uint8_t n;
    button(MELLO_UI_BTN_REC);
    CHECK(u.keys_mode && u.keys_rec && u.keys_rec_target == MELLO_UI_REC_SEQ, "REC on HOME did not record");
    tap(white_note(3u));
    for (n = 0u; n < 6u && u.keys_rec; ++n) button(MELLO_UI_BTN_REC);
    CHECK(!u.keys_rec, "REC round did not end the take");
    CHECK(t.engine.roll_lock & MELLO_PART_LEAD, "the take did not lock the lead (%02x)", t.engine.roll_lock);
}

int main(void)
{
    MelloEngine before;

    /* REC from outside KEYS: off is out of KEYS, the black keys their own; RNDMZR rolls, UNLOCK unlocks */
    fresh();
    take_round();
    CHECK(!u.keys_mode, "REC round to off left KEYS on");
    before = t.engine;
    tap(black_note(9u)); /* RNDMZR */
    CHECK(memcmp(&before.bass, &t.engine.bass, sizeof before.bass) ||
              memcmp(before.drums.hits, t.engine.drums.hits, sizeof before.drums.hits),
          "after the take RNDMZR rolled nothing");
    CHECK(t.engine.roll_lock & MELLO_PART_LEAD, "RNDMZR undid the take's lock");
    tap(black_note(7u)); /* UNLOCK */
    CHECK(t.engine.roll_lock == 0u && u.readout == MELLO_UI_READOUT_UNLOCK, "B7 did not unlock (%02x)",
          t.engine.roll_lock);

    /* in KEYS first (EDIT): REC round to off stays in KEYS, where it was */
    fresh();
    button(MELLO_UI_BTN_EDIT);
    CHECK(u.keys_mode, "EDIT on HOME did not enter KEYS");
    take_round();
    CHECK(u.keys_mode, "REC round to off left KEYS that EDIT had turned on");

    /* REC from outside KEYS, then EDIT out and in again in the middle: the next take was started in KEYS */
    fresh();
    button(MELLO_UI_BTN_REC);
    button(MELLO_UI_BTN_EDIT); /* KEYS off, the take ends */
    button(MELLO_UI_BTN_EDIT); /* KEYS on */
    take_round();
    CHECK(u.keys_mode, "a take started in KEYS (EDIT) left KEYS at off");

    if (fails) return 1;
    puts("PASS: rec_off_keys");
    return 0;
}
