/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* ON, the same on every page (DECISIONS "One thread", G1): wherever B0 says ON, its key is lit while the page's part
 * is on and dark while it is off, and its label in the strip bright the same. The product owner, 2026-11-10: "ON is
 * not always the same on the same black key"; on CHORD and SAMPLE it switched the part but never lit. */
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

/* the strip's first label (the screen's last row, 223) and its colour (bright on a cell that is on), B0's LED */
static char first[32];
static uint16_t first_colour;
static bool first_seen;
static uint32_t key_lit;
static void fill(void *c, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t k)
{
    (void)c, (void)x, (void)y, (void)w, (void)h, (void)k;
}
static void text(void *c, int16_t x, int16_t y, int16_t w, uint8_t l, const char *t, uint16_t k, uint8_t a)
{
    (void)c, (void)x, (void)w, (void)l, (void)a;
    if (y == 223 && !first_seen) {
        snprintf(first, sizeof first, "%s", t);
        first_colour = k;
        first_seen = true;
    }
}
static void key_led(void *c, uint8_t k, bool on)
{
    (void)c;
    if (on)
        key_lit |= 1u << k;
    else
        key_lit &= ~(1u << k);
}
static void key_glow(void *c, uint8_t k, bool on)
{
    (void)c, (void)k, (void)on;
}
static void button_led(void *c, uint8_t b, bool on)
{
    (void)c, (void)b, (void)on;
}

static MelloProject p;
static MelloTransport t;
static MelloUi u;
static MelloUiDrawOps o;
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
static bool b0_lit(void)
{
    first_seen = false, first[0] = 0;
    u.dirty = MELLO_UI_DIRTY_ALL;
    mello_ui_render(&u, &o);
    mello_ui_sync_leds(&u, &o);
    return (key_lit >> black_note(0u)) & 1u;
}

int main(void)
{
    uint8_t pg, ons = 0u;
    memset(&o, 0, sizeof o);
    o.fill = fill, o.text = text, o.key_led = key_led, o.key_glow = key_glow, o.button_led = button_led;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 7u, 0, 0);
    mello_ui_init(&u, &t, 0);

    for (pg = 0u; pg <= (uint8_t)MELLO_UI_MIX; ++pg) {
        bool was, now;
        uint16_t colour;
        mello_ui_set_page(&u, (MelloUiPage)pg);
        idle(20u);
        was = b0_lit();
        if (strcmp(first, "ON")) continue; /* no ON on this page */
        ++ons;
        colour = first_colour;
        tap(black_note(0u));
        now = b0_lit();
        CHECK(now != was, "page %u: ON tapped, its key stayed %s", pg, was ? "lit" : "dark");
        CHECK(first_colour != colour, "page %u: ON tapped, its label kept its colour", pg);
        tap(black_note(0u));
        CHECK(b0_lit() == was, "page %u: ON tapped again, its key not back", pg);
    }
    CHECK(ons >= 8u, "only %u pages with ON", ons);

    if (fails) return 1;
    printf("PASS: on_key (%u pages with ON)\n", ons);
    return 0;
}
