/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* SUB MELLO (ROADMAP E6, DECISIONS "SUB MELLO"): a new project's chord part sounds as one voice (POLY off; POLY on
 * gives the four voices back, a STAB's tones together). In every mode no two of its
 * notes start together; BROKEN, the default, plays a triad low, high, middle, high; WALK has the chord's tones on the
 * beats and scale steps between; every note is in the key now; the audio never holds more than its two voices. */
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"
#include "mello_engine.h"
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

static MelloEngine e;
static int16_t db[65536], rb[24576];
static MelloAudio a;
static MelloTransport tr;
static void sink(void *ctx, const MelloEvent *ev, uint32_t due)
{
    (void)mello_audio_push_engine_event((MelloAudio *)ctx, ev, due);
}

static bool in_key(uint8_t note)
{
    uint8_t kr, n, i, rel;
    MelloScale ks;
    const int8_t *t;
    mello_engine_key_now(&e, &kr, &ks);
    n = mello_scale_tones(ks, &t);
    rel = (uint8_t)(((int32_t)note - (int32_t)kr + 120) % 12);
    for (i = 0u; i < n; ++i)
        if (t[i] == (int8_t)rel) return true;
    return false;
}

static void start(MelloChordMode mode)
{
    MelloProject p;
    MelloChordConfig c;
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 5u);
    c = e.chord;
    c.on = true;
    c.mode = mode;
    c.voices = 3u;
    mello_engine_chord_set_config(&e, &c);
    mello_engine_start(&e);
}

int main(void)
{
    static const MelloChordMode modes[] = {MELLO_CHORD_STAB,     MELLO_CHORD_HOLD,   MELLO_CHORD_ARP_UP,
                                           MELLO_CHORD_ARP_DOWN, MELLO_CHORD_ARP_UPDOWN, MELLO_CHORD_PATTERN,
                                           MELLO_CHORD_BROKEN,   MELLO_CHORD_WALK};
    MelloChordConfig d;
    MelloEvent ev;
    unsigned m, i;

    mello_chord_defaults(&d);
    CHECK(d.mode == MELLO_CHORD_BROKEN, "SUB MELLO's default mode is not BROKEN (%d)", (int)d.mode);

    /* every mode: one note at a time, every note in the key */
    for (m = 0u; m < sizeof modes / sizeof modes[0]; ++m) {
        unsigned ons = 0u, together = 0u, off_key = 0u;
        uint32_t last_on = 0xFFFFFFFFu;
        start(modes[m]);
        for (i = 0u; i < 256u; ++i) {
            (void)mello_engine_advance_pulse(&e);
            while (mello_event_pop(&e, &ev)) {
                if (ev.type != MELLO_EVENT_CHORD_NOTE_ON) continue;
                ++ons;
                if (ev.tick == last_on) ++together;
                last_on = ev.tick;
                if (!in_key(ev.note)) ++off_key;
            }
        }
        CHECK(ons > 4u, "mode %u: only %u notes", m, ons);
        CHECK(together == 0u, "mode %u: %u notes started with another", m, together);
        CHECK(off_key == 0u, "mode %u: %u notes off the key", m, off_key);
    }

    /* BROKEN: low, high, middle, high of the triad sounding */
    {
        uint8_t got[8], k = 0u, chord[4], n;
        start(MELLO_CHORD_BROKEN);
        for (i = 0u; i < 64u && k < 8u; ++i) {
            (void)mello_engine_advance_pulse(&e);
            while (mello_event_pop(&e, &ev))
                if (ev.type == MELLO_EVENT_CHORD_NOTE_ON && k < 8u) got[k++] = ev.note;
        }
        n = mello_engine_resolve_chord_notes(&e, e.chord_voiced_slot, chord);
        CHECK(k >= 4u && n == 3u, "BROKEN: %u notes, a chord of %u", k, n);
        if (k >= 4u && n == 3u) {
            uint8_t lo = chord[0], hi = chord[0], mid;
            for (i = 1u; i < 3u; ++i) {
                if (chord[i] < lo) lo = chord[i];
                if (chord[i] > hi) hi = chord[i];
            }
            mid = (uint8_t)(chord[0] + chord[1] + chord[2] - lo - hi);
            CHECK(got[0] == chord[0] && got[1] == chord[2] && got[2] == chord[1] && got[3] == chord[2],
                  "BROKEN: %u %u %u %u, not the voiced chord's 1st, 3rd, 2nd, 3rd (%u %u %u; %u..%u..%u)", got[0],
                  got[1], got[2], got[3], chord[0], chord[1], chord[2], lo, mid, hi);
        }
    }

    /* WALK: on every fourth note a chord tone, between them a step of the scale (at most a whole tone) */
    {
        unsigned k = 0u, beats_off = 0u, leaps = 0u;
        uint8_t prev = 0u;
        start(MELLO_CHORD_WALK);
        for (i = 0u; i < 256u; ++i) {
            (void)mello_engine_advance_pulse(&e);
            while (mello_event_pop(&e, &ev)) {
                uint8_t chord[4], n, j;
                bool tone = false;
                if (ev.type != MELLO_EVENT_CHORD_NOTE_ON) continue;
                n = mello_engine_resolve_chord_notes(&e, e.chord_voiced_slot, chord);
                for (j = 0u; j < n; ++j)
                    if ((chord[j] - ev.note + 120) % 12 == 0) tone = true;
                if (e.chord_arp_pos % 4u == 1u && !tone) ++beats_off; /* arp_pos counted on past this note */
                if (prev && e.chord_arp_pos % 4u != 1u && (ev.note > prev + 2 || prev > ev.note + 2)) ++leaps;
                prev = ev.note;
                ++k;
            }
        }
        CHECK(k > 32u, "WALK: only %u notes", k);
        CHECK(beats_off == 0u, "WALK: %u beats not on a chord tone", beats_off);
        CHECK(leaps == 0u, "WALK: %u steps between the beats were leaps", leaps);
    }

    /* POLY on: the four voices again, a STAB's tones together (a new project is SUB MELLO) */
    {
        unsigned together = 0u;
        uint32_t last_on = 0xFFFFFFFFu;
        MelloChordConfig c;
        start(MELLO_CHORD_STAB);
        CHECK(!e.chord.poly, "a new project's chords are not SUB MELLO");
        c = e.chord;
        c.poly = true;
        c.strum_ms = 0u;
        mello_engine_chord_set_config(&e, &c);
        for (i = 0u; i < 64u; ++i) {
            (void)mello_engine_advance_pulse(&e);
            while (mello_event_pop(&e, &ev))
                if (ev.type == MELLO_EVENT_CHORD_NOTE_ON) {
                    if (ev.tick == last_on) ++together;
                    last_on = ev.tick;
                }
        }
        CHECK(together > 4u, "POLY: a STAB's tones did not start together (%u)", together);
    }

    /* the audio, through the transport: never more than its two voices, and sound */
    {
        MelloProject p;
        int32_t out[96];
        uint32_t ms;
        int peak = 0;
        mello_project_defaults(&p);
        mello_transport_init(&tr, &p, 1u, 0, 0);
        tr.engine.chord.on = true;
        mello_audio_init(&a, db, 65536, rb, 24576);
        mello_audio_set_config(&a, &tr.engine.sound, &tr.engine.fx, &tr.engine.sub, &tr.engine.sample);
        mello_audio_set_chord_config(&a, &tr.engine.chord);
        mello_transport_set_event_sink(&tr, sink, &a);
        mello_transport_start(&tr, 0u);
        CHECK(MELLO_SUB_MELLO_VOICES == 2u, "SUB MELLO takes %u voices", MELLO_SUB_MELLO_VOICES);
        for (ms = 0u; ms < 4000u; ++ms) {
            uint32_t j, fr = 44u + (ms % 10u == 0u ? 1u : 0u);
            mello_transport_poll_ahead(&tr, ms * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
            memset(out, 0, sizeof out);
            mello_audio_render(&a, out, fr, ms * 1000u, tr.engine.project.bpm);
            for (j = 0u; j < fr * 2u; ++j)
                if (out[j] > peak) peak = out[j];
            if (a.chord.active_count > 2u) {
                CHECK(0, "the audio holds %u chord voices", a.chord.active_count);
                break;
            }
        }
        CHECK(peak > 200, "SUB MELLO made no sound (peak %d)", peak);
    }

    if (fails) return 1;
    puts("PASS: sub_mello");
    return 0;
}
