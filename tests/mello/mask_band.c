/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* MASK's BAND (DECISIONS "MASK's BAND"). The product owner, testing: a whole thing built around one theme, the mask on
 * PITCH alone to roll only the melody, and a tap of RNDMZR still rolled another bass, style, chords and drums. BAND,
 * the key after the targets on RANDOM > MASK: off, a tap rolls the lead alone, within the mask, in the style that
 * plays; on (a new project's, and an older project's), the band, as before. NONE and ALL set the lead's targets and
 * leave BAND. Saved with the mask. And with every part but the lead locked, a tap keeps the style. */
#include <stdio.h>
#include <string.h>

#include "mello_storage.h"
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

#define FLASH_SIZE (MELLO_STORAGE_BANK0_ADDR + 2u * MELLO_STORAGE_BANK_SIZE)
static uint8_t flash[FLASH_SIZE];
static bool ram_read(void *c, uint32_t ad, void *dst, uint32_t n)
{
    (void)c;
    if (ad + n > FLASH_SIZE) return false;
    memcpy(dst, flash + ad, n);
    return true;
}
static bool ram_erase4k(void *c, uint32_t ad)
{
    (void)c;
    if (ad % 4096u || ad + 4096u > FLASH_SIZE) return false;
    memset(flash + ad, 0xFF, 4096u);
    return true;
}
static bool ram_program(void *c, uint32_t ad, const void *src, uint32_t n)
{
    const uint8_t *q = src;
    uint32_t i;
    (void)c;
    if (ad + n > FLASH_SIZE) return false;
    for (i = 0u; i < n; ++i) flash[ad + i] &= q[i];
    return true;
}
static const MelloStorageIo io = {ram_read, ram_erase4k, ram_program, NULL};

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

/* the key LEDs, as the panel shows them */
static uint32_t key_lit, key_glow;
static void no_fill(void *c, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t k)
{
    (void)c, (void)x, (void)y, (void)w, (void)h, (void)k;
}
static void no_text(void *c, int16_t x, int16_t y, int16_t w, uint8_t l, const char *t, uint16_t k, uint8_t a)
{
    (void)c, (void)x, (void)y, (void)w, (void)l, (void)t, (void)k, (void)a;
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
static MelloStorage st;
static MelloUi u;
static MelloEngine before;
static uint32_t now_ms = 1000u;
static int16_t enc[MELLO_UI_ENCODER_COUNT];

static void tap(uint8_t note)
{
    mello_ui_set_held_notes(&u, 1u << note);
    mello_ui_input(&u, now_ms * 1000u, now_ms, 0u, 0u, 0u, 1u << note, enc);
    now_ms += 60u;
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, now_ms * 1000u, now_ms, 0u, 0u, 0u, 0u, enc);
    now_ms += 200u;
}
static void rndmzr(void) /* a tap of RNDMZR on HOME, stopped: the roll lands at once */
{
    mello_ui_set_page(&u, MELLO_UI_HOME);
    before = t.engine;
    tap(black_note(9u));
}
static unsigned pitches_rolled(void)
{
    unsigned i, n = 0u;
    for (i = 0u; i < MELLO_MAX_STEPS; ++i)
        n += t.engine.project.steps[i].pitch != before.project.steps[i].pitch;
    return n;
}
static bool rest_of_lead_same(void)
{
    MelloProject q = t.engine.project;
    unsigned i;
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) q.steps[i].pitch = before.project.steps[i].pitch;
    return !memcmp(&q, &before.project, sizeof q);
}
static bool band_same(void)
{
    return !memcmp(&t.engine.bass, &before.bass, sizeof t.engine.bass) &&
           !memcmp(&t.engine.sub, &before.sub, sizeof t.engine.sub) &&
           !memcmp(&t.engine.drums, &before.drums, sizeof t.engine.drums) &&
#if MELLO_FEATURE_CHORD_MACHINE
           !memcmp(&t.engine.chord, &before.chord, sizeof t.engine.chord) &&
#endif
           !memcmp(&t.engine.sound, &before.sound, sizeof t.engine.sound) &&
           !memcmp(&t.engine.fx, &before.fx, sizeof t.engine.fx) && t.engine.style_now == before.style_now;
}

int main(void)
{
    MelloUiDrawOps o;
    uint8_t band_key = white_note(MELLO_FEATURE_CHORD_MACHINE ? 13u : 12u); /* the key after the targets */
    uint8_t k;
    memset(&o, 0, sizeof o);
    o.fill = no_fill, o.text = no_text, o.key_led = key_led, o.button_led = button_led,
    o.key_glow = key_glow_cb;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 7u, 0, 0);
    memset(flash, 0xFF, sizeof flash);
    mello_storage_init(&st, &io, NULL);
    mello_ui_init(&u, &t, &st);
    u.random_intensity = MELLO_RANDOMIZE_GROOVE; /* STYLE ANY: a tap rolls the band in a new style */
    (void)mello_engine_style_roll(&t.engine, MELLO_STYLE_TRIPHOP); /* the band the theme was built on */
    CHECK(!(t.engine.randomize.mask & MELLO_RAND_BAND_OFF), "a new project's BAND is off");

    /* the mask on PITCH alone, BAND on: the band rolls, as before (the product owner's tap) */
    mello_engine_set_randomize_mask(&t.engine, MELLO_RAND_PITCH);
    rndmzr();
    CHECK(!band_same(), "BAND on: a tap did not roll the band");

    /* BAND off on RANDOM > MASK, the key after the targets: dark there, still glowing */
    mello_ui_set_page(&u, MELLO_UI_RANDOM);
    tap(band_key);
    CHECK(t.engine.randomize.mask == (MELLO_RAND_PITCH | MELLO_RAND_BAND_OFF),
          "BAND's key made the mask %04x", t.engine.randomize.mask);
    mello_ui_sync_leds(&u, &o);
    CHECK(key_lit & (1u << white_note(0u)), "PIT, on, is not lit");
    CHECK(!(key_lit & (1u << band_key)) && (key_glow & (1u << band_key)), "BAND off: its key lit %d, glow %d",
          (int)!!(key_lit & (1u << band_key)), (int)!!(key_glow & (1u << band_key)));

    /* BAND off: the melody alone, within the mask, in the style that plays */
    for (k = 0u; k < 3u; ++k) {
        rndmzr();
        CHECK(pitches_rolled() > 0u, "BAND off: tap %u rolled no pitch", k + 1u);
        CHECK(rest_of_lead_same(),
              "BAND off, the mask PITCH: tap %u changed more of the lead than its pitches", k + 1u);
        CHECK(band_same(), "BAND off: tap %u rolled the band (style %s -> %s)", k + 1u,
              mello_style_name(before.style_now), mello_style_name(t.engine.style_now));
    }

    /* NONE and ALL set the lead's targets and leave BAND */
    mello_ui_set_page(&u, MELLO_UI_RANDOM);
    tap(black_note(0u)); /* NONE */
    CHECK(t.engine.randomize.mask == MELLO_RAND_BAND_OFF, "NONE made the mask %04x", t.engine.randomize.mask);
    tap(black_note(1u)); /* ALL */
    CHECK(t.engine.randomize.mask == (MELLO_RAND_ALL | MELLO_RAND_BAND_OFF), "ALL made the mask %04x",
          t.engine.randomize.mask);
    mello_engine_set_randomize_mask(&t.engine, MELLO_RAND_PITCH | MELLO_RAND_BAND_OFF);

    /* saved with the mask, and read back; an older project (no such bit) reads BAND on */
    {
        static MelloEngine e2;
        MelloProject q;
        CHECK(mello_storage_save_now(&st, &t.engine, now_ms) == MELLO_STORAGE_SAVE_WRITTEN,
              "the save failed");
        mello_project_defaults(&q);
        mello_engine_init(&e2, &q, 2u);
        mello_storage_init(&st, &io, NULL);
        CHECK(mello_storage_load(&st, &e2) == MELLO_STORAGE_LOAD_OK &&
                  e2.randomize.mask == (MELLO_RAND_PITCH | MELLO_RAND_BAND_OFF),
              "BAND off did not survive a save (mask %04x)", e2.randomize.mask);
        mello_engine_set_randomize_mask(&e2, MELLO_RAND_DEFAULT);
        CHECK(!(e2.randomize.mask & MELLO_RAND_BAND_OFF), "a mask from before reads BAND off");
    }

    /* BAND on again: the band rolls */
    mello_ui_set_page(&u, MELLO_UI_RANDOM);
    tap(band_key);
    CHECK(t.engine.randomize.mask == MELLO_RAND_PITCH, "BAND's key again made the mask %04x",
          t.engine.randomize.mask);
    rndmzr();
    CHECK(!band_same(), "BAND on again: a tap did not roll the band");

    /* every part but the lead locked: a tap keeps the style (it used to name a new one, the band playing the old) */
    t.engine.roll_lock = MELLO_PART_ALL & ~MELLO_PART_LEAD;
    rndmzr();
    CHECK(pitches_rolled() > 0u, "all but the lead locked: no pitch rolled");
    CHECK(band_same(), "all but the lead locked: the style went %s -> %s, or a part rolled",
          mello_style_name(before.style_now), mello_style_name(t.engine.style_now));

    if (fails) return 1;
    puts("PASS: mask_band");
    return 0;
}
