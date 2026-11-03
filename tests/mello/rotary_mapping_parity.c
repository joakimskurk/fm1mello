/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
#include "mello_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char label[4][32];
    char value[4][32];
} Footer;

typedef struct {
    const char *name;
    MelloUiPage page;
    uint8_t subpage;
    uint8_t active_mask;
    uint8_t action_mask;
    const char *label[4];
} Case;

static void fail(const char *name, unsigned k, const char *what, const char *a, const char *b)
{
    fprintf(stderr, "FAIL: %s K%u %s: '%s' vs '%s'\n", name, k + 1, what, a ? a : "", b ? b : "");
    exit(1);
}
static void fill(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c)
{
    (void)ctx;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)c;
}
static void text(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large, const char *s, uint16_t c,
                 uint8_t align)
{
    Footer *f = (Footer *)ctx;
    int k;
    (void)w;
    (void)large;
    (void)c;
    (void)align;
    if (x < 2) return;
    k = (x - 2) / 60;
    if (k < 0 || k > 3) return;
    if (y == 189) { /* the footer label row (189: the hairline is at 188) */
        snprintf(f->label[k], sizeof f->label[k], "%s", s ? s : "");
    } else if (y == 205) {
        snprintf(f->value[k], sizeof f->value[k], "%s", s ? s : "");
    }
}
static void led_key(void *ctx, uint8_t i, bool on)
{
    (void)ctx;
    (void)i;
    (void)on;
}
static void led_btn(void *ctx, uint8_t i, bool on)
{
    (void)ctx;
    (void)i;
    (void)on;
}
static void midi(void *ctx, uint8_t a, uint8_t b, uint8_t c)
{
    (void)ctx;
    (void)a;
    (void)b;
    (void)c;
}

static void set_subpage(MelloUi *u, const Case *c)
{
    switch (c->page) {
    case MELLO_UI_SEQ_STEP: u->step_subpage = c->subpage; break;
    case MELLO_UI_SEQ_SEGMENT: u->segment_subpage = c->subpage; break;
    case MELLO_UI_SEQ_LOOP: u->loop_subpage = c->subpage; break;
    case MELLO_UI_SLOTS: u->slots_subpage = c->subpage; break;
    case MELLO_UI_TURING: u->tm_subpage = c->subpage; break;
    case MELLO_UI_SUB: u->sub_subpage = c->subpage; break;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: u->chord_subpage = c->subpage; break;
#endif
    case MELLO_UI_DRUMS: u->drum_subpage = c->subpage; break;
    case MELLO_UI_GLOBAL: u->global_subpage = c->subpage; break;
    case MELLO_UI_SOUND_RESERVED: u->sound_subpage = c->subpage; break;
    case MELLO_UI_AUDIO_FX: u->fx_subpage = c->subpage; break;
    case MELLO_UI_SAMPLE_RESERVED: u->sample_subpage = c->subpage; break;
    default: break;
    }
}

static void setup(MelloProject *p, MelloTransport *t, MelloStorage *st, MelloUi *u, const Case *c)
{
    mello_project_defaults(p);
    mello_transport_init(t, p, 123u, midi, NULL);
    mello_storage_init(st, 0, 0);
    mello_ui_init(u, t, st);
    t->engine.project.seg_count = 3u;
    t->engine.project.seg_splits[0] = 8u;
    t->engine.project.seg_splits[1] = 12u;
    u->selected_segment = 1u;
    t->engine.loop_cycle_pos = 3u;
    t->engine.loop_cycle_tick = 4u;
    t->engine.sample.slots[0].trim_start = 200u;
    t->engine.sample.slots[0].trim_end = 800u;
    (void)mello_engine_save_slot(&t->engine, 0u, "TEST");
    u->selected_slot = 0u;
    u->slot_name_cursor = 0u;
    mello_ui_set_page(u, c->page);
    set_subpage(u, c);
    u->dirty = MELLO_UI_DIRTY_ALL;
}

static void render(MelloUi *u, Footer *f)
{
    MelloUiDrawOps ops = {fill, text, led_key, led_btn, f, 0};
    memset(f, 0, sizeof *f);
    mello_ui_render(u, &ops);
}
static void turn_delta(MelloUi *u, unsigned k, int16_t delta)
{
    int16_t e[MELLO_UI_ENCODER_COUNT] = {0};
    e[MELLO_UI_ENC_KNOB1 + k] = delta;
    mello_ui_input(u, 100000u, 100u, 0, 0, 0, 0, e);
}
static void turn(MelloUi *u, unsigned k)
{
    turn_delta(u, k, 1);
}

#define C(n, p, s, m, a, l1, l2, l3, l4)                                                                     \
    {                                                                                                        \
        n, p, s, m, a,                                                                                       \
        {                                                                                                    \
            l1, l2, l3, l4                                                                                   \
        }                                                                                                    \
    }
static const Case cases[] = {
    C("HOME", MELLO_UI_HOME, 0, 15, 0, "CUTOFF", "DECAY", "SPACE",
      "DRIVE"), /* macros; a held step: see ui_home_perform */
    C("STEP TRIG", MELLO_UI_SEQ_STEP, 0, 15, 0, "PITCH", "PULSE", "RATCH", "GATE"),
    C("STEP NOTE", MELLO_UI_SEQ_STEP, 1, 15, 0, "OCT", "PROB", "SAMP", "WAVE"),
    C("STEP FLAGS", MELLO_UI_SEQ_STEP, 2, 15, 0, "ACCENT", "SUB", "SLIDE", "SKIP"),
    C("STEP SEQ", MELLO_UI_SEQ_STEP, 3, 15, 0, "LENGTH", "RESET", "DIV", "SPAN"),
    C("SEGMENT", MELLO_UI_SEQ_SEGMENT, 0, 15, 0, "REPEAT", "ORDER", "SKIP", "LENGTH"),
    C("LOOP STEP", MELLO_UI_SEQ_LOOP, 0, 15, 0, "HOLD", "ROOT+", "OCT", "TRANS"),
    C("LOOP HARM", MELLO_UI_SEQ_LOOP, 1, 15, 0, "KEY", "SCALE", "SUB", "BASS TR"),
    C("LOOP LOOP", MELLO_UI_SEQ_LOOP, 2, 7, 0, "LENGTH", "SYNC", "ON", ""),
    C("SLOTSEQ", MELLO_UI_SEQ_SLOTSEQ, 0, 15, 0, "SLOT", "HOLD", "ON", "LENGTH"),
    C("RANDOM", MELLO_UI_RANDOM, 0, 1, 0, "INTENS", "", "", ""),
    C("SLOTS LOAD", MELLO_UI_SLOTS, 0, 2, 0, "", "VISIBLE", "", ""),
    C("SLOTS NAME", MELLO_UI_SLOTS, 1, 3, 0, "CURSOR", "CHAR", "", ""),
    C("DOT", MELLO_UI_DOT, 0, 15, 0, "MODE", "LENGTH", "CHANCE", "EVOLVE"),
    C("TURING MAIN", MELLO_UI_TURING, 0, 15, 0, "MUTATE", "LENGTH", "RATE", "OFFSET"),
    C("TURING ROUTE", MELLO_UI_TURING, 1, 15, 0, "GATE", "FILTER", "OCT", "DECAY"),
    C("TURING DEPTH", MELLO_UI_TURING, 2, 15, 0, "GATE", "FILTER", "OCT", "DECAY"),
    C("TURING SOUND", MELLO_UI_TURING, 3, 15, 0, "DEST A", "DEPTH A", "DEST B", "DEPTH B"),
    C("SUB TRIG", MELLO_UI_SUB, 0, 15, 0, "MODE", "PULSES", "ROTATE", "OCT"),
    C("SUB SEQ", MELLO_UI_SUB, 1, 15, 0, "SEQ", "LENGTH", "FOLLOW", "GATE"),
    C("SUB STEP", MELLO_UI_SUB, 2, 15, 0, "TONE", "OCT", "GLIDE", "ACCENT"),
    C("SUB VOICE", MELLO_UI_SUB, 3, 15, 0, "LEVEL", "WAVE", "DECAY", "RELEASE"),
    C("SUB TONE", MELLO_UI_SUB, 4, 15, 0, "GLIDE", "CUT", "DRIVE", "OCT"),
    C("SUB FILTER", MELLO_UI_SUB, 5, 15, 0, "RES", "ENV", "F.DEC", "ACCENT"),
#if MELLO_FEATURE_CHORD_MACHINE
    C("CHORD PLAY", MELLO_UI_CHORD, 0, 15, 0, "MODE", "TRIG", "CHORDS", "GATE"),
    C("CHORD CHORD", MELLO_UI_CHORD, 1, 15, 0, "DEG", "SHAPE", "HOLD", "OCT"),
    C("CHORD VOICING", MELLO_UI_CHORD, 2, 15, 0, "INV", "SPREAD", "VOICES", "VLEAD"),
    C("CHORD GRID", MELLO_UI_CHORD, 3, 15, 0, "MODE", "LENGTH", "DEPTH", "FOLLOW"),
    C("CHORD PATTERN", MELLO_UI_CHORD, 4, 3, 0, "MASK", "LENGTH", "", ""),
#if MELLO_FEATURE_CHORD_POLY
    C("CHORD TONE", MELLO_UI_CHORD, 5, 15, 0, "WAVE", "CHORUS", "SPACE", "LEVEL"),
#else /* SUB MELLO: one voice, no chorus and nothing to strum */
    C("CHORD TONE", MELLO_UI_CHORD, 5, 15, 0, "WAVE", "", "SPACE", "LEVEL"),
#endif
    C("CHORD FILTER", MELLO_UI_CHORD, 6, 15, 0, "CUT", "RES", "ENV", "LINK"),
#if MELLO_FEATURE_CHORD_POLY
    C("CHORD MOTION", MELLO_UI_CHORD, 7, 15, 0, "F.ATK", "F.DEC", "STRUM", "NEXT"),
#else
    C("CHORD MOTION", MELLO_UI_CHORD, 7, 15, 0, "F.ATK", "F.DEC", "", "NEXT"),
#endif
    C("CHORD AMP", MELLO_UI_CHORD, 8, 15, 0, "ATTACK", "DECAY", "SUSTAIN", "RELEASE"),
    C("CHORD HARMONY", MELLO_UI_CHORD, 9, 7, 0, "FOLLOW", "DRIFT", "DEPTH", ""),
#endif
    C("KEY SCALE", MELLO_UI_KEY_SCALE, 0, 15, 0, "ROOT", "SCALE", "OCT", "SPAN"),
    C("GLOBAL CLOCK", MELLO_UI_GLOBAL, 0, 15, 0, "BPM", "DIV", "SYNC", "NOTEOUT"),
    C("GLOBAL MIDI", MELLO_UI_GLOBAL, 1, 15, 0, "LEAD", "SUB", "NOTE", "LENGTH"),
    C("GLOBAL GROOVE", MELLO_UI_GLOBAL, 2, 1, 0, "SWING", "", "", ""),
    C("SOUND OSC", MELLO_UI_SOUND_RESERVED, 0, 15, 0, "WAVE", "PWM", "CUT", "RES"),
    C("SOUND ENV", MELLO_UI_SOUND_RESERVED, 1, 15, 0, "ATTACK", "DECAY", "SUSTAIN", "RELEASE"),
    C("SOUND LFO", MELLO_UI_SOUND_RESERVED, 2, 15, 0, "LFO", "RATE", "DEPTH", "TARGET"),
    C("SOUND FOLLOW", MELLO_UI_SOUND_RESERVED, 3, 15, 0, "SUBOSC", "LEVEL", "OCT", "WAVE"),
    C("SOUND PLAY", MELLO_UI_SOUND_RESERVED, 4, 3, 0, "SLIDE", "MASTER", "", ""),
    C("FX DIST", MELLO_UI_AUDIO_FX, 0, 15, 0, "AMT", "MIX", "CHAR", "TONE"),
    C("FX DELAY", MELLO_UI_AUDIO_FX, 1, 15, 0, "WET", "FDBK", "LO CUT", "HI CUT"),
    C("FX DELAY2", MELLO_UI_AUDIO_FX, 2, 15, 0, "SYNC", "DIV", "TIME", "LEVEL"),
    C("FX REVERB", MELLO_UI_AUDIO_FX, 3, 15, 0, "WET", "SIZE", "DECAY", "TONE"),
    C("FX RETURNS", MELLO_UI_AUDIO_FX, 4, 7, 0, "PRE", "LEVEL", "D.LEVEL", ""),
    C("SAMPLE MAIN", MELLO_UI_SAMPLE_RESERVED, 0, 15, 0, "LEVEL", "DIST", "TRACK", "LOOP"),
    C("SAMPLE PITCH", MELLO_UI_SAMPLE_RESERVED, 1, 15, 0, "COARSE", "FINE", "LEVEL", "DIST"),
    C("SAMPLE TRIM", MELLO_UI_SAMPLE_RESERVED, 2, 15, 0, "START", "END", "TRACK", "LOOP"),
    C("SAMPLE ENV", MELLO_UI_SAMPLE_RESERVED, 3, 15, 0, "ATTACK", "DECAY", "SUSTAIN", "RELEASE"),
    C("DRUMS VOICE", MELLO_UI_DRUMS, 0, 15, 0, "TUNE", "DECAY", "TONE", "LEVEL"),
    C("DRUMS STEP", MELLO_UI_DRUMS, 1, 15, 0, "PITCH", "DECAY", "CRUSH", "RATCH"),
    C("DRUMS VARY", MELLO_UI_DRUMS, 2, 15, 0, "CHANCE", "CRUSH", "FEED", "ROLL"),
    C("DRUMS KIT", MELLO_UI_DRUMS, 3, 15, 0, "LENGTH", "FOLLOW", "SPACE", "LEVEL"),
    C("DRUMS DIST", MELLO_UI_DRUMS, 4, 7, 0, "DRIVE", "FOLD", "DUCK", ""),
#if MELLO_FEATURE_CHORD_MACHINE
    C("MIX", MELLO_UI_MIX, 0, 15, 0, "LEAD", "BASS", "CHORD", "DRUMS")};
#else
    C("MIX", MELLO_UI_MIX, 0, 11, 0, "LEAD", "BASS", "CHORD", "DRUMS")};
#endif

int main(void)
{
    unsigned ci, k, checked = 0;
    for (ci = 0; ci < sizeof(cases) / sizeof(cases[0]); ++ci) {
        for (k = 0; k < 4; ++k) {
            MelloProject p;
            MelloTransport t;
            MelloStorage st;
            MelloUi u;
            Footer before, after;
            const Case *c = &cases[ci];
            setup(&p, &t, &st, &u, c);
            render(&u, &before);
            if (strcmp(before.label[k], c->label[k])) fail(c->name, k, "label", before.label[k], c->label[k]);
            if (!(c->active_mask & (1u << k))) {
                if (strcmp(before.value[k], "-")) fail(c->name, k, "inactive value", before.value[k], "-");
                checked++;
                continue;
            }
            turn(&u, k);
            render(&u, &after);
            if (strcmp(after.label[k], c->label[k]))
                fail(c->name, k, "label after turn", after.label[k], c->label[k]);
            if (c->action_mask & (1u << k)) {
                if (c->page == MELLO_UI_SEQ_LOOP && c->subpage == 2u && k == 3u &&
                    t.engine.loop_cycle_pos != 0u)
                    fail(c->name, k, "RESET action", "position", "0");
                checked++;
                continue;
            }
            if (!strcmp(before.value[k], after.value[k])) {
                /* Defaults can be at a clamp edge and some fine controls quantize their display. */
                turn_delta(&u, k, -2);
                render(&u, &after);
            }
            if (!strcmp(before.value[k], after.value[k])) {
                turn_delta(&u, k, 4);
                render(&u, &after);
            }
            if (!strcmp(before.value[k], after.value[k])) {
                turn_delta(&u, k, 16);
                render(&u, &after);
            }
            if (!strcmp(before.value[k], after.value[k])) {
                turn_delta(&u, k, -32);
                render(&u, &after);
            }
            if (!strcmp(before.value[k], after.value[k]))
                fail(c->name, k, "display did not follow handler", before.value[k], after.value[k]);
            checked++;
        }
    }
    printf("PASS: %u rotary/footer mappings across %u page/layer fixtures stay aligned\n", checked,
           (unsigned)(sizeof(cases) / sizeof(cases[0])));
    return 0;
}
