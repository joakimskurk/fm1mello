/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* SUB DRONE (feedback 2026-10-30: "a mode where the sub just drones, always on while playing"). The sub is gated and
 * at full level from the first step to STOP, through rests and skips; a new pitch (OCT here) moves the same note; SUB
 * MUTE silences it and lets it back; leaving DRONE ends it, and so does a stop. */
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

/* Runs ms milliseconds as dinghy does; counts the milliseconds the sub was gated, and the lowest level it had then. */
static unsigned run(uint32_t from_ms, uint32_t ms, int32_t *lowest)
{
    int32_t out[2 * 44];
    unsigned gated = 0u;
    uint32_t i;
    for (i = from_ms; i < from_ms + ms; ++i) {
        mello_transport_poll_ahead(&t, i * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
        mello_audio_render(&a, out, 44u, i * 1000u, t.engine.project.bpm);
        if (a.sub.gate) {
            gated++;
            if (lowest && a.sub.env_q24 < *lowest) *lowest = a.sub.env_q24;
        }
    }
    return gated;
}

static void apply(void)
{
    mello_audio_set_config(&a, &t.engine.sound, &t.engine.fx, &t.engine.sub, &t.engine.sample);
}

int main(void)
{
    MelloProject p;
    MelloPerf perf;
    unsigned i, gated;
    int32_t lowest = 1 << 30;
    uint8_t note;
    mello_project_defaults(&p);
    for (i = 0u; i < 16u; ++i) p.steps[i].gate = (i % 4u) ? MELLO_GATE_REST : MELLO_GATE_SINGLE;
    mello_transport_init(&t, &p, 1u, 0, 0);
    t.engine.sub.on = true;
    t.engine.sub.mode = MELLO_SUB_MODE_DRONE;
    mello_audio_init(&a, 0, 0u, 0, 0u);
    apply();
    mello_transport_set_event_sink(&t, sink, &a);
    mello_transport_start(&t, 0u);

    (void)run(0u, 200u, 0); /* the attack and the first decay */
    gated = run(200u, 4000u, &lowest);
    printf("DRONE: the sub gated %u of 4000 ms, lowest level %d / %d\n", gated, (int)lowest, 32767 << 8);
    if (gated != 4000u || lowest < (32767 << 8) / 10 * 9) {
        fprintf(stderr, "FAIL: the drone was not held at full level\n");
        return 1;
    }

    note = a.sub.note; /* a new pitch moves the drone, which stays on */
    t.engine.sub.octave = (int8_t)(t.engine.sub.octave + 1);
    gated = run(4200u, 2000u, 0);
    printf("OCT +1: the note %u -> %u, gated %u of 2000 ms\n", note, a.sub.note, gated);
    if (a.sub.note != note + 12u || gated != 2000u) {
        fprintf(stderr, "FAIL: the drone did not follow its pitch, or stopped\n");
        return 1;
    }

    memset(&perf, 0, sizeof perf); /* SUB MUTE, as the performance key: silent while held */
    perf.sub_mute = true;
    mello_engine_set_perf(&t.engine, &perf);
    mello_audio_set_sub_mute(&a, true);
    (void)run(6200u, 500u, 0);
    if (a.sub_gain > 0) {
        fprintf(stderr, "FAIL: SUB MUTE did not silence the drone (%d)\n", (int)a.sub_gain);
        return 1;
    }
    perf.sub_mute = false;
    mello_engine_set_perf(&t.engine, &perf);
    mello_audio_set_sub_mute(&a, false);
    gated = run(6700u, 500u, 0);
    if (gated != 500u || a.sub_gain < 32000) {
        fprintf(stderr, "FAIL: the drone did not come back after SUB MUTE\n");
        return 1;
    }

    t.engine.sub.mode = MELLO_SUB_MODE_STEP; /* leaving DRONE (no step has SUB on) ends it */
    apply();
    (void)run(7200u, 1000u, 0);
    if (a.sub.gate) {
        fprintf(stderr, "FAIL: the drone sounds after leaving DRONE\n");
        return 1;
    }

    t.engine.sub.mode = MELLO_SUB_MODE_DRONE; /* back to DRONE, then STOP */
    apply();
    gated = run(8200u, 1000u, 0);
    if (gated < 800u) {
        fprintf(stderr, "FAIL: the drone did not start again (%u ms)\n", gated);
        return 1;
    }
    mello_transport_stop(&t);
    mello_audio_all_notes_off(&a);
    (void)run(9200u, 10u, 0);
    if (a.sub.gate) {
        fprintf(stderr, "FAIL: the drone is gated after a stop\n");
        return 1;
    }
    puts("PASS: DRONE holds the sub from START to STOP, follows its pitch, mutes, and ends when left");
    return 0;
}
