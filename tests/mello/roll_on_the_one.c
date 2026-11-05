/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* RNDMZR's roll on the one, and the tempo the player's (DECISIONS "A roll lands on the one", "The tempo is the
 * player's"). Stopped, a roll is at once. Playing, it waits for the next bar's one (the 16th pulse) and lands there,
 * the style it will be known when pressed; cancel takes it back; a stop plays it at once. No roll and no UNDO ever
 * changes the tempo. */
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

static void pulse(void)
{
    MelloEvent ev;
    (void)mello_engine_advance_pulse(&e);
    while (mello_event_pop(&e, &ev)) {
    }
}
static bool lead_same(const MelloStep *was)
{
    return !memcmp(was, e.project.steps, sizeof e.project.steps);
}

int main(void)
{
    static MelloStep was[MELLO_MAX_STEPS];
    MelloProject p;
    uint8_t st, i;

    mello_project_defaults(&p);
    p.bpm = 117u;
    mello_engine_init(&e, &p, 5u);

    /* stopped: at once */
    st = mello_engine_style_roll_at_bar(&e, MELLO_STYLE_ANY, MELLO_PART_ALL);
    CHECK(st >= MELLO_STYLE_AMBIENT && e.style_now == st && !e.roll_wait,
          "stopped, the roll did not land at once");
    CHECK(e.project.bpm == 117u, "a roll changed the tempo to %u", e.project.bpm);

    /* playing: on the next bar's one */
    mello_engine_start(&e);
    for (i = 0u; i < 3u; ++i) pulse();
    memcpy(was, e.project.steps, sizeof was);
    st = mello_engine_style_roll_at_bar(&e, MELLO_STYLE_ANY, MELLO_PART_ALL);
    CHECK(e.roll_wait == 1u && e.roll_style == st && st >= MELLO_STYLE_AMBIENT, "the roll does not wait");
    for (i = 3u; i < 16u; ++i) {
        pulse();
        CHECK(e.roll_wait && lead_same(was), "the roll landed at pulse %u, before the one", i);
    }
    pulse(); /* the bar's one */
    CHECK(!e.roll_wait && e.style_now == st && !lead_same(was), "the roll did not land on the one");
    CHECK(e.project.bpm == 117u, "the roll on the one changed the tempo to %u", e.project.bpm);

    /* cancel */
    memcpy(was, e.project.steps, sizeof was);
    (void)mello_engine_style_roll_at_bar(&e, MELLO_STYLE_TECHNO, MELLO_PART_LEAD);
    pulse();
    mello_engine_roll_cancel(&e);
    for (i = 0u; i < 20u; ++i) pulse();
    CHECK(lead_same(was), "a cancelled roll landed");

    /* the lead by INT, and a stop plays it at once */
    memcpy(was, e.project.steps, sizeof was);
    pulse();
    mello_engine_randomize_at_bar(&e, MELLO_RANDOMIZE_HIGH);
    CHECK(e.roll_wait == 2u && lead_same(was), "INT's roll did not wait");
    mello_engine_stop(&e);
    CHECK(!e.roll_wait && !lead_same(was), "a stop did not play the waiting roll");

    /* UNDO leaves the tempo */
    (void)mello_engine_style_roll(&e, MELLO_STYLE_DUB);
    e.project.bpm = 140u;
    CHECK(mello_engine_undo_swap(&e) && e.project.bpm == 140u, "UNDO changed the tempo to %u", e.project.bpm);

    if (fails) return 1;
    puts("PASS: roll_on_the_one");
    return 0;
}
