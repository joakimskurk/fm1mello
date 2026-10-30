/* Phase 5: the Virus layer (DECISIONS.md "Virus layer"). Off by default, and OSC 2 settings without its mix change
 * nothing; each part does what it says (HP and BP take the low end, notch the fundamental, sync and FM change the
 * tone, the MOD envelope opens the filter early in the note); SAT keeps the level; everything at full stays in range
 * (built with UBSan). */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"

#define FRAMES 256u
#define BLOCKS 40u
static int16_t delay_buf[4096];
static int16_t reverb_buf[24576];
static int32_t out[BLOCKS][FRAMES * 2u];

static void play(const MelloSoundConfig *s, uint8_t note)
{
    static MelloAudio a;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    MelloEvent e;
    uint32_t blk, us = 0u;
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sm);
    fx.delay_wet = fx.reverb_wet = 0u;
    sub.on = false;
    mello_audio_init(&a, delay_buf, 4096u, reverb_buf, 24576u);
    mello_audio_set_config(&a, s, &fx, &sub, &sm);
    memset(&e, 0, sizeof e);
    e.type = MELLO_EVENT_NOTE_ON;
    e.note = note;
    e.velocity = 110u;
    mello_audio_push_engine_event(&a, &e, 0u);
    for (blk = 0u; blk < BLOCKS; ++blk) {
        memset(out[blk], 0, sizeof out[blk]);
        mello_audio_render(&a, out[blk], FRAMES, us, 80u);
        us += FRAMES * 1000000u / 44100u;
    }
}

/* RMS over blocks [b0, b1), and the share of it that a first difference keeps (brightness, 0..2). */
static double rms(uint32_t b0, uint32_t b1, double *bright)
{
    double sq = 0.0, d = 0.0;
    uint32_t blk, i, n = 0u;
    int32_t prev = out[b0][0];
    for (blk = b0; blk < b1; ++blk)
        for (i = 0u; i < FRAMES; ++i, ++n) {
            int32_t v = out[blk][i * 2u];
            sq += (double)v * v;
            d += (double)(v - prev) * (v - prev);
            prev = v;
        }
    if (bright) *bright = sq > 0.0 ? sqrt(d / sq) : 0.0;
    return sqrt(sq / n);
}

static int differs(const int32_t (*ref)[FRAMES * 2u])
{
    return memcmp(ref, out, sizeof out) != 0;
}

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fputc('\n', stderr);                                                                             \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

int main(void)
{
    static int32_t plain[BLOCKS][FRAMES * 2u];
    MelloSoundConfig base, s;
    double r0, b0, r, b, early, late;
    mello_sound_defaults(&base);
    base.sustain_permille = 1000u;
    base.follower_sub_on = false;
    CHECK(!base.osc2_mix && !base.fm_amount && !base.saturation && base.filter_mode == MELLO_FILTER_LP &&
              !base.menv_amount,
          "the Virus layer is on by default");
    play(&base, 45u);
    memcpy(plain, out, sizeof out);
    r0 = rms(BLOCKS / 2u, BLOCKS, &b0);
    printf("plain: rms %.0f, brightness %.3f\n", r0, b0);

    s = base; /* OSC 2 set up but not mixed in, an envelope with no amount: nothing changes */
    s.osc2_semi = 7, s.osc2_sync = true, s.fm_amount = 500u, s.menv_decay_ms = 900u,
    s.menv_target = MELLO_MENV_PITCH;
    play(&s, 45u);
    CHECK(!differs(plain), "OSC 2 or the MOD envelope changed the sound at mix / amount 0");

    s = base;
    s.filter_mode = MELLO_FILTER_HP;
    play(&s, 45u);
    r = rms(BLOCKS / 2u, BLOCKS, &b);
    printf("HP: rms %.0f, brightness %.3f\n", r, b);
    CHECK(r < r0 * 0.8 && b > b0 * 1.5, "HP did not take the low end");
    s.filter_mode = MELLO_FILTER_BP;
    play(&s, 45u);
    r = rms(BLOCKS / 2u, BLOCKS, &b);
    printf("BP: rms %.0f, brightness %.3f\n", r, b);
    CHECK(r < r0 * 0.8 && b > b0, "BP did not take the low end");
    s.filter_mode = MELLO_FILTER_NOTCH;
    s.filter_cut = 248u; /* 80 * 200^0.248 = 297 Hz: between the 110 Hz fundamental's harmonics 2 and 3 */
    play(&s, 45u);
    CHECK(differs(plain), "NOTCH did nothing");

    s = base;
    s.osc2_mix = 1000u, s.osc2_sync = true, s.osc2_semi = 7;
    play(&s, 45u);
    r = rms(BLOCKS / 2u, BLOCKS, &b);
    printf("OSC 2 synced +7: rms %.0f, brightness %.3f\n", r, b);
    CHECK(differs(plain) && r > r0 * 0.5 && r < r0 * 2.0, "sync did nothing, or its level is off");

    s = base;
    s.osc2_mix = 1000u, s.osc2_wave = MELLO_WAVE_SINE, s.fm_amount = 700u;
    play(&s, 45u);
    r = rms(BLOCKS / 2u, BLOCKS, &b);
    printf("FM 70%% on a sine: rms %.0f, brightness %.3f\n", r, b);
    {
        double bs;
        int32_t keep[BLOCKS][FRAMES * 2u];
        memcpy(keep, out, sizeof out);
        s.fm_amount = 0u;
        play(&s, 45u);
        (void)rms(BLOCKS / 2u, BLOCKS, &bs);
        CHECK(memcmp(keep, out, sizeof out) && b > bs * 1.3, "FM did not brighten the sine (%.3f vs %.3f)", b,
              bs);
    }

    s = base; /* MOD envelope on the cutoff, from a closed filter: bright early, dark late */
    s.filter_cut = 250u, s.menv_amount = 900, s.menv_decay_ms = 150u;
    play(&s, 45u);
    (void)rms(1u, 4u, &early);
    (void)rms(BLOCKS - 8u, BLOCKS, &late);
    printf("MOD ENV -> CUTOFF: brightness early %.3f, late %.3f\n", early, late);
    CHECK(early > late * 1.5, "the MOD envelope did not open the filter");

    {
        static const uint16_t sats[4] = {150u, 400u, 700u, 1000u};
        unsigned i;
        for (i = 0u; i < 4u; ++i) {
            s = base;
            s.saturation = sats[i];
            play(&s, 45u);
            r = rms(BLOCKS / 2u, BLOCKS, 0);
            printf("SAT %u: %+.1f dB\n", sats[i] / 10u, 20.0 * log10(r / r0));
            CHECK(r > r0 * 0.6 && r < r0 * 1.6, "SAT moved the level more than 4 dB");
        }
    }

    /* Everything at full, high and low, with the LFO on OSC 2 and the envelope on FM: no overflow (UBSan). */
    s = base;
    s.osc_wave = MELLO_WAVE_SQUARE, s.filter_res = 1000u, s.unison = MELLO_UNISON_MAX, s.detune = 1000u;
    s.noise_level = s.noise_fm = 1000u;
    s.osc2_mix = 500u, s.osc2_semi = 24, s.osc2_sync = true, s.fm_amount = 1000u, s.saturation = 1000u;
    s.filter_mode = MELLO_FILTER_BP, s.menv_amount = 1000, s.menv_target = MELLO_MENV_FM;
    s.lfo_on = true, s.lfo_target = MELLO_LFO_OSC2, s.lfo_depth = 1000u;
    play(&s, 108u);
    play(&s, 24u);
    s.menv_amount = -1000, s.menv_target = MELLO_MENV_PITCH, s.lfo_target = MELLO_LFO_FM, s.osc2_semi = -24;
    play(&s, 108u);
    puts("PASS: Virus layer off = plain saw; filter modes, sync, FM, MOD envelope and SAT do what they say, "
         "in range");
    return 0;
}
