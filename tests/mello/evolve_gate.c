/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* EVOLVE's GATE is two routes (ROADMAP E7). LENGTH, on by default at 35%, shortens each note by the register, at
 * DEPTH 100% down to a quarter of its length: the same notes as with EVOLVE off, none longer, none new, no rest
 * played. RETRIG, off by default, is the HTML's GATE: a lit bit makes a playing step MULTI, a new attack on every
 * pulse, but never plays a rest. A saved GATE ON is LENGTH. */
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

typedef struct {
    unsigned ons, shorter, longer, under_quarter, rests_played;
    uint32_t base, shortest;
} Run;

/* 16 steps of 2 pulses, SINGLE, every fourth a REST; EVOLVE on with `routes` at `depth`, mutating often. */
static void run(bool tm_on, uint8_t routes, uint16_t depth, Run *r)
{
    MelloProject p;
    MelloTmConfig t;
    MelloEvent ev;
    uint32_t on_tick = 0u;
    unsigned i;
    memset(r, 0, sizeof *r);
    mello_project_defaults(&p);
    p.seq_len = 16u;
    for (i = 0u; i < 16u; ++i) {
        p.steps[i].pitch = (int8_t)(i % 7u);
        p.steps[i].gate = i % 4u == 3u ? MELLO_GATE_REST : MELLO_GATE_SINGLE;
        p.steps[i].pulses = 2u;
        p.steps[i].ratchet = 1u;
        p.steps[i].skip = false;
        p.steps[i].probability = 65535u;
    }
    mello_engine_init(&e, &p, 7u);
    mello_tm_defaults(&t);
    t.on = tm_on;
    t.mutation_prob = 500u;
    t.routes = routes;
    t.depth_gate = depth;
    mello_engine_tm_set_config(&e, &t);
    {
        MelloModRouting m = e.modr;
        m.tm_on_loop = false; /* mutate on every step */
        mello_engine_mod_routing_set(&e, &m);
    }
    mello_engine_start(&e);
    r->base = 0u;
    r->shortest = 0xFFFFFFFFu;
    for (i = 0u; i < 16u * 2u * 8u; ++i) {
        (void)mello_engine_advance_pulse(&e);
        while (mello_event_pop(&e, &ev)) {
            if (ev.type == MELLO_EVENT_NOTE_ON) {
                ++r->ons;
                on_tick = ev.tick;
                if (p.steps[ev.step_index].gate == MELLO_GATE_REST) ++r->rests_played;
            } else if (ev.type == MELLO_EVENT_NOTE_OFF) {
                uint32_t d = ev.tick - on_tick;
                if (d < r->shortest) r->shortest = d;
                if (!r->base) r->base = d;
            }
        }
    }
}

int main(void)
{
    MelloTmConfig t;
    Run off, len, len35, retrig;
    uint32_t base;

    mello_tm_defaults(&t);
    CHECK(t.routes == MELLO_TM_ROUTE_LENGTH, "EVOLVE's default routes are not LENGTH alone (%u)", t.routes);
    CHECK(t.depth_gate == 350u, "EVOLVE's GATE depth is not 35%% (%u)", t.depth_gate);

    run(false, MELLO_TM_ROUTE_LENGTH, 1000u, &off);
    base = off.base;
    CHECK(base > 0u && off.shortest == base, "with EVOLVE off every note is not one length (%u, %u)", base,
          off.shortest);
    CHECK(off.rests_played == 0u, "a REST played with EVOLVE off");

    /* LENGTH at 100%: the same notes, none longer, some much shorter, none under a quarter */
    run(true, MELLO_TM_ROUTE_LENGTH, 1000u, &len);
    CHECK(len.ons == off.ons, "LENGTH changed how many notes play (%u, not %u)", len.ons, off.ons);
    CHECK(len.rests_played == 0u, "LENGTH played a REST");
    CHECK(len.shortest < base * 3u / 4u, "LENGTH at 100%% never shortened a note below 3/4 (%u of %u)",
          len.shortest, base);
    CHECK(len.shortest + 2u >= base / 4u, "LENGTH at 100%% made a note shorter than a quarter (%u of %u)",
          len.shortest, base);

    /* LENGTH at 35% (the default): at most about a quarter shorter */
    run(true, MELLO_TM_ROUTE_LENGTH, 350u, &len35);
    CHECK(len35.ons == off.ons, "LENGTH 35%% changed how many notes play");
    CHECK(len35.shortest < base && len35.shortest * 100u >= base * 72u,
          "LENGTH 35%% is not between 73%% and 100%% (%u of %u)", len35.shortest, base);

    /* RETRIG: extra attacks on lit bits, never on a REST */
    run(true, MELLO_TM_ROUTE_RETRIG, 1000u, &retrig);
    CHECK(retrig.ons > off.ons, "RETRIG never added an attack (%u, %u)", retrig.ons, off.ons);
    CHECK(retrig.rests_played == 0u, "RETRIG played a REST (%u)", retrig.rests_played);

    printf("evolve_gate: %u notes; LENGTH 100%% shortest %u of %u ticks; RETRIG %u attacks\n", off.ons,
           len.shortest, base, retrig.ons);
    if (fails) return 1;
    puts("PASS: evolve_gate");
    return 0;
}
