/* Feedback 2026-10-30: "the randomizer gets an EDIT: how much each parameter is affected, not only on / off". Each
 * target has an amount, the chance a roll changes a step (WAVE / CHORD: the sound): 0% never, 100% every step (as
 * before, drawing nothing more, so default rolls are unchanged). RANDOM's EDIT: a key picks a target, K1 sets its
 * amount. The amounts survive a power cycle (SEC_MOD version 3). */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mello_storage.h"
#include "mello_ui.h"

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

/* How many steps' pitch changed in each of n rolls at the given pitch amount, in total. */
static unsigned changed(MelloEngine *e, uint8_t amount, unsigned rolls)
{
    MelloModRouting m = e->modr;
    unsigned r, i, n = 0u;
    m.rnd_amount[0] = amount; /* PITCH is bit 0 */
    mello_engine_mod_routing_set(e, &m);
    e->randomize.mask = MELLO_RAND_PITCH;
    for (r = 0u; r < rolls; ++r) {
        MelloProject before = e->project;
        mello_engine_randomize_steps_intensity(e,
                                               MELLO_RANDOMIZE_HIGH); /* HIGH always moves a rolled pitch */
        for (i = 0u; i < MELLO_MAX_STEPS; ++i) n += e->project.steps[i].pitch != before.steps[i].pitch;
    }
    return n;
}

int main(void)
{
    static MelloEngine e, ref;
    static MelloStorage s;
    MelloStorageLoadResult r;
    MelloProject p;
    unsigned none, half, all;
    mello_project_defaults(&p);

    mello_engine_init(&e, &p, 11u); /* at the defaults a roll is what it was: same seed, same result */
    mello_engine_init(&ref, &p, 11u);
    memset(ref.modr.rnd_amount, 100, sizeof ref.modr.rnd_amount);
    CHECK(e.modr.rnd_amount[0] == 100u && e.modr.rnd_amount[12] == 100u, "amounts are not 100%% by default");
    mello_engine_randomize_steps_intensity(&e, MELLO_RANDOMIZE_MED);
    mello_engine_randomize_steps_intensity(&ref, MELLO_RANDOMIZE_MED);
    CHECK(!memcmp(e.project.steps, ref.project.steps, sizeof e.project.steps), "a default roll changed");

    none = changed(&e, 0u, 10u);
    half = changed(&e, 50u, 10u);
    all = changed(&e, 100u, 10u);
    printf("pitches changed over 10 rolls of 32 steps: 0%%: %u, 50%%: %u, 100%%: %u\n", none, half, all);
    CHECK(none == 0u, "amount 0 still rolled pitches");
    CHECK(all >= 300u, "amount 100 left pitches");
    CHECK(half > 110u && half < 210u, "amount 50 is not about half (%u of 320)", half);

    { /* RANDOM's EDIT: EDIT, a key picks SUB, K1 sets it */
        static MelloTransport t;
        static MelloStorage st0;
        static MelloUi u;
        int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
        uint8_t key = 0xFFu, k;
        for (k = 0u; k < 27u; ++k)
            if (mello_ui_white_key_position(k) == 6) key = k; /* SUB, the 7th pill */
        mello_transport_init(&t, &p, 1u, 0, 0);
        mello_storage_init(&st0, 0, 0);
        mello_ui_init(&u, &t, &st0);
        mello_ui_set_page(&u, MELLO_UI_RANDOM);
        mello_ui_input(&u, 1000u, 1u, 1u << MELLO_UI_BTN_EDIT, 0u, 0u, 0u, enc);
        CHECK(u.random_edit, "EDIT did not open the amounts");
        mello_ui_input(&u, 2000u, 2u, 0u, 0u, 0u, 1u << key, enc);
        CHECK(u.random_sel == 6u && (t.engine.randomize.mask & MELLO_RAND_SUB),
              "a key in EDIT toggled instead of picking");
        enc[MELLO_UI_ENC_KNOB1] = -1; /* eight detents, slowly (no acceleration): 100 - 8 x 5 */
        for (k = 0u; k < 8u; ++k)
            mello_ui_input(&u, (3000u + k * 200u) * 1000u, 3000u + k * 200u, 0u, 0u, 0u, 0u, enc);
        CHECK(t.engine.modr.rnd_amount[6] == 60u, "K1 did not set SUB's amount (%u)",
              t.engine.modr.rnd_amount[6]);
    }

    memset(flash, 0xFF, sizeof flash); /* through a power cycle */
    boot(&s, &e, &r);
    (void)mello_storage_poll(&s, &e, 0u, true);
    {
        uint32_t tm = 100u;
        MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
        MelloModRouting m = e.modr;
        m.rnd_amount[0] = 35u, m.rnd_amount[12] = 0u;
        mello_engine_mod_routing_set(&e, &m);
        for (; tm <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; tm += 100u)
            w = mello_storage_poll(&s, &e, tm, true);
        CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "never saved");
    }
    boot(&s, &e, &r);
    CHECK(r == MELLO_STORAGE_LOAD_OK && e.modr.rnd_amount[0] == 35u && e.modr.rnd_amount[12] == 0u &&
              e.modr.rnd_amount[5] == 100u,
          "amounts lost in a power cycle");
    puts("PASS: randomizer amounts: 0 never, 50 about half, 100 as before; EDIT picks and sets; saved");
    return 0;
}
