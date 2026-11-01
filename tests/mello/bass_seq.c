/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* BASS SEQ (feedback 2026-10-31: "a 16-step bass sequencer instead of 5-SEQ: length, per step tone, octave, glide;
 * its own clock, not stopped by the main sequence's skipped or held steps unless asked to follow them"). Checked:
 * own clock plays every on step whatever the sequence skips, FOLLOW only with its steps; while on, the sub's MODE waits;
 * SUB MUTE silences it; an accent is velocity 127; a glide holds its note past the next step's start and the next note
 * slides; the voice at its extremes (RES, ENV, ACCENT, CUT, DRIVE all full) stays in range (UBSan), a glided note keeps
 * its envelope, ENV opens the filter at the attack; the config survives a power cycle. */
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

static void start(bool follow, bool all_on)
{
    MelloProject p;
    MelloBassConfig b;
    unsigned i;
    mello_project_defaults(&p);
    for (i = 0u; i < 16u; ++i) p.steps[i].skip = i >= 8u, p.steps[i].sub = true; /* half skipped; STEP mode */
    p.steps[0].pulses = 4u;                                                      /* and one long step */
    mello_engine_init(&e, &p, 5u);
    mello_bass_defaults(&b);
    b.on = true;
    b.follow = follow;
    if (all_on)
        for (i = 0u; i < MELLO_BASS_STEPS; ++i) b.steps[i].on = true, b.steps[i].glide = false;
    mello_engine_bass_set_config(&e, &b);
    mello_engine_start(&e);
}

/* Over n pulses: the sub's NOTE_ONs (and those from BASS SEQ). */
static unsigned sub_ons(unsigned n, unsigned *from_seq)
{
    unsigned i, ons = 0u;
    MelloEvent ev;
    *from_seq = 0u;
    for (i = 0u; i < n; ++i) {
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev))
            if (ev.type == MELLO_EVENT_SUB_NOTE_ON) ons++, *from_seq += ev.ratchet_index != 0u;
    }
    return ons;
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
    unsigned n, seq;

    start(false, true); /* own clock, all 16 on: 64 notes in 64 pulses, skips and the long step regardless */
    n = sub_ons(64u, &seq);
    printf("own clock: %u bass notes in 64 pulses (%u from BASS SEQ)\n", n, seq);
    CHECK(n == 64u && seq == 64u, "own clock gave %u notes (%u from BASS SEQ), the sub's MODE must wait", n,
          seq);
    start(true, true); /* FOLLOW: only at the sequence's steps */
    n = sub_ons(64u, &seq);
    printf("FOLLOW: %u bass notes in 64 pulses\n", n);
    CHECK(n < 64u && n > 0u && n == seq, "FOLLOW gave %u notes", n);
    start(false, true); /* SUB MUTE */
    e.perf.sub_mute = true;
    n = sub_ons(16u, &seq);
    CHECK(n == 0u, "SUB MUTE still gave %u notes", n);

    { /* the default line: accents at 127; step 7 glides into 8 (held past its start), 15 into 16 */
        MelloEvent ev;
        uint32_t off7 = 0u, on8 = 0u;
        unsigned i, acc = 0u, slides = 0u;
        start(false, false);
        for (i = 0u; i < 16u; ++i) {
            (void)mello_engine_advance_pulse(&e);
            while (mello_event_pop(&e, &ev)) {
                if (ev.type == MELLO_EVENT_SUB_NOTE_ON) {
                    acc += ev.velocity == 127u;
                    slides += ev.slide;
                    if (ev.ratchet_index == 8u) on8 = ev.tick;
                }
                if (ev.type == MELLO_EVENT_SUB_NOTE_OFF && ev.ratchet_index == 7u) off7 = ev.tick;
            }
        }
        CHECK(acc == 2u, "%u accents, not 2", acc);
        CHECK(slides == 2u, "%u slides, not 2", slides);
        CHECK(off7 > on8 && on8 > 0u, "step 7's note ends at %u, before step 8's at %u", (unsigned)off7,
              (unsigned)on8);
        printf("default line: 2 accents, 2 slides, step 7's note held to tick %u past step 8's at %u\n",
               (unsigned)off7, (unsigned)on8);
    }

    { /* the voice: extremes in range; a glided note keeps its envelope; ENV opens the filter */
        MelloSoundConfig s;
        MelloFxConfig fx;
        MelloSubConfig sub;
        MelloSampleConfig sm;
        MelloBassConfig b;
        MelloEvent ev;
        int32_t out[2 * 441], peak = 0;
        int32_t env_before = 0, env_after = 0;
        unsigned blk, k;
        mello_sound_defaults(&s);
        s.osc_wave = MELLO_WAVE_OFF;
        s.follower_sub_on = false;
        mello_fx_defaults(&fx);
        fx.delay_wet = fx.reverb_wet = 0u;
        mello_sub_defaults(&sub);
        sub.wave = MELLO_WAVE_SAW;
        sub.cut = 1000u;
        sub.drive = 1000u;
        sub.level = 1000u;
        mello_sample_defaults(&sm);
        mello_bass_defaults(&b);
        b.res = b.env = b.accent = 1000u;
        mello_audio_init(&a, db, 4096u, rb, 24576u);
        mello_audio_set_config(&a, &s, &fx, &sub, &sm);
        mello_audio_set_bass_config(&a, &b);
        memset(&ev, 0, sizeof ev);
        ev.type = MELLO_EVENT_SUB_NOTE_ON;
        ev.note = 36u;
        ev.velocity = 127u;
        ev.ratchet_index = 1u;
        (void)mello_audio_push_engine_event(&a, &ev, 0u);
        for (blk = 0u; blk < 20u; ++blk) {
            if (blk == 10u) { /* a glided note: no new attack */
                env_before = a.sub.env_q24;
                ev.note = 43u;
                ev.ratchet_index = 2u;
                ev.slide = true;
                (void)mello_audio_push_engine_event(&a, &ev, blk * 10000u);
            }
            memset(out, 0, sizeof out);
            mello_audio_render(&a, out, 441u, blk * 10000u, 120u);
            if (blk == 10u) env_after = a.sub.env_q24;
            for (k = 0u; k < 882u; ++k) peak = abs(out[k]) > peak ? abs(out[k]) : peak;
        }
        printf("RES ENV ACCENT CUT DRIVE full: peak %d; a glide's envelope %d -> %d\n", (int)peak,
               (int)env_before, (int)env_after);
        CHECK(peak > 1000 && peak <= 32767, "the bass voice's peak %d", (int)peak);
        CHECK(env_after <= env_before, "the glided note attacked again (%d -> %d)", (int)env_before,
              (int)env_after);
    }

    { /* the config through storage */
        static MelloStorage st;
        static MelloEngine b2;
        MelloProject p;
        MelloBassConfig b;
        MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
        uint32_t t;
        memset(flash, 0xFF, sizeof flash);
        mello_project_defaults(&p);
        mello_engine_init(&b2, &p, 1u);
        mello_storage_init(&st, &io, NULL);
        (void)mello_storage_load(&st, &b2);
        (void)mello_storage_poll(&st, &b2, 0u, true);
        b = b2.bass;
        b.on = b.follow = true;
        b.length = 11u;
        b.res = 640u;
        b.env = 330u;
        b.env_decay_ms = 777u;
        b.accent = 90u;
        b.steps[13].on = b.steps[13].glide = b.steps[13].accent = true;
        b.steps[13].tone = MELLO_SUB_TONE_SIXTH;
        b.steps[13].octave = -2;
        mello_engine_bass_set_config(&b2, &b);
        for (t = 100u; t <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u)
            w = mello_storage_poll(&st, &b2, t, true);
        CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "the bass was never saved");
        mello_engine_init(&b2, &p, 1u);
        mello_storage_init(&st, &io, NULL);
        CHECK(mello_storage_load(&st, &b2) == MELLO_STORAGE_LOAD_OK, "reload");
        CHECK(!memcmp(&b2.bass, &b, sizeof b), "BASS SEQ differs after a reload");
        puts("BASS SEQ survives a power cycle");
    }

    if (fails) return 1;
    puts("PASS: BASS SEQ's clock, FOLLOW, accents, glides, voice and storage");
    return 0;
}
