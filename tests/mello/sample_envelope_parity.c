/* Phase 2: a sample slot's envelope follows Tone.AmplitudeEnvelope as the lead's does. Its decay and release are
 * exponential (tau = ln(t + 1) / ln 200, then a linear tail), not straight lines: halfway through a 200 ms decay to
 * sustain 0 the level is ~5%, where a linear decay is at 50%. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"

#define SAMPLES 4096u
static int16_t delay_buf[4096];
static int16_t reverb_buf[24576];
static uint8_t slot_data[MELLO_SAMPLE_SLOT_COUNT]
                        [(SAMPLES / MELLO_SAMPLE_BLOCK_SAMPLES) * MELLO_SAMPLE_BLOCK_BYTES];

/* The slot's envelope (in 1/1000 of full) n frames after the step starts, rendered a frame at a time. */
static int level_after(MelloAudio *a, uint32_t frames_on, uint32_t frames_after_off, bool off)
{
    static int32_t out[2];
    uint32_t k, us = 0u;
    MelloEvent e;
    memset(&e, 0, sizeof e);
    e.type = MELLO_EVENT_SAMPLE_NOTE_ON;
    e.note = 60u;
    e.velocity = 127u;
    mello_audio_push_engine_event(a, &e, 0u);
    for (k = 0u; k < frames_on + frames_after_off; ++k) {
        if (off && k == frames_on) {
            e.type = MELLO_EVENT_SAMPLE_NOTE_OFF;
            mello_audio_push_engine_event(a, &e, us);
        }
        mello_audio_render(a, out, 1u, us, 120u);
        us = (uint32_t)((uint64_t)(k + 1u) * 1000000u / 44100u);
    }
    return (int)(((int64_t)a->sample_voice[0].env_q24 * 1000) / (32767 << 8)); /* Q24_ONE is 32767 << 8 */
}

static void setup(MelloAudio *a, MelloSampleBank *bank, uint16_t sustain)
{
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    uint32_t i;
    mello_sample_bank_init(bank, slot_data[0], slot_data[1], slot_data[2], slot_data[3], sizeof slot_data[0]);
    for (i = 0u; i < sizeof slot_data[0]; ++i)
        slot_data[0][i] = (uint8_t)(i % MELLO_SAMPLE_BLOCK_BYTES == 2u ? 40u : (i * 37u) & 0xFFu);
    bank->meta[0].loaded = true;
    bank->meta[0].sample_rate = 44100u;
    bank->meta[0].sample_count = SAMPLES;
    bank->meta[0].block_count = (uint16_t)(SAMPLES / MELLO_SAMPLE_BLOCK_SAMPLES);
    bank->meta[0].data_bytes = sizeof slot_data[0];
    mello_sound_defaults(&s);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sm);
    s.osc_wave = MELLO_WAVE_OFF;
    fx.delay_wet = fx.reverb_wet = 0u;
    sm.on = true;
    sm.slots[0].loop = true;
    sm.slots[0].attack_ms = 1u;
    sm.slots[0].decay_ms = 200u;
    sm.slots[0].sustain_permille = sustain;
    sm.slots[0].release_ms = 200u;
    sm.slots[0].level = 1000u;
    mello_audio_init(a, delay_buf, 4096u, reverb_buf, 24576u);
    mello_audio_attach_sample_bank(a, bank);
    mello_audio_set_config(a, &s, &fx, &sub, &sm);
}

int main(void)
{
    static MelloAudio a;
    static MelloSampleBank bank;
    int mid_decay, mid_release;

    setup(&a, &bank, 0u);
    mid_decay = level_after(&a, 44u + 4410u, 0u, false); /* 1 ms attack, then 100 ms of the 200 ms decay */
    setup(&a, &bank, 1000u);
    mid_release =
        level_after(&a, 2205u, 4410u, true); /* held at sustain 1, then 100 ms of a 200 ms release */
    printf("halfway: decay %d/1000, release %d/1000 (Tone ~55, linear 500)\n", mid_decay, mid_release);
    if (mid_decay < 30 || mid_decay > 100 || mid_release < 30 || mid_release > 100) {
        fprintf(stderr, "sample envelope is not Tone's exponential curve\n");
        return 1;
    }
    return 0;
}
