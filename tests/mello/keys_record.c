/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Record it yourself (DECISIONS.md): in KEYS (EDIT on HOME) the 27 keys are a chromatic keyboard, and the key rule
 * holds everywhere: a note off the project's scale rounds to the nearest on it, the smaller way, down on a tie
 * (CHROMATIC: as played); the LEDs light the scale's keys and leave the rest dark. REC cycles what the keys write,
 * SEQ, BASS, CHORD, then off, the header saying which. Stopped, each key writes the next step (or chord slot, the
 * chain growing as you play, a second key held naming the shape); playing, a key writes the step under the clock,
 * or the next past half a pulse. A recording is one UNDO. */
#include "mello_engine.h"
#include "mello_storage.h"
#include "mello_transport.h"
#include "mello_ui.h"
#include <stdio.h>
#include <string.h>

static char header[64];
static uint32_t key_leds = 0x07FFFFFFu;  /* the UI assumes every LED lit at init and only sends changes */
static uint32_t key_glows = 0x07FFFFFFu; /* (and every glow) */
static void fill(void *c, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t col)
{
    (void)c;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)col;
}
static void text(void *c, int16_t x, int16_t y, int16_t w, uint8_t large, const char *s, uint16_t col,
                 uint8_t al)
{
    (void)c;
    (void)x;
    (void)w;
    (void)large;
    (void)col;
    (void)al;
    if (!s) return;
    if (y == 7) {
        size_t n = strlen(header);
        snprintf(header + n, sizeof header - n, "%s%s", n ? " " : "", s);
    }
}
static void key_led(void *c, uint8_t i, bool on)
{
    (void)c;
    if (on)
        key_leds |= 1u << i;
    else
        key_leds &= ~(1u << i);
}
static void key_glow(void *c, uint8_t i, bool on)
{
    (void)c;
    if (on)
        key_glows |= 1u << i;
    else
        key_glows &= ~(1u << i);
}
static void button_led(void *c, uint8_t i, bool on)
{
    (void)c;
    (void)i;
    (void)on;
}
static void midi(void *c, uint8_t a, uint8_t b, uint8_t d)
{
    (void)c;
    (void)a;
    (void)b;
    (void)d;
}

static MelloTransport t;
static MelloStorage st;
static MelloUi ui;
static uint32_t ms = 1000u;

static void render(void)
{
    MelloUiDrawOps ops = {fill, text, key_led, button_led, 0, 0, key_glow};
    header[0] = 0;
    ui.dirty = MELLO_UI_DIRTY_ALL;
    mello_ui_render(&ui, &ops);
    mello_ui_sync_leds(&ui, &ops);
}
static void in(uint32_t pressed, uint32_t released, uint32_t held, uint32_t notes)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    ms += 50u;
    mello_ui_input(&ui, ms * 1000u, ms, pressed, released, held, notes, enc);
}
static void tap_button(uint8_t b)
{
    in(1u << b, 0u, 1u << b, 0u);
    in(0u, 1u << b, 0u, 0u);
}
static void turn(uint8_t which, int16_t d)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    enc[which] = d;
    ms += 50u;
    mello_ui_input(&ui, ms * 1000u, ms, 0u, 0u, 0u, 0u, enc);
}
static void knob(uint8_t k, int16_t d) /* K1-K4, a detent at a time (no acceleration) */
{
    int16_t i, n = d < 0 ? -d : d;
    for (i = 0; i < n; ++i) turn((uint8_t)(MELLO_UI_ENC_KNOB1 + k), d < 0 ? -1 : 1);
}
static void select_turn(int16_t d)
{
    int16_t i, n = d < 0 ? -d : d;
    for (i = 0; i < n; ++i) turn(MELLO_UI_ENC_SELECT, d < 0 ? -1 : 1);
}
static void key(uint8_t note_index) /* MIDI 53 + note_index at OCT 0 */
{
    mello_ui_set_held_notes(&ui, 1u << note_index);
    in(0u, 0u, 0u, 1u << note_index);
    mello_ui_set_held_notes(&ui, 0u);
}

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                      \
            printf(__VA_ARGS__);                                                                             \
            printf("\n");                                                                                    \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

static unsigned popcount(uint32_t v)
{
    unsigned n = 0u;
    while (v) n += v & 1u, v >>= 1;
    return n;
}

int main(void)
{
    static MelloProject p;
    MelloEngine *e;
    unsigned i;

    /* the key rule, as a function: C minor on C4 */
    CHECK(mello_scale_quantize(MELLO_SCALE_MINOR, 60u, 61u) == 60u, "C# rounds down to C (a tie goes down)");
    CHECK(mello_scale_quantize(MELLO_SCALE_MINOR, 60u, 63u) == 63u, "Eb is in C minor");
    CHECK(mello_scale_quantize(MELLO_SCALE_MINOR, 60u, 66u) == 65u, "F# rounds down to F");
    CHECK(mello_scale_quantize(MELLO_SCALE_MINOR, 60u, 71u) == 70u, "B rounds down to Bb");
    CHECK(mello_scale_quantize(MELLO_SCALE_MINOR, 60u, 64u) == 63u, "E rounds down to Eb");
    CHECK(mello_scale_quantize(MELLO_SCALE_CHROMATIC, 60u, 61u) == 61u, "CHROMATIC leaves C# alone");
    CHECK(mello_scale_quantize(MELLO_SCALE_PENTATONIC, 60u, 65u) == 64u,
          "F in C pentatonic rounds down to E");

    mello_project_defaults(&p); /* C minor on 60, 16 steps */
    mello_transport_init(&t, &p, 1u, midi, NULL);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&ui, &t, &st);
    e = &t.engine;

    /* KEYS: the scale's keys lit, F3 .. G5 is 27 keys = 16 of C minor's (two octaves and a bit) */
    tap_button(MELLO_UI_BTN_EDIT);
    render();
    CHECK(strstr(header, "KEYS"), "EDIT tapped: KEYS, the header reads \"%s\"", header);
    /* G5: the scale's tones glow, the root lit */
    CHECK(popcount(key_leds | key_glows) == 16u, "C minor lights %u of 27 keys, wanted 16",
          popcount(key_leds | key_glows));
    CHECK((key_leds & (1u << 7)) && !(key_glows & (1u << 7)), "C4 (key 8), the root, is lit");
    CHECK(popcount(key_leds) == 2u, "the root lit on its 2 keys, C4 and C5 (%u)", popcount(key_leds));
    CHECK(!((key_leds | key_glows) & (1u << 8)), "C#4 (key 9) is dark");
    e->project.scale = MELLO_SCALE_CHROMATIC;
    render();
    CHECK(popcount(key_leds | key_glows) == 27u, "CHROMATIC lights every key (%u)",
          popcount(key_leds | key_glows));
    e->project.scale = MELLO_SCALE_MINOR;

    /* REC > SEQ, stopped: the keys write step 1, 2, 3 ..., a key off the scale rounded */
    tap_button(MELLO_UI_BTN_REC);
    render();
    CHECK(strstr(header, "STEP SEQ 1/16"), "REC once: SEQ (stopped: step entry), the header reads \"%s\"",
          header);
    CHECK(ui.keys_rec && ui.keys_rec_target == MELLO_UI_REC_SEQ, "the target is SEQ");
    for (i = 0u; i < 16u; ++i) e->project.steps[i].pitch = 0, e->project.steps[i].octave = 0;
    key(7u);  /* C4 */
    key(14u); /* G4 */
    key(13u); /* F#4: not in C minor, rounds to F4 */
    key(10u); /* Eb4 */
    CHECK(ui.keys_rec_pos == 4u, "four keys: the next step is 5 (%u)", ui.keys_rec_pos + 1u);
    CHECK(mello_resolve_step_midi(&e->project, &e->project.steps[0]) == 60u, "step 1 is C4");
    CHECK(mello_resolve_step_midi(&e->project, &e->project.steps[1]) == 67u, "step 2 is G4");
    CHECK(mello_resolve_step_midi(&e->project, &e->project.steps[2]) == 65u,
          "step 3 is F4 (F# rounded), got %u", mello_resolve_step_midi(&e->project, &e->project.steps[2]));
    CHECK(mello_resolve_step_midi(&e->project, &e->project.steps[3]) == 63u, "step 4 is Eb4");
    CHECK(!e->project.steps[2].skip && e->project.steps[2].gate != MELLO_GATE_REST, "a written step plays");

    /* REC > BASS: root, fifth, the root an octave up */
    tap_button(MELLO_UI_BTN_REC);
    render();
    CHECK(strstr(header, "STEP BASS"), "REC twice: BASS, the header reads \"%s\"", header);
    CHECK(ui.keys_rec_pos == 0u, "a new target starts at step 1");
    key(7u);  /* C4: ROOT, octave 0 */
    key(14u); /* G4: FIFTH */
    key(19u); /* C5: ROOT, octave +1 */
    key(8u);  /* C#4: rounds to C */
    CHECK(e->bass.on, "a bass key turns BASS SEQ on");
    CHECK(e->bass.steps[0].on && e->bass.steps[0].tone == MELLO_SUB_TONE_ROOT && e->bass.steps[0].octave == 0,
          "bass step 1: root at 0 (tone %u oct %d)", e->bass.steps[0].tone, e->bass.steps[0].octave);
    CHECK(e->bass.steps[1].tone == MELLO_SUB_TONE_FIFTH, "bass step 2: the fifth (tone %u)",
          e->bass.steps[1].tone);
    CHECK(e->bass.steps[2].tone == MELLO_SUB_TONE_ROOT && e->bass.steps[2].octave == 1,
          "bass step 3: the root an octave up (tone %u oct %d)", e->bass.steps[2].tone,
          e->bass.steps[2].octave);
    CHECK(e->bass.steps[3].tone == MELLO_SUB_TONE_ROOT, "bass step 4: C# rounded to the root");

#if MELLO_FEATURE_CHORD_MACHINE
    /* REC > CHORD: a key a chord, the chain as long as you have played; a second key held names the shape */
    tap_button(MELLO_UI_BTN_REC);
    render();
    CHECK(strstr(header, "STEP CHORD"), "REC thrice: CHORD, the header reads \"%s\"", header);
    key(7u); /* C: degree i */
    CHECK(e->chord.on && e->chord.length == 1u && e->chord.steps[0].degree == 0u,
          "one key: a chain of one, the tonic (on %d, length %u, degree %u)", e->chord.on, e->chord.length,
          e->chord.steps[0].degree);
    CHECK(e->chord.steps[0].octave == -1, "played at the root's octave: the chord's default octave -1 (%d)",
          e->chord.steps[0].octave);
    mello_ui_set_held_notes(&ui, (1u << 7) | (1u << 14)); /* C still held, G pressed: a power chord */
    in(0u, 0u, 0u, 1u << 14);
    mello_ui_set_held_notes(&ui, 0u);
    CHECK(e->chord.length == 1u && e->chord.steps[0].shape == MELLO_CHORD_POWER,
          "a fifth over the first key: POWER on the same chord (length %u, shape %d)", e->chord.length,
          e->chord.steps[0].shape);
    key(12u);                                              /* F: iv */
    key(14u);                                              /* G: v */
    mello_ui_set_held_notes(&ui, (1u << 14) | (1u << 24)); /* G held, F5 pressed: a seventh */
    in(0u, 0u, 0u, 1u << 24);
    mello_ui_set_held_notes(&ui, 0u);
    CHECK(e->chord.length == 3u && e->chord.steps[1].degree == 3u && e->chord.steps[2].degree == 4u,
          "three chords: i iv v (length %u: %u %u %u)", e->chord.length, e->chord.steps[0].degree,
          e->chord.steps[1].degree, e->chord.steps[2].degree);
    CHECK(e->chord.steps[2].shape == MELLO_CHORD_SEVENTH, "a seventh over the v: SEVENTH (%d)",
          e->chord.steps[2].shape);
    CHECK(e->chord.steps[1].shape == MELLO_CHORD_TRIAD, "the iv alone stays a triad");
#endif

    /* REC once more: DRUMS (DECISIONS "Record it yourself, easily"); and again: off, KEYS as before */
    tap_button(MELLO_UI_BTN_REC);
    render();
    CHECK(strstr(header, "STEP DRUMS"), "REC four times: DRUMS, the header reads \"%s\"", header);
    tap_button(MELLO_UI_BTN_REC);
    render();
    CHECK(!ui.keys_rec && strstr(header, "KEYS"), "REC round again: off, the header reads \"%s\"", header);

    /* UNDO takes the whole recording back: the sequence, the bass line and the chords as they were */
    CHECK(mello_engine_undo_swap(e), "there is an undo");
    CHECK(mello_resolve_step_midi(&e->project, &e->project.steps[1]) != 67u, "step 2 is no longer G4");
    CHECK(!e->bass.on, "the bass line is off again");

#if MELLO_FEATURE_CHORD_MACHINE
    { /* a lower second key is an inversion; C Eb G together is a triad, not a power chord */
        tap_button(MELLO_UI_BTN_REC);                          /* SEQ */
        tap_button(MELLO_UI_BTN_REC);                          /* BASS */
        tap_button(MELLO_UI_BTN_REC);                          /* CHORD, from slot 1 */
        key(19u);                                              /* C5 */
        mello_ui_set_held_notes(&ui, (1u << 19) | (1u << 10)); /* Eb4 under it */
        in(0u, 0u, 0u, 1u << 10);
        mello_ui_set_held_notes(&ui, 0u);
        CHECK(e->chord.steps[0].inversion == 1u, "the third under the root: inversion 1 (%u)",
              e->chord.steps[0].inversion);
        key(19u);                                              /* C5, the second chord */
        mello_ui_set_held_notes(&ui, (1u << 19) | (1u << 14)); /* G4 under it */
        in(0u, 0u, 0u, 1u << 14);
        mello_ui_set_held_notes(&ui, 0u);
        CHECK(e->chord.steps[1].inversion == 2u, "the fifth under the root: inversion 2 (%u)",
              e->chord.steps[1].inversion);
        key(7u);                                              /* C4, the third chord */
        mello_ui_set_held_notes(&ui, (1u << 7) | (1u << 10)); /* Eb4 */
        in(0u, 0u, 0u, 1u << 10);
        mello_ui_set_held_notes(&ui, (1u << 7) | (1u << 10) | (1u << 14)); /* and G4 */
        in(0u, 0u, 0u, 1u << 14);
        mello_ui_set_held_notes(&ui, 0u);
        CHECK(e->chord.steps[2].shape == MELLO_CHORD_TRIAD, "C Eb G held: a triad (%d)",
              e->chord.steps[2].shape);
        tap_button(MELLO_UI_BTN_REC); /* off */
    }
#endif

    /* playing, REC > SEQ: a key writes the step under the clock, or the next one past half a pulse */
    {
        uint32_t pulse_us = 60000000u / e->project.bpm / 4u, us;
        uint8_t at, before[16], changed = 0u, which = 0xFFu;
        mello_project_defaults(&p);
        for (i = 0u; i < 16u; ++i) p.steps[i].pitch = 0, p.steps[i].octave = 0;
        mello_transport_init(&t, &p, 1u, midi, NULL);
        mello_ui_init(&ui, &t, &st);
        e = &t.engine;
        tap_button(MELLO_UI_BTN_EDIT);
        tap_button(MELLO_UI_BTN_REC);
        mello_transport_start(&t, ms * 1000u);
        for (us = ms * 1000u; us < ms * 1000u + 5u * pulse_us + pulse_us / 4u; us += 1000u)
            mello_transport_poll_ahead(&t, us, MELLO_TRANSPORT_LOOKAHEAD_US);
        at = t.display_step;
        for (i = 0u; i < 16u; ++i) before[i] = (uint8_t)e->project.steps[i].pitch;
        ms = us / 1000u;
        mello_ui_set_held_notes(&ui, 1u << 14);
        {
            int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
            mello_ui_input(&ui, us, ms, 0u, 0u, 0u, 1u << 14, enc); /* G4 a quarter into a pulse */
        }
        mello_ui_set_held_notes(&ui, 0u);
        for (i = 0u; i < 16u; ++i)
            if ((uint8_t)e->project.steps[i].pitch != before[i]) changed++, which = (uint8_t)i;
        CHECK(changed == 1u, "one step written while playing (%u)", changed);
        CHECK(which == at || which == (uint8_t)((at + 1u) % 16u),
              "written at the clock's step %u or the next, got %u", at, which);
        CHECK(mello_resolve_step_midi(&e->project, &e->project.steps[which]) == 67u, "and it is G4");
        CHECK(t.running, "the transport kept running");
        printf("playing: the key landed on step %u with the clock at %u\n", which + 1u, at + 1u);
    }

    { /* stopped (step entry, DECISIONS "Step entry"): SEL is a rest; a key held and K1-K4 turned is that step's PULSE,
       * RATCH, GATE, ACC SLD (BASS: GLIDE, ACCENT, OCT), and the next steps get the same; SELECT moves the cursor; a
       * key held long no longer makes a held note by itself */
        mello_project_defaults(&p);
        mello_transport_init(&t, &p, 1u, midi, NULL);
        mello_ui_init(&ui, &t, &st);
        e = &t.engine;
        tap_button(MELLO_UI_BTN_EDIT);
        tap_button(MELLO_UI_BTN_REC); /* SEQ */
        key(7u);
        tap_button(MELLO_UI_BTN_SEL);
        CHECK(e->project.steps[1].gate == MELLO_GATE_REST && ui.keys_rec_pos == 2u,
              "SEL writes a rest and moves on (gate %d, next %u)", e->project.steps[1].gate,
              ui.keys_rec_pos + 1u);
        CHECK(ui.page == MELLO_UI_HOME, "SEL stays on HOME while recording");
        mello_ui_set_held_notes(&ui, 1u << 14);
        in(0u, 0u, 0u, 1u << 14);
        for (i = 0u; i < 8u; ++i) in(0u, 0u, 0u, 0u); /* 400 ms */
        CHECK(e->project.steps[2].gate == MELLO_GATE_SINGLE, "a long key made a held note by itself (%d)",
              e->project.steps[2].gate);
        knob(0u, 1); /* K1: PULSE 2 */
        knob(2u, 2); /* K3: GATE SINGLE, MULTI, HOLD */
        mello_ui_set_held_notes(&ui, 0u);
        in(0u, 0u, 0u, 0u);
        CHECK(e->project.steps[2].pulses == 2u && e->project.steps[2].gate == MELLO_GATE_HOLD,
              "the held key's step: PULSE %u, GATE %d", e->project.steps[2].pulses, e->project.steps[2].gate);
        key(9u);
        CHECK(e->project.steps[3].pulses == 2u && e->project.steps[3].gate == MELLO_GATE_HOLD,
              "the next step did not get the same (PULSE %u, GATE %d)", e->project.steps[3].pulses,
              e->project.steps[3].gate);
        knob(0u, -1), knob(2u, -2); /* nothing held: what the next steps get, back to PULSE 1, SINGLE */
        CHECK(e->project.steps[3].pulses == 2u, "with no key held, K1 changed the step before");
        select_turn(-2); /* SELECT: the cursor back two, to step 3 */
        CHECK(ui.keys_rec_pos == 2u && ui.selected_step == 2u, "SELECT did not move the cursor (next %u)",
              ui.keys_rec_pos + 1u);
        key(10u); /* step 3 written again */
        CHECK(e->project.steps[2].pulses == 1u && e->project.steps[2].gate == MELLO_GATE_SINGLE &&
                  ui.keys_rec_pos == 3u,
              "the key at the cursor did not overwrite step 3 as set (PULSE %u)", e->project.steps[2].pulses);
        tap_button(MELLO_UI_BTN_REC); /* BASS */
        key(7u);
        tap_button(MELLO_UI_BTN_SEL);
        CHECK(!e->bass.steps[1].on, "SEL: a bass rest");
        mello_ui_set_held_notes(&ui, 1u << 14);
        in(0u, 0u, 0u, 1u << 14);
        knob(0u, 1); /* K1: GLIDE on */
        mello_ui_set_held_notes(&ui, 0u);
        in(0u, 0u, 0u, 0u);
        CHECK(e->bass.steps[2].glide, "K1 with the bass key held: no glide");
#if MELLO_FEATURE_CHORD_MACHINE
        tap_button(MELLO_UI_BTN_REC); /* CHORD: a chord held, K1 its HOLD, K3 its inversion */
        mello_ui_set_held_notes(&ui, 1u << 7);
        in(0u, 0u, 0u, 1u << 7);
        knob(0u, 1); /* HOLD 8 -> 16 */
        knob(2u, 1); /* INV 1 */
        mello_ui_set_held_notes(&ui, 0u);
        in(0u, 0u, 0u, 0u);
        CHECK(e->chord.steps[0].hold == 16u && e->chord.steps[0].inversion == 1u,
              "a held chord's K1/K3: HOLD %u, INV %u", e->chord.steps[0].hold, e->chord.steps[0].inversion);
        key(12u); /* the next chord: HOLD 16 as set, no inversion */
        CHECK(e->chord.steps[1].hold == 16u && e->chord.steps[1].inversion == 0u, "the next chord's HOLD %u",
              e->chord.steps[1].hold);
#endif
    }

    { /* playing: a key kept down carries the note on to the steps the clock reaches */
        uint32_t pulse_us = 60000000u / e->project.bpm / 4u, us, end;
        uint8_t first;
        int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
        mello_project_defaults(&p);
        for (i = 0u; i < 16u; ++i) p.steps[i].pitch = 0, p.steps[i].octave = 0;
        mello_transport_init(&t, &p, 1u, midi, NULL);
        mello_ui_init(&ui, &t, &st);
        e = &t.engine;
        tap_button(MELLO_UI_BTN_EDIT);
        tap_button(MELLO_UI_BTN_REC);
        mello_transport_start(&t, ms * 1000u);
        us = ms * 1000u;
        for (end = us + 4u * pulse_us + pulse_us / 4u; us < end; us += 1000u)
            mello_transport_poll_ahead(&t, us, MELLO_TRANSPORT_LOOKAHEAD_US);
        mello_ui_set_held_notes(&ui, 1u << 14);
        mello_ui_input(&ui, us, us / 1000u, 0u, 0u, 0u, 1u << 14, enc);
        first = ui.rec_key_pos;
        for (end = us + 3u * pulse_us; us < end; us += 1000u) {
            mello_transport_poll_ahead(&t, us, MELLO_TRANSPORT_LOOKAHEAD_US);
            mello_ui_input(&ui, us, us / 1000u, 0u, 0u, 0u, 0u, enc);
        }
        mello_ui_set_held_notes(&ui, 0u);
        CHECK(ui.rec_key_pos != first, "the note moved on with the clock (still step %u)", first + 1u);
        CHECK(e->project.steps[first].gate == MELLO_GATE_HOLD, "the first step is held");
        CHECK(mello_resolve_step_midi(&e->project, &e->project.steps[(first + 1u) % 16u]) == 67u &&
                  e->project.steps[(first + 1u) % 16u].gate == MELLO_GATE_HOLD &&
                  e->project.steps[(first + 1u) % 16u].slide,
              "the next step is the same note, held and slid into");
    }

    printf("PASS: the key rule rounds and lights the scale; REC cycles SEQ, BASS, CHORD; stopped it steps, "
           "playing it follows the clock; UNDO takes it back\n");
    return 0;
}
