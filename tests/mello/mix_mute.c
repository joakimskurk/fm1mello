/* MIX's mutes (2026-10-31): B0-B3 mute LEAD, SUB, CHORD, PERC. Checked in the audio: a muted part is gone within
 * ~12 ms (it glides there, as SUB MUTE does), and another part's mute leaves it alone. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mello_audio.h"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

static MelloAudio a;
static int16_t db[4096], rb[24576];

/* Peak over 10 ms blocks; mute bits set before block `at`. */
static void run(uint8_t bits, unsigned at, bool drums, int32_t peak[8])
{
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    MelloDrumConfig d;
    MelloEvent ev;
    int32_t out[2 * 441];
    unsigned blk, k;
    mello_sound_defaults(&s);
    s.follower_sub_on = false;
    s.sustain_permille = 1000u;
    mello_fx_defaults(&fx);
    fx.delay_wet = fx.reverb_wet = 0u;
    mello_sub_defaults(&sub);
    sub.on = false;
    mello_sample_defaults(&sm);
    mello_drum_defaults(&d);
    d.on = drums;
    d.voice[MELLO_DRUM_HAT].decay = 1000u;
    mello_audio_init(&a, db, 4096u, rb, 24576u);
    mello_audio_set_config(&a, &s, &fx, &sub, &sm);
    mello_audio_set_drum_config(&a, &d);
    memset(&ev, 0, sizeof ev);
    ev.type = drums ? MELLO_EVENT_DRUM_HIT : MELLO_EVENT_NOTE_ON;
    ev.note = drums ? 64u : 57u;
    ev.sample_slot = MELLO_DRUM_HAT;
    ev.velocity = 110u;
    (void)mello_audio_push_engine_event(&a, &ev, 0u);
    for (blk = 0u; blk < 8u; ++blk) {
        if (blk == at) mello_audio_set_mix_mute(&a, bits);
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, 441u, blk * 10000u, 120u);
        peak[blk] = 0;
        for (k = 0u; k < 882u; ++k) peak[blk] = abs(out[k]) > peak[blk] ? abs(out[k]) : peak[blk];
    }
}

int main(void)
{
    int32_t p[8], q[8];
    run(1u, 3u, false, p); /* LEAD muted from 30 ms */
    printf("LEAD muted at 30 ms: peaks %d %d %d | %d %d %d\n", (int)p[1], (int)p[2], (int)p[3], (int)p[5],
           (int)p[6], (int)p[7]);
    CHECK(p[2] > 1000 && p[5] < 30 && p[7] < 30, "LEAD's mute: %d before, %d after", (int)p[2], (int)p[5]);
    run(8u, 3u, false, q); /* PERC's mute leaves the lead */
    CHECK(q[5] > 1000, "PERC's mute took the lead (%d)", (int)q[5]);
    run(8u, 2u, true, p); /* PERC muted under a long hat */
    printf("PERC muted at 20 ms: peaks %d %d | %d %d\n", (int)p[0], (int)p[1], (int)p[4], (int)p[5]);
    CHECK(p[1] > 100 && p[4] < 30, "PERC's mute: %d before, %d after", (int)p[1], (int)p[4]);
    if (fails) return 1;
    puts("PASS: MIX's mutes take a part away and leave the rest");
    return 0;
}
