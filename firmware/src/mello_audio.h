#ifndef MELLO_AUDIO_H
#define MELLO_AUDIO_H

#include "mello_engine.h"
#include "mello_sample.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MELLO_AUDIO_FS             44100u
#define MELLO_AUDIO_EVENT_CAP      128u
#define MELLO_AUDIO_EVENT_LANES    4u
#define MELLO_AUDIO_EVENT_LANE_CAP (MELLO_AUDIO_EVENT_CAP / MELLO_AUDIO_EVENT_LANES)
#define MELLO_AUDIO_DELAY_SAMPLES  65536u
#define MELLO_AUDIO_REVERB_SAMPLES 24576u

typedef struct {
    uint32_t due_us;
    MelloEvent event;
} MelloAudioQueuedEvent;

typedef enum {
    MELLO_ENV_OFF = 0,
    MELLO_ENV_ATTACK,
    MELLO_ENV_DECAY,
    MELLO_ENV_SUSTAIN,
    MELLO_ENV_RELEASE
} MelloEnvStage;

typedef struct {
    uint32_t phase;
    uint32_t inc;
    uint32_t target_inc;
    int32_t slide_step;
    uint32_t slide_left;
    uint32_t sub_phase;
    uint32_t sub_inc;
    uint32_t sub_target_inc;
    int32_t sub_slide_step;
    uint32_t sub_slide_left;
    int32_t env_q24;
    int32_t env_step_q24;
    int32_t env_target_q24;
    MelloEnvStage env_stage;
    uint8_t note;
    uint8_t velocity;
    bool gate;
} MelloAudioLeadVoice;

typedef struct {
    uint32_t phase;
    uint32_t inc;
    uint32_t target_inc;
    int32_t slide_step;
    uint32_t slide_left;
    int32_t env_q24;
    int32_t env_step_q24;
    int32_t env_target_q24;
    MelloEnvStage env_stage;
    uint8_t note;
    uint8_t velocity;
    bool gate;
    uint32_t env_curve_left;
    uint32_t env_linear_left;
    uint32_t env_curve_coeff_q31;
    int32_t biquad1_x1;
    int32_t biquad1_x2;
    int32_t biquad1_y1;
    int32_t biquad1_y2;
    int32_t biquad2_x1;
    int32_t biquad2_x2;
    int32_t biquad2_y1;
    int32_t biquad2_y2;
    int32_t drive_prev;
} MelloAudioSubVoice;

#if MELLO_FEATURE_CHORD_MACHINE
typedef struct {
    uint32_t phase[MELLO_CHORD_MAX_VOICES];
    uint32_t inc[MELLO_CHORD_MAX_VOICES];
    uint8_t note[MELLO_CHORD_MAX_VOICES];
    uint8_t active[MELLO_CHORD_MAX_VOICES];
    uint8_t active_count;
    int32_t env_q24;
    int32_t env_step_q24;
    int32_t env_target_q24;
    MelloEnvStage env_stage;
    /* Dedicated soft band-pass state keeps the chord layer out of the lead/sub extremes. */
    int32_t tone_lp;
    int32_t hp_x1;
    int32_t hp_y1;
} MelloAudioChordVoice;
#endif

typedef struct {
    bool active;
    bool gate;
    bool loop;
    uint8_t slot;
    uint8_t velocity;
    uint32_t pos_q16;
    uint32_t step_q16;
    uint32_t target_step_q16;
    int32_t slide_step_q16;
    uint32_t slide_left;
    uint32_t trim_start_sample;
    uint32_t trim_end_sample;
    int32_t env_q24;
    int32_t env_step_q24;
    int32_t env_target_q24;
    MelloEnvStage env_stage;
    uint16_t decoded_block;
    uint16_t decoded_valid;
    bool decoded_ready;
} MelloAudioSampleVoice;

typedef struct {
    MelloSoundConfig config;
    MelloFxConfig fx;
    MelloSubConfig sub_config;
    MelloSampleConfig sample_config;
#if MELLO_FEATURE_CHORD_MACHINE
    MelloChordConfig chord_config;
#endif
    MelloSoundConfig pending_config;
    MelloFxConfig pending_fx;
    MelloSubConfig pending_sub_config;
    MelloSampleConfig pending_sample_config;
#if MELLO_FEATURE_CHORD_MACHINE
    MelloChordConfig pending_chord_config;
#endif
    volatile uint32_t config_generation;
    uint32_t applied_generation;

    MelloAudioLeadVoice lead;
    MelloAudioSubVoice sub;
#if MELLO_FEATURE_CHORD_MACHINE
    MelloAudioChordVoice chord;
#endif
    MelloAudioSampleVoice sample_voice[MELLO_SAMPLE_SLOT_COUNT];
    int16_t sample_decode[MELLO_SAMPLE_SLOT_COUNT][MELLO_SAMPLE_BLOCK_SAMPLES];
    uint32_t sample_last_target_step_q16[MELLO_SAMPLE_SLOT_COUNT];
    MelloSampleBank *sample_bank;
    uint32_t lfo_phase;
    uint32_t lfo_inc;
    int32_t filter1;
    int32_t filter2;
    int32_t filter3;
    int32_t filter4;
    int32_t dist_lp;
    int32_t matrix_env_q15;
    int32_t matrix_env_step_q15;
    int32_t matrix_env_target_q15;
    uint32_t matrix_env_left;
    bool matrix_env_attack;
    uint16_t effective_decay_ms;
    int16_t last_lfo;
    int16_t tm_filter_mod_q15;
    int16_t tm_decay_mod_q15;
    uint16_t matrix_filter_amount;

    int16_t *delay_buffer;
    uint32_t delay_capacity;
    uint32_t delay_pos;
    uint32_t delay_samples;
    int32_t delay_lp;
    int16_t *reverb_buffer;
    uint32_t reverb_capacity;
    uint32_t reverb_pos;
    uint32_t reverb_delay_a;
    uint32_t reverb_delay_b;
    int32_t reverb_lp; /* retained compatibility state */
    int32_t reverb_damp;
    int32_t reverb_tone_lp;
    int32_t reverb_hp_x1;
    int32_t reverb_hp_y1;
    int32_t reverb_tone_lp_r;
    int32_t reverb_hp_x1_r;
    int32_t reverb_hp_y1_r;
    uint16_t reverb_feedback_q15; /* representative/debug value; per-line gains below */
    uint16_t reverb_pre_samples;
    uint16_t reverb_pre_cap;
    uint16_t reverb_ap_offset[3];
    uint16_t reverb_ap_cap[3];
    uint16_t reverb_ap_delay[3];
    uint16_t reverb_ap_pos[3];
    uint16_t reverb_line_offset[8];
    uint16_t reverb_line_cap[8];
    uint16_t reverb_line_delay[8];
    uint16_t reverb_line_pos[8];
    uint16_t reverb_line_feedback_q15[8];
    int32_t reverb_line_damp[8];
    int32_t reverb_line_lf[8];
    uint16_t reverb_send_permille;
    uint16_t dist_drive_permille;
    int16_t sub_dist_curve[257];
    int16_t pending_sub_dist_curve[257];

    /* Four monotonic SPSC lanes. app_main only appends; the audio ISR only pops.
     * This avoids head-of-line blocking without rewriting entries visible to the ISR. */
    volatile uint8_t q_head[MELLO_AUDIO_EVENT_LANES];
    volatile uint8_t q_tail[MELLO_AUDIO_EVENT_LANES];
    uint32_t q_last_due[MELLO_AUDIO_EVENT_LANES];
    MelloAudioQueuedEvent queue[MELLO_AUDIO_EVENT_CAP];
    volatile bool queue_overflow;
    volatile bool all_notes_off_pending;
    volatile bool silent;
    uint32_t tail_guard_frames;
    uint32_t rendered_frames;
    int16_t peak;
#if defined(MELLO_LIVE_EDIT_TEST_HOOK) && MELLO_LIVE_EDIT_TEST_HOOK
    uint32_t test_config_applies;
    uint32_t test_sub_curve_builds;
    uint32_t test_sub_curve_applies;
    uint32_t test_reverb_reconfigs;
#endif
} MelloAudio;

void mello_audio_init(MelloAudio *a, int16_t *delay_buffer, uint32_t delay_capacity, int16_t *reverb_buffer,
                      uint32_t reverb_capacity);
void mello_audio_set_config(MelloAudio *a, const MelloSoundConfig *sound, const MelloFxConfig *fx,
                            const MelloSubConfig *sub, const MelloSampleConfig *sample);
#if MELLO_FEATURE_CHORD_MACHINE
void mello_audio_set_chord_config(MelloAudio *a, const MelloChordConfig *chord);
#endif
void mello_audio_attach_sample_bank(MelloAudio *a, MelloSampleBank *bank);
bool mello_audio_push_engine_event(MelloAudio *a, const MelloEvent *event, uint32_t due_us);
void mello_audio_event_sink(void *ctx, const MelloEvent *event, uint32_t due_us);
void mello_audio_render(MelloAudio *a, int32_t *out_stereo, uint32_t frames, uint32_t block_start_us,
                        uint16_t bpm);
void mello_audio_all_notes_off(MelloAudio *a);
bool mello_audio_queue_overflowed(const MelloAudio *a);
bool mello_audio_is_silent(const MelloAudio *a);
#if defined(MELLO_DISTORTION_TEST_HOOK) && MELLO_DISTORTION_TEST_HOOK
uint16_t mello_audio_test_global_dist_drive(uint16_t amount);
int32_t mello_audio_test_global_distort(int32_t x, uint16_t amount, uint16_t character);
#endif
#if defined(MELLO_REVERB_TEST_HOOK) && MELLO_REVERB_TEST_HOOK
int32_t mello_audio_reverb_ir_probe(MelloAudio *a, int32_t send);
void mello_audio_reverb_stereo_probe(MelloAudio *a, int32_t send, int32_t *left, int32_t *right);
#endif

#ifdef __cplusplus
}
#endif
#endif
