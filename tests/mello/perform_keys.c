/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* PERFORM's keys (DECISIONS "PERFORM: performance only"): performance only, no RESTART or transpose.
 * - ARP: every pulse plays, root, third, fifth, octave of the step's note, round; let go, the sequence as it was.
 * - ACCENT: every lead note accented (121, as an accented step) and every drum hit at 127.
 * - The mutes, playing: asked for, they come in on the next beat (a pulse that is a multiple of 4), not at once.
 * - LP SWP / HP SWP: held, the mix's highs (lows) fall away over the sweep (by 10 dB at least).
 * - THROW: with the delay's WET at 0, the music goes into the delay all the same and its echo is heard after;
 *   WASH: so with the reverb. Built with the output's gain at 0 dB (run_mello.sh). */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"
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
static MelloTransport t;
static void sink(void *c, const MelloEvent *ev, uint32_t due)
{
    (void)mello_audio_push_engine_event((MelloAudio *)c, ev, due);
}

/* One pulse: the lead's note (-1 none), its velocity, the drums' first hit's velocity */
static int pulse(int *vel, int *drum_vel)
{
    MelloEvent ev;
    int note = -1;
    *vel = *drum_vel = 0;
    (void)mello_engine_advance_pulse(&e);
    while (mello_event_pop(&e, &ev)) {
        if (ev.type == MELLO_EVENT_NOTE_ON && note < 0) note = ev.note, *vel = ev.velocity;
        if (ev.type == MELLO_EVENT_DRUM_HIT && !*drum_vel) *drum_vel = ev.velocity;
    }
    return note;
}

static void engine_part(void)
{
    MelloProject p;
    MelloPerf pf;
    MelloDrumConfig d;
    int n[8], v, dv, i;
    mello_project_defaults(&p); /* C minor */
    for (i = 0; i < 16; ++i) p.steps[i].pitch = 7, p.steps[i].octave = 0, p.steps[i].accent = false;
    p.steps[2].gate = MELLO_GATE_REST;
    mello_engine_init(&e, &p, 3u);
    d = e.drums;
    d.on = true;
    d.hits[MELLO_DRUM_KICK] = 0xFFFFu, d.accents[MELLO_DRUM_KICK] = 0u;
    mello_engine_drums_set_config(&e, &d);
    mello_engine_start(&e);
    pf = e.perf;
    pf.arp = true;
    mello_engine_set_perf(&e, &pf);
    for (i = 0; i < 8; ++i) n[i] = pulse(&v, &dv);
    for (i = 0; i < 8; ++i) CHECK(n[i] >= 0, "ARP: pulse %d did not play (a rest under it too)", i);
    {
        int r = n[0], ok = 1; /* the pulses' places in the beat: 0, +3, +7, +12 semitones (C minor from the root) */
        static const int want[4] = {0, 3, 7, 12};
        for (i = 0; i < 8; ++i) ok &= n[i] - r == want[i % 4] - want[0];
        CHECK(ok, "ARP did not play root, third, fifth, octave (%d %d %d %d)", n[0], n[1], n[2], n[3]);
    }
    pf.arp = false;
    pf.accent = true;
    mello_engine_set_perf(&e, &pf);
    (void)pulse(&v, &dv);
    if (!v) (void)pulse(&v, &dv);
    CHECK(v == 121 && dv == 127, "ACCENT: the lead at %d (an accent is 121), the kick at %d (127)", v, dv);
    pf.accent = false;
    mello_engine_set_perf(&e, &pf);
    /* a mute asked for mid-beat comes in on the beat */
    while (e.absolute_pulse % 4u != 1u) (void)pulse(&v, &dv);
    e.perf.mute_req = 1u;
    (void)pulse(&v, &dv);
    CHECK(!(e.perf.mix_mute & 1u), "the mute came in off the beat");
    while (e.absolute_pulse % 4u) (void)pulse(&v, &dv);
    CHECK((e.perf.mix_mute & 1u) && !e.perf.mute_req, "the mute did not come in on the beat");
    e.perf.mute_req = 2u; /* asked for, then STOP: it comes in then, not at the next PLAY's first beat */
    mello_engine_stop(&e);
    CHECK((e.perf.mix_mute & 2u) && !e.perf.mute_req, "a mute asked for before STOP was left waiting");
}

/* 3.6 s of the default pattern; fx held from 1 s to 2.6 s (a sweep closes in 1.5 s); energy per window (dB): highs (diff) and all */
static void render(uint8_t fx, bool wet_off, double *hi_held, double *all_after, double *lo_held)
{
    MelloProject p;
    MelloFxConfig f;
    uint32_t ms, i;
    int32_t out[96], prev = 0;
    double hh = 0, aa = 0, ll = 0, lp = 0;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 4u, 0, 0);
    f = t.engine.fx;
    if (wet_off) f.delay_wet = f.reverb_wet = 0u;
    mello_engine_fx_set_config(&t.engine, &f);
    mello_audio_init(&a, db, 65536, rb, 24576);
    mello_audio_set_config(&a, &t.engine.sound, &t.engine.fx, &t.engine.sub, &t.engine.sample);
    mello_transport_set_event_sink(&t, sink, &a);
    mello_transport_start(&t, 0u);
    for (ms = 0; ms < 3600u; ++ms) {
        uint32_t fr = 44u + (ms % 10u == 0u ? 1u : 0u);
        if (ms == 1000u) mello_audio_set_perf_fx(&a, fx);
        if (ms == 2600u) mello_audio_set_perf_fx(&a, 0u), mello_transport_stop(&t), mello_audio_all_notes_off(&a);
        mello_transport_poll_ahead(&t, ms * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, fr, ms * 1000u, t.engine.project.bpm);
        for (i = 0; i < fr; ++i) {
            int32_t x = out[2 * i], d = x - prev;
            prev = x;
            lp += ((double)x - lp) / 64.0; /* below ~110 Hz */
            if (ms >= 2300u && ms < 2600u) ll += lp * lp;
            if (ms >= 2300u && ms < 2600u) hh += (double)d * d; /* the sweep at its end */
            if (ms >= 2900u) aa += (double)x * x;               /* after the music stopped */
        }
    }
    *hi_held = 10 * log10(hh + 1), *all_after = 10 * log10(aa + 1), *lo_held = 10 * log10(ll + 1);
}

static void audio_part(void)
{
    double hi0, hi_lp, hi_hp, after0, after_throw, after_wash, lo0, lo_hp, x;
    render(0u, true, &hi0, &after0, &lo0);
    render(MELLO_PERF_FX_LP, true, &hi_lp, &x, &x);
    render(MELLO_PERF_FX_HP, true, &hi_hp, &x, &lo_hp);
    render(MELLO_PERF_FX_THROW, true, &x, &after_throw, &x);
    render(MELLO_PERF_FX_WASH, true, &x, &after_wash, &x);
    printf("perform_keys: highs %.1f dB, with LP SWP %.1f; lows %.1f, with HP SWP %.1f; after the stop %.1f, THROW %.1f, "
           "WASH %.1f\n",
           hi0, hi_lp, lo0, lo_hp, after0, after_throw, after_wash);
    CHECK(lo_hp < lo0 - 10.0, "HP SWP did not take the lows away (%.1f against %.1f)", lo_hp, lo0);
    (void)hi_hp;
    CHECK(hi_lp < hi0 - 10.0, "LP SWP did not take the highs away (%.1f against %.1f)", hi_lp, hi0);
    CHECK(after_throw > after0 + 20.0, "THROW: no echo after the music stopped (%.1f against %.1f)", after_throw,
          after0);
    CHECK(after_wash > after0 + 20.0, "WASH: no reverb after the music stopped (%.1f against %.1f)", after_wash,
          after0);
}

int main(void)
{
    engine_part();
    audio_part();
    if (fails) return 1;
    puts("PASS: perform_keys");
    return 0;
}
