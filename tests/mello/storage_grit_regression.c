/* Phase 5: the grit is saved, in its own section (SEC_GRIT), for the working sound and for each pattern slot; a
 * project without grit is written exactly as before, so older builds read it unchanged. Flash is a RAM model. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

static MelloStorageSaveResult save(MelloStorage *s, MelloEngine *e, uint32_t *t)
{
    MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
    uint32_t end = *t + 30000u;
    for (; *t <= end && w != MELLO_STORAGE_SAVE_WRITTEN; *t += 100u) w = mello_storage_poll(s, e, *t, true);
    return w;
}

int main(void)
{
    static MelloStorage s;
    static MelloEngine e;
    MelloStorageLoadResult r;
    MelloSoundConfig c;
    uint32_t plain_size, grit_size, crc, t = 100u;

    memset(flash, 0xFF, sizeof flash);
    boot(&s, &e, &r);
    (void)mello_storage_poll(&s, &e, 0u, true); /* baseline: the defaults */
    CHECK(mello_storage_measure_payload(&e, &plain_size, &crc), "measure");

    /* Slot 2 gets a gritty sound; then the working sound gets another. */
    c = e.sound;
    c.noise_level = 310u;
    c.unison = 3u;
    c.detune = 420u;
    mello_engine_sound_set_config(&e, &c);
    CHECK(mello_engine_save_slot(&e, 2u, "GRIT"), "save slot");
    c.noise_level = 0u;
    c.noise_fm = 650u;
    c.unison = 2u;
    c.detune = 900u;
    mello_engine_sound_set_config(&e, &c);
    CHECK(mello_storage_measure_payload(&e, &grit_size, &crc), "measure");
    CHECK(save(&s, &e, &t) == MELLO_STORAGE_SAVE_WRITTEN, "grit edit never saved");

    boot(&s, &e, &r);
    CHECK(r == MELLO_STORAGE_LOAD_OK, "reload %d", (int)r);
    CHECK(e.sound.noise_level == 0u && e.sound.noise_fm == 650u && e.sound.unison == 2u &&
              e.sound.detune == 900u,
          "working grit lost: noise %u fm %u unison %u detune %u", e.sound.noise_level, e.sound.noise_fm,
          e.sound.unison, e.sound.detune);
    CHECK(e.slots[2].valid && e.slots[2].sound.noise_level == 310u && e.slots[2].sound.unison == 3u &&
              e.slots[2].sound.detune == 420u && e.slots[2].sound.noise_fm == 0u,
          "slot grit lost");

    /* Without grit there is no SEC_GRIT section at all: two entries cost a section header, a count and 2 x 8. */
    {
        uint32_t clean, both;
        CHECK(mello_storage_measure_payload(&e, &both, &crc), "measure");
        c = e.sound;
        c.noise_fm = c.detune = 0u;
        c.unison = 1u;
        mello_engine_sound_set_config(&e, &c);
        e.slots[2].sound = e.sound;
        CHECK(mello_storage_measure_payload(&e, &clean, &crc), "measure");
        CHECK(both == clean + 8u + 1u + 2u * 8u, "grit section %u bytes, expected 25", both - clean);
    }
    printf("PASS: grit survives a power cycle, working and slot (payload %u -> %u bytes)\n", plain_size,
           grit_size);
    return 0;
}
