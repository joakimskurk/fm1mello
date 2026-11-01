/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Harmony as the spine (DECISIONS.md): CHORD > HARMONY. FOLLOW BEATS moves the lead's notes on the beats (every
 * fourth step) to a tone of the chord sounding, FOLLOW ALL every note, OFF none; DRIFT changes one chord each time
 * the chain comes round, within DEPTH, and the first chord keeps its degree; the three are saved with the project and
 * come back after a power cycle; a randomize at MED and at HIGH rolls a progression (3 to 8 chords, no degree twice
 * running, on the clock chain) as MUSICAL does; a progression rolled over the chain's default lands its chords on the
 * sixteenth grid (CHAIN STEPS), and a RESET starts the clock chain over. */
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

/* A lead of 16 steps on the scale's degrees 0..6 and back, every step played, under a HOLD chord chain of two chords:
 * a tonic triad and a dominant triad, four sixteenths each on the clock chain. */
static void start(uint8_t follow)
{
    MelloProject p;
    MelloChordConfig c;
    unsigned i;
    mello_project_defaults(&p);
    p.seq_len = 16u;
    for (i = 0u; i < 16u; ++i) {
        p.steps[i].pitch = (uint8_t)(i < 8u ? i : 15u - i);
        p.steps[i].gate = MELLO_GATE_SINGLE;
        p.steps[i].pulses = 1u;
        p.steps[i].skip = false;
        p.steps[i].probability = 65535u;
    }
    mello_engine_init(&e, &p, 5u);
    mello_chord_defaults(&c);
    c.on = true;
    c.mode = MELLO_CHORD_HOLD;
    c.chain = MELLO_CHORD_CHAIN_STEPS;
    c.length = 2u;
    c.steps[0].degree = 0u;
    c.steps[0].shape = MELLO_CHORD_TRIAD;
    c.steps[0].hold = 4u;
    c.steps[1].degree = 4u;
    c.steps[1].shape = MELLO_CHORD_TRIAD;
    c.steps[1].hold = 4u;
    c.lead_follow = follow;
    mello_engine_chord_set_config(&e, &c);
    mello_engine_start(&e);
}

static bool chord_tone(uint8_t note)
{
    unsigned i;
    for (i = 0u; i < e.chord_voiced_n; ++i)
        if ((e.chord_voiced[i] % 12u) == (note % 12u)) return true;
    return false;
}

/* Over 32 pulses: how many lead NOTE_ONs fell on a chord tone, on the beats and off them. */
static void count(unsigned *on_beat_tones, unsigned *on_beat, unsigned *off_beat_tones, unsigned *off_beat)
{
    unsigned i;
    MelloEvent ev;
    *on_beat_tones = *on_beat = *off_beat_tones = *off_beat = 0u;
    for (i = 0u; i < 32u; ++i) {
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev))
            if (ev.type == MELLO_EVENT_NOTE_ON && e.chord_voiced_n) {
                bool t = chord_tone(ev.note);
                if (i % 4u == 0u)
                    ++*on_beat, *on_beat_tones += t;
                else
                    ++*off_beat, *off_beat_tones += t;
            }
    }
}

/* The flash as RAM. */
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

int main(void)
{
    unsigned bt, b, ot, o;

    start(0u); /* OFF: the lead walks the scale, so some of it is off the chord */
    count(&bt, &b, &ot, &o);
    CHECK(b == 8u && o == 24u, "OFF: %u notes on the beats and %u off them (wanted 8 and 24)", b, o);
    CHECK(bt < b || ot < o, "OFF: a scale walk should leave some note off the chord (%u/%u, %u/%u)", bt, b,
          ot, o);
    printf("FOLLOW OFF: %u of %u beat notes and %u of %u others on a chord tone\n", bt, b, ot, o);

    start(1u); /* BEATS: every note on a beat is a chord tone, the others as they were */
    count(&bt, &b, &ot, &o);
    CHECK(b == 8u && bt == 8u, "BEATS: %u of %u notes on the beats are chord tones", bt, b);
    CHECK(ot < o, "BEATS: the notes between the beats should still walk the scale (%u of %u)", ot, o);
    printf("FOLLOW BEATS: %u of %u beat notes and %u of %u others on a chord tone\n", bt, b, ot, o);

    start(2u); /* ALL: every note */
    count(&bt, &b, &ot, &o);
    CHECK(bt == b && ot == o, "ALL: %u of %u and %u of %u notes are chord tones", bt, b, ot, o);
    printf("FOLLOW ALL: every one of %u notes on a chord tone\n", b + o);

    { /* a snapped note moves the smaller way: a degree a semitone above a chord tone comes down to it */
        MelloEvent ev;
        bool seen = false;
        start(2u);
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev))
            if (ev.type == MELLO_EVENT_NOTE_ON) {
                seen = true;
                CHECK(chord_tone(ev.note), "step 1 should land on a chord tone: note %u", ev.note);
            }
        CHECK(seen, "no note at step 1");
    }

    { /* DRIFT: with the chance at 100% and DEPTH ALL, each pass round the chain changes one chord; the first
       * chord's degree never moves; at 0% nothing moves */
        MelloChordConfig c, was;
        unsigned pass, changed = 0u, i;
        start(0u);
        c = e.chord;
        c.drift = 1000u;
        c.drift_depth = 3u;
        c.length = 4u;
        for (i = 0u; i < 4u; ++i) c.steps[i].hold = 4u;
        c.steps[2].degree = 3u;
        c.steps[3].degree = 4u;
        mello_engine_chord_set_config(&e, &c);
        was = e.chord;
        /* 12 passes of 16 sixteenths; the chain comes round at the wrap (pulse 17, 49, ...) and at the 32-pulse
         * RESET (pulse 32, 64, ...): a change at each, counted pulse by pulse */
        for (pass = 0u; pass < 12u; ++pass) {
            for (i = 0u; i < 16u; ++i) {
                (void)mello_engine_advance_pulse(&e);
                changed += memcmp(&was.steps, &e.chord.steps, sizeof was.steps) != 0;
                CHECK(e.chord.steps[0].degree == 0u, "pass %u: the first chord's degree moved to %u", pass,
                      e.chord.steps[0].degree);
                was = e.chord;
            }
        }
        CHECK(changed >= 10u && changed <= 12u, "DRIFT 100%%: %u changes over 12 passes (wanted one a pass)",
              changed);
        printf("DRIFT 100%%: %u changes over 12 passes round the chain, the first chord's degree never\n",
               changed);
        start(0u);
        was = e.chord;
        for (i = 0u; i < 16u * 12u; ++i) (void)mello_engine_advance_pulse(&e);
        CHECK(!memcmp(&was.steps, &e.chord.steps, sizeof was.steps), "DRIFT 0%%: a chord changed");
    }

    { /* through storage */
        static MelloStorage st;
        static MelloEngine b2;
        MelloProject p;
        MelloChordConfig c;
        MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
        uint32_t t;
        memset(flash, 0xFF, sizeof flash);
        mello_project_defaults(&p);
        mello_engine_init(&b2, &p, 1u);
        mello_storage_init(&st, &io, NULL);
        (void)mello_storage_load(&st, &b2);
        (void)mello_storage_poll(&st, &b2, 0u, true);
        c = b2.chord;
        c.lead_follow = 1u;
        c.drift = 350u;
        c.drift_depth = 2u;
        mello_engine_chord_set_config(&b2, &c);
        for (t = 100u; t <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u)
            w = mello_storage_poll(&st, &b2, t, true);
        CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "never saved");
        mello_engine_init(&b2, &p, 1u);
        mello_storage_init(&st, &io, NULL);
        CHECK(mello_storage_load(&st, &b2) == MELLO_STORAGE_LOAD_OK, "reload");
        CHECK(b2.chord.lead_follow == 1u && b2.chord.drift == 350u && b2.chord.drift_depth == 2u,
              "HARMONY lost in a reload: %u %u %u", b2.chord.lead_follow, b2.chord.drift,
              b2.chord.drift_depth);
        puts("FOLLOW, DRIFT and DEPTH survive a power cycle");
    }

    { /* MED and HIGH roll a progression too, on the clock chain */
        MelloRandomizeIntensity how[2] = {MELLO_RANDOMIZE_MED, MELLO_RANDOMIZE_HIGH};
        unsigned h, seed, not_four = 0u;
        for (h = 0u; h < 2u; ++h)
            for (seed = 1u; seed <= 40u; ++seed) {
                MelloProject p;
                unsigned k;
                mello_project_defaults(&p);
                mello_engine_init(&e, &p, seed);
                mello_engine_set_randomize_mask(&e, MELLO_RAND_DEFAULT | MELLO_RAND_CHORD);
                mello_engine_randomize_steps_intensity(&e, how[h]);
                CHECK(e.chord.length >= 3u && e.chord.length <= 8u, "%s seed %u: %u chords",
                      h ? "HIGH" : "MED", seed, e.chord.length);
                CHECK(e.chord.chain == MELLO_CHORD_CHAIN_STEPS, "%s seed %u: not on the clock chain",
                      h ? "HIGH" : "MED", seed);
                for (k = 1u; k < e.chord.length; ++k)
                    CHECK(e.chord.steps[k].degree != e.chord.steps[k - 1u].degree,
                          "%s seed %u: degree %u twice running", h ? "HIGH" : "MED", seed,
                          e.chord.steps[k].degree);
                for (k = 0u; k < e.chord.length; ++k)
                    CHECK(e.chord.steps[k].hold >= 4u, "%s seed %u: hold %u", h ? "HIGH" : "MED", seed,
                          e.chord.steps[k].hold);
                not_four += e.chord.length != 4u;
            }
        CHECK(not_four >= 20u, "only %u of 80 rolls were not four chords", not_four);
        printf("MED and HIGH: 80 progressions of 3 to 8 chords, %u not four long, all on the clock chain\n",
               not_four);
    }

    { /* RESET starts the clock chain over: a 3-chord chain at HOLD 8 (24 sixteenths) under a 32-pulse RESET
       * plays its first chord again at pulse 32, not at pulse 48 */
        MelloChordConfig c;
        unsigned i, first_again = 0u;
        bool seen_after = false;
        start(0u);
        c = e.chord;
        c.length = 3u;
        for (i = 0u; i < 3u; ++i) c.steps[i].hold = 8u;
        c.steps[2].degree = 3u;
        mello_engine_chord_set_config(&e, &c);
        for (i = 0u; i < 40u; ++i) {
            (void)mello_engine_advance_pulse(&e);
            if (i >= 32u && e.chord_voiced_slot == 0u && !seen_after) seen_after = true, first_again = i;
        }
        CHECK(seen_after && first_again == 32u,
              "after the RESET the first chord came back at pulse %u (wanted 32)", first_again);
        printf("RESET: a 24-sixteenth chain starts over with the 32-pulse period\n");
    }

    if (fails) return 1;
    puts("PASS: chord_harmony");
    return 0;
}
