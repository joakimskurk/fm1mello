/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* SEED (GLOBAL > SYSTEM, DECISIONS "SEED"): the engine's random numbers (RNDMZR and the STYLE rolls, EVOLVE, the drums'
 * chance and rolls) from a seed. The same SEED number gives the same rolls in the same order, every time; another
 * number, others; mello_engine_reseed seeds the three streams as mello_engine_init does; mello_engine_mix_seed moves
 * all three streams (a press's time, the chip's register), and 0 moves nothing. (AUTO itself, the chip's register and
 * the first press's time, is dinghy.c's on the FM-1: not on a computer.) */
#include <stdio.h>
#include <string.h>

#include "mello_transport.h"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

static MelloProject p;
static MelloTransport t;

/* the styles eight RNDMZR rolls of the band land on, and the three streams' next numbers after */
static void rolls(uint8_t out[8], uint32_t after[3])
{
    uint8_t k;
    for (k = 0u; k < 8u; ++k) {
        (void)mello_engine_style_roll(&t.engine, MELLO_STYLE_ANY);
        out[k] = (uint8_t)t.engine.style_now;
    }
    after[0] = mello_prng_next_u32(&t.engine.rng);
    after[1] = mello_prng_next_u32(&t.engine.gen_rng);
    after[2] = mello_prng_next_u32(&t.engine.drum_rng);
}
static void fresh(void)
{
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 0x4D454C4Cu, 0, 0); /* (the FM-1's power-on seed) */
}

int main(void)
{
    uint8_t a[8], b[8], c[8];
    uint32_t xa[3], xb[3], xc[3];
    unsigned k, same;

    /* SEED 7, twice: the same rolls, the same streams after */
    fresh();
    mello_engine_reseed(&t.engine, mello_seed_of(7u));
    rolls(a, xa);
    fresh();
    mello_engine_reseed(&t.engine, mello_seed_of(7u));
    rolls(b, xb);
    CHECK(!memcmp(a, b, sizeof a) && !memcmp(xa, xb, sizeof xa), "SEED 7 twice: not the same rolls");

    /* SEED 8: other rolls (eight styles in a row the same as 7's would be chance itself: not these seeds) */
    fresh();
    mello_engine_reseed(&t.engine, mello_seed_of(8u));
    rolls(c, xc);
    CHECK(memcmp(a, c, sizeof a) && xa[0] != xc[0] && xa[1] != xc[1] && xa[2] != xc[2],
          "SEED 8 rolled as SEED 7");
    printf("SEED 7 rolls styles");
    for (k = 0u; k < 8u; ++k) printf(" %u", a[k]);
    printf("; SEED 8");
    for (k = 0u; k < 8u; ++k) printf(" %u", c[k]);
    printf("\n");

    /* the init's own seeding: the three streams seeded as mello_engine_init seeds them */
    {
        MelloPrng r, g, d;
        fresh();
        mello_engine_reseed(&t.engine, 0x12345678u);
        mello_prng_seed(&r, 0x12345678u);
        mello_prng_seed(&g, 0x12345678u ^ 0x9E3779B9u);
        mello_prng_seed(&d, 0x12345678u ^ 0x44524D53u);
        for (k = 0u, same = 0u; k < 16u; ++k)
            same += mello_prng_next_u32(&t.engine.rng) == mello_prng_next_u32(&r) &&
                    mello_prng_next_u32(&t.engine.gen_rng) == mello_prng_next_u32(&g) &&
                    mello_prng_next_u32(&t.engine.drum_rng) == mello_prng_next_u32(&d);
        CHECK(same == 16u, "reseeded: %u of 16 numbers as the init's seeding gives them", same);
    }

    /* mixed: 0 changes nothing; a press's time moves all three */
    fresh();
    rolls(a, xa);
    fresh();
    mello_engine_mix_seed(&t.engine, 0u);
    rolls(b, xb);
    CHECK(!memcmp(a, b, sizeof a) && !memcmp(xa, xb, sizeof xa), "mixed with 0: other rolls");
    fresh();
    mello_engine_mix_seed(&t.engine, 123457u);
    rolls(c, xc);
    for (k = 0u, same = 0u; k < 3u; ++k) same += xa[k] == xc[k];
    CHECK(same == 0u, "mixed with a press's time: %u of the three streams as before", same);

    /* SEED's numbers: none 0 (the xorshift's dead state), each its own */
    for (k = 1u, same = 0u; k <= MELLO_SEED_MAX; ++k) {
        if (!mello_seed_of((uint16_t)k)) same++;
        if (k > 1u && mello_seed_of((uint16_t)k) == mello_seed_of((uint16_t)(k - 1u))) same++;
    }
    CHECK(same == 0u, "SEED 1..%u: %u seeds 0 or the same as the one before", MELLO_SEED_MAX, same);
    if (fails) return 1;
    puts("PASS: seed");
    return 0;
}
