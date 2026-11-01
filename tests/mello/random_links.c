/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Phase 5: the randomizer's links (DECISIONS.md "Randomizer links"). A rolled target linked + follows each step's pitch
 * (pulses and ratchets 1-4 and chance 30-100% set by it, flags drawn with it), linked - mirrors it; RANDOM's LINKS
 * layer cycles a target free -> + -> - -> free and rolls it; the links survive a power cycle (SEC_MOD version 2). */
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

static uint32_t prob_pct(uint16_t p)
{
    return ((uint32_t)p * 100u + 32767u) / 65535u;
}

int main(void)
{
    static MelloEngine e;
    static MelloStorage s;
    MelloStorageLoadResult r;
    MelloProject p;
    MelloModRouting m;
    unsigned roll, i, acc_hi = 0u, acc_lo = 0u, n_hi = 0u, n_lo = 0u;
    uint8_t pmax;

    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 4242u);
    pmax = (uint8_t)(7u * e.project.pitch_span_oct - 1u); /* minor: 7 a span octave */
    e.randomize.mask = MELLO_RAND_PITCH | MELLO_RAND_PULSES | MELLO_RAND_PROB | MELLO_RAND_ACCENT;
    m = e.modr;
    m.rnd_follow = MELLO_RAND_PULSES | MELLO_RAND_ACCENT;
    m.rnd_mirror = MELLO_RAND_PROB;
    mello_engine_mod_routing_set(&e, &m);
    for (roll = 0u; roll < 20u; ++roll) {
        mello_engine_randomize_steps_intensity(&e, roll & 1u ? MELLO_RANDOMIZE_HIGH : MELLO_RANDOMIZE_MED);
        for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
            const MelloStep *st = &e.project.steps[i];
            uint32_t up = (uint32_t)st->pitch * 1000u / pmax;
            CHECK(st->pulses == 1u + (up * 3u + 500u) / 1000u,
                  "PULSES+ does not follow pitch (step %u: %d -> %u)", i, st->pitch, st->pulses);
            CHECK(prob_pct(st->probability) == 30u + (1000u - up) * 70u / 1000u ||
                      prob_pct(st->probability) + 1u == 30u + (1000u - up) * 70u / 1000u,
                  "PROB- does not mirror pitch (step %u: %d -> %u%%)", i, st->pitch,
                  prob_pct(st->probability));
            if (up >= 600u) n_hi++, acc_hi += st->accent;
            if (up <= 400u) n_lo++, acc_lo += st->accent;
        }
    }
    printf("accent: high notes %u/%u, low notes %u/%u\n", acc_hi, n_hi, acc_lo, n_lo);
    CHECK(n_hi && n_lo && acc_hi * n_lo > 3u * acc_lo * n_hi, "ACCENT+ does not lean to high notes");

    { /* LINKS: a key cycles free -> + -> - -> free, and rolls its target */
        static MelloTransport t;
        static MelloStorage st0;
        static MelloUi u;
        int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
        uint8_t key = 0xFFu, k;
        for (k = 0u; k < 27u; ++k)
            if (mello_ui_white_key_position(k) == 5) key = k; /* ACC */
        mello_transport_init(&t, &p, 1u, 0, 0);
        mello_storage_init(&st0, 0, 0);
        mello_ui_init(&u, &t, &st0);
        mello_ui_set_page(&u, MELLO_UI_RANDOM);
        t.engine.randomize.mask = 0u;
        enc[MELLO_UI_ENC_SELECT] = 1;
        mello_ui_input(&u, 1000u, 1u, 0u, 0u, 0u, 0u, enc);
        CHECK(u.random_links, "SELECT did not open LINKS");
        mello_ui_input(&u, 2000u, 2u, 0u, 0u, 0u, 1u << key, 0);
        CHECK((t.engine.modr.rnd_follow & MELLO_RAND_ACCENT) && (t.engine.randomize.mask & MELLO_RAND_ACCENT),
              "first press is not ACC+ (rolled)");
        mello_ui_input(&u, 3000u, 3u, 0u, 0u, 0u, 1u << key, 0);
        CHECK(!(t.engine.modr.rnd_follow & MELLO_RAND_ACCENT) &&
                  (t.engine.modr.rnd_mirror & MELLO_RAND_ACCENT),
              "second press is not ACC-");
        mello_ui_input(&u, 4000u, 4u, 0u, 0u, 0u, 1u << key, 0);
        CHECK(!((t.engine.modr.rnd_follow | t.engine.modr.rnd_mirror) & MELLO_RAND_ACCENT),
              "third press is not free");
    }

    /* Through a power cycle. */
    memset(flash, 0xFF, sizeof flash);
    boot(&s, &e, &r);
    (void)mello_storage_poll(&s, &e, 0u, true);
    {
        uint32_t t = 100u;
        MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
        m = e.modr;
        m.rnd_follow = MELLO_RAND_SLIDE | MELLO_RAND_RATCHET;
        m.rnd_mirror = MELLO_RAND_SKIP;
        mello_engine_mod_routing_set(&e, &m);
        for (; t <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u)
            w = mello_storage_poll(&s, &e, t, true);
        CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "never saved");
    }
    boot(&s, &e, &r);
    CHECK(r == MELLO_STORAGE_LOAD_OK && e.modr.rnd_follow == (MELLO_RAND_SLIDE | MELLO_RAND_RATCHET) &&
              e.modr.rnd_mirror == MELLO_RAND_SKIP,
          "links lost in a power cycle");
    puts("PASS: randomizer links: + follows pitch, - mirrors it; LINKS cycles a target; saved");
    return 0;
}
