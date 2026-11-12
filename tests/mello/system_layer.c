/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* GLOBAL > SYSTEM (DECISIONS "The second core", "SEED"): the fourth layer of GLOBAL. K1 is CORE 2, the switch the
 * platform saves for the next power-on (OFF on a new FM-1); K2 SEED, AUTO (a new FM-1's) or 1..999, a detent a
 * step, stopping at both ends; CPU is the render's load and CPU 2 the second core's jobs'. The picture has the two
 * cores, CORE 2 lit while it runs and why one when it was asked for (NO REPLY, GAVE UP: NOW's before SEED took its
 * knob), and in gold what the next power-on will do when the switch differs from now. The switch is the FM-1's, not
 * the project's: K2-K4 leave it, and the host shows one core, off. */
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

static char said[4096];
static void fill(void *c, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t k)
{
    (void)c, (void)x, (void)y, (void)w, (void)h, (void)k;
}
static void text(void *c, int16_t x, int16_t y, int16_t w, uint8_t l, const char *t, uint16_t k, uint8_t a)
{
    (void)c, (void)x, (void)y, (void)w, (void)l, (void)k, (void)a;
    if (strlen(said) + strlen(t) + 2u < sizeof said) strcat(strcat(said, t), "|");
}
static void key_led(void *c, uint8_t k, bool on)
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

static void idle(uint32_t ms)
{
    uint32_t end = now_ms + ms;
    memset(enc, 0, sizeof enc);
    for (; now_ms < end; now_ms += 10u) mello_ui_input(&u, now_ms * 1000u, now_ms, 0u, 0u, 0u, 0u, enc);
}
static void turn(uint8_t which, int16_t d) /* 0 SELECT (the layer), 2..5 K1..K4 */
{
    memset(enc, 0, sizeof enc);
    enc[which] = d;
    mello_ui_input(&u, now_ms * 1000u, now_ms, 0u, 0u, 0u, 0u, enc);
    now_ms += 10u;
    idle(2000u); /* past the readout */
}
static void render(void)
{
    said[0] = 0;
    u.dirty = MELLO_UI_DIRTY_ALL;
    mello_ui_render(&u, &o);
}
static bool has(const char *s)
{
    char k[64];
    snprintf(k, sizeof k, "|%s|", s);
    return strstr(said, k + 1) == said || strstr(said, k) != 0;
}

int main(void)
{
    uint8_t k;
    memset(&o, 0, sizeof o);
    o.fill = fill, o.text = text, o.key_led = key_led, o.key_glow = key_led, o.button_led = button_led;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 7u, 0, 0);
    mello_ui_init(&u, &t, 0);

    /* GLOBAL's layers: CLOCK, MIDI, GROOVE, SYSTEM, then CLOCK again */
    mello_ui_set_page(&u, MELLO_UI_GLOBAL);
    idle(20u);
    for (k = 0u; k < 3u; ++k) turn(0u, 1);
    render();
    CHECK(u.global_subpage == 3u && has("SYSTEM"), "three SELECT steps on GLOBAL: layer %u, not SYSTEM",
          u.global_subpage);

    /* the host: one core, the switch off */
    CHECK(has("CORE 2") && has("OFF") && has("SEED") && has("AUTO") && u.seed == 0u,
          "SYSTEM on the host said %s", said);
    CHECK(!strstr(said, "POWER-ON"), "nothing pending, yet: %s", said);

    /* the FM-1 with both cores running, switched on: CPU for the render, CPU 2 for the second core's jobs */
    mello_ui_set_cores(&u, MELLO_UI_CORE_ON, true);
    mello_ui_set_audio_load(&u, 537u);
    mello_ui_set_audio_load2(&u, 412u);
    render();
    CHECK(has("ON") && has("CORE 1") && has("CORE 2") && !has("NO REPLY") && !has("GAVE UP"), "two cores: %s",
          said);
    CHECK(has("CPU") && has("54%") && has("CPU 2") && has("41%"), "the two loads: %s", said);
    CHECK(!strstr(said, "POWER-ON"), "two cores, on: something pending: %s", said);

    /* K1 down: off at the next power-on; K2-K4 leave the switch; K1 up again: nothing pending */
    turn(2u, -1);
    render();
    CHECK(!u.second_core, "K1 down left CORE 2 on");
    CHECK(has("OFF AT NEXT POWER-ON"), "switched off while two run: %s", said);
    for (k = 3u; k < 6u; ++k) turn(k, 1);
    CHECK(!u.second_core, "K2-K4 turned CORE 2 on");
    render();
    CHECK(u.seed == 1u && has("SEED") && has("1"), "K2 a detent up: SEED %u (%s)", u.seed, said);
    turn(3u, -1), turn(3u, -1);
    CHECK(u.seed == 0u, "K2 two down from 1: SEED %u, not AUTO", u.seed);
    turn(3u, 2000);
    CHECK(u.seed == 999u, "K2 far up: SEED %u, not 999", u.seed);
    mello_ui_set_seed(&u, 1200u);
    CHECK(u.seed == 0u, "a saved SEED past 999 taken as %u, not AUTO", u.seed);
    mello_ui_set_seed(&u, 42u);
    render();
    CHECK(u.seed == 42u && has("42"), "the saved SEED 42 shown as %s", said);
    turn(2u, 1);
    render();
    CHECK(u.second_core && !strstr(said, "POWER-ON"), "K1 up again: %s", said);

    /* switched on, but one core: why (no pending word: it was asked for and did not come) */
    mello_ui_set_cores(&u, MELLO_UI_CORE_NO_REPLY, true);
    render();
    CHECK(has("NO REPLY") && !strstr(said, "POWER-ON"), "no reply: %s", said);
    mello_ui_set_cores(&u, MELLO_UI_CORE_GAVE_UP, true);
    render();
    CHECK(has("GAVE UP") && !strstr(said, "POWER-ON"), "given up: %s", said);

    /* off now, switched on: on at the next power-on */
    mello_ui_set_cores(&u, MELLO_UI_CORE_OFF, false);
    turn(2u, 1);
    render();
    CHECK(u.second_core && has("ON AT NEXT POWER-ON"), "switched on while one runs: %s", said);

    /* the layer wraps to CLOCK */
    turn(0u, 1);
    CHECK(u.global_subpage == 0u, "SELECT past SYSTEM: layer %u", u.global_subpage);

    if (fails) return 1;
    puts("PASS: system_layer");
    return 0;
}
