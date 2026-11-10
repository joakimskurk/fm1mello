/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* A silent lead rolls notes (DECISIONS "A silent lead rolls notes"). The product owner, 2026-11-10: "when I clear,
 * everything goes quiet, and though the lead shows as on I no longer hear anything". CLR leaves every step a rest; a
 * roll changes only what RANDOM's mask lets it, so with GATE masked away (PITCH alone, say) the rests stayed rests
 * through every roll after. Now a lead with nothing to play gets its notes from the roll whatever the mask: at every
 * intensity and in every style. A lead that plays keeps the mask, and a mask of nothing still changes nothing. */
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
static unsigned notes(const MelloEngine *e)
{
    unsigned i, n = 0u, len = e->project.seq_len ? e->project.seq_len : 16u;
    for (i = 0u; i < len && i < MELLO_MAX_STEPS; ++i)
        n += e->project.steps[i].gate != MELLO_GATE_REST && !e->project.steps[i].skip;
    return n;
}
static MelloEngine *fresh(uint16_t mask, uint32_t seed)
{
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, seed, 0, 0);
    mello_ui_init(&u, &t, 0);
    mello_engine_set_randomize_mask(&t.engine, mask);
    return &t.engine;
}

int main(void)
{
    static const char *const names[] = {"LOW", "MED", "HIGH", "MUSICAL", "GROOVE"};
    MelloEngine *e;
    uint8_t in, st;
    uint32_t seed;
    uint16_t gates[MELLO_MAX_STEPS];
    unsigned i;

    /* every intensity, a few seeds: CLR, then a roll with the mask on PITCH alone, plays */
    for (in = MELLO_RANDOMIZE_LOW; in <= MELLO_RANDOMIZE_GROOVE; ++in)
        for (seed = 1u; seed <= 8u; ++seed) {
            e = fresh(MELLO_RAND_PITCH, seed);
            mello_engine_clear_steps(e);
            CHECK(notes(e) == 0u, "CLR left %u notes", notes(e));
            mello_engine_randomize_steps_intensity(e, (MelloRandomizeIntensity)in);
            CHECK(notes(e) >= 3u, "%s, seed %u: after CLR a roll (PITCH alone) plays %u notes", names[in],
                  seed, notes(e));
        }

    /* every style: the lead rolled from silence plays */
    for (st = MELLO_STYLE_AMBIENT; st < MELLO_STYLE_COUNT; ++st) {
        e = fresh(MELLO_RAND_PITCH, 5u);
        mello_engine_clear_steps(e);
        mello_engine_style_roll_parts(e, st, MELLO_PART_LEAD);
        CHECK(notes(e) >= 2u, "style %s: after CLR the lead plays %u notes", mello_style_name(st), notes(e));
    }

    /* a lead that plays keeps the mask: PITCH alone leaves its gates */
    e = fresh(MELLO_RAND_PITCH, 3u);
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) gates[i] = e->project.steps[i].gate;
    CHECK(notes(e) > 0u, "the default sequence is silent");
    mello_engine_randomize_steps_intensity(e, MELLO_RANDOMIZE_HIGH);
    for (i = 0u; i < MELLO_MAX_STEPS; ++i)
        if (e->project.steps[i].gate != gates[i]) break;
    CHECK(i == MELLO_MAX_STEPS, "PITCH alone changed a playing lead's gate (step %u)", i);

    /* a mask of nothing changes nothing, a silent lead too */
    e = fresh(0u, 3u);
    mello_engine_clear_steps(e);
    mello_engine_randomize_steps_intensity(e, MELLO_RANDOMIZE_HIGH);
    CHECK(notes(e) == 0u, "a mask of nothing gave the lead %u notes", notes(e));

    /* as the product owner played it: HOME, CLR (B4, twice), RNDMZR (B9): the lead plays again */
    e = fresh(MELLO_RAND_PITCH, 7u);
    u.random_intensity = MELLO_RANDOMIZE_GROOVE;
    tap(black_note(4u));
    tap(black_note(4u));
    CHECK(notes(e) == 0u, "HOME's CLR left %u notes", notes(e));
    tap(black_note(9u));
    CHECK(notes(e) >= 3u, "HOME: after CLR, RNDMZR gave the lead %u notes", notes(e));

    if (fails) return 1;
    puts("PASS: lead_from_silence");
    return 0;
}
