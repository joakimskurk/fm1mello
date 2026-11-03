/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* REC on every page with a sequence (G3, ROADMAP F5, DECISIONS "REC where you are"). On STEP, BASS and CHORD the REC
 * button makes the keys a keyboard writing that page's part from its picked step, stopped one step a key, each key
 * heard in the part's own voice; on DRUMS white keys 1-4 are KICK, SNARE, HAT, GLITCH (a key held with another, the
 * same step) and 5-8 the last hit's locks. REC again, another page or PLAY's stop ends it; one UNDO takes it back. */
#include "mello_engine.h"
#include "mello_storage.h"
#include "mello_transport.h"
#include "mello_ui.h"
#include <stdio.h>
#include <string.h>

static void midi(void *c, uint8_t a, uint8_t b, uint8_t d)
{
    (void)c, (void)a, (void)b, (void)d;
}
static MelloEventType heard[16];
static unsigned heard_n;
static void sink(void *ctx, const MelloEvent *ev, uint32_t due)
{
    (void)ctx, (void)due;
    if (ev->step_index == 0xFFu || ev->type == MELLO_EVENT_DRUM_HIT)
        if (heard_n < 16u) heard[heard_n++] = ev->type;
}

static MelloTransport t;
static MelloStorage st;
static MelloUi ui;
static uint32_t ms = 1000u;

static void in(uint32_t pressed, uint32_t notes)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    ms += 50u;
    mello_ui_input(&ui, ms * 1000u, ms, pressed, 0u, pressed, notes, enc);
}
static void tap(uint8_t b)
{
    in(1u << b, 0u);
    in(0u, 0u);
}
static void key(uint8_t note_index) /* MIDI 53 + note_index at OCT 0 */
{
    mello_ui_set_held_notes(&ui, 1u << note_index);
    in(0u, 1u << note_index);
    mello_ui_set_held_notes(&ui, 0u);
    in(0u, 0u);
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

int main(void)
{
    static MelloProject p;
    MelloEngine *e;
    MelloStep before;

    mello_project_defaults(&p); /* C minor on 60 */
    mello_transport_init(&t, &p, 1u, midi, NULL);
    mello_transport_set_event_sink(&t, sink, NULL);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&ui, &t, &st);
    e = &t.engine;

    /* STEP: from the picked step, one step a key, heard as the lead */
    mello_ui_set_page(&ui, MELLO_UI_SEQ_STEP);
    ui.selected_step = 2u;
    before = e->project.steps[2];
    tap(MELLO_UI_BTN_REC);
    CHECK(ui.page_rec == MELLO_UI_SEQ_STEP && ui.keys_rec && ui.keys_rec_target == MELLO_UI_REC_SEQ,
          "REC on STEP did not record the sequence");
    heard_n = 0u;
    key(9u);  /* D4 (62): the second degree */
    key(12u); /* F4 (65): the fourth */
    CHECK(e->project.steps[2].pitch == 1 && e->project.steps[3].pitch == 3,
          "STEP REC wrote %d, %d at steps 3-4, not 1, 3", e->project.steps[2].pitch, e->project.steps[3].pitch);
    CHECK(heard_n >= 2u && heard[0] == MELLO_EVENT_NOTE_ON, "a key on STEP was not heard as the lead");
    CHECK(mello_engine_undo_swap(e) && !memcmp(&e->project.steps[2], &before, sizeof before),
          "UNDO did not take the take back");
    tap(MELLO_UI_BTN_REC);
    CHECK(!ui.page_rec && !ui.keys_rec, "REC again did not stop");

    /* BASS: the line from its picked step, heard as the bass */
    mello_ui_set_page(&ui, MELLO_UI_SUB);
    ui.selected_sub_x_step = 0u;
    tap(MELLO_UI_BTN_REC);
    heard_n = 0u;
    key(9u); /* D: the second tone */
    CHECK(e->bass.on && e->bass.steps[0].on && e->bass.steps[0].tone == 1u, "BASS REC wrote tone %u (on %d)",
          e->bass.steps[0].tone, (int)e->bass.steps[0].on);
    CHECK(heard_n >= 1u && heard[0] == MELLO_EVENT_SUB_NOTE_ON, "a key on BASS was not heard as the bass");

    /* another page ends it */
    mello_ui_set_page(&ui, MELLO_UI_HOME);
    CHECK(!ui.page_rec && !ui.keys_rec, "leaving BASS did not end REC");

#if MELLO_FEATURE_CHORD_MACHINE
    /* CHORD: the chain from its picked chord */
    mello_ui_set_page(&ui, MELLO_UI_CHORD);
    ui.selected_chord_step = 0u;
    tap(MELLO_UI_BTN_REC);
    heard_n = 0u;
    key(12u); /* F: the fourth degree */
    CHECK(e->chord.steps[0].degree == 3u, "CHORD REC wrote degree %u", e->chord.steps[0].degree);
    CHECK(heard_n >= 1u && heard[0] == MELLO_EVENT_CHORD_NOTE_ON, "a key on CHORD was not heard as the chords");
    mello_ui_set_page(&ui, MELLO_UI_HOME);
#endif

    /* DRUMS: 1-4 the voices, a key held with another the same step; 5-8 the last hit's locks */
    {
        mello_ui_set_page(&ui, MELLO_UI_DRUMS);
        ui.selected_drum_step = 0u;
        e->drums.hits[MELLO_DRUM_HAT] = 0u; /* (the default beat's hats) */
        tap(MELLO_UI_BTN_REC);
        CHECK(ui.keys_rec_target == MELLO_UI_REC_DRUMS, "REC on DRUMS did not record the drums");
        heard_n = 0u;
        key(2u); /* white 2: SNARE at step 1 */
        key(0u); /* white 1: KICK at step 2 */
        mello_ui_set_held_notes(&ui, 1u << 0u); /* KICK held (no new hit: held, not pressed), HAT pressed */
        mello_ui_set_held_notes(&ui, (1u << 0u) | (1u << 4u));
        in(0u, 1u << 4u);
        mello_ui_set_held_notes(&ui, 0u);
        in(0u, 0u);
        key(7u); /* white 5: ACCENT on the last hit (the hat) */
        key(9u); /* white 6: RATCH 2 */
        CHECK((e->drums.hits[MELLO_DRUM_SNARE] & 1u) && (e->drums.hits[MELLO_DRUM_KICK] & 2u),
              "DRUMS REC: SNARE %04x, KICK %04x", e->drums.hits[MELLO_DRUM_SNARE], e->drums.hits[MELLO_DRUM_KICK]);
        CHECK((e->drums.hits[MELLO_DRUM_HAT] & 2u) && !(e->drums.hits[MELLO_DRUM_HAT] & 4u),
              "DRUMS REC: the hat with the kick held is not at the kick's step (%04x)",
              e->drums.hits[MELLO_DRUM_HAT]);
        CHECK((e->drums.accents[MELLO_DRUM_HAT] & 2u) && e->drums.lock[MELLO_DRUM_HAT][1].ratchet == 2u,
              "DRUMS REC: the hat's ACCENT %04x, RATCH %u", e->drums.accents[MELLO_DRUM_HAT],
              e->drums.lock[MELLO_DRUM_HAT][1].ratchet);
        CHECK(heard_n >= 3u && heard[0] == MELLO_EVENT_DRUM_HIT, "a drum key was not heard");
    }

    /* PLAY's stop ends a take */
    tap(MELLO_UI_BTN_PLAY);
    tap(MELLO_UI_BTN_PLAY);
    CHECK(!ui.page_rec, "stopping did not end REC");

    puts("PASS: rec_pages");
    return 0;
}
