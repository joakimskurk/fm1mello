/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The first LED sync after init sends every LED. It used to start from "all lit" and send only what differed, so only
 * the LEDs to be dark: at boot (the panel all off) HOME's button and its lit and glowing keys stayed dark until they
 * changed, and after RESET ALL (an init on the page that was up) a key dark on the old page stayed dark on HOME.
 * Whatever the panel shows, after init and a sync it shows the UI's LEDs; a sync after that sends only what changed. */
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

/* the panel: what its LEDs show, and how many the UI sent */
static uint32_t panel_lit, panel_glow, panel_btn;
static unsigned sent;
static void set_bit(uint32_t *m, uint8_t b, bool on)
{
    if (on)
        *m |= 1u << b;
    else
        *m &= ~(1u << b);
    sent++;
}
static void key_led(void *c, uint8_t k, bool on)
{
    (void)c;
    set_bit(&panel_lit, k, on);
}
static void key_glow(void *c, uint8_t k, bool on)
{
    (void)c;
    set_bit(&panel_glow, k, on);
}
static void button_led(void *c, uint8_t b, bool on)
{
    (void)c;
    set_bit(&panel_btn, b, on);
}

static MelloProject p;
static MelloTransport t;
static MelloUi u;
static MelloUiDrawOps o;

/* the panel as the UI means it: its masks after a sync are what it computed */
static bool panel_shows_ui(void)
{
    return panel_lit == u.last_key_led_mask && panel_glow == u.last_key_glow_mask &&
           panel_btn == u.last_button_led_mask;
}
static void init_on(uint32_t lit, uint32_t glow, uint32_t btn, const char *what)
{
    panel_lit = lit, panel_glow = glow, panel_btn = btn;
    mello_ui_init(&u, &t, 0);
    mello_ui_sync_leds(&u, &o);
    CHECK(panel_shows_ui(),
          "%s: the panel shows keys %07x glow %07x buttons %04x, the UI keys %07x glow %07x "
          "buttons %04x",
          what, panel_lit, panel_glow, panel_btn, u.last_key_led_mask, u.last_key_glow_mask,
          u.last_button_led_mask);
    CHECK(panel_btn & (1u << MELLO_UI_BTN_HOME), "%s: HOME's button is dark on HOME", what);
    CHECK(panel_glow != 0u, "%s: no key glows on HOME (its steps)", what);
}

int main(void)
{
    uint32_t lit, glow, btn;
    memset(&o, 0, sizeof o);
    o.key_led = key_led, o.key_glow = key_glow, o.button_led = button_led;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 7u, 0, 0);

    init_on(0u, 0u, 0u, "boot (the panel all off)");
    init_on(0x07FFFFFFu, 0x07FFFFFFu, 0x3FFFu, "the panel all on");

    /* nothing changed: the next sync sends nothing */
    sent = 0u;
    u.dirty |= MELLO_UI_DIRTY_LEDS;
    mello_ui_sync_leds(&u, &o);
    CHECK(sent == 0u, "a sync with nothing changed sent %u LEDs", sent);

    /* RESET ALL: an init with RANDOM's LEDs up (factory_reset_ui) */
    mello_ui_set_page(&u, MELLO_UI_RANDOM);
    mello_ui_sync_leds(&u, &o);
    CHECK(panel_shows_ui(), "RANDOM: the panel is not RANDOM's");
    lit = panel_lit, glow = panel_glow, btn = panel_btn;
    init_on(lit, glow, btn, "RESET ALL on RANDOM");

    if (fails) return 1;
    puts("PASS: led_first_sync");
    return 0;
}
