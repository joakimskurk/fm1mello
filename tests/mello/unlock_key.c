/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* UNLOCK (DECISIONS "UNLOCK"). The product owner, after a drum take: the drums locked against RNDMZR is fine, "but how
 * do I unlock it again? It is not intuitive; it must say so somewhere, and an unlock button or the like". While a
 * part is locked, B7 (free on every page) is UNLOCK: in the strip in gold, its key lit, on every page and with room
 * for the rest (UNLOCK on HOME; UNL, then U, where the strip is full); a tap unlocks every part, the readout names
 * them, and RNDMZR rolls them again. With nothing locked B7 is free again: no label, no light, a tap does nothing. */
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

/* the strip's texts (the screen's last row, 223: its rows 171-188 drawn under K1-K4), all texts, the key LEDs */
#define MAX_TEXTS 64
static struct {
    int16_t x, w;
    char s[32];
} strip[MAX_TEXTS];
static unsigned strip_n;
static char said[2048];
static uint32_t key_lit, key_glow;
static void no_fill(void *c, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t k)
{
    (void)c, (void)x, (void)y, (void)w, (void)h, (void)k;
}
static void text(void *c, int16_t x, int16_t y, int16_t w, uint8_t l, const char *t, uint16_t k, uint8_t a)
{
    (void)c, (void)l, (void)k, (void)a;
    if (y == 223 && strip_n < MAX_TEXTS) {
        strip[strip_n].x = x, strip[strip_n].w = w;
        snprintf(strip[strip_n].s, sizeof strip[strip_n].s, "%s", t);
        strip_n++;
    }
    if (strlen(said) + strlen(t) + 2u < sizeof said) strcat(strcat(said, t), "|");
}
static void key_led(void *c, uint8_t k, bool on)
{
    (void)c;
    if (on)
        key_lit |= 1u << k;
    else
        key_lit &= ~(1u << k);
}
static void key_glow_cb(void *c, uint8_t k, bool on)
{
    (void)c;
    if (on)
        key_glow |= 1u << k;
    else
        key_glow &= ~(1u << k);
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
static void render(void)
{
    strip_n = 0u, said[0] = 0;
    u.dirty = MELLO_UI_DIRTY_ALL;
    mello_ui_render(&u, &o);
    mello_ui_sync_leds(&u, &o);
}
static bool strip_has(const char *s)
{
    unsigned i;
    for (i = 0u; i < strip_n; ++i)
        if (!strcmp(strip[i].s, s)) return true;
    return false;
}
static bool strip_unlock(void) /* B7's label, as short as the strip's room asks */
{
    return strip_has("UNLOCK") || strip_has("UNL") || strip_has("U");
}
static bool strip_apart(void) /* each label in its own cell, inside the screen */
{
    unsigned i, j;
    for (i = 0u; i < strip_n; ++i) {
        if (strip[i].x < 0 || strip[i].x + strip[i].w > 240) return false;
        for (j = 0u; j < strip_n; ++j)
            if (i != j && strip[i].x < strip[j].x + strip[j].w && strip[j].x < strip[i].x + strip[i].w)
                return false;
    }
    return true;
}

int main(void)
{
    const uint32_t b7 = 1u << black_note(7u);
    uint8_t pg, k;
    uint32_t hits[MELLO_DRUM_VOICES];
    memset(&o, 0, sizeof o);
    o.fill = no_fill, o.text = text, o.key_led = key_led, o.key_glow = key_glow_cb, o.button_led = button_led;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 7u, 0, 0);
    mello_ui_init(&u, &t, 0);
    u.random_intensity = MELLO_RANDOMIZE_GROOVE;

    /* nothing locked: B7 free (no label, no light), a tap does nothing */
    render();
    CHECK(!strip_unlock(), "nothing locked: UNLOCK in the strip");
    CHECK(!((key_lit | key_glow) & b7), "nothing locked: B7 lights");
    tap(black_note(7u));
    CHECK(u.readout != MELLO_UI_READOUT_UNLOCK, "nothing locked: B7 showed UNLOCKED");

    /* a drum take on DRUMS: the drums locked, and UNLOCK there and on every page, lit, with room for the rest */
    mello_ui_set_page(&u, MELLO_UI_DRUMS);
    button(MELLO_UI_BTN_REC);
    for (k = 0u; k < 3u; ++k) tap(white_note(0u));
    button(MELLO_UI_BTN_REC);
    CHECK(t.engine.roll_lock == MELLO_PART_DRUMS, "the drum take locked %02x", t.engine.roll_lock);
    for (pg = 0u; pg <= (uint8_t)MELLO_UI_MIX; ++pg) {
        mello_ui_set_page(&u, (MelloUiPage)pg);
        idle(20u);
        render();
        CHECK(pg ? strip_unlock() : strip_has("UNLOCK"),
              "page %u: no UNLOCK in the strip while the drums are locked", pg);
        CHECK(strip_apart(), "page %u: the strip's labels overlap with UNLOCK in it", pg);
        CHECK(key_lit & b7, "page %u: B7 dark while the drums are locked", pg);
    }

    /* UNLOCK on HOME: every lock off, the readout names the drums, B7 free again, and RNDMZR rolls the drums */
    mello_ui_set_page(&u, MELLO_UI_HOME);
    t.engine.roll_lock |= MELLO_PART_LEAD; /* (RNDMZR held and B0: a second part) */
    tap(black_note(7u));
    CHECK(t.engine.roll_lock == 0u, "UNLOCK left %02x locked", t.engine.roll_lock);
    CHECK(u.readout == MELLO_UI_READOUT_UNLOCK && u.unlocked == (MELLO_PART_LEAD | MELLO_PART_DRUMS),
          "UNLOCK's readout %u, parts %02x", u.readout, u.unlocked);
    render();
    CHECK(strstr(said, "UNLOCKED") && strstr(said, "LEAD DRUMS"), "the readout said %s", said);
    CHECK(!strip_unlock(), "unlocked: UNLOCK still in the strip");
    CHECK(!((key_lit | key_glow) & b7), "unlocked: B7 still lights");
    memcpy(hits, t.engine.drums.hits, sizeof hits);
    tap(black_note(9u)); /* RNDMZR */
    CHECK(memcmp(hits, t.engine.drums.hits, sizeof hits), "unlocked: RNDMZR left the drums' hits");

    /* three parts or more: how many */
    t.engine.roll_lock = MELLO_PART_BASS | MELLO_PART_KIT | MELLO_PART_SOUND;
    tap(black_note(7u));
    render();
    CHECK(strstr(said, "3 PARTS") != 0, "three parts unlocked: the readout said %s", said);

    if (fails) return 1;
    puts("PASS: unlock_key");
    return 0;
}
