/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* HOME's knobs are yours (DECISIONS.md): EDIT tapped on HOME is still KEYS; EDIT held half a second is MACROS, where
 * each of K1-K4 picks what it will turn, round the list of performance parameters; a tap leaves. Out of MACROS the
 * knob turns what it was given. The choice is saved with the project (SEC_MACROS) and comes back after a boot; a
 * record without the section gives the HTML's four. Nothing on the list is a black key's or an encoder's job. */
#include "mello_engine.h"
#include "mello_storage.h"
#include "mello_transport.h"
#include "mello_ui.h"
#include <stdio.h>
#include <string.h>

#define FLASH_SIZE (MELLO_STORAGE_BANK0_ADDR + 2u * MELLO_STORAGE_BANK_SIZE)
static uint8_t flash[FLASH_SIZE];
static bool ram_read(void *ctx, uint32_t a, void *dst, uint32_t n)
{
    (void)ctx;
    if (a + n > FLASH_SIZE) return false;
    memcpy(dst, flash + a, n);
    return true;
}
static bool ram_erase4k(void *ctx, uint32_t a)
{
    (void)ctx;
    if (a % 4096u || a + 4096u > FLASH_SIZE) return false;
    memset(flash + a, 0xFF, 4096u);
    return true;
}
static bool ram_program(void *ctx, uint32_t a, const void *src, uint32_t n)
{
    const uint8_t *p = src;
    uint32_t i;
    (void)ctx;
    if (a + n > FLASH_SIZE) return false;
    for (i = 0u; i < n; ++i) flash[a + i] &= p[i];
    return true;
}
static const MelloStorageIo io = {ram_read, ram_erase4k, ram_program, NULL};

static char header[64], label[4][16];
static void fill(void *c, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t col)
{
    (void)c;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)col;
}
static void text(void *c, int16_t x, int16_t y, int16_t w, uint8_t large, const char *s, uint16_t col,
                 uint8_t al)
{
    (void)c;
    (void)w;
    (void)large;
    (void)col;
    (void)al;
    if (!s) return;
    if (y == 7) { /* the header's words, in order */
        size_t n = strlen(header);
        snprintf(header + n, sizeof header - n, "%s%s", n ? " " : "", s);
    } else if (y == 190 && x >= 2) { /* (G7: K1-K4 one row lower) */
        int k = (x - 2) / 60;
        if (k >= 0 && k < 4) snprintf(label[k], sizeof label[k], "%s", s);
    }
}
static void led(void *c, uint8_t i, bool on)
{
    (void)c;
    (void)i;
    (void)on;
}
static void midi(void *c, uint8_t a, uint8_t b, uint8_t d)
{
    (void)c;
    (void)a;
    (void)b;
    (void)d;
}

static MelloTransport t;
static MelloStorage st;
static MelloUi ui;

static void render(void)
{
    MelloUiDrawOps ops = {fill, text, led, led, 0, 0, 0};
    header[0] = 0;
    memset(label, 0, sizeof label);
    ui.dirty = MELLO_UI_DIRTY_ALL;
    mello_ui_render(&ui, &ops);
}
static void in(uint32_t ms, uint32_t pressed, uint32_t released, uint32_t held, int knob, int delta)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    if (knob >= 0) enc[MELLO_UI_ENC_KNOB1 + knob] = (int16_t)delta;
    mello_ui_input(&ui, ms * 1000u, ms, pressed, released, held, 0u, enc);
}
#define EDIT (1u << MELLO_UI_BTN_EDIT)
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                      \
            printf(__VA_ARGS__);                                                                             \
            printf("\n");                                                                                    \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)
static int labels_are(const char *a, const char *b, const char *c, const char *d)
{
    ui.readout = 0u; /* G7: the readout covers K1-K4 for a moment; read them under it */
    render();
    return !strcmp(label[0], a) && !strcmp(label[1], b) && !strcmp(label[2], c) && !strcmp(label[3], d);
}

int main(void)
{
    static MelloProject p;
    uint16_t res0;
    uint32_t ms = 1000u;
    MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;

    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 1u, midi, NULL);
    mello_storage_init(&st, &io, NULL);
    (void)mello_storage_load(&st, &t.engine); /* nothing there: the defaults */
    mello_ui_init(&ui, &t, &st);
    CHECK(labels_are("CUTOFF", "DECAY", "SPACE", "DRIVE"), "the default macros are the HTML's: %s %s %s %s",
          label[0], label[1], label[2], label[3]);

    /* a tap of EDIT is KEYS, as before */
    in(ms, EDIT, 0u, EDIT, -1, 0);
    in(ms += 120u, 0u, EDIT, 0u, -1, 0);
    render();
    CHECK(strstr(header, "KEYS"), "a tap of EDIT should give KEYS, the header reads \"%s\"", header);
    in(ms += 500u, EDIT, 0u, EDIT, -1, 0);
    in(ms += 120u, 0u, EDIT, 0u, -1, 0);
    render();
    CHECK(!strstr(header, "KEYS") && !strstr(header, "MACROS"), "a second tap leaves KEYS: \"%s\"", header);

    /* EDIT held half a second is MACROS; letting go then stays in it */
    in(ms += 500u, EDIT, 0u, EDIT, -1, 0);
    in(ms += 300u, 0u, 0u, EDIT, -1, 0);
    render();
    CHECK(!strstr(header, "MACROS"), "300 ms is not a hold yet: \"%s\"", header);
    in(ms += 300u, 0u, 0u, EDIT, -1, 0);
    render();
    CHECK(strstr(header, "MACROS"), "EDIT held 600 ms should open MACROS: \"%s\"", header);
    in(ms += 100u, 0u, EDIT, 0u, -1, 0);
    render();
    CHECK(strstr(header, "MACROS") && !strstr(header, "KEYS"), "letting go stays in MACROS: \"%s\"", header);

    /* the knobs pick: K1 up twice (RES, DECAY), K4 down once (SPACE), K2 round the end of the list */
    res0 = t.engine.sound.filter_res;
    in(ms += 100u, 0u, 0u, 0u, 0, 1);
    CHECK(labels_are("RES", "DECAY", "SPACE", "DRIVE"), "K1 +1: %s", label[0]);
    in(ms += 100u, 0u, 0u, 0u, 0, 1);
    CHECK(labels_are("DECAY", "DECAY", "SPACE", "DRIVE"), "K1 +1 again: %s", label[0]);
    in(ms += 100u, 0u, 0u, 0u, 3, -1);
    CHECK(labels_are("DECAY", "DECAY", "SPACE", "SPACE"), "K4 -1: %s", label[3]);
    in(ms += 100u, 0u, 0u, 0u, 1, -3); /* a turn of any size is one step, as the other pickers */
    CHECK(labels_are("DECAY", "RES", "SPACE", "SPACE"), "K2 -3 from DECAY is one step back, RES: %s",
          label[1]);
    {
        int k;
        for (k = 0; k < 6; ++k)
            in(ms += 100u, 0u, 0u, 0u, 2, -1); /* SPACE back past the start: the list wraps */
    }
    CHECK(labels_are("DECAY", "RES", "SWING", "SPACE"), "K3 six back from SPACE wraps to SWING: %s",
          label[2]);
    CHECK(t.engine.sound.filter_res == res0, "picking must not turn the parameter (RES moved %u -> %u)", res0,
          t.engine.sound.filter_res);
    CHECK(t.engine.macros.id[0] == MELLO_MACRO_DECAY && t.engine.macros.id[1] == MELLO_MACRO_RES &&
              t.engine.macros.id[2] == MELLO_MACRO_SWING,
          "the engine holds the choice (%u %u %u)", t.engine.macros.id[0], t.engine.macros.id[1],
          t.engine.macros.id[2]);

    /* a tap leaves MACROS; now the knobs turn what they were given */
    in(ms += 100u, EDIT, 0u, EDIT, -1, 0);
    in(ms += 100u, 0u, EDIT, 0u, -1, 0);
    render();
    CHECK(!strstr(header, "MACROS") && !strstr(header, "KEYS"), "a tap leaves MACROS: \"%s\"", header);
    {
        uint16_t swing0 = t.engine.drums.swing, rw0 = t.engine.fx.reverb_wet;
        in(ms += 100u, 0u, 0u, 0u, 1, 2);
        CHECK(t.engine.sound.filter_res == res0 + 20u, "K2 is RES now: %u -> %u", res0,
              t.engine.sound.filter_res);
        in(ms += 100u, 0u, 0u, 0u, 2, 3);
        CHECK(t.engine.drums.swing == swing0 + 30u, "K3 is SWING now: %u -> %u", swing0,
              t.engine.drums.swing);
        in(ms += 100u, 0u, 0u, 0u, 3, 1);
        CHECK(t.engine.fx.reverb_wet == rw0 + 10u && t.engine.fx.delay_wet == (rw0 + 10u) / 2u,
              "K4 is SPACE now (reverb %u, delay %u)", t.engine.fx.reverb_wet, t.engine.fx.delay_wet);
    }

    /* saved with the project, back after a boot; a record without the section gives the defaults */
    mello_storage_request_save(&st, ms);
    for (; ms < 40000u && w != MELLO_STORAGE_SAVE_WRITTEN; ms += 100u)
        w = mello_storage_poll(&st, &t.engine, ms, true);
    CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "the project did not save (%d)", (int)w);
    {
        static MelloTransport t2;
        static MelloStorage st2;
        mello_transport_init(&t2, &p, 1u, midi, NULL);
        mello_storage_init(&st2, &io, NULL);
        CHECK(mello_storage_load(&st2, &t2.engine) == MELLO_STORAGE_LOAD_OK,
              "the saved project did not load");
        CHECK(t2.engine.macros.id[0] == MELLO_MACRO_DECAY && t2.engine.macros.id[1] == MELLO_MACRO_RES &&
                  t2.engine.macros.id[2] == MELLO_MACRO_SWING && t2.engine.macros.id[3] == MELLO_MACRO_SPACE,
              "the macros came back as %u %u %u %u", t2.engine.macros.id[0], t2.engine.macros.id[1],
              t2.engine.macros.id[2], t2.engine.macros.id[3]);
    }
    {
        MelloMacroConfig bad = {{200u, 1u, 2u, 3u}};
        mello_engine_macros_set(&t.engine, &bad);
        CHECK(t.engine.macros.id[0] == 0u, "an unknown id falls back to the knob's default");
    }

    printf("PASS: EDIT tapped is KEYS, held is MACROS; the knobs pick from %u parameters, turn what they "
           "picked, "
           "and the choice survives a save and a boot\n",
           (unsigned)MELLO_MACRO_COUNT);
    return 0;
}
