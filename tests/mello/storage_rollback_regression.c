/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* P1 regression (review 2026-10, P1-2): after a rollback to an older firmware, its autosave used to erase
 * the project a newer firmware had written (format version > ours), and then the user's older project too.
 * A bank holding a newer format is now never erased. Flash is a RAM model. */
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

/* Turn the record in a bank into one written by a newer firmware: bump the header's format version and
 * fix its CRC (header: magic, u16 version, u16 header size, size, revision, payload CRC, header CRC). */
static void make_newer(uint8_t bank)
{
    uint8_t *h = flash + (bank ? MELLO_STORAGE_BANK1_ADDR : MELLO_STORAGE_BANK0_ADDR);
    uint32_t crc;
    h[4] = (uint8_t)(MELLO_STORAGE_FORMAT_VERSION + 1u);
    h[5] = 0u;
    crc = mello_storage_crc32(h, 20u);
    h[20] = (uint8_t)crc;
    h[21] = (uint8_t)(crc >> 8);
    h[22] = (uint8_t)(crc >> 16);
    h[23] = (uint8_t)(crc >> 24);
}

/* Two saves: bank 0 holds revision 1, bank 1 revision 2. */
static void two_saves(MelloStorage *s, MelloEngine *e)
{
    MelloStorageLoadResult r;
    memset(flash, 0xFF, sizeof flash);
    boot(s, e, &r);
    e->project.steps[0].pitch = 1;
    (void)mello_storage_save_now(s, e, 0u);
    e->project.steps[0].pitch = 2;
    (void)mello_storage_save_now(s, e, 0u);
}

/* Edit and poll for a minute while stopped and silent; returns the erase count. */
static unsigned edit_and_wait(MelloStorage *s, MelloEngine *e, MelloStorageSaveResult *last)
{
    uint32_t t;
    erases = 0u;
    e->sound.filter_cut = (uint16_t)(e->sound.filter_cut == 111u ? 222u : 111u);
    mello_storage_request_save(s, 0u); /* an explicit SAVE too */
    for (t = 0u; t <= 60000u; t += 100u) {
        MelloStorageSaveResult w = mello_storage_poll(s, e, t, true);
        if (w != MELLO_STORAGE_SAVE_IDLE && w != MELLO_STORAGE_SAVE_DEFERRED) *last = w;
    }
    return erases;
}

int main(void)
{
    static MelloStorage s;
    static MelloEngine e;
    static uint8_t before[2u * MELLO_STORAGE_BANK_SIZE];
    MelloStorageLoadResult r;
    MelloStorageSaveResult last = MELLO_STORAGE_SAVE_IDLE;

    /* Both banks newer: load is UNSUPPORTED, nothing may be erased. */
    two_saves(&s, &e);
    make_newer(0u);
    make_newer(1u);
    memcpy(before, flash + MELLO_STORAGE_BANK0_ADDR, sizeof before);
    boot(&s, &e, &r);
    CHECK(r == MELLO_STORAGE_LOAD_UNSUPPORTED, "load %d, want UNSUPPORTED", (int)r);
    CHECK(edit_and_wait(&s, &e, &last) == 0u, "erased a newer-format bank (both newer)");
    CHECK(last == MELLO_STORAGE_SAVE_PROTECTED, "save result %d, want PROTECTED", (int)last);
    CHECK(memcmp(before, flash + MELLO_STORAGE_BANK0_ADDR, sizeof before) == 0, "flash changed (both newer)");

    /* The review's case: the newer firmware wrote the newest record (bank 1); bank 0 still holds the user's
     * previous project in our format. Load is UNSUPPORTED (runs on defaults); neither bank may be erased. */
    two_saves(&s, &e);
    make_newer(1u);
    memcpy(before, flash + MELLO_STORAGE_BANK0_ADDR, sizeof before);
    boot(&s, &e, &r);
    CHECK(r == MELLO_STORAGE_LOAD_UNSUPPORTED, "load %d, want UNSUPPORTED", (int)r);
    last = MELLO_STORAGE_SAVE_IDLE;
    CHECK(edit_and_wait(&s, &e, &last) == 0u, "erased a bank (newest record is newer format)");
    CHECK(last == MELLO_STORAGE_SAVE_PROTECTED, "save result %d, want PROTECTED", (int)last);
    CHECK(memcmp(before, flash + MELLO_STORAGE_BANK0_ADDR, sizeof before) == 0,
          "flash changed (bank 1 newer)");

    /* A newer-format record that is older than ours: ours loads, but the newer one is still kept. */
    two_saves(&s, &e);
    make_newer(0u);
    memcpy(before, flash + MELLO_STORAGE_BANK0_ADDR, sizeof before);
    boot(&s, &e, &r);
    CHECK(r == MELLO_STORAGE_LOAD_OK, "load %d, want OK from bank 1", (int)r);
    CHECK(e.project.steps[0].pitch == 2, "loaded pitch %d, want bank 1's 2", e.project.steps[0].pitch);
    last = MELLO_STORAGE_SAVE_IDLE;
    CHECK(edit_and_wait(&s, &e, &last) == 0u, "erased the newer-format bank 0");
    CHECK(last == MELLO_STORAGE_SAVE_PROTECTED, "save result %d, want PROTECTED", (int)last);
    CHECK(memcmp(before, flash + MELLO_STORAGE_BANK0_ADDR, sizeof before) == 0,
          "flash changed (bank 0 newer)");

    /* An older format in a bank is still overwritten (an upgrade must be able to save). */
    two_saves(&s, &e);
    flash[MELLO_STORAGE_BANK0_ADDR + 4u] = (uint8_t)(MELLO_STORAGE_FORMAT_VERSION - 1u);
    {
        uint8_t *h = flash + MELLO_STORAGE_BANK0_ADDR;
        uint32_t crc = mello_storage_crc32(h, 20u);
        h[20] = (uint8_t)crc;
        h[21] = (uint8_t)(crc >> 8);
        h[22] = (uint8_t)(crc >> 16);
        h[23] = (uint8_t)(crc >> 24);
    }
    flash[MELLO_STORAGE_BANK1_ADDR + 4u] = (uint8_t)(MELLO_STORAGE_FORMAT_VERSION - 1u);
    {
        uint8_t *h = flash + MELLO_STORAGE_BANK1_ADDR;
        uint32_t crc = mello_storage_crc32(h, 20u);
        h[20] = (uint8_t)crc;
        h[21] = (uint8_t)(crc >> 8);
        h[22] = (uint8_t)(crc >> 16);
        h[23] = (uint8_t)(crc >> 24);
    }
    boot(&s, &e, &r);
    last = MELLO_STORAGE_SAVE_IDLE;
    CHECK(edit_and_wait(&s, &e, &last) > 0u && last == MELLO_STORAGE_SAVE_WRITTEN,
          "could not save over an older format (result %d)", (int)last);

    printf("PASS: a newer firmware's project survives a rollback; older formats are still replaced\n");
    return 0;
}
