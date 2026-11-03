/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* DRUMS (feedback 2026-10-30: "a very simple 16-step drum machine ... not stopped by the main sequence's per-step
 * stops unless asked"). Checked: the drums step every pulse whatever the sequence skips, and with FOLLOW only with
 * its steps; RATCH and CHANCE, a roll rising; a ROLL keeps the kick on the one and UNDO takes it back; each voice and
 * each GLITCH kind sounds, stays in range at its extremes (UBSan) and CRUSH changes it; STUTTER loops what was just
 * heard; FEED+ rolls the voices' sound within their ranges and FEED alone does not; DRIVE and FOLD at full stay in
 * range; the kit, its DRIVE, FOLD and FEED+ and the pattern survive a power cycle. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mello_audio.h"
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
static MelloAudio a;
static int16_t db[4096], rb[24576];

/* The drum hits over n pulses, counted per voice. */
static unsigned hits_over(unsigned n, unsigned per_voice[MELLO_DRUM_VOICES])
{
    unsigned i, total = 0u;
    MelloEvent ev;
    memset(per_voice, 0, sizeof(unsigned) * MELLO_DRUM_VOICES);
    for (i = 0u; i < n; ++i) {
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev))
            if (ev.type == MELLO_EVENT_DRUM_HIT) per_voice[ev.sample_slot % MELLO_DRUM_VOICES]++, total++;
    }
    return total;
}

static void start(bool follow)
{
    MelloProject p;
    MelloDrumConfig d;
    unsigned i;
    mello_project_defaults(&p);
    for (i = 0u; i < 16u; ++i) p.steps[i].skip = i >= 8u; /* half the sequence skipped */
    p.steps[0].pulses = 4u;                               /* and one long step */
    mello_engine_init(&e, &p, 7u);
    mello_drum_defaults(&d);
    d.on = true;
    d.follow = follow;
    memset(d.hits, 0, sizeof d.hits);
    d.hits[MELLO_DRUM_HAT] = 0xFFFFu; /* a hat on every step, one hit each */
    for (i = 0u; i < MELLO_DRUM_STEPS; ++i) d.lock[MELLO_DRUM_HAT][i].ratchet = 1u;
    mello_engine_drums_set_config(&e, &d);
    mello_engine_start(&e);
}

/* The flash as RAM, as storage_chord_regression's. */
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
    unsigned per[MELLO_DRUM_VOICES], n, v;

    start(false); /* 64 pulses, the sequence's skips and long step regardless: 64 hats */
    n = hits_over(64u, per);
    printf("own clock: %u hats in 64 pulses\n", per[MELLO_DRUM_HAT]);
    CHECK(per[MELLO_DRUM_HAT] == 64u, "the drums' own clock gave %u hats in 64 pulses", per[MELLO_DRUM_HAT]);
    start(true); /* FOLLOW: only at the sequence's steps (8 playing, the first 4 pulses long) */
    n = hits_over(64u, per);
    printf("FOLLOW: %u hats in 64 pulses\n", per[MELLO_DRUM_HAT]);
    CHECK(per[MELLO_DRUM_HAT] < 64u && per[MELLO_DRUM_HAT] > 0u, "FOLLOW gave %u hats", per[MELLO_DRUM_HAT]);

    start(false); /* RATCH 4 on every step: four hits a step */
    for (v = 0u; v < MELLO_DRUM_STEPS; ++v) e.drums.lock[MELLO_DRUM_HAT][v].ratchet = 4u;
    n = hits_over(16u, per);
    CHECK(per[MELLO_DRUM_HAT] == 64u, "RATCH 4 gave %u hits in 16 steps", per[MELLO_DRUM_HAT]);
    { /* the roll rises 2 semitones a hit */
        MelloEvent ev;
        uint8_t notes[4], k = 0u;
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev))
            if (ev.type == MELLO_EVENT_DRUM_HIT && ev.sample_slot == MELLO_DRUM_HAT && k < 4u)
                notes[k++] = ev.note;
        CHECK(k == 4u && notes[1] == notes[0] + 2u && notes[3] == notes[0] + 6u,
              "the roll did not rise (%u hits)", k);
    }
    start(false); /* CHANCE 0: none */
    e.drums.voice[MELLO_DRUM_HAT].chance = 0u;
    n = hits_over(32u, per);
    CHECK(n == 0u, "CHANCE 0 still gave %u hits", n);
    puts("RATCH 4: 64 hits in 16 steps; CHANCE 0: none");

    start(false); /* ROLL, then UNDO */
    {
        MelloDrumConfig before = e.drums;
        unsigned changed = 0u;
        e.drums.roll = 1000u;
        before.roll = 1000u;
        mello_engine_undo_snapshot(&e);
        mello_engine_drums_roll(&e);
        for (v = 0u; v < MELLO_DRUM_VOICES; ++v) changed += e.drums.hits[v] != before.hits[v];
        CHECK(changed > 0u, "a full ROLL changed no voice's hits");
        CHECK(e.drums.hits[MELLO_DRUM_KICK] & 1u, "a ROLL took the kick off the one");
        (void)mello_engine_undo_swap(&e);
        CHECK(!memcmp(e.drums.hits, before.hits, sizeof before.hits), "UNDO did not bring the drums back");
        printf("ROLL changed %u voices' hits, the kick on the one; UNDO restored them\n", changed);
    }

    { /* FEED+: a full roll moves TUNE, DECAY and TONE within each voice's range; FEED alone leaves them */
        MelloDrumVoice was[MELLO_DRUM_VOICES];
        unsigned moved = 0u;
        start(false);
        memcpy(was, e.drums.voice, sizeof was);
        e.drums.roll = 1000u;
        mello_engine_drums_roll(&e);
        CHECK(!memcmp(was, e.drums.voice, sizeof was), "a roll without FEED+ changed the voices");
        e.drums.feed_sound = true;
        mello_engine_drums_roll(&e);
        for (v = 0u; v < MELLO_DRUM_VOICES; ++v)
            moved += (was[v].tune != e.drums.voice[v].tune) + (was[v].decay != e.drums.voice[v].decay) +
                     (was[v].tone != e.drums.voice[v].tone);
        CHECK(moved >= 8u, "FEED+ moved only %u of 12 values", moved);
        CHECK(e.drums.voice[MELLO_DRUM_KICK].tune >= 250u && e.drums.voice[MELLO_DRUM_KICK].tune <= 600u &&
                  e.drums.voice[MELLO_DRUM_KICK].decay <= 250u,
              "FEED+ took the kick out of its range");
        printf("FEED+: a full roll moved %u of the voices' 12 values\n", moved);
    }

    { /* DRIVE and FOLD at full: in range */
        MelloSoundConfig s;
        MelloFxConfig fx;
        MelloSubConfig sub;
        MelloSampleConfig sm;
        MelloDrumConfig d;
        MelloEvent ev;
        int32_t out[2 * 441], peak = 0, clean = 0;
        unsigned blk, k, pass;
        for (pass = 0u; pass < 2u; ++pass) {
            mello_sound_defaults(&s);
            mello_fx_defaults(&fx);
            fx.delay_wet = fx.reverb_wet = 0u;
            mello_sub_defaults(&sub);
            mello_sample_defaults(&sm);
            mello_drum_defaults(&d);
            d.on = true;
            d.level = 1000u;
            d.drive = d.fold = pass ? 1000u : 0u;
            mello_audio_init(&a, db, 4096u, rb, 24576u);
            mello_audio_set_config(&a, &s, &fx, &sub, &sm);
            mello_audio_set_drum_config(&a, &d);
            memset(&ev, 0, sizeof ev);
            ev.type = MELLO_EVENT_DRUM_HIT;
            ev.velocity = 127u;
            ev.note = 64u;
            for (v = 0u; v < 3u; ++v) {
                ev.sample_slot = (uint8_t)v;
                (void)mello_audio_push_engine_event(&a, &ev, 0u);
            }
            for (blk = 0u; blk < 10u; ++blk) {
                memset(out, 0, sizeof out);
                mello_audio_render(&a, out, 441u, blk * 10000u, 120u);
                for (k = 0u; k < 882u; ++k) {
                    int32_t x = abs(out[k]);
                    if (pass)
                        peak = x > peak ? x : peak;
                    else
                        clean = x > clean ? x : clean;
                }
            }
        }
        printf("DRIVE and FOLD at full: peak %d (clean %d)\n", (int)peak, (int)clean);
        /* the level is held to the clean kit's (drive_makeup: mean, not peak), so the flattened peaks sit lower */
        CHECK(peak > clean / 8 && peak <= 32767, "DRIVE and FOLD: peak %d, clean %d", (int)peak, (int)clean);
    }

    for (v = 0u; v < 7u; ++v) { /* each voice and GLITCH kind, plain and crushed, at its extremes */
        static const char *names[7] = {"KICK", "SNARE", "HAT", "BLEEP", "CHIRP", "CRACKLE", "STUTTER"};
        static const uint16_t tone[7] = {1000u, 1000u, 500u, 0u, 400u, 600u, 999u};
        uint8_t voice = v < 3u ? (uint8_t)v : MELLO_DRUM_GLITCH;
        int32_t peak[2] = {0, 0};
        uint64_t diff = 0u;
        int32_t keep[2][4410];
        unsigned c;
        for (c = 0u; c < 2u; ++c) {
            MelloSoundConfig s;
            MelloFxConfig fx;
            MelloSubConfig sub;
            MelloSampleConfig sm;
            MelloDrumConfig d;
            MelloEvent ev;
            int32_t out[2 * 441];
            unsigned blk, k;
            mello_sound_defaults(&s);
            mello_fx_defaults(&fx);
            fx.delay_wet = fx.reverb_wet = 0u;
            mello_sub_defaults(&sub);
            mello_sample_defaults(&sm);
            mello_drum_defaults(&d);
            d.on = true;
            d.level = 1000u;
            d.voice[voice].level = 1000u;
            d.voice[voice].tone = tone[v];
            d.voice[voice].crush = c ? 1000u : 0u;
            mello_audio_init(&a, db, 4096u, rb, 24576u);
            mello_audio_set_config(&a, &s, &fx, &sub, &sm);
            mello_audio_set_drum_config(&a, &d);
            memset(&ev, 0, sizeof ev);
            ev.type = MELLO_EVENT_DRUM_HIT;
            ev.note = 64u + 24u; /* PITCH +24 */
            ev.velocity = 127u;
            ev.drum_decay = 100;
            if (v == 6u) { /* STUTTER: a kick first, for it to loop */
                ev.sample_slot = MELLO_DRUM_KICK;
                (void)mello_audio_push_engine_event(&a, &ev, 0u);
            }
            ev.sample_slot = voice;
            (void)mello_audio_push_engine_event(&a, &ev, v == 6u ? 20000u : 0u);
            for (blk = 0u; blk < 10u; ++blk) {
                memset(out, 0, sizeof out);
                mello_audio_render(&a, out, 441u, blk * 10000u, 120u);
                for (k = 0u; k < 441u; ++k) {
                    int32_t x = out[2u * k];
                    if (abs(x) > peak[c]) peak[c] = abs(x);
                    keep[c][blk * 441u + k] = x;
                }
            }
        }
        for (c = 0u; c < 4410u; ++c) diff += (uint64_t)abs(keep[0][c] - keep[1][c]);
        printf("%-7s peak %5d, crushed %5d, differ by %llu\n", names[v], (int)peak[0], (int)peak[1],
               (unsigned long long)diff);
        CHECK(peak[0] > 300 && peak[0] <= 32767, "%s's peak %d", names[v], (int)peak[0]);
        CHECK(diff > 10000u, "%s: CRUSH changed nothing", names[v]);
    }

    { /* the kit and pattern through storage */
        static MelloStorage st;
        static MelloEngine b;
        MelloProject p;
        MelloDrumConfig d;
        MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
        uint32_t t;
        memset(flash, 0xFF, sizeof flash);
        mello_project_defaults(&p);
        mello_engine_init(&b, &p, 1u);
        mello_storage_init(&st, &io, NULL);
        (void)mello_storage_load(&st, &b);
        (void)mello_storage_poll(&st, &b, 0u, true);
        d = b.drums;
        d.on = d.feed = d.follow = d.feed_sound = true;
        d.drive = 370u;
        d.fold = 820u;
        d.length = 11u;
        d.swing = 333u;
        d.voice[MELLO_DRUM_GLITCH].tone = 777u;
        d.hits[MELLO_DRUM_SNARE] = 0x1234u;
        d.accents[MELLO_DRUM_HAT] = 0x8001u;
        d.lock[MELLO_DRUM_KICK][5].pitch = -7;
        d.lock[MELLO_DRUM_HAT][15].ratchet = 3u;
        d.lock[MELLO_DRUM_SNARE][2].crush = 55u;
        mello_engine_drums_set_config(&b, &d);
        for (t = 100u; t <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u)
            w = mello_storage_poll(&st, &b, t, true);
        CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "the drums were never saved");
        mello_engine_init(&b, &p, 1u);
        mello_storage_init(&st, &io, NULL);
        CHECK(mello_storage_load(&st, &b) == MELLO_STORAGE_LOAD_OK, "reload");
        CHECK(b.drums.on && b.drums.feed && b.drums.follow && b.drums.length == 11u &&
                  b.drums.swing == 333u && b.drums.voice[MELLO_DRUM_GLITCH].tone == 777u &&
                  b.drums.hits[MELLO_DRUM_SNARE] == 0x1234u && b.drums.accents[MELLO_DRUM_HAT] == 0x8001u &&
                  b.drums.lock[MELLO_DRUM_KICK][5].pitch == -7 &&
                  b.drums.lock[MELLO_DRUM_HAT][15].ratchet == 3u &&
                  b.drums.lock[MELLO_DRUM_SNARE][2].crush == 55u && b.drums.feed_sound &&
                  b.drums.drive == 370u && b.drums.fold == 820u,
              "the drums differ after a reload");
        puts("the kit and pattern survive a power cycle");
    }

    (void)n;
    if (fails) return 1;
    puts("PASS: the drums' clock, RATCH, CHANCE, ROLL / UNDO, voices, CRUSH and storage");
    return 0;
}
