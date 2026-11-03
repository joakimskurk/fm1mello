/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* ROADMAP A1: 64 full slots, their grit and MOD entries with them, must fit a bank and come back after a power
 * cycle; a record in the 40 KiB layout's second bank (0.5-f21 and before) is read at boot and the first save after
 * it goes under it in bank 0, leaving it whole; a project that does not fit says so (SAVE_TOO_LARGE: the status
 * line's FULL). Flash is a RAM model up to the sample staging area, which the banks must not reach. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mello_sample.h"
#include "mello_storage.h"

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

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            printf("FAIL: " __VA_ARGS__);                                                                    \
            printf("\n");                                                                                    \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

static void fresh(MelloEngine *e)
{
    MelloProject p;
    mello_project_defaults(&p);
    mello_engine_init(e, &p, 1u);
}

/* Every slot full, and different: steps, grit and MOD (so every slot writes its own entries). */
static void fill_slots(MelloEngine *e)
{
    unsigned i, k;
    for (i = 0u; i < MELLO_MAX_SLOTS; ++i) {
        for (k = 0u; k < MELLO_MAX_STEPS; ++k) {
            e->project.steps[k].pitch = (int8_t)((i + k) % 12u);
            e->project.steps[k].ratchet = (uint8_t)(1u + (i + k) % 4u);
            e->project.steps[k].accent = ((i + k) & 1u) != 0u;
        }
        e->project.bpm = (uint16_t)(60u + i);
        e->sound.noise_level = (uint16_t)(100u + i);
        e->sound.lead_level = 900u;
        e->sound.unison = (uint8_t)(1u + i % 4u);
        e->modr.rnd_amount[0] = (uint8_t)(10u + i);
        e->modr.tm_depth[0] = (int16_t)(i * 7u);
        (void)mello_engine_save_slot(e, (uint8_t)i, "SLOT");
    }
}

static int slots_equal(const MelloEngine *a, const MelloEngine *b)
{
    unsigned i;
    for (i = 0u; i < MELLO_MAX_SLOTS; ++i) /* a slot does not keep FX (it is global) */
        if (memcmp(&a->slots[i], &b->slots[i], sizeof a->slots[i])) return 0;
    return 1;
}

int main(void)
{
    static MelloEngine e, e2;
    static MelloStorage s;
    static uint8_t legacy_before[0xA000u]; /* the old layout's second bank */
    uint32_t size, crc;

    CHECK(MELLO_STORAGE_BANK0_ADDR + 2u * MELLO_STORAGE_BANK_SIZE <= MELLO_SAMPLE_FLASH_STAGING_ADDR,
          "the banks reach into the sample staging area");

    /* 1. 64 full slots fit, and come back */
    memset(flash, 0xFF, sizeof flash);
    fresh(&e);
    fill_slots(&e);
    CHECK(mello_storage_measure_payload(&e, &size, &crc), "measure");
    printf("64 full slots, grit and MOD in each: %u B of a %u B bank\n", (unsigned)size,
           (unsigned)MELLO_STORAGE_BANK_SIZE);
    CHECK(size + MELLO_STORAGE_HEADER_SIZE + MELLO_STORAGE_FOOTER_SIZE <= MELLO_STORAGE_BANK_SIZE,
          "64 full slots do not fit the bank");
    mello_storage_init(&s, &io, NULL);
    CHECK(mello_storage_save_now(&s, &e, 1000u) == MELLO_STORAGE_SAVE_WRITTEN,
          "save of 64 full slots refused");
    fresh(&e2);
    mello_storage_init(&s, &io, NULL);
    CHECK(mello_storage_load(&s, &e2) == MELLO_STORAGE_LOAD_OK, "load after the power cycle");
    CHECK(slots_equal(&e, &e2), "a slot came back different");

    /* 2. the 40 KiB layout's record in its second bank is read, and the first save leaves it whole */
    memset(flash, 0xFF, sizeof flash);
    {
        MelloStorageConfig old;
        mello_storage_config_defaults(&old);
        old.bank_size = 0xA000u;
        old.bank_addr[1] = MELLO_STORAGE_LEGACY_BANK1_ADDR;
        fresh(&e);
        e.project.bpm = 99u;
        mello_storage_init(&s, &io, &old);
        CHECK(mello_storage_save_now(&s, &e, 1000u) == MELLO_STORAGE_SAVE_WRITTEN, "old layout, first save");
        e.project.bpm = 123u; /* the newer record, in the old second bank */
        CHECK(mello_storage_save_now(&s, &e, 2000u) == MELLO_STORAGE_SAVE_WRITTEN, "old layout, second save");
        CHECK(s.active_bank == 1 && s.revision == 2u, "the old layout's second save went to its bank 1");
    }
    memcpy(legacy_before, flash + MELLO_STORAGE_LEGACY_BANK1_ADDR, sizeof legacy_before);
    fresh(&e2);
    mello_storage_init(&s, &io, NULL);
    CHECK(mello_storage_load(&s, &e2) == MELLO_STORAGE_LOAD_OK, "the legacy record was not read");
    CHECK(e2.project.bpm == 123u, "the legacy record read was not the newest (bpm %u)", e2.project.bpm);
    CHECK(s.revision == 2u && s.active_bank == 0, "the legacy record counts as bank 0");
    e2.project.bpm = 124u;
    CHECK(mello_storage_save_now(&s, &e2, 3000u) == MELLO_STORAGE_SAVE_WRITTEN, "the save after the upgrade");
    CHECK(s.active_bank == 0, "the save after the upgrade did not go under the old record");
    CHECK(memcmp(legacy_before, flash + MELLO_STORAGE_LEGACY_BANK1_ADDR, sizeof legacy_before) == 0,
          "the save after the upgrade touched the old layout's record");
    fresh(&e);
    mello_storage_init(&s, &io, NULL);
    CHECK(mello_storage_load(&s, &e) == MELLO_STORAGE_LOAD_OK && e.project.bpm == 124u,
          "the record saved after the upgrade is not the one read next");
    e.project.bpm = 125u; /* and from there the banks alternate as ever */
    CHECK(mello_storage_save_now(&s, &e, 4000u) == MELLO_STORAGE_SAVE_WRITTEN && s.active_bank == 1,
          "the second save after the upgrade did not go to bank 1");

    /* 3. a bank too small says so */
    {
        MelloStorageConfig small;
        mello_storage_config_defaults(&small);
        small.bank_size = 0x4000u;
        small.bank_addr[1] = MELLO_STORAGE_BANK0_ADDR + 0x4000u;
        memset(flash, 0xFF, sizeof flash);
        fresh(&e);
        fill_slots(&e);
        mello_storage_init(&s, &io, &small);
        CHECK(mello_storage_save_now(&s, &e, 1000u) == MELLO_STORAGE_SAVE_TOO_LARGE,
              "a project past the bank was not refused as too large");
    }
    puts("PASS: 64 full slots fit and return; the old layout's record is read and kept; too large says so");
    return 0;
}
