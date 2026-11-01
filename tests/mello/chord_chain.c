/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* How the chord chain moves (ROADMAP: "HOLD in steps rather than triggers; chords that follow the segments; the sub on
 * the chord's root"). Checked: CHAIN STEPS changes chord every HOLD sixteenths whatever the sequence skips or holds,
 * and HOLD's chord sounds at once; CHAIN SEG plays chord k in segment k, also when the segments play in another order;
 * SUB ROOT moves the sub (and BASS SEQ) by the chord's root, the nearest way; TRIG is as it was; it all survives a
 * power cycle. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mello_engine.h"
#include "mello_storage.h"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

static MelloEngine e;

static void start(MelloChordChain chain, bool odd)
{
    MelloProject p;
    MelloChordConfig c;
    unsigned i;
    mello_project_defaults(&p);
    if (odd) /* steps 5-8 skipped and step 1 four pulses long: no effect on STEPS */
        for (i = 4u; i < 8u; ++i) p.steps[i].skip = true;
    if (odd) p.steps[0].pulses = 4u;
    mello_engine_init(&e, &p, 3u);
    mello_chord_defaults(&c);
    c.on = true;
    c.mode = MELLO_CHORD_HOLD;
    c.chain = chain;
    c.length = 4u;
    for (i = 0u; i < 4u; ++i) c.steps[i].hold = 4u;
    mello_engine_chord_set_config(&e, &c);
    mello_engine_start(&e);
}

/* Over n pulses: at which pulses a chord began (its first NOTE_ON) and with which chain slot. */
static unsigned chords_over(unsigned n, unsigned at[32], uint8_t slot[32])
{
    unsigned i, k = 0u;
    MelloEvent ev;
    for (i = 0u; i < n; ++i) {
        bool began = false;
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev))
            if (ev.type == MELLO_EVENT_CHORD_NOTE_ON && !began && k < 32u) {
                began = true;
                at[k] = i, slot[k] = e.chord_voiced_slot, ++k;
            }
    }
    return k;
}

/* The flash as RAM, as drum_machine's. */
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
    const uint8_t *s = src;
    uint32_t i;
    (void)ctx;
    if (address + length > FLASH_SIZE) return false;
    for (i = 0u; i < length; ++i) flash[address + i] &= s[i];
    return true;
}
static const MelloStorageIo io = {ram_read, ram_erase4k, ram_program, NULL};

int main(void)
{
    unsigned at[32], n, i;
    uint8_t slot[32];

    start(MELLO_CHORD_CHAIN_STEPS,
          true); /* HOLD 4 sixteenths each: chords at 0, 4, 8 .. whatever the steps do */
    n = chords_over(32u, at, slot);
    printf("STEPS: %u chords in 32 pulses, at", n);
    for (i = 0u; i < n; ++i) printf(" %u(%u)", at[i], slot[i] + 1u);
    printf("\n");
    CHECK(n == 8u, "STEPS gave %u chords in 32 pulses, not 8", n);
    for (i = 0u; i < n && i < 8u; ++i)
        CHECK(at[i] == i * 4u && slot[i] == i % 4u, "STEPS: chord %u at pulse %u (slot %u)", i, at[i],
              slot[i]);

    start(MELLO_CHORD_CHAIN_SEG, false); /* the default four segments of four: chord k in segment k */
    n = chords_over(32u, at, slot);
    printf("SEG: %u chords in 32 pulses, at", n);
    for (i = 0u; i < n; ++i) printf(" %u(%u)", at[i], slot[i] + 1u);
    printf("\n");
    CHECK(n == 8u, "SEG gave %u chords in 32 pulses, not 8", n);
    for (i = 0u; i < n && i < 8u; ++i)
        CHECK(at[i] == i * 4u && slot[i] == i % 4u, "SEG: chord %u at pulse %u (slot %u)", i, at[i], slot[i]);
    { /* segments in another order: B and D reversed, C pinged; the chord is still the segment's */
        MelloEvent ev;
        unsigned bad = 0u;
        start(MELLO_CHORD_CHAIN_SEG, false);
        e.project.seg_direction[1] = MELLO_SEG_REV;
        e.project.seg_repeats[2] = 2u;
        (void)mello_engine_rebuild_sequence(&e);
        for (i = 0u; i < 40u; ++i) {
            uint8_t seg =
                mello_engine_current_segment(&e); /* the pulse's (advancing moves to the next one's) */
            (void)mello_engine_advance_pulse(&e);
            while (mello_event_pop(&e, &ev)) {
            }
            if (e.chord_voiced_slot != (uint8_t)(seg % 4u)) bad++;
        }
        CHECK(!bad, "SEG: %u pulses with another segment's chord", bad);
    }

    start(MELLO_CHORD_CHAIN_TRIG, true); /* TRIG: HOLD counts the steps that play, as before */
    n = chords_over(32u, at, slot);
    printf("TRIG: %u chords in 32 pulses\n", n);
    CHECK(n < 8u && n > 0u, "TRIG gave %u chords, as STEPS would", n);

    { /* SUB ROOT: the sub's note moves by the chord's root, the nearest way (VI down 4, iv up 5, v down 5) */
        static const int want[4] = {0, -4, 5, -5};
        int got[4] = {99, 99, 99, 99};
        uint8_t plain = 0u;
        MelloEvent ev;
        MelloChordConfig c;
        start(MELLO_CHORD_CHAIN_STEPS, false);
        c = e.chord;
        c.sub_root = true;
        mello_engine_chord_set_config(&e, &c);
        for (i = 0u; i < 16u; ++i) e.project.steps[i].sub = true;
        for (i = 0u; i < 16u; ++i) {
            (void)mello_engine_advance_pulse(&e);
            while (mello_event_pop(&e, &ev))
                if (ev.type == MELLO_EVENT_SUB_NOTE_ON && i % 4u == 1u) got[i / 4u] = ev.note;
        }
        start(MELLO_CHORD_CHAIN_STEPS, false);
        for (i = 0u; i < 16u; ++i) e.project.steps[i].sub = true;
        for (i = 0u; i < 2u; ++i) {
            (void)mello_engine_advance_pulse(&e);
            while (mello_event_pop(&e, &ev))
                if (ev.type == MELLO_EVENT_SUB_NOTE_ON) plain = ev.note;
        }
        printf("SUB ROOT: the sub at %d %d %d %d against %u\n", got[0], got[1], got[2], got[3], plain);
        for (i = 0u; i < 4u; ++i)
            CHECK(got[i] - (int)plain == want[i], "SUB ROOT: chord %u moved the sub %d, not %d", i + 1u,
                  got[i] - (int)plain, want[i]);
    }

    { /* through storage */
        static MelloStorage st;
        static MelloEngine b;
        MelloProject p;
        MelloChordConfig c;
        MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
        uint32_t t;
        memset(flash, 0xFF, sizeof flash);
        mello_project_defaults(&p);
        mello_engine_init(&b, &p, 1u);
        mello_storage_init(&st, &io, NULL);
        (void)mello_storage_load(&st, &b);
        (void)mello_storage_poll(&st, &b, 0u, true);
        c = b.chord;
        c.chain = MELLO_CHORD_CHAIN_SEG;
        c.sub_root = true;
        c.steps[2].hold = 48u;
        mello_engine_chord_set_config(&b, &c);
        for (t = 100u; t <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u)
            w = mello_storage_poll(&st, &b, t, true);
        CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "never saved");
        mello_engine_init(&b, &p, 1u);
        mello_storage_init(&st, &io, NULL);
        CHECK(mello_storage_load(&st, &b) == MELLO_STORAGE_LOAD_OK, "reload");
        CHECK(b.chord.chain == MELLO_CHORD_CHAIN_SEG && b.chord.sub_root && b.chord.steps[2].hold == 48u,
              "CHAIN, SUB ROOT or a long HOLD lost in a reload");
        puts("CHAIN, SUB ROOT and HOLD 48 survive a power cycle");
    }

    if (fails) return 1;
    puts("PASS: the chord chain by steps, by segments, the sub on its root");
    return 0;
}
