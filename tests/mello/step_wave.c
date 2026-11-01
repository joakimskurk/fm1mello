/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* A wave per step (feedback 2026-10-30: "WAVE on in the randomizer should give a wave PER STEP, not OFF"). Checked: a
 * step's wave rides its NOTE_ON (0 for a step that has none); the lead plays it in place of the sound's WAVE (a sine
 * step has far less high end than the sound's saw); the sound's WAVE OFF still silences the lead; the compact slot format
 * keeps it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mello_audio.h"

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

/* One lead note at A3 with this step wave over the sound's SAW (or OFF): its high end (the energy of the second
 * difference, x[n] - 2x[n-1] + x[n-2], over that of the signal, x 100000) and its peak. */
static void lead_note(uint8_t wave, MelloWaveform sound_wave, uint64_t *rough, int32_t *peak)
{
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    MelloEvent ev;
    int32_t out[2 * 441], x1 = 0, x2 = 0;
    uint64_t hf = 0u, all = 1u;
    unsigned blk, k;
    mello_sound_defaults(&s);
    s.osc_wave = sound_wave;
    s.filter_cut = 1000u;
    s.follower_sub_on = false;
    mello_fx_defaults(&fx);
    fx.delay_wet = fx.reverb_wet = 0u;
    mello_sub_defaults(&sub);
    sub.on = false;
    mello_sample_defaults(&sm);
    mello_audio_init(&a, db, 4096u, rb, 24576u);
    mello_audio_set_config(&a, &s, &fx, &sub, &sm);
    memset(&ev, 0, sizeof ev);
    ev.type = MELLO_EVENT_NOTE_ON;
    ev.note = 57u;
    ev.velocity = 100u;
    ev.wave = wave;
    (void)mello_audio_push_engine_event(&a, &ev, 0u);
    *rough = 0u, *peak = 0;
    for (blk = 0u; blk < 5u; ++blk) {
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, 441u, blk * 10000u, 120u);
        for (k = 0u; k < 441u; ++k) {
            int32_t x = out[2u * k], d2 = x - 2 * x1 + x2;
            hf += (uint64_t)((int64_t)d2 * d2);
            all += (uint64_t)((int64_t)x * x);
            x2 = x1, x1 = x;
            if (abs(x) > *peak) *peak = abs(x);
        }
    }
    *rough = hf * 100000u / all;
}

int main(void)
{
    MelloProject p;
    MelloStoredProject sp;
    MelloEvent ev;
    uint64_t saw, sine;
    int32_t pk_saw, pk_sine, pk_off;
    unsigned i, seen = 0u, right = 0u;

    mello_project_defaults(&p);
    for (i = 0u; i < 16u; ++i) p.steps[i].gate = MELLO_GATE_SINGLE, p.steps[i].wave = (uint8_t)(i % 6u);
    mello_engine_init(&e, &p, 1u);
    mello_engine_start(&e);
    for (i = 0u; i < 16u; ++i) {
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev))
            if (ev.type == MELLO_EVENT_NOTE_ON) seen++, right += ev.wave == ev.step_index % 6u;
    }
    printf("16 steps: %u NOTE_ONs, %u with their step's wave\n", seen, right);
    CHECK(seen == 16u && right == 16u, "%u of %u NOTE_ONs carried their step's wave", right, seen);

    lead_note(0u, MELLO_WAVE_SAW, &saw, &pk_saw);
    lead_note((uint8_t)MELLO_WAVE_SINE + 1u, MELLO_WAVE_SAW, &sine, &pk_sine);
    lead_note((uint8_t)MELLO_WAVE_SINE + 1u, MELLO_WAVE_OFF, &saw /* unused */, &pk_off);
    lead_note(0u, MELLO_WAVE_SAW, &saw, &pk_saw);
    printf(
        "lead: high end of the sound's saw %llu (peak %d), of a SIN step %llu (peak %d); WAVE OFF: peak %d\n",
        (unsigned long long)saw, (int)pk_saw, (unsigned long long)sine, (int)pk_sine, (int)pk_off);
    CHECK(pk_sine > 500 && sine * 3u < saw, "a SIN step had not less high end than the saw (%llu vs %llu)",
          (unsigned long long)sine, (unsigned long long)saw);
    CHECK(pk_off == 0, "WAVE OFF did not silence a step with its own wave (peak %d)", (int)pk_off);

    for (i = 0u; i < MELLO_MAX_STEPS; ++i) p.steps[i].wave = (uint8_t)((i * 5u) % 6u);
    mello_stored_project_encode(&sp, &p);
    {
        MelloProject q;
        mello_project_defaults(&q);
        mello_stored_project_decode(&q, &sp);
        for (i = 0u; i < MELLO_MAX_STEPS; ++i)
            CHECK(q.steps[i].wave == p.steps[i].wave && q.steps[i].sample_slot == p.steps[i].sample_slot &&
                      q.steps[i].sub == p.steps[i].sub,
                  "step %u: wave %u, not %u, after the slot format", i, q.steps[i].wave, p.steps[i].wave);
    }
    puts("the slot format keeps each step's wave");

    if (fails) return 1;
    puts("PASS: a wave per step, from the step to the lead, and kept");
    return 0;
}
