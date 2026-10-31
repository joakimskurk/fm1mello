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

/* A parameter moving linearly to its target over a fixed time, as Tone's rampTo: value << 8. */
typedef struct {
    int32_t cur;
    int32_t target;
    int32_t step;
    uint32_t left;
} MelloRamp;

/* A DOT MOD lane's envelope (MelloModRouting): v in permille << 16. Stage 0 off, 1 attack, 2 hold (GATE), 3 release. */
typedef struct {
    int32_t v;
    int32_t peak;
    int32_t step;
    uint32_t left;
    uint8_t stage;
} MelloDotLane;

typedef struct {
    uint32_t phase;
    uint32_t inc;
    uint32_t target_inc;
    int32_t pitch_q16;  /* the pitch in semitones (MIDI note) << 16: slides move it, inc follows */
    uint32_t on_tag;    /* the latest NOTE_ON (event_tag): only its own NOTE_OFF releases */
    int32_t slide_step; /* per sample, in pitch_q16 units */
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
    uint32_t env_curve_left; /* Tone.Envelope decay/release: exponential part, then a linear tail */
    uint32_t env_linear_left;
    uint32_t env_curve_coeff_q31;
    uint32_t uni_phase[MELLO_UNISON_MAX - 1u]; /* the unison oscillators beside phase; free-running */
    int16_t vel_q15; /* tracking: the note's velocity from the plain step's, +-32767 (an accent is +32767) */
    uint8_t vel_level; /* the velocity the level uses (vel_level sets how far it is from the plain step's) */
    uint32_t osc2_phase;
} MelloAudioLeadVoice;

typedef struct {
    uint32_t phase;
    uint32_t inc;
    uint32_t target_inc;
    int32_t pitch_q16; /* as the lead's */
    uint32_t on_tag;
    int32_t slide_step; /* per sample, in pitch_q16 units */
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
/* Voices to play from: a chord's four, and two more so a new chord does not cut the last one's release. */
#define MELLO_CHORD_AUDIO_VOICES 6u
#define MELLO_CHORD_CHORUS_LEN   1024u /* 23 ms: the chorus's delay line */
typedef struct {
    uint32_t phase[MELLO_CHORD_AUDIO_VOICES];
    uint32_t inc[MELLO_CHORD_AUDIO_VOICES];
    int32_t venv[MELLO_CHORD_AUDIO_VOICES];  /* each voice's ADSR, Q24, every 16 samples */
    int32_t vgain[MELLO_CHORD_AUDIO_VOICES]; /* and its gain, Q24, ramped between: no steps */
    int32_t vstep[MELLO_CHORD_AUDIO_VOICES];
    uint8_t vstage[MELLO_CHORD_AUDIO_VOICES]; /* 0 off, 1 attack, 2 decay / sustain, 3 release */
    uint32_t vage[MELLO_CHORD_AUDIO_VOICES];  /* when it started (the oldest is taken when none is free) */
    uint8_t note[MELLO_CHORD_AUDIO_VOICES];
    uint8_t active[MELLO_CHORD_AUDIO_VOICES]; /* gated: its NOTE_OFF has not come */
    uint8_t active_count;
    uint32_t age;
    int32_t env_q24; /* the loudest voice's level: 0 when the layer is silent */
    MelloEnvStage env_stage;
    int32_t fenv;   /* the filter envelope, Q24, restarted by every note */
    uint8_t fstage; /* 1 attack, 2 decay */
    int32_t lp, bp; /* the filter (state variable, 12 dB) */
    int32_t hp_x1, hp_y1;
    int32_t f_q15, damp_q15; /* its coefficients, every 16 samples */
    uint8_t ctl;
    int16_t chorus_buf[MELLO_CHORD_CHORUS_LEN];
    uint16_t chorus_pos;
    uint32_t chorus_lfo;
    /* from the config: the envelopes' rates for 16 samples (Q24), the sustain */
    int32_t k_att, k_dec, k_rel, k_fatt, k_fdec, sus_q24;
} MelloAudioChordVoice;
#endif

/* A drum voice (DRUMS): one hit at a time, retriggered by the next. */
typedef struct {
    uint8_t on;
    uint8_t gated; /* the tail cut short (the snare's gate) */
    uint8_t kind;  /* GLITCH: MELLO_GLITCH_* */
    int32_t env; /* Q24, falls by k a sample (one-pole, to -60 dB in the hit's decay) after hold_s samples */
    int32_t k;
    uint16_t hold_s;
    int32_t penv, pk; /* a slower envelope, Q24: the kick's body pitch, the snare's body, the bleep's index */
    int32_t cenv, ck; /* the attack, Q24: the kick's click, the snare's sweep, the chirp's fall */
    uint32_t ph[2], inc[2]; /* the tones (kick sine; snare body; the hat's tick; the glitch's pair) */
    int32_t gain;           /* Q15: LEVEL and the hit's velocity */
    int32_t gl, gr;         /* Q15: where this hit sits (the hat and glitch are scattered) */
    int32_t lp, bp, hp1;    /* filters: the state-variable pair, a one-pole */
    int32_t f, damp;        /* the state-variable filter's base frequency and damping, Q15 */
    uint16_t loop_start, loop_len, loop_pos, sh_n, sh_left; /* STUTTER's loop; CRACKLE's sample and hold */
    int32_t sh;
    uint8_t hold, hold_n; /* CRUSH: sample and hold, and the bits kept */
    int32_t held, mask;
} MelloAudioDrumVoice;
#define MELLO_DRUM_BUF 2048u /* STUTTER's memory of the music: 46 ms */

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
    uint32_t env_curve_left; /* Tone.Envelope decay/release, as the lead's */
    uint32_t env_linear_left;
    uint32_t env_curve_coeff_q31;
    uint16_t decoded_block;
    uint16_t decoded_valid;
    bool decoded_ready;
} MelloAudioSampleVoice;

typedef struct {
    MelloSoundConfig config;
    MelloModRouting modr;
    MelloFxConfig fx;
    MelloSubConfig sub_config;
    MelloSampleConfig sample_config;
#if MELLO_FEATURE_CHORD_MACHINE
    MelloChordConfig chord_config;
#endif
    MelloSoundConfig pending_config;
    MelloModRouting pending_modr;
    MelloFxConfig pending_fx;
    MelloSubConfig pending_sub_config;
    MelloSampleConfig pending_sample_config;
#if MELLO_FEATURE_CHORD_MACHINE
    MelloChordConfig pending_chord_config;
#endif
    MelloDrumConfig drum_config;
    MelloDrumConfig pending_drum_config;
    MelloAudioDrumVoice drum[MELLO_DRUM_VOICES];
    uint32_t drum_noise;
    int16_t drum_buf[MELLO_DRUM_BUF]; /* the music and the drums, for STUTTER */
    uint16_t drum_buf_pos;
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
    uint8_t sample_decode_budget; /* IMA blocks the rest of this render call may decode */
    uint32_t sample_decodes;      /* diagnostics: IMA blocks decoded since init */
    uint32_t sample_decode_skips; /* diagnostics: sample frames muted because the budget ran out */
    uint32_t lfo_phase;
    uint32_t lfo_inc;
    uint32_t noise_seed;    /* grit: the white noise generator (an LCG) */
    int32_t nfm_lp;         /* grit: the noise that frequency-modulates the lead, lowpassed */
    int32_t noise_lc[2];    /* grit: the noise's low cut, two one-pole lowpasses taken away */
    int32_t menv_q24;       /* the MOD envelope, 0..1 << 24: attack (linear), then decay (exponential) */
    int32_t menv_step;      /* attack: per sample */
    int32_t menv_k_q16;     /* decay: the fraction lost per sample, Q16 */
    uint8_t menv_stage;     /* 0 off, 1 attack, 2 decay */
    int32_t mf_state[2][4]; /* main filter: two biquads, x1 x2 y1 y2 each */
    int32_t mf_coeff[5];    /* b0 b1 b2 a1 a2, Q28, for mf_cut/mf_res */
    uint16_t mf_cut;
    uint16_t mf_res;
    uint8_t mf_mode;
    bool mf_coeff_valid;
    int32_t dist_tone_s[2][4]; /* distTone: lowpass -24 = two biquads */
    MelloDotLane dot_lane[MELLO_DOT_MOD_LANES];
    int16_t tm_value; /* EVOLVE's register value at the last pulse, centred */
    bool sub_mute;    /* SUB MUTE (HOME's performance key), held */
    int32_t sub_gain; /* the bass's gain, Q15, sliding to 0 or 1 in ~10 ms so muting never clicks */
    uint16_t effective_decay_ms;
    int16_t last_lfo;
    int16_t tm_filter_mod_q15;
    int16_t tm_decay_mod_q15;

    int16_t *delay_buffer;
    uint32_t delay_capacity;
    uint32_t delay_pos;
    uint32_t delay_samples;
    /* Knob changes ramp as the HTML's rampTo calls do (lines 989-1077); the delay time too (read between samples). */
    int32_t limiter_env_q4; /* limiter_step's peak follower */
    MelloRamp rp_cut, rp_res, rp_master, rp_rev_send, rp_rev_level, rp_dly_send, rp_dly_level, rp_dly_fb,
        rp_dist_level, rp_sub_level, rp_fsub_level, rp_delay;
    int32_t delay_tone_s[2][4]; /* dlyTone */
    int32_t delay_lc[2];        /* LO CUT: two one-pole low-passes, taken away */
    int16_t *reverb_buffer;
    uint32_t reverb_capacity;
    uint32_t reverb_pos;
    uint32_t reverb_delay_a;
    uint32_t reverb_delay_b;
    int32_t reverb_lp; /* retained compatibility state */
    int32_t reverb_damp;
    int32_t reverb_tone_s[2][2][4]; /* revTone, [L/R][stage] */
    int32_t reverb_hp_y1;
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
    uint32_t reverb_mod_phase[8]; /* slow delay modulation per FDN line (reverb_network_step) */
    int32_t reverb_mod_y[8];      /* its allpass interpolators' last outputs */
    uint16_t reverb_mod_di[8];    /* their whole delays and allpass coefficients, updated every 16 samples */
    int16_t reverb_mod_eta[8];
    uint8_t reverb_mod_tick;
    /* The half-rate network's way in and out: a 31-tap half-band filter each (the input at full rate, the outputs
     * at half rate), so nothing above 11 kHz folds into the reverb or images out of it. */
    int16_t reverb_hb_in[32];
    int16_t reverb_hb_out[2][16];
    uint8_t reverb_hb_in_pos, reverb_hb_out_pos;
    bool reverb_odd;
    uint16_t reverb_send_permille;
    int16_t sub_dist_curve[257];
    int16_t pending_sub_dist_curve[257];
    int16_t main_dist_curve[257]; /* Tone.Distortion after the main filter: always in the path */
    int16_t pending_main_dist_curve[257];
    int32_t main_dist_prev;

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
/* SUB MUTE as held now: the dedicated and the follower sub fade out in ~10 ms, and back in when let go. */
void mello_audio_set_sub_mute(MelloAudio *a, bool mute);
/* DOT's MOD lanes and EVOLVE's sound routes (the engine's modr). */
void mello_audio_set_mod_routing(MelloAudio *a, const MelloModRouting *modr);
void mello_audio_set_config(MelloAudio *a, const MelloSoundConfig *sound, const MelloFxConfig *fx,
                            const MelloSubConfig *sub, const MelloSampleConfig *sample);
#if MELLO_FEATURE_CHORD_MACHINE
void mello_audio_set_chord_config(MelloAudio *a, const MelloChordConfig *chord);
#endif
void mello_audio_set_drum_config(MelloAudio *a, const MelloDrumConfig *drums);
void mello_audio_attach_sample_bank(MelloAudio *a, MelloSampleBank *bank);
bool mello_audio_push_engine_event(MelloAudio *a, const MelloEvent *event, uint32_t due_us);
void mello_audio_event_sink(void *ctx, const MelloEvent *event, uint32_t due_us);
void mello_audio_render(MelloAudio *a, int32_t *out_stereo, uint32_t frames, uint32_t block_start_us,
                        uint16_t bpm);
void mello_audio_all_notes_off(MelloAudio *a);
bool mello_audio_queue_overflowed(const MelloAudio *a);
bool mello_audio_is_silent(const MelloAudio *a);
#if defined(MELLO_REVERB_TEST_HOOK) && MELLO_REVERB_TEST_HOOK
int32_t mello_audio_reverb_ir_probe(MelloAudio *a, int32_t send);
void mello_audio_reverb_stereo_probe(MelloAudio *a, int32_t send, int32_t *left, int32_t *right);
#endif

#ifdef __cplusplus
}
#endif
#endif
