/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Phase 3: the performance keys (DECISIONS.md "Performance keys"), since 2026-11-03 on PERFORM (G4):
 * - B5 PERF tapped opens PERFORM until PERF or a page button, held while held; its white keys: FILL and REPEAT while
 *   held (no step edited), the mutes latched (stopped, at once; playing, on the beat: perform_keys), MIX's mutes kept
 *   while an action is held; B6 MUT mutates on any page. (The engine's RESTART stays, off PERFORM.)
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
    uint32_t perf = 1u << black_note(10u), mut = 1u << black_note(6u), fill = 1u << white_note(0u),
             rep4 = 1u << white_note(5u), lead = 1u << white_note(12u);
    MelloStep before;
    MelloStep steps[MELLO_MAX_STEPS];
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 1u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_HOME);

    /* G4: B5 PERF tapped opens PERFORM, which stays */
    mello_ui_set_held_notes(&u, perf);
    mello_ui_input(&u, 1000u, 1u, 0u, 0u, 0u, perf, enc);
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 101000u, 101u, 0u, 0u, 0u, 0u, enc);
    CHECK(u.perform == 1u, "a tap on PERF did not keep PERFORM open (%u)", u.perform);

    /* its white keys: FILL while held, REPEAT 4 while held (and no step edited), LEAD's mute latched */
    mello_ui_set_held_notes(&u, fill);
    mello_ui_input(&u, 200000u, 200u, 0u, 0u, 0u, fill, enc);
    CHECK(t.engine.perf.fill && !t.engine.perf.repeat, "PERFORM's FILL held did not act");
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 300000u, 300u, 0u, 0u, 0u, 0u, enc);
    CHECK(!t.engine.perf.fill, "let go, FILL stayed on");
    before = t.engine.project.steps[3];
    mello_ui_set_held_notes(&u, rep4);
    mello_ui_input(&u, 400000u, 400u, 0u, 0u, 0u, rep4, enc);
    CHECK(t.engine.perf.repeat && t.engine.perf.repeat_len == 4u,
          "PERFORM's REP 4 is not a 4-pulse repeat (%u)", t.engine.perf.repeat_len);
    CHECK(u.held_step == MELLO_UI_NO_STEP && !memcmp(&before, &t.engine.project.steps[3], sizeof before),
          "a PERFORM key edited a step");
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 500000u, 500u, 0u, 0u, 0u, 0u, enc);
    CHECK(!t.engine.perf.repeat, "let go, REPEAT stayed on");
    mello_ui_set_held_notes(&u, lead);
    mello_ui_input(&u, 600000u, 600u, 0u, 0u, 0u, lead, enc);
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 700000u, 700u, 0u, 0u, 0u, 0u, enc);
    CHECK((t.engine.perf.mix_mute & 1u) && !t.engine.perf.fill, "PERFORM's LEAD did not mute the lead");
    mello_ui_set_held_notes(&u, fill); /* a held action keeps MIX's mutes */
    mello_ui_input(&u, 800000u, 800u, 0u, 0u, 0u, fill, enc);
    CHECK(t.engine.perf.mix_mute & 1u, "holding FILL cleared the lead's mute");
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 900000u, 900u, 0u, 0u, 0u, 0u, enc);

    /* a page button closes PERFORM and goes to its page */
    mello_ui_input(&u, 1000000u, 1000u, 1u << MELLO_UI_BTN_ENV, 0u, 0u, 0u, enc);
    CHECK(!u.perform && u.page == MELLO_UI_SOUND_RESERVED, "ENV did not close PERFORM (%u, page %d)",
          u.perform, (int)u.page);

    /* PERF held long: PERFORM while held, closed when let go */
    mello_ui_set_held_notes(&u, perf);
    mello_ui_input(&u, 1100000u, 1100u, 0u, 0u, 0u, perf, enc);
    CHECK(u.perform == 2u, "PERF held did not open PERFORM");
    mello_ui_input(&u, 1800000u, 1800u, 0u, 0u, 0u, 0u, enc);
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 1900000u, 1900u, 0u, 0u, 0u, 0u, enc);
    CHECK(!u.perform, "PERF held and let go did not close PERFORM");

    /* B6 MUT: MUTATE on any page (here SOUND) */
    memcpy(steps, t.engine.project.steps, sizeof steps);
    mello_ui_set_held_notes(&u, mut);
    mello_ui_input(&u, 2000000u, 2000u, 0u, 0u, 0u, mut, enc);
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 2100000u, 2100u, 0u, 0u, 0u, 0u, enc);
    CHECK(memcmp(steps, t.engine.project.steps, sizeof steps), "B6 MUT did not mutate off HOME");
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
    puts("PASS: PERFORM (PERF tapped or held, its white keys, B6 MUT) and the performance actions; in time");
    return 0;
}
