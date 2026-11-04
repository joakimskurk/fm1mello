/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The chord voice (feedback 2026-10-30: "the chords should sound as good as the rest, and I want to see them move").
 * Checked: the chain's names in the key; voice leading moves the voices less than the chords as written and stays
 * near them; STRUM spreads a chord's NOTE_ONs and never puts a NOTE_OFF before its NOTE_ON; the voice at its
 * extremes (CUT, RES, ENV, CHORUS all at 100%) stays in range under UBSan; CHORUS is what makes it stereo. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mello_audio.h"
#include "mello_engine.h"

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

static void setup(MelloChordMode mode, bool lead, uint8_t strum)
{
    MelloProject p;
    MelloChordConfig c;
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 1u);
    mello_chord_defaults(&c);
    c.on = true;
    c.poly = true; /* the four voices (a new project's are SUB MELLO) */
    c.mode = mode;
    c.voices = 4u;
    c.voice_lead = lead;
    c.strum_ms = strum;
    c.length = 4u; /* i VI iv v (a new project's chain is one tonic chord since FIT) */
    for (unsigned i = 0u; i < 4u; ++i)
        c.steps[i].hold = 1u, c.steps[i].shape = MELLO_CHORD_SEVENTH,
        c.steps[i].degree = (uint8_t)(i == 1u ? 5u : i == 2u ? 3u : i == 3u ? 4u : 0u);
    mello_engine_chord_set_config(&e, &c);
    mello_engine_start(&e);
}

/* Plays the chain twice: the voices' movement from chord to chord (sum of |steps|), and how far the voicings' middle
 * strays from the written chords' (largest). */
static void movement(bool lead, int *moved, int *strayed)
{
    uint8_t prev[4] = {0}, written[4];
    int pn = 0, i, k;
    setup(MELLO_CHORD_HOLD, lead, 0u);
    *moved = 0, *strayed = 0;
    for (i = 0; i < 8 * 16; ++i) {
        uint8_t slot = e.chord_voiced_slot;
        MelloEvent ev;
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev)) {
        }
        if (e.chord_voiced_slot != slot && e.chord_voiced_n == 4u) {
            int sum = 0, wsum = 0, n = mello_engine_resolve_chord_notes(&e, e.chord_voiced_slot, written);
            for (k = 0; k < 4; ++k) sum += e.chord_voiced[k];
            for (k = 0; k < n; ++k) wsum += written[k];
            if (abs(sum / 4 - wsum / n) > *strayed) *strayed = abs(sum / 4 - wsum / n);
            if (pn)
                for (k = 0; k < 4; ++k) *moved += abs((int)e.chord_voiced[k] - (int)prev[k]);
            memcpy(prev, e.chord_voiced, 4u);
            pn = 4;
        }
    }
}

int main(void)
{
    char name[8];
    static const char *want3[4] = {"Cm", "G#", "Fm", "Gm"}, *want4[4] = {"Cm7", "G#M7", "Fm7", "Gm7"};
    unsigned i;
    int m_off, m_on, s_off, s_on;

    setup(MELLO_CHORD_STAB, false, 0u); /* the default chain in C minor */
    for (i = 0u; i < 4u; ++i) {
        mello_engine_chord_name(&e, (uint8_t)i, name);
        CHECK(!strcmp(name, want4[i]), "chord %u is %s, not %s", i + 1u, name, want4[i]);
    }
    e.chord.voices = 3u;
    for (i = 0u; i < 4u; ++i) {
        mello_engine_chord_name(&e, (uint8_t)i, name);
        CHECK(!strcmp(name, want3[i]), "chord %u (3 voices) is %s, not %s", i + 1u, name, want3[i]);
    }
    puts("names: Cm7 G#M7 Fm7 Gm7, and with three voices Cm G# Fm Gm");

    movement(false, &m_off, &s_off);
    movement(true, &m_on, &s_on);
    printf("voice leading: the voices move %d semitones over the chain (%d as written), stray %d\n", m_on,
           m_off, s_on);
    CHECK(m_on < m_off, "voice leading did not move the voices less (%d, written %d)", m_on, m_off);
    CHECK(s_on <= 7, "voice leading strayed %d semitones from the written chords", s_on);

    for (i = 0u; i < 2u; ++i) { /* STRUM: HOLD, then STAB */
        uint32_t on_tick[4] = {0}, first = 0u;
        int ons = 0, k;
        MelloEvent ev;
        setup(i ? MELLO_CHORD_STAB : MELLO_CHORD_HOLD, false, 20u);
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev)) {
            if (ev.type == MELLO_EVENT_CHORD_NOTE_ON && ev.ratchet_index < 4u) {
                on_tick[ev.ratchet_index] = ev.tick;
                if (!ons++) first = ev.tick;
            } else if (ev.type == MELLO_EVENT_CHORD_NOTE_OFF && ev.ratchet_index < 4u)
                CHECK(ev.tick > on_tick[ev.ratchet_index], "%s: voice %u's OFF before its ON",
                      i ? "STAB" : "HOLD", (unsigned)ev.ratchet_index);
        }
        CHECK(ons == 4, "%s: %d NOTE_ONs", i ? "STAB" : "HOLD", ons);
        for (k = 1; k < 4; ++k)
            CHECK(on_tick[k] > on_tick[k - 1], "%s: voice %d not after voice %d", i ? "STAB" : "HOLD", k,
                  k - 1);
        printf("STRUM 20 ms (%s): the voices at +0 / +%u / +%u / +%u ticks\n", i ? "STAB" : "HOLD",
               (unsigned)(on_tick[1] - first), (unsigned)(on_tick[2] - first),
               (unsigned)(on_tick[3] - first));
    }

    for (i = 0u; i < 2u; ++i) { /* the voice at its extremes; CHORUS off is mono */
        static const uint8_t notes[4] = {45, 52, 60, 67};
        MelloSoundConfig s;
        MelloFxConfig fx;
        MelloSubConfig sub;
        MelloSampleConfig sm;
        MelloChordConfig c;
        int32_t out[512];
        int32_t peak = 0, side = 0;
        uint32_t blk, us = 0u, k;
        mello_sound_defaults(&s);
        mello_fx_defaults(&fx);
        fx.delay_wet = fx.reverb_wet = 0u;
        mello_sub_defaults(&sub);
        mello_sample_defaults(&sm);
        mello_chord_defaults(&c);
        c.on = true;
        c.poly = true;
        c.voices = 4u;
        c.level = 1000u;
        c.cut = c.res = c.fenv = 1000u;
        c.chorus = i ? 0u : 1000u;
        c.wave = MELLO_WAVE_SQUARE;
        mello_audio_init(&a, db, 4096u, rb, 24576u);
        mello_audio_set_config(&a, &s, &fx, &sub, &sm);
        mello_audio_set_chord_config(&a, &c);
        for (blk = 0u; blk < 2u * 44100u / 256u; ++blk) {
            if (blk % 40u == 0u)
                for (k = 0u; k < 4u; ++k) {
                    MelloEvent ev;
                    memset(&ev, 0, sizeof ev);
                    ev.type = blk % 80u ? MELLO_EVENT_CHORD_NOTE_OFF : MELLO_EVENT_CHORD_NOTE_ON;
                    ev.note = notes[k];
                    ev.velocity = 88u;
                    (void)mello_audio_push_engine_event(&a, &ev, us);
                }
            memset(out, 0, sizeof out);
            mello_audio_render(&a, out, 256u, us, 120u);
            for (k = 0u; k < 512u; ++k) peak = abs(out[k]) > peak ? abs(out[k]) : peak;
            for (k = 0u; k < 256u; ++k)
                side =
                    abs(out[2u * k] - out[2u * k + 1u]) > side ? abs(out[2u * k] - out[2u * k + 1u]) : side;
            us += 256u * 1000000u / 44100u;
        }
        printf("CHORUS %s, CUT RES ENV 100%%: peak %d, left - right up to %d\n", i ? "off" : "100%",
               (int)peak, (int)side);
        CHECK(peak > 1000 && peak <= 32767, "the chord voice's peak %d", (int)peak);
        CHECK(i ? side == 0 : side > 500, "CHORUS %s: left - right %d", i ? "off" : "on", (int)side);
    }

    if (fails) return 1;
    puts("PASS: chord names, voice leading, STRUM and the chord voice");
    return 0;
}
