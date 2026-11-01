/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
#include "mello_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    MelloUiPage page;
    uint8_t layers;
} LayerCase;

#define L(n, p, c) {n, p, c}
static const LayerCase cases[] = {
    L("HOME", MELLO_UI_HOME, 1u),
    L("STEP", MELLO_UI_SEQ_STEP, 4u),
    L("SEGMENT", MELLO_UI_SEQ_SEGMENT, 1u),
    L("LOOP", MELLO_UI_SEQ_LOOP, 3u),
    L("SLOTSEQ", MELLO_UI_SEQ_SLOTSEQ, 1u),
    L("RANDOM", MELLO_UI_RANDOM, 1u),
    L("KEY/SCALE", MELLO_UI_KEY_SCALE, 1u),
    L("SLOTS", MELLO_UI_SLOTS, 2u),
    L("DOT", MELLO_UI_DOT, 4u),
    L("TURING", MELLO_UI_TURING, 3u),
    L("SUB", MELLO_UI_SUB, 5u),
#if MELLO_FEATURE_CHORD_MACHINE
    L("CHORD", MELLO_UI_CHORD, 5u),
#endif
    L("GLOBAL", MELLO_UI_GLOBAL, 2u),
    L("SOUND", MELLO_UI_SOUND_RESERVED, 5u),
    L("AUDIO FX", MELLO_UI_AUDIO_FX, 5u),
    L("SAMPLE", MELLO_UI_SAMPLE_RESERVED, 4u),
};

static void fail(const char *where, const char *what)
{
    fprintf(stderr, "FAIL: %s: %s\n", where, what);
    exit(1);
}

static void setup(MelloProject *p, MelloTransport *t, MelloStorage *st, MelloUi *u, MelloUiPage page)
{
    mello_project_defaults(p);
    mello_transport_init(t, p, 123u, 0, 0);
    mello_storage_init(st, 0, 0);
    mello_ui_init(u, t, st);
    mello_ui_set_page(u, page);
}

static void turn(MelloUi *u, uint8_t enc_id, int16_t delta)
{
    int16_t e[MELLO_UI_ENCODER_COUNT] = {0};
    e[enc_id] = delta;
    mello_ui_input(u, 100000u, 100u, 0u, 0u, 0u, 0u, e);
}

static uint8_t layer(const MelloUi *u, MelloUiPage p)
{
    switch (p) {
    case MELLO_UI_SEQ_STEP: return u->step_subpage;
    case MELLO_UI_SEQ_SEGMENT: return u->segment_subpage;
    case MELLO_UI_SEQ_LOOP: return u->loop_subpage;
    case MELLO_UI_SLOTS: return u->slots_subpage;
    case MELLO_UI_DOT: return u->selected_dot_target;
    case MELLO_UI_TURING: return u->tm_subpage;
    case MELLO_UI_SUB: return u->sub_subpage;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: return u->chord_subpage;
#endif
    case MELLO_UI_GLOBAL: return u->global_subpage;
    case MELLO_UI_SOUND_RESERVED: return u->sound_subpage;
    case MELLO_UI_AUDIO_FX: return u->fx_subpage;
    case MELLO_UI_SAMPLE_RESERVED: return u->sample_subpage;
    default: return 0u;
    }
}

/* R1 (2026-10-31): SELECT is the layer, ALGORITHM the page of the section. Where ALGORITHM +1 goes from each page,
 * or stays (a section of one page, and HOME, where it is RANDOM's intensity). */
static MelloUiPage section_next_page(MelloUiPage p)
{
    switch (p) {
    case MELLO_UI_SEQ_STEP: return MELLO_UI_SEQ_SEGMENT;
    case MELLO_UI_SEQ_SEGMENT: return MELLO_UI_SEQ_LOOP;
    case MELLO_UI_SEQ_LOOP: return MELLO_UI_SEQ_SLOTSEQ;
    case MELLO_UI_SEQ_SLOTSEQ: return MELLO_UI_SEQ_STEP;
    case MELLO_UI_DOT: return MELLO_UI_TURING;
    case MELLO_UI_TURING: return MELLO_UI_SOUND_RESERVED; /* the LFO page */
    case MELLO_UI_SUB: return MELLO_UI_DRUMS;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: return MELLO_UI_SUB;
    case MELLO_UI_DRUMS: return MELLO_UI_CHORD;
#else
    case MELLO_UI_DRUMS: return MELLO_UI_SUB;
#endif
    case MELLO_UI_GLOBAL: return MELLO_UI_MIX;
    case MELLO_UI_SOUND_RESERVED: return MELLO_UI_SAMPLE_RESERVED;
    case MELLO_UI_SAMPLE_RESERVED: return MELLO_UI_SOUND_RESERVED;
    default: return p; /* HOME, RANDOM, KEY / SCALE, SLOTS, AUDIO FX (its pages are its layer groups) */
    }
}

static void verify_select_is_layer_algorithm_is_page(void)
{
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        MelloProject pa, pb;
        MelloTransport ta, tb;
        MelloStorage sa, sb;
        MelloUi a, b;
        setup(&pa, &ta, &sa, &a, cases[i].page);
        setup(&pb, &tb, &sb, &b, cases[i].page);
        turn(&a, MELLO_UI_ENC_SELECT, +1);
        turn(&b, MELLO_UI_ENC_ALGORITHM, +1);
        /* SOUND's second layer is OSC 2, subpage 6 (Phase 5's layers came later, and sit after OSC). */
        if (cases[i].layers > 1u && cases[i].page != MELLO_UI_AUDIO_FX &&
            layer(&a, cases[i].page) != (cases[i].page == MELLO_UI_SOUND_RESERVED ? 6u : 1u))
            fail(cases[i].name, "layer encoder baseline mismatch");
        if (cases[i].layers == 1u && layer(&a, cases[i].page) != 0u)
            fail(cases[i].name, "single-layer page unexpectedly moved");
        if (b.page != section_next_page(cases[i].page))
            fail(cases[i].name, "ALGORITHM did not turn to the section's next page");
        if (cases[i].page == MELLO_UI_AUDIO_FX && b.fx_subpage != 1u)
            fail(cases[i].name, "ALGORITHM did not turn FX to DELAY");
        if (cases[i].page == MELLO_UI_AUDIO_FX && a.fx_subpage != 0u)
            fail(cases[i].name, "SELECT left DISTORTION, which has one layer");
    }
}

static void verify_presets_global_slot_cursor(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi u;
    MelloEngine before;
    /* Off HOME (HOME's PRESETS picks the PATTERN: ui_home_perform). */
    setup(&p, &t, &st, &u, MELLO_UI_SOUND_RESERVED);
    u.selected_slot = 0u;
    u.slot_bank = 0u;
    before = t.engine;
    turn(&u, MELLO_UI_ENC_PRESETS, +17);
    if (u.page != MELLO_UI_SOUND_RESERVED || u.selected_slot != 17u || u.slot_bank != 1u)
        fail("PRESETS", "global slot cursor baseline mismatch");
    if (memcmp(&before, &t.engine, sizeof before))
        fail("PRESETS", "global slot cursor unexpectedly changed engine state");
}

static void verify_loop_k4_inert(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi u;
    setup(&p, &t, &st, &u, MELLO_UI_SEQ_LOOP);
    u.loop_subpage = 2u;
    t.engine.loop_cycle_pos = 5u;
    t.engine.loop_cycle_tick = 9u;
    u.dirty = 0u;
    turn(&u, MELLO_UI_ENC_KNOB4, +1);
    if (t.engine.loop_cycle_pos != 5u || t.engine.loop_cycle_tick != 9u || u.dirty)
        fail("LOOP K4", "must be inert after contextual RESET migration");
}

int main(void)
{
    verify_select_is_layer_algorithm_is_page();
    verify_presets_global_slot_cursor();
    verify_loop_k4_inert();
    printf("C1 ENCODER MAP: page/layers=%u ALGORITHM=section-page PRESETS=global-slot-cursor "
           "LOOP-K4=inert\n",
           (unsigned)(sizeof(cases) / sizeof(cases[0])));
    puts("PASS: encoder baseline: SELECT the layer, ALGORITHM the page, PRESETS the slot, while LOOP K4 is "
         "inert after C4 "
         "migration");
    return 0;
}
