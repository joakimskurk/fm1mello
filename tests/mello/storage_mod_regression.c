/* Phase 5: DOT's MOD 2, the MOD lanes' routing, DOT's evolve and EVOLVE's sound routes are saved in their own section
 * (SEC_MOD), for the working pattern and each slot. A project that leaves them alone writes no section, and MOD 1's
 * amount keeps the HTML's meaning in SEC_WORKING / SEC_SLOTS (1000 = +24% cutoff = depth 240). Flash is a RAM model. */
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
    MelloModRouting m;
    MelloDotMatrixConfig d;
    uint32_t plain, crc, t = 100u;

    memset(flash, 0xFF, sizeof flash);
    boot(&s, &e, &r);
    (void)mello_storage_poll(&s, &e, 0u, true);
    CHECK(mello_storage_measure_payload(&e, &plain, &crc), "measure");

    /* Slot 5: MOD 2 lit, its lane on PITCH as GATE. Working: other routes, EVOLVE on every step, MOD 1 at 777. */
    m = e.modr;
    m.dot_target[1] = MELLO_DEST_PITCH;
    m.dot_gate[1] = true;
    mello_engine_mod_routing_set(&e, &m);
    d = e.dot[MELLO_DOT_MOD2];
    d.cells = 0x8421u;
    d.length = 12u;
    mello_engine_dot_set_config(&e, MELLO_DOT_MOD2, &d);
    CHECK(mello_engine_save_slot(&e, 5u, "MODS"), "save slot");
    m.dot_target[0] = MELLO_DEST_FM;
    m.dot_attack_ms[0] = 333u;
    m.dot_release_ms[0] = 1999u;
    m.dot_evolve[MELLO_DOT_SUB] = 420u;
    m.tm_dest[0] = MELLO_DEST_SAT;
    m.tm_depth[0] = -650;
    m.tm_dest[1] = MELLO_DEST_OSC2;
    m.tm_on_loop = false;
    mello_engine_mod_routing_set(&e, &m);
    d = e.dot[MELLO_DOT_MOD1];
    d.amount = 777u;
    mello_engine_dot_set_config(&e, MELLO_DOT_MOD1, &d);
    CHECK(save(&s, &e, &t) == MELLO_STORAGE_SAVE_WRITTEN, "never saved");

    boot(&s, &e, &r);
    CHECK(r == MELLO_STORAGE_LOAD_OK, "reload %d", (int)r);
    CHECK(e.modr.dot_target[0] == MELLO_DEST_FM && e.modr.dot_attack_ms[0] == 333u &&
              e.modr.dot_release_ms[0] == 1999u && e.modr.dot_evolve[MELLO_DOT_SUB] == 420u &&
              e.modr.tm_dest[0] == MELLO_DEST_SAT && e.modr.tm_depth[0] == -650 &&
              e.modr.tm_dest[1] == MELLO_DEST_OSC2 && !e.modr.tm_on_loop &&
              e.modr.dot_target[1] == MELLO_DEST_PITCH && e.modr.dot_gate[1],
          "working routing lost");
    CHECK(e.dot[MELLO_DOT_MOD1].amount == 777u, "MOD 1's depth lost (%u)", e.dot[MELLO_DOT_MOD1].amount);
    CHECK(e.dot[MELLO_DOT_MOD2].cells == 0x8421u && e.dot[MELLO_DOT_MOD2].length == 12u, "MOD 2 lost");
    CHECK(e.slots[5].valid && e.slots[5].modr.dot_target[1] == MELLO_DEST_PITCH &&
              e.slots[5].modr.dot_gate[1] && e.slots[5].modr.tm_on_loop &&
              e.slots[5].dot[MELLO_DOT_MOD2].cells == 0x8421u,
          "slot routing lost");

    /* Back to the defaults: no section, so the payload is what it was (with the slot, in SEC_SLOTS). */
    {
        uint32_t clean, with_slot;
        mello_mod_routing_defaults(&m);
        mello_engine_mod_routing_set(&e, &m);
        mello_dot_defaults_for(&d, MELLO_DOT_MOD1);
        mello_engine_dot_set_config(&e, MELLO_DOT_MOD1, &d);
        mello_dot_defaults_for(&d, MELLO_DOT_MOD2);
        mello_engine_dot_set_config(&e, MELLO_DOT_MOD2, &d);
        e.slots[5].modr = e.modr;
        e.slots[5].dot[MELLO_DOT_MOD2] = d;
        CHECK(mello_storage_measure_payload(&e, &clean, &crc), "measure");
        e.slots[5].valid = false;
        CHECK(mello_storage_measure_payload(&e, &with_slot, &crc), "measure");
        CHECK(with_slot == plain, "defaults wrote SEC_MOD (%u vs %u bytes)", with_slot, plain);
        CHECK(clean > plain, "the slot vanished");
    }
    puts(
        "PASS: DOT's MOD lanes, evolve and EVOLVE's routes survive a power cycle; defaults write no section");
    return 0;
}
