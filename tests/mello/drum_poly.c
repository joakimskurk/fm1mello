/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* A length per voice (DECISIONS "A length per voice"): KICK 16, SNARE 16, HAT 12, GLITCH 7, each voice plays its own
 * steps in its own length; FREE they run on and meet again at the lengths' least common multiple (336), BAR they
 * all start over at every bar's one (and FREE at the project's RESET, as every part); V.LEN 0 is the kit's LENGTH, so an old project plays as before. RESET and a
 * stop start them over. Saved (SEC_DRUMS v3), and an old section reads as the kit's LENGTH, FREE. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mello_storage.h"

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

static MelloEngine e;

/* the steps each voice plays over n pulses, every step a hit and no chance */
static void play(uint32_t n, int16_t steps[][4])
{
    uint32_t p;
    mello_engine_stop(&e);
    mello_engine_start(&e);
    for (p = 0u; p < n; ++p) {
        MelloEvent ev;
        uint8_t v;
        for (v = 0u; v < 4u; ++v) steps[p][v] = -1;
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev))
            if (ev.type == MELLO_EVENT_DRUM_HIT) steps[p][ev.sample_slot % 4u] = (int16_t)ev.step_index;
    }
}

int main(void)
{
    static int16_t at[400][4];
    static const uint8_t lens[4] = {16u, 16u, 12u, 7u};
    MelloProject p;
    MelloDrumConfig d;
    uint32_t k;
    uint8_t v;

    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 3u);
    d = e.drums;
    d.on = true, d.follow = false, d.length = 16u, d.swing = 0u;
    for (v = 0u; v < 4u; ++v) {
        d.hits[v] = 0xFFFFu, d.voice[v].chance = 1000u;
        for (k = 0u; k < 16u; ++k) d.lock[v][k].ratchet = 1u;
    }
    mello_engine_drums_set_config(&e, &d);
    play(40u, at);
    for (k = 0u; k < 40u; ++k)
        for (v = 0u; v < 4u; ++v) CHECK(at[k][v] == (int16_t)(k % 16u), "the kit's LENGTH: voice %u at %u", v, k);

    /* FREE: each its own (the project's RESET off too: a RESET starts every part over) */
    e.project.master_len = 0u;
    memcpy(d.vlen, lens, sizeof d.vlen);
    mello_engine_drums_set_config(&e, &d);
    CHECK(e.drum_wrap == 336u, "they meet again after %u steps, not 336", e.drum_wrap);
    play(340u, at);
    for (k = 0u; k < 340u; ++k)
        for (v = 0u; v < 4u; ++v)
            CHECK(at[k][v] == (int16_t)(k % lens[v]), "FREE: voice %u at pulse %u played step %d", v, k, at[k][v]);

    /* BAR: all of them at 0 on every bar's one */
    d.bar_reset = true;
    mello_engine_drums_set_config(&e, &d);
    play(64u, at);
    for (k = 0u; k < 64u; ++k)
        for (v = 0u; v < 4u; ++v)
            CHECK(at[k][v] == (int16_t)((k % 16u) % lens[v]), "BAR: voice %u at pulse %u played step %d", v, k,
                  at[k][v]);

    /* the longest the count can be */
    {
        static const uint8_t worst[4] = {16u, 15u, 13u, 11u};
        memcpy(d.vlen, worst, sizeof d.vlen);
        mello_engine_drums_set_config(&e, &d);
        CHECK(e.drum_wrap == 34320u, "16 15 13 11 meet after %u", e.drum_wrap);
    }

    /* saved and read back */
    {
        static MelloStorage s;
        MelloStorageLoadResult r;
        uint32_t t = 100u;
        MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
        memset(flash, 0xFF, sizeof flash);
        memcpy(d.vlen, lens, sizeof d.vlen);
        d.bar_reset = true;
        mello_engine_drums_set_config(&e, &d);
        {
            MelloFxConfig f = e.fx; /* (and FX's TRIM, SEC_FX v6, with them) */
            f.trim_db = -3;
            mello_engine_fx_set_config(&e, &f);
        }
        mello_storage_init(&s, &io, NULL);
        mello_storage_request_save(&s, t);
        for (; t <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u) w = mello_storage_poll(&s, &e, t, true);
        CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "not saved");
        mello_engine_init(&e, &p, 4u);
        CHECK(e.drums.vlen[2] == 0u && !e.drums.bar_reset, "a new engine is not the kit's LENGTH, FREE");
        mello_storage_init(&s, &io, NULL);
        r = mello_storage_load(&s, &e);
        (void)r;
        CHECK(e.fx.trim_db == -3, "TRIM read back as %d", e.fx.trim_db);
        CHECK(!memcmp(e.drums.vlen, lens, sizeof lens) && e.drums.bar_reset && e.drum_wrap == 336u,
              "read back: %u %u %u %u, BAR %d", e.drums.vlen[0], e.drums.vlen[1], e.drums.vlen[2], e.drums.vlen[3],
              (int)e.drums.bar_reset);
    }

    if (fails) return 1;
    puts("PASS: drum_poly");
    return 0;
}
