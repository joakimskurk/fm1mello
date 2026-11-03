/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The performance keys act on every part (ROADMAP E2 / F3, DECISIONS "Performance on every part"). Held REPEAT
 * loops the drums', BASS SEQ's and the chords' last pulses with the lead's; REVERSE plays the drums backwards; let
 * go, every part is where it would have been. FILL: a snare roll on every step, every step of the bass line, the
 * chords struck on every step. */
#include <stdio.h>
#include <string.h>

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

static MelloEngine a, b;

/* Drums: a kick on every step (an event says its step), the snare on none; BASS SEQ: 16 steps, the odd ones on;
 * the chords on, BROKEN. */
static void start(MelloEngine *e)
{
    MelloProject p;
    MelloDrumConfig d;
    MelloBassConfig bs;
    MelloSubConfig s;
    MelloChordConfig c;
    uint8_t i;
    mello_project_defaults(&p);
    mello_engine_init(e, &p, 11u);
    d = e->drums;
    d.on = true;
    d.follow = false;
    d.length = 16u;
    d.hits[MELLO_DRUM_KICK] = 0xFFFFu;
    d.hits[MELLO_DRUM_SNARE] = 0u;
    d.hits[MELLO_DRUM_HAT] = d.hits[MELLO_DRUM_GLITCH] = 0u;
    for (i = 0u; i < MELLO_DRUM_VOICES; ++i) d.voice[i].chance = 1000u;
    mello_engine_drums_set_config(e, &d);
    s = e->sub;
    s.on = true;
    mello_engine_sub_set_config(e, &s);
    bs = e->bass;
    bs.on = true;
    bs.follow = false;
    bs.length = 16u;
    for (i = 0u; i < 16u; ++i) bs.steps[i].on = (i & 1u) != 0u;
    mello_engine_bass_set_config(e, &bs);
    c = e->chord;
    c.on = true;
    c.mode = MELLO_CHORD_BROKEN;
    mello_engine_chord_set_config(e, &c);
    mello_engine_start(e);
}

typedef struct {
    int kick_step, bass_step, chord_note, snares, bass_notes;
} Pulse;

static Pulse pulse(MelloEngine *e)
{
    Pulse r = {-1, -1, -1, 0, 0};
    MelloEvent ev;
    (void)mello_engine_advance_pulse(e);
    while (mello_event_pop(e, &ev)) {
        if (ev.type == MELLO_EVENT_DRUM_HIT && ev.sample_slot == MELLO_DRUM_KICK) r.kick_step = ev.step_index;
        if (ev.type == MELLO_EVENT_DRUM_HIT && ev.sample_slot == MELLO_DRUM_SNARE) r.snares++;
        if (ev.type == MELLO_EVENT_SUB_NOTE_ON) r.bass_step = ev.ratchet_index - 1, r.bass_notes++;
        if (ev.type == MELLO_EVENT_CHORD_NOTE_ON) r.chord_note = ev.note;
    }
    return r;
}

static void set_perf(MelloEngine *e, bool fill, bool reverse, bool repeat, uint8_t len)
{
    MelloPerf p = e->perf;
    p.fill = fill, p.reverse = reverse, p.repeat = repeat, p.repeat_len = len;
    mello_engine_set_perf(e, &p);
}

int main(void)
{
    Pulse ra[12], rb;
    unsigned i;

    /* REPEAT 4 after 20 pulses: the drums' and the bass's last four steps round and round, the chords' four notes */
    start(&a);
    start(&b);
    for (i = 0u; i < 20u; ++i) (void)pulse(&a), (void)pulse(&b);
    set_perf(&a, false, false, true, 4u);
    for (i = 0u; i < 12u; ++i) ra[i] = pulse(&a), (void)pulse(&b);
    for (i = 0u; i < 12u; ++i) {
        int want = (int)(16u + i % 4u) % 16;
        CHECK(ra[i].kick_step == want, "REPEAT 4, pulse %u: the drums' step %d, not %d", i, ra[i].kick_step, want);
        if (ra[i].bass_step >= 0)
            CHECK(ra[i].bass_step == want, "REPEAT 4, pulse %u: the bass's step %d, not %d", i, ra[i].bass_step,
                  want);
        if (i >= 4u)
            CHECK(ra[i].chord_note == ra[i - 4u].chord_note, "REPEAT 4, pulse %u: the chords' note %d, not %d", i,
                  ra[i].chord_note, ra[i - 4u].chord_note);
    }
    /* let go: every part where it would have been */
    set_perf(&a, false, false, false, 4u);
    ra[0] = pulse(&a), rb = pulse(&b);
    CHECK(ra[0].kick_step == rb.kick_step, "after REPEAT: the drums at %d, not %d", ra[0].kick_step, rb.kick_step);
    CHECK(ra[0].bass_step == rb.bass_step, "after REPEAT: the bass at %d, not %d", ra[0].bass_step, rb.bass_step);

    /* REVERSE: the drums backwards from the pulse before */
    start(&a);
    for (i = 0u; i < 20u; ++i) (void)pulse(&a);
    set_perf(&a, false, true, false, 4u);
    for (i = 0u; i < 8u; ++i) ra[i] = pulse(&a);
    for (i = 0u; i < 8u; ++i) {
        int want = (int)((32u + 18u - i) % 16u);
        CHECK(ra[i].kick_step == want, "REVERSE, pulse %u: the drums' step %d, not %d", i, ra[i].kick_step, want);
    }

    /* FILL: a snare roll on every step, every step of the bass line */
    start(&a);
    for (i = 0u; i < 4u; ++i) (void)pulse(&a);
    set_perf(&a, true, false, false, 4u);
    for (i = 0u; i < 8u; ++i) ra[i] = pulse(&a);
    for (i = 0u; i < 8u; ++i) {
        CHECK(ra[i].snares >= 2, "FILL, pulse %u: %d snare hits, not a roll", i, ra[i].snares);
        CHECK(ra[i].bass_notes == 1, "FILL, pulse %u: %d bass notes", i, ra[i].bass_notes);
    }

    if (fails) return 1;
    puts("PASS: perf_parts");
    return 0;
}
