/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* A clean slate and happy accidents (DECISIONS "A clean slate", "Happy accidents"). HOME's CLR tapped twice empties
 * the sequence's steps to rests (it gave the default scale run before); CLR held a second is INIT: the steps rests,
 * the bass, chords and drums off and empty, the sound and the effects a new project's, the tempo kept; one UNDO takes
 * it back. RNDMZR on MED rolls the lead raw, no style (STYLE ANY rolls the band only with MUSICAL or GROOVE). (The black keys' places:
 * control_context_contract.) */
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

static uint32_t black(uint8_t pos)
{
    uint8_t i, n = 0u;
    for (i = 0u; i < 27u; ++i) {
        if (mello_ui_white_key_position(i) >= 0) continue;
        if (n++ == pos) return 1u << i;
    }
    return 0u;
}
static void hold(uint32_t key, uint32_t for_ms)
{
    uint32_t start = ms;
    mello_ui_set_held_notes(&u, key);
    ms += 10u, mello_ui_input(&u, ms * 1000u, ms, 0u, 0u, 0u, key, enc);
    while (ms - start < for_ms) ms += 50u, mello_ui_input(&u, ms * 1000u, ms, 0u, 0u, 0u, 0u, enc);
    mello_ui_set_held_notes(&u, 0u);
    ms += 10u, mello_ui_input(&u, ms * 1000u, ms, 0u, 0u, 0u, 0u, enc);
}
static unsigned plays(void)
{
    unsigned i, n = 0u;
    for (i = 0u; i < 16u; ++i) n += t.engine.project.steps[i].gate != MELLO_GATE_REST;
    return n;
}

int main(void)
{
    MelloEngine *e;
    uint32_t clr = black(4u), rnd = black(9u);
    uint16_t bpm;
    mello_project_defaults(&p);
    p.bpm = 111u;
    mello_transport_init(&t, &p, 4u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_HOME);
    e = &t.engine;

    /* MED: raw, no style */
    u.random_intensity = MELLO_RANDOMIZE_MED;
    hold(rnd, 50u);
    CHECK(e->style_now == MELLO_STYLE_OFF, "RNDMZR on MED rolled a style (%u)", e->style_now);
    u.random_intensity = MELLO_RANDOMIZE_MUSICAL;
    hold(rnd, 50u);
    CHECK(e->style_now >= MELLO_STYLE_AMBIENT, "RNDMZR on MUSICAL rolled no style");

    /* CLR twice: rests */
    CHECK(plays() > 0u, "nothing plays to clear");
    hold(clr, 50u);
    hold(clr, 50u);
    CHECK(plays() == 0u, "CLR twice left %u steps playing", plays());

    /* CLR held a second: INIT; the tempo kept; UNDO back */
    {
        MelloDrumConfig d = e->drums;
        d.on = true, d.hits[0] = 0x1111u;
        mello_engine_drums_set_config(e, &d);
        e->roll_lock = MELLO_PART_LEAD;
    }
    bpm = e->project.bpm;
    hold(clr, 1200u);
    CHECK(!e->drums.on && !e->drums.hits[0] && !e->bass.on && !e->sub.on && plays() == 0u && !e->roll_lock,
          "INIT left something (drums %d, bass %d, sub %d, %u steps, locks %u)", (int)e->drums.on,
          (int)e->bass.on, (int)e->sub.on, plays(), e->roll_lock);
#if MELLO_FEATURE_CHORD_MACHINE
    CHECK(!e->chord.on, "INIT left the chords on");
#endif
    CHECK(e->project.bpm == bpm && u.readout == MELLO_UI_READOUT_INIT,
          "INIT moved the tempo, or said nothing");
    CHECK(mello_engine_undo_swap(e) && e->drums.on && e->drums.hits[0] == 0x1111u,
          "UNDO did not take INIT back");

    if (fails) return 1;
    puts("PASS: clean_slate");
    return 0;
}
