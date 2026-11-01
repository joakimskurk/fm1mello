/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Regression (feedback 2026-10-30: "playback stops and the last note hangs for ever"). A HOLD step over several
 * pulses sent its NOTE_ON at pulse 0 and its NOTE_OFF at the last pulse, tagged with that pulse; mello_audio releases
 * only on the OFF of the note it is playing (event_tag, so that legato notes are not cut), so the OFF was ignored and
 * the note sounded until another note came: after it rests or skips, for ever. Also checked: REVERSE and REPEAT, which
 * replay pulses out of order, never leave a HOLD note on, and a stop releases everything. */
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"
#include "mello_transport.h"

static MelloAudio a;
static MelloTransport t;
static void sink(void *ctx, const MelloEvent *e, uint32_t due)
{
    (void)mello_audio_push_engine_event((MelloAudio *)ctx, e, due);
}

/* Runs ms milliseconds as dinghy does (transport ahead, then the sound); returns the longest the lead was gated. */
static unsigned run(uint32_t from_ms, uint32_t ms, const MelloPerf *perf)
{
    int32_t out[2 * 44];
    unsigned longest = 0u, on = 0u;
    uint32_t i;
    for (i = from_ms; i < from_ms + ms; ++i) {
        if (perf) mello_engine_set_perf(&t.engine, perf);
        mello_transport_poll_ahead(&t, i * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
        mello_audio_render(&a, out, 44u, i * 1000u, t.engine.project.bpm);
        on = a.lead.gate ? on + 1u : 0u;
        if (on > longest) longest = on;
    }
    return longest;
}

int main(void)
{
    MelloProject p;
    MelloPerf perf;
    unsigned i, longest, step_ms;
    mello_project_defaults(&p);
    for (i = 0u; i < 16u; ++i) p.steps[i].gate = MELLO_GATE_REST;
    p.steps[0].gate = MELLO_GATE_HOLD; /* one note held over three pulses, then rests */
    p.steps[0].pulses = 3u;
    p.steps[8].gate = MELLO_GATE_HOLD;
    p.steps[8].pulses = 2u;
    mello_transport_init(&t, &p, 1u, 0, 0);
    mello_audio_init(&a, 0, 0u, 0, 0u);
    mello_audio_set_config(&a, &t.engine.sound, &t.engine.fx, &t.engine.sub, &t.engine.sample);
    mello_transport_set_event_sink(&t, sink, &a);
    step_ms = 60000u / t.engine.project.bpm / 4u;
    mello_transport_start(&t, 0u);

    longest = run(0u, 6000u, 0);
    printf("HOLD over 3 pulses (%u ms each): the lead gated for at most %u ms\n", step_ms, longest);
    if (longest > 3u * step_ms + 20u) {
        fprintf(stderr, "FAIL: a HOLD note was not released (%u ms)\n", longest);
        return 1;
    }

    memset(&perf, 0, sizeof perf); /* REVERSE, then REPEAT 2, over the held notes */
    perf.repeat_len = 2u;
    perf.reverse = true;
    longest = run(6000u, 3000u, &perf);
    perf.reverse = false, perf.repeat = true;
    i = run(9000u, 3000u, &perf);
    if (i > longest) longest = i;
    perf.repeat = false; /* and back to the sequence */
    i = run(12000u, 3000u, &perf);
    if (i > longest) longest = i;
    printf("REVERSE / REPEAT over HOLD notes: the lead gated for at most %u ms\n", longest);
    if (longest > 3u * step_ms + 20u) {
        fprintf(stderr, "FAIL: REVERSE / REPEAT left a HOLD note on (%u ms)\n", longest);
        return 1;
    }

    { /* an edit in the middle of a HOLD (as a roll does): its last pulse goes, and the note must still end */
        unsigned at;
        for (at = 15000u;; ++at) { /* to where step 1's HOLD has started */
            (void)run(at, 1u, 0);
            if (a.lead.gate && t.engine.step_index == 0u && t.engine.pulse_index == 1u) break;
            if (at > 30000u) {
                fprintf(stderr, "FAIL: step 1 never played\n");
                return 1;
            }
        }
        t.engine.project.steps[0].gate = MELLO_GATE_SINGLE;
        t.engine.project.steps[0].pulses = 1u;
        (void)mello_engine_rebuild_sequence(&t.engine);
        longest = run(at + 1u, 3000u, 0);
        printf("a HOLD cut short by an edit: the lead gated for at most %u ms after it\n", longest);
        if (longest > 3u * step_ms) {
            fprintf(stderr, "FAIL: an edited HOLD note was left on (%u ms)\n", longest);
            return 1;
        }
    }

    mello_transport_stop(&t); /* a stop releases everything (dinghy: all notes off on the running edge) */
    mello_audio_all_notes_off(&a);
    (void)run(40000u, 10u, 0);
    if (a.lead.gate) {
        fprintf(stderr, "FAIL: the lead is gated after a stop\n");
        return 1;
    }
    puts("PASS: HOLD notes are released; REVERSE / REPEAT leave none on; a stop releases the lead");
    return 0;
}
