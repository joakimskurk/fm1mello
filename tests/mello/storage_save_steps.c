/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* A save erases a sector a poll (ROADMAP F22): every sector at once held the main loop 0.5-4 s on the chip. Checked:
 * no poll erases more than one sector; the record is whole once the bank is erased and loads back; a save stopped
 * half-way (PLAY: not safe to write) goes on where it was and erases nothing twice; a manual SAVE goes the same way;
 * a bank holding a newer firmware's project is still never erased, not even its first sector. Flash is a RAM
 * model. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mello_storage.h"

#define FLASH_SIZE (MELLO_STORAGE_BANK0_ADDR + 2u * MELLO_STORAGE_BANK_SIZE)
static uint8_t flash[FLASH_SIZE];
static unsigned erases, erased_at[FLASH_SIZE / 4096u];

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
    erased_at[address / 4096u]++;
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

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            printf("FAIL: " __VA_ARGS__);                                                                    \
            printf("\n");                                                                                    \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

static MelloStorage s;
static MelloEngine e;
static uint32_t now = 0u;

/* Every pattern slot full and different (as storage_full_slots): the record a full project writes, three sectors
 * with the 16 slots (ten with the 64 before 2026-11-05). */
static void fill_slots(void)
{
    unsigned i, k;
    for (i = 0u; i < MELLO_MAX_SLOTS; ++i) {
        for (k = 0u; k < MELLO_MAX_STEPS; ++k) {
            e.project.steps[k].pitch = (int8_t)((i + k) % 12u);
            e.project.steps[k].ratchet = (uint8_t)(1u + (i + k) % 4u);
        }
        e.sound.noise_level = (uint16_t)(100u + i);
        e.modr.tm_depth[0] = (int16_t)(i * 7u);
        (void)mello_engine_save_slot(&e, (uint8_t)i, "SLOT");
    }
}

/* Polls until the save is written (or `limit` polls), safe to write; the most sectors one poll erased. */
static unsigned run(unsigned limit, MelloStorageSaveResult *last)
{
    unsigned most = 0u, n;
    for (n = 0u; n < limit; ++n) {
        unsigned before = erases;
        now += 10u;
        *last = mello_storage_poll(&s, &e, now, true);
        if (erases - before > most) most = erases - before;
        if (*last == MELLO_STORAGE_SAVE_WRITTEN) break;
    }
    return most;
}

int main(void)
{
    MelloProject p;
    MelloStorageLoadResult r;
    MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
    unsigned most, i, twice;

    memset(flash, 0xFF, sizeof flash);
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 1u);
    mello_storage_init(&s, &io, NULL);
    r = mello_storage_load(&s, &e);
    CHECK(r == MELLO_STORAGE_LOAD_NONE, "fresh flash load %d", (int)r);
    fill_slots();

    /* a manual SAVE: a sector a poll, then the record */
    mello_storage_request_save(&s, now);
    erases = 0u;
    most = run(200u, &w);
    CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "the manual save was not written (%d)", (int)w);
    CHECK(most == 1u, "a poll erased %u sectors", most);
    CHECK(erases >= 3u, "only %u sectors erased: a full project's record is bigger than that", erases);
    printf("storage_save_steps: a save erases %u sectors, one a poll\n", erases);

    /* an edit, autosaved; PLAY half-way: nothing erased while it plays, the rest after, nothing twice */
    e.project.bpm = 97u;
    memset(erased_at, 0, sizeof erased_at);
    erases = 0u;
    for (i = 0u; i < 4000u && erases < 2u; ++i) now += 10u, w = mello_storage_poll(&s, &e, now, true);
    CHECK(erases == 2u, "the autosave did not start (%u erases)", erases);
    for (i = 0u; i < 50u; ++i) {
        now += 10u;
        w = mello_storage_poll(&s, &e, now, false); /* playing */
    }
    CHECK(erases == 2u && w == MELLO_STORAGE_SAVE_DEFERRED, "erased while playing (%u)", erases);
    most = run(400u, &w);
    CHECK(w == MELLO_STORAGE_SAVE_WRITTEN && most == 1u, "the stopped save did not finish a sector a poll");
    for (i = 0u, twice = 0u; i < FLASH_SIZE / 4096u; ++i) twice += erased_at[i] > 1u;
    CHECK(twice == 0u, "%u sectors erased twice", twice);

    /* it loads back */
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 1u);
    mello_storage_init(&s, &io, NULL);
    r = mello_storage_load(&s, &e);
    CHECK(r == MELLO_STORAGE_LOAD_OK && e.project.bpm == 97u, "reload %d, bpm %u", (int)r, e.project.bpm);

    /* a newer firmware's project in the other bank: no sector erased, not even the first */
    {
        uint8_t other = s.active_bank == 0 ? 1u : 0u, *h;
        uint32_t crc;
        /* put a record there by saving once more, then mark it newer (as storage_rollback_regression does) */
        e.project.bpm = 98u;
        run(4000u, &w);
        CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "the third save (%d)", (int)w);
        h = flash + (other ? MELLO_STORAGE_BANK1_ADDR : MELLO_STORAGE_BANK0_ADDR);
        h[4] = (uint8_t)(MELLO_STORAGE_FORMAT_VERSION + 1u);
        h[5] = 0u;
        crc = mello_storage_crc32(h, 20u);
        h[20] = (uint8_t)crc, h[21] = (uint8_t)(crc >> 8), h[22] = (uint8_t)(crc >> 16),
        h[23] = (uint8_t)(crc >> 24);
        e.project.bpm = 99u;
        erases = 0u;
        run(4000u, &w);
        CHECK(erases == 0u, "%u sectors erased with a newer project in flash", erases);
    }

    puts("PASS: storage_save_steps");
    return 0;
}
