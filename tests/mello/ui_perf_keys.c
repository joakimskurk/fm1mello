/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Phase 3: HOME's performance keys (DECISIONS.md "Performance keys").
 * - Held on HOME, FILL / SUB MUTE / REVERSE / REPEAT reach the engine; let go, they stop; a white key while REPEAT is
 *   held sets its length (and edits no step); RESTART goes to step 1 at the next pulse.
 * - FILL plays every step (chance 0 and rests too) with doubled ratchets; SUB MUTE stops new bass notes and
 *   mello_audio fades the sounding bass in ~10 ms.
 * - REVERSE plays what was heard backwards, REPEAT the last n pulses round and round; let go, the sequence is where it
 *   would have been without them (it keeps time). */
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"
#include "mello_ui.h"

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fputc('\n', stderr);                                                                             \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

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
static uint8_t white_note(uint8_t pos)
{
    uint8_t i;
    for (i = 0u; i < 27u; ++i)
        if (mello_ui_white_key_position(i) == (int8_t)pos) return i;
    return 0xFFu;
}

/* One pulse: the step it played, how many lead notes and bass notes it started. */
static uint8_t pulse(MelloEngine *e, unsigned *notes, unsigned *bass)
{
    MelloEvent ev;
    uint8_t step = 0xFFu;
    (void)mello_engine_advance_pulse(e);
    while (mello_event_pop(e, &ev)) {
        if (ev.type == MELLO_EVENT_PULSE) step = ev.step_index;
        if (ev.type == MELLO_EVENT_NOTE_ON && notes) ++*notes;
        if (ev.type == MELLO_EVENT_SUB_NOTE_ON && bass) ++*bass;
    }
    return step;
}

static int ui_part(void)
{
    static MelloProject p;
    static MelloTransport t;
    static MelloStorage st;
    static MelloUi u;
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    uint32_t fill = 1u << black_note(5u), subm = 1u << black_note(6u), rep = 1u << black_note(8u),
             w2 = 1u << white_note(1u);
    MelloStep before;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 1u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_HOME);

    mello_ui_set_held_notes(&u, fill | subm);
    mello_ui_input(&u, 1000u, 1u, 0u, 0u, 0u, fill | subm, enc);
    CHECK(t.engine.perf.fill && t.engine.perf.sub_mute && !t.engine.perf.repeat,
          "held FILL + SUB MUTE did not act");
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 2000u, 2u, 0u, 0u, 0u, 0u, enc);
    CHECK(!t.engine.perf.fill && !t.engine.perf.sub_mute, "let go, FILL / SUB MUTE stayed on");

    before = t.engine.project.steps[1];
    mello_ui_set_held_notes(&u, rep);
    mello_ui_input(&u, 3000u, 3u, 0u, 0u, 0u, rep, enc);
    mello_ui_set_held_notes(&u, rep | w2);
    mello_ui_input(&u, 4000u, 4u, 0u, 0u, 0u, w2, enc);
    CHECK(t.engine.perf.repeat && t.engine.perf.repeat_len == 2u,
          "REPEAT + white key 2 is not a 2-pulse repeat (%u)", t.engine.perf.repeat_len);
    CHECK(u.held_step == MELLO_UI_NO_STEP && !memcmp(&before, &t.engine.project.steps[1], sizeof before),
          "the white key under REPEAT edited a step");

    mello_ui_set_held_notes(&u, 0u);
    mello_ui_set_page(&u, MELLO_UI_SOUND_RESERVED); /* R3: B5-B8 are the performance keys on every page */
    mello_ui_set_held_notes(&u, fill);
    mello_ui_input(&u, 5000u, 5u, 0u, 0u, 0u, fill, enc);
    CHECK(t.engine.perf.fill && !t.engine.perf.repeat, "FILL did not act off HOME");
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 6000u, 6u, 0u, 0u, 0u, 0u, enc);
    return 0;
}

static int engine_part(void)
{
    static MelloEngine e, ref;
    MelloProject p;
    MelloPerf perf;
    unsigned notes, bass, i;
    uint8_t heard[8], got;

    mello_project_defaults(&p);
    for (i = 0u; i < 16u; ++i) p.steps[i].sub = true, p.steps[i].ratchet = 1u, p.steps[i].pulses = 1u;
    p.steps[2].probability = 0u;       /* never plays ... */
    p.steps[3].gate = MELLO_GATE_REST; /* ... a rest ... */
    mello_engine_init(&e, &p, 7u);
    mello_engine_init(&ref, &p, 7u);
    mello_engine_start(&e);
    mello_engine_start(&ref);
    memset(&perf, 0, sizeof perf);
    perf.repeat_len = 4u;

    for (i = 0u; i < 8u; ++i) { /* the reference's first 8 pulses, and ours with FILL from step 3 */
        heard[i] = pulse(&ref, 0, 0);
        if (i == 2u) perf.fill = true, mello_engine_set_perf(&e, &perf);
        notes = bass = 0u;
        got = pulse(&e, &notes, &bass);
        CHECK(got == heard[i], "FILL moved the sequence");
        if (i == 2u || i == 3u)
            CHECK(notes >= 2u, "FILL did not play step %u with two ratchets (%u notes)", i + 1u, notes);
    }
    perf.fill = false;
    perf.sub_mute = true;
    mello_engine_set_perf(&e, &perf);
    bass = 0u;
    (void)pulse(&ref, 0, 0), (void)pulse(&e, 0, &bass);
    CHECK(!bass, "SUB MUTE let a bass note through");
    perf.sub_mute = false;

    perf.reverse = true; /* steps 1..9 heard; REVERSE plays 8, 7, 6 ... */
    mello_engine_set_perf(&e, &perf);
    for (i = 0u; i < 4u; ++i) {
        (void)pulse(&ref, 0, 0);
        got = pulse(&e, 0, 0);
        CHECK(got == (uint8_t)(7u - i), "REVERSE played step %u, not %u", got + 1u, 8u - i);
    }
    perf.reverse = false;
    mello_engine_set_perf(&e, &perf);
    CHECK(pulse(&e, 0, 0) == pulse(&ref, 0, 0), "let go of REVERSE, the sequence was not in time");

    perf.repeat = true; /* the last 2 pulses, round and round */
    perf.repeat_len = 2u;
    mello_engine_set_perf(&e, &perf);
    {
        uint8_t a = pulse(&e, 0, 0), b = pulse(&e, 0, 0);
        (void)pulse(&ref, 0, 0), (void)pulse(&ref, 0, 0);
        for (i = 0u; i < 4u; ++i) {
            (void)pulse(&ref, 0, 0);
            CHECK(pulse(&e, 0, 0) == (i & 1u ? b : a), "REPEAT 2 did not go round its two pulses");
        }
        CHECK(a != b, "REPEAT 2 repeated one pulse");
    }
    perf.repeat = false;
    mello_engine_set_perf(&e, &perf);
    CHECK(pulse(&e, 0, 0) == pulse(&ref, 0, 0), "let go of REPEAT, the sequence was not in time");

    mello_engine_perf_restart(&e);
    CHECK(pulse(&e, 0, 0) == 0u && e.absolute_pulse == 1u, "RESTART did not go to step 1 and the loop's one");
    return 0;
}

static int audio_part(void)
{
    static MelloAudio a;
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    int32_t out[2 * 44];
    unsigned ms;
    mello_sound_defaults(&s);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sm);
    mello_audio_init(&a, 0, 0u, 0, 0u);
    mello_audio_set_config(&a, &s, &fx, &sub, &sm);
    mello_audio_set_sub_mute(&a, true);
    for (ms = 0u; ms < 15u; ++ms) mello_audio_render(&a, out, 44u, ms * 1000u, 120u);
    CHECK(a.sub_gain == 0, "SUB MUTE did not fade the bass out in 15 ms (%d)", (int)a.sub_gain);
    mello_audio_set_sub_mute(&a, false);
    mello_audio_render(&a, out, 44u, 20000u, 120u);
    CHECK(a.sub_gain > 0 && a.sub_gain < 32767, "the bass did not fade back in");
    return 0;
}

int main(void)
{
    if (ui_part() || engine_part() || audio_part()) return 1;
    puts("PASS: HOME's performance keys: held FILL / SUB MUTE / REVERSE / REPEAT (+ white key), RESTART; in "
         "time");
    return 0;
}
