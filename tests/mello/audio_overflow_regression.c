/* P0 regression (review 2026-10, P0-1): legal settings must not overflow the fixed-point path.
 * The ladder filter and the LFO->AMP multiply used to wrap int32; the ladder then latched at -21903 DC
 * until reboot. tests/run_mello.sh builds this test with UBSan, so any signed overflow fails it too. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"

#define FRAMES 256u
static int16_t delay_buf[4096];
static int16_t reverb_buf[24576];

typedef struct {
    const char *name;
    MelloWaveform wave, follower_wave;
    uint16_t follower_level, cut, res;
    bool lfo_amp;
} Case;

/* Returns the peak |output| over the last 0.5 s of 3 s of silence after a 1 s note. */
static int32_t tail_peak(const Case *c)
{
    static MelloAudio a;
    static int32_t out[FRAMES * 2u];
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    uint32_t blk, us = 0u, blocks = (44100u * 4u) / FRAMES, tail_from = (44100u * 7u / 2u) / FRAMES;
    int32_t peak = 0;

    mello_sound_defaults(&s);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sm);
    s.osc_wave = c->wave;
    s.follower_sub_on = c->follower_level != 0u;
    s.follower_sub_wave = c->follower_wave;
    s.follower_sub_level = c->follower_level;
    s.filter_cut = c->cut;
    s.filter_res = c->res;
    if (c->lfo_amp) {
        s.lfo_on = true;
        s.lfo_target = MELLO_LFO_AMP;
        s.lfo_depth = 1000u;
    }
    /* FX off: this test is about the voice and filter, not the reverb's own tail. */
    fx.delay_wet = 0u;
    fx.reverb_wet = 0u;
    mello_audio_init(&a, delay_buf, 4096u, reverb_buf, 24576u);
    mello_audio_set_config(&a, &s, &fx, &sub, &sm);
    for (blk = 0u; blk < blocks; ++blk) {
        uint32_t i;
        if (blk == 2u || blk == 2u + 44100u / FRAMES) {
            MelloEvent e;
            memset(&e, 0, sizeof e);
            e.type = blk == 2u ? MELLO_EVENT_NOTE_ON : MELLO_EVENT_NOTE_OFF;
            e.note = 48u;
            e.velocity = 127u;
            mello_audio_push_engine_event(&a, &e, us);
        }
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, FRAMES, us, 120u);
        us += FRAMES * 1000000u / 44100u;
        if (blk >= tail_from)
            for (i = 0u; i < FRAMES * 2u; ++i) {
                int32_t v = out[i] < 0 ? -out[i] : out[i];
                if (v > peak) peak = v;
            }
    }
    return peak;
}

int main(void)
{
    static const Case cases[] = {
        {"square + square follower 1000, cut 900", MELLO_WAVE_SQUARE, MELLO_WAVE_SQUARE, 1000u, 900u, 200u,
         false},
        {"saw + saw follower 1000, cut 1000, res 1000", MELLO_WAVE_SAW, MELLO_WAVE_SAW, 1000u, 1000u, 1000u,
         false},
        {"pulse + square follower 1000, cut 850, res 0", MELLO_WAVE_PULSE, MELLO_WAVE_SQUARE, 1000u, 850u, 0u,
         false},
        {"default + LFO->AMP depth 1000", MELLO_WAVE_SAW, MELLO_WAVE_SINE, 220u, 700u, 200u, true},
        {"square + square follower 1000 + LFO->AMP 1000, cut 1000", MELLO_WAVE_SQUARE, MELLO_WAVE_SQUARE,
         1000u, 1000u, 500u, true},
    };
    unsigned i, failed = 0u;
    for (i = 0u; i < sizeof cases / sizeof cases[0]; ++i) {
        int32_t p = tail_peak(&cases[i]);
        printf("%-58s tail peak %6d %s\n", cases[i].name, (int)p, p <= 8 ? "ok" : "FAIL");
        if (p > 8) failed++;
    }
    if (failed) {
        printf("FAIL: %u case(s) left output stuck after the note\n", failed);
        return 1;
    }
    printf("PASS: no fixed-point overflow, silence after release\n");
    return 0;
}
