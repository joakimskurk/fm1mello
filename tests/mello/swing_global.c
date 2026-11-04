/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* SWING for everything (feedback 2026-10-31: "I'm afraid the drums' swing clashes with the main sequence"). Checked:
 * at SWING 0 nothing moves; at 500 every lead, sub and drum event of an odd sixteenth comes a quarter of a step late,
 * the even ones stay, and so the drums and the sequence still land together; no NOTE_OFF comes before its NOTE_ON;
 * the loop's wrap stays on the grid. */
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

static MelloEngine e;

/* Each pulse's first lead NOTE_ON and first drum hit, as ticks past the pulse's start (-1 none); and the wraps' ticks
 * past the grid. */
static void run(uint16_t swing, int16_t lead_part, int16_t drum_part, int32_t lead[32], int32_t drum[32],
                int32_t *wrap_off, int *off_before_on)
{
    MelloProject p;
    MelloDrumConfig d;
    uint32_t pt, i, on_tick[128] = {0};
    mello_project_defaults(&p);
    for (i = 0u; i < 16u; ++i)
        p.steps[i].gate = MELLO_GATE_SINGLE, p.steps[i].sub = true, p.steps[i].skip = false;
    mello_engine_init(&e, &p, 3u);
    mello_drum_defaults(&d);
    d.on = true;
    d.swing = swing;
    d.swing_part[MELLO_SWING_LEAD] = lead_part, d.swing_part[MELLO_SWING_DRUMS] = drum_part;
    d.hits[MELLO_DRUM_HAT] = 0xFFFFu;
    for (i = 0u; i < MELLO_DRUM_STEPS; ++i) d.lock[MELLO_DRUM_HAT][i].ratchet = 1u;
    mello_engine_drums_set_config(&e, &d);
    mello_engine_start(&e);
    pt = mello_pulse_ticks(&e.project);
    *wrap_off = 0, *off_before_on = 0;
    for (i = 0u; i < 32u; ++i) {
        MelloEvent ev;
        uint32_t start = e.now_tick;
        lead[i] = drum[i] = -1;
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev)) {
            int32_t at = (int32_t)(ev.tick - start);
            if (ev.type == MELLO_EVENT_NOTE_ON && lead[i] < 0) lead[i] = at, on_tick[ev.note] = ev.tick;
            if (ev.type == MELLO_EVENT_NOTE_OFF && (int32_t)(ev.tick - on_tick[ev.note]) < 0)
                (*off_before_on)++;
            if (ev.type == MELLO_EVENT_DRUM_HIT && ev.sample_slot == MELLO_DRUM_HAT && drum[i] < 0)
                drum[i] = at;
            if (ev.type == MELLO_EVENT_PATTERN_WRAP) *wrap_off += (int32_t)(ev.tick % pt);
        }
    }
}

int main(void)
{
    int32_t l0[32], d0[32], l5[32], d5[32], w0, w5;
    int ob0, ob5;
    uint32_t pt, i;
    run(0u, -1, -1, l0, d0, &w0, &ob0);
    run(500u, -1, -1, l5, d5, &w5, &ob5);
    pt = mello_pulse_ticks(&e.project);
    for (i = 0u; i < 32u; ++i) {
        int32_t want = (i & 1u) ? (int32_t)(pt / 4u) : 0;
        CHECK(l0[i] == 0 && d0[i] == 0, "SWING 0, pulse %u: lead at %d, hat at %d", i, l0[i], d0[i]);
        if (l5[i] >= 0) CHECK(l5[i] == want, "SWING 50%%, pulse %u: lead at %d, not %d", i, l5[i], want);
        CHECK(d5[i] == want, "SWING 50%%, pulse %u: hat at %d, not %d", i, d5[i], want);
    }
    CHECK(!ob0 && !ob5, "a NOTE_OFF before its NOTE_ON (%d, %d)", ob0, ob5);
    { /* per part (DECISIONS "Swing per part"): the lead straight under a swung global; the drums their own */
        int32_t lp[32], dp[32], wp;
        int obp;
        run(500u, 0, -1, lp, dp, &wp, &obp);
        for (i = 0u; i < 32u; ++i) {
            if (lp[i] >= 0) CHECK(lp[i] == 0, "LEAD 0%% under SWING 50%%, pulse %u: lead at %d", i, lp[i]);
            CHECK(dp[i] == ((i & 1u) ? (int32_t)(pt / 4u) : 0), "DRUMS GLOBAL, pulse %u: hat at %d", i, dp[i]);
        }
        run(0u, -1, 1000, lp, dp, &wp, &obp);
        for (i = 0u; i < 32u; ++i) {
            if (lp[i] >= 0) CHECK(lp[i] == 0, "LEAD GLOBAL 0%%, pulse %u: lead at %d", i, lp[i]);
            CHECK(dp[i] == ((i & 1u) ? (int32_t)(pt / 2u) : 0), "DRUMS 100%%, pulse %u: hat at %d", i, dp[i]);
        }
        CHECK(!obp, "per part: a NOTE_OFF before its NOTE_ON");
    }
    CHECK(!w0 && !w5, "the wrap left the grid (%d, %d ticks)", w0, w5);
    printf("SWING 50%%: the odd sixteenths' lead and hat %u ticks late of %u, the even ones on the grid\n",
           pt / 4u, pt);
    if (fails) return 1;
    puts("PASS: SWING moves the sequence and the drums together");
    return 0;
}
