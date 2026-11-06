/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* STYLE has no OFF (2026-11-06, DECISIONS "STYLE has no OFF"). The product owner: "if it is on OFF and still on
 * GROOVE you should get a random style; now it is locked in the style chosen before OFF". On the RANDOM page K2 turned
 * all the way left stops at ANY (it went on to OFF, which rolled the lead alone and left the band in the style). With
 * ANY each tap of RNDMZR on GROOVE rolls the band in a new style, never the one before; a part alone rolls in the style
 * that plays. */
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
static void k2(int16_t d)
{
    int16_t none[MELLO_UI_ENCODER_COUNT] = {0};
    none[MELLO_UI_ENC_KNOB2] = d;
    ms += 20u, mello_ui_input(&u, ms * 1000u, ms, 0u, 0u, 0u, 0u, none);
}

int main(void)
{
    MelloEngine *e;
    MelloDrumConfig drums;
    uint32_t rnd = black(9u);
    uint8_t i, before;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 9u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_RANDOM);
    e = &t.engine;
    u.random_intensity = MELLO_RANDOMIZE_GROOVE;

    /* TECHNO chosen and rolled */
    CHECK(u.random_style == MELLO_STYLE_ANY, "a new project's STYLE is not ANY (%u)", u.random_style);
    for (i = MELLO_STYLE_ANY; i < MELLO_STYLE_TECHNO; ++i) k2(1);
    CHECK(u.random_style == MELLO_STYLE_TECHNO, "K2 did not reach TECHNO (%u)", u.random_style);
    hold(rnd, 50u);
    CHECK(e->style_now == MELLO_STYLE_TECHNO, "RNDMZR on TECHNO rolled %u", e->style_now);

    /* K2 all the way left: ANY, never OFF */
    for (i = 0u; i < 30u; ++i) k2(-1);
    CHECK(u.random_style == MELLO_STYLE_ANY, "K2 all the way left is %u, not ANY", u.random_style);

    /* every tap a new style, the band rolled with it */
    for (i = 0u; i < 8u; ++i) {
        before = e->style_now;
        drums = e->drums;
        hold(rnd, 50u);
        CHECK(e->style_now >= MELLO_STYLE_AMBIENT && e->style_now < MELLO_STYLE_COUNT,
              "tap %u on ANY rolled no style (%u)", i, e->style_now);
        CHECK(e->style_now != before, "tap %u on ANY kept the style before (%u)", i, before);
        CHECK(memcmp(&drums, &e->drums, sizeof drums) || !e->drums.on,
              "tap %u on ANY left the drums as the style before had them", i);
    }

    /* a stale OFF (from before) rolls as ANY */
    u.random_style = MELLO_STYLE_OFF;
    before = e->style_now;
    hold(rnd, 50u);
    CHECK(e->style_now != before && e->style_now >= MELLO_STYLE_AMBIENT,
          "STYLE OFF kept the style before (%u)", before);

    if (fails) return 1;
    puts(
        "PASS: STYLE has no OFF: K2 stops at ANY, and ANY on GROOVE rolls the band in a new style every tap");
    return 0;
}
