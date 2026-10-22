#include "mello_audio.h"
#include <stdio.h>
#include <string.h>

#if MELLO_FEATURE_CHORD_MACHINE
static int push_ev(MelloAudio *a, MelloEventType type, uint8_t note, uint32_t due)
{
    MelloEvent e;
    memset(&e, 0, sizeof e);
    e.type = type;
    e.note = note;
    e.velocity = 100u;
    if (!mello_audio_push_engine_event(a, &e, due)) {
        fprintf(stderr, "queue rejected type=%d note=%u due=%u\n", (int)type, (unsigned)note, (unsigned)due);
        return 0;
    }
    return 1;
}
#endif

int main(void)
{
#if MELLO_FEATURE_CHORD_MACHINE
    MelloAudio a;
    MelloChordConfig chord;
    int32_t out[2] = {0, 0};

    mello_audio_init(&a, 0, 0u, 0, 0u);
    mello_chord_defaults(&chord);
    chord.on = true;
    chord.attack_ms = 1u;
    mello_audio_set_chord_config(&a, &chord);

    /* Reproduce the original bug exactly: pairwise chord events put a future OFF between
     * current ONs, then a current lead ON arrives after those future events. All current
     * attacks must become visible on the first rendered frame. */
    if(!push_ev(&a, MELLO_EVENT_CHORD_NOTE_ON, 60u, 1000u))return 10;
    if(!push_ev(&a, MELLO_EVENT_CHORD_NOTE_OFF, 60u, 5000u))return 11;
    if(!push_ev(&a, MELLO_EVENT_CHORD_NOTE_ON, 64u, 1000u))return 12;
    if(!push_ev(&a, MELLO_EVENT_CHORD_NOTE_OFF, 64u, 5000u))return 13;
    if(!push_ev(&a, MELLO_EVENT_CHORD_NOTE_ON, 67u, 1000u))return 14;
    if(!push_ev(&a, MELLO_EVENT_CHORD_NOTE_OFF, 67u, 5000u))return 15;
    if(!push_ev(&a, MELLO_EVENT_CHORD_NOTE_ON, 71u, 1000u))return 16;
    if(!push_ev(&a, MELLO_EVENT_CHORD_NOTE_OFF, 71u, 5000u))return 17;
    if(!push_ev(&a, MELLO_EVENT_NOTE_ON, 72u, 1000u))return 18;
    mello_audio_render(&a, out, 1u, 1000u, 120u);
    if (a.chord.active_count != 4u) {
        fprintf(stderr, "chord onset split by future OFF: active=%u\n", (unsigned)a.chord.active_count);
        return 1;
    }
    if (a.lead.env_stage == MELLO_ENV_OFF || a.lead.env_q24 <= 0) {
        fprintf(stderr, "lead onset blocked behind chord future event\n");
        return 2;
    }
    if (mello_audio_queue_overflowed(&a)) return 3;

    /* At the future deadline all chord releases must become visible, with no stale lane. */
    mello_audio_render(&a, out, 1u, 5000u, 120u);
    if (a.chord.active_count != 0u) {
        fprintf(stderr, "chord releases did not land together: active=%u\n", (unsigned)a.chord.active_count);
        return 4;
    }
    puts("PASS: future chord releases cannot head-of-line block current chord/lead attacks");
#else
    puts("PASS: chord timing queue smoke skipped with chord feature compiled out");
#endif
    return 0;
}
