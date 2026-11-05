/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* RANDOM: GROOVE (ROADMAP E10 / F15, DECISIONS "GROOVE"): MUSICAL's phrase with more rhythm. Checked over 300 rolls
 * of 16 steps: the bar stays a bar (a tie's two pulses and the step it skips), a skip only ever follows a tie; 15-25%
 * of the notes ratchet, rolls of three or four into a cell's end in most rolls, ties and ghost notes (chance notes,
 * though the default mask leaves chance alone) in most; an offbeat ratchet is answered by a rest more often than
 * not; the first step plays, the phrase still ends on the root, strong steps are chord tones; GROOVE has more of all
 * of this than MUSICAL, and MUSICAL is as it was (no ties). */
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

static void roll(uint32_t seed, MelloRandomizeIntensity how)
{
    MelloProject p;
    mello_project_defaults(&p); /* C minor, 16 steps, span 2: home is degree 7 */
    p.seq_len = 16u;
    mello_engine_init(&e, &p, seed);
    mello_engine_set_randomize_mask(&e, MELLO_RAND_DEFAULT);
    mello_engine_randomize_steps_intensity(&e, how);
}

typedef struct {
    unsigned notes, ratchets, rolls_in, ties, ghosts, offbeat, answered, with_roll, with_tie, with_ghost;
} Count;

static void count(Count *c)
{
    const MelloStep *s = e.project.steps;
    unsigned i, pulses = 0u, rolls = 0u, ties = 0u, ghosts = 0u;
    int last = -1;
    for (i = 0u; i < 16u; ++i) {
        if (s[i].skip) {
            CHECK(i > 0u && s[i - 1u].pulses == 2u, "a skip at step %u not after a tie", i + 1u);
            continue;
        }
        pulses += s[i].pulses;
        if (s[i].gate == MELLO_GATE_REST) continue;
        last = (int)i;
        c->notes++;
        if (s[i].ratchet > 1u) c->ratchets++;
        if (s[i].ratchet >= 3u) rolls++;
        if (s[i].pulses == 2u) ties++;
        if (s[i].probability < 65535u) ghosts++;
        if ((i & 1u) && s[i].ratchet == 2u && i + 1u < 16u && (i + 1u) % 4u) {
            c->offbeat++;
            if (s[i + 1u].gate == MELLO_GATE_REST) c->answered++;
        }
        if (i % 4u == 0u) {
            unsigned d = (unsigned)s[i].pitch % 7u; /* minor: root 0, third 2, fifth 4 */
            CHECK(d == 0u || d == 2u || d == 4u, "a strong step %u off the chord (degree %u)", i + 1u, d);
        }
    }
    CHECK(pulses == 16u, "the bar is %u pulses, not 16", pulses);
    CHECK(s[0].gate != MELLO_GATE_REST && !s[0].skip, "the first step does not play");
    CHECK(last >= 0 && s[last].pitch % 7 == 0, "the phrase ends on degree %d, not the root",
          s[last].pitch % 7);
    c->rolls_in += rolls, c->ties += ties, c->ghosts += ghosts;
    c->with_roll += rolls > 0u, c->with_tie += ties > 0u, c->with_ghost += ghosts > 0u;
}

int main(void)
{
    Count g, m;
    uint32_t s;
    unsigned share;
    memset(&g, 0, sizeof g);
    memset(&m, 0, sizeof m);
    for (s = 1u; s <= 300u; ++s) {
        roll(s, MELLO_RANDOMIZE_GROOVE);
        count(&g);
        roll(s, MELLO_RANDOMIZE_MUSICAL);
        count(&m);
    }
    share = g.ratchets * 100u / g.notes;
    printf(
        "random_groove: GROOVE %u notes, ratchets %u%%, rolls in %u/300, ties %u/300, ghosts %u/300, offbeat "
        "answered %u/%u; MUSICAL ratchets %u%%, ties %u\n",
        g.notes, share, g.with_roll, g.with_tie, g.with_ghost, g.answered, g.offbeat,
        m.ratchets * 100u / m.notes, m.ties);
    CHECK(share >= 15u && share <= 25u, "GROOVE ratchets %u%% of its notes, not 15-25", share);
    CHECK(g.with_roll >= 180u, "a roll into a cell's end in %u rolls of 300", g.with_roll);
    CHECK(g.with_tie >= 180u, "a tie in %u rolls of 300", g.with_tie);
    CHECK(g.with_ghost >= 180u, "a ghost note in %u rolls of 300 (the default mask leaves chance alone)",
          g.with_ghost);
    CHECK(g.answered * 2u > g.offbeat, "%u of %u offbeat ratchets answered by a rest", g.answered, g.offbeat);
    CHECK(m.ties == 0u && m.with_ghost == 0u, "MUSICAL changed: %u ties, %u rolls with chance notes", m.ties,
          m.with_ghost);
    CHECK(g.ratchets * 100u / g.notes > m.ratchets * 100u / m.notes + 5u,
          "GROOVE is not busier than MUSICAL");
    if (fails) return 1;
    puts("PASS: random_groove");
    return 0;
}
