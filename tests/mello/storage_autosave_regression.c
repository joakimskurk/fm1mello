/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* P1 regression (review 2026-10): step, sound and FX edits were never marked dirty, so they were lost at
 * power-off. mello_storage_poll now notices any change to what a save would write. Flash is a RAM model. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mello_storage.h"

#define FLASH_SIZE (MELLO_STORAGE_BANK0_ADDR + 2u * MELLO_STORAGE_BANK_SIZE)
static uint8_t flash[FLASH_SIZE];
static unsigned erases;

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
    erases++;
    return true;
}
static bool ram_program(void *ctx, uint32_t address, const void *src, uint32_t length)
{
    const uint8_t *p = src;
    uint32_t i;
    (void)ctx;
    if (address + length > FLASH_SIZE) return false;
    for (i = 0u; i < length; ++i) flash[address + i] &= p[i]; /* NOR: program only clears bits */
    return true;
}
static const MelloStorageIo io = {ram_read, ram_erase4k, ram_program, NULL};

static void boot(MelloStorage *s, MelloEngine *e, MelloStorageLoadResult *r)
{
    MelloProject p;
    mello_project_defaults(&p);
    mello_engine_init(e, &p, 1u);
    mello_storage_init(s, &io, NULL);
    *r = mello_storage_load(s, e);
}

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            printf("FAIL: " __VA_ARGS__);                                                                    \
            printf("\n");                                                                                    \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

int main(void)
{
    static MelloStorage s;
    static MelloEngine e;
    MelloStorageLoadResult r;
    MelloStorageSaveResult w;
    uint32_t t;
    int8_t pitch;

    memset(flash, 0xFF, sizeof flash);
    boot(&s, &e, &r);
    CHECK(r == MELLO_STORAGE_LOAD_NONE, "fresh flash load %d", (int)r);

    /* Untouched defaults are never written: a fresh unit does not wear flash on its own. */
    for (t = 0u; t <= 60000u; t += 100u) (void)mello_storage_poll(&s, &e, t, true);
    CHECK(erases == 0u, "defaults were written without an edit (%u erases)", erases);

    /* A step edit (no mark_dirty call): saved after the debounce, while stopped and silent. */
    pitch = (int8_t)(e.project.steps[3].pitch + 2);
    e.project.steps[3].pitch = pitch;
    w = MELLO_STORAGE_SAVE_IDLE;
    for (t = 60100u; t <= 64000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u)
        w = mello_storage_poll(&s, &e, t, true);
    CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "step edit never saved (last result %d)", (int)w);

    /* Sound and FX edits while playing: deferred, then written once safe. */
    e.sound.filter_cut = 333u;
    e.fx.reverb_wet = 777u;
    for (t = 64100u; t <= 120000u; t += 100u) {
        w = mello_storage_poll(&s, &e, t, false);
        CHECK(w != MELLO_STORAGE_SAVE_WRITTEN, "wrote while not safe");
    }
    w = MELLO_STORAGE_SAVE_IDLE;
    for (t = 120100u; t <= 125000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u)
        w = mello_storage_poll(&s, &e, t, true);
    CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "sound/FX edit never saved (last result %d)", (int)w);

    /* Power cycle: everything is back. */
    boot(&s, &e, &r);
    CHECK(r == MELLO_STORAGE_LOAD_OK, "reload %d", (int)r);
    CHECK(e.project.steps[3].pitch == pitch, "step pitch %d, want %d", e.project.steps[3].pitch, pitch);
    CHECK(e.sound.filter_cut == 333u, "filter_cut %u", (unsigned)e.sound.filter_cut);
    CHECK(e.fx.reverb_wet == 777u, "reverb_wet %u", (unsigned)e.fx.reverb_wet);

    /* After a load nothing is dirty and nothing is rewritten. */
    erases = 0u;
    for (t = 0u; t <= 60000u; t += 100u) (void)mello_storage_poll(&s, &e, t, true);
    CHECK(erases == 0u, "a loaded, unchanged project was rewritten");

    printf("PASS: step, sound and FX edits are autosaved and survive a power cycle\n");
    return 0;
}
