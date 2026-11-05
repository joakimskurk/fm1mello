/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
#ifndef MELLO_AUDIO_H
#define MELLO_AUDIO_H

#include "mello_engine.h"
#include "mello_sample.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MELLO_AUDIO_FS 44100u
#define MELLO_AUDIO_EVENT_CAP                                                                                \
    64u /* 16 a lane: 300 BPM with 8 ratchets, chords, sub and samples keeps order (audio_timing_stress) */
#define MELLO_AUDIO_EVENT_LANES    4u
#define MELLO_AUDIO_EVENT_LANE_CAP (MELLO_AUDIO_EVENT_CAP / MELLO_AUDIO_EVENT_LANES)
#define MELLO_AUDIO_DELAY_SAMPLES  65536u
#define MELLO_AUDIO_REVERB_SAMPLES 24576u

#define MELLO_PERF_BUF 6144u /* PERFORM's record of the mix: frames at half the rate, ~0.28 s (24 KB) */
#define MELLO_LA_N     64u   /* the output limiter's look-ahead: 1.45 ms */
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
    uint8_t wave; /* the playing step's wave (MelloStep.wave), 0 the sound's */
    uint8_t velocity;
    bool gate;
    uint32_t env_curve_left; /* Tone.Envelope decay/release: exponential part, then a linear tail */
    uint32_t env_linear_left;
    uint32_t env_curve_coeff_q31;
    uint32_t uni_phase[MELLO_UNISON_MAX - 1u]; /* the unison oscillators beside phase; free-running */
    int16_t vel_q15; /* tracking: the note's velocity from the plain step's, +-32767 (an accent is +32767) */
    uint8_t vel_level; /* the velocity the level uses (vel_level sets how far it is from the plain step's) */
    uint32_t osc2_phase;
    int16_t vel_cut;      /* tracking, worked out at the note: VEL > CUTOFF's offset, permille */
    int32_t menv_amt_q15; /* MOD ENV's amount for this note (VEL > MOD ENV in it), Q15 */
    int32_t osc2_q8_last; /* OSC 2: the pitch offset semi_ratio_q16 was last taken for, and what it gave */
    uint32_t osc2_r2;
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
    /* BASS SEQ's filter (RES or ENV above 0): two state-variable low-passes, the second resonant */
    int32_t svf_lp[2], svf_bp[2];
    int32_t svf_f, svf_damp; /* Q15, set every 16 samples */
    uint8_t svf_ctl;
    int32_t fenv, fenv_k; /* Q24: the filter envelope, 1 at a note's attack, and its fall a sample */
    bool accent;
} MelloAudioSubVoice;

#if MELLO_FEATURE_CHORD_MACHINE
/* Voices to play from: a chord's four, and two more so a new chord does not cut the last one's release. SUB MELLO
 * (POLY off) takes the first two: one tone at a time, and one so a new tone does not cut the last one's release. */
#define MELLO_CHORD_AUDIO_VOICES 6u
#define MELLO_SUB_MELLO_VOICES   2u
#define MELLO_CHORD_CHORUS_LEN                                                                               \
    256u /* the chorus's delay line: it reads back 3.5 ms +- 1.8 (234 samples) at most */
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
    int32_t chorus_depth, chorus_mix_q15; /* CHORUS: the swing in samples, and how much is wet */
    int16_t chorus_buf[MELLO_CHORD_CHORUS_LEN];
    uint16_t chorus_pos;
    uint32_t chorus_lfo;
    /* from the config: the envelopes' rates for 16 samples (Q24), the sustain */
    int32_t k_att, k_dec, k_rel, k_fatt, k_fdec, sus_q24;
} MelloAudioChordVoice;
#endif

/* What the audio keeps of the drums' and BASS SEQ's configs: their sound, not their patterns (each copy of a whole
 * config cost RAM the FM-1 does not have: 2026-10-31, 288 bytes over). */
typedef struct {
    bool on;
    uint16_t level, space, drive, fold, duck;
    MelloDrumVoice voice[MELLO_DRUM_VOICES];
} MelloAudioDrumKit;
typedef struct {
    uint16_t res, env, env_decay_ms, accent;
} MelloAudioBassVoice;

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
    MelloAudioDrumKit drum_config;
    MelloAudioDrumKit pending_drum_config;
    MelloAudioBassVoice bass_config;
    MelloAudioBassVoice pending_bass_config;
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
    /* What modulation does to each MelloModDest now, permille of its range (CUTOFF and RES with the LFO, the MOD
     * envelope and EVOLVE's filter in), as of the block's last sample: the screen's second thumbs (ROADMAP F14) */
    int16_t mod_view[MELLO_DEST_COUNT];
    int16_t chord_mod_cut;   /* the chords' share of the routes this block: C.CUT, permille of CUT's range */
    int16_t chord_mod_level; /* C.LEVEL, permille: -1000 silent .. +1000 twice */
    int32_t duck_env;        /* DUCK: the kick's sidechain envelope, Q24, falling over ~160 ms */
    bool sub_mute;           /* SUB MUTE (HOME's performance key), held */
    int32_t sub_gain;        /* the bass's gain, Q15, sliding to 0 or 1 in ~10 ms so muting never clicks */
    uint8_t mix_mute;        /* MIX's mutes: bit 0 LEAD, 1 SUB, 2 CHORD, 3 PERC */
    int16_t
        drum_mod[4]; /* the drums' modulation (DOT, EVOLVE, LFO), -1000..1000: TUNE, DECAY, CRUSH, DRIVE */
    int32_t mix_gain[4]; /* each part's, Q15, sliding as sub_gain does */
    uint16_t effective_decay_ms;
    /* From the config, worked out when it changes (apply_pending), so the per-sample path has no divide: the chip
     * divides in hardware but slowly, and a divide by a constant is a divide too; its widening multiply is one
     * instruction. Gains in Q12 (4096 = 1), modulation depths in Q15. */
    int32_t lfo_depth_q15, lfo_filter_span_q15, lfo_pitch_cents, lfo_pwm_swing, key_track_q15, osc2_q8_base,
        osc2_mix_q15, delay_lc_k, drum_level_q12, drum_fold_q12;
    int32_t send_dly_q12[MELLO_SEND_PARTS],
        send_rev_q12[MELLO_SEND_PARTS]; /* MIX's sends (the chords' x 0.8) */
#if MELLO_FEATURE_CHORD_MACHINE
    int32_t chord_level_q12;
#endif
    int16_t last_lfo;
    /* the control step's values (render: every 4 samples), read by the samples between */
    uint8_t ctl_phase;
    int32_t c_lfo, c_ampmod, c_cut_off, c_res_off, c_noise, c_nfm, c_sat, c_det, c_fm, c_osc2_q8;
    uint32_t c_lead_ratio; /* Q16 on the lead's increment; 65536 = as it is */
    uint16_t c_pwm;
    int16_t tm_filter_mod_q15;
    int16_t tm_decay_mod_q15;

    int16_t *delay_buffer;
    uint32_t delay_capacity;
    uint32_t delay_pos;
    /* HALF RATE (ROADMAP E11/F9): a time past the line's 1.49 s runs it at half the rate, every second sample (two
     * averaged) written, so it reaches 2.97 s (1 BAR from 81 BPM); HI CUT already takes what that loses */
    uint8_t delay_hr, delay_hr_phase;
    int32_t delay_hr_acc;
    uint32_t delay_samples;
    /* Knob changes ramp as the HTML's rampTo calls do (lines 989-1077); the delay time too (read between samples). */
    /* The output stage (DECISIONS "The level, all of it"): the bus compressor's high-pass and follower (Q4) and its
     * gain (Q15, ramped over a block), then the look-ahead limiter's line, its gains' window and their sum */
    int32_t comp_hp_q4, comp_env_q4, comp_g, comp_step;
    uint8_t comp_n, la_pos, la_hold;
    int32_t la_line[2][MELLO_LA_N];
    int16_t la_g[MELLO_LA_N];
    int32_t la_sum, la_min;
    MelloRamp rp_cut, rp_res, rp_master, rp_rev_send, rp_rev_level, rp_dly_send, rp_dly_level, rp_dly_fb,
        rp_dist_level, rp_sub_level, rp_fsub_level, rp_delay, rp_lead_level;
    int32_t delay_tone_s[2][4];                  /* dlyTone */
    int32_t delay_lc[2];                         /* LO CUT: two one-pole low-passes, taken away */
    int32_t delay_lc_r[2], delay_tone_r_s[2][4]; /* PING's second tap: its own LO CUT and HI CUT */
    /* PERFORM's sound (mello_audio_set_perf_fx, MELLO_PERF_FX_*): the sweeps' places (Q20, 0 open) and their state
     * variable filters (per channel: low, band), THROW's and WASH's sends (Q12, sliding), and a tail kept running */
    uint8_t perf_fx;
    uint32_t sweep_pos[2];
    int32_t sweep_f[2], sweep_mix[2]; /* their coefficients, worked out every 16 samples */
    int32_t
        sweep_band; /* 0..4096: LP SWP's output from low-pass to band-pass, while HP SWP is held with it */
    int32_t sweep_s[2][2][2];
    int32_t throw_q12, wash_q12;
    uint32_t perf_tail;
    /* PERFORM's black keys (mello_audio_set_perf_fx2, DECISIONS "PERFORM's black keys"): what is held, the amounts,
     * the notes' transposition; STUT, TAPE and WARP read a record of the mix (stereo, at half the rate, a ring of
     * MELLO_PERF_BUF frames, ~0.28 s), faded in and out; CRUSH's hold, CHOP's gate, PLUCK's envelope */
    uint8_t pfx, pfx_was, pb_mode;
    uint16_t pfx_amt[8];
    int8_t pfx_semis;
    uint16_t render_bpm;
    int16_t pbuf[2][MELLO_PERF_BUF];
    uint16_t pb_w;
    uint8_t pb_half;
    int32_t pb_acc[2], pb_mix_q12;
    uint32_t pb_pos_q16, pb_len, pb_t, pb_k, pb_speed_q16, pb_dec_q16, warp_ph[2];
    uint32_t chop_n, chop_len;
    int32_t chop_g;
    uint8_t crush_n;
    int32_t crush_h[2], pluck_q24, pluck_k;
    uint32_t tape_frames, crush_in,
        warp_in; /* TAPE's stop (frames at half the rate); CRUSH's, WARP's fades in */
    int32_t pluck_mk, pluck_mk_now, chop_mk, stut_q12; /* their levels made up (Q12 over 1; STUT's a gain) */
    bool pluck_up;
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
    uint16_t reverb_ap_delay[3]; /* where SIZE puts them; reverb_ap_now follows once the tail has died away */
    uint16_t reverb_ap_now[3];
    uint16_t reverb_ap_pos[3];
    uint16_t reverb_line_offset[8];
    uint16_t reverb_line_cap[8];
    uint16_t reverb_line_delay[8]; /* where SIZE puts them; reverb_line_dq8 glides there (Q8 samples) */
    uint32_t reverb_line_dq8[8];
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
    /* Gain compensation for the drives (DECISIONS "Distortion keeps its level"), worked out on the main loop when
     * the config changes and taken over with it (apply_pending), Q12: the kit's DRIVE at 0, 62.5 .. 1000 (with its
     * FOLD), each sample slot's DIST. */
    int16_t drum_makeup_q12[17], pending_drum_makeup_q12[17];
    int16_t sample_makeup_q12[MELLO_SAMPLE_SLOT_COUNT], pending_sample_makeup_q12[MELLO_SAMPLE_SLOT_COUNT];
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
    bool ramps_moving; /* a ramp was started: the ramps are stepped each sample until all have arrived */
    bool gains_moving; /* a mute changed: its gain slides until it sits */
    uint32_t tail_guard_frames;
    uint32_t reverb_live; /* frames the reverb still has to run after its last input (render: its own tail) */
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
void mello_audio_set_mix_mute(MelloAudio *a, uint8_t mute_bits);
/* DOT's MOD lanes and EVOLVE's sound routes (the engine's modr). */
void mello_audio_set_mod_routing(MelloAudio *a, const MelloModRouting *modr);
void mello_audio_set_config(MelloAudio *a, const MelloSoundConfig *sound, const MelloFxConfig *fx,
                            const MelloSubConfig *sub, const MelloSampleConfig *sample);
#if MELLO_FEATURE_CHORD_MACHINE
void mello_audio_set_chord_config(MelloAudio *a, const MelloChordConfig *chord);
#endif
void mello_audio_set_drum_config(MelloAudio *a, const MelloDrumConfig *drums);
/* PERFORM's held sound effects (MELLO_PERF_FX_* bits); called as often as the mutes are */
void mello_audio_set_perf_fx(MelloAudio *a, uint8_t fx);
/* PERFORM's black keys (MELLO_PERF_FX2_*) and their amounts (0..1000) */
void mello_audio_set_perf_fx2(MelloAudio *a, uint8_t fx2, const uint16_t amt[8]);
void mello_audio_set_bass_config(MelloAudio *a, const MelloBassConfig *bass);
/* What of a config the audio takes (dinghy.c compares these to know when to send them). */
void mello_audio_drum_kit(const MelloDrumConfig *drums, MelloAudioDrumKit *out);
void mello_audio_bass_voice(const MelloBassConfig *bass, MelloAudioBassVoice *out);
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
