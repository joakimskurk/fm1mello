/* Phase 2 regression: a knob turned while a note sounds ramps as the HTML's rampTo does (master 50 ms) instead of
 * stepping, which clicks (zipper noise on every encoder detent). A held saw, dry; MASTER 750 -> 150 mid-note: the
 * level must not drop at once, and must have arrived after the ramp. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mello_audio.h"

#define FRAMES 256u /* more than a period of A3 */
static int16_t delay_buf[4096];
static int16_t reverb_buf[24576];

static int32_t peak_of(const int32_t *out, uint32_t n)
{
    int32_t p = 0;
    uint32_t i;
    for (i = 0u; i < n * 2u; ++i) {
        int32_t v = out[i] < 0 ? -out[i] : out[i];
        if (v > p) p = v;
    }
    return p;
}

int main(void)
{
    static MelloAudio a;
    static int32_t out[FRAMES * 2u];
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    MelloEvent e;
    uint32_t blk, us = 0u;
    int32_t before = 0, first = 0, after = 0;

    mello_sound_defaults(&s);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sm);
    s.sustain_permille = 1000u;
    s.follower_sub_on = false;
    fx.delay_wet = fx.reverb_wet = 0u;
    sub.on = false;
    mello_audio_init(&a, delay_buf, 4096u, reverb_buf, 24576u);
    mello_audio_set_config(&a, &s, &fx, &sub, &sm);
    memset(&e, 0, sizeof e);
    e.type = MELLO_EVENT_NOTE_ON;
    e.note = 57u;
    e.velocity = 110u;
    mello_audio_push_engine_event(&a, &e, 0u);
    for (blk = 0u; blk < 50u; ++blk) { /* ~0.29 s */
        if (blk == 25u) {
            s.master_level = 150u;
            mello_audio_set_config(&a, &s, &fx, &sub, &sm);
        }
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, FRAMES, us, 80u);
        us += FRAMES * 1000000u / 44100u;
        if (blk == 24u) before = peak_of(out, FRAMES);
        if (blk == 25u) first = peak_of(out, FRAMES);
        if (blk == 49u) after = peak_of(out, FRAMES);
    }
    printf("peak before %d, first block after the change %d, 140 ms later %d\n", before, first, after);
    /* 256 of the ramp's 2205 samples: the first block keeps > 85% of the level; after it, ~1/5 (150/750). */
    if (before < 2000 || first * 100 < before * 85) {
        fprintf(stderr, "MASTER stepped instead of ramping\n");
        return 1;
    }
    if (after * 4 > before) {
        fprintf(stderr, "MASTER did not arrive\n");
        return 1;
    }
    return 0;
}
