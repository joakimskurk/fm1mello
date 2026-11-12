/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
#include "mello_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char hint[96];
    unsigned buttons[14];
} Capture;

typedef struct {
    const char *name;
    MelloUiPage page;
    const char *hint;
} HintCase;

static const HintCase hints[] = {
    {"HOME", MELLO_UI_HOME, "B0:- B1:+ B3:RST B4:CLR B5:ROLL B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"STEP", MELLO_UI_SEQ_STEP, "B1:SKIP B5:ROLL B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"SEGMENT", MELLO_UI_SEQ_SEGMENT,
     "B0:EVEN B1:A B2:B B3:C B4:D B5:ROLL B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"LOOP", MELLO_UI_SEQ_LOOP, "B0:ON B1:RST B5:ROLL B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"SLOTSEQ", MELLO_UI_SEQ_SLOTSEQ, "B0:ON B5:ROLL B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"RANDOM", MELLO_UI_RANDOM, "B0:NONE B1:ALL B2:CLR B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"KEY/SCALE", MELLO_UI_KEY_SCALE, "B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"SLOTS", MELLO_UI_SLOTS, "B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"DOT", MELLO_UI_DOT, "B0:ON B2:RST B3:FLW B4:REV B5:ROLL B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"TURING", MELLO_UI_TURING, "B0:ON B2:LOOP B5:ROLL B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
#if MELLO_FEATURE_CHORD_MACHINE
    {"SUB", MELLO_UI_SUB, "B0:ON B1:SEQ B3:CHD B5:ROLL B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
#else
    {"SUB", MELLO_UI_SUB, "B0:ON B1:SEQ B5:ROLL B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
#endif
#if MELLO_FEATURE_CHORD_MACHINE
    {"CHORD", MELLO_UI_CHORD, "B0:ON B1:BASS B2:POLY B3:FIT B5:ROLL B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
#endif
    {"GLOBAL", MELLO_UI_GLOBAL, "B1:TAP B2:RST2 B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"SOUND", MELLO_UI_SOUND_RESERVED, "B1:INIT B5:ROLL B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"AUDIO FX", MELLO_UI_AUDIO_FX, "B1:PRE-DIST B2:PING B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"SAMPLE", MELLO_UI_SAMPLE_RESERVED, "B0:ON B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
    {"DRUMS", MELLO_UI_DRUMS, "B0:ON B1:VOX B2:ACC B5:ROLL B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
#if MELLO_FEATURE_CHORD_MACHINE
    {"MIX", MELLO_UI_MIX, "B0:LD B1:BS B2:CH B3:DR B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
#else
    {"MIX", MELLO_UI_MIX, "B0:LD B1:BS B3:DR B6:MUT B8:UNDO B9:RNDMZR B10:PERF"},
#endif
};

/* The strip shows labels only, in key order: "B0:RND B1:UNDO" is drawn as "RND UNDO". */
static void labels_of(const char *hint, char *out)
{
    *out = 0;
    while (*hint) {
        const char *colon = strchr(hint, ':'), *end = strchr(hint, ' ');
        size_t n;
        if (!colon) break;
        if (!end) end = hint + strlen(hint);
        if (*out) strcat(out, " ");
        n = strlen(out);
        memcpy(out + n, colon + 1, (size_t)(end - colon - 1));
        out[n + (size_t)(end - colon - 1)] = 0;
        hint = *end ? end + 1 : end;
    }
}
/* A strip too full for frames shows PERF and MUT as P and M (draw_control_hint), and B5's ROLL as RL or R if it needs
 * the room too (roll: 0 as it is, else the short form): the same keys. */
static void short_of(const char *labels, char *out, const char *roll)
{
    const char *p = labels;
    *out = 0;
    while (*p) {
        const char *e = strchr(p, ' ');
        size_t n = e ? (size_t)(e - p) : strlen(p);
        if (*out) strcat(out, " ");
        if (n == 4u && !strncmp(p, "PERF", 4u))
            strcat(out, "P");
        else if (n == 3u && !strncmp(p, "MUT", 3u))
            strcat(out, "M");
        else if (roll && n == 4u && !strncmp(p, "ROLL", 4u))
            strcat(out, roll);
        else
            strncat(out, p, n);
        p = e ? e + 1 : p + n;
    }
}

static uint32_t bit_u32(unsigned n)
{
    return 1u << n;
}
static void die(const char *where, const char *what)
{
    fprintf(stderr, "FAIL: %s: %s\n", where, what);
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
    Capture *cap = (Capture *)ctx;
    (void)x;
    (void)w;
    (void)large;
    (void)c;
    (void)align;
    if (y == 223 && s && *s &&                                   /* the strip: at the bottom since G7 */
        strlen(cap->hint) + strlen(s) + 2u < sizeof cap->hint) { /* the strip's labels */
        if (cap->hint[0]) strcat(cap->hint, " ");
        strcat(cap->hint, s);
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
    Capture *cap = (Capture *)ctx;
    if (i < 14u) cap->buttons[i] = on ? 1u : 0u;
}
static void midi(void *ctx, uint8_t a, uint8_t b, uint8_t c)
{
    (void)ctx;
    (void)a;
    (void)b;
    (void)c;
}

static void setup(MelloProject *p, MelloTransport *t, MelloStorage *st, MelloUi *u, MelloUiPage page)
{
    mello_project_defaults(p);
    mello_transport_init(t, p, 123u, midi, 0);
    mello_storage_init(st, 0, 0);
    mello_ui_init(u, t, st);
    mello_ui_set_page(u, page);
}
static void press(MelloUi *u, uint32_t ms, uint32_t pressed, uint32_t held)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    mello_ui_input(u, ms * 1000u, ms, pressed, 0u, held, 0u, enc);
}
static uint8_t black_note(uint8_t pos)
{
    uint8_t i, n = 0u;
    for (i = 0u; i < 27u; ++i) {
        if (mello_ui_white_key_position(i) >= 0) continue;
        if (n == pos) return i;
        ++n;
    }
    return 0xFFu;
}
static void black(MelloUi *u, uint32_t ms, uint8_t pos)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    uint8_t note = black_note(pos);
    if (note == 0xFFu) die("BLACK", "missing key");
    mello_ui_input(u, ms * 1000u, ms, 0u, 0u, 0u, bit_u32(note), enc);
}
static void turn_k4(MelloUi *u)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    enc[MELLO_UI_ENC_KNOB4] = 1;
    mello_ui_input(u, 100000u, 100u, 0, 0, 0, 0, enc);
}
static void render(MelloUi *u, Capture *cap)
{
    MelloUiDrawOps ops = {fill, text, led_key, led_btn, cap, 0, 0};
    memset(cap, 0, sizeof *cap);
    u->last_key_led_mask = 0u;
    u->last_button_led_mask = 0u;
    u->dirty = MELLO_UI_DIRTY_ALL;
    mello_ui_render(u, &ops);
    mello_ui_sync_leds(u, &ops);
}

static void verify_hints(void)
{
    unsigned i;
    for (i = 0u; i < sizeof(hints) / sizeof(hints[0]); ++i) {
        MelloProject p;
        MelloTransport t;
        MelloStorage st;
        MelloUi u;
        Capture cap;
        setup(&p, &t, &st, &u, hints[i].page);
        render(&u, &cap);
        {
            char want[96];
            char shrt[96];
            labels_of(hints[i].hint, want);
            char rl[96], r1[96];
            short_of(want, shrt, 0);
            short_of(want, rl, "RL");
            short_of(want, r1, "R");
            if (strcmp(cap.hint, want) && strcmp(cap.hint, shrt) && strcmp(cap.hint, rl) &&
                strcmp(cap.hint, r1)) {
                fprintf(stderr, "FAIL: %s strip '%s' != '%s'\n", hints[i].name, cap.hint, want);
                exit(1);
            }
        }
    }
}

static void verify_random_mask_and_oct(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi u;
    uint32_t mask = MELLO_RAND_PITCH | MELLO_RAND_GATE;
    setup(&p, &t, &st, &u, MELLO_UI_RANDOM);
    mello_engine_set_randomize_mask(&t.engine, mask);
    u.dirty = 0u;
    press(&u, 100u, bit_u32(MELLO_UI_BTN_OCT_DOWN), 0u);
    if (t.engine.randomize.mask != mask || u.dirty) die("RANDOM OCT-", "still changes RANDOM state");
    press(&u, 200u, bit_u32(MELLO_UI_BTN_OCT_UP), 0u);
    if (t.engine.randomize.mask != mask || u.dirty) die("RANDOM OCT+", "still changes RANDOM state");
    black(&u, 300u, 0u); /* NONE and ALL lead the RANDOM page's strip; B10/B9 are RNDMZR/UNDO everywhere */
    if (t.engine.randomize.mask != 0u) die("RANDOM B0", "mask NONE missing");
    black(&u, 400u, 1u);
    if (t.engine.randomize.mask != MELLO_RAND_ALL) die("RANDOM B1", "mask ALL missing");
}

static void verify_global_reset(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi u;
    uint32_t both = bit_u32(MELLO_UI_BTN_OCT_DOWN) | bit_u32(MELLO_UI_BTN_OCT_UP);
    setup(&p, &t, &st, &u, MELLO_UI_GLOBAL);
    t.engine.project.bpm = 199u;
    t.engine.project.seq_len = 24u;
    u.dirty = 0u;
    press(&u, 100u, both, both);
    if (u.reset_armed || t.engine.project.bpm != 199u || t.engine.project.seq_len != 24u)
        die("GLOBAL OCT chord", "hidden RESET ALL survived");
    black(&u, 200u, 2u);
    if (!u.reset_armed || t.engine.project.bpm != 199u) die("GLOBAL B2", "first press did not arm only");
    black(&u, 500u, 2u);
    if (u.reset_armed || u.page != MELLO_UI_HOME || t.engine.project.bpm != 80u ||
        t.engine.project.seq_len != 16u)
        die("GLOBAL B3", "confirmed factory reset failed");
}

static void verify_loop_reset(void)
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
    turn_k4(&u);
    if (t.engine.loop_cycle_pos != 5u || t.engine.loop_cycle_tick != 9u || u.dirty)
        die("LOOP K4", "still acts as RESET");
    black(&u, 200u, 1u);
    if (t.engine.loop_cycle_pos || t.engine.loop_cycle_tick) die("LOOP B1", "context reset missing");
}

static void verify_mod_navigation(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi u;
    bool on;
    setup(&p, &t, &st, &u, MELLO_UI_HOME);
    on = t.engine.tm.on;
    press(&u, 100u, bit_u32(MELLO_UI_BTN_LFO), 0u);
    if (u.page != MELLO_UI_SOUND_RESERVED || u.sound_subpage != 2u || t.engine.tm.on != on)
        die("MOD 1", "did not open Audio LFO cleanly");
    press(&u, 200u, bit_u32(MELLO_UI_BTN_LFO), 0u);
    if (u.page != MELLO_UI_DOT || t.engine.tm.on != on) die("MOD 2", "did not advance to DOT");
    press(&u, 300u, bit_u32(MELLO_UI_BTN_LFO), 0u);
    if (u.page != MELLO_UI_TURING || t.engine.tm.on != on) die("MOD 3", "did not advance to EVOLVE");
    black(&u, 350u, 0u);
    if (t.engine.tm.on == on) die("TURING B0", "state toggle missing");
    on = t.engine.tm.on;
    press(&u, 400u, bit_u32(MELLO_UI_BTN_LFO), 0u);
    if (u.page != MELLO_UI_SOUND_RESERVED || u.sound_subpage != 2u || t.engine.tm.on != on)
        die("MOD wrap", "physical MOD toggled Turing or failed wrap");
    /* ENV after LFO: SOUND from OSC, not the LFO again (R1: ENV's pages are SOUND and SAMPLE; the LFO is LFO's). */
    press(&u, 410u, bit_u32(MELLO_UI_BTN_ENV), 0u);
    if (u.page != MELLO_UI_SOUND_RESERVED || u.sound_subpage != 0u)
        die("ENV after LFO", "stayed on Audio LFO");
    press(&u, 420u, bit_u32(MELLO_UI_BTN_ENV), 0u);
    if (u.page != MELLO_UI_SAMPLE_RESERVED) die("ENV 2", "SOUND is not followed by SAMPLE");
    press(&u, 430u, bit_u32(MELLO_UI_BTN_ENV), 0u);
    if (u.page != MELLO_UI_SOUND_RESERVED || u.sound_subpage != 0u)
        die("ENV 3", "SAMPLE did not go back to SOUND");
    mello_ui_set_page(&u, MELLO_UI_HOME); /* from elsewhere: always the first layer */
    press(&u, 440u, bit_u32(MELLO_UI_BTN_ENV), 0u);
    if (u.page != MELLO_UI_SOUND_RESERVED || u.sound_subpage != 0u) die("ENV", "did not open SOUND on OSC");

    mello_ui_set_page(&u, MELLO_UI_HOME);
    press(&u, 500u, bit_u32(MELLO_UI_BTN_FX), 0u);
    if (u.page != MELLO_UI_AUDIO_FX) die("FX", "did not open Audio FX");
    press(&u, 600u, bit_u32(MELLO_UI_BTN_FX), 0u);
    if (u.page != MELLO_UI_AUDIO_FX) die("FX", "still routes to DOT");
}

static void verify_led_family(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi u;
    Capture cap;
    setup(&p, &t, &st, &u, MELLO_UI_DOT);
    render(&u, &cap);
    if (!cap.buttons[MELLO_UI_BTN_LFO] || cap.buttons[MELLO_UI_BTN_FX]) die("DOT LED", "not in MOD family");
    mello_ui_set_page(&u, MELLO_UI_TURING);
    render(&u, &cap);
    if (!cap.buttons[MELLO_UI_BTN_LFO]) die("TURING LED", "MOD family not lit");
    mello_ui_set_page(&u, MELLO_UI_SOUND_RESERVED);
    u.sound_subpage = 2u;
    render(&u, &cap);
    if (!cap.buttons[MELLO_UI_BTN_LFO] || cap.buttons[MELLO_UI_BTN_ENV])
        die("LFO LED", "Audio LFO not owned by MOD");
    u.sound_subpage = 0u;
    render(&u, &cap);
    if (!cap.buttons[MELLO_UI_BTN_ENV] || cap.buttons[MELLO_UI_BTN_LFO])
        die("SOUND LED", "non-LFO SOUND lost ENV ownership");
    mello_ui_set_page(&u, MELLO_UI_RANDOM);
    render(&u, &cap);
    if (cap.buttons[MELLO_UI_BTN_EDIT]) die("RANDOM LED", "still falsely owned by EDIT");
}

static void verify_inactive_and_chord(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi u;
    MelloEngine before;
    bool on;
    setup(&p, &t, &st, &u, MELLO_UI_HOME);
    before = t.engine;
    u.dirty = 0u;
    black(&u, 100u, 7u); /* B7: free since PERFORM (G4): a press changes nothing */
    if (u.page != MELLO_UI_HOME || memcmp(&before, &t.engine, sizeof before) || u.dirty)
        die("HOME B7", "a free key acted");
    black(&u, 200u, 9u); /* RNDMZR (B9), tapped: rolls, stays on HOME (held, it opens RANDOM) */
    if (u.page != MELLO_UI_HOME ||
        !memcmp(&before.project.steps, &t.engine.project.steps, sizeof before.project.steps))
        die("HOME B5", "RNDMZR tap did not roll");
#if MELLO_FEATURE_CHORD_MACHINE
    setup(&p, &t, &st, &u, MELLO_UI_CHORD);
    on = t.engine.chord.on;
    black(&u, 100u, 0u);
    if (t.engine.chord.on == on) die("CHORD B0", "toggle missing");
#else
    (void)on;
#endif
}

/* The DOT page is static except for its read position: when the engine moves it, the page redraws (it did not, so
 * the matrix sat still while playing), and the key LEDs follow. */
static void verify_dot_playhead(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi u;
    setup(&p, &t, &st, &u, MELLO_UI_DOT);
    mello_ui_tick(&u);
    u.dirty = 0u;
    t.engine.dot_active_cell[u.selected_dot_target] = 5;
    mello_ui_tick(&u);
    if (!(u.dirty & MELLO_UI_DIRTY_BODY) || !(u.dirty & MELLO_UI_DIRTY_LEDS))
        die("DOT playhead", "not redrawn");
    u.dirty = 0u;
    mello_ui_tick(&u);
    if (u.dirty & MELLO_UI_DIRTY_BODY) die("DOT playhead", "redrawn without a move");
}

int main(void)
{
    verify_hints();
    verify_random_mask_and_oct();
    verify_global_reset();
    verify_loop_reset();
    verify_mod_navigation();
    verify_led_family();
    verify_inactive_and_chord();
    verify_dot_playhead();
    printf("C4 CONTRACT: hints=%u MOD=LFO>DOT>EVOLVE OCT=nav-only LOOP-K4=inert\n",
           (unsigned)(sizeof(hints) / sizeof(hints[0])));
    puts("PASS: WP-C4 contextual rail, visible guidance and MOD-family navigation stay aligned");
    return 0;
}
