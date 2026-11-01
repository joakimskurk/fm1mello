/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* P1 regression (review 2026-10, P1-4): sample decoding in the audio callback was unbounded. A loop wrap
 * threw the decoded block away, and high pitch needs a new block every few frames: up to 84 IMA block
 * decodes per 256-frame callback (2.3x the normal render cost). Now a loop keeps its block and each render
 * call decodes at most MELLO_SAMPLE_DECODE_BUDGET blocks. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"

#define FRAMES  256u
#define SAMPLES 4096u /* per slot: 16 IMA blocks */
#define BUDGET  20u   /* MELLO_SAMPLE_DECODE_BUDGET */
static int16_t delay_buf[4096];
static int16_t reverb_buf[24576];
static uint8_t slot_data[MELLO_SAMPLE_SLOT_COUNT]
                        [(SAMPLES / MELLO_SAMPLE_BLOCK_SAMPLES) * MELLO_SAMPLE_BLOCK_BYTES];

typedef struct {
    const char *name;
    int8_t coarse;
    uint8_t note;
    bool loop;
    uint16_t trim_start, trim_end;
} Case;

typedef struct {
    uint32_t max_per_render, total, skips;
    int32_t peak;
} Result;

static Result run(const Case *c)
{
    static MelloAudio a;
    static MelloSampleBank bank;
    static int32_t out[FRAMES * 2u];
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    Result res = {0u, 0u, 0u, 0};
    uint32_t blk, us = 0u, i;
    uint8_t slot;

    mello_sample_bank_init(&bank, slot_data[0], slot_data[1], slot_data[2], slot_data[3],
                           sizeof slot_data[0]);
    for (slot = 0u; slot < MELLO_SAMPLE_SLOT_COUNT; ++slot) {
        for (i = 0u; i < sizeof slot_data[slot]; ++i) /* any IMA bytes decode; the header sets index 40 */
            slot_data[slot][i] =
                (uint8_t)(i % MELLO_SAMPLE_BLOCK_BYTES == 2u ? 40u : (i * 37u + slot) & 0xFFu);
        bank.meta[slot].loaded = true;
        bank.meta[slot].sample_rate = 44100u;
        bank.meta[slot].sample_count = SAMPLES;
        bank.meta[slot].block_count = (uint16_t)(SAMPLES / MELLO_SAMPLE_BLOCK_SAMPLES);
        bank.meta[slot].data_bytes = sizeof slot_data[slot];
    }
    mello_sound_defaults(&s);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sm);
    s.osc_wave = MELLO_WAVE_OFF;
    fx.delay_wet = 0u;
    fx.reverb_wet = 0u;
    sm.on = true;
    for (slot = 0u; slot < MELLO_SAMPLE_SLOT_COUNT; ++slot) {
        sm.slots[slot].coarse_semitones = c->coarse;
        sm.slots[slot].track = true;
        sm.slots[slot].loop = c->loop;
        sm.slots[slot].trim_start = c->trim_start;
        sm.slots[slot].trim_end = c->trim_end;
        sm.slots[slot].level = 1000u;
        sm.slots[slot].sustain_permille = 1000u;
    }
    mello_audio_init(&a, delay_buf, 4096u, reverb_buf, 24576u);
    mello_audio_attach_sample_bank(&a, &bank);
    mello_audio_set_config(&a, &s, &fx, &sub, &sm);
    for (blk = 0u; blk < 200u; ++blk) {
        uint32_t before = a.sample_decodes;
        if (blk == 2u)
            for (slot = 0u; slot < MELLO_SAMPLE_SLOT_COUNT; ++slot) {
                MelloEvent e;
                memset(&e, 0, sizeof e);
                e.type = MELLO_EVENT_SAMPLE_NOTE_ON;
                e.sample_slot = slot;
                e.note = c->note;
                e.velocity = 100u;
                mello_audio_push_engine_event(&a, &e, us);
            }
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, FRAMES, us, 120u);
        us += FRAMES * 1000000u / 44100u;
        if (a.sample_decodes - before > res.max_per_render) res.max_per_render = a.sample_decodes - before;
        for (i = 0u; i < FRAMES * 2u; ++i) {
            int32_t v = out[i] < 0 ? -out[i] : out[i];
            if (v > res.peak) res.peak = v;
        }
    }
    res.total = a.sample_decodes;
    res.skips = a.sample_decode_skips;
    return res;
}

int main(void)
{
    static const Case cases[] = {
        {"unity pitch, no loop", 0, 60u, false, 0u, 1000u},
        {"+24 coarse, note 60 (4x)", 24, 60u, true, 0u, 1000u},
        {"+24 coarse, note 96 (16x, clamped)", 24, 96u, true, 0u, 1000u},
        {"loop inside one block (trim 0..50)", 0, 60u, true, 0u, 50u},
        {"loop across a block edge (trim 60..66)", 0, 60u, true, 60u, 66u},
    };
    Result r[5];
    unsigned i, failed = 0u;
    for (i = 0u; i < 5u; ++i) {
        r[i] = run(&cases[i]);
        printf("%-40s decodes/render max %3u, total %5u, muted frames %6u, peak %5d\n", cases[i].name,
               (unsigned)r[i].max_per_render, (unsigned)r[i].total, (unsigned)r[i].skips, (int)r[i].peak);
        if (r[i].max_per_render > BUDGET) {
            printf("  FAIL: over the decode budget\n");
            failed++;
        }
        if (r[i].peak == 0) {
            printf("  FAIL: no sound\n");
            failed++;
        }
    }
    /* Normal settings must not be touched by the budget. */
    if (r[0].skips || r[1].skips || r[3].skips) {
        printf("FAIL: the budget muted frames at normal settings\n");
        failed++;
    }
    /* A loop that stays inside one block decodes it once per voice, not on every wrap. */
    if (r[3].total > MELLO_SAMPLE_SLOT_COUNT) {
        printf("FAIL: a loop inside one block was decoded %u times\n", (unsigned)r[3].total);
        failed++;
    }
    if (failed) return 1;
    printf("PASS: sample decoding is bounded per render call\n");
    return 0;
}
