/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* FX > DELAY LOOP (ROADMAP H7, DECISIONS "The looping delay"): GLIDE, PITCH, DIFFUSE and FREEZE on the DELAY page's
 * third layer, after SEND and CONFIG on SELECT, both ways round; the knobs a step a detent (PITCH a semitone, FREEZE
 * OFF, ON, REV); saved and loaded (the FX section's version 9) but FREEZE, a performance's state; a project saved
 * before them loads them off, so it sounds as it did. */
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
static bool ram_read(void *ctx, uint32_t address, void *dst, uint32_t length)
{
    (void)ctx;
    if (address + length > FLASH_SIZE) return false;
    memcpy(dst, flash + address, length);
    return true;
}
static bool ram_erase4k(void *ctx, uint32_t address)
{
    (void)ctx;
    if (address % 4096u || address + 4096u > FLASH_SIZE) return false;
    memset(flash + address, 0xFF, 4096u);
    return true;
}
static bool ram_program(void *ctx, uint32_t address, const void *src, uint32_t length)
{
    const uint8_t *p = src;
    uint32_t i;
    (void)ctx;
    if (address + length > FLASH_SIZE) return false;
    for (i = 0u; i < length; ++i) flash[address + i] &= p[i];
    return true;
}
static const MelloStorageIo io = {ram_read, ram_erase4k, ram_program, NULL};
static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v, p[1] = (uint8_t)(v >> 8), p[2] = (uint8_t)(v >> 16), p[3] = (uint8_t)(v >> 24);
}
static MelloEngine e;
static void load(void)
{
    static MelloProject p;
    MelloStorage s;
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 2u);
    mello_storage_init(&s, &io, NULL);
    (void)mello_storage_load(&s, &e);
}
static void storage(void)
{
    static MelloProject p;
    MelloStorage s;
    MelloFxConfig f;
    MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
    uint32_t t = 0u, base = 0u, size, pos;
    uint8_t *pl;
    int patched = 0;
    memset(flash, 0xFF, sizeof flash);
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 1u);
    CHECK(!e.fx.delay_glide && !e.fx.delay_diffuse && !e.fx.delay_pitch && !e.fx.delay_freeze,
          "a new project's loop: GLIDE %u, DIFFUSE %u, PITCH %d, FREEZE %u", e.fx.delay_glide,
          e.fx.delay_diffuse, e.fx.delay_pitch, e.fx.delay_freeze);
    f = e.fx;
    f.delay_glide = 430u, f.delay_diffuse = 610u, f.delay_pitch = -5, f.delay_freeze = 2u, f.olv_rate = 70u;
    mello_engine_fx_set_config(&e, &f);
    f.delay_pitch = 40, f.delay_freeze = 9u;
    mello_engine_fx_set_config(&e, &f);
    CHECK(e.fx.delay_pitch == 24 && e.fx.delay_freeze == 0u, "PITCH 40 kept as %d, FREEZE 9 as %u",
          e.fx.delay_pitch, e.fx.delay_freeze);
    f.delay_pitch = -5, f.delay_freeze = 2u;
    mello_engine_fx_set_config(&e, &f);
    mello_storage_init(&s, &io, NULL);
    mello_storage_request_save(&s, t);
    for (; t <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u) w = mello_storage_poll(&s, &e, t, true);
    CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "not saved");
    load();
    CHECK(e.fx.delay_glide == 430u && e.fx.delay_diffuse == 610u && e.fx.delay_pitch == -5 &&
              e.fx.delay_freeze == 0u && e.fx.olv_rate == 70u,
          "saved, loaded as GLIDE %u, DIFFUSE %u, PITCH %d, FREEZE %u (want 430, 610, -5 and not saved), "
          "RATE %u",
          e.fx.delay_glide, e.fx.delay_diffuse, e.fx.delay_pitch, e.fx.delay_freeze, e.fx.olv_rate);
    /* the same record, its FX section made version 8: its last 5 bytes go */
    if (get32(flash + MELLO_STORAGE_BANK0_ADDR) == MELLO_STORAGE_MAGIC) base = MELLO_STORAGE_BANK0_ADDR;
    if (get32(flash + MELLO_STORAGE_BANK1_ADDR) == MELLO_STORAGE_MAGIC) base = MELLO_STORAGE_BANK1_ADDR;
    CHECK(base != 0u, "no record");
    if (!base) return;
    size = get32(flash + base + 8u);
    pl = flash + base + MELLO_STORAGE_HEADER_SIZE;
    for (pos = 0u; pos + 8u <= size;) {
        uint32_t id = pl[pos] | (uint32_t)pl[pos + 1u] << 8, ver = pl[pos + 2u] | (uint32_t)pl[pos + 3u] << 8,
                 len = get32(pl + pos + 4u);
        if (id == 5u) { /* SEC_FX */
            const uint8_t *k = pl + pos + 8u + len - 5u;
            CHECK(ver == 9u && (k[0] | k[1] << 8) == 430 && (k[2] | k[3] << 8) == 610 && (int8_t)k[4] == -5,
                  "the FX section is version %u, its last 5 bytes not the loop's", ver);
            memmove(pl + pos + 8u + len - 5u, pl + pos + 8u + len, /* (the footer after the payload too) */
                    size - (pos + 8u + len) + MELLO_STORAGE_FOOTER_SIZE);
            len -= 5u, size -= 5u;
            put32(pl + pos + 4u, len);
            put32(flash + base + 8u, size);
            pl[pos + 2u] = 8u, patched = 1;
        }
        pos += 8u + len;
    }
    CHECK(patched, "no FX section");
    put32(flash + base + 16u, mello_storage_crc32(pl, size));
    put32(flash + base + 20u, mello_storage_crc32(flash + base, 20u));
    load();
    CHECK(!e.fx.delay_glide && !e.fx.delay_diffuse && !e.fx.delay_pitch && e.fx.olv_rate == 70u,
          "saved before the loop, loaded as GLIDE %u, DIFFUSE %u, PITCH %d (want off), RATE %u",
          e.fx.delay_glide, e.fx.delay_diffuse, e.fx.delay_pitch, e.fx.olv_rate);
}

static void ui(void)
{
    static MelloProject p;
    static MelloTransport t;
    static MelloStorage st;
    static MelloUi u;
    static const uint8_t fwd[4] = {1u, 2u, 3u, 1u}, back[4] = {1u, 3u, 2u, 1u};
    int16_t enc[MELLO_UI_ENCODER_COUNT];
    uint32_t ms = 100u;
    uint8_t i;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 123u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_AUDIO_FX);
    u.fx_subpage = 1u;
    for (i = 1u; i < 4u; ++i) {
        memset(enc, 0, sizeof enc);
        enc[MELLO_UI_ENC_SELECT] = 1;
        mello_ui_input(&u, ms * 1000u, ms, 0u, 0u, 0u, 0u, enc), ms += 50u;
        CHECK(u.fx_subpage == fwd[i], "SELECT on: DELAY's layer %u, want %u", u.fx_subpage, fwd[i]);
    }
    for (i = 1u; i < 4u; ++i) {
        memset(enc, 0, sizeof enc);
        enc[MELLO_UI_ENC_SELECT] = -1;
        mello_ui_input(&u, ms * 1000u, ms, 0u, 0u, 0u, 0u, enc), ms += 50u;
        CHECK(u.fx_subpage == back[i], "SELECT back: DELAY's layer %u, want %u", u.fx_subpage, back[i]);
    }
    u.fx_subpage = 3u;
    for (i = 0u; i < 3u;
         ++i) { /* FREEZE a detent at a time (half a second apart: no speed-up); past REV, REV */
        memset(enc, 0, sizeof enc);
        enc[MELLO_UI_ENC_KNOB1] = i ? 0 : 3, enc[MELLO_UI_ENC_KNOB2] = i ? 0 : 2,
        enc[MELLO_UI_ENC_KNOB3] = i ? 0 : 5, enc[MELLO_UI_ENC_KNOB4] = 1;
        mello_ui_input(&u, ms * 1000u, ms, 0u, 0u, 0u, 0u, enc), ms += 500u;
    }
    {
        const MelloFxConfig *f = &t.engine.fx;
        CHECK(f->delay_glide == 30u && f->delay_pitch == 1 && f->delay_diffuse == 50u &&
                  f->delay_freeze == 2u,
              "the knobs turned: GLIDE %u, PITCH %d, DIFFUSE %u, FREEZE %u (want 30, +1, 50, REV)",
              f->delay_glide, f->delay_pitch, f->delay_diffuse, f->delay_freeze);
    }
}

int main(void)
{
    storage();
    ui();
    if (fails) return 1;
    puts("PASS: delay_loop_ui");
    return 0;
}
