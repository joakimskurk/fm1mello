/* Phase 5: DOT's orders (DECISIONS.md "DOT orders"). The routes are what they say (spiral, diagonals, ping-pong
 * without doubled ends, the bouncing ball, LEN taking the route's first cells); the engine reads them in that order,
 * backwards with REV; WALK steps to neighbours within LEN; RANDOM plays each of the first LEN cells once a round; the
 * order and REV survive a power cycle (REV in bit 7 of the mode byte). */
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

static int same(const uint8_t *a, const uint8_t *b, uint8_t n)
{
    return !memcmp(a, b, n);
}

/* The cells MOD 1 reads over n pulses (all of them lit, reading every pulse). */
static void read_cells(MelloEngine *e, MelloDotMode mode, uint8_t len, bool rev, uint8_t n, uint8_t *out)
{
    MelloEvent ev;
    MelloDotMatrixConfig d;
    uint8_t i;
    mello_dot_defaults_for(&d, MELLO_DOT_MOD1);
    d.cells = 0xFFFFu, d.mode = mode, d.length = len, d.reverse = rev, d.reset = false;
    mello_engine_stop(e);
    mello_engine_dot_set_config(e, MELLO_DOT_MOD1, &d);
    mello_engine_dot_reset_runtime(e);
    mello_engine_start(e);
    for (i = 0u; i < n; ++i) {
        (void)mello_engine_advance_pulse(e);
        while (mello_event_pop(e, &ev)) {
        }
        out[i] = (uint8_t)e->dot_active_cell[MELLO_DOT_MOD1];
    }
}

int main(void)
{
    static MelloEngine e;
    static MelloStorage s;
    MelloStorageLoadResult r;
    MelloProject p;
    uint8_t route[MELLO_DOT_ROUTE_MAX], got[40], i, n;
    static const uint8_t spiral[16] = {0, 1, 2, 3, 7, 11, 15, 14, 13, 12, 8, 4, 5, 6, 10, 9};
    static const uint8_t diag[16] = {0, 1, 4, 2, 5, 8, 3, 6, 9, 12, 7, 10, 13, 11, 14, 15};
    static const uint8_t pp4[6] = {0, 1, 2, 3, 2, 1};
    static const uint8_t bounce8[6] = {0, 5, 2, 5, 2, 7};

    CHECK(mello_dot_route(MELLO_DOT_SPIRAL, 16u, route) == 16u && same(route, spiral, 16u), "SPIRAL route");
    CHECK(mello_dot_route(MELLO_DOT_SPIRAL, 12u, route) == 12u && same(route, spiral, 12u),
          "SPIRAL LEN 12: the ring");
    CHECK(mello_dot_route(MELLO_DOT_DIAGONAL, 16u, route) == 16u && same(route, diag, 16u), "DIAG route");
    CHECK(mello_dot_route(MELLO_DOT_PINGPONG, 4u, route) == 6u && same(route, pp4, 6u),
          "P-PONG doubles its ends");
    CHECK(mello_dot_route(MELLO_DOT_PINGPONG, 16u, route) == 30u, "P-PONG 16 is not 30 long");
    CHECK(mello_dot_route(MELLO_DOT_BOUNCE, 16u, route) == 12u, "BOUNCE is not 12 long");
    CHECK(mello_dot_route(MELLO_DOT_BOUNCE, 8u, route) == 6u && same(route, bounce8, 6u), "BOUNCE LEN 8");

    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 99u);
    read_cells(&e, MELLO_DOT_SPIRAL, 16u, false, 16u, got);
    CHECK(same(got, spiral, 16u), "the engine does not read SPIRAL in its order");
    read_cells(&e, MELLO_DOT_SPIRAL, 16u, true, 16u, got);
    for (i = 0u; i < 16u; ++i) CHECK(got[i] == spiral[15u - i], "REV does not read SPIRAL backwards");
    read_cells(&e, MELLO_DOT_PINGPONG, 4u, false, 12u, got);
    CHECK(same(got, pp4, 6u) && same(got + 6, pp4, 6u), "the engine does not ping-pong");
    read_cells(&e, MELLO_DOT_WALK, 8u, false, 40u, got);
    for (i = 1u; i < 40u; ++i) {
        uint8_t a = got[i - 1u], b = got[i], d = (uint8_t)(a > b ? a - b : b - a);
        CHECK(b < 8u, "WALK left LEN (%u)", b);
        CHECK(d == 4u || (d == 1u && (a >> 2) == (b >> 2)), "WALK jumped %u -> %u", a, b);
    }
    read_cells(&e, MELLO_DOT_RANDOM, 6u, false, 12u, got);
    for (n = 0u; n < 2u; ++n) {
        uint8_t seen = 0u;
        for (i = 0u; i < 6u; ++i) seen |= (uint8_t)(1u << got[n * 6u + i]);
        CHECK(seen == 0x3Fu, "RANDOM round %u did not play each of its 6 cells once", n);
    }

    /* The order and REV through a power cycle. */
    memset(flash, 0xFF, sizeof flash);
    mello_engine_init(&e, &p, 1u);
    mello_storage_init(&s, &io, NULL);
    (void)mello_storage_load(&s, &e);
    (void)mello_storage_poll(&s, &e, 0u, true);
    {
        MelloDotMatrixConfig d = e.dot[MELLO_DOT_RATCHET];
        uint32_t t = 100u;
        MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
        d.mode = MELLO_DOT_WALK, d.reverse = true;
        mello_engine_dot_set_config(&e, MELLO_DOT_RATCHET, &d);
        for (; t <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u)
            w = mello_storage_poll(&s, &e, t, true);
        CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "never saved");
    }
    boot(&s, &e, &r);
    CHECK(r == MELLO_STORAGE_LOAD_OK && e.dot[MELLO_DOT_RATCHET].mode == MELLO_DOT_WALK &&
              e.dot[MELLO_DOT_RATCHET].reverse,
          "WALK + REV lost in a power cycle");
    puts("PASS: DOT orders: routes, the engine reads them (REV, P-PONG, WALK, RANDOM), saved with REV");
    return 0;
}
