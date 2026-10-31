/* Segments made easy (feedback 2026-10-30: "the segmenting is confusing and hard to adjust"). A new project has four
 * segments of four that play as one of sixteen; a step key cuts and joins; EVEN spreads them; LENGTH moves a segment's
 * end and the ones after keep theirs; a new sequence length keeps their proportions. */
#include <stdio.h>
#include <string.h>

#include "mello_engine.h"

static int fails;

/* The segments as "4 4 4 4". */
static const char *lengths(const MelloProject *p)
{
    static char out[64];
    uint8_t b[MELLO_MAX_SEGMENTS + 1u], sc = mello_segment_bounds(p, b), i;
    char *q = out;
    for (i = 0u; i < sc; ++i)
        q += snprintf(q, (size_t)(out + sizeof out - q), i ? " %u" : "%u", (unsigned)(b[i + 1u] - b[i]));
    return out;
}

static void expect(const char *what, const MelloProject *p, const char *want)
{
    const char *got = lengths(p);
    printf("%-34s %s\n", what, got);
    if (strcmp(got, want)) {
        fprintf(stderr, "FAIL: %s: %s, not %s\n", what, got, want);
        fails++;
    }
}

/* The steps in the order the engine plays them over one loop. */
static void play_order(MelloProject *p, uint8_t *out, unsigned n)
{
    static MelloEngine e;
    unsigned i;
    mello_engine_init(&e, p, 1u);
    mello_engine_start(&e);
    for (i = 0u; i < n; ++i) {
        out[i] = e.step_index;
        (void)mello_engine_advance_pulse(&e);
    }
}

int main(void)
{
    MelloProject p, one;
    uint8_t a[32], b[32];
    int8_t r;
    unsigned i;

    mello_project_defaults(&p);
    expect("default", &p, "4 4 4 4");
    one = p; /* plays as one segment of sixteen */
    one.seg_count = 1u;
    play_order(&p, a, 32u);
    play_order(&one, b, 32u);
    if (memcmp(a, b, sizeof a)) {
        fprintf(stderr, "FAIL: four forward segments do not play as one\n");
        fails++;
    }

    r = mello_segment_toggle_start(&p, 2u); /* cut A at step 3 */
    expect("cut at step 3", &p, "2 2 4 4 4");
    if (r != 1 || p.seg_repeats[1] != p.seg_repeats[0]) fails++, fprintf(stderr, "FAIL: the cut picks B\n");
    r = mello_segment_toggle_start(&p, 2u); /* and join it again */
    expect("join at step 3", &p, "4 4 4 4");
    if (r != 0) fails++, fprintf(stderr, "FAIL: the join picks A\n");

    p.seg_repeats[1] = 3u, p.seg_mode[2] = MELLO_SEG_PINGPONG; /* settings move with their segments */
    (void)mello_segment_toggle_start(&p, 4u);                  /* B joins A: C, D move up */
    expect("join B into A", &p, "8 4 4");
    if (p.seg_repeats[0] != 1u || p.seg_mode[1] != MELLO_SEG_PINGPONG)
        fails++, fprintf(stderr, "FAIL: the settings did not follow their segments\n");
    (void)mello_segment_toggle_start(&p, 0u); /* step 1 never cuts */
    expect("step 1", &p, "8 4 4");
    for (i = 1u; i < 6u; ++i) (void)mello_segment_toggle_start(&p, (uint8_t)i);
    expect("six at most", &p, "1 1 1 5 4 4");

    mello_segment_even(&p);
    expect("EVEN (6 over 16)", &p, "3 2 3 3 2 3");
    p.seg_count = 3u;
    mello_segment_even(&p);
    expect("EVEN (3 over 16)", &p, "5 6 5");

    mello_segment_set_length(&p, 0u, 3u);
    expect("LENGTH A 3", &p, "3 6 7");
    mello_segment_set_length(&p, 1u, 4u);
    expect("LENGTH B 4", &p, "3 4 9");
    mello_segment_set_length(&p, 0u, 12u); /* the last ones give way */
    expect("LENGTH A 12", &p, "12 3 1");
    mello_segment_set_length(&p, 0u, 16u);
    expect("LENGTH A 16 (B, C keep a step)", &p, "14 1 1");
    mello_segment_set_length(&p, 2u, 5u); /* the last: its start moves */
    expect("LENGTH C 5", &p, "14 1 1");
    mello_segment_set_length(&p, 1u, 1u);
    mello_segment_set_length(&p, 0u, 4u);
    expect("LENGTH A 4", &p, "4 1 11");
    mello_segment_set_length(&p, 2u, 5u);
    expect("LENGTH C 5", &p, "4 7 5");

    mello_project_defaults(&p);
    p.seq_len = 8u;
    mello_segment_rescale(&p, 16u);
    expect("16 -> 8 steps", &p, "2 2 2 2");
    p.seq_len = 32u;
    mello_segment_rescale(&p, 8u);
    expect("8 -> 32 steps", &p, "8 8 8 8");
    p.seq_len = 3u;
    mello_segment_rescale(&p, 32u);
    expect("32 -> 3 steps", &p, "1 1 1");

    if (fails) return 1;
    puts("PASS: segments cut, join, spread, lengthen and rescale as the SEGMENT page shows them");
    return 0;
}
