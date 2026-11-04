/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
#include "mello_engine.h"

#define MELLO_ZERO_SEED_REMAP 0x6D2B79F5u

static const int8_t SCALE_CHROMATIC[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
static const int8_t SCALE_MINOR[] = {0, 2, 3, 5, 7, 8, 10};
static const int8_t SCALE_DORIAN[] = {0, 2, 3, 5, 7, 9, 10};
static const int8_t SCALE_PHRYGIAN[] = {0, 1, 3, 5, 7, 8, 10};
static const int8_t SCALE_LYDIAN[] = {0, 2, 4, 6, 7, 9, 11};
static const int8_t SCALE_MIXOLYDIAN[] = {0, 2, 4, 5, 7, 9, 10};
static const int8_t SCALE_PENTATONIC[] = {0, 2, 4, 7, 9};
static const int8_t SCALE_MINOR_PENTATONIC[] = {0, 3, 5, 7, 10};
static const int8_t SCALE_DIMINISHED[] = {0, 2, 3, 5, 6, 8, 9, 11};
static const int8_t SCALE_WHOLE_TONE[] = {0, 2, 4, 6, 8, 10};
static const int8_t SCALE_AUGMENTED[] = {0, 3, 4, 7, 8, 11};
static const int8_t SCALE_MAJOR[] = {0, 2, 4, 5, 7, 9, 11};

typedef struct {
    const int8_t *v;
    uint8_t n;
    const char *name;
} ScaleDef;
static const ScaleDef SCALES[MELLO_SCALE_COUNT] = {
    {SCALE_CHROMATIC, 12, "chromatic"},  {SCALE_MINOR, 7, "minor"},
    {SCALE_DORIAN, 7, "dorian"},         {SCALE_PHRYGIAN, 7, "phrygian"},
    {SCALE_LYDIAN, 7, "lydian"},         {SCALE_MIXOLYDIAN, 7, "mixolydian"},
    {SCALE_PENTATONIC, 5, "pentatonic"}, {SCALE_MINOR_PENTATONIC, 5, "min penta"},
    {SCALE_DIMINISHED, 8, "diminished"}, {SCALE_WHOLE_TONE, 6, "whole tone"},
    {SCALE_AUGMENTED, 6, "augmented"},   {SCALE_MAJOR, 7, "major"}};

static uint8_t clamp_u8_i32(int32_t v, uint8_t lo, uint8_t hi)
{
    if (v < (int32_t)lo) return lo;
    if (v > (int32_t)hi) return hi;
    return (uint8_t)v;
}

static uint8_t clamp_1_8(uint8_t v)
{
    if (v < 1u) return 1u;
    if (v > 8u) return 8u;
    return v;
}

static uint8_t clamp_repeat(uint8_t v)
{
    if (v < 1u) return 1u;
    if (v > 4u) return 4u;
    return v;
}

static uint16_t clamp_hold(uint16_t v)
{
    if (v < 1u) return 1u;
    if (v > 256u) return 256u;
    return v;
}

static int8_t clamp_i8(int32_t v, int8_t lo, int8_t hi)
{
    if (v < (int32_t)lo) return lo;
    if (v > (int32_t)hi) return hi;
    return (int8_t)v;
}

static uint8_t sanitize_visible_slot_count(uint8_t v)
{
    switch (v) {
    case 2u:
    case 4u:
    case 8u:
    case 16u:
    case 32u:
    case 64u: return v;
    default: return 8u;
    }
}

static uint8_t sanitize_slot_seq_length(uint8_t v)
{
    switch (v) {
    case 2u:
    case 4u:
    case 8u:
    case 16u:
    case 32u:
    case 64u: return v;
    default: return 16u;
    }
}

static void sanitize_project(MelloProject *p)
{
    uint8_t i;
    if (!p) return;
    if (p->seq_len < 1u) p->seq_len = 1u;
    if (p->seq_len > MELLO_MAX_STEPS) p->seq_len = MELLO_MAX_STEPS;
    if (!(p->clock_div == 1u || p->clock_div == 2u || p->clock_div == 4u)) p->clock_div = 1u;
    if (p->bpm < 20u) p->bpm = 20u;
    if (p->bpm > 300u) p->bpm = 300u;
    if (p->root_note < 12u) p->root_note = 12u;
    if (p->root_note > 108u) p->root_note = 108u;
    if (p->scale >= MELLO_SCALE_COUNT) p->scale = MELLO_SCALE_MINOR;
    p->global_octave = clamp_i8(p->global_octave, -2, 2);
    if (p->pitch_span_oct < 1u) p->pitch_span_oct = 1u;
    if (p->pitch_span_oct > 3u) p->pitch_span_oct = 3u;
    if (p->seg_count < 1u) p->seg_count = 1u;
    if (p->seg_count > MELLO_MAX_SEGMENTS) p->seg_count = MELLO_MAX_SEGMENTS;
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        p->steps[i].pulses = clamp_1_8(p->steps[i].pulses);
        p->steps[i].ratchet = clamp_1_8(p->steps[i].ratchet);
        if (p->steps[i].gate > MELLO_GATE_MULTI) p->steps[i].gate = MELLO_GATE_SINGLE;
        if (p->steps[i].wave > (uint8_t)MELLO_WAVE_PULSE + 1u) p->steps[i].wave = 0u;
        p->steps[i].octave = clamp_i8(p->steps[i].octave, -2, 2);
        if (p->steps[i].sample_slot > MELLO_SAMPLE_SLOT_COUNT)
            p->steps[i].sample_slot = MELLO_SAMPLE_SLOT_COUNT;
    }
    for (i = 0u; i < MELLO_MAX_SEGMENTS; ++i) {
        p->seg_repeats[i] = clamp_repeat(p->seg_repeats[i]);
        if (p->seg_direction[i] > MELLO_SEG_REV) p->seg_direction[i] = MELLO_SEG_FWD;
        if (p->seg_mode[i] > MELLO_SEG_RANDOM) p->seg_mode[i] = MELLO_SEG_LINEAR;
    }
}

uint8_t mello_loop_cycle_sub_tone(const MelloLoopCycleStep *step)
{
    return step ? (uint8_t)(step->sub_tone & 0x07u) : MELLO_SUB_TONE_ROOT;
}

int8_t mello_loop_cycle_sub_transpose(const MelloLoopCycleStep *step)
{
    uint8_t raw;
    if (!step) return 0;
    raw = (uint8_t)((step->sub_tone >> 3) & 0x1Fu);
    return (int8_t)((raw & 0x10u) ? ((int16_t)raw - 32) : raw);
}

void mello_loop_cycle_set_sub(MelloLoopCycleStep *step, uint8_t tone, int8_t transpose)
{
    uint8_t packed_tr;
    if (!step) return;
    if (tone > MELLO_SUB_TONE_OCTAVE) tone = MELLO_SUB_TONE_ROOT;
    transpose = clamp_i8(transpose, -12, 12);
    packed_tr = (uint8_t)transpose & 0x1Fu;
    step->sub_tone = (uint8_t)((tone & 0x07u) | (uint8_t)(packed_tr << 3));
}

static MelloLoopCycleStep sanitize_loop_step(MelloLoopCycleStep in)
{
    uint8_t sub_tone = mello_loop_cycle_sub_tone(&in);
    int8_t sub_transpose = mello_loop_cycle_sub_transpose(&in);
    in.hold = clamp_hold(in.hold ? in.hold : 16u);
    in.root_offset = clamp_i8(in.root_offset, -12, 12);
    if (in.root_note < -1 || in.root_note > 11) in.root_note = -1;
    if (in.scale_set < -1 || in.scale_set >= (int8_t)MELLO_SCALE_COUNT) in.scale_set = -1;
    in.octave = clamp_i8(in.octave, -2, 2);
    in.pitch = clamp_i8(in.pitch, -12, 12);
    mello_loop_cycle_set_sub(&in, sub_tone, sub_transpose);
    return in;
}

static void sanitize_loop_config(MelloLoopCycleConfig *c)
{
    uint8_t i;
    if (!c) return;
    if (c->length < 2u) c->length = 2u;
    if (c->length > MELLO_MAX_LOOP_CYCLE_STEPS) c->length = MELLO_MAX_LOOP_CYCLE_STEPS;
    for (i = 0u; i < MELLO_MAX_LOOP_CYCLE_STEPS; ++i) c->steps[i] = sanitize_loop_step(c->steps[i]);
}

#if MELLO_FEATURE_CHORD_MACHINE
static MelloChordConfig sanitize_chord_config(MelloChordConfig c)
{
    uint8_t i;
    if (c.trigger > MELLO_CHORD_TRIGGER_GRID) c.trigger = MELLO_CHORD_TRIGGER_STEP;
    if (c.mode >= MELLO_CHORD_MODE_COUNT) c.mode = MELLO_CHORD_STAB;
    if (c.length < 1u) c.length = 1u;
    if (c.length > MELLO_CHORD_MAX_STEPS) c.length = MELLO_CHORD_MAX_STEPS;
    if (c.voices < 2u) c.voices = 2u;
    if (c.voices > MELLO_CHORD_MAX_VOICES) c.voices = MELLO_CHORD_MAX_VOICES;
    if (c.level > 1000u) c.level = 1000u;
    if (c.wave > MELLO_WAVE_SINE) c.wave = MELLO_WAVE_TRIANGLE;
    if (c.gate_amount < 100u) c.gate_amount = 100u;
    if (c.gate_amount > 1200u) c.gate_amount = 1200u;
    if (c.attack_ms < 1u) c.attack_ms = 1u;
    if (c.attack_ms > 2000u) c.attack_ms = 2000u;
    if (c.release_ms < 5u) c.release_ms = 5u;
    if (c.release_ms > 4000u) c.release_ms = 4000u;
    if ((uint32_t)c.grid_mode >= MELLO_DOT_MODE_COUNT) c.grid_mode = MELLO_DOT_HORIZONTAL;
    if (c.grid_length < 1u) c.grid_length = 1u;
    if (c.grid_length > 16u) c.grid_length = 16u;
    if (c.grid_amount > 1000u) c.grid_amount = 1000u;
    if (c.pattern_length < 1u) c.pattern_length = 1u;
    if (c.pattern_length > MELLO_CHORD_PATTERN_STEPS) c.pattern_length = MELLO_CHORD_PATTERN_STEPS;
    for (i = 0u; i < MELLO_CHORD_MAX_STEPS; ++i) {
        MelloChordStep *s = &c.steps[i];
        if (s->shape >= MELLO_CHORD_SHAPE_COUNT) s->shape = MELLO_CHORD_TRIAD;
        if (s->inversion > 3u) s->inversion = 3u;
        s->octave = clamp_i8(s->octave, -2, 2);
        if (s->spread > 3u) s->spread = 3u;
        if (s->hold < 1u) s->hold = 1u;
        if (s->hold > 64u) s->hold = 64u;
    }
    for (i = 0u; i < MELLO_CHORD_PATTERN_STEPS; ++i) c.pattern_masks[i] &= 0x0Fu;
    if (c.chain > MELLO_CHORD_CHAIN_SEG) c.chain = MELLO_CHORD_CHAIN_TRIG;
    if (c.cut > 1000u) c.cut = 1000u;
    if (c.res > 1000u) c.res = 1000u;
    if (c.fenv > 1000u) c.fenv = 1000u;
    if (c.fenv_attack_ms < 1u) c.fenv_attack_ms = 1u;
    if (c.fenv_attack_ms > 2000u) c.fenv_attack_ms = 2000u;
    if (c.fenv_decay_ms < 10u) c.fenv_decay_ms = 10u;
    if (c.fenv_decay_ms > 4000u) c.fenv_decay_ms = 4000u;
    if (c.decay_ms < 10u) c.decay_ms = 10u;
    if (c.decay_ms > 4000u) c.decay_ms = 4000u;
    if (c.sustain > 1000u) c.sustain = 1000u;
    if (c.chorus > 1000u) c.chorus = 1000u;
    if (c.space > 1000u) c.space = 1000u;
    if (c.strum_ms > 60u) c.strum_ms = 60u;
    if (c.lead_follow > 2u) c.lead_follow = 2u;
    if (c.drift > 1000u) c.drift = 1000u;
    if (c.drift_depth > 3u) c.drift_depth = 3u;
    return c;
}
#endif

static uint32_t ms_to_ticks(const MelloProject *p, uint32_t ms)
{
    /* MELLO_PPQN/60 == 280 exactly. Max: 280*250*4000 = 280,000,000,
     * safely inside uint32_t and no target-forbidden 64-bit division. */
    uint32_t bpm = p ? p->bpm : 80u;
    uint32_t num = 280u * bpm * ms;
    return (num + 500u) / 1000u;
}

static uint32_t clamp_duration_ms_as_ticks(const MelloProject *p, uint32_t raw_ticks, uint32_t min_ms,
                                           uint32_t max_ms)
{
    uint32_t lo = ms_to_ticks(p, min_ms);
    uint32_t hi = ms_to_ticks(p, max_ms);
    if (raw_ticks < lo) return lo;
    if (raw_ticks > hi) return hi;
    return raw_ticks;
}

void mello_prng_seed(MelloPrng *rng, uint32_t seed)
{
    if (!rng) return;
    rng->state = seed ? seed : MELLO_ZERO_SEED_REMAP;
}

uint32_t mello_prng_next_u32(MelloPrng *rng)
{
    uint32_t x;
    if (!rng) return 0u;
    x = rng->state;
    if (!x) x = MELLO_ZERO_SEED_REMAP;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng->state = x;
    return x;
}

static uint32_t rng_bounded(MelloPrng *rng, uint32_t bound)
{
    return bound ? (mello_prng_next_u32(rng) % bound) : 0u;
}

static bool probability_pass(MelloEngine *e, uint16_t p)
{
    uint32_t r = mello_prng_next_u32(&e->rng);
    /* UINT32_MAX == 65535 * 65537, so p/65535 maps exactly to p*65537. */
    uint32_t threshold = ((uint32_t)p << 16) | (uint32_t)p;
    return r <= threshold;
}

static uint16_t gen_unit16(MelloPrng *rng)
{
    return (uint16_t)(mello_prng_next_u32(rng) >> 16);
}

static bool chance_permille(MelloPrng *rng, uint16_t amount)
{
    uint32_t lhs = (uint32_t)gen_unit16(rng) * MELLO_AMOUNT_MAX;
    if (amount > MELLO_AMOUNT_MAX) amount = MELLO_AMOUNT_MAX;
    return lhs < (uint32_t)amount * 65536u;
}

static uint16_t clamp_amount(uint16_t v)
{
    return v > MELLO_AMOUNT_MAX ? MELLO_AMOUNT_MAX : v;
}

static uint16_t clamp_u16_range(uint16_t v, uint16_t lo, uint16_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static MelloSoundConfig sanitize_sound_config(MelloSoundConfig c)
{
    if (c.osc_wave > MELLO_WAVE_OFF) c.osc_wave = MELLO_WAVE_SAW;
    c.pwm_permille = clamp_u16_range(c.pwm_permille, 50u, 950u);
    c.attack_ms = clamp_u16_range(c.attack_ms, 1u, 500u);
    c.decay_ms = clamp_u16_range(c.decay_ms, 5u, 1000u);
    c.sustain_permille = clamp_amount(c.sustain_permille);
    c.release_ms = clamp_u16_range(c.release_ms, 5u, 2000u);
    c.slide_ms = clamp_u16_range(c.slide_ms, 0u, 300u);
    c.filter_cut = clamp_amount(c.filter_cut);
    c.filter_res = clamp_amount(c.filter_res);
    c.lfo_rate_millihz = clamp_u16_range(c.lfo_rate_millihz, 8u, 20000u);
    c.lfo_depth = clamp_amount(c.lfo_depth);
    if (c.lfo_target >= MELLO_LFO_TARGET_COUNT) c.lfo_target = MELLO_LFO_FILTER;
    c.follower_sub_level = clamp_amount(c.follower_sub_level);
    c.follower_sub_octave = clamp_i8(c.follower_sub_octave, -2, 0);
    if (c.follower_sub_wave > MELLO_WAVE_OFF) c.follower_sub_wave = MELLO_WAVE_SINE;
    c.master_level = clamp_amount(c.master_level);
    c.lead_level = clamp_amount(c.lead_level);
    c.noise_level = clamp_amount(c.noise_level);
    c.noise_fm = clamp_amount(c.noise_fm);
    if (c.unison < 1u) c.unison = 1u;
    if (c.unison > MELLO_UNISON_MAX) c.unison = MELLO_UNISON_MAX;
    c.detune = clamp_amount(c.detune);
    if (c.osc2_wave > MELLO_WAVE_OFF) c.osc2_wave = MELLO_WAVE_OFF;
    c.osc2_semi = clamp_i8(c.osc2_semi, -24, 24);
    c.osc2_fine = clamp_i8(c.osc2_fine, -50, 50);
    c.osc2_mix = clamp_amount(c.osc2_mix);
    c.fm_amount = clamp_amount(c.fm_amount);
    c.saturation = clamp_amount(c.saturation);
    if (c.filter_mode >= MELLO_FILTER_MODE_COUNT) c.filter_mode = MELLO_FILTER_LP;
    if (c.menv_amount > 1000) c.menv_amount = 1000;
    if (c.menv_amount < -1000) c.menv_amount = -1000;
    c.menv_attack_ms = clamp_u16_range(c.menv_attack_ms, 0u, 2000u);
    c.menv_decay_ms = clamp_u16_range(c.menv_decay_ms, 5u, 5000u);
    if (c.menv_target >= MELLO_MENV_TARGET_COUNT) c.menv_target = MELLO_MENV_CUTOFF;
    if (c.vel_cutoff > 1000) c.vel_cutoff = 1000;
    if (c.vel_cutoff < -1000) c.vel_cutoff = -1000;
    if (c.vel_level > 2000u) c.vel_level = 2000u;
    c.key_track = clamp_amount(c.key_track);
    if (c.vel_menv > 1000) c.vel_menv = 1000;
    if (c.vel_menv < -1000) c.vel_menv = -1000;
    return c;
}

static MelloSampleSlotConfig sanitize_sample_slot(MelloSampleSlotConfig c)
{
    c.coarse_semitones = clamp_i8(c.coarse_semitones, -24, 24);
    c.fine_cents = clamp_i8(c.fine_cents, -100, 100);
    c.level = clamp_u16_range(c.level, 0u, 1500u);
    c.dist = clamp_amount(c.dist);
    c.trim_start = clamp_u16_range(c.trim_start, 0u, 990u);
    c.trim_end = clamp_u16_range(c.trim_end, 10u, 1000u);
    if (c.trim_end <= c.trim_start) {
        c.trim_end = (uint16_t)(c.trim_start + 10u);
        if (c.trim_end > 1000u) {
            c.trim_end = 1000u;
            c.trim_start = 990u;
        }
    }
    c.attack_ms = clamp_u16_range(c.attack_ms, 1u, 2000u);
    c.decay_ms = clamp_u16_range(c.decay_ms, 5u, 2000u);
    c.sustain_permille = clamp_amount(c.sustain_permille);
    c.release_ms = clamp_u16_range(c.release_ms, 5u, 3000u);
    return c;
}

static MelloSampleConfig sanitize_sample_config(MelloSampleConfig c)
{
    uint8_t i;
    if (c.edit_slot >= MELLO_SAMPLE_SLOT_COUNT) c.edit_slot = 0u;
    for (i = 0u; i < MELLO_SAMPLE_SLOT_COUNT; ++i) c.slots[i] = sanitize_sample_slot(c.slots[i]);
    return c;
}

static MelloFxConfig sanitize_fx_config(MelloFxConfig c)
{
    c.dist_amount = clamp_amount(c.dist_amount);
    c.dist_mix = clamp_amount(c.dist_mix);
    c.dist_character = clamp_amount(c.dist_character);
    c.dist_tone = clamp_amount(c.dist_tone);
    c.dist_level = clamp_u16_range(c.dist_level, 200u, 1400u);
    c.delay_wet = clamp_amount(c.delay_wet);
    if (c.delay_div > 13u) c.delay_div = 0u; /* 9..13: 32N, 16D, 2T, 2D, 1 BAR (2026-11-03) */
    c.delay_time_ms = clamp_u16_range(c.delay_time_ms, 30u, 1500u);
    c.delay_feedback = clamp_u16_range(c.delay_feedback, 0u, 950u);
    c.delay_tone = clamp_amount(c.delay_tone);
    c.delay_lowcut = clamp_amount(c.delay_lowcut);
    c.delay_level = clamp_u16_range(c.delay_level, 0u, 1500u);
    c.reverb_wet = clamp_amount(c.reverb_wet);
    c.reverb_size = clamp_amount(c.reverb_size);
    c.reverb_decay_ms = clamp_u16_range(c.reverb_decay_ms, 400u, 60000u);
    c.reverb_pre_ms = clamp_u16_range(c.reverb_pre_ms, 0u, 120u);
    c.reverb_tone = clamp_amount(c.reverb_tone);
    c.reverb_level = clamp_u16_range(c.reverb_level, 0u, 3000u);
    {
        uint8_t i;
        for (i = 0u; i < MELLO_SEND_PARTS; ++i) {
            c.send_dly[i] = clamp_amount(c.send_dly[i]);
            c.send_rev[i] = clamp_amount(c.send_rev[i]);
        }
    }
    return c;
}

static uint8_t sanitize_tm_length(uint8_t v)
{
    if (v < 3u) return 3u;
    if (v > MELLO_TM_MAX_STEPS) return MELLO_TM_MAX_STEPS;
    return v;
}

static uint8_t sanitize_sub_x_length(uint8_t v)
{
    if (v == 2u || v == 3u || v == 5u) return v;
    return 5u;
}

static MelloDotMatrixConfig sanitize_dot_config(MelloDotMatrixConfig c)
{
    if ((uint32_t)c.mode >= MELLO_DOT_MODE_COUNT) c.mode = MELLO_DOT_HORIZONTAL;
    if (c.length < 1u) c.length = 1u;
    if (c.length > MELLO_DOT_CELL_COUNT) c.length = MELLO_DOT_CELL_COUNT;
    c.amount = clamp_amount(c.amount);
    return c;
}

static MelloModRouting sanitize_mod_routing(MelloModRouting c)
{
    uint8_t i;
    for (i = 0u; i < MELLO_DOT_MOD_LANES; ++i) {
        if (c.dot_target[i] == MELLO_DEST_OFF || c.dot_target[i] >= MELLO_DEST_COUNT)
            c.dot_target[i] = i ? MELLO_DEST_NOISE : MELLO_DEST_CUTOFF;
        c.dot_attack_ms[i] = clamp_u16_range(c.dot_attack_ms[i], 0u, 2000u);
        c.dot_release_ms[i] = clamp_u16_range(c.dot_release_ms[i], 5u, 4000u);
        if (c.tm_dest[i] >= MELLO_DEST_COUNT) c.tm_dest[i] = MELLO_DEST_OFF;
        if (c.tm_depth[i] > 1000) c.tm_depth[i] = 1000;
        if (c.tm_depth[i] < -1000) c.tm_depth[i] = -1000;
    }
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) c.dot_evolve[i] = clamp_amount(c.dot_evolve[i]);
    c.rnd_mirror &= MELLO_RAND_LINKABLE;
    for (i = 0u; i < MELLO_RAND_TARGETS; ++i)
        if (c.rnd_amount[i] > 100u) c.rnd_amount[i] = 100u;
    c.rnd_follow &= (uint16_t)(MELLO_RAND_LINKABLE & ~c.rnd_mirror);
    return c;
}

static MelloTmConfig sanitize_tm_config(MelloTmConfig c)
{
    c.mutation_prob = c.mutation_prob > MELLO_TM_PROB_MAX ? MELLO_TM_PROB_MAX : c.mutation_prob;
    c.length = sanitize_tm_length(c.length);
    if (c.rate > MELLO_TM_RATE_DOUBLE) c.rate = MELLO_TM_RATE_ONE;
    c.offset = clamp_i8(c.offset, -31, 31);
    c.routes &= (uint8_t)(MELLO_TM_ROUTE_LENGTH | MELLO_TM_ROUTE_FILTER | MELLO_TM_ROUTE_OCTAVE |
                          MELLO_TM_ROUTE_DECAY | MELLO_TM_ROUTE_RETRIG);
    c.depth_gate = clamp_amount(c.depth_gate);
    c.depth_filter = clamp_amount(c.depth_filter);
    c.depth_octave = clamp_amount(c.depth_octave);
    c.depth_decay = clamp_amount(c.depth_decay);
    return c;
}

static MelloSubConfig sanitize_sub_config(MelloSubConfig c)
{
    uint8_t i;
    if (c.mode > MELLO_SUB_MODE_DRONE) c.mode = MELLO_SUB_MODE_STEP;
    if (c.euclid_pulses < 1u) c.euclid_pulses = 1u;
    if (c.euclid_pulses > 64u) c.euclid_pulses = 64u;
    if (c.euclid_rotate > 64u) c.euclid_rotate = 64u;
    c.octave = clamp_i8(c.octave, -2, 1);
    if (c.gate_amount < 150u) c.gate_amount = 150u;
    if (c.gate_amount > 1200u) c.gate_amount = 1200u;
    c.level = clamp_amount(c.level);
    if (c.wave > MELLO_WAVE_OFF) c.wave = MELLO_WAVE_SINE;
    c.decay_ms = clamp_u16_range(c.decay_ms, 30u, 700u);
    c.release_ms = clamp_u16_range(c.release_ms, 30u, 1200u);
    c.glide_ms = clamp_u16_range(c.glide_ms, 0u, 200u);
    c.cut = clamp_amount(c.cut);
    c.drive = clamp_amount(c.drive);
    c.x_length = sanitize_sub_x_length(c.x_length);
    for (i = 0u; i < MELLO_SUB_X_MAX_STEPS; ++i) {
        if (c.x_steps[i].tone > MELLO_SUB_TONE_OCTAVE) c.x_steps[i].tone = MELLO_SUB_TONE_ROOT;
        c.x_steps[i].octave = clamp_i8(c.x_steps[i].octave, -2, 2);
    }
    return c;
}

static bool queue_push(MelloEngine *e, const MelloEvent *in)
{
    MelloEvent late;
    const MelloEvent *ev = in;
    uint8_t pos, prev;
    if (e->swing_off) { /* SWING: this pulse's sounds, late together */
        late = *in;
        late.tick += e->swing_off;
        ev = &late;
    }
    if (e->q_count >= MELLO_EVENT_QUEUE_CAP) {
        e->queue_overflow = true;
        return false;
    }
    /* Events produced by one engine pulse include current NOTE ONs and future NOTE OFFs.
     * Keep this main-thread-only queue stably ordered by tick before transport publishes
     * them to the audio ISR. Equal-tick events preserve their source order. */
    pos = e->q_tail;
    while (pos != e->q_head) {
        prev = (uint8_t)(pos == 0u ? MELLO_EVENT_QUEUE_CAP - 1u : pos - 1u);
        if ((int32_t)(e->queue[prev].tick - ev->tick) <= 0) break;
        e->queue[pos] = e->queue[prev];
        pos = prev;
    }
    e->queue[pos] = *ev;
    e->q_tail = (uint8_t)((e->q_tail + 1u) % MELLO_EVENT_QUEUE_CAP);
    e->q_count++;
    return true;
}

bool mello_event_pop(MelloEngine *e, MelloEvent *out)
{
    if (!e || !out || e->q_count == 0u) return false;
    *out = e->queue[e->q_head];
    e->q_head = (uint8_t)((e->q_head + 1u) % MELLO_EVENT_QUEUE_CAP);
    e->q_count--;
    return true;
}

void mello_event_clear(MelloEngine *e)
{
    if (!e) return;
    e->q_head = e->q_tail = e->q_count = 0u;
    e->queue_overflow = false;
}

bool mello_event_overflowed(const MelloEngine *e)
{
    return e ? e->queue_overflow : false;
}

void mello_project_defaults(MelloProject *p)
{
    uint8_t i;
    if (!p) return;
    memset(p, 0, sizeof(*p));
    p->seq_len = 16u;
    p->bpm = 80u;
    p->clock_div = 1u;
    p->root_note = 60u;
    p->scale = MELLO_SCALE_MINOR;
    p->global_octave = 0;
    p->pitch_span_oct = 2u;
    for (i = 0; i < MELLO_MAX_STEPS; ++i) {
        p->steps[i].pitch = (int8_t)(i % 7u);
        p->steps[i].pulses = 1u;
        p->steps[i].ratchet = 1u;
        p->steps[i].gate = MELLO_GATE_SINGLE;
        p->steps[i].octave = 0;
        p->steps[i].slide = false;
        p->steps[i].skip = false;
        p->steps[i].accent = false;
        p->steps[i].sub = false;
        p->steps[i].sample_slot = 1u;
        p->steps[i].probability = 65535u;
    }
    /* Four segments of four, each once forward: it plays as one of sixteen, ready to be changed (the HTML has one). */
    p->seg_count = 4u;
    p->seg_splits[0] = 4u;
    p->seg_splits[1] = 8u;
    p->seg_splits[2] = 12u;
    p->seg_splits[3] = 14u;
    p->seg_splits[4] = 15u;
    for (i = 0u; i < MELLO_MAX_SEGMENTS; ++i) {
        p->seg_repeats[i] = 1u;
        p->seg_direction[i] = MELLO_SEG_FWD;
        p->seg_mode[i] = MELLO_SEG_LINEAR;
        p->seg_skip[i] = false;
    }
    p->master_len = 32u; /* WP-D: default 2 BAR = 32 sixteenth notes. */
}

void mello_loop_cycle_defaults(MelloLoopCycleConfig *c)
{
    uint8_t i;
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->active = false;
    c->sync = true;
    c->length = 4u;
    for (i = 0u; i < MELLO_MAX_LOOP_CYCLE_STEPS; ++i) {
        c->steps[i].hold = 16u;
        c->steps[i].root_offset = 0;
        c->steps[i].root_note = -1;
        c->steps[i].scale_set = -1;
        c->steps[i].octave = 0;
        c->steps[i].pitch = 0;
        mello_loop_cycle_set_sub(&c->steps[i], MELLO_SUB_TONE_ROOT, 0);
    }
}

void mello_slot_seq_defaults(MelloSlotSeqConfig *c)
{
    uint8_t i;
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->active = false;
    c->length = 16u;
    for (i = 0u; i < MELLO_MAX_SLOT_SEQ_STEPS; ++i) {
        c->steps[i].slot = -1;
        c->steps[i].hold = 16u;
    }
}

void mello_dot_matrix_defaults(MelloDotMatrixConfig *c)
{
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->cells = 0u;
    c->mode = MELLO_DOT_HORIZONTAL;
    c->length = 16u;
    c->reset = true;
    c->follow = false;
    c->amount = MELLO_AMOUNT_MAX;
}

void mello_dot_defaults_for(MelloDotMatrixConfig *c, uint8_t id)
{
    mello_dot_matrix_defaults(c);
    if (c && (id == MELLO_DOT_MOD1 || id == MELLO_DOT_MOD2)) c->amount = 240u;
}

void mello_mod_routing_defaults(MelloModRouting *c)
{
    uint8_t i;
    if (!c) return;
    memset(c, 0, sizeof(*c));
    for (i = 0u; i < MELLO_DOT_MOD_LANES; ++i) {
        /* The HTML's filter dot: 20 ms up, 140 ms down, one hit per lit cell. */
        c->dot_attack_ms[i] = 20u;
        c->dot_release_ms[i] = 140u;
        c->tm_depth[i] = 500;
    }
    c->dot_target[0] = MELLO_DEST_CUTOFF;
    c->dot_target[1] = MELLO_DEST_NOISE;
    c->tm_on_loop = true;
    memset(c->rnd_amount, 100, sizeof c->rnd_amount);
}

void mello_tm_defaults(MelloTmConfig *c)
{
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->on = false;
    c->locked = false;
    c->mutation_prob = 100u;
    c->length = 8u;
    c->rate = MELLO_TM_RATE_ONE;
    c->offset = 0;
    c->routes = MELLO_TM_ROUTE_LENGTH;
    c->depth_gate = 350u;
    c->depth_filter = 350u;
    c->depth_octave = 350u;
    c->depth_decay = 350u;
}

void mello_sub_defaults(MelloSubConfig *c)
{
    uint8_t i;
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->on = true;
    c->mode = MELLO_SUB_MODE_STEP;
    c->euclid_pulses = 6u;
    c->euclid_rotate = 0u;
    c->octave = -1;
    c->gate_amount = 900u;
    c->level = 750u; /* 2026-11-04: was 420, 7.5 dB under the lead alone; now ~2.5 dB under (DECISIONS "The mix") */
    c->wave = MELLO_WAVE_SINE;
    c->decay_ms = 160u;
    c->release_ms = 120u;
    c->glide_ms = 20u;
    c->cut = 450u;
    c->drive = 0u;
    c->x_on = false;
    c->x_reset = true;
    c->x_length = 5u;
    for (i = 0u; i < MELLO_SUB_X_MAX_STEPS; ++i) {
        c->x_steps[i].on = (i == 0u);
        c->x_steps[i].tone = MELLO_SUB_TONE_ROOT;
        c->x_steps[i].octave = 0;
    }
}

void mello_drum_defaults(MelloDrumConfig *c)
{
    /* The default beat, BROKEN (feedback 2026-10-31: "I like 3"): a syncopated kick, the snare on the backbeat with a
     * ghost before the second, sixteenths of hat that leave gaps, a few glitches that jump in pitch. Straight: SWING 0,
     * as the sequence, which has none (DECISIONS.md "Drums"). The glitch kit of the same day: a short kick with no
     * PITCH on its steps, a snare between crack and zap, a closed hat with a little tick, GLITCH as STUTTER; CRUSH 0. */
    static const MelloDrumVoice v[MELLO_DRUM_VOICES] = {
        {400u, 180u, 250u, 900u, 1000u, 0u}, /* KICK ~59 Hz, 340 ms, driven: weight (2026-11-01) */
        {500u, 250u, 350u, 750u, 1000u, 0u}, /* SNARE ~2 kHz, between crack and zap */
        {500u, 60u, 200u, 380u, 1000u, 0u},  /* HAT ~7 kHz, closed, a little tick */
        {500u, 300u, 875u, 600u, 1000u, 0u}  /* GLITCH: STUTTER, ~26 ms loops */
    };
    /* Bit k is step k + 1:  KICK x..x...x..x..x..  SNARE ....x....x..x...  HAT x.xxx.x.x.xxx.xx  GLITCH .....x.....x..x. */
    static const uint16_t hits[MELLO_DRUM_VOICES] = {0x2489u, 0x1210u, 0xDD5Du, 0x4820u},
                          accents[MELLO_DRUM_VOICES] = {0x0081u, 0x1010u, 0x1010u, 0x4020u};
    /* The locks that differ from none: voice, step, PITCH, DECAY, RATCH. */
    static const struct {
        uint8_t v, k;
        int8_t pitch, decay;
        uint8_t ratchet;
    } locks[] = {{MELLO_DRUM_KICK, 13u, 0, -50, 1u}, {MELLO_DRUM_SNARE, 9u, -2, -60, 1u}, /* the ghost */
                 {MELLO_DRUM_HAT, 3u, 3, 0, 1u},     {MELLO_DRUM_HAT, 11u, 5, 0, 1u},
                 {MELLO_DRUM_HAT, 15u, 7, 0, 4u},    {MELLO_DRUM_GLITCH, 5u, -5, 0, 1u},
                 {MELLO_DRUM_GLITCH, 11u, 7, 0, 1u}, {MELLO_DRUM_GLITCH, 14u, 12, 0, 2u}};
    uint8_t i, k;
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->length = 16u;
    c->swing = 0u;
    c->level = 950u; /* 70% to 80% (2026-11-01), to 95% (2026-11-04): a little over the lead (DECISIONS "The mix") */
    c->space = 250u;
    c->roll = 500u;
    for (i = 0u; i < MELLO_DRUM_VOICES; ++i) {
        c->voice[i] = v[i];
        c->hits[i] = hits[i];
        c->accents[i] = accents[i];
        for (k = 0u; k < MELLO_DRUM_STEPS; ++k) c->lock[i][k].ratchet = 1u;
    }
    for (i = 0u; i < sizeof locks / sizeof locks[0]; ++i) {
        MelloDrumLock *l = &c->lock[locks[i].v][locks[i].k];
        l->pitch = locks[i].pitch;
        l->decay = locks[i].decay;
        l->ratchet = locks[i].ratchet;
    }
}

static MelloDrumConfig sanitize_drum_config(MelloDrumConfig c)
{
    uint8_t i, k;
    if (c.length < 1u) c.length = 1u;
    if (c.length > MELLO_DRUM_STEPS) c.length = MELLO_DRUM_STEPS;
    if (c.swing > 1000u) c.swing = 1000u;
    if (c.level > 1000u) c.level = 1000u;
    if (c.space > 1000u) c.space = 1000u;
    if (c.roll > 1000u) c.roll = 1000u;
    if (c.drive > 1000u) c.drive = 1000u;
    if (c.fold > 1000u) c.fold = 1000u;
    if (c.duck > 1000u) c.duck = 1000u;
    for (i = 0u; i < MELLO_DRUM_VOICES; ++i) {
        MelloDrumVoice *v = &c.voice[i];
        if (v->tune > 1000u) v->tune = 1000u;
        if (v->decay > 1000u) v->decay = 1000u;
        if (v->tone > 1000u) v->tone = 1000u;
        if (v->level > 1000u) v->level = 1000u;
        if (v->chance > 1000u) v->chance = 1000u;
        if (v->crush > 1000u) v->crush = 1000u;
        for (k = 0u; k < MELLO_DRUM_STEPS; ++k) {
            MelloDrumLock *l = &c.lock[i][k];
            l->pitch = clamp_i8(l->pitch, -24, 24);
            l->decay = clamp_i8(l->decay, -100, 100);
            if (l->crush > 100u) l->crush = 100u;
            if (l->ratchet < 1u) l->ratchet = 1u;
            if (l->ratchet > 4u) l->ratchet = 4u;
        }
    }
    return c;
}

void mello_bass_defaults(MelloBassConfig *c)
{
    /* Off; when turned on, a line under the default beat: the root with an accent on the one, again on 4, up an
     * octave on 7 sliding down into the fifth on 8, the root (accented) on 11, the third on 14, the seventh on 15
     * sliding home to the root on 16. A glide reaches only a step that plays next. */
    static const struct {
        uint8_t k, tone;
        int8_t octave;
        bool glide, accent;
    } line[8] = {{0u, MELLO_SUB_TONE_ROOT, 0, false, true},     {3u, MELLO_SUB_TONE_ROOT, 0, false, false},
                 {6u, MELLO_SUB_TONE_OCTAVE, 0, true, false},   {7u, MELLO_SUB_TONE_FIFTH, 0, false, false},
                 {10u, MELLO_SUB_TONE_ROOT, 0, false, true},    {13u, MELLO_SUB_TONE_THIRD, 0, false, false},
                 {14u, MELLO_SUB_TONE_SEVENTH, 0, true, false}, {15u, MELLO_SUB_TONE_ROOT, 0, false, false}};
    uint8_t i;
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->length = 16u;
    c->env_decay_ms = 300u;
    c->accent = 500u;
    for (i = 0u; i < 8u; ++i) {
        MelloBassStep *s = &c->steps[line[i].k];
        s->on = true;
        s->tone = line[i].tone;
        s->octave = line[i].octave;
        s->glide = line[i].glide;
        s->accent = line[i].accent;
    }
}

static MelloBassConfig sanitize_bass_config(MelloBassConfig c)
{
    uint8_t i;
    if (c.length < 1u) c.length = 1u;
    if (c.length > MELLO_BASS_STEPS) c.length = MELLO_BASS_STEPS;
    if (c.res > 1000u) c.res = 1000u;
    if (c.env > 1000u) c.env = 1000u;
    c.env_decay_ms = clamp_u16_range(c.env_decay_ms, 20u, 2000u);
    if (c.accent > 1000u) c.accent = 1000u;
    for (i = 0u; i < MELLO_BASS_STEPS; ++i) {
        if (c.steps[i].tone > MELLO_SUB_TONE_OCTAVE) c.steps[i].tone = MELLO_SUB_TONE_ROOT;
        c.steps[i].octave = clamp_i8(c.steps[i].octave, -2, 2);
    }
    return c;
}

void mello_engine_bass_set_config(MelloEngine *e, const MelloBassConfig *config)
{
    if (!e || !config) return;
    e->bass = sanitize_bass_config(*config);
    if (e->bass_pos >= e->bass.length) e->bass_pos = 0u;
}

void mello_engine_drums_set_config(MelloEngine *e, const MelloDrumConfig *config)
{
    if (!e || !config) return;
    e->drums = sanitize_drum_config(*config);
    if (e->drum_pos >= e->drums.length) e->drum_pos = 0u;
}

void mello_engine_drums_roll(MelloEngine *e)
{
    /* How likely each voice is to hit a step when rolled (permille), and how far its pitch goes. */
    static const uint16_t density[MELLO_DRUM_VOICES] = {220u, 180u, 550u, 170u};
    static const int8_t pitch_span[MELLO_DRUM_VOICES] = {0, 7, 12, 12}; /* the kick stays where it is tuned */
    MelloDrumConfig *d;
    uint8_t v, k;
    if (!e) return;
    d = &e->drums;
    for (v = 0u; v < MELLO_DRUM_VOICES; ++v)
        for (k = 0u; k < d->length; ++k) {
            MelloDrumLock *l = &d->lock[v][k];
            uint16_t bit = (uint16_t)(1u << k), dens = density[v];
            if (!chance_permille(&e->drum_rng, d->roll)) continue;
            if (v == MELLO_DRUM_SNARE && (k % 8u) == 4u) dens = 850u; /* the backbeat stays likely */
            if (v == MELLO_DRUM_KICK && (k % 8u) == 0u) dens = 750u;
            if (chance_permille(&e->drum_rng, dens))
                d->hits[v] |= bit;
            else
                d->hits[v] &= (uint16_t)~bit;
            if (chance_permille(&e->drum_rng, 200u))
                d->accents[v] |= bit;
            else
                d->accents[v] &= (uint16_t)~bit;
            l->pitch = chance_permille(&e->drum_rng, 500u)
                           ? 0
                           : (int8_t)((int32_t)rng_bounded(&e->drum_rng, (uint32_t)pitch_span[v] * 2u + 1u) -
                                      pitch_span[v]);
            l->decay = chance_permille(&e->drum_rng, 500u)
                           ? 0
                           : (int8_t)((int32_t)rng_bounded(&e->drum_rng, 161u) - 80);
            if (v == MELLO_DRUM_KICK && l->decay > 0)
                l->decay = 0; /* shorter, never longer: it leaves the bass room */
            l->crush =
                chance_permille(&e->drum_rng, 700u) ? 0u : (uint8_t)(10u + rng_bounded(&e->drum_rng, 91u));
            l->ratchet =
                chance_permille(&e->drum_rng, 850u) ? 1u : (uint8_t)(2u + rng_bounded(&e->drum_rng, 3u));
        }
    d->hits[MELLO_DRUM_KICK] |= 1u; /* the one */
    if (d->feed_sound) { /* FEED+: each voice's TUNE, DECAY and TONE, within what suits the voice */
        static const uint16_t lo[MELLO_DRUM_VOICES][3] = {{250u, 40u, 0u},
                                                          {150u, 120u, 0u},
                                                          {0u, 20u, 0u},
                                                          {0u, 100u, 0u}},
                              hi[MELLO_DRUM_VOICES][3] = {{600u, 250u, 350u},
                                                          {900u, 500u, 1000u},
                                                          {1000u, 350u, 800u},
                                                          {1000u, 700u, 999u}};
        for (v = 0u; v < MELLO_DRUM_VOICES; ++v) {
            uint16_t *p[3] = {&d->voice[v].tune, &d->voice[v].decay, &d->voice[v].tone};
            for (k = 0u; k < 3u; ++k)
                if (chance_permille(&e->drum_rng, d->roll))
                    *p[k] = (uint16_t)(lo[v][k] +
                                       rng_bounded(&e->drum_rng, (uint32_t)(hi[v][k] - lo[v][k]) + 1u));
        }
    }
}

/* ROLL on BASS (DECISIONS.md "A bass line that fits"). A bass player under this sequence: the one is the root, the
 * other beats (every fourth step) are roots or fifths, or the lead's note where the lead has a chord tone there; the
 * offbeats are walks into the next beat (the second or the seventh under a root, the fourth or sixth under a fifth),
 * octave jumps before a root, or the lead's note; the line sits an octave down, with octave jumps up to 0 for the
 * fills. Five to eleven notes of sixteen, always the one; glides where a note runs into the next, accents on the
 * one and the three and a few more. A roll never gives two lines alike (the rng moves on). */
static uint8_t mus_degree(const ScaleDef *sd, int8_t semis); /* the MUSICAL randomizer's, below */
void mello_engine_bass_roll(MelloEngine *e)
{
    static const uint8_t walk_to_root[3] = {MELLO_SUB_TONE_SEVENTH, MELLO_SUB_TONE_SECOND,
                                            MELLO_SUB_TONE_FIFTH},
                         walk_to_fifth[3] = {MELLO_SUB_TONE_FOURTH, MELLO_SUB_TONE_SIXTH,
                                             MELLO_SUB_TONE_ROOT};
    MelloBassConfig b;
    const ScaleDef *sd;
    uint8_t n, k, len, hits = 0u, lead_deg[MELLO_BASS_STEPS], beat_tone[MELLO_BASS_STEPS];
    bool lead_chord[MELLO_BASS_STEPS];
    int16_t density;
    if (!e) return;
    b = e->bass;
    sd = &SCALES[e->project.scale < MELLO_SCALE_COUNT ? e->project.scale : MELLO_SCALE_MINOR];
    n = sd->n;
    len = b.length ? (b.length > MELLO_BASS_STEPS ? MELLO_BASS_STEPS : b.length) : 16u;
    density = (int16_t)((int32_t)rng_bounded(&e->gen_rng, 3u) * 120 - 120); /* sparse, as is, busy */
    /* what the lead plays at each bass step: its scale degree, and whether that is a chord tone (0, 3rd, 5th) */
    for (k = 0u; k < len; ++k) {
        const MelloStep *st = &e->project.steps[k % (e->project.seq_len ? e->project.seq_len : 16u)];
        uint8_t deg = (uint8_t)((st->pitch < 0 ? 0 : st->pitch) % (n ? n : 7u));
        lead_deg[k] = deg;
        lead_chord[k] =
            st->gate != MELLO_GATE_REST && !st->skip &&
            (deg == 0u || deg == mus_degree(sd, 3) || deg == mus_degree(sd, 4) || deg == mus_degree(sd, 7));
    }
    /* the beats first: the one is the root; the others a root or a fifth, or the lead's chord tone when it has one */
    for (k = 0u; k < len; ++k) {
        MelloBassStep *s = &b.steps[k];
        /* at the default line's octave (0): the voice itself sits an octave down since 2026-11-02, and -1 here put a
         * rolled line a second octave under it (~33 Hz) */
        s->on = false, s->glide = false, s->accent = false, s->octave = 0, s->tone = MELLO_SUB_TONE_ROOT;
        if (k % 4u) continue;
        if (k == 0u)
            beat_tone[k] = MELLO_SUB_TONE_ROOT;
        else if (lead_chord[k] && chance_permille(&e->gen_rng, 550u))
            beat_tone[k] =
                lead_deg[k] == 0u ? MELLO_SUB_TONE_ROOT : (uint8_t)lead_deg[k]; /* the degree is the tone */
        else
            beat_tone[k] = chance_permille(&e->gen_rng, 600u) ? MELLO_SUB_TONE_ROOT : MELLO_SUB_TONE_FIFTH;
        s->tone = beat_tone[k];
        s->on = k == 0u || chance_permille(&e->gen_rng, (uint16_t)(780 + density));
        s->accent = k == 0u || (k % 8u == 0u && chance_permille(&e->gen_rng, 600u));
        if (s->on) ++hits;
    }
    /* the offbeats: a walk into the next beat, an octave jump before a root, the lead's note, or nothing */
    for (k = 0u; k < len; ++k) {
        MelloBassStep *s = &b.steps[k];
        uint8_t next = (uint8_t)(((k / 4u + 1u) * 4u) % len), to = beat_tone[next], r;
        uint16_t p =
            k % 2u ? (uint16_t)(280 + density) : (uint16_t)(420 + density); /* the "and" more than the "e" */
        if (k % 4u == 0u) continue;
        if (!chance_permille(&e->gen_rng, p)) continue;
        r = (uint8_t)rng_bounded(&e->gen_rng, 100u);
        if (k % 4u == 3u && r < 45u) /* the step before a beat: a walk into it */
            s->tone = to == MELLO_SUB_TONE_FIFTH ? walk_to_fifth[rng_bounded(&e->gen_rng, 3u)]
                                                 : walk_to_root[rng_bounded(&e->gen_rng, 3u)];
        else if (k % 4u == 3u && r < 65u && to == MELLO_SUB_TONE_ROOT) /* an octave jump before the root */
            s->tone = MELLO_SUB_TONE_ROOT, s->octave = 1;
        else if (lead_chord[k] && r < 85u)
            s->tone = lead_deg[k] == 0u ? MELLO_SUB_TONE_ROOT : (uint8_t)lead_deg[k];
        else
            s->tone = beat_tone[(k / 4u) * 4u]; /* the beat's note again: a pulse */
        s->on = true;
        s->accent = chance_permille(&e->gen_rng, 120u);
        ++hits;
    }
    if (hits < 5u) /* never sporadic: fill the ands until five play */
        for (k = 2u; k < len && hits < 5u; k += 4u)
            if (!b.steps[k].on) b.steps[k].on = true, b.steps[k].tone = beat_tone[(k / 4u) * 4u], ++hits;
    for (k = 1u; k < len && hits > 11u; k += 2u) /* never a wall of sixteenths: the "e"s go first */
        if (b.steps[k].on) b.steps[k].on = false, --hits;
    for (k = 0u; k < len; ++k) /* glides where a note runs into the next and the two differ */
        if (b.steps[k].on && b.steps[(k + 1u) % len].on &&
            (b.steps[k].tone != b.steps[(k + 1u) % len].tone ||
             b.steps[k].octave != b.steps[(k + 1u) % len].octave) &&
            chance_permille(&e->gen_rng, 300u))
            b.steps[k].glide = true;
    b.on = true;
    mello_engine_bass_set_config(e, &b);
}

/* One BASS SEQ step, pulses long: its note on the sub (unless SUB MUTE), held into the next step if it glides, the
 * next one marked to slide; accents louder (127, else 114 as the sub's). The events carry ratchet_index 1 + the step
 * (mello_audio: a BASS SEQ note slides only when asked, and tags each step apart). */
static uint8_t resolve_sub_midi(const MelloEngine *e, uint8_t x_tone, int8_t x_octave);
static void emit_basic(MelloEngine *e, MelloEventType type, uint32_t tick, uint8_t step_idx,
                       uint8_t pulse_idx, uint8_t total_pulses, uint8_t note, uint8_t vel,
                       uint8_t ratchet_index, bool slide, bool pass);
static void bass_tick(MelloEngine *e, uint8_t pulses, uint8_t main_step)
{
    const MelloBassConfig *b = &e->bass;
    uint8_t st = (uint8_t)(e->bass_pos % b->length);
    const MelloBassStep *s = &b->steps[st];
    e->bass_step = (int8_t)st;
    e->bass_pos = (uint8_t)((st + 1u) % b->length);
    if ((s->on || e->perf.fill) && !e->perf.sub_mute) { /* FILL: every step of the line */
        uint32_t pt = mello_pulse_ticks(&e->project), len = pt * pulses, dur;
        uint8_t note = resolve_sub_midi(e, s->tone, s->octave);
        dur = s->glide
                  ? len + pt / 4u
                  : clamp_duration_ms_as_ticks(&e->project, len * e->sub.gate_amount / 1000u, 30u, 2500u);
        emit_basic(e, MELLO_EVENT_SUB_NOTE_ON, e->now_tick, main_step, 0u, pulses, note,
                   s->accent ? 127u : 114u, (uint8_t)(1u + st), e->bass_glide_in, true);
        emit_basic(e, MELLO_EVENT_SUB_NOTE_OFF, e->now_tick + dur, main_step, 0u, pulses, note, 0u,
                   (uint8_t)(1u + st), false, true);
        e->bass_glide_in = s->glide;
    } else
        e->bass_glide_in = false;
}

/* One drum step: its hits (each with CHANCE), RATCH spread over the step, rising; SWING moves them with the rest
 * (queue_push). */
static void drums_tick(MelloEngine *e, uint32_t pulse_ticks)
{
    MelloDrumConfig *d = &e->drums;
    uint8_t st, v, r;
    if (!d->on) return;
    st = (uint8_t)(e->drum_pos % d->length);
    e->drum_step = (int8_t)st;
    e->drum_pos = (uint8_t)((st + 1u) % d->length);
    for (v = 0u; v < MELLO_DRUM_VOICES; ++v) {
        const MelloDrumLock *l = &d->lock[v][st];
        uint8_t n = l->ratchet < 1u ? 1u : l->ratchet > 4u ? 4u : l->ratchet;
        bool roll = e->perf.fill && (v == MELLO_DRUM_SNARE || v == MELLO_DRUM_GLITCH); /* FILL: a roll on each step */
        if (!(d->hits[v] & (1u << st)) && !roll) continue;
        if (!roll && d->voice[v].chance < 1000u && !chance_permille(&e->drum_rng, d->voice[v].chance)) continue;
        if (roll && n < 2u) n = 2u;
        for (r = 0u; r < n; ++r) {
            MelloEvent ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = MELLO_EVENT_DRUM_HIT;
            ev.tick = e->now_tick + pulse_ticks * r / n;
            ev.step_index = st;
            ev.sample_slot = v;
            /* A roll rises (2 semitones a hit, the kick's excepted) and tightens, as drill'n'bass rolls do */
            ev.note = (uint8_t)(64 + l->pitch + (v != MELLO_DRUM_KICK ? 2 * (int32_t)r : 0));
            ev.velocity = ((d->accents[v] & (1u << st)) || (e->perf.accent && !r)) ? 127u /* ACCENT: every hit */
                          : r                          ? 72u
                                                       : 96u; /* a roll's later hits softer */
            ev.drum_decay = (int8_t)(l->decay - 15 * (int32_t)r < -100 ? -100 : l->decay - 15 * (int32_t)r);
            ev.drum_crush = l->crush;
            (void)queue_push(e, &ev);
        }
    }
}

#if MELLO_FEATURE_CHORD_MACHINE
/* FIT: a chord every eight sixteenths over the loop (the bass line's length, or the sequence's), each on the scale
 * degree heard at its start: the bass line's first note in its eight steps, or with the bass off the lead's first
 * note; nothing heard, the tonic. Pressed again over the same notes, the next variant: 0 triads on those degrees, 1
 * sevenths, 2 colour (sus2 on the tonic and the fourth, sus4 on the fifth), 3 the chord whose third the note is (two
 * degrees down, in first inversion, so the note stays the lowest). The bass's note is in the chord every time. */
/* FIT's reading: the scale degree heard at the start of each `hold` sixteenths over the loop (the bass line's
 * first note there, or the lead's with the bass off; nothing, the tonic), into deg; the number of chords. */
static uint8_t fit_read(const MelloEngine *e, uint8_t hold, uint8_t deg[8])
{
    const ScaleDef *sd = &SCALES[e->project.scale < MELLO_SCALE_COUNT ? e->project.scale : MELLO_SCALE_MINOR];
    uint8_t n = sd->n, len, slots, k, j;
    bool bass = e->bass.on && e->sub.on && e->bass.length;
    if (hold < 1u) hold = 1u;
    len = bass ? e->bass.length : (e->project.seq_len ? e->project.seq_len : 16u);
    slots = (uint8_t)(len / hold);
    if (slots < 1u) slots = 1u;
    if (slots > 8u) slots = 8u;
    for (k = 0u; k < slots; ++k) {
        int16_t d = -1;
        for (j = 0u; j < hold && d < 0; ++j) {
            uint8_t at = (uint8_t)(k * hold + j);
            if (bass) {
                const MelloBassStep *b = &e->bass.steps[at % e->bass.length];
                if (b->on) d = b->tone == MELLO_SUB_TONE_OCTAVE ? 0 : (int16_t)(b->tone % n);
            } else {
                const MelloStep *st = &e->project.steps[at % len];
                if (st->gate != MELLO_GATE_REST && !st->skip) d = (int16_t)((st->pitch < 0 ? 0 : st->pitch) % n);
            }
        }
        deg[k] = d < 0 ? 0u : (uint8_t)d;
    }
    return slots;
}

void mello_engine_chord_fit(MelloEngine *e)
{
    const ScaleDef *sd;
    MelloChordConfig c;
    uint8_t n, slots, k, deg[8], var;
    uint32_t key = 2166136261u;
    if (!e) return;
    sd = &SCALES[e->project.scale < MELLO_SCALE_COUNT ? e->project.scale : MELLO_SCALE_MINOR];
    n = sd->n;
    slots = fit_read(e, 8u, deg);
    for (k = 0u; k < slots; ++k) key = (key ^ (uint32_t)(deg[k] + 1u)) * 16777619u;
    key = (key ^ slots) * 16777619u;
    var = key == e->chord_fit_key ? (uint8_t)((e->chord_fit_var + 1u) % 4u) : 0u;
    e->chord_fit_key = key, e->chord_fit_var = var;
    mello_engine_undo_snapshot(e);
    c = e->chord;
    c.on = true;
    c.sub_root = false; /* the bass is the chords' root already: moved by them as well, it would go twice */
    c.chain = MELLO_CHORD_CHAIN_STEPS;
    c.length = slots;
    for (k = 0u; k < slots; ++k) {
        MelloChordStep *cs = &c.steps[k];
        uint8_t d = deg[k];
        cs->hold = 8u, cs->inversion = 0u, cs->spread = 1u, cs->octave = -1, cs->degree = d;
        cs->shape = MELLO_CHORD_TRIAD;
        if (var == 1u)
            cs->shape = MELLO_CHORD_SEVENTH;
        else if (var == 2u)
            cs->shape = d == 0u || d == 3u ? MELLO_CHORD_SUS2 : d == 4u ? MELLO_CHORD_SUS4 : MELLO_CHORD_TRIAD;
        else if (var == 3u)
            cs->degree = (uint8_t)((d + n - 2u) % n), cs->inversion = 1u;
    }
    mello_engine_chord_set_config(e, &c);
}

void mello_chord_defaults(MelloChordConfig *c)
{
    uint8_t i;
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->on = false;
    c->trigger = MELLO_CHORD_TRIGGER_STEP;
    c->mode = MELLO_CHORD_BROKEN; /* SUB MELLO: a broken chord, the delay making it a chord */
    c->poly = false;
    c->length = 1u; /* one chord, the tonic: it fits whatever stays in the key; FIT and RNDMZR make the rest (2026-11-04;
                     * was four, i VI iv v, whatever the bass played: DECISIONS "FIT") */
    c->voices = 3u;
    c->level = 130u; /* E4: was 320, then 180 (2026-11-04: 130, ~6 dB under the lead; DECISIONS "The mix") */
    c->wave = MELLO_WAVE_SAW;
    c->gate_amount = 720u;
    c->attack_ms = 40u;
    c->release_ms = 900u;
    c->cut = 550u; /* ~800 Hz, opened to ~1.5 kHz by the envelope at each chord (E4; was 600, ~1 kHz) */
    c->res = 180u;
    c->fenv = 220u; /* E4: was 380 */
    c->fenv_attack_ms = 8u;
    c->fenv_decay_ms = 700u;
    c->decay_ms = 900u;
    c->sustain = 750u;
    c->chorus = 600u;
    c->space = 600u; /* E4: was 1000, all of it to the delay and the reverb */
    c->strum_ms = 0u;
    c->link = false;
    c->voice_lead = false; /* the chords as they are written; VLEAD is a choice */
    c->lead_follow = 0u;   /* HARMONY off: nothing saved sounds different */
    c->drift = 0u;
    c->drift_depth = 3u;
    for (i = 0u; i < MELLO_CHORD_MAX_STEPS; ++i) {
        c->steps[i].degree = 0u;
        c->steps[i].shape = MELLO_CHORD_TRIAD;
        c->steps[i].inversion = 0u;
        c->steps[i].octave = -1;
        c->steps[i].spread = 1u;
        c->steps[i].hold = 8u; /* eight sixteenths on the clock chain: two chords a bar (2026-11-01, was 2 by
                                * triggers, the arpeggio's; DECISIONS "Harmony as the spine") */
    }
    c->chain = MELLO_CHORD_CHAIN_STEPS;
    c->grid_cells = 0x8888u;
    c->grid_mode = MELLO_DOT_HORIZONTAL;
    c->grid_length = 16u;
    c->grid_reset = true;
    c->grid_follow = false;
    c->grid_amount = 1000u;
    c->pattern_length = 8u;
    c->pattern_masks[0] = 0x1u;
    c->pattern_masks[1] = 0x2u;
    c->pattern_masks[2] = 0xFu;
    c->pattern_masks[3] = 0x4u;
    c->pattern_masks[4] = 0x9u;
    c->pattern_masks[5] = 0x2u;
    c->pattern_masks[6] = 0x6u;
    c->pattern_masks[7] = 0x8u;
}
#endif

void mello_randomize_defaults(MelloRandomizeConfig *c)
{
    if (!c) return;
    c->mask = MELLO_RAND_DEFAULT; /* feedback 2026-10-30: rolling the sound by default was too much */
}

void mello_sound_defaults(MelloSoundConfig *c)
{
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->osc_wave = MELLO_WAVE_SAW;
    c->pwm_permille = 500u;
    c->attack_ms = 5u;
    c->decay_ms = 180u;
    c->sustain_permille = 400u;
    c->release_ms = 120u;
    c->slide_ms = 50u;
    c->filter_cut = 700u;
    c->filter_res = 200u;
    c->lfo_on = false;
    c->lfo_rate_millihz = 2000u;
    c->lfo_depth = 350u;
    c->lfo_target = MELLO_LFO_FILTER;
    c->follower_sub_on = true;
    c->follower_sub_level = 220u;
    c->follower_sub_octave = -1;
    c->follower_sub_wave = MELLO_WAVE_SINE;
    c->master_level = 750u;
    c->lead_level = 1000u;
    c->noise_level = 0u;
    c->noise_fm = 0u;
    c->unison = 1u;
    c->detune = 0u;
    c->osc2_wave = MELLO_WAVE_OFF;
    c->osc2_semi = 0;
    c->osc2_fine = 7; /* mixed in, a little apart from OSC 1 */
    c->osc2_mix = 0u;
    c->osc2_sync = false;
    c->fm_amount = 0u;
    c->saturation = 0u;
    c->filter_mode = MELLO_FILTER_LP;
    c->menv_amount = 0;
    c->menv_attack_ms = 0u;
    c->menv_decay_ms = 300u;
    c->menv_target = MELLO_MENV_CUTOFF;
    c->vel_cutoff = 0;
    c->vel_level = 1000u;
    c->key_track = 0u;
    c->vel_menv = 0;
}

void mello_sample_defaults(MelloSampleConfig *c)
{
    uint8_t i;
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->on = false;
    c->per_step = true;
    c->edit_slot = 0u;
    for (i = 0u; i < MELLO_SAMPLE_SLOT_COUNT; ++i) {
        MelloSampleSlotConfig *s = &c->slots[i];
        s->coarse_semitones = 0;
        s->fine_cents = 0;
        s->track = true;
        s->loop = false;
        s->level = 700u;
        s->dist = 0u;
        s->trim_start = 0u;
        s->trim_end = 1000u;
        s->attack_ms = 5u;
        s->decay_ms = 120u;
        s->sustain_permille = 700u;
        s->release_ms = 150u;
    }
}

/* A division's time from a beat's (ms): the HTML's four first, then 16T .. 2N, then 32N, 16D, 2T, 2D, 1 BAR. */
uint32_t mello_delay_div_ms(uint8_t div, uint32_t qms)
{
    switch (div) {
    case 1: return qms / 4u;
    case 2: return qms;
    case 3: return qms / 3u;
    case 4: return qms / 6u;          /* 16T */
    case 5: return qms * 2u / 3u;     /* 4T */
    case 6: return qms * 3u / 4u;     /* 8D */
    case 7: return qms * 3u / 2u;     /* 4D */
    case 8: return qms * 2u;          /* 2N */
    case 9: return qms / 8u;          /* 32N */
    case 10: return qms * 3u / 8u;    /* 16D */
    case 11: return qms * 4u / 3u;    /* 2T */
    case 12: return qms * 3u;         /* 2D */
    case 13: return qms * 4u;         /* 1 BAR */
    default: return qms / 2u;         /* 8N */
    }
}
void mello_fx_defaults(MelloFxConfig *c)
{
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->dist_amount = 0u;
    c->dist_mix = 1000u;
    c->dist_character = 350u;
    c->dist_tone = 820u;
    c->dist_level = 900u;
    c->delay_wet = 120u;
    c->delay_sync = true;
    c->delay_div = 0u;
    c->delay_time_ms = 250u;
    c->delay_feedback = 320u;
    c->delay_tone = 720u;
    c->delay_lowcut = 390u; /* LO CUT ~120 Hz on a new project's echoes (E11/F10; it was OFF, and hard to find) */
    c->delay_level = 750u;
    c->reverb_wet = 240u;
    c->reverb_size = 700u;
    c->reverb_decay_ms = 6000u;
    c->reverb_pre_ms = 12u;
    c->reverb_tone = 620u;
    c->reverb_level = 1150u;
    c->send_dly[MELLO_SEND_LEAD] = c->send_rev[MELLO_SEND_LEAD] = 1000u;
    c->send_dly[MELLO_SEND_BASS] = 0u; /* a new project's bass stays out of the echoes, a little in the room */
    c->send_rev[MELLO_SEND_BASS] = 200u;
    c->send_dly[MELLO_SEND_CHORD] = c->send_rev[MELLO_SEND_CHORD] = 600u; /* the chords' SPACE before */
    c->send_dly[MELLO_SEND_DRUMS] = c->send_rev[MELLO_SEND_DRUMS] = 250u; /* and the drums' */
}

void mello_stored_project_encode(MelloStoredProject *d, const MelloProject *p)
{
    uint8_t i;
    if (!d || !p) return;
    memset(d, 0, sizeof(*d));
    d->seq_len = p->seq_len;
    d->bpm_lo = (uint8_t)(p->bpm & 0xFFu);
    d->bpm_hi = (uint8_t)(p->bpm >> 8);
    d->clock_div = p->clock_div;
    d->root_note = p->root_note;
    d->scale = (uint8_t)p->scale;
    d->global_octave = p->global_octave;
    d->pitch_span_oct = p->pitch_span_oct;
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        const MelloStep *s = &p->steps[i];
        MelloStoredStep *o = &d->steps[i];
        uint8_t pulses = clamp_1_8(s->pulses), ratchet = clamp_1_8(s->ratchet);
        int8_t oct = clamp_i8(s->octave, -2, 2);
        o->pitch = s->pitch;
        o->pulse_ratchet = (uint8_t)(((pulses - 1u) & 7u) | (((ratchet - 1u) & 7u) << 3));
        o->flags = (uint8_t)(((uint8_t)s->gate & 3u) | (((uint8_t)(oct + 2) & 7u) << 2) |
                             (s->slide ? 0x20u : 0u) | (s->skip ? 0x40u : 0u) | (s->accent ? 0x80u : 0u));
        o->extras = (uint8_t)((s->sub ? 1u : 0u) | ((s->sample_slot & 7u) << 1) | ((s->wave & 7u) << 4));
        o->probability_lo = (uint8_t)(s->probability & 0xFFu);
        o->probability_hi = (uint8_t)(s->probability >> 8);
    }
    d->seg_count = p->seg_count;
    for (i = 0u; i < MELLO_MAX_SEGMENTS - 1u; ++i) d->seg_splits[i] = p->seg_splits[i];
    for (i = 0u; i < MELLO_MAX_SEGMENTS; ++i) {
        d->seg_repeats[i] = p->seg_repeats[i];
        d->seg_direction[i] = (uint8_t)p->seg_direction[i];
        d->seg_mode[i] = (uint8_t)p->seg_mode[i];
        if (p->seg_skip[i]) d->seg_skip_mask |= (uint8_t)(1u << i);
    }
    d->master_len_lo = (uint8_t)(p->master_len & 0xFFu);
    d->master_len_hi = (uint8_t)(p->master_len >> 8);
}

void mello_stored_project_decode(MelloProject *p, const MelloStoredProject *d)
{
    uint8_t i;
    if (!p || !d) return;
    mello_project_defaults(p);
    p->seq_len = d->seq_len;
    p->bpm = (uint16_t)((uint16_t)d->bpm_lo | ((uint16_t)d->bpm_hi << 8));
    p->clock_div = d->clock_div;
    p->root_note = d->root_note;
    p->scale = (MelloScale)d->scale;
    p->global_octave = d->global_octave;
    p->pitch_span_oct = d->pitch_span_oct;
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        const MelloStoredStep *o = &d->steps[i];
        MelloStep *st = &p->steps[i];
        st->pitch = o->pitch;
        st->pulses = (uint8_t)((o->pulse_ratchet & 7u) + 1u);
        st->ratchet = (uint8_t)(((o->pulse_ratchet >> 3) & 7u) + 1u);
        st->gate = (MelloGateMode)(o->flags & 3u);
        st->octave = (int8_t)((int32_t)((o->flags >> 2) & 7u) - 2);
        st->slide = (o->flags & 0x20u) != 0u;
        st->skip = (o->flags & 0x40u) != 0u;
        st->accent = (o->flags & 0x80u) != 0u;
        st->sub = (o->extras & 1u) != 0u;
        st->sample_slot = (uint8_t)((o->extras >> 1) & 7u);
        st->wave = (uint8_t)((o->extras >> 4) & 7u);
        st->probability = (uint16_t)((uint16_t)o->probability_lo | ((uint16_t)o->probability_hi << 8));
    }
    p->seg_count = d->seg_count;
    for (i = 0u; i < MELLO_MAX_SEGMENTS - 1u; ++i) p->seg_splits[i] = d->seg_splits[i];
    for (i = 0u; i < MELLO_MAX_SEGMENTS; ++i) {
        p->seg_repeats[i] = d->seg_repeats[i];
        p->seg_direction[i] = (MelloSegmentDirection)d->seg_direction[i];
        p->seg_mode[i] = (MelloSegmentMode)d->seg_mode[i];
        p->seg_skip[i] = (d->seg_skip_mask & (uint8_t)(1u << i)) != 0u;
    }
    p->master_len = (uint16_t)((uint16_t)d->master_len_lo | ((uint16_t)d->master_len_hi << 8));
    sanitize_project(p);
}

/* A matrix's route: the cells in the order they are read, of the first len cells; returns its period. RANDOM is
 * HORIZONTAL here (dot_route shuffles it each round); WALK has no fixed route (dot_route steps it). */
static uint8_t dot_build_order(MelloDotMode mode, uint8_t len, uint8_t out[MELLO_DOT_ROUTE_MAX])
{
    static const uint8_t spiral[16] = {0, 1, 2, 3, 7, 11, 15, 14, 13, 12, 8, 4, 5, 6, 10, 9};
    static const uint8_t diagonal[16] = {0, 1, 4, 2, 5, 8, 3, 6, 9, 12, 7, 10, 13, 11, 14, 15};
    /* down a row each read, across 0 1 2 3 2 1: twelve reads, so it comes round against 16 steps */
    static const uint8_t bounce[12] = {0, 5, 10, 15, 2, 5, 8, 13, 2, 7, 10, 13};
    uint8_t n = 0u, r, c, i;
    if (len < 1u) len = 1u;
    if (len > MELLO_DOT_CELL_COUNT) len = MELLO_DOT_CELL_COUNT;
    if (mode == MELLO_DOT_VERTICAL) {
        for (c = 0u; c < 4u && n < len; ++c)
            for (r = 0u; r < 4u && n < len; ++r) out[n++] = (uint8_t)(r * 4u + c);
    } else if (mode == MELLO_DOT_SNAKE) {
        for (r = 0u; r < 4u && n < len; ++r) {
            if ((r & 1u) == 0u)
                for (c = 0u; c < 4u && n < len; ++c) out[n++] = (uint8_t)(r * 4u + c);
            else
                for (c = 4u; c > 0u && n < len; --c) out[n++] = (uint8_t)(r * 4u + (c - 1u));
        }
    } else if (mode == MELLO_DOT_SPIRAL || mode == MELLO_DOT_DIAGONAL) {
        for (n = 0u; n < len; ++n) out[n] = mode == MELLO_DOT_SPIRAL ? spiral[n] : diagonal[n];
    } else if (mode == MELLO_DOT_BOUNCE) { /* the ball's path, on the first len cells only */
        for (i = 0u; i < 12u; ++i)
            if (bounce[i] < len) out[n++] = bounce[i];
        if (!n) out[n++] = 0u;
    } else {
        for (n = 0u; n < len; ++n) out[n] = n;
        if (mode == MELLO_DOT_PINGPONG)
            for (i = (uint8_t)(len - 1u); i > 1u; --i) out[n++] = (uint8_t)(i - 1u);
    }
    for (i = n; i < MELLO_DOT_ROUTE_MAX; ++i) out[i] = 0u;
    return n;
}

/* The cell a matrix reads now: base counts its reads (RESET folds it to the sequence's length). */
static uint8_t dot_route(MelloEngine *e, MelloDotRuntime *rt, MelloDotMode mode, uint8_t len, bool reverse,
                         uint32_t base);

uint8_t mello_dot_route(MelloDotMode mode, uint8_t length, uint8_t out[MELLO_DOT_ROUTE_MAX])
{
    return dot_build_order(mode, length, out);
}

static void dot_shuffle_order(MelloEngine *e, uint8_t order[MELLO_DOT_ROUTE_MAX], uint8_t len)
{
    int32_t i;
    if (!e || len <= 1u) return;
    for (i = (int32_t)len - 1; i > 0; --i) {
        uint8_t j = (uint8_t)rng_bounded(&e->gen_rng, (uint32_t)i + 1u);
        uint8_t t = order[(uint8_t)i];
        order[(uint8_t)i] = order[j];
        order[j] = t;
    }
}

void mello_engine_dot_reset_runtime(MelloEngine *e)
{
    uint8_t i;
    if (!e) return;
    e->dot_tick = 0u;
    e->dot_follow = 0u;
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) {
        MelloDotMatrixConfig c = sanitize_dot_config(e->dot[i]);
        e->dot[i] = c;
        e->dot_runtime[i].mode = c.mode;
        e->dot_runtime[i].length = c.length;
        e->dot_runtime[i].cycle = -1;
        e->dot_runtime[i].walk = 0u;
        e->dot_runtime[i].period = dot_build_order(c.mode, c.length, e->dot_runtime[i].order);
        if (c.mode == MELLO_DOT_RANDOM) dot_shuffle_order(e, e->dot_runtime[i].order, c.length);
        e->dot_active_cell[i] = -1;
    }
}

static void tm_fill_new_registers(MelloEngine *e, uint8_t from, uint8_t to)
{
    uint8_t i;
    if (!e) return;
    if (to > MELLO_TM_MAX_STEPS) to = MELLO_TM_MAX_STEPS;
    for (i = from; i < to; ++i) {
        e->tm_bool_reg[i] = gen_unit16(&e->gen_rng) >= 32768u ? 1u : 0u;
        e->tm_float_reg[i] = gen_unit16(&e->gen_rng);
    }
}

void mello_engine_init(MelloEngine *e, const MelloProject *project, uint32_t seed)
{
    uint8_t i;
    if (!e) return;
    memset(e, 0, sizeof(*e));
    if (project)
        e->project = *project;
    else
        mello_project_defaults(&e->project);
    sanitize_project(&e->project);
    mello_loop_cycle_defaults(&e->loop_cycle);
    mello_slot_seq_defaults(&e->slot_seq);
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) mello_dot_defaults_for(&e->dot[i], i);
    mello_tm_defaults(&e->tm);
    mello_mod_routing_defaults(&e->modr);
    e->perf.repeat_len = 4u; /* a beat, until a white key says otherwise */
    mello_sub_defaults(&e->sub);
    mello_drum_defaults(&e->drums);
    mello_prng_seed(&e->drum_rng, seed ^ 0x44524D53u);
    e->drum_step = -1;
    mello_bass_defaults(&e->bass);
    e->bass_step = -1;
#if MELLO_FEATURE_CHORD_MACHINE
    mello_chord_defaults(&e->chord);
    e->chord_grid_runtime.cycle = -1;
    e->chord_grid_active_cell = -1;
    e->chord_arp_dir = 1;
#endif
    mello_randomize_defaults(&e->randomize);
    mello_sound_defaults(&e->sound);
    mello_sample_defaults(&e->sample);
    mello_fx_defaults(&e->fx);
    mello_macros_defaults(&e->macros);
    e->visible_slot_count = 8u;
    e->active_slot = -1;
    e->queued_slot = -1;
    e->tm_active_index = -1;
    e->sub_x_active_index = -1;
    mello_prng_seed(&e->rng, seed);
    mello_prng_seed(&e->gen_rng, seed ^ 0x9E3779B9u);
    e->tm_bool_len = MELLO_TM_MAX_STEPS;
    e->tm_float_len = MELLO_TM_MAX_STEPS;
    tm_fill_new_registers(e, 0u, MELLO_TM_MAX_STEPS);
    mello_engine_dot_reset_runtime(e);
    mello_engine_reset_transport(e);
    e->runtime_built = false;
}

static uint8_t build_bounds(const MelloProject *p, uint8_t bounds[MELLO_MAX_SEGMENTS + 1u])
{
    uint8_t n = p->seq_len;
    uint8_t sc = p->seg_count;
    uint8_t i;
    if (sc < 1u) sc = 1u;
    if (sc > MELLO_MAX_SEGMENTS) sc = MELLO_MAX_SEGMENTS;
    if (sc > n) sc = n;
    bounds[0] = 0u;
    for (i = 0u; i < sc - 1u; ++i) {
        uint8_t minv = (uint8_t)(bounds[i] + 1u);
        uint8_t remaining = (uint8_t)((sc - 1u) - i);
        uint8_t maxv = (uint8_t)(n - remaining);
        uint8_t v = p->seg_splits[i];
        if (v < minv) v = minv;
        if (v > maxv) v = maxv;
        bounds[i + 1u] = v;
    }
    bounds[sc] = n;
    return sc;
}

uint8_t mello_segment_bounds(const MelloProject *p, uint8_t bounds[MELLO_MAX_SEGMENTS + 1u])
{
    return build_bounds(p, bounds);
}

/* Writes bounds back as the splits (and sc as the count). */
static void segment_store(MelloProject *p, const uint8_t *bounds, uint8_t sc)
{
    uint8_t i;
    p->seg_count = sc;
    for (i = 0u; i + 1u < sc; ++i) p->seg_splits[i] = bounds[i + 1u];
}

/* Segment i's settings to segment j. */
static void segment_copy(MelloProject *p, uint8_t j, uint8_t i)
{
    p->seg_repeats[j] = p->seg_repeats[i];
    p->seg_direction[j] = p->seg_direction[i];
    p->seg_mode[j] = p->seg_mode[i];
    p->seg_skip[j] = p->seg_skip[i];
}

int8_t mello_segment_toggle_start(MelloProject *p, uint8_t step)
{
    uint8_t b[MELLO_MAX_SEGMENTS + 2u], sc, j, k;
    if (!p || step == 0u || step >= p->seq_len) return -1;
    sc = build_bounds(p, b);
    for (j = 1u; j < sc; ++j)
        if (b[j] == step) { /* segment j joins j - 1, which keeps its settings */
            for (k = j; k < sc; ++k) b[k] = b[k + 1u];
            for (k = j; k + 1u < sc; ++k) segment_copy(p, k, (uint8_t)(k + 1u));
            segment_store(p, b, (uint8_t)(sc - 1u));
            return (int8_t)(j - 1u);
        }
    if (sc >= MELLO_MAX_SEGMENTS) return -1;
    for (j = 0u; j + 1u < sc && b[j + 1u] <= step; ++j) {
    }
    for (k = (uint8_t)(sc + 1u); k > j + 1u; --k) b[k] = b[k - 1u]; /* segment j is cut at step */
    b[j + 1u] = step;
    for (k = sc; k > j + 1u; --k) segment_copy(p, k, (uint8_t)(k - 1u));
    segment_copy(p, (uint8_t)(j + 1u), j);
    segment_store(p, b, (uint8_t)(sc + 1u));
    return (int8_t)(j + 1u);
}

void mello_segment_even(MelloProject *p)
{
    uint8_t b[MELLO_MAX_SEGMENTS + 1u], sc, i;
    if (!p) return;
    sc = build_bounds(p, b);
    for (i = 1u; i < sc; ++i) b[i] = (uint8_t)(((uint32_t)i * p->seq_len + sc / 2u) / sc);
    segment_store(p, b, sc);
}

void mello_segment_set_length(MelloProject *p, uint8_t seg, uint8_t len)
{
    uint8_t b[MELLO_MAX_SEGMENTS + 1u], sc, n, k;
    int32_t d, end;
    if (!p) return;
    sc = build_bounds(p, b);
    n = b[sc];
    if (seg >= sc || sc < 2u) return;
    if (seg + 1u == sc) { /* the last: its start moves, the one before gives way */
        int32_t start = (int32_t)n - (int32_t)len;
        if (start < (int32_t)b[seg - 1u] + 1) start = (int32_t)b[seg - 1u] + 1;
        if (start > (int32_t)n - 1) start = (int32_t)n - 1;
        b[seg] = (uint8_t)start;
        segment_store(p, b, sc);
        return;
    }
    end = (int32_t)b[seg] + (int32_t)(len ? len : 1u);
    if (end > (int32_t)n - (int32_t)(sc - 1u - seg)) end = (int32_t)n - (int32_t)(sc - 1u - seg);
    d = end - (int32_t)b[seg + 1u];
    for (k = (uint8_t)(seg + 1u); k < sc; ++k) b[k] = (uint8_t)((int32_t)b[k] + d);
    for (k = (uint8_t)(sc - 1u); k > seg + 1u; --k) /* the last ones give way at the sequence's end */
        if (b[k] > b[k + 1u] - 1u) b[k] = (uint8_t)(b[k + 1u] - 1u);
    segment_store(p, b, sc);
}

void mello_segment_rescale(MelloProject *p, uint8_t old_len)
{
    uint8_t b[MELLO_MAX_SEGMENTS + 1u], sc, i, n;
    MelloProject q;
    if (!p || !old_len || old_len == p->seq_len) return;
    q = *p; /* the segments as they played at the old length */
    q.seq_len = old_len;
    sc = build_bounds(&q, b);
    n = p->seq_len;
    if (sc > n) sc = n;
    for (i = 1u; i < sc; ++i) {
        uint32_t v = ((uint32_t)b[i] * n + old_len / 2u) / old_len;
        if (v < (uint32_t)b[i - 1u] + 1u) v = (uint32_t)b[i - 1u] + 1u;
        if (v > (uint32_t)(n - (sc - i))) v = (uint32_t)(n - (sc - i));
        b[i] = (uint8_t)v;
    }
    segment_store(p, b, sc);
}

static uint16_t build_base_range(MelloEngine *e, uint8_t start, uint8_t end)
{
    uint16_t count = 0u;
    uint8_t si;
    for (si = start; si < end && si < e->project.seq_len; ++si) {
        const MelloStep *s = &e->project.steps[si];
        uint8_t pulses, pi;
        if (s->skip) continue;
        pulses = clamp_1_8(s->pulses);
        for (pi = 0u; pi < pulses; ++pi) {
            if (count >= MELLO_MAX_BASE_PULSES) return count;
            e->build_scratch[count].step_index = si;
            e->build_scratch[count].pulse_index = pi;
            e->build_scratch[count].total_pulses = pulses;
            count++;
        }
    }
    return count;
}

static bool runtime_append(MelloEngine *e, MelloPulseEntry pe)
{
    if (e->runtime_pulse_count >= MELLO_MAX_RUNTIME_PULSES) return false;
    e->runtime_pulses[e->runtime_pulse_count++] = pe;
    return true;
}

static bool build_segment_runtime(MelloEngine *e, uint8_t project_seg, uint8_t start, uint8_t end,
                                  MelloRuntimeSegment *out)
{
    uint16_t base_len, offset, i;
    MelloSegmentDirection dir;
    MelloSegmentMode mode;
    if (e->project.seg_skip[project_seg]) {
        out->offset = e->runtime_pulse_count;
        out->length = 0u;
        out->project_segment = project_seg;
        return true;
    }
    base_len = build_base_range(e, start, end);
    offset = e->runtime_pulse_count;
    dir = e->project.seg_direction[project_seg] == MELLO_SEG_REV ? MELLO_SEG_REV : MELLO_SEG_FWD;
    mode = e->project.seg_mode[project_seg];
    if (mode > MELLO_SEG_RANDOM) mode = MELLO_SEG_LINEAR;

    /* ordered = FWD base, or REV base. */
    for (i = 0u; i < base_len; ++i) {
        uint16_t src = (dir == MELLO_SEG_REV) ? (uint16_t)(base_len - 1u - i) : i;
        if (!runtime_append(e, e->build_scratch[src])) return false;
    }

    if (mode == MELLO_SEG_PINGPONG && base_len > 1u) {
        /* JS: ordered.concat(ordered.slice(1,-1).reverse()) */
        int32_t j;
        for (j = (int32_t)base_len - 2; j >= 1; --j) {
            if (!runtime_append(e, e->runtime_pulses[offset + (uint16_t)j])) return false;
        }
    } else if (mode == MELLO_SEG_RANDOM && base_len > 1u) {
        /* Fisher-Yates on the already direction-ordered segment list. */
        uint16_t len = (uint16_t)(e->runtime_pulse_count - offset);
        int32_t j;
        for (j = (int32_t)len - 1; j > 0; --j) {
            uint16_t k = (uint16_t)rng_bounded(&e->rng, (uint32_t)j + 1u);
            MelloPulseEntry tmp = e->runtime_pulses[offset + (uint16_t)j];
            e->runtime_pulses[offset + (uint16_t)j] = e->runtime_pulses[offset + k];
            e->runtime_pulses[offset + k] = tmp;
        }
    }

    out->offset = offset;
    out->length = (uint16_t)(e->runtime_pulse_count - offset);
    out->project_segment = project_seg;
    return true;
}

bool mello_engine_rebuild_sequence(MelloEngine *e)
{
    uint8_t bounds[MELLO_MAX_SEGMENTS + 1u];
    uint8_t sc, i;
    uint32_t natural = 0u;
    if (!e) return false;
    e->runtime_pulse_count = 0u;
    e->runtime_segment_count = 0u;
    sc = build_bounds(&e->project, bounds);
    for (i = 0u; i < sc; ++i) {
        MelloRuntimeSegment seg;
        if (!build_segment_runtime(e, i, bounds[i], bounds[i + 1u], &seg)) {
            e->runtime_built = false;
            e->has_active_step = false;
            return false;
        }
        if (seg.length > 0u) {
            if (e->runtime_segment_count >= MELLO_MAX_SEGMENTS) {
                e->runtime_built = false;
                e->has_active_step = false;
                return false;
            }
            e->runtime_segments[e->runtime_segment_count++] = seg;
            natural += (uint32_t)seg.length * (uint32_t)clamp_repeat(e->project.seg_repeats[i]);
        }
    }
    /* WP-D: master_len is a musical 1/16-note duration. Runtime sequence
     * pulses are scaled by clock_div so BAR duration does not change with DIV.
     * Canonical UI values divide exactly; ceil keeps malformed legacy/storage
     * values non-zero and safe without changing the six supported choices. */
    e->loop_pulse_length = e->project.master_len > 0u
                               ? ((uint32_t)e->project.master_len + (uint32_t)e->project.clock_div - 1u) /
                                     (uint32_t)e->project.clock_div
                               : natural;
    e->runtime_built = true;
    e->has_active_step = e->runtime_segment_count > 0u;
    return e->has_active_step;
}

void mello_engine_reset_transport(MelloEngine *e)
{
    if (!e) return;
    e->now_tick = 0u;
    e->loop_count = 0u;
    e->absolute_pulse = 0u;
    e->active_runtime_segment = 0u;
    e->segment_pulse_pos = 0u;
    e->segment_repeat_done = 0u;
    e->step_index = 0u;
    e->pulse_index = 0u;
    e->has_active_step = false;
    e->perf_hist_n = 0u; /* REVERSE / REPEAT start from this run's pulses */
    e->hold_open = false;
    e->drone_open = false;
    e->drum_pos = 0u;
    e->drum_step = -1;
    e->bass_pos = 0u;
    e->bass_step = -1;
    e->bass_glide_in = false;
    e->perf_buf_n = 0u;
    mello_event_clear(e);
}

void mello_engine_start(MelloEngine *e)
{
    if (!e || e->playing) return;
    /* Source buildSeq() runs on each fresh start, so RANDOM reshuffles here. */
    mello_engine_reset_transport(e);
    if (e->sub.x_reset) {
        e->sub_x_pos = 0u;
        e->sub_x_active_index = e->sub.x_on ? 0 : -1;
    }
#if MELLO_FEATURE_CHORD_MACHINE
    e->chord_grid_tick = 0u;
    e->chord_grid_follow = 0u;
    e->chord_grid_runtime.cycle = -1;
    e->chord_grid_active_cell = -1;
    e->chord_chain_pos = 0u;
    e->chord_chain_hold = 0u;
    e->chord_step_count = 0u;
    e->chord_arp_pos = 0u;
    e->chord_arp_dir = 1;
    e->chord_walk_note = 0u;
    e->chord_pattern_pos = 0u;
    e->chord_last_count = 0u;
    e->chord_voiced_n = 0u;
    e->chord_prev_n = 0u;
    e->chord_voiced_slot = 0xFFu;
#endif
    if (!mello_engine_rebuild_sequence(e)) {
        e->playing = true;
        return;
    }
    e->playing = true;
    {
        MelloRuntimeSegment *seg = &e->runtime_segments[0];
        MelloPulseEntry *pe = &e->runtime_pulses[seg->offset];
        e->step_index = pe->step_index;
        e->pulse_index = pe->pulse_index;
    }
}

void mello_engine_stop(MelloEngine *e)
{
    if (!e) return;
    e->playing = false;
    e->queued_slot = -1;
    e->perf.mix_mute ^= e->perf.mute_req; /* PERFORM's mutes asked for, with no beat to wait for now */
    e->perf.mute_req = 0u;
    /* Source STOP always clears Loop Cycle hold/count. Position returns to 0 only when ON;
     * OFF freezes the selected harmonic slot across STOP/START. Slot Seq runtime is untouched. */
    e->loop_cycle_tick = 0u;
    e->loop_cycle_count = 0u;
    if (e->loop_cycle.active) e->loop_cycle_pos = 0u;
    if (e->sub.x_reset) {
        e->sub_x_pos = 0u;
        e->sub_x_active_index = e->sub.x_on ? 0 : -1;
    }
    /* Next start rebuilds just like source buildSeq(). */
    e->runtime_built = false;
}

uint32_t mello_pulse_ticks(const MelloProject *p)
{
    uint8_t div = p ? p->clock_div : 1u;
    if (!(div == 1u || div == 2u || div == 4u)) div = 1u;
    return MELLO_SIXTEENTH_TICKS * div;
}

uint8_t mello_resolve_step_midi(const MelloProject *p, const MelloStep *step)
{
    const ScaleDef *sd;
    int32_t degree, oct, idx, semitone, midi;
    if (!p || !step) return 60u;
    sd = &SCALES[p->scale < MELLO_SCALE_COUNT ? p->scale : MELLO_SCALE_MINOR];
    degree = step->pitch;
    if (degree < 0) degree = 0;
    oct = degree / sd->n;
    idx = degree % sd->n;
    semitone = sd->v[idx] + oct * 12;
    midi = (int32_t)p->root_note + semitone + ((int32_t)p->global_octave + (int32_t)step->octave) * 12;
    return clamp_u8_i32(midi, 12u, 108u);
}

static void current_loop_context(const MelloEngine *e, int32_t *root_out, MelloScale *scale_out,
                                 int8_t *oct_out, int8_t *pitch_out, uint8_t *sub_tone_out,
                                 int8_t *sub_transpose_out)
{
    const MelloProject *p;
    const MelloLoopCycleStep *lc;
    uint8_t len, pos;
    int32_t root;
    MelloScale scale;
    if (!e) return;
    p = &e->project;
    len = e->loop_cycle.length;
    if (len < 2u) len = 2u;
    if (len > MELLO_MAX_LOOP_CYCLE_STEPS) len = MELLO_MAX_LOOP_CYCLE_STEPS;
    pos = e->loop_cycle.active ? (uint8_t)(e->loop_cycle_pos % len)
                               : (uint8_t)(e->loop_cycle_pos % MELLO_MAX_LOOP_CYCLE_STEPS);
    lc = &e->loop_cycle.steps[pos];
    root = (int32_t)p->root_note;
    if (lc->root_note >= 0 && lc->root_note <= 11)
        root = ((int32_t)p->root_note / 12) * 12 + (int32_t)lc->root_note;
    root += (int32_t)lc->root_offset;
    scale = (lc->scale_set >= 0 && lc->scale_set < (int8_t)MELLO_SCALE_COUNT) ? (MelloScale)lc->scale_set
                                                                              : p->scale;
    if (root_out) *root_out = root;
    if (scale_out) *scale_out = scale;
    if (oct_out) *oct_out = lc->octave;
    if (pitch_out) *pitch_out = lc->pitch;
    if (sub_tone_out) *sub_tone_out = mello_loop_cycle_sub_tone(lc);
    if (sub_transpose_out) *sub_transpose_out = mello_loop_cycle_sub_transpose(lc);
}

/* The key now (ROADMAP E1): the KEY page's root and scale, changed by the loop step only where its KEY or SCALE is not
 * FOLLOW, ROOT+ moving the tonic. Everything that plays or shows a key reads it. */
void mello_engine_key_now(const MelloEngine *e, uint8_t *root_out, MelloScale *scale_out)
{
    int32_t root = 60;
    MelloScale scale = MELLO_SCALE_MINOR;
    if (e) current_loop_context(e, &root, &scale, NULL, NULL, NULL, NULL);
    if (root_out) *root_out = clamp_u8_i32(root, 12u, 108u);
    if (scale_out) *scale_out = scale;
}

/* A note rounded into the key (ROADMAP E1): a transpose is in semitones, and what it lands on off the scale moves on
 * in the transpose's direction to the next tone of the scale (+1 in C minor: C to D; -1: D to C), so two notes never
 * collapse into one; dir 0 takes the nearest tone (down on a tie). CHROMATIC: as it is. */
static uint8_t key_round(MelloScale scale, int32_t root, int32_t midi, int32_t dir)
{
    const ScaleDef *sd = &SCALES[scale < MELLO_SCALE_COUNT ? scale : MELLO_SCALE_MINOR];
    int32_t m = midi;
    uint8_t k, i;
    if (sd->n >= 12u) return clamp_u8_i32(midi, 12u, 108u);
    if (!dir)
        return mello_scale_quantize(scale, (uint8_t)((root % 12 + 12) % 12 + 12),
                                    clamp_u8_i32(midi, 12u, 108u));
    for (k = 0u; k < 12u; ++k, m += dir > 0 ? 1 : -1) {
        int32_t rel = ((m - root) % 12 + 12) % 12;
        for (i = 0u; i < sd->n; ++i)
            if (sd->v[i] == rel) return clamp_u8_i32(m, 12u, 108u);
    }
    return clamp_u8_i32(midi, 12u, 108u);
}

#if MELLO_FEATURE_CHORD_MACHINE
/* HARMONY's FOLLOW: the note moved to the nearest tone of the chord sounding (any octave), the smaller way; ties go
 * down. Nothing moves when no chord has sounded yet. */
static uint8_t chord_snap_note(const MelloEngine *e, uint8_t note)
{
    int32_t best = note, best_d = 99;
    uint8_t i;
    if (!e->chord_voiced_n) return note;
    for (i = 0u; i < e->chord_voiced_n; ++i) {
        int32_t pc = (int32_t)e->chord_voiced[i] % 12, d = ((int32_t)note % 12 - pc + 12) % 12, cand;
        /* d semitones above the chord tone's class: down by d, or up by 12 - d */
        cand = d <= 6 ? (int32_t)note - d : (int32_t)note + (12 - d);
        if ((cand > best ? cand - best : best - cand) < 99 && (d <= 6 ? d : 12 - d) < best_d) {
            best_d = d <= 6 ? d : 12 - d;
            best = cand;
        }
    }
    return clamp_u8_i32(best, 12u, 108u);
}
#endif

/* HARMONY FOLLOW applied to a lead note at step `step_index`: OFF leaves it, BEATS snaps the notes on the beats (every
 * fourth step), ALL every note. */
static uint8_t lead_follow_note(const MelloEngine *e, uint8_t note, uint8_t step_index)
{
#if MELLO_FEATURE_CHORD_MACHINE
    if (e->chord.on && e->chord.lead_follow && (e->chord.lead_follow == 2u || step_index % 4u == 0u))
        return chord_snap_note(e, note);
#else
    (void)e;
    (void)step_index;
#endif
    return note;
}

static uint8_t resolve_engine_step_midi_mod(const MelloEngine *e, const MelloStep *step, int8_t extra_octave)
{
    const ScaleDef *sd;
    MelloScale scale = MELLO_SCALE_MINOR;
    int32_t root = 60, degree, oct, idx, semitone, midi;
    int8_t lc_oct = 0, lc_pitch = 0;
    if (!e || !step) return 60u;
    current_loop_context(e, &root, &scale, &lc_oct, &lc_pitch, NULL, NULL);
    sd = &SCALES[scale < MELLO_SCALE_COUNT ? scale : MELLO_SCALE_MINOR];
    degree = step->pitch;
    if (degree < 0) degree = 0;
    oct = degree / sd->n;
    idx = degree % sd->n;
    semitone = sd->v[idx] + oct * 12;
    midi = root + semitone +
           ((int32_t)e->project.global_octave + (int32_t)step->octave + (int32_t)lc_oct +
            (int32_t)extra_octave) *
               12 +
           (int32_t)lc_pitch;
    return lc_pitch ? key_round(scale, root, midi, lc_pitch) : clamp_u8_i32(midi, 12u, 108u);
}

static int8_t sub_tone_offset(MelloScale scale, uint8_t tone)
{
    const ScaleDef *sd = &SCALES[scale < MELLO_SCALE_COUNT ? scale : MELLO_SCALE_MINOR];
    uint8_t idx;
    if (tone == MELLO_SUB_TONE_OCTAVE) return 12;
    if (tone > MELLO_SUB_TONE_SEVENTH) return 0;
    if (tone == MELLO_SUB_TONE_ROOT) return 0;
    idx = tone;
    if (idx >= sd->n) idx = (uint8_t)(sd->n - 1u);
    return sd->v[idx];
}

#if MELLO_FEATURE_CHORD_MACHINE
static int32_t chord_root_shift(const MelloEngine *e, MelloScale scale);
#endif
static uint8_t resolve_sub_midi(const MelloEngine *e, uint8_t x_tone, int8_t x_octave)
{
    int32_t root = 60, midi;
    MelloScale scale = MELLO_SCALE_MINOR;
    int8_t lc_oct = 0, lc_pitch = 0, lc_sub_transpose = 0;
    uint8_t base_tone = 0u;
    current_loop_context(e, &root, &scale, &lc_oct, &lc_pitch, &base_tone, &lc_sub_transpose);
    midi =
        root + (int32_t)sub_tone_offset(scale, base_tone) + (int32_t)sub_tone_offset(scale, x_tone) +
#if MELLO_FEATURE_CHORD_MACHINE
        (e->chord.on && e->chord.sub_root ? chord_root_shift(e, scale) : 0) +
#endif
        /* the bass sounds an octave under its OCT (ROADMAP E9): OCT -1, the default, is two octaves under the root */
        ((int32_t)e->project.global_octave + (int32_t)lc_oct + (int32_t)e->sub.octave + (int32_t)x_octave - 1) *
            12 +
        (int32_t)lc_pitch + (int32_t)lc_sub_transpose;
    /* the tones' offsets are added as semitones and the chord's root shift too: the sum can leave the scale (a 6th
     * over a 3rd in C minor is B), so it is rounded into the key, in the transpose's direction when there is one */
    midi = key_round(scale, root, midi, (int32_t)lc_pitch + (int32_t)lc_sub_transpose);
    return clamp_u8_i32(midi, 12u, 96u);
}

static uint8_t current_project_segment(const MelloEngine *e)
{
    if (!e || e->runtime_segment_count == 0u || e->active_runtime_segment >= e->runtime_segment_count)
        return 0u;
    return e->runtime_segments[e->active_runtime_segment].project_segment;
}
uint8_t mello_engine_current_segment(const MelloEngine *e)
{
    return current_project_segment(e);
}

static void emit_basic(MelloEngine *e, MelloEventType type, uint32_t tick, uint8_t step_idx,
                       uint8_t pulse_idx, uint8_t total_pulses, uint8_t note, uint8_t vel,
                       uint8_t ratchet_index, bool slide, bool pass)
{
    uint8_t i;
    MelloEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.tick = tick;
    ev.type = type;
    ev.step_index = step_idx;
    ev.pulse_index = pulse_idx;
    ev.total_pulses = total_pulses;
    ev.segment_index = current_project_segment(e);
    ev.segment_repeat = e->segment_repeat_done;
    ev.note = note;
    ev.velocity = vel;
    ev.ratchet_index = ratchet_index;
    ev.slide = slide;
    if (type == MELLO_EVENT_NOTE_ON && step_idx < MELLO_MAX_STEPS) ev.wave = e->project.steps[step_idx].wave;
    ev.probability_pass = pass;
    ev.slot_index = e->active_slot;
    ev.loop_cycle_index = e->loop_cycle_pos;
    ev.slot_seq_index = e->slot_seq_pos;
    ev.voice = (type == MELLO_EVENT_SUB_NOTE_ON || type == MELLO_EVENT_SUB_NOTE_OFF) ? MELLO_VOICE_SUB
                                                                                     : MELLO_VOICE_LEAD;
    ev.effective_gate = e->last_mod.effective_gate;
    ev.effective_ratchet = e->last_mod.effective_ratchet;
    ev.effective_accent = e->last_mod.effective_accent;
    ev.tm_index = e->last_mod.tm_index;
    ev.tm_octave_mod = e->last_mod.tm_octave_mod;
    ev.tm_filter_mod_q15 = e->last_mod.tm_filter_mod_q15;
    ev.tm_decay_mod_q15 = e->last_mod.tm_decay_mod_q15;
    ev.dot_mod[0] = e->last_mod.dot_mod[0];
    ev.dot_mod[1] = e->last_mod.dot_mod[1];
    ev.tm_value =
        e->tm.on ? (int16_t)(e->last_mod.tm_float ? (int32_t)e->last_mod.tm_float - 32768 : -32767) : 0;
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) ev.dot_cell[i] = e->last_mod.dot_cell[i];
    (void)queue_push(e, &ev);
}

static void emit_note_pair(MelloEngine *e, uint32_t on_tick, uint32_t off_tick, uint8_t step_idx,
                           uint8_t pulse_idx, uint8_t total_pulses, uint8_t note, uint8_t vel,
                           uint8_t ratchet_index, bool slide, bool pass)
{
    emit_basic(e, MELLO_EVENT_NOTE_ON, on_tick, step_idx, pulse_idx, total_pulses, note, vel, ratchet_index,
               slide, pass);
    emit_basic(e, MELLO_EVENT_NOTE_OFF, off_tick, step_idx, pulse_idx, total_pulses, note, 0u, ratchet_index,
               slide, pass);
}

static void emit_sub_pair(MelloEngine *e, uint32_t on_tick, uint32_t off_tick, uint8_t step_idx,
                          uint8_t pulse_idx, uint8_t total_pulses, uint8_t note, bool pass)
{
    emit_basic(e, MELLO_EVENT_SUB_NOTE_ON, on_tick, step_idx, pulse_idx, total_pulses, note, 114u, 0u, false,
               pass);
    emit_basic(e, MELLO_EVENT_SUB_NOTE_OFF, off_tick, step_idx, pulse_idx, total_pulses, note, 0u, 0u, false,
               pass);
}

static void emit_sample_pair(MelloEngine *e, uint32_t on_tick, uint32_t off_tick, uint8_t step_idx,
                             uint8_t total_pulses, uint8_t note, uint8_t vel, uint8_t sample_slot, bool slide,
                             bool pass)
{
    MelloEvent ev;
    uint8_t i;
    memset(&ev, 0, sizeof ev);
    ev.tick = on_tick;
    ev.type = MELLO_EVENT_SAMPLE_NOTE_ON;
    ev.step_index = step_idx;
    ev.pulse_index = 0u;
    ev.total_pulses = total_pulses;
    ev.segment_index = current_project_segment(e);
    ev.segment_repeat = e->segment_repeat_done;
    ev.note = note;
    ev.velocity = vel;
    ev.slide = slide;
    ev.probability_pass = pass;
    ev.slot_index = e->active_slot;
    ev.loop_cycle_index = e->loop_cycle_pos;
    ev.slot_seq_index = e->slot_seq_pos;
    ev.voice = MELLO_VOICE_LEAD;
    ev.sample_slot = sample_slot;
    ev.effective_gate = e->last_mod.effective_gate;
    ev.effective_ratchet = e->last_mod.effective_ratchet;
    ev.effective_accent = e->last_mod.effective_accent;
    ev.tm_index = e->last_mod.tm_index;
    ev.tm_octave_mod = e->last_mod.tm_octave_mod;
    ev.tm_filter_mod_q15 = e->last_mod.tm_filter_mod_q15;
    ev.tm_decay_mod_q15 = e->last_mod.tm_decay_mod_q15;
    ev.dot_mod[0] = e->last_mod.dot_mod[0];
    ev.dot_mod[1] = e->last_mod.dot_mod[1];
    ev.tm_value =
        e->tm.on ? (int16_t)(e->last_mod.tm_float ? (int32_t)e->last_mod.tm_float - 32768 : -32767) : 0;
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) ev.dot_cell[i] = e->last_mod.dot_cell[i];
    (void)queue_push(e, &ev);
    ev.tick = off_tick;
    ev.type = MELLO_EVENT_SAMPLE_NOTE_OFF;
    ev.velocity = 0u;
    (void)queue_push(e, &ev);
}

static uint8_t velocity_for_accent(bool accent)
{
    return accent ? 121u : 91u;
}

/* EVOLVE LENGTH: a duration times the pulse's length (1/1024), without a 64-bit product. */
static uint32_t tm_length_scale(const MelloEngine *e, uint32_t ticks)
{
    uint32_t f = e->last_mod.tm_length;
    return (ticks >> 10) * f + (((ticks & 1023u) * f) >> 10);
}

static void generate_gate_events(MelloEngine *e, const MelloStep *s, bool pass, uint8_t pulse_idx,
                                 uint8_t total, MelloGateMode gate, uint8_t ratchets, bool accent,
                                 int8_t octave_mod)
{
    uint32_t pulse_ticks = mello_pulse_ticks(&e->project);
    uint8_t note = lead_follow_note(e, resolve_engine_step_midi_mod(e, s, octave_mod), e->step_index);
    uint8_t vel = velocity_for_accent(accent);
    uint32_t t = e->now_tick;
    ratchets = clamp_1_8(ratchets);
    if (gate == MELLO_GATE_REST) return;
    if (gate == MELLO_GATE_HOLD && e->perf_mode) {
        /* REVERSE / REPEAT replay pulses out of order, so a HOLD's ON (its first pulse) and OFF (its last) may not
         * both come, or not in that order: while they play, a HOLD step is a note of one pulse (as their stutter),
         * which also never overlaps the next replay. */
        if (pulse_idx == 0u)
            emit_note_pair(e, t, t + (pulse_ticks * 22u) / 25u, e->step_index, 0u, total, note, vel, 0u,
                           s->slide, pass);
        return;
    }
    if (gate == MELLO_GATE_HOLD) {
        if (pulse_idx == 0u) {
            emit_basic(e, MELLO_EVENT_NOTE_ON, t, e->step_index, pulse_idx, total, note, vel, 0u, s->slide,
                       pass);
            e->hold_open = true, e->hold_note = note, e->hold_step = e->step_index, e->hold_total = total;
        }
        if (pulse_idx == (uint8_t)(total - 1u)) {
            /* The OFF of the ON at pulse 0, so tagged with that pulse: mello_audio releases a note only on its own
             * OFF (event_tag), and an OFF tagged with the last pulse left the note on for ever. */
            uint32_t off = t + (pulse_ticks * 22u) / 25u;
            emit_basic(e, MELLO_EVENT_NOTE_OFF, off, e->step_index, 0u, total, note, 0u, 0u, s->slide, pass);
            e->hold_open = false;
        }
        return;
    }
    if (gate == MELLO_GATE_MULTI || (gate == MELLO_GATE_SINGLE && ratchets > 1u)) {
        if (ratchets <= 1u) {
            uint32_t dur = tm_length_scale(e, (pulse_ticks * 2u) / 5u);
            emit_note_pair(e, t, t + dur, e->step_index, pulse_idx, total, note, vel, 0u, s->slide, pass);
        } else {
            uint32_t sub = pulse_ticks / ratchets;
            uint32_t raw_dur = tm_length_scale(e, (sub * 18u) / 25u);
            uint32_t dur = clamp_duration_ms_as_ticks(&e->project, raw_dur, 10u, 250u);
            uint8_t r;
            for (r = 0u; r < ratchets; ++r) {
                uint32_t rt = t + sub * r;
                emit_note_pair(e, rt, rt + dur, e->step_index, pulse_idx, total, note, vel, r, s->slide,
                               pass);
            }
        }
        return;
    }
    if (pulse_idx == 0u) {
        uint32_t total_ticks = pulse_ticks * total;
        uint32_t raw_dur = tm_length_scale(e, (total_ticks * 41u) / 50u);
        uint32_t dur = clamp_duration_ms_as_ticks(&e->project, raw_dur, 20u, 4000u);
        emit_note_pair(e, t, t + dur, e->step_index, pulse_idx, total, note, vel, 0u, s->slide, pass);
    }
}

static void set_current_entry(MelloEngine *e)
{
    MelloRuntimeSegment *seg;
    MelloPulseEntry *pe;
    if (!e || e->runtime_segment_count == 0u) {
        if (e) e->has_active_step = false;
        return;
    }
    if (e->active_runtime_segment >= e->runtime_segment_count) e->active_runtime_segment = 0u;
    seg = &e->runtime_segments[e->active_runtime_segment];
    if (seg->length == 0u) {
        e->has_active_step = false;
        return;
    }
    if (e->segment_pulse_pos >= seg->length) e->segment_pulse_pos = 0u;
    pe = &e->runtime_pulses[seg->offset + e->segment_pulse_pos];
    e->step_index = pe->step_index;
    e->pulse_index = pe->pulse_index;
    e->has_active_step = true;
}

static void advance_segment_traversal(MelloEngine *e)
{
    MelloRuntimeSegment *seg = &e->runtime_segments[e->active_runtime_segment];
    uint8_t project_seg = seg->project_segment;
    uint8_t needed = clamp_repeat(e->project.seg_repeats[project_seg]);
    e->segment_pulse_pos++;
    if (e->segment_pulse_pos >= seg->length) {
        e->segment_pulse_pos = 0u;
        e->segment_repeat_done++;
        if (e->segment_repeat_done >= needed) {
            e->segment_repeat_done = 0u;
            e->active_runtime_segment =
                (uint8_t)((e->active_runtime_segment + 1u) % e->runtime_segment_count);
        }
    }
}

static void emit_state_event(MelloEngine *e, MelloEventType type, uint32_t tick)
{
    MelloEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.tick = tick;
    ev.type = type;
    ev.step_index = e->step_index;
    ev.pulse_index = e->pulse_index;
    ev.segment_index = current_project_segment(e);
    ev.slot_index = e->active_slot;
    ev.loop_cycle_index = e->loop_cycle_pos;
    ev.slot_seq_index = e->slot_seq_pos;
    (void)queue_push(e, &ev);
}

void mello_engine_loop_cycle_set_active(MelloEngine *e, bool active)
{
    if (!e) return;
    e->loop_cycle.active = active;
    if (active && e->loop_cycle.length && e->loop_cycle_pos >= e->loop_cycle.length)
        e->loop_cycle_pos = (uint8_t)(e->loop_cycle_pos % e->loop_cycle.length);
}
void mello_engine_loop_cycle_set_sync(MelloEngine *e, bool sync)
{
    if (e) e->loop_cycle.sync = sync;
}

void mello_engine_loop_cycle_set_length(MelloEngine *e, uint8_t length)
{
    if (!e) return;
    if (length < 2u) length = 2u;
    if (length > MELLO_MAX_LOOP_CYCLE_STEPS) length = MELLO_MAX_LOOP_CYCLE_STEPS;
    e->loop_cycle.length = length;
    e->loop_cycle_tick = 0u;
    if (e->loop_cycle_pos >= length) e->loop_cycle_pos = (uint8_t)(e->loop_cycle_pos % length);
}

void mello_engine_loop_cycle_set_step(MelloEngine *e, uint8_t index, const MelloLoopCycleStep *step)
{
    if (!e || !step || index >= MELLO_MAX_LOOP_CYCLE_STEPS) return;
    e->loop_cycle.steps[index] = sanitize_loop_step(*step);
    e->loop_cycle_tick = 0u;
}

void mello_engine_loop_cycle_select(MelloEngine *e, uint8_t index)
{
    uint8_t len;
    if (!e) return;
    len = e->loop_cycle.length;
    if (len < 2u) len = 2u;
    if (len > MELLO_MAX_LOOP_CYCLE_STEPS) len = MELLO_MAX_LOOP_CYCLE_STEPS;
    if (e->loop_cycle.active) {
        if (index >= len) index = (uint8_t)(index % len);
    } else if (index >= MELLO_MAX_LOOP_CYCLE_STEPS)
        index = (uint8_t)(index % MELLO_MAX_LOOP_CYCLE_STEPS);
    e->loop_cycle_pos = index;
    e->loop_cycle_tick = 0u;
}

void mello_engine_loop_cycle_reset_position(MelloEngine *e)
{
    if (!e) return;
    e->loop_cycle_pos = 0u;
    e->loop_cycle_tick = 0u;
    e->loop_cycle_count = 0u;
}

static void copy_name(char dst[MELLO_SLOT_NAME_CHARS + 1u], const char *src)
{
    uint8_t i = 0u;
    if (!dst) return;
    if (src) {
        while (i < MELLO_SLOT_NAME_CHARS && src[i]) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
    while (++i <= MELLO_SLOT_NAME_CHARS) dst[i] = '\0';
}

static void rebuild_preserving_phase(MelloEngine *e)
{
    uint8_t active = e->active_runtime_segment;
    uint16_t pos = e->segment_pulse_pos;
    uint8_t rep = e->segment_repeat_done;
    e->runtime_built = false;
    if (!mello_engine_rebuild_sequence(e)) return;
    if (e->runtime_segment_count) {
        e->active_runtime_segment = (uint8_t)(active % e->runtime_segment_count);
        if (e->runtime_segments[e->active_runtime_segment].length)
            e->segment_pulse_pos = (uint16_t)(pos % e->runtime_segments[e->active_runtime_segment].length);
        else
            e->segment_pulse_pos = 0u;
        e->segment_repeat_done = rep;
        set_current_entry(e);
    }
}

void mello_engine_clear_pattern(MelloEngine *e)
{
    MelloProject defaults;
    MelloLoopCycleConfig loop_defaults;
    bool loop_active, loop_sync;
    uint8_t loop_length, i;
    if (!e) return;

    /* Source CLEAR resets only the 32 main steps plus the eight Loop Cycle
     * step records/position. Global sequence, segment and generator settings stay. */
    mello_project_defaults(&defaults);
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) e->project.steps[i] = defaults.steps[i];

    loop_active = e->loop_cycle.active;
    loop_sync = e->loop_cycle.sync;
    loop_length = e->loop_cycle.length;
    mello_loop_cycle_defaults(&loop_defaults);
    for (i = 0u; i < MELLO_MAX_LOOP_CYCLE_STEPS; ++i) e->loop_cycle.steps[i] = loop_defaults.steps[i];
    e->loop_cycle.active = loop_active;
    e->loop_cycle.sync = loop_sync;
    e->loop_cycle.length = loop_length;
    e->loop_cycle_pos = 0u;
    e->loop_cycle_tick = 0u;
    e->loop_cycle_count = 0u;

    if (e->playing)
        rebuild_preserving_phase(e);
    else
        e->runtime_built = false;
}

static bool apply_slot(MelloEngine *e, uint8_t index, bool from_slot_seq, uint32_t event_tick)
{
    MelloProject next;
    uint16_t bpm;
    MelloLoopCycleConfig preserved_loop;
    uint8_t lp;
    uint16_t lt;
    uint32_t lc;
    uint8_t i;
    if (!e || index >= MELLO_MAX_SLOTS || !e->slots[index].valid) return false;
    bpm = e->project.bpm;
    mello_stored_project_decode(&next, &e->slots[index].project);
    next.bpm = bpm;
    sanitize_project(&next);
    preserved_loop = e->loop_cycle;
    lp = e->loop_cycle_pos;
    lt = e->loop_cycle_tick;
    lc = e->loop_cycle_count;
    e->project = next;

    /* Source snapshots switch all generative config with a slot; Slot Seq preserves only Loop Cycle. */
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) e->dot[i] = sanitize_dot_config(e->slots[index].dot[i]);
    e->tm = sanitize_tm_config(e->slots[index].tm);
    e->modr = sanitize_mod_routing(e->slots[index].modr);
    {
        uint8_t saved_len = e->slots[index].tm_bool_len;
        if (saved_len > MELLO_TM_MAX_STEPS) saved_len = MELLO_TM_MAX_STEPS;
        for (i = 0u; i < saved_len; ++i) e->tm_bool_reg[i] = e->slots[index].tm_bool_reg[i] ? 1u : 0u;
        for (i = saved_len; i < MELLO_TM_MAX_STEPS; ++i)
            e->tm_bool_reg[i] = gen_unit16(&e->gen_rng) >= 32768u ? 1u : 0u;
        e->tm_bool_len = MELLO_TM_MAX_STEPS; /* loadSnapshot uses tmEnsureLength(..., MAX_STEPS). */
    }
    if (e->tm_float_len < e->tm.length) {
        for (i = e->tm_float_len; i < e->tm.length; ++i) e->tm_float_reg[i] = gen_unit16(&e->gen_rng);
        e->tm_float_len = e->tm.length;
    }
    e->sub = sanitize_sub_config(e->slots[index].sub);
    e->randomize = e->slots[index].randomize;
    e->sound = sanitize_sound_config(e->slots[index].sound);
    e->sample = sanitize_sample_config(e->slots[index].sample);
    e->randomize.mask &= MELLO_RAND_ALL;
    mello_engine_dot_reset_runtime(e); /* dotMatrixCfg setEffect resets runtime and counters. */
    if (e->sub.x_reset) {
        e->sub_x_pos = 0u;
        e->sub_x_active_index = e->sub.x_on ? 0 : -1;
    }

    if (from_slot_seq) {
        e->loop_cycle = preserved_loop;
        sanitize_loop_config(&e->loop_cycle);
        e->loop_cycle_pos = lp;
        e->loop_cycle_tick = lt;
        e->loop_cycle_count = lc;
        rebuild_preserving_phase(e);
    } else {
        e->loop_cycle = e->slots[index].loop_cycle;
        sanitize_loop_config(&e->loop_cycle);
        e->loop_cycle_pos = 0u;
        e->loop_cycle_tick = 0u;
        e->loop_cycle_count = 0u;
        e->absolute_pulse = 0u;
        e->active_runtime_segment = 0u;
        e->segment_pulse_pos = 0u;
        e->segment_repeat_done = 0u;
        e->runtime_built = false;
        (void)mello_engine_rebuild_sequence(e);
        set_current_entry(e);
    }
    e->active_slot = (int8_t)index;
    emit_state_event(e, MELLO_EVENT_SLOT_CHANGE, event_tick);
    return true;
}

void mello_engine_capture_working(const MelloEngine *e, MelloPatternSlot *out)
{
    uint8_t i;
    if (!e || !out) return;
    memset(out, 0, sizeof(*out));
    out->valid = true;
    mello_stored_project_encode(&out->project, &e->project);
    out->loop_cycle = e->loop_cycle;
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) out->dot[i] = e->dot[i];
    out->tm = e->tm;
    out->modr = e->modr;
    out->tm_bool_len = e->tm_bool_len > MELLO_TM_MAX_STEPS ? MELLO_TM_MAX_STEPS : e->tm_bool_len;
    for (i = 0u; i < MELLO_TM_MAX_STEPS; ++i)
        out->tm_bool_reg[i] = (i < out->tm_bool_len && e->tm_bool_reg[i]) ? 1u : 0u;
    out->sub = e->sub;
    out->randomize = e->randomize;
    out->sound = e->sound;
    out->sample = e->sample;
}

bool mello_engine_restore_working(MelloEngine *e, const MelloPatternSlot *snapshot)
{
    MelloProject p;
    uint8_t i, saved_len;
    if (!e || !snapshot) return false;
    mello_stored_project_decode(&p, &snapshot->project);
    e->project = p;
    e->loop_cycle = snapshot->loop_cycle;
    sanitize_loop_config(&e->loop_cycle);
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) e->dot[i] = sanitize_dot_config(snapshot->dot[i]);
    e->tm = sanitize_tm_config(snapshot->tm);
    e->modr = sanitize_mod_routing(snapshot->modr);
    saved_len = snapshot->tm_bool_len > MELLO_TM_MAX_STEPS ? MELLO_TM_MAX_STEPS : snapshot->tm_bool_len;
    for (i = 0u; i < saved_len; ++i) e->tm_bool_reg[i] = snapshot->tm_bool_reg[i] ? 1u : 0u;
    for (i = saved_len; i < MELLO_TM_MAX_STEPS; ++i)
        e->tm_bool_reg[i] = gen_unit16(&e->gen_rng) >= 32768u ? 1u : 0u;
    e->tm_bool_len = MELLO_TM_MAX_STEPS;
    e->sub = sanitize_sub_config(snapshot->sub);
    e->randomize = snapshot->randomize;
    e->randomize.mask &= MELLO_RAND_ALL;
    e->sound = sanitize_sound_config(snapshot->sound);
    e->sample = sanitize_sample_config(snapshot->sample); /* (FX: its own section, SEC_FX) */
    e->playing = false;
    e->has_active_step = false;
    e->step_index = 0u;
    e->pulse_index = 0u;
    e->absolute_pulse = 0u;
    e->loop_count = 0u;
    e->loop_cycle_pos = 0u;
    e->loop_cycle_tick = 0u;
    e->loop_cycle_count = 0u;
    e->active_slot = -1;
    e->queued_slot = -1;
    e->slot_seq_pos = 0u;
    e->slot_seq_hold = 0u;
    e->sub_x_pos = 0u;
    e->sub_x_active_index = e->sub.x_on ? 0 : -1;
    mello_engine_dot_reset_runtime(e);
    e->runtime_built = false;
    e->active_runtime_segment = 0u;
    e->segment_pulse_pos = 0u;
    e->segment_repeat_done = 0u;
    mello_event_clear(e);
    return mello_engine_rebuild_sequence(e);
}

bool mello_engine_import_slot(MelloEngine *e, uint8_t index, const MelloPatternSlot *slot)
{
    MelloPatternSlot clean;
    MelloProject p;
    uint8_t i, n;
    if (!e || !slot || index >= MELLO_MAX_SLOTS) return false;
    memset(&clean, 0, sizeof(clean));
    clean.valid = slot->valid;
    copy_name(clean.name, slot->name);
    mello_stored_project_decode(&p, &slot->project);
    mello_stored_project_encode(&clean.project, &p);
    clean.loop_cycle = slot->loop_cycle;
    sanitize_loop_config(&clean.loop_cycle);
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) clean.dot[i] = sanitize_dot_config(slot->dot[i]);
    clean.tm = sanitize_tm_config(slot->tm);
    clean.modr = sanitize_mod_routing(slot->modr);
    n = slot->tm_bool_len > MELLO_TM_MAX_STEPS ? MELLO_TM_MAX_STEPS : slot->tm_bool_len;
    clean.tm_bool_len = n;
    for (i = 0u; i < n; ++i) clean.tm_bool_reg[i] = slot->tm_bool_reg[i] ? 1u : 0u;
    clean.sub = sanitize_sub_config(slot->sub);
    clean.randomize = slot->randomize;
    clean.randomize.mask &= MELLO_RAND_ALL;
    clean.sound = sanitize_sound_config(slot->sound);
    clean.sample = sanitize_sample_config(slot->sample);
    e->slots[index] = clean;
    return true;
}

void mello_engine_clear_slots(MelloEngine *e)
{
    if (!e) return;
    memset(e->slots, 0, sizeof(e->slots));
    e->active_slot = -1;
    e->queued_slot = -1;
}

bool mello_engine_save_slot(MelloEngine *e, uint8_t index, const char *name)
{
    uint8_t i;
    if (!e || index >= MELLO_MAX_SLOTS) return false;
    e->slots[index].valid = true;
    mello_stored_project_encode(&e->slots[index].project, &e->project);
    e->slots[index].loop_cycle = e->loop_cycle;
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) e->slots[index].dot[i] = e->dot[i];
    e->slots[index].tm = e->tm;
    e->slots[index].modr = e->modr;
    e->slots[index].tm_bool_len = e->tm_bool_len;
    for (i = 0u; i < MELLO_TM_MAX_STEPS; ++i)
        e->slots[index].tm_bool_reg[i] = (i < e->tm_bool_len && e->tm_bool_reg[i]) ? 1u : 0u;
    e->slots[index].sub = e->sub;
    e->slots[index].randomize = e->randomize;
    e->slots[index].sound = e->sound;
    e->slots[index].sample = e->sample;
    copy_name(e->slots[index].name, name);
    e->active_slot = (int8_t)index;
    return true;
}

bool mello_engine_request_slot(MelloEngine *e, uint8_t index)
{
    if (!e || index >= MELLO_MAX_SLOTS || !e->slots[index].valid) return false;
    if (e->playing) {
        e->queued_slot = (e->queued_slot == (int8_t)index) ? -1 : (int8_t)index;
        return true;
    }
    e->queued_slot = -1;
    return apply_slot(e, index, false, e->now_tick);
}

bool mello_engine_slot_valid(const MelloEngine *e, uint8_t index)
{
    return e && index < MELLO_MAX_SLOTS && e->slots[index].valid;
}

void mello_engine_set_visible_slot_count(MelloEngine *e, uint8_t count)
{
    uint8_t i;
    if (!e) return;
    e->visible_slot_count = sanitize_visible_slot_count(count);
    if (e->active_slot >= (int8_t)e->visible_slot_count) e->active_slot = -1;
    if (e->queued_slot >= (int8_t)e->visible_slot_count) e->queued_slot = -1;
    for (i = 0u; i < MELLO_MAX_SLOT_SEQ_STEPS; ++i)
        if (e->slot_seq.steps[i].slot >= (int8_t)e->visible_slot_count) e->slot_seq.steps[i].slot = -1;
}

void mello_engine_slot_seq_set_active(MelloEngine *e, bool active)
{
    if (e) e->slot_seq.active = active;
}

void mello_engine_slot_seq_set_length(MelloEngine *e, uint8_t length)
{
    if (!e) return;
    e->slot_seq.length = sanitize_slot_seq_length(length);
    if (e->slot_seq_pos >= e->slot_seq.length) mello_engine_slot_seq_reset_position(e);
}

void mello_engine_slot_seq_set_step(MelloEngine *e, uint8_t index, int8_t slot, uint16_t hold)
{
    if (!e || index >= MELLO_MAX_SLOT_SEQ_STEPS) return;
    if (slot < -1 || slot >= (int8_t)MELLO_MAX_SLOTS) slot = -1;
    if (slot >= (int8_t)e->visible_slot_count) slot = -1;
    e->slot_seq.steps[index].slot = slot;
    e->slot_seq.steps[index].hold = clamp_hold(hold);
}

void mello_engine_slot_seq_reset_position(MelloEngine *e)
{
    if (!e) return;
    e->slot_seq_pos = 0u;
    e->slot_seq_hold = 0u;
}

void mello_engine_dot_set_config(MelloEngine *e, MelloDotTarget target, const MelloDotMatrixConfig *config)
{
    if (!e || !config || target >= MELLO_DOT_MATRIX_COUNT) return;
    e->dot[target] = sanitize_dot_config(*config);
    /* React useEffect([dotMatrixCfg]) rebuilds every route runtime and zeros both counters. */
    mello_engine_dot_reset_runtime(e);
}

void mello_engine_dot_set_cells(MelloEngine *e, MelloDotTarget target, uint16_t cells)
{
    if (!e || target >= MELLO_DOT_MATRIX_COUNT) return;
    e->dot[target].cells = cells;
}

void mello_engine_dot_randomize_cells(MelloEngine *e, MelloDotTarget target)
{
    uint8_t i;
    uint16_t cells = 0u;
    if (!e || target >= MELLO_DOT_MATRIX_COUNT) return;
    for (i = 0u; i < MELLO_DOT_CELL_COUNT; ++i)
        if (chance_permille(&e->gen_rng, 350u)) cells |= (uint16_t)(1u << i);
    e->dot[target].cells = cells;
}

static void tm_ensure_lengths(MelloEngine *e, uint8_t len)
{
    uint8_t i;
    if (!e) return;
    len = sanitize_tm_length(len);
    if (e->tm_bool_len < len) {
        for (i = e->tm_bool_len; i < len; ++i)
            e->tm_bool_reg[i] = gen_unit16(&e->gen_rng) >= 32768u ? 1u : 0u;
        e->tm_bool_len = len;
    }
    if (e->tm_float_len < len) {
        for (i = e->tm_float_len; i < len; ++i) e->tm_float_reg[i] = gen_unit16(&e->gen_rng);
        e->tm_float_len = len;
    }
}

/* REVERSE: the history newest first (but the pulse just heard); REPEAT: its last n pulses, oldest first. */
static void perf_fill_buffer(MelloEngine *e, uint8_t mode, uint8_t n)
{
    uint8_t i, have = e->perf_hist_n;
    e->perf_buf_n = 0u;
    e->perf_buf_pos = 0u;
    if (mode == 1u) {
        for (i = 1u; i < have; ++i) {
            uint8_t k = (uint8_t)((e->perf_hist_w + MELLO_PERF_HISTORY - 1u - i) % MELLO_PERF_HISTORY);
            e->perf_parts_buf[e->perf_buf_n] = e->perf_parts_hist[k];
            e->perf_buf[e->perf_buf_n++] = e->perf_hist[k];
        }
    } else {
        if (n > have) n = have;
        for (i = 0u; i < n; ++i) {
            uint8_t k = (uint8_t)((e->perf_hist_w + MELLO_PERF_HISTORY - n + i) % MELLO_PERF_HISTORY);
            e->perf_parts_buf[e->perf_buf_n] = e->perf_parts_hist[k];
            e->perf_buf[e->perf_buf_n++] = e->perf_hist[k];
        }
    }
}

void mello_engine_set_perf(MelloEngine *e, const MelloPerf *p)
{
    uint8_t mode, len;
    if (!e || !p) return;
    len = p->repeat_len < 1u ? 1u : (p->repeat_len > 16u ? 16u : p->repeat_len);
    if (p->fill == e->perf.fill && p->sub_mute == e->perf.sub_mute && p->reverse == e->perf.reverse &&
        p->repeat == e->perf.repeat && len == e->perf.repeat_len && p->arp == e->perf.arp &&
        p->accent == e->perf.accent && p->fx == e->perf.fx)
        return; /* nothing changed (it is called on every input) */
    e->perf = *p;
    e->perf.repeat_len = len;
    mode = p->repeat ? 2u : (p->reverse ? 1u : 0u); /* REPEAT wins while both are held */
    if (mode != e->perf_mode || (mode == 2u && len != e->perf_mode_len)) {
        e->perf_mode = mode;
        e->perf_mode_len = len;
        if (mode) perf_fill_buffer(e, mode, len);
    }
}

void mello_engine_perf_restart(MelloEngine *e)
{
    if (e) e->perf_restart = true;
}

void mello_engine_mod_routing_set(MelloEngine *e, const MelloModRouting *config)
{
    if (e && config) e->modr = sanitize_mod_routing(*config);
}

void mello_engine_tm_set_config(MelloEngine *e, const MelloTmConfig *config)
{
    if (!e || !config) return;
    e->tm = sanitize_tm_config(*config);
    tm_ensure_lengths(e, e->tm.length);
}

void mello_engine_tm_set_length(MelloEngine *e, uint8_t length)
{
    if (!e) return;
    e->tm.length = sanitize_tm_length(length);
    tm_ensure_lengths(e, e->tm.length);
    if (e->tm_active_index >= (int8_t)e->tm.length) e->tm_active_index = -1;
}

void mello_engine_tm_reroll(MelloEngine *e)
{
    uint8_t i, len;
    if (!e) return;
    len = sanitize_tm_length(e->tm.length);
    for (i = 0u; i < len; ++i) {
        e->tm_bool_reg[i] = gen_unit16(&e->gen_rng) >= 32768u ? 1u : 0u;
        e->tm_float_reg[i] = gen_unit16(&e->gen_rng);
    }
    e->tm_bool_len = len;
    e->tm_float_len = len;
}

void mello_engine_tm_set_bool_register(MelloEngine *e, const uint8_t *values, uint8_t count)
{
    uint8_t i;
    if (!e || !values) return;
    if (count > MELLO_TM_MAX_STEPS) count = MELLO_TM_MAX_STEPS;
    for (i = 0u; i < count; ++i) e->tm_bool_reg[i] = values[i] ? 1u : 0u;
    e->tm_bool_len = count;
    tm_ensure_lengths(e, e->tm.length);
}

void mello_engine_sub_set_config(MelloEngine *e, const MelloSubConfig *config)
{
    if (!e || !config) return;
    e->sub = sanitize_sub_config(*config);
    if (!e->sub.x_on) e->sub_x_active_index = -1;
}

void mello_engine_sub_reset_x(MelloEngine *e)
{
    if (!e) return;
    e->sub_x_pos = 0u;
    e->sub_x_active_index = e->sub.x_on ? 0 : -1;
}

#if MELLO_FEATURE_CHORD_MACHINE
void mello_engine_chord_set_config(MelloEngine *e, const MelloChordConfig *config)
{
    if (!e || !config) return;
    e->chord = sanitize_chord_config(*config);
    if (e->chord_chain_pos >= e->chord.length) e->chord_chain_pos = 0u;
}
void mello_engine_chord_reset(MelloEngine *e)
{
    if (!e) return;
    e->chord_grid_tick = 0u;
    e->chord_grid_follow = 0u;
    e->chord_grid_runtime.cycle = -1;
    e->chord_grid_active_cell = -1;
    e->chord_chain_pos = 0u;
    e->chord_chain_hold = 0u;
    e->chord_step_count = 0u;
    e->chord_arp_pos = 0u;
    e->chord_arp_dir = 1;
    e->chord_walk_note = 0u;
    e->chord_pattern_pos = 0u;
    e->chord_last_count = 0u;
    e->chord_voiced_n = 0u;
    e->chord_prev_n = 0u;
    e->chord_voiced_slot = 0xFFu;
}
#endif

void mello_engine_sound_set_config(MelloEngine *e, const MelloSoundConfig *c)
{
    if (e && c) e->sound = sanitize_sound_config(*c);
}
void mello_engine_sample_set_config(MelloEngine *e, const MelloSampleConfig *c)
{
    if (e && c) e->sample = sanitize_sample_config(*c);
}
void mello_engine_fx_set_config(MelloEngine *e, const MelloFxConfig *c)
{
    if (e && c) e->fx = sanitize_fx_config(*c);
}

void mello_macros_defaults(MelloMacroConfig *c)
{
    if (!c) return;
    c->id[0] = MELLO_MACRO_CUTOFF;
    c->id[1] = MELLO_MACRO_DECAY;
    c->id[2] = MELLO_MACRO_SPACE;
    c->id[3] = MELLO_MACRO_DRIVE;
}

void mello_engine_macros_set(MelloEngine *e, const MelloMacroConfig *c)
{
    uint8_t k;
    if (!e || !c) return;
    for (k = 0u; k < 4u; ++k) e->macros.id[k] = c->id[k] < MELLO_MACRO_COUNT ? c->id[k] : (uint8_t)k;
}

void mello_engine_set_randomize_mask(MelloEngine *e, uint16_t mask)
{
    if (e) e->randomize.mask = (uint16_t)(mask & MELLO_RAND_ALL);
}

static uint16_t prob_from_hundredths(uint8_t hundredths);

/* The randomizer's links, after a roll: each linked target, if rolled, follows the step's pitch (0 the lowest the
 * scale and span allow, 1000 the highest) or mirrors it. Numbers are set by it (pulses and ratchets 1-4, chance
 * 30-100%), flags drawn with it as their chance (accent / sub up to 70%, slide 60%, skip 40%), so the roll stays a
 * roll but leans with the melody. */
/* Which targets a roll changes on each step: the mask, each target kept with its amount's chance (EDIT). A target at
 * 100% draws nothing, so rolls at the defaults are what they were. Bit 15 (no target) carries WAVE / CHORD's draw. */
#if MELLO_FEATURE_CHORD_MACHINE
/* A progression with a function (DECISIONS.md "Progressions with a function", feedback: "the most common pop
 * progression ever, a bit boring"): three to eight chords walked through tonic, subdominant and dominant functions
 * on the scale's own degrees (so a dorian IV or a phrygian II colours it), a deceptive cadence now and then, an open
 * ending on the dominant one time in four, a tonic substitute instead of the tonic itself, no degree twice in a row;
 * sevenths, sus chords and inversions on top, more of them the higher colour_min; the holds either even or
 * long-short; HOLD 8 and the clock chain (NEXT STEPS) when the chain still stands on its default 2, so a chord lands
 * on the one whatever the lead rests. Shared by MED, HIGH and MUSICAL (2026-11-01); the pattern masks are the
 * caller's. */
static void roll_progression(MelloEngine *e, uint8_t n, uint16_t colour_min)
{
    static const uint8_t T_deg[3] = {0u, 5u, 2u}, S_deg[3] = {3u, 1u, 5u}, D_deg[3] = {4u, 6u, 1u};
    static const uint8_t lens[7] = {3u, 4u, 4u, 5u, 6u, 8u, 8u}; /* eight twice: four bars at HOLD 8 */
    uint8_t len = lens[rng_bounded(&e->gen_rng, 7u)], hold = e->chord.steps[0].hold, fn = 0u /* T */,
            last = 255u, k;
    uint16_t colour =
        (uint16_t)(colour_min + rng_bounded(&e->gen_rng, 1000u - colour_min)); /* beyond triads */
    bool long_short = chance_permille(&e->gen_rng, 400u);
    if (!n) n = 7u;
    if (hold <= 2u) {
        hold = 8u;
        e->chord.chain = MELLO_CHORD_CHAIN_STEPS;
    }
    e->chord.length = len;
    for (k = 0u; k < len; ++k) {
        MelloChordStep *cs = &e->chord.steps[k];
        uint8_t deg;
        uint32_t r = rng_bounded(&e->gen_rng, 100u);
        if (k == 0u)
            fn = chance_permille(&e->gen_rng, 850u) ? 0u : (chance_permille(&e->gen_rng, 500u) ? 1u : 2u);
        else if (k + 1u == len)
            fn = chance_permille(&e->gen_rng, 800u) ? 0u : 2u; /* home, or left open on the dominant */
        else if (fn == 0u)
            fn = r < 50u ? 1u : (r < 75u ? 2u : 0u);
        else if (fn == 1u)
            fn = r < 55u ? 2u : (r < 80u ? 0u : 1u);
        else
            fn = r < 70u ? 0u
                         : (r < 85u ? 1u : 2u); /* the dominant resolves, or deceives to the subdominant */
        {
            const uint8_t *pool = fn == 0u ? T_deg : (fn == 1u ? S_deg : D_deg);
            uint8_t pick = (uint8_t)(rng_bounded(&e->gen_rng, 100u) <
                                             (fn == 0u && (k == 0u || k + 1u == len) ? 90u : 55u)
                                         ? 0u
                                         : 1u + rng_bounded(&e->gen_rng, 2u));
            deg = pool[pick];
            if (deg == last) deg = pool[(pick + 1u) % 3u]; /* never the same chord twice running */
            if (fn == 0u && k + 1u == len && last != 0u && chance_permille(&e->gen_rng, 900u))
                deg = 0u; /* home is home */
            if (deg == last && len > 1u) deg = (uint8_t)((deg + 3u) % 7u);
        }
        cs->degree = (uint8_t)(deg % n);
        cs->shape = rng_bounded(&e->gen_rng, 1000u) < colour
                        ? (chance_permille(&e->gen_rng, 600u)
                               ? MELLO_CHORD_SEVENTH
                               : (chance_permille(&e->gen_rng, 500u) ? MELLO_CHORD_SUS2 : MELLO_CHORD_SUS4))
                        : MELLO_CHORD_TRIAD;
        cs->inversion = (uint8_t)(colour > 300u ? rng_bounded(&e->gen_rng, 3u) : 0u);
        cs->spread = (uint8_t)(colour > 600u ? rng_bounded(&e->gen_rng, 3u) : cs->spread);
        cs->octave = (int8_t)(k && chance_permille(&e->gen_rng, 150u) ? -1 : 0);
        cs->hold = (uint8_t)(long_short ? (k % 2u ? (hold + 1u) / 2u : hold + hold / 2u) : hold);
        last = deg;
    }
    e->chord = sanitize_chord_config(e->chord);
}
#endif

static void randomize_step_masks(MelloEngine *e, uint16_t mask, uint16_t out[MELLO_MAX_STEPS])
{
    uint8_t i, b;
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        uint16_t m = mask;
        for (b = 0u; b < MELLO_RAND_TARGETS; ++b)
            if ((m & (1u << b)) && e->modr.rnd_amount[b] < 100u &&
                !chance_permille(&e->gen_rng, (uint16_t)(e->modr.rnd_amount[b] * 10u)))
                m &= (uint16_t)~(1u << b);
        out[i] = m;
    }
}

static void randomize_links(MelloEngine *e, uint8_t pmax, const uint16_t smask[MELLO_MAX_STEPS])
{
    uint16_t links =
        (uint16_t)((e->modr.rnd_follow | e->modr.rnd_mirror) & e->randomize.mask & MELLO_RAND_LINKABLE);
    uint8_t i;
    if (!links) return;
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        MelloStep *st = &e->project.steps[i];
        uint16_t links_here = links;
        links = (uint16_t)(links_here & smask[i]); /* only where the roll reached */
        uint32_t up = pmax ? (uint32_t)(st->pitch < 0 ? 0 : st->pitch) * 1000u / pmax : 500u, p;
        if (up > 1000u) up = 1000u;
#define LINKED(bit) ((links & (bit)) ? ((e->modr.rnd_mirror & (bit)) ? 1000u - up : up) : 0u)
        if (links & MELLO_RAND_PULSES)
            p = LINKED(MELLO_RAND_PULSES), st->pulses = (uint8_t)(1u + (p * 3u + 500u) / 1000u);
        if (links & MELLO_RAND_RATCHET)
            p = LINKED(MELLO_RAND_RATCHET), st->ratchet = (uint8_t)(1u + (p * 3u + 500u) / 1000u);
        if (links & MELLO_RAND_PROB)
            p = LINKED(MELLO_RAND_PROB),
            st->probability = prob_from_hundredths((uint8_t)(30u + p * 70u / 1000u));
        if (links & MELLO_RAND_ACCENT)
            st->accent = chance_permille(&e->gen_rng, (uint16_t)(LINKED(MELLO_RAND_ACCENT) * 7u / 10u));
        if (links & MELLO_RAND_SUB)
            st->sub = chance_permille(&e->gen_rng, (uint16_t)(LINKED(MELLO_RAND_SUB) * 7u / 10u));
        if (links & MELLO_RAND_SLIDE)
            st->slide = chance_permille(&e->gen_rng, (uint16_t)(LINKED(MELLO_RAND_SLIDE) * 6u / 10u));
        if (links & MELLO_RAND_SKIP)
            st->skip = chance_permille(&e->gen_rng, (uint16_t)(LINKED(MELLO_RAND_SKIP) * 4u / 10u));
#undef LINKED
        links = links_here;
    }
}

static uint16_t prob_from_hundredths(uint8_t hundredths)
{
    uint32_t n = (uint32_t)hundredths * 65535u + 50u;
    return (uint16_t)(n / 100u);
}

/* RANDOM's WAVE (feedback 2026-10-30: "WAVE on in the randomizer should give a wave PER STEP, not OFF") rolls each
 * step's own wave (MelloStep.wave, 1..5), not the sound's; a step at 0 keeps playing the sound's WAVE. MED's: mostly
 * saw and square, as the sound's was rolled. */
static uint8_t roll_wave_med(MelloEngine *e)
{
    uint32_t wr = rng_bounded(&e->gen_rng, 1000u);
    return (uint8_t)(1u + (wr < 350u   ? MELLO_WAVE_SAW
                           : wr < 600u ? MELLO_WAVE_SQUARE
                           : wr < 800u ? MELLO_WAVE_TRIANGLE
                           : wr < 920u ? MELLO_WAVE_SINE
                                       : MELLO_WAVE_PULSE));
}

void mello_engine_randomize_steps(MelloEngine *e)
{
    uint8_t i;
    uint16_t mask, smask[MELLO_MAX_STEPS];
    const ScaleDef *sd;
    uint8_t pmax;
    static const int8_t oct_choices[5] = {0, 0, 0, 1, -1};
    if (!e) return;
    mask = (uint16_t)(e->randomize.mask & MELLO_RAND_ALL);
    sd = &SCALES[e->project.scale < MELLO_SCALE_COUNT ? e->project.scale : MELLO_SCALE_MINOR];
    pmax = (uint8_t)(sd->n * e->project.pitch_span_oct - 1u);
    randomize_step_masks(e, mask, smask);
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        MelloStep *st = &e->project.steps[i];
        if (smask[i] & MELLO_RAND_PITCH) st->pitch = (int8_t)rng_bounded(&e->gen_rng, (uint32_t)pmax + 1u);
        if (smask[i] & MELLO_RAND_PULSES)
            st->pulses =
                chance_permille(&e->gen_rng, 600u) ? 1u : (uint8_t)(rng_bounded(&e->gen_rng, 3u) + 1u);
        if (smask[i] & MELLO_RAND_RATCHET) {
            if (chance_permille(&e->gen_rng, 780u))
                st->ratchet = 1u;
            else if (chance_permille(&e->gen_rng, 600u))
                st->ratchet = 2u;
            else
                st->ratchet = (uint8_t)(rng_bounded(&e->gen_rng, 4u) + 2u);
        }
        if (smask[i] & MELLO_RAND_GATE) {
            if (chance_permille(&e->gen_rng, 100u))
                st->gate = MELLO_GATE_REST;
            else if (chance_permille(&e->gen_rng, 200u))
                st->gate = MELLO_GATE_HOLD;
            else
                st->gate = MELLO_GATE_SINGLE;
        }
        if (smask[i] & MELLO_RAND_OCTAVE) st->octave = oct_choices[rng_bounded(&e->gen_rng, 5u)];
        if (smask[i] & MELLO_RAND_ACCENT) st->accent = chance_permille(&e->gen_rng, 120u);
        if (smask[i] & MELLO_RAND_SUB) st->sub = chance_permille(&e->gen_rng, 220u);
        if (smask[i] & MELLO_RAND_SLIDE) st->slide = chance_permille(&e->gen_rng, 80u);
        if (smask[i] & MELLO_RAND_SKIP) st->skip = chance_permille(&e->gen_rng, 50u);
        if (smask[i] & MELLO_RAND_PROB) {
            if (chance_permille(&e->gen_rng, 900u))
                st->probability = 65535u;
            else {
                uint32_t u = gen_unit16(&e->gen_rng);
                uint8_t h = (uint8_t)(40u + (uint8_t)((u * 50u + 32767u) / 65535u));
                st->probability = prob_from_hundredths(h);
            }
        }
        if (smask[i] & MELLO_RAND_SAMPLE)
            st->sample_slot = chance_permille(&e->gen_rng, 150u)
                                  ? 0u
                                  : (uint8_t)(rng_bounded(&e->gen_rng, MELLO_SAMPLE_SLOT_COUNT) + 1u);
        if (smask[i] & MELLO_RAND_WAVE) st->wave = roll_wave_med(e);
    }
#if MELLO_FEATURE_CHORD_MACHINE
    if ((mask & MELLO_RAND_CHORD) && e->modr.rnd_amount[11] < 100u &&
        !chance_permille(&e->gen_rng, (uint16_t)(e->modr.rnd_amount[11] * 10u)))
        mask &= (uint16_t)~MELLO_RAND_CHORD;
#endif
#if MELLO_FEATURE_CHORD_MACHINE
    if (mask & MELLO_RAND_CHORD) {
        roll_progression(e, sd->n, 0u); /* a progression, not four dice (2026-11-01) */
        for (i = 0u; i < e->chord.pattern_length; ++i) {
            uint8_t pm = (uint8_t)(1u << rng_bounded(&e->gen_rng, e->chord.voices));
            if (chance_permille(&e->gen_rng, 380u))
                pm |= (uint8_t)(1u << rng_bounded(&e->gen_rng, e->chord.voices));
            if (chance_permille(&e->gen_rng, 180u)) pm = (uint8_t)((1u << e->chord.voices) - 1u);
            e->chord.pattern_masks[i] = (uint8_t)(pm & 0x0Fu);
        }
        e->chord = sanitize_chord_config(e->chord);
    }
#endif
    randomize_links(e, pmax, smask);
    sanitize_project(&e->project);
    if (e->playing)
        rebuild_preserving_phase(e);
    else
        e->runtime_built = false;
}

static int32_t random_delta(MelloPrng *rng, int32_t radius)
{
    uint32_t span = (uint32_t)(radius * 2 + 1);
    return (int32_t)rng_bounded(rng, span) - radius;
}

static uint8_t clamp_u8_random(int32_t v, uint8_t lo, uint8_t hi)
{
    if (v < (int32_t)lo) return lo;
    if (v > (int32_t)hi) return hi;
    return (uint8_t)v;
}

static uint16_t clamp_u16_random(int32_t v, uint16_t lo, uint16_t hi)
{
    if (v < (int32_t)lo) return lo;
    if (v > (int32_t)hi) return hi;
    return (uint16_t)v;
}

static void randomize_steps_low_high(MelloEngine *e, MelloRandomizeIntensity intensity)
{
    uint8_t i;
    uint16_t mask, smask[MELLO_MAX_STEPS];
    const ScaleDef *sd;
    uint8_t pmax;
    bool high = intensity == MELLO_RANDOMIZE_HIGH;
    if (!e) return;
    mask = (uint16_t)(e->randomize.mask & MELLO_RAND_ALL);
    sd = &SCALES[e->project.scale < MELLO_SCALE_COUNT ? e->project.scale : MELLO_SCALE_MINOR];
    pmax = (uint8_t)(sd->n * e->project.pitch_span_oct - 1u);
    randomize_step_masks(e, mask, smask);
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        MelloStep *st = &e->project.steps[i];
        if (smask[i] & MELLO_RAND_PITCH) {
            if (high) {
                uint8_t v = (uint8_t)rng_bounded(&e->gen_rng, (uint32_t)pmax + 1u);
                if (pmax && v == (uint8_t)st->pitch)
                    v = (uint8_t)((v + 1u + rng_bounded(&e->gen_rng, pmax)) % ((uint32_t)pmax + 1u));
                st->pitch = (int8_t)v;
            } else
                st->pitch =
                    (int8_t)clamp_u8_random((int32_t)st->pitch + random_delta(&e->gen_rng, 1), 0u, pmax);
        }
        if (smask[i] & MELLO_RAND_PULSES) {
            if (high)
                st->pulses = (uint8_t)(1u + rng_bounded(&e->gen_rng, 8u));
            else
                st->pulses = clamp_u8_random((int32_t)st->pulses + random_delta(&e->gen_rng, 1), 1u, 8u);
        }
        if (smask[i] & MELLO_RAND_RATCHET) {
            if (high) {
                uint32_t r = rng_bounded(&e->gen_rng, 1000u);
                st->ratchet = r < 320u ? 1u : (uint8_t)(2u + rng_bounded(&e->gen_rng, 7u));
            } else
                st->ratchet = clamp_u8_random((int32_t)st->ratchet + random_delta(&e->gen_rng, 1), 1u, 8u);
        }
        if (smask[i] & MELLO_RAND_GATE) {
            if (high) {
                uint32_t r = rng_bounded(&e->gen_rng, 1000u);
                st->gate = r < 120u ? MELLO_GATE_REST
                                    : (r < 300u ? MELLO_GATE_HOLD
                                                : (r < 460u ? MELLO_GATE_MULTI : MELLO_GATE_SINGLE));
            } else if (chance_permille(&e->gen_rng, 180u)) {
                uint32_t r = rng_bounded(&e->gen_rng, 3u);
                st->gate = r == 0u ? MELLO_GATE_SINGLE : (r == 1u ? MELLO_GATE_HOLD : MELLO_GATE_REST);
            }
        }
        if (smask[i] & MELLO_RAND_OCTAVE) {
            if (high)
                st->octave = (int8_t)((int32_t)rng_bounded(&e->gen_rng, 5u) - 2);
            else
                st->octave = clamp_i8((int32_t)st->octave + random_delta(&e->gen_rng, 1), -2, 2);
        }
        if (smask[i] & MELLO_RAND_ACCENT)
            st->accent = high ? chance_permille(&e->gen_rng, 500u)
                              : (chance_permille(&e->gen_rng, 160u) ? !st->accent : st->accent);
        if (smask[i] & MELLO_RAND_SUB)
            st->sub = high ? chance_permille(&e->gen_rng, 500u)
                           : (chance_permille(&e->gen_rng, 150u) ? !st->sub : st->sub);
        if (smask[i] & MELLO_RAND_SLIDE)
            st->slide = high ? chance_permille(&e->gen_rng, 420u)
                             : (chance_permille(&e->gen_rng, 120u) ? !st->slide : st->slide);
        if (smask[i] & MELLO_RAND_SKIP)
            st->skip = high ? chance_permille(&e->gen_rng, 260u)
                            : (chance_permille(&e->gen_rng, 80u) ? !st->skip : st->skip);
        if (smask[i] & MELLO_RAND_PROB) {
            if (high) {
                if (chance_permille(&e->gen_rng, 250u))
                    st->probability = 65535u;
                else
                    st->probability = prob_from_hundredths((uint8_t)(20u + rng_bounded(&e->gen_rng, 81u)));
            } else {
                int32_t d = random_delta(&e->gen_rng, 7) * 655;
                st->probability = clamp_u16_random((int32_t)st->probability + d, 0u, 65535u);
            }
        }
        if (smask[i] & MELLO_RAND_SAMPLE) {
            if (high)
                st->sample_slot = (uint8_t)rng_bounded(&e->gen_rng, MELLO_SAMPLE_SLOT_COUNT + 1u);
            else
                st->sample_slot = clamp_u8_random((int32_t)st->sample_slot + random_delta(&e->gen_rng, 1), 0u,
                                                  MELLO_SAMPLE_SLOT_COUNT);
        }
        if (smask[i] & MELLO_RAND_WAVE) { /* HIGH: any other wave; LOW: the next one up or down */
            uint8_t w = st->wave
                            ? st->wave
                            : (uint8_t)(e->sound.osc_wave >= MELLO_WAVE_OFF ? 1u : 1u + e->sound.osc_wave);
            if (high) {
                uint8_t n = (uint8_t)(1u + rng_bounded(&e->gen_rng, 5u));
                if (n == w) n = (uint8_t)(1u + (n + rng_bounded(&e->gen_rng, 4u)) % 5u);
                st->wave = n;
            } else
                st->wave = clamp_u8_random((int32_t)w + random_delta(&e->gen_rng, 1), 1u, 5u);
        }
    }
#if MELLO_FEATURE_CHORD_MACHINE
    if ((mask & MELLO_RAND_CHORD) && e->modr.rnd_amount[11] < 100u &&
        !chance_permille(&e->gen_rng, (uint16_t)(e->modr.rnd_amount[11] * 10u)))
        mask &= (uint16_t)~MELLO_RAND_CHORD;
#endif
#if MELLO_FEATURE_CHORD_MACHINE
    if (mask & MELLO_RAND_CHORD) {
        if (high) { /* a progression with plenty of colour, and the voicings thrown wide (2026-11-01) */
            roll_progression(e, sd->n, 500u);
            for (i = 0u; i < e->chord.length; ++i) {
                MelloChordStep *cs = &e->chord.steps[i];
                cs->octave = (int8_t)((int32_t)rng_bounded(&e->gen_rng, 3u) - 1);
                cs->spread = (uint8_t)rng_bounded(&e->gen_rng, 4u);
                if (chance_permille(&e->gen_rng, 150u)) cs->shape = MELLO_CHORD_POWER;
            }
        } else {
            uint8_t n = e->chord.length < 2u ? 2u : e->chord.length;
            e->chord.length = n;
            for (i = 0u; i < n; ++i) {
                MelloChordStep *cs = &e->chord.steps[i];
                cs->degree = clamp_u8_random((int32_t)cs->degree + random_delta(&e->gen_rng, 1), 0u,
                                             (uint8_t)(sd->n - 1u));
                if (chance_permille(&e->gen_rng, 120u))
                    cs->shape = (MelloChordShape)rng_bounded(&e->gen_rng, MELLO_CHORD_SHAPE_COUNT);
                cs->inversion =
                    clamp_u8_random((int32_t)cs->inversion + random_delta(&e->gen_rng, 1), 0u, 3u);
                cs->octave = clamp_i8((int32_t)cs->octave + random_delta(&e->gen_rng, 1), -2, 2);
                cs->spread = clamp_u8_random((int32_t)cs->spread + random_delta(&e->gen_rng, 1), 0u, 3u);
                cs->hold = clamp_u8_random((int32_t)cs->hold + random_delta(&e->gen_rng, 1), 1u, 16u);
            }
        }
        for (i = 0u; i < e->chord.pattern_length; ++i) {
            if (high) {
                uint8_t all = (uint8_t)((1u << e->chord.voices) - 1u);
                e->chord.pattern_masks[i] = (uint8_t)(1u + rng_bounded(&e->gen_rng, all));
            } else if (chance_permille(&e->gen_rng, 180u)) {
                uint8_t bit = (uint8_t)(1u << rng_bounded(&e->gen_rng, e->chord.voices));
                e->chord.pattern_masks[i] = (uint8_t)((e->chord.pattern_masks[i] ^ bit) & 0x0Fu);
                if (!e->chord.pattern_masks[i]) e->chord.pattern_masks[i] = bit;
            }
        }
        e->chord = sanitize_chord_config(e->chord);
    }
#endif
    randomize_links(e, pmax, smask);
    sanitize_project(&e->project);
    e->sound = sanitize_sound_config(e->sound);
    if (e->playing)
        rebuild_preserving_phase(e);
    else
        e->runtime_built = false;
}

/* MUSICAL (feedback 2026-10-31: "musical instead of random; I press RANDOM many times and then tweak"). The sequence
 * as a phrase of four cells in a form (A A' A B, A B A B', A A A B or A B A C), or, at 16 and 32 steps, sometimes two
 * (A A' or A B); each cell drawn once and repeated, so the loop has a motif and an answer. A length that is not 8, 12
 * .. 32 is one cell. In a cell the first step and every fourth are strong: they mostly play (A's first always), on a
 * chord tone (root, third, fifth), and take the accents; the steps between
 * walk by scale steps and rest more often. A' and a middle B end on the fifth, the last cell on the root. Pulses stay 1
 * and nothing is skipped, so a bar stays a bar. The mask and EDIT's amounts still pick what changes. */
static uint8_t mus_metric(uint8_t o)
{
    return o % 4u == 0u ? 2u : (o % 2u == 0u ? 1u : 0u);
}

static uint8_t mus_degree(const ScaleDef *sd, int8_t semis)
{
    uint8_t i, best = 0u;
    for (i = 0u; i < sd->n; ++i) {
        int32_t a = sd->v[i] - semis, b = sd->v[best] - semis;
        if (a * a < b * b) best = i;
    }
    return best;
}

/* The chord tone nearest d, in 0..pmax. */
static uint8_t mus_snap(int32_t d, uint8_t pmax, uint8_t n, uint8_t t3, uint8_t t5)
{
    int32_t k;
    for (k = 0; k <= (int32_t)n; ++k) {
        int32_t c[2] = {d - k, d + k}, j;
        for (j = 0; j < 2; ++j) {
            uint8_t deg;
            if (c[j] < 0 || c[j] > (int32_t)pmax) continue;
            deg = (uint8_t)(c[j] % n);
            if (deg == 0u || deg == t3 || deg == t5) return (uint8_t)c[j];
        }
    }
    return (uint8_t)(d < 0 ? 0 : (d > (int32_t)pmax ? pmax : d));
}

/* A cell's notes (MUSICAL v2, DECISIONS.md "A melody, not a walk"): the line heads for a target (the contour's
 * register for this cell, a few scale degrees off the start), mostly by step, sometimes by a skip of a third, now
 * and then by a leap of a fourth to a sixth that the next note answers with a step back the other way (the oldest
 * rule of melody), and never the same note three times. Strong steps still land on chord tones; weak steps pass
 * between them. A note pushed past the range goes up an octave (the octave field) rather than bouncing back. */
/* v2.2 (feedback: "not complex enough"): a cell's rhythm is a motif from a library (bit o = step o plays; 3+3+2
 * figures, syncopations, a denser or an offbeat answer), varied by a flipped weak step now and then; 0 falls back
 * to the play chances (cells of odd lengths). */
static uint16_t mus_rhythm(MelloEngine *e, uint8_t cl, bool down, int16_t density)
{
    static const uint16_t four_down[6] = {13u, 11u, 9u, 15u, 5u, 3u}, /* x.xx xx.x x..x xxxx x.x. xx.. */
        four_answer[6] = {13u, 14u, 10u, 11u, 6u, 7u},                /* x.xx .xxx .x.x xx.x .xx. xxx. */
        eight_down[7] = {73u, 109u, 91u, 213u, 153u, 87u, 81u}, /* x..x..x. x.xx.xx. xx.xx.x. x.x.x.xx ... */
        eight_answer[7] = {90u, 221u, 150u, 109u, 73u, 91u, 213u}; /* .x.xx.x. x.xxx.xx .xx.x..x ... */
    uint16_t r;
    if (cl == 4u)
        r = down ? four_down[rng_bounded(&e->gen_rng, 6u)] : four_answer[rng_bounded(&e->gen_rng, 6u)];
    else if (cl == 8u)
        r = down ? eight_down[rng_bounded(&e->gen_rng, 7u)] : eight_answer[rng_bounded(&e->gen_rng, 7u)];
    else if (cl == 16u)
        r = (uint16_t)(eight_down[rng_bounded(&e->gen_rng, 7u)] |
                       (eight_answer[rng_bounded(&e->gen_rng, 7u)] << 8));
    else
        return 0u;
    if (density > 0 && cl >= 8u)
        r |= (uint16_t)(1u << (1u + 2u * rng_bounded(&e->gen_rng, cl / 2u)));               /* busy */
    if (density < 0) r &= (uint16_t)~(1u << (1u + 2u * rng_bounded(&e->gen_rng, cl / 2u))); /* sparse */
    return r;
}

static void mus_cell(MelloEngine *e, MelloStep *c, uint8_t cl, uint8_t start, int32_t target, uint8_t lo,
                     uint8_t hi, uint8_t pmax, uint8_t n, uint8_t t3, uint8_t t5, int16_t density,
                     const uint8_t slot[2], bool down)
{
    static const uint16_t play[3] = {420u, 680u, 880u}; /* weak, medium, strong */
    uint8_t o, hits = 0u, same = 0u;
    uint16_t rhythm = mus_rhythm(e, cl, down, density);
    bool cross =
        chance_permille(&e->gen_rng, 350u); /* accents every third step: a cross-rhythm over the bar */
    int32_t d = start, before = -99, last_leap = 0, lo32 = lo,
            hi32 = (int32_t)hi + (int32_t)n; /* an octave of headroom */
    for (o = 0u; o < cl; ++o) {
        MelloStep *st = &c[o];
        uint8_t m = mus_metric(o);
        int32_t p = o ? (int32_t)play[m] + density : (down ? 1000 : 920 + density), prev = d;
        bool plays;
        st->pulses = 1u, st->ratchet = 1u, st->skip = false, st->octave = 0, st->slide = false;
        st->probability = 65535u;
        if (rhythm) {
            plays = (rhythm >> o) & 1u;
            if (m == 0u && chance_permille(&e->gen_rng, 100u))
                plays = !plays; /* a weak step flipped: variety */
            if (o == 0u && down) plays = true;
        } else
            plays = chance_permille(&e->gen_rng, (uint16_t)(p < 0 ? 0 : p));
        if (!plays) {
            st->gate = MELLO_GATE_REST, st->accent = st->sub = false, st->sample_slot = 0u;
            continue;
        }
        if (hits++) {
            uint32_t r = rng_bounded(&e->gen_rng, 100u);
            int32_t toward = target > d ? 1 : (target < d ? -1 : 0), dir;
            /* the direction: towards the target four times in five, else the other way; never straight back to
             * the note before the last (a line, not a trill) */
            dir = toward && chance_permille(&e->gen_rng, 800u)
                      ? toward
                      : (chance_permille(&e->gen_rng, 500u) ? 1 : -1);
            if (d + dir == before && toward) dir = toward;
            if (last_leap) { /* after a leap: a step back the other way */
                d -= last_leap > 0 ? 1 : -1;
                last_leap = 0;
            } else if (r < 40u)
                d += dir;
            else if (r < 65u)
                d += 2 * dir;
            else if (
                r <
                85u) { /* a leap of a fourth, fifth or sixth to a chord tone; one leap in five an octave */
                int32_t leap = (r >= 82u ? (int32_t)n : (int32_t)(3u + rng_bounded(&e->gen_rng, 3u))) * dir;
                d = mus_snap(d + leap, pmax + n, n, t3, t5);
                last_leap = d - prev;
            } else if (same >= 1u) /* a repeat, but never three alike */
                d += dir;
            if (d < lo32) d = lo32 + (lo32 - d);
            if (d > hi32) d = hi32 - (d - hi32);
            if (d < lo32) d = lo32;
            if (d > hi32) d = hi32;
        }
        if (m == 2u) d = mus_snap(d, pmax + n, n, t3, t5);
        same = d == prev && hits > 1u ? (uint8_t)(same + 1u) : 0u;
        if (hits > 1u) before = prev;
        if (d > (int32_t)pmax) { /* above the range: the same degree an octave up */
            st->octave = 1;
            st->pitch = (int8_t)(d - n);
        } else
            st->pitch = (int8_t)d;
        st->gate = m == 0u && chance_permille(&e->gen_rng, 200u) ? MELLO_GATE_MULTI : MELLO_GATE_SINGLE;
        st->accent = cross ? o % 3u == 0u : chance_permille(&e->gen_rng, m == 2u ? 400u : 120u);
        st->sub = chance_permille(&e->gen_rng, d % n == 0 ? 550u : 80u);
        if (m == 0u && chance_permille(&e->gen_rng, 140u)) st->ratchet = 2u; /* a ratchet on a weak step */
        if (m != 2u &&
            chance_permille(&e->gen_rng, 250u)) /* a chance note: the loop varies from round to round */
            st->probability = prob_from_hundredths(m == 0u ? 60u : 75u);
        st->sample_slot = m == 2u ? slot[0] : (chance_permille(&e->gen_rng, 250u) ? slot[1] : 0u);
    }
    if (!down && cl >= 4u && c[2].gate != MELLO_GATE_REST && c[1].gate == MELLO_GATE_REST &&
        chance_permille(&e->gen_rng, 300u)) { /* an answer's syncopation: the middle beat anticipated */
        c[1] = c[2];
        c[2].gate = MELLO_GATE_REST, c[2].accent = c[2].sub = false, c[2].sample_slot = 0u;
    }
    for (o = 0u; o + 1u < cl;
         ++o) /* slides into the weak steps that follow a note, a long note before a rest */
        if (c[o].gate != MELLO_GATE_REST) {
            if (c[o + 1u].gate == MELLO_GATE_REST)
                c[o].gate = chance_permille(&e->gen_rng, 300u) ? MELLO_GATE_HOLD : c[o].gate;
            else if (!mus_metric((uint8_t)(o + 1u)))
                c[o + 1u].slide = chance_permille(&e->gen_rng, 400u);
        }
    if (!hits) c[0].gate = MELLO_GATE_SINGLE, c[0].pitch = (int8_t)start;
}

/* A' from A (and B' from B): the same rhythm, flags and samples, the line moved by shift scale degrees (a sequence,
 * as a tune answers itself a third up or a step down); strong steps snapped back onto chord tones, a note pushed
 * past the top taken an octave up on the octave field, one below the bottom reflected. */
/* how: 0 a sequence (the line moved), 1 an inversion (the line mirrored about its first note, then moved), 2 a
 * displacement (the rhythm a step later, the line moved), so the answer is recognisably A and still not A again. */
static void mus_develop(MelloStep *dst, const MelloStep *src, uint8_t cl, int32_t shift, uint8_t how,
                        uint8_t lo, uint8_t pmax, uint8_t n, uint8_t t3, uint8_t t5)
{
    uint8_t o;
    int32_t axis = 0;
    for (o = 0u; o < cl; ++o)
        if (src[o].gate != MELLO_GATE_REST) {
            axis = (int32_t)src[o].pitch + (src[o].octave > 0 ? (int32_t)n : 0);
            break;
        }
    for (o = 0u; o < cl; ++o) {
        int32_t d;
        const MelloStep *s = &src[how == 2u ? (o + cl - 1u) % cl : o];
        dst[o] = *s;
        if (s->gate == MELLO_GATE_REST) continue;
        d = (int32_t)s->pitch + (s->octave > 0 ? (int32_t)n : 0);
        if (how == 1u) d = axis - (d - axis);
        d += shift;
        if (d < (int32_t)lo) d = (int32_t)lo + ((int32_t)lo - d);
        if (mus_metric(o) == 2u) d = mus_snap(d, pmax + n, n, t3, t5);
        if (d > (int32_t)pmax + (int32_t)n) d = (int32_t)pmax + (int32_t)n;
        if (d < 0) d = 0;
        dst[o].octave = d > (int32_t)pmax ? 1 : 0;
        dst[o].pitch = (int8_t)(d > (int32_t)pmax ? d - n : d);
    }
}

/* The cell's last note on the root (2) or the fifth (1). */
static void mus_end(MelloStep *c, uint8_t cl, uint8_t how, uint8_t home, uint8_t fifth)
{
    uint8_t o = cl;
    while (o-- > 0u)
        if (c[o].gate != MELLO_GATE_REST) {
            c[o].pitch = (int8_t)(how == 2u ? home : fifth);
            c[o].octave = 0;
            if (how == 2u) c[o].probability = 65535u;
            return;
        }
}

/* GROOVE (ROADMAP E10 / F15): MUSICAL's cell with more rhythm on it. In order: ties (a note on a beat before a rest
 * held over two pulses, the rest skipped, so a bar stays a bar), ratchets on the offbeats each answered by a rest, a
 * roll into the cell's end (its last note three or four times, accented), and ghost notes (a weak rest between two
 * notes plays the note before it again, quietly, as a chance note). Strong steps keep their chord tones; nothing
 * plays after the cell's last note, so the phrase still ends where MUSICAL ends it. */
#define GROOVE_GHOST_HUNDREDTHS 45u /* a ghost note's chance; MUSICAL's own chance notes are 60 and 75 */

static void groove_rest(MelloStep *st)
{
    st->gate = MELLO_GATE_REST, st->accent = st->sub = false, st->sample_slot = 0u, st->ratchet = 1u;
    st->probability = 65535u;
}

/* A tie must stay in its cell: a skip follows a held two-pulse note, and a two-pulse note is followed by a skip. */
static void groove_tidy(MelloStep *c, uint8_t cl)
{
    uint8_t o;
    for (o = 0u; o < cl; ++o) {
        if (c[o].skip && (o == 0u || c[o - 1u].pulses != 2u)) c[o].skip = false;
        if (c[o].pulses == 2u && (o + 1u >= cl || !c[o + 1u].skip)) c[o].pulses = 1u;
    }
}

static void groove_cell(MelloEngine *e, MelloStep *c, uint8_t cl, bool down)
{
    uint32_t answered = 0u; /* steps that must stay rests: the answers */
    int last = -1, o;
    for (o = 0; o + 1 < (int)cl; ++o) /* ties */
        if (c[o].gate != MELLO_GATE_REST && mus_metric((uint8_t)o) && !(o == 0 && down) &&
            chance_permille(&e->gen_rng, c[o + 1].gate == MELLO_GATE_REST ? 650u : 150u)) {
            c[o].pulses = 2u, c[o].gate = MELLO_GATE_HOLD, c[o].ratchet = 1u;
            groove_rest(&c[o + 1]); /* held over the rest after it, or over the weak note it swallows */
            c[o + 1].skip = true;
            answered |= 1u << (o + 1);
        }
    for (o = 1; o < (int)cl; o += 2) /* offbeat ratchets, each answered by a rest */
        if (c[o].gate != MELLO_GATE_REST && c[o].pulses == 1u && chance_permille(&e->gen_rng, 200u)) {
            c[o].ratchet = 2u;
            if (o + 1 < (int)cl && mus_metric((uint8_t)(o + 1)) != 2u && chance_permille(&e->gen_rng, 650u)) {
                groove_rest(&c[o + 1]);
                answered |= 1u << (o + 1);
            }
        }
    for (o = (int)cl - 1; o >= 0; --o)
        if (c[o].gate != MELLO_GATE_REST) {
            last = o;
            break;
        }
    if (last > 0 && c[last].pulses == 1u && chance_permille(&e->gen_rng, down ? 400u : 600u)) /* a roll in */
        c[last].ratchet = (uint8_t)(3u + rng_bounded(&e->gen_rng, 2u)), c[last].accent = true;
    for (o = 1; o < last; ++o) /* ghost notes on the weak rests between notes: the note before, again */
        if (c[o].gate == MELLO_GATE_REST && !c[o].skip && !mus_metric((uint8_t)o) && !(answered & (1u << o)) &&
            chance_permille(&e->gen_rng, 600u)) {
            int b = o - 1;
            while (b > 0 && c[b].gate == MELLO_GATE_REST) --b;
            if (c[b].gate == MELLO_GATE_REST) continue;
            c[o] = c[b];
            c[o].gate = MELLO_GATE_SINGLE, c[o].pulses = 1u, c[o].ratchet = 1u, c[o].skip = false;
            c[o].accent = c[o].sub = c[o].slide = false, c[o].sample_slot = 0u;
            c[o].probability = prob_from_hundredths(GROOVE_GHOST_HUNDREDTHS);
        } else if (c[o].gate != MELLO_GATE_REST && !mus_metric((uint8_t)o) && c[o].ratchet == 1u &&
                   chance_permille(&e->gen_rng, 320u)) /* or a weak note made one */
            c[o].accent = false, c[o].probability = prob_from_hundredths(GROOVE_GHOST_HUNDREDTHS);
    groove_tidy(c, cl);
}

static void randomize_musical(MelloEngine *e, bool groove)
{
    /* Forms: cell ids 0 A, 1 A', 2 B, 3 B', 4 C; and their chance in permille. */
    /* Forms (v2.1, feedback 2026-11-01: "sounds like a looped 8-step"): A comes back word for word in two forms of
     * five, and late or not at all in the others; the answer develops A or brings new material. Two halves are
     * always A B, a motif and a contrasting answer, never A and its transposition. */
    static const uint8_t forms[5][4] = {{0, 1, 2, 4}, {0, 2, 1, 4}, {0, 2, 4, 1}, {0, 1, 0, 2}, {0, 2, 0, 3}},
                         halves[2][2] = {{0, 2}, {0, 2}};
    static const uint16_t form_w[5] = {250u, 250u, 200u, 150u, 150u};
    static const uint8_t base_of[5] = {0, 0, 1, 1, 2};
    MelloStep tmp[MELLO_MAX_STEPS];
    uint16_t mask, smask[MELLO_MAX_STEPS];
    const ScaleDef *sd;
    uint8_t pmax, n, t3, t5, home, fifth, lo, hi, len, cl, cells, f = 0u, i, fp,
                                                                  first[3] = {255u, 255u, 255u};
    uint8_t slot[2];
    int16_t density;
    uint32_t r;
    mask = (uint16_t)(e->randomize.mask & MELLO_RAND_ALL);
    sd = &SCALES[e->project.scale < MELLO_SCALE_COUNT ? e->project.scale : MELLO_SCALE_MINOR];
    n = sd->n;
    pmax = (uint8_t)(n * e->project.pitch_span_oct - 1u);
    t3 = mus_degree(sd, 3);
    if (sd->v[t3] != 3 && sd->v[t3] != 4) t3 = mus_degree(sd, 4);
    t5 = mus_degree(sd, 7);
    home = pmax >= n + 4u ? n : 0u;
    fifth = (uint8_t)(home + t5 <= pmax ? home + t5 : (home + t5 >= n ? home + t5 - n : home));
    lo = home > n / 2u ? (uint8_t)(home - n / 2u) : 0u;
    hi = pmax; /* the whole span; a cell heading higher takes the octave field (mus_cell) */
    len = e->project.seq_len ? e->project.seq_len : 16u;
    if (len > MELLO_MAX_STEPS) len = MELLO_MAX_STEPS;
    if (len % 16u == 0u && chance_permille(&e->gen_rng, 250u))
        cl = (uint8_t)(len / 2u), cells = 2u;
    else if (len >= 8u && len % 4u == 0u)
        cl = (uint8_t)(len / 4u), cells = 4u;
    else
        cl = len, cells = 1u;
    r = rng_bounded(&e->gen_rng, 1000u);
    while (f < 4u && r >= form_w[f]) r -= form_w[f++];
    if (cells == 2u) f = (uint8_t)(f & 1u);
    density = (int16_t)((int32_t)rng_bounded(&e->gen_rng, 3u) * 150 - 150); /* sparse, as is, busy */
    slot[0] = chance_permille(&e->gen_rng, 400u)
                  ? 0u
                  : (uint8_t)(1u + rng_bounded(&e->gen_rng, MELLO_SAMPLE_SLOT_COUNT));
    slot[1] = (uint8_t)(1u + rng_bounded(&e->gen_rng, MELLO_SAMPLE_SLOT_COUNT));
    /* The phrase's contour (DECISIONS.md "A melody, not a walk"): where each cell sits, in scale degrees from
     * home: an ARCH climbs to its climax in the answer and comes home, a RISE keeps climbing to the end, a WAVE
     * dips before it climbs, a FALL opens high and settles. The climax is the third or the fourth cell, never the
     * first, as in most tunes; A' and B' are A and B moved along the contour (a sequence), not copies. */
    {
        static const int8_t contour[4][5] = {                    /* by cell id: A, A', B, B', C */
                                             {0, 3, 5, 4, 2},    /* ARCH */
                                             {0, 2, 4, 6, 7},    /* RISE */
                                             {0, -3, 4, 5, 2},   /* WAVE */
                                             {0, -2, 5, 3, -3}}; /* FALL (B high, C low) */
        uint8_t ct = (uint8_t)rng_bounded(&e->gen_rng, 4u), climax_oct = chance_permille(&e->gen_rng, 450u);
        for (i = 0u; i < MELLO_MAX_STEPS; ++i) tmp[i] = e->project.steps[i];
        for (fp = 0u; fp < cells; ++fp) {
            uint8_t id = cells == 1u ? 0u : (cells == 2u ? halves[f][fp] : forms[f][fp]), b = base_of[id];
            MelloStep *c = &tmp[fp * cl];
            int32_t reg = contour[ct][id]; /* this cell's register, in degrees from home */
            if (id == 1u || id == 3u) {    /* A', B': the base cell moved along the contour */
                uint8_t base = id == 1u ? first[0] : first[1];
                int32_t shift = reg - contour[ct][id - 1u];
                if (base == 255u) base = first[0];
                r = rng_bounded(&e->gen_rng, 1000u);
                mus_develop(c, &tmp[base * cl], cl, shift, r < 450u ? 0u : (r < 750u ? 1u : 2u), lo, pmax, n,
                            t3, t5);
                if (groove) groove_tidy(c, cl); /* a displaced answer may push a tie across the cell */
            } else if (first[b] == 255u) { /* A from home, B and C from a chord tone at their register */
                int32_t start = (int32_t)home + reg, target = start;
                uint8_t o;
                if (b) {
                    start = mus_snap(start, pmax + n, n, t3, t5);
                    if (start == (int32_t)home) start = (int32_t)home + t3;
                    target =
                        start +
                        (cells == 4u ? (int32_t)contour[ct][fp + 1u < 4u ? forms[f][fp + 1u] : id] - reg : 0);
                    if (climax_oct && reg >= 3) start += n, target += n; /* the climax an octave up */
                } else
                    target =
                        (int32_t)home +
                        (cells > 1u ? (int32_t)contour[ct][cells == 2u ? halves[f][1] : forms[f][1]] : 0);
                if (start > (int32_t)pmax + (int32_t)n) start = (int32_t)pmax + (int32_t)n;
                mus_cell(e, c, cl, (uint8_t)(start > (int32_t)pmax ? start - n : start), target, lo, hi, pmax,
                         n, t3, t5, density, slot, b == 0u);
                if (groove) groove_cell(e, c, cl, b == 0u);
                if (start > (int32_t)pmax) /* the cell's first note carries the octave the start asked for */
                    for (o = 0u; o < cl; ++o)
                        if (c[o].gate != MELLO_GATE_REST) {
                            c[o].octave = 1;
                            break;
                        }
                if (mask & MELLO_RAND_WAVE) { /* WAVE: a wave a cell, so the timbre moves with the phrase */
                    uint8_t w = roll_wave_med(e);
                    for (o = 0u; o < cl; ++o) c[o].wave = w;
                }
                first[b] = fp;
            } else
                memcpy(c, &tmp[first[b] * cl], sizeof(MelloStep) * cl);
            if (fp + 1u == cells)
                mus_end(c, cl, 2u, home, fifth);
            else if (id)
                mus_end(c, cl, 1u, home, fifth);
            if (fp + 1u == cells && chance_permille(&e->gen_rng, 200u)) c[cl - 1u].ratchet = 2u; /* a fill */
        }
    }
    for (i = len; i < MELLO_MAX_STEPS; ++i)
        tmp[i] = tmp[i % len]; /* a longer sequence later plays it again */
    randomize_step_masks(e, mask, smask);
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        MelloStep *st = &e->project.steps[i];
        const MelloStep *m = &tmp[i];
        if (smask[i] & MELLO_RAND_PITCH) st->pitch = m->pitch;
        if (smask[i] & MELLO_RAND_PULSES) st->pulses = m->pulses;
        if (smask[i] & MELLO_RAND_RATCHET) st->ratchet = m->ratchet;
        if (smask[i] & MELLO_RAND_GATE) st->gate = m->gate;
        if (smask[i] & MELLO_RAND_OCTAVE) st->octave = m->octave;
        if (smask[i] & MELLO_RAND_ACCENT) st->accent = m->accent;
        if (smask[i] & MELLO_RAND_SUB) st->sub = m->sub;
        if (smask[i] & MELLO_RAND_SLIDE) st->slide = m->slide;
        if (smask[i] & MELLO_RAND_SKIP) st->skip = false;
        if (smask[i] & MELLO_RAND_PROB) st->probability = m->probability;
        if (smask[i] & MELLO_RAND_SAMPLE) st->sample_slot = m->sample_slot;
        if (smask[i] & MELLO_RAND_WAVE) st->wave = m->wave;
        if (groove) {
            if (smask[i] & MELLO_RAND_PULSES) st->pulses = m->pulses, st->skip = m->skip; /* the ties */
            if ((smask[i] & MELLO_RAND_GATE) && m->probability == prob_from_hundredths(GROOVE_GHOST_HUNDREDTHS))
                st->probability = m->probability; /* a ghost note is a chance note whatever the mask says */
        }
    }
    if (groove) /* a tie the mask cut in two (one step rolled, the next not) is no tie */
        for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
            bool here = (smask[i] & MELLO_RAND_PULSES) != 0u;
            if (here && tmp[i].pulses == 2u && !(i + 1u < MELLO_MAX_STEPS && (smask[i + 1u] & MELLO_RAND_PULSES)))
                e->project.steps[i].pulses = 1u;
            if (here && tmp[i].skip && !(i > 0u && (smask[i - 1u] & MELLO_RAND_PULSES)))
                e->project.steps[i].skip = false;
        }
#if MELLO_FEATURE_CHORD_MACHINE
    if ((mask & MELLO_RAND_CHORD) &&
        (e->modr.rnd_amount[11] >= 100u ||
         chance_permille(&e->gen_rng, (uint16_t)(e->modr.rnd_amount[11] * 10u)))) {
        roll_progression(e, n, 0u);
    }
#endif
    sanitize_project(&e->project);
    e->sound = sanitize_sound_config(e->sound);
    if (e->playing)
        rebuild_preserving_phase(e);
    else
        e->runtime_built = false;
}

/* STYLE (2026-11-04, DECISIONS "STYLE"). The product owner: "it feels very oompah; I want electronica, ambient,
 * Aphex Twin, Oneohtrix Point Never, even techno or EDM; more styles, and part of the randomness, so a press does
 * not feel the same every time". The oompah was the defaults working together: BROKEN's Alberti figure in the
 * chords, a bass walking root and fifth on the beats, a rock beat. A STYLE roll writes the whole band in one style,
 * each with its own lead shape, bass line, beat, chords and a little of the sound, every part with some dice so no
 * two rolls of a style are alike. */
static const char *const STYLE_NAMES[MELLO_STYLE_COUNT] = {"OFF",    "ANY",  "AMBIENT", "DUB", "TRIPHOP",
                                                           "TECHNO", "ACID", "ELECTRO", "BREAKS", "IDM"};
const char *mello_style_name(uint8_t style)
{
    return STYLE_NAMES[style < MELLO_STYLE_COUNT ? style : 0u];
}

/* The lead: MUSICAL's phrase or GROOVE's, then shaped; only what RANDOM's mask lets change. ACID writes its own. */
static void style_lead(MelloEngine *e, uint8_t st)
{
    const ScaleDef *sd = &SCALES[e->project.scale < MELLO_SCALE_COUNT ? e->project.scale : MELLO_SCALE_MINOR];
    uint16_t mask = e->randomize.mask;
    uint8_t len = e->project.seq_len ? e->project.seq_len : 16u, i, n = sd->n;
    bool gate = (mask & MELLO_RAND_GATE) != 0u, rat = (mask & MELLO_RAND_RATCHET) != 0u,
         acc = (mask & MELLO_RAND_ACCENT) != 0u, sld = (mask & MELLO_RAND_SLIDE) != 0u,
         oct = (mask & MELLO_RAND_OCTAVE) != 0u, pit = (mask & MELLO_RAND_PITCH) != 0u;
    if (len > MELLO_MAX_STEPS) len = MELLO_MAX_STEPS;
    if (st == MELLO_STYLE_ACID) { /* a 303's line: nearly every sixteenth, the root most, slides and accents */
        static const uint8_t pick[8] = {0u, 0u, 0u, 0u, 2u, 4u, 6u, 255u}; /* 255: the octave */
        for (i = 0u; i < len; ++i) {
            MelloStep *x = &e->project.steps[i];
            uint8_t d = pick[rng_bounded(&e->gen_rng, 8u)];
            if (pit) x->pitch = (int8_t)(d == 255u ? n : d % n), x->octave = 0;
            if (oct && chance_permille(&e->gen_rng, 220u)) x->octave = 1;
            if (gate) x->gate = i && chance_permille(&e->gen_rng, 160u) ? MELLO_GATE_REST : MELLO_GATE_SINGLE;
            if (rat) x->ratchet = 1u;
            if (sld) x->slide = chance_permille(&e->gen_rng, 300u);
            if (acc) x->accent = chance_permille(&e->gen_rng, 350u);
            if (mask & MELLO_RAND_PULSES) x->pulses = 1u;
            if (mask & MELLO_RAND_SKIP) x->skip = false;
        }
        for (i = len; mask && i < MELLO_MAX_STEPS; ++i) e->project.steps[i] = e->project.steps[i % len];
        return;
    }
    randomize_musical(e, st == MELLO_STYLE_TECHNO || st == MELLO_STYLE_ELECTRO || st == MELLO_STYLE_IDM ||
                             st == MELLO_STYLE_BREAKS);
    for (i = 0u; i < len; ++i) {
        MelloStep *x = &e->project.steps[i];
        bool plays = x->gate != MELLO_GATE_REST && !x->skip, weak = (i & 1u) != 0u;
        switch (st) {
        case MELLO_STYLE_AMBIENT: /* few notes, each held into the next, gliding now and then */
            if (gate && plays && i && chance_permille(&e->gen_rng, weak ? 650u : 300u))
                x->gate = MELLO_GATE_REST;
            else if (gate && plays)
                x->gate = MELLO_GATE_HOLD;
            if (rat) x->ratchet = 1u;
            if (acc) x->accent = false;
            if (sld) x->slide = chance_permille(&e->gen_rng, 300u);
            break;
        case MELLO_STYLE_DUB: /* sparse, short: the delay plays the rest */
            if (gate && plays && i && chance_permille(&e->gen_rng, 600u)) x->gate = MELLO_GATE_REST;
            else if (gate && plays) x->gate = MELLO_GATE_SINGLE;
            if (rat) x->ratchet = 1u;
            break;
        case MELLO_STYLE_TRIPHOP: /* long, lazy notes */
            if (gate && plays && chance_permille(&e->gen_rng, 500u)) x->gate = MELLO_GATE_HOLD;
            if (rat) x->ratchet = 1u;
            if (sld && plays) x->slide = chance_permille(&e->gen_rng, 200u);
            break;
        case MELLO_STYLE_TECHNO: /* one bar's motif of four sixteenths, over and over; accents off the beat */
            if (i >= 4u && pit && gate) {
                MelloStep m = e->project.steps[i % 4u];
                *x = m;
            }
            if (acc) x->accent = (i % 4u) == 2u;
            if (rat) x->ratchet = 1u;
            break;
        case MELLO_STYLE_ELECTRO: /* staccato, robotic, the octave jumping */
            if (gate && plays) x->gate = MELLO_GATE_SINGLE;
            if (rat) x->ratchet = 1u;
            if (sld) x->slide = false;
            if (oct && plays) x->octave = chance_permille(&e->gen_rng, 200u) ? 1 : 0;
            break;
        case MELLO_STYLE_IDM: /* rolls of three or four, the octave either way */
            if (rat && plays && chance_permille(&e->gen_rng, 220u)) x->ratchet = (uint8_t)(3u + rng_bounded(&e->gen_rng, 2u));
            if (oct && plays && chance_permille(&e->gen_rng, 250u)) x->octave = chance_permille(&e->gen_rng, 500u) ? 1 : -1;
            break;
        default: break; /* BREAKS: GROOVE as it is */
        }
    }
    for (i = len; mask && i < MELLO_MAX_STEPS; ++i) e->project.steps[i] = e->project.steps[i % len];
}

/* A bass line in a style, from a figure (steps, tone, octave), with some dice; the voice's decay to suit. */
typedef struct {
    uint8_t k, tone;
    int8_t octave;
    bool glide, accent;
} StyleNote;
static void bass_figure(MelloBassConfig *b, const StyleNote *f, uint8_t count)
{
    uint8_t i;
    memset(b->steps, 0, sizeof b->steps);
    for (i = 0u; i < count; ++i) {
        MelloBassStep *x = &b->steps[f[i].k % MELLO_BASS_STEPS];
        x->on = true, x->tone = f[i].tone, x->octave = f[i].octave, x->glide = f[i].glide, x->accent = f[i].accent;
    }
}
static void style_bass_line(MelloEngine *e, uint8_t st, MelloBassConfig *b, MelloSubConfig *sub)
{
    enum { R = MELLO_SUB_TONE_ROOT, T3 = MELLO_SUB_TONE_THIRD, F4 = MELLO_SUB_TONE_FOURTH, F5 = MELLO_SUB_TONE_FIFTH,
           S7 = MELLO_SUB_TONE_SEVENTH };
    uint8_t i;
    b->length = 16u, b->follow = false;
    sub->on = true;
    if (sub->mode == MELLO_SUB_MODE_DRONE) sub->mode = MELLO_SUB_MODE_STEP;
    b->on = true;
    switch (st) {
    case MELLO_STYLE_AMBIENT: /* no line: one low note held, moving with the chords' root */
        b->on = false;
        sub->mode = MELLO_SUB_MODE_DRONE;
        break;
    case MELLO_STYLE_DUB: {
        static const StyleNote f[4] = {{0, R, 0, false, true}, {6, R, 0, false, false}, {8, R, 0, false, false},
                                       {14, F5, 0, true, false}};
        bass_figure(b, f, 4u);
        b->env_decay_ms = 700u, b->env = 150u, b->res = 150u;
        break;
    }
    case MELLO_STYLE_TRIPHOP: {
        static const StyleNote f[5] = {{0, R, 0, false, true}, {7, R, 0, false, false}, {10, F5, 0, true, false},
                                       {11, F4, 0, false, false}, {14, S7, 0, true, false}};
        bass_figure(b, f, 5u);
        b->env_decay_ms = 450u, b->env = 250u, b->res = 300u;
        break;
    }
    case MELLO_STYLE_TECHNO: { /* the off-beat eighths, the sixteenths after them now and then */
        static const StyleNote f[4] = {{2, R, 0, false, true}, {6, R, 0, false, true}, {10, R, 0, false, true},
                                       {14, R, 0, false, true}};
        bass_figure(b, f, 4u);
        for (i = 3u; i < 16u; i += 4u)
            if (chance_permille(&e->gen_rng, 350u)) b->steps[i].on = true, b->steps[i].tone = R, b->steps[i].octave = 1;
        b->env_decay_ms = 140u, b->env = 450u, b->res = 450u;
        break;
    }
    case MELLO_STYLE_ACID: { /* the lead is the 303; the bass a low pedal under it */
        static const StyleNote f[2] = {{0, R, 0, false, true}, {8, R, 0, false, false}};
        bass_figure(b, f, 2u);
        b->env_decay_ms = 260u, b->env = 100u, b->res = 100u;
        break;
    }
    case MELLO_STYLE_ELECTRO: {
        static const StyleNote f[6] = {{0, R, 0, false, true}, {3, R, 0, false, false}, {6, R, 1, false, false},
                                       {10, F5, 0, false, true}, {11, R, 0, true, false}, {14, R, 0, false, false}};
        bass_figure(b, f, 6u);
        b->env_decay_ms = 180u, b->env = 550u, b->res = 500u;
        break;
    }
    case MELLO_STYLE_BREAKS: {
        static const StyleNote f[5] = {{0, R, 0, false, true}, {6, R, 0, false, false}, {10, R, 0, false, true},
                                       {11, F5, 0, true, false}, {14, S7, 0, false, false}};
        bass_figure(b, f, 5u);
        b->env_decay_ms = 280u, b->env = 350u, b->res = 300u;
        break;
    }
    case MELLO_STYLE_IDM: { /* off the grid: five to nine notes anywhere, leaps and glides */
        static const uint8_t tones[6] = {R, R, F5, T3, S7, MELLO_SUB_TONE_OCTAVE};
        uint8_t notes = (uint8_t)(5u + rng_bounded(&e->gen_rng, 5u));
        memset(b->steps, 0, sizeof b->steps);
        b->steps[0].on = true, b->steps[0].accent = true;
        for (i = 1u; i < notes; ++i) {
            MelloBassStep *x = &b->steps[1u + rng_bounded(&e->gen_rng, 15u)];
            x->on = true, x->tone = tones[rng_bounded(&e->gen_rng, 6u)];
            x->octave = (int8_t)(chance_permille(&e->gen_rng, 300u) ? 1 : 0);
            x->glide = chance_permille(&e->gen_rng, 350u);
            x->accent = chance_permille(&e->gen_rng, 250u);
        }
        b->env_decay_ms = 220u, b->env = 500u, b->res = 550u;
        return;
    }
    default: return;
    }
    /* the dice: a note moved by a step or another tone, now and then, the one left alone */
    for (i = 1u; i < 16u; ++i)
        if (b->steps[i].on && chance_permille(&e->gen_rng, 180u)) {
            uint8_t to = (uint8_t)(i + (chance_permille(&e->gen_rng, 500u) ? 1u : 15u)) % 16u;
            if (to && !b->steps[to].on) b->steps[to] = b->steps[i], b->steps[i].on = false;
        } else if (b->steps[i].on && b->steps[i].tone == R && chance_permille(&e->gen_rng, 120u))
            b->steps[i].tone = F5;
}

void mello_engine_style_bass(MelloEngine *e, uint8_t style)
{
    MelloBassConfig b;
    MelloSubConfig sub;
    if (!e) return;
    if (style <= MELLO_STYLE_ANY) {
        mello_engine_bass_roll(e);
        return;
    }
    mello_engine_undo_snapshot(e);
    b = e->bass, sub = e->sub;
    style_bass_line(e, style, &b, &sub);
    mello_engine_sub_set_config(e, &sub);
    mello_engine_bass_set_config(e, &b);
}

/* The beat: a figure per voice (hits, accents), the locks cleared, a few ghost notes and rolls by chance. */
static void style_drums(MelloEngine *e, uint8_t st)
{
    /* KICK, SNARE, HAT, GLITCH: hits and accents */
    static const uint16_t figs[8][8] = {
        /* AMBIENT: no kick, a few ticks */ {0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x1001u, 0x0000u, 0x0000u, 0x0000u},
        /* DUB */ {0x1111u, 0x1111u, 0x1000u, 0x1000u, 0x4444u, 0x4444u, 0x0000u, 0x0000u},
        /* TRIPHOP */ {0x0501u, 0x0001u, 0x1010u, 0x1010u, 0x5555u, 0x1111u, 0x0000u, 0x0000u},
        /* TECHNO */ {0x1111u, 0x1111u, 0x1010u, 0x1010u, 0xEEEEu, 0x4444u, 0x0000u, 0x0000u},
        /* ACID */ {0x1111u, 0x1111u, 0x1010u, 0x0000u, 0xEEEEu, 0x4444u, 0x0000u, 0x0000u},
        /* ELECTRO */ {0x0481u, 0x0081u, 0x1010u, 0x1010u, 0xFFFFu, 0x4444u, 0x0000u, 0x0000u},
        /* BREAKS */ {0x0401u, 0x0401u, 0x1290u, 0x1010u, 0x5555u, 0x0000u, 0x0000u, 0x0000u},
        /* IDM: from dice below */ {0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u}};
    static const uint16_t swing[8] = {0u, 120u, 280u, 0u, 60u, 0u, 160u, 0u};
    const uint16_t *f = figs[st - MELLO_STYLE_AMBIENT];
    MelloDrumConfig d = e->drums;
    uint8_t v, k;
    d.on = true, d.follow = false, d.length = 16u;
    d.swing = swing[st - MELLO_STYLE_AMBIENT];
    memset(d.lock, 0, sizeof d.lock);
    for (v = 0u; v < MELLO_DRUM_VOICES; ++v) {
        d.hits[v] = f[v * 2u], d.accents[v] = f[v * 2u + 1u], d.voice[v].chance = 1000u;
        for (k = 0u; k < MELLO_DRUM_STEPS; ++k) d.lock[v][k].ratchet = 1u;
    }
    switch (st) {
    case MELLO_STYLE_AMBIENT: /* soft ticks that come and go, a glitch now and then */
        d.voice[MELLO_DRUM_HAT].chance = 450u;
        for (k = 0u; k < 16u; ++k)
            if (chance_permille(&e->gen_rng, 180u)) d.hits[MELLO_DRUM_HAT] |= (uint16_t)(1u << k);
        if (chance_permille(&e->gen_rng, 600u)) d.hits[MELLO_DRUM_GLITCH] = (uint16_t)(1u << (2u + rng_bounded(&e->gen_rng, 14u)));
        break;
    case MELLO_STYLE_TECHNO:
    case MELLO_STYLE_ACID: /* the snare a clap, sometimes a beat late; the glitch on a sixteenth */
        if (chance_permille(&e->gen_rng, 300u)) d.hits[MELLO_DRUM_SNARE] |= 0x4000u;
        if (chance_permille(&e->gen_rng, 500u)) d.hits[MELLO_DRUM_GLITCH] = (uint16_t)(1u << (rng_bounded(&e->gen_rng, 8u) * 2u + 1u));
        break;
    case MELLO_STYLE_IDM: { /* a broken beat: kicks off the grid, a snare that moves, hats that roll */
        uint8_t kicks = (uint8_t)(3u + rng_bounded(&e->gen_rng, 3u));
        d.hits[MELLO_DRUM_KICK] = 1u;
        for (k = 1u; k < kicks; ++k) d.hits[MELLO_DRUM_KICK] |= (uint16_t)(1u << (1u + rng_bounded(&e->gen_rng, 15u)));
        d.hits[MELLO_DRUM_SNARE] = (uint16_t)((1u << 4u) | (1u << (9u + rng_bounded(&e->gen_rng, 5u))));
        d.accents[MELLO_DRUM_SNARE] = d.hits[MELLO_DRUM_SNARE];
        for (k = 0u; k < 16u; ++k) {
            if (chance_permille(&e->gen_rng, 600u)) d.hits[MELLO_DRUM_HAT] |= (uint16_t)(1u << k);
            if (chance_permille(&e->gen_rng, 250u)) d.lock[MELLO_DRUM_HAT][k].ratchet = (uint8_t)(2u + rng_bounded(&e->gen_rng, 3u));
            if (chance_permille(&e->gen_rng, 120u)) d.hits[MELLO_DRUM_GLITCH] |= (uint16_t)(1u << k);
            if (chance_permille(&e->gen_rng, 200u)) d.lock[MELLO_DRUM_GLITCH][k].ratchet = 3u;
            d.lock[MELLO_DRUM_HAT][k].pitch = (int8_t)((int32_t)rng_bounded(&e->gen_rng, 9u) - 4);
        }
        break;
    }
    default: /* a ghost note or two, a roll into the bar's end */
        for (k = 0u; k < 16u; ++k)
            if (!(d.hits[MELLO_DRUM_SNARE] & (1u << k)) && (k & 1u) && chance_permille(&e->gen_rng, 70u))
                d.hits[MELLO_DRUM_SNARE] |= (uint16_t)(1u << k);
        if (chance_permille(&e->gen_rng, 400u)) d.lock[MELLO_DRUM_HAT][15].ratchet = 2u;
        break;
    }
    mello_engine_drums_set_config(e, &d);
}

#if MELLO_FEATURE_CHORD_MACHINE
/* The chords: their voice and how they are struck, and the chain from what the bass plays (FIT's reading) or, for
 * AMBIENT, a progression the drone follows. */
static void style_chords(MelloEngine *e, uint8_t st)
{
    MelloChordConfig c = e->chord;
    const ScaleDef *sd = &SCALES[e->project.scale < MELLO_SCALE_COUNT ? e->project.scale : MELLO_SCALE_MINOR];
    uint8_t deg[8], slots, k, hold = 8u;
    MelloChordShape colour = MELLO_CHORD_SEVENTH;
    c.on = true, c.poly = true, c.trigger = MELLO_CHORD_TRIGGER_STEP, c.chain = MELLO_CHORD_CHAIN_STEPS;
    c.voices = 4u, c.strum_ms = 0u, c.voice_lead = true, c.sub_root = false, c.lead_follow = 0u;
    switch (st) {
    case MELLO_STYLE_AMBIENT: /* a slow pad, a chord a bar or two, the drone under its root */
        c.mode = MELLO_CHORD_HOLD, c.attack_ms = 900u, c.release_ms = 3000u, c.decay_ms = 2000u, c.sustain = 800u;
        c.cut = 450u, c.fenv = 120u, c.chorus = 800u, c.space = 1000u, c.strum_ms = 30u, c.sub_root = true;
        hold = chance_permille(&e->gen_rng, 500u) ? 16u : 32u;
        colour = chance_permille(&e->gen_rng, 500u) ? MELLO_CHORD_SUS2 : MELLO_CHORD_SEVENTH;
        break;
    case MELLO_STYLE_DUB: /* minor sevenths stabbed off the beat, into the delay */
    case MELLO_STYLE_TECHNO:
        c.mode = MELLO_CHORD_STAB, c.trigger = MELLO_CHORD_TRIGGER_GRID;
        c.grid_mode = MELLO_DOT_HORIZONTAL, c.grid_length = 16u, c.grid_reset = true, c.grid_amount = 1000u;
        c.grid_follow = false;
        c.grid_cells = st == MELLO_STYLE_DUB ? 0x4444u : (uint16_t)(0x4444u & (chance_permille(&e->gen_rng, 500u) ? 0x4444u : 0x0404u));
        c.attack_ms = 3u, c.release_ms = st == MELLO_STYLE_DUB ? 260u : 120u, c.decay_ms = 180u, c.sustain = 200u;
        c.cut = st == MELLO_STYLE_DUB ? 420u : 520u, c.fenv = 300u, c.chorus = 300u;
        c.space = st == MELLO_STYLE_DUB ? 1000u : 500u;
        hold = 16u;
        colour = st == MELLO_STYLE_DUB ? MELLO_CHORD_SEVENTH : (chance_permille(&e->gen_rng, 500u) ? MELLO_CHORD_SUS4 : MELLO_CHORD_SEVENTH);
        break;
    case MELLO_STYLE_TRIPHOP:
    case MELLO_STYLE_BREAKS: /* held sevenths, a chord a bar */
        c.mode = MELLO_CHORD_HOLD, c.attack_ms = 60u, c.release_ms = 1200u, c.decay_ms = 900u, c.sustain = 600u;
        c.cut = 500u, c.fenv = 200u, c.chorus = 600u, c.space = 700u;
        hold = 16u;
        break;
    case MELLO_STYLE_IDM: /* one voice running through the chord, coloured, a chord a half bar */
        c.poly = false, c.mode = chance_permille(&e->gen_rng, 500u) ? MELLO_CHORD_ARP_UPDOWN : MELLO_CHORD_WALK;
        c.attack_ms = 5u, c.release_ms = 400u, c.cut = 600u, c.fenv = 350u, c.space = 800u;
        hold = 8u;
        colour = chance_permille(&e->gen_rng, 500u) ? MELLO_CHORD_SUS2 : MELLO_CHORD_SEVENTH;
        break;
    default: /* ACID, ELECTRO: no chords */
        c.on = false;
        mello_engine_chord_set_config(e, &c);
        return;
    }
    if (st == MELLO_STYLE_AMBIENT) {
        e->chord = c;
        roll_progression(e, sd->n, 300u); /* the walk through the functions; the drone follows */
        c = e->chord;
        for (k = 0u; k < c.length; ++k) c.steps[k].hold = hold, c.steps[k].shape = colour, c.steps[k].spread = 2u;
    } else {
        slots = fit_read(e, hold, deg);
        c.length = slots;
        for (k = 0u; k < slots; ++k) {
            MelloChordStep *cs = &c.steps[k];
            cs->degree = deg[k], cs->hold = hold, cs->shape = colour, cs->inversion = 0u, cs->spread = 1u;
            cs->octave = -1;
        }
    }
    mello_engine_chord_set_config(e, &c);
}
#endif

/* A little of the sound: the lead's envelope (and ACID's filter), the delay and the reverb. */
static void style_sound(MelloEngine *e, uint8_t st)
{
    static const struct {
        uint16_t attack, decay, release, rev_wet, rev_size, rev_ms, dly_wet, dly_fb;
        uint8_t div;
        bool ping;
    } t[8] = {{120u, 900u, 1600u, 600u, 900u, 9000u, 300u, 550u, 7u, true},  /* AMBIENT: 4D */
              {5u, 220u, 300u, 300u, 700u, 4000u, 450u, 650u, 6u, true},    /* DUB: 8D */
              {10u, 400u, 600u, 350u, 700u, 3500u, 200u, 400u, 0u, false},  /* TRIPHOP: 8N */
              {3u, 120u, 120u, 180u, 500u, 2000u, 150u, 300u, 10u, false},  /* TECHNO: 16D */
              {3u, 160u, 100u, 120u, 400u, 1500u, 150u, 350u, 10u, false},  /* ACID: 16D */
              {3u, 100u, 80u, 150u, 400u, 1500u, 180u, 250u, 1u, false},    /* ELECTRO: 16N */
              {5u, 250u, 300u, 250u, 600u, 2500u, 200u, 350u, 0u, false},   /* BREAKS: 8N */
              {3u, 180u, 250u, 300u, 650u, 3000u, 250u, 400u, 4u, true}};   /* IDM: 16T */
    uint8_t i = (uint8_t)(st - MELLO_STYLE_AMBIENT);
    MelloSoundConfig s = e->sound;
    MelloFxConfig f = e->fx;
    s.attack_ms = t[i].attack, s.decay_ms = t[i].decay, s.release_ms = t[i].release;
    if (st == MELLO_STYLE_ACID) s.filter_cut = 280u, s.filter_res = 600u;
    f.reverb_wet = t[i].rev_wet, f.reverb_size = t[i].rev_size, f.reverb_decay_ms = t[i].rev_ms;
    f.delay_wet = t[i].dly_wet, f.delay_feedback = t[i].dly_fb, f.delay_sync = true, f.delay_div = t[i].div;
    f.delay_pingpong = t[i].ping;
    mello_engine_sound_set_config(e, &s);
    mello_engine_fx_set_config(e, &f);
}

uint8_t mello_engine_style_roll(MelloEngine *e, uint8_t style)
{
    uint8_t st = style;
    if (!e) return MELLO_STYLE_OFF;
    if (st == MELLO_STYLE_ANY) { /* a style, never the one before */
        do st = (uint8_t)(MELLO_STYLE_AMBIENT + rng_bounded(&e->gen_rng, MELLO_STYLE_COUNT - MELLO_STYLE_AMBIENT));
        while (st == e->style_now);
    }
    if (st < MELLO_STYLE_AMBIENT || st >= MELLO_STYLE_COUNT) {
        mello_engine_randomize_steps_intensity(e, MELLO_RANDOMIZE_MED);
        return MELLO_STYLE_OFF;
    }
    mello_engine_undo_snapshot(e);
    { /* the style's tempo, somewhere in its range (the transport takes it from the project: the UI's roll) */
        static const uint8_t lo[8] = {76u, 112u, 84u, 124u, 124u, 118u, 128u, 120u},
                             span[8] = {16u, 10u, 12u, 8u, 10u, 10u, 12u, 40u};
        uint8_t i = (uint8_t)(st - MELLO_STYLE_AMBIENT);
        e->project.bpm = (uint16_t)(lo[i] + rng_bounded(&e->gen_rng, span[i] + 1u));
    }
    style_lead(e, st);
    {
        MelloBassConfig b = e->bass;
        MelloSubConfig sub = e->sub;
        style_bass_line(e, st, &b, &sub);
        mello_engine_sub_set_config(e, &sub);
        mello_engine_bass_set_config(e, &b);
    }
    style_drums(e, st);
#if MELLO_FEATURE_CHORD_MACHINE
    style_chords(e, st);
#endif
    style_sound(e, st);
    e->style_now = st;
    sanitize_project(&e->project);
    if (e->playing)
        rebuild_preserving_phase(e);
    else
        e->runtime_built = false;
    return st;
}

void mello_engine_undo_snapshot(MelloEngine *e)
{
    if (!e) return;
    e->undo_project = e->project;
    e->undo_sound = e->sound;
#if MELLO_FEATURE_CHORD_MACHINE
    e->undo_chord = e->chord;
#endif
    e->undo_drums = e->drums;
    e->undo_bass = e->bass;
    e->undo_fx = e->fx;
    e->undo_sub = e->sub;
    e->undo_valid = true;
    e->undo_is_redo = false;
}

/* FNV-1a over what UNDO swaps. */
static uint32_t undo_hash_bytes(uint32_t h, const void *p, uint32_t n)
{
    const uint8_t *b = p;
    while (n--) h = (h ^ *b++) * 16777619u;
    return h;
}
static uint32_t undo_hash(const MelloEngine *e)
{
    uint32_t h = undo_hash_bytes(2166136261u, &e->project, sizeof e->project);
    h = undo_hash_bytes(h, &e->sound, sizeof e->sound);
#if MELLO_FEATURE_CHORD_MACHINE
    h = undo_hash_bytes(h, &e->chord, sizeof e->chord);
#endif
    h = undo_hash_bytes(h, &e->drums, sizeof e->drums);
    h = undo_hash_bytes(h, &e->fx, sizeof e->fx);
    h = undo_hash_bytes(h, &e->sub, sizeof e->sub);
    return undo_hash_bytes(h, &e->bass, sizeof e->bass);
}

uint8_t mello_engine_undo_state(MelloEngine *e)
{
    if (!e || !e->undo_valid) return MELLO_UNDO_NONE;
    if (e->undo_is_redo && undo_hash(e) != e->undo_hash) e->undo_valid = false; /* edited since: no redo */
    return !e->undo_valid ? MELLO_UNDO_NONE : e->undo_is_redo ? MELLO_UNDO_REDO : MELLO_UNDO_UNDO;
}

bool mello_engine_undo_swap(MelloEngine *e)
{
    MelloProject p;
    MelloSoundConfig s;
    if (!e || mello_engine_undo_state(e) == MELLO_UNDO_NONE) return false;
    p = e->project;
    e->project = e->undo_project;
    e->undo_project = p;
    s = e->sound;
    e->sound = e->undo_sound;
    e->undo_sound = s;
#if MELLO_FEATURE_CHORD_MACHINE
    {
        MelloChordConfig c = e->chord;
        e->chord = e->undo_chord;
        e->undo_chord = c;
    }
#endif
    {
        MelloDrumConfig d = e->drums;
        MelloBassConfig b = e->bass;
        e->drums = e->undo_drums;
        e->undo_drums = d;
        e->bass = e->undo_bass;
        e->undo_bass = b;
    }
    {
        MelloFxConfig f = e->fx;
        MelloSubConfig s2 = e->sub;
        e->fx = e->undo_fx;
        e->undo_fx = f;
        e->sub = e->undo_sub;
        e->undo_sub = s2;
    }
    (void)mello_engine_rebuild_sequence(e);
    e->undo_is_redo = !e->undo_is_redo;
    e->undo_hash = undo_hash(e);
    return true;
}

void mello_engine_randomize_steps_intensity(MelloEngine *e, MelloRandomizeIntensity intensity)
{
    if (!e) return;
    mello_engine_undo_snapshot(e);
    if (e->drums.on && e->drums.feed) mello_engine_drums_roll(e); /* FEED: the drums roll with the pattern */
    if (intensity == MELLO_RANDOMIZE_MED) {
        mello_engine_randomize_steps(e);
        return;
    }
    if (intensity == MELLO_RANDOMIZE_MUSICAL || intensity == MELLO_RANDOMIZE_GROOVE) {
        randomize_musical(e, intensity == MELLO_RANDOMIZE_GROOVE);
        return;
    }
    if (intensity > MELLO_RANDOMIZE_HIGH) intensity = MELLO_RANDOMIZE_HIGH;
    randomize_steps_low_high(e, intensity);
}

static void tm_mutate(MelloEngine *e)
{
    uint8_t i;
    int32_t delta, next;
    if (!e || !e->tm.on || e->tm.locked) return;
    if (e->tm_bool_len == 0u) e->tm_bool_len = 1u;
    if (chance_permille(&e->gen_rng, e->tm.mutation_prob)) {
        uint8_t idx = (uint8_t)rng_bounded(&e->gen_rng, e->tm_bool_len);
        e->tm_bool_reg[idx] = gen_unit16(&e->gen_rng) >= 32768u ? 1u : 0u;
    }
    for (i = 0u; i < e->tm_float_len; ++i) {
        if (chance_permille(&e->gen_rng, e->tm.mutation_prob)) {
            int32_t centered = (int32_t)gen_unit16(&e->gen_rng) - 32768;
            delta = (centered * 3) / 5; /* source: (random-.5)*0.3*2 => +/-0.3 */
            next = (int32_t)e->tm_float_reg[i] + delta;
            if (next < 0) next = 0;
            if (next > 65535) next = 65535;
            e->tm_float_reg[i] = (uint16_t)next;
        }
    }
}

static uint8_t tm_index_for(const MelloEngine *e)
{
    uint8_t len = sanitize_tm_length(e->tm.length);
    uint32_t base;
    int32_t idx;
    if (e->tm.rate == MELLO_TM_RATE_HALF)
        base = (e->absolute_pulse / 2u) % len;
    else if (e->tm.rate == MELLO_TM_RATE_DOUBLE)
        base = ((e->absolute_pulse % len) * 2u) % len;
    else
        base = e->absolute_pulse % len;
    idx = (int32_t)base + (int32_t)e->tm.offset;
    while (idx < 0) idx += (int32_t)len;
    idx %= (int32_t)len;
    return (uint8_t)idx;
}

static int8_t tm_octave_from_value(uint16_t v, uint16_t depth)
{
    int32_t centered = (int32_t)v - 32768;
    int32_t num = centered * 4 * (int32_t)depth;
    const int32_t half = 32767500; /* (65535*1000)/2 */
    const int32_t three_half = 98302500;
    if (num >= three_half) return 2;
    if (num >= half) return 1;
    if (num >= -half) return 0; /* JS Math.round(-0.5) -> -0 */
    if (num >= -three_half) return -1;
    return -2;
}

static bool euclid_hit_u32(uint32_t step, uint32_t total, uint32_t pulses, uint32_t rotation)
{
    uint32_t p, i;
    if (total == 0u || pulses == 0u) return false;
    p = pulses < total ? pulses : total;
    i = (step + (rotation % total)) % total;
    return (((i + 1u) * p) / total) != ((i * p) / total);
}

/* DOT's evolve, at the loop's one: each lit cell moves, with a chance of EVOLVE / 3 (at most 33%), to a free
 * neighbour on the 4x4 grid among the cells the matrix plays. The number of lit cells stays. */
static void dot_evolve(MelloEngine *e)
{
    uint8_t id;
    for (id = 0u; id < MELLO_DOT_MATRIX_COUNT; ++id) {
        MelloDotMatrixConfig *c = &e->dot[id];
        uint16_t play = 0u, cells = c->cells;
        uint8_t i, len;
        if (!e->modr.dot_evolve[id] || !cells) continue;
        len = c->length < 1u ? 1u : (c->length > MELLO_DOT_CELL_COUNT ? MELLO_DOT_CELL_COUNT : c->length);
        if (e->dot_runtime[id].length == len && e->dot_runtime[id].mode != MELLO_DOT_WALK)
            for (i = 0u; i < e->dot_runtime[id].period; ++i)
                play |= (uint16_t)(1u << e->dot_runtime[id].order[i]);
        else
            play = (uint16_t)((1u << len) - 1u);
        for (i = 0u; i < MELLO_DOT_CELL_COUNT; ++i) {
            uint8_t cand[4], n = 0u, r = i >> 2, col = i & 3u, k;
            if (!(c->cells & (1u << i)) ||
                !chance_permille(&e->gen_rng, (uint16_t)(e->modr.dot_evolve[id] / 3u)))
                continue;
            if (r > 0u) cand[n++] = (uint8_t)(i - 4u);
            if (r < 3u) cand[n++] = (uint8_t)(i + 4u);
            if (col > 0u) cand[n++] = (uint8_t)(i - 1u);
            if (col < 3u) cand[n++] = (uint8_t)(i + 1u);
            for (k = 0u; k < n;) /* free cells the matrix plays */
                if ((cells & (1u << cand[k])) || !(play & (1u << cand[k])))
                    cand[k] = cand[--n];
                else
                    ++k;
            if (!n) continue;
            k = cand[(uint8_t)rng_bounded(&e->gen_rng, n)];
            cells = (uint16_t)((cells & ~(1u << i)) | (1u << k));
        }
        c->cells = cells;
    }
}

static uint8_t dot_route(MelloEngine *e, MelloDotRuntime *rt, MelloDotMode mode, uint8_t len, bool reverse,
                         uint32_t base)
{
    uint32_t cycle, pos;
    if (len < 1u) len = 1u;
    if (rt->mode != mode || rt->length != len || !rt->period) {
        rt->mode = mode;
        rt->length = len;
        rt->cycle = -1;
        rt->walk = 0u;
        rt->period = dot_build_order(mode, len, rt->order);
        if (mode == MELLO_DOT_RANDOM) dot_shuffle_order(e, rt->order, len);
    }
    if (mode == MELLO_DOT_WALK) { /* here now; next time a random neighbour among the first len cells */
        uint8_t here = rt->walk < len ? rt->walk : 0u, cand[4], n = 0u, r = here >> 2, c = here & 3u;
        if (r > 0u && here - 4u < len) cand[n++] = (uint8_t)(here - 4u);
        if (r < 3u && here + 4u < len) cand[n++] = (uint8_t)(here + 4u);
        if (c > 0u) cand[n++] = (uint8_t)(here - 1u);
        if (c < 3u && here + 1u < len) cand[n++] = (uint8_t)(here + 1u);
        rt->walk = n ? cand[rng_bounded(&e->gen_rng, n)] : here;
        return here;
    }
    cycle = base / rt->period;
    if (mode == MELLO_DOT_RANDOM && rt->cycle != (int32_t)cycle) dot_shuffle_order(e, rt->order, len);
    rt->cycle = (int32_t)cycle;
    pos = base % rt->period;
    if (reverse) pos = rt->period - 1u - pos;
    return rt->order[pos];
}

static void evaluate_dot_matrices(MelloEngine *e, bool is_step_start, bool *matrix_accent,
                                  uint8_t *ratchet_add, bool *matrix_sub,
                                  uint16_t dot_mod[MELLO_DOT_MOD_LANES])
{
    uint8_t id;
    uint32_t tick = e->dot_tick, follow = e->dot_follow;
    *matrix_accent = false;
    *ratchet_add = 0u;
    *matrix_sub = false;
    dot_mod[0] = dot_mod[1] = MELLO_DOT_NOT_READ;
    for (id = 0u; id < MELLO_DOT_MATRIX_COUNT; ++id) {
        MelloDotMatrixConfig cfg = sanitize_dot_config(e->dot[id]);
        MelloDotRuntime *rt = &e->dot_runtime[id];
        uint32_t linear, base;
        uint8_t cell;
        bool on;
        e->dot[id] = cfg;
        if (cfg.follow && !is_step_start) { /* not read this pulse: a MOD lane keeps its state */
            e->last_mod.dot_cell[id] = e->dot_active_cell[id];
            continue;
        }
        if (id == MELLO_DOT_MOD1 || id == MELLO_DOT_MOD2) dot_mod[id == MELLO_DOT_MOD2] = 0u;
        linear = cfg.follow ? follow : tick;
        base = cfg.reset ? (linear % (uint32_t)(e->project.seq_len ? e->project.seq_len : 16u)) : linear;
        cell = dot_route(e, rt, cfg.mode, cfg.length, cfg.reverse, base);
        e->dot_active_cell[id] = (int8_t)cell;
        e->last_mod.dot_cell[id] = (int8_t)cell;
        on = !cfg.off && (cfg.cells & (uint16_t)(1u << cell)) != 0u; /* OFF: the route moves, nothing is sent */
        if (!on) continue;
        if (id == MELLO_DOT_ACCENT) {
            if (chance_permille(&e->gen_rng, cfg.amount)) *matrix_accent = true;
        } else if (id == MELLO_DOT_MOD1 || id == MELLO_DOT_MOD2) {
            /* A MOD lane's amount is its depth; mello_audio shapes it (MelloModRouting). */
            dot_mod[id == MELLO_DOT_MOD2] = cfg.amount;
        } else if (id == MELLO_DOT_RATCHET) {
            if (chance_permille(&e->gen_rng, cfg.amount)) *ratchet_add = 1u;
        } else if (id == MELLO_DOT_SUB) {
            if (chance_permille(&e->gen_rng, cfg.amount)) *matrix_sub = true;
        }
    }
    e->dot_tick = tick + 1u;
    if (is_step_start) e->dot_follow = follow + 1u;
}

#if MELLO_FEATURE_CHORD_MACHINE
static uint8_t chord_scale_degree_midi(const MelloEngine *e, uint8_t degree, int8_t octave)
{
    int32_t root = 60;
    MelloScale scale = MELLO_SCALE_MINOR;
    int8_t lc_oct = 0, lc_pitch = 0;
    const ScaleDef *sd;
    int32_t d, idx, oct, midi;
    current_loop_context(e, &root, &scale, &lc_oct, &lc_pitch, NULL, NULL);
    sd = &SCALES[scale < MELLO_SCALE_COUNT ? scale : MELLO_SCALE_MINOR];
    d = degree;
    idx = d % sd->n;
    oct = d / sd->n;
    midi = root + sd->v[idx] + oct * 12 +
           ((int32_t)e->project.global_octave + (int32_t)lc_oct + (int32_t)octave) * 12 + (int32_t)lc_pitch;
    return lc_pitch ? key_round(scale, root, midi, lc_pitch) : clamp_u8_i32(midi, 12u, 108u);
}
uint8_t mello_engine_resolve_chord_notes(const MelloEngine *e, uint8_t chord_step,
                                         uint8_t out[MELLO_CHORD_MAX_VOICES])
{
    static const uint8_t tri[4] = {0u, 2u, 4u, 6u}, sus2[4] = {0u, 1u, 4u, 6u}, sus4[4] = {0u, 3u, 4u, 6u};
    const MelloChordStep *s;
    const uint8_t *off = tri;
    uint8_t n, i, inv;
    if (!e || !out) return 0u;
    s = &e->chord.steps[chord_step % e->chord.length];
    n = e->chord.voices;
    if (n < 2u) n = 2u;
    if (n > 4u) n = 4u;
    if (s->shape == MELLO_CHORD_TRIAD && n > 3u)
        n = 3u;
    else if (s->shape == MELLO_CHORD_POWER && n > 3u)
        n = 3u;
    if (s->shape == MELLO_CHORD_SUS2)
        off = sus2;
    else if (s->shape == MELLO_CHORD_SUS4)
        off = sus4;
    for (i = 0u; i < n; ++i) {
        if (s->shape == MELLO_CHORD_POWER) {
            uint8_t root = chord_scale_degree_midi(e, s->degree, s->octave);
            static const int8_t po[3] = {0, 7, 12};
            out[i] = clamp_u8_i32((int32_t)root + po[i], 12u, 108u);
        } else
            out[i] = chord_scale_degree_midi(e, (uint8_t)(s->degree + off[i]), s->octave);
    }
    inv = s->inversion;
    while (inv-- && n > 1u) {
        uint8_t x = (uint8_t)(out[0] + 12u), j;
        for (j = 0u; j + 1u < n; ++j) out[j] = out[j + 1u];
        out[n - 1u] = x > 108u ? 108u : x;
    }
    if (s->spread == 1u && n > 2u)
        out[n - 1u] = clamp_u8_i32((int32_t)out[n - 1u] + 12, 12u, 108u);
    else if (s->spread == 2u) {
        for (i = 1u; i < n; i += 2u) out[i] = clamp_u8_i32((int32_t)out[i] + 12, 12u, 108u);
    } else if (s->spread >= 3u) {
        for (i = 1u; i < n; ++i) out[i] = clamp_u8_i32((int32_t)out[i] + (i == n - 1u ? 24 : 12), 12u, 108u);
    }
    return n;
}
void mello_engine_chord_name(const MelloEngine *e, uint8_t chord_step, char out[8])
{
    static const char *names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const MelloChordStep *s;
    const char *q = "", *n;
    int32_t root, i3, i5, i7;
    uint8_t k = 0u;
    if (!out) return;
    out[0] = 0;
    if (!e || !e->chord.length) return;
    s = &e->chord.steps[chord_step % e->chord.length];
    root = chord_scale_degree_midi(e, s->degree, 0);
    i3 = ((int32_t)chord_scale_degree_midi(e, (uint8_t)(s->degree + 2u), 0) - root + 24) % 12;
    i5 = ((int32_t)chord_scale_degree_midi(e, (uint8_t)(s->degree + 4u), 0) - root + 24) % 12;
    i7 = ((int32_t)chord_scale_degree_midi(e, (uint8_t)(s->degree + 6u), 0) - root + 24) % 12;
    if (s->shape == MELLO_CHORD_POWER)
        q = "5";
    else if (s->shape == MELLO_CHORD_SUS2)
        q = "sus2";
    else if (s->shape == MELLO_CHORD_SUS4)
        q = "sus4";
    else if (s->shape == MELLO_CHORD_SEVENTH && e->chord.voices >= 4u)
        q = i3 == 3 && i5 == 6   ? (i7 == 9 ? "o7" : "h7") /* half-diminished */
            : i3 == 3            ? (i7 == 11 ? "mM7" : "m7")
            : i3 == 4 && i5 == 8 ? "+7"
                                 : (i7 == 11 ? "M7" : "7");
    else
        q = i3 == 3 && i5 == 6 ? "o" : i3 == 3 ? "m" : i5 == 8 ? "+" : "";
    for (n = names[root % 12]; *n && k < 7u; ++n) out[k++] = *n;
    for (; *q && k < 7u; ++q) out[k++] = *q;
    out[k] = 0;
}
static bool chord_grid_hit(MelloEngine *e, bool is_step_start)
{
    MelloChordConfig *c = &e->chord;
    MelloDotRuntime *rt = &e->chord_grid_runtime;
    uint32_t linear, base;
    uint8_t cell;
    bool on;
    if (c->trigger != MELLO_CHORD_TRIGGER_GRID) return is_step_start;
    if (c->grid_follow && !is_step_start) return false;
    linear = c->grid_follow ? e->chord_grid_follow : e->chord_grid_tick;
    base = c->grid_reset ? (linear % (uint32_t)(e->project.seq_len ? e->project.seq_len : 16u)) : linear;
    cell = dot_route(e, rt, c->grid_mode, c->grid_length, false, base);
    e->chord_grid_active_cell = (int8_t)cell;
    on = (c->grid_cells & (uint16_t)(1u << cell)) != 0u;
    e->chord_grid_tick++;
    if (is_step_start) e->chord_grid_follow++;
    return on && chance_permille(&e->gen_rng, c->grid_amount);
}
/* voice: which of the chord's voices (0 the lowest), carried in ratchet_index for the CHORD page's lanes. */
static void emit_chord_note(MelloEngine *e, MelloEventType type, uint32_t tick, uint8_t step, uint8_t pulse,
                            uint8_t total, uint8_t note, uint8_t vel, uint8_t voice)
{
    emit_basic(e, type, tick, step, pulse, total, note, vel, voice, false, true);
}

static void sort_notes(uint8_t *n, uint8_t c)
{
    uint8_t i, j;
    for (i = 1u; i < c; ++i)
        for (j = i; j > 0u && n[j - 1u] > n[j]; --j) {
            uint8_t t = n[j];
            n[j] = n[j - 1u];
            n[j - 1u] = t;
        }
}

/* Voice leading: of the chord's inversions, an octave down, as written or up, the one whose voices move least from
 * prev's (lowest to lowest, and so on) while staying near where the chord is written (a cost that grows with the
 * square of the distance), so a progression does not climb or sink round after round. notes is sorted on return. */
static void chord_voice_lead(uint8_t *notes, uint8_t n, const uint8_t *prev, uint8_t pn)
{
    uint8_t best[MELLO_CHORD_MAX_VOICES], cand[MELLO_CHORD_MAX_VOICES], r, i, m = n < pn ? n : pn;
    int32_t best_cost = 0x7FFFFFFF, written = 0, o;
    sort_notes(notes, n);
    for (i = 0u; i < n; ++i) written += notes[i];
    for (r = 0u; r < n; ++r)
        for (o = -12; o <= 12; o += 12) {
            int32_t cost = 0, sum = 0, d;
            bool ok = true;
            for (i = 0u; i < n; ++i) {
                int32_t v = (int32_t)notes[i] + (i < r ? 12 : 0) + o;
                if (v < 24 || v > 96) ok = false;
                cand[i] = (uint8_t)v;
            }
            if (!ok) continue;
            sort_notes(cand, n);
            for (i = 0u; i < n; ++i) sum += cand[i];
            for (i = 0u; i < m; ++i) {
                d = (int32_t)cand[i] - (int32_t)prev[i];
                cost += 2 * (d < 0 ? -d : d);
            }
            d = (sum - written) /
                (int32_t)n; /* the pull back to where it is written grows with the distance */
            cost += d * d;
            if (cost < best_cost) {
                best_cost = cost;
                memcpy(best, cand, n);
            }
        }
    if (best_cost != 0x7FFFFFFF) memcpy(notes, best, n);
}
/* HARMONY's DRIFT (DECISIONS.md "Harmony as the spine"): at each pass of the chain, with the chance DRIFT, one chord
 * changes within what DEPTH allows: DEG swaps it for a degree of the same function (tonic i / VI / III, subdominant
 * iv / ii / VI, dominant v / VII), SHAPE gives it another shape, VOICE another inversion, spread or octave, ALL any
 * of those. The first chord keeps its degree, so home stays home while the rest wanders: a progression that is
 * still itself after half an hour, and never quite the same. */
static void chord_drift(MelloEngine *e)
{
    static const uint8_t same_fn[7][2] = {{5u, 2u}, {3u, 6u}, {0u, 5u}, {1u, 5u},
                                          {6u, 1u}, {0u, 3u}, {4u, 1u}};
    MelloChordConfig *c = &e->chord;
    MelloChordStep *cs;
    uint8_t k, what;
    if (!c->drift || c->length < 2u || !chance_permille(&e->gen_rng, c->drift)) return;
    k = (uint8_t)rng_bounded(&e->gen_rng, c->length);
    cs = &c->steps[k];
    what = c->drift_depth < 3u ? c->drift_depth : (uint8_t)rng_bounded(&e->gen_rng, 3u);
    if (what == 0u && k == 0u) what = 1u + (uint8_t)rng_bounded(&e->gen_rng, 2u); /* home keeps its degree */
    switch (what) {
    case 0u: cs->degree = same_fn[cs->degree % 7u][rng_bounded(&e->gen_rng, 2u)]; break;
    case 1u: {
        static const MelloChordShape sh[4] = {MELLO_CHORD_TRIAD, MELLO_CHORD_SEVENTH, MELLO_CHORD_SUS2,
                                              MELLO_CHORD_SUS4};
        MelloChordShape was = cs->shape;
        do cs->shape = sh[rng_bounded(&e->gen_rng, 4u)];
        while (cs->shape == was);
        break;
    }
    default: {
        uint32_t r = rng_bounded(&e->gen_rng, 3u);
        if (r == 0u)
            cs->inversion = (uint8_t)((cs->inversion + 1u + rng_bounded(&e->gen_rng, 2u)) % 3u);
        else if (r == 1u)
            cs->spread = (uint8_t)((cs->spread + 1u + rng_bounded(&e->gen_rng, 2u)) % 3u);
        else
            cs->octave = (int8_t)(cs->octave == 0 ? -1 : 0);
        break;
    }
    }
}

static void chord_trigger(MelloEngine *e, const MelloPulseEntry *pe, bool advance_chain)
{
    MelloChordConfig *c = &e->chord;
    MelloChordStep *cs;
    uint8_t notes[4], n, i, mask = 0u, slot;
    uint32_t dur, unit_ticks, strum;
    bool fresh;
    if (!c->on || !pe) return;
    slot = (uint8_t)(e->chord_chain_pos % c->length);
    cs = &c->steps[slot];
    fresh = e->chord_chain_hold == 0u;
    n = mello_engine_resolve_chord_notes(e, slot, notes);
    if (slot !=
        e->chord_voiced_slot) { /* a new chord in the chain: the one sounding is voice leading's reference */
        memcpy(e->chord_prev, e->chord_voiced, sizeof e->chord_prev);
        e->chord_prev_n = e->chord_voiced_n;
        e->chord_voiced_slot = slot;
    }
    if (c->voice_lead && e->chord_prev_n) chord_voice_lead(notes, n, e->chord_prev, e->chord_prev_n);
    memcpy(e->chord_voiced, notes, n);
    e->chord_voiced_n = n;
    strum = c->strum_ms ? ms_to_ticks(&e->project, c->strum_ms) : 0u;
    if (!c->poly && c->mode == MELLO_CHORD_HOLD && n > 1u) { /* SUB MELLO holds the top tone (VLEAD: least move) */
        notes[0] = notes[n - 1u];
        n = 1u;
    }
    if (c->mode == MELLO_CHORD_HOLD) {
        if (fresh && !advance_chain && c->chain != MELLO_CHORD_CHAIN_TRIG)
            e->chord_chain_hold = 1u; /* STEPS / SEG: sounded; the clock says when the next one is */
        if (fresh) {
            for (i = 0u; i < e->chord_last_count; ++i)
                emit_chord_note(e, MELLO_EVENT_CHORD_NOTE_OFF, e->now_tick, pe->step_index, pe->pulse_index,
                                pe->total_pulses, e->chord_last_notes[i], 0u, i);
            for (i = 0u; i < n; ++i) { /* STRUM: each voice after the one under it */
                emit_chord_note(e, MELLO_EVENT_CHORD_NOTE_ON, e->now_tick + strum * i, pe->step_index,
                                pe->pulse_index, pe->total_pulses, notes[i], 88u, i);
                e->chord_last_notes[i] = notes[i];
            }
            e->chord_last_count = n;
        }
    } else {
        if (c->mode == MELLO_CHORD_STAB)
            mask = (uint8_t)((1u << n) - 1u);
        else if (c->mode == MELLO_CHORD_ARP_UP) {
            mask = (uint8_t)(1u << (e->chord_arp_pos % n));
            e->chord_arp_pos = (uint8_t)((e->chord_arp_pos + 1u) % n);
        } else if (c->mode == MELLO_CHORD_ARP_DOWN) {
            uint8_t p = (uint8_t)(e->chord_arp_pos % n);
            mask = (uint8_t)(1u << (n - 1u - p));
            e->chord_arp_pos = (uint8_t)((p + 1u) % n);
        } else if (c->mode == MELLO_CHORD_ARP_UPDOWN) {
            uint8_t p = e->chord_arp_pos;
            if (p >= n) p = (uint8_t)(n - 1u);
            mask = (uint8_t)(1u << p);
            if (n > 1u) {
                if (e->chord_arp_dir > 0) {
                    if (p + 1u >= n) {
                        e->chord_arp_dir = -1;
                        e->chord_arp_pos = (uint8_t)(n - 2u);
                    } else
                        e->chord_arp_pos = (uint8_t)(p + 1u);
                } else {
                    if (p == 0u) {
                        e->chord_arp_dir = 1;
                        e->chord_arp_pos = 1u;
                    } else
                        e->chord_arp_pos = (uint8_t)(p - 1u);
                }
            }
        } else if (c->mode == MELLO_CHORD_BROKEN) { /* low, high, middle, high (an Alberti bass's figure) */
            static const uint8_t fig3[4] = {0u, 2u, 1u, 2u}, fig4[4] = {0u, 2u, 1u, 3u};
            uint8_t k = (uint8_t)(e->chord_arp_pos % 4u);
            mask = (uint8_t)(1u << (n >= 4u ? fig4[k] : n == 3u ? fig3[k] : (uint8_t)(k % n)));
            e->chord_arp_pos = (uint8_t)((k + 1u) % 4u);
        } else if (c->mode == MELLO_CHORD_WALK) {
            /* On a beat (every fourth trigger) the chord tone nearest a step on; between, a scale step on: a line
             * that walks between the chord's tones and turns at its ends, so the chord is heard in time. */
            int32_t root = 60, lo = notes[0], hi = notes[0], last, best = notes[0];
            MelloScale scale = MELLO_SCALE_MINOR;
            uint8_t k;
            for (k = 1u; k < n; ++k) {
                if (notes[k] < lo) lo = notes[k];
                if (notes[k] > hi) hi = notes[k];
            }
            last = e->chord_walk_note ? e->chord_walk_note : lo;
            current_loop_context(e, &root, &scale, NULL, NULL, NULL, NULL);
            if (e->chord_arp_dir == 0) e->chord_arp_dir = 1;
            if (last >= hi + 12) e->chord_arp_dir = -1;
            if (last <= lo) e->chord_arp_dir = 1;
            if (!e->chord_walk_note || e->chord_arp_pos % 4u == 0u) {
                int32_t want = last + e->chord_arp_dir * 2, d = 0x7FFF;
                for (k = 0u; k < n * 2u; ++k) { /* the chord and its octave above */
                    int32_t t = (int32_t)notes[k % n] + (k >= n ? 12 : 0), dd = t > want ? t - want : want - t;
                    if (dd < d) d = dd, best = t;
                }
            } else
                best = key_round(scale, root, last + e->chord_arp_dir, e->chord_arp_dir);
            e->chord_walk_note = clamp_u8_i32(best, 24u, 96u);
            e->chord_arp_pos++;
            notes[0] = e->chord_walk_note; /* one note, as an ARP's */
            n = 1u;
            mask = 1u;
        } else {
            uint8_t pl = c->pattern_length ? c->pattern_length : 1u;
            mask = (uint8_t)(c->pattern_masks[e->chord_pattern_pos % pl] & ((1u << n) - 1u));
            e->chord_pattern_pos = (uint8_t)((e->chord_pattern_pos + 1u) % pl);
        }
        if (!c->poly && (mask & (uint8_t)(mask - 1u))) /* SUB MELLO: one tone, STAB's top, a PATTERN step's lowest */
            mask = c->mode == MELLO_CHORD_STAB ? (uint8_t)(1u << (n - 1u)) : (uint8_t)(mask & (uint8_t)-mask);
        /* STAB is deliberately pulse-locked: short chord hit. HOLD is the pad/background mode,
         * while ARP/PATTERN also advance on the master pulse. Harmony itself advances on steps. */
        unit_ticks = mello_pulse_ticks(&e->project);
        dur = (unit_ticks * (uint32_t)c->gate_amount) / 1000u;
        dur = clamp_duration_ms_as_ticks(&e->project, dur, 20u, 4000u);
        for (i = 0u; i < n; ++i)
            if (mask &
                (uint8_t)(1u << i)) { /* STRUM moves a voice's whole note (its OFF never comes first) */
                uint32_t at = e->now_tick + (c->mode == MELLO_CHORD_STAB ? strum * i : 0u);
                emit_chord_note(e, MELLO_EVENT_CHORD_NOTE_ON, at, pe->step_index, pe->pulse_index,
                                pe->total_pulses, notes[i], 88u, i);
                emit_chord_note(e, MELLO_EVENT_CHORD_NOTE_OFF, at + dur, pe->step_index, pe->pulse_index,
                                pe->total_pulses, notes[i], 0u, i);
            }
    }
    if (advance_chain) {
        e->chord_chain_hold++;
        if (e->chord_chain_hold >= cs->hold) {
            e->chord_chain_hold = 0u;
            e->chord_chain_pos = (uint8_t)((slot + 1u) % c->length);
            if (e->chord_chain_pos == 0u) chord_drift(e); /* the chain has gone round: HARMONY's DRIFT */
            e->chord_arp_pos = 0u;
            e->chord_arp_dir = 1;
            e->chord_pattern_pos = 0u;
        }
    }
}

/* CHAIN STEPS and SEG, every pulse: the chain's place by the clock (HOLD sixteenths of each chord) or by the segment
 * playing; a chord that has not sounded yet sounds now in HOLD (the other modes play it at their triggers). */
static void chord_chain_clock(MelloEngine *e, const MelloPulseEntry *pe)
{
    MelloChordConfig *c = &e->chord;
    uint8_t len = c->length ? c->length : 1u, slot = (uint8_t)(e->chord_chain_pos % len), next = slot;
    if (c->chain == MELLO_CHORD_CHAIN_SEG)
        next = (uint8_t)(current_project_segment(e) % len);
    else if (e->chord_step_count >= c->steps[slot].hold)
        next = (uint8_t)((slot + 1u) % len);
    if (next != slot || e->chord_step_count == 0u) {
        if (next != slot) e->chord_chain_hold = 0u;
        if (next != slot && next == 0u) chord_drift(e); /* round again: HARMONY's DRIFT */
        e->chord_chain_pos = next;
        e->chord_step_count = 0u;
        e->chord_arp_pos = 0u;
        e->chord_arp_dir = 1;
        e->chord_pattern_pos = 0u;
    }
    if (e->chord_step_count < 255u) e->chord_step_count++;
    if (c->mode == MELLO_CHORD_HOLD && e->chord_chain_hold == 0u) chord_trigger(e, pe, false);
}

/* SUB ROOT: the chord's root as semitones from the key's, the nearest way (-5 .. +6). */
static int32_t chord_root_shift(const MelloEngine *e, MelloScale scale)
{
    const ScaleDef *sd = &SCALES[scale < MELLO_SCALE_COUNT ? scale : MELLO_SCALE_MINOR];
    uint8_t len = e->chord.length ? e->chord.length : 1u,
            slot = e->chord_voiced_n ? e->chord_voiced_slot : (uint8_t)(e->chord_chain_pos % len),
            d = (uint8_t)(e->chord.steps[slot % MELLO_CHORD_MAX_STEPS].degree % sd->n);
    int32_t s = sd->v[d] % 12;
    return s > 6 ? s - 12 : s;
}
#endif

bool mello_engine_advance_pulse(MelloEngine *e)
{
    MelloRuntimeSegment *seg;
    MelloPulseEntry pe, real_pe;
    MelloPerfParts pp = {0u, 0u, 0u, 0u, 0u};
    MelloStep *s, arp_step;
    bool pass, did_wrap, matrix_accent = false, matrix_sub = false, tm_gate_sub = false,
                         has_gate_override = false, replay = false;
    uint8_t ratchet_add = 0u, eff_ratchet, main_note;
    MelloGateMode gate_override = MELLO_GATE_SINGLE, eff_gate;
    uint16_t dot_mod[MELLO_DOT_MOD_LANES];
    uint32_t pulse_ticks, next_tick, old_loop_ticks;
    int8_t auto_slot = -1;
    uint8_t i;

    if (!e || !e->playing || !e->has_active_step || !e->runtime_built || e->runtime_segment_count == 0u)
        return false;

    if (e->perf_restart) { /* RESTART: the first step, and the loop's one, now */
        e->perf_restart = false;
        e->active_runtime_segment = 0u;
        e->segment_pulse_pos = 0u;
        e->segment_repeat_done = 0u;
        e->absolute_pulse = 0u;
        e->dot_tick = e->dot_follow = 0u;
        if (e->sub.x_reset) e->sub_x_pos = 0u;
        e->perf_mode = 0u; /* a held REVERSE / REPEAT starts again from here */
        e->perf_hist_n = 0u;
    }
    set_current_entry(e);
    if (!e->has_active_step) return false;
    /* SWING (DECISIONS.md "Drums"): everything this pulse sounds, on an odd sixteenth, comes this much late. */
    e->swing_off = (e->absolute_pulse & 1u) ? mello_pulse_ticks(&e->project) * e->drums.swing / 2000u : 0u;
    seg = &e->runtime_segments[e->active_runtime_segment];
    pe = e->runtime_pulses[seg->offset + e->segment_pulse_pos];
    /* The performance keys: the sequence's own pulse goes into the history, and REVERSE / REPEAT play theirs. The
     * sequence's place moves on below as always, so letting go is in time. */
    e->perf_hist[e->perf_hist_w] = pe;
    {
        MelloPerfParts *h = &e->perf_parts_hist[e->perf_hist_w]; /* (the chord's, after its clock, below) */
        h->drum = e->drum_pos, h->bass = e->bass_pos;
#if MELLO_FEATURE_CHORD_MACHINE
        h->chord_pos = e->chord_chain_pos, h->chord_arp = e->chord_arp_pos, h->chord_pattern = e->chord_pattern_pos;
#endif
    }
    e->perf_hist_w = (uint8_t)((e->perf_hist_w + 1u) % MELLO_PERF_HISTORY);
    if (e->perf_hist_n < MELLO_PERF_HISTORY) e->perf_hist_n++;
    if (e->perf_mode && !e->perf_buf_n) perf_fill_buffer(e, e->perf_mode, e->perf_mode_len);
    if (e->hold_open && (e->perf_mode || pe.step_index != e->hold_step)) {
        /* A HOLD note nothing will end: REVERSE / REPEAT may not play its last pulse, and an edit (a roll, a step's
         * gate or pulses, a segment) may have taken that pulse away. End it now. */
        emit_basic(e, MELLO_EVENT_NOTE_OFF, e->now_tick, e->hold_step, 0u, e->hold_total, e->hold_note, 0u,
                   0u, false, true);
        e->hold_open = false;
    }
    if (e->drone_open && (!e->sub.on || e->sub.mode != MELLO_SUB_MODE_DRONE)) {
        emit_basic(e, MELLO_EVENT_SUB_NOTE_OFF, e->now_tick, e->drone_step, 0u, 1u, e->drone_note, 0u, 0u,
                   false, true);
        e->drone_open = false;
    }
    real_pe = pe;
    if (e->perf_mode && e->perf_buf_n) {
        replay = true;
        pp = e->perf_parts_buf[e->perf_buf_pos];
        pe = e->perf_buf[e->perf_buf_pos];
        e->perf_buf_pos = (uint8_t)((e->perf_buf_pos + 1u) % e->perf_buf_n);
    }
    s = &e->project.steps[pe.step_index];
    if (e->perf.arp) { /* ARP: the step's note as an arpeggio, root, third, fifth, octave, a pulse each */
        const ScaleDef *sd = &SCALES[e->project.scale < MELLO_SCALE_COUNT ? e->project.scale : MELLO_SCALE_MINOR];
        int32_t n = sd->n, pmax = n * (int32_t)(e->project.pitch_span_oct ? e->project.pitch_span_oct : 1u) - 1,
                off[4] = {0, 2, 4, n}, d = s->pitch + off[e->absolute_pulse % 4u];
        arp_step = *s;
        arp_step.gate = MELLO_GATE_SINGLE, arp_step.ratchet = 1u, arp_step.slide = false;
        if (d > pmax) d -= n, arp_step.octave = (int8_t)(arp_step.octave + 1);
        arp_step.pitch = (int8_t)d;
        s = &arp_step;
    }
    e->step_index = pe.step_index;
    e->pulse_index = pe.pulse_index;
    old_loop_ticks = e->loop_pulse_length;

    memset(&e->last_mod, 0, sizeof(e->last_mod));
    e->last_mod.tm_index = -1;
    e->last_mod.tm_length = 1024u;
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) e->last_mod.dot_cell[i] = e->dot_active_cell[i];

    /* Source order: evolve registers -> read TM routes -> probability -> Dot Matrix -> pitch/gates/sub. */
    if (e->tm.on) {
        uint8_t ti;
        bool b;
        uint16_t fv;
        int32_t centered;
        tm_ensure_lengths(e, e->tm.length);
        if (!e->modr.tm_on_loop) tm_mutate(e); /* else at the loop's one, below */
        ti = tm_index_for(e);
        e->tm_active_index = (int8_t)ti;
        b = e->tm_bool_reg[ti] != 0u;
        fv = e->tm_float_reg[ti];
        centered = (int32_t)fv - 32768;
        e->last_mod.tm_index = (int8_t)ti;
        e->last_mod.tm_bool = b;
        e->last_mod.tm_float = fv;
        tm_gate_sub = b; /* the sub's TM mode reads the bit, whatever the routes */
        /* LENGTH (ROADMAP E7): the register shortens the note, at DEPTH 100% down to a quarter; never a new attack,
         * never a rest played or a note silenced. RETRIG: the HTML's GATE, a lit bit (at DEPTH's chance) makes a
         * step that plays MULTI, a new attack on every pulse. */
        if ((e->tm.routes & MELLO_TM_ROUTE_LENGTH) != 0u)
            e->last_mod.tm_length =
                (uint16_t)(1024u - ((uint32_t)(65535u - fv) * e->tm.depth_gate / 65535u) * 768u / 1000u);
        if ((e->tm.routes & MELLO_TM_ROUTE_RETRIG) != 0u && b && chance_permille(&e->gen_rng, e->tm.depth_gate)) {
            has_gate_override = true;
            gate_override = MELLO_GATE_MULTI;
        }
        if ((e->tm.routes & MELLO_TM_ROUTE_FILTER) != 0u)
            e->last_mod.tm_filter_mod_q15 = (int16_t)((centered * (int32_t)e->tm.depth_filter) / 4000);
        if ((e->tm.routes & MELLO_TM_ROUTE_OCTAVE) != 0u)
            e->last_mod.tm_octave_mod = tm_octave_from_value(fv, e->tm.depth_octave);
        if ((e->tm.routes & MELLO_TM_ROUTE_DECAY) != 0u)
            e->last_mod.tm_decay_mod_q15 = (int16_t)((centered * (int32_t)e->tm.depth_decay) / 10000);
    } else {
        e->tm_active_index = -1;
    }

    pass = e->perf.fill || e->perf.arp || probability_pass(e, s->probability); /* FILL, ARP: every step plays */
    eff_gate = pass ? (has_gate_override && s->gate != MELLO_GATE_REST ? gate_override : s->gate) : MELLO_GATE_REST;
    if ((e->perf.fill || e->perf.arp) && eff_gate == MELLO_GATE_REST) eff_gate = MELLO_GATE_SINGLE;

    evaluate_dot_matrices(e, pe.pulse_index == 0u, &matrix_accent, &ratchet_add, &matrix_sub, dot_mod);
#if MELLO_FEATURE_CHORD_MACHINE
    {
        bool step_start = pe.pulse_index == 0u;
        bool fire;
        if (e->chord.trigger == MELLO_CHORD_TRIGGER_GRID)
            fire = chord_grid_hit(e, step_start);
        else if (e->chord.mode == MELLO_CHORD_ARP_UP || e->chord.mode == MELLO_CHORD_ARP_DOWN ||
                 e->chord.mode == MELLO_CHORD_ARP_UPDOWN || e->chord.mode == MELLO_CHORD_PATTERN ||
                 e->chord.mode == MELLO_CHORD_BROKEN || e->chord.mode == MELLO_CHORD_WALK)
            fire = true;
        else
            fire = step_start;
        if (e->perf.fill && step_start) /* FILL: the chords struck again on every step */
            fire = true;
        /* STEP-mode harmony advances only at logical step boundaries; its arp/pattern clock may tick inside the step.
         * CHAIN STEPS and SEG move it by their clock instead (chord_chain_clock), before the step's triggers. */
        if (e->chord.on && e->chord.chain != MELLO_CHORD_CHAIN_TRIG) {
            uint8_t real_pos, real_arp, real_pat;
            MelloPerfParts *h =
                &e->perf_parts_hist[(uint8_t)((e->perf_hist_w + MELLO_PERF_HISTORY - 1u) % MELLO_PERF_HISTORY)];
            chord_chain_clock(e, &pe);
            h->chord_pos = e->chord_chain_pos; /* the chord this pulse plays, after the clock */
            if (e->perf.fill && step_start && e->chord.mode == MELLO_CHORD_HOLD && e->chord_chain_hold) {
                e->chord_chain_hold = 0u; /* FILL: HOLD's chord again on each step */
                chord_trigger(e, &pe, false);
            }
            real_pos = e->chord_chain_pos, real_arp = e->chord_arp_pos, real_pat = e->chord_pattern_pos;
            if (replay) /* REVERSE / REPEAT: the chord, arpeggio and pattern of the pulse replayed */
                e->chord_chain_pos = pp.chord_pos, e->chord_arp_pos = pp.chord_arp,
                e->chord_pattern_pos = pp.chord_pattern;
            if (fire && e->chord.mode != MELLO_CHORD_HOLD) chord_trigger(e, &pe, false);
            if (replay) /* the chain keeps its own time underneath */
                e->chord_chain_pos = real_pos, e->chord_arp_pos = real_arp, e->chord_pattern_pos = real_pat;
        } else if (fire) /* CHAIN TRIG: the chain moves by its triggers, which REVERSE / REPEAT replay as they come */
            chord_trigger(e, &pe, e->chord.trigger == MELLO_CHORD_TRIGGER_GRID ? true : step_start);
    }
#endif
    eff_ratchet = clamp_1_8((uint8_t)(clamp_1_8(s->ratchet) + ratchet_add));
    if (e->perf.fill) eff_ratchet = clamp_1_8((uint8_t)(eff_ratchet * 2u)); /* FILL: twice the ratchets */
    e->last_mod.effective_gate = eff_gate;
    e->last_mod.effective_ratchet = eff_ratchet;
    e->last_mod.effective_accent = (bool)(s->accent || matrix_accent || e->perf.accent); /* ACCENT: all */
    e->last_mod.matrix_sub = matrix_sub;
    e->last_mod.dot_mod[0] = dot_mod[0];
    e->last_mod.dot_mod[1] = dot_mod[1];

    main_note =
        lead_follow_note(e, resolve_engine_step_midi_mod(e, s, e->last_mod.tm_octave_mod), pe.step_index);
    emit_basic(e, MELLO_EVENT_PULSE, e->now_tick, pe.step_index, pe.pulse_index, pe.total_pulses, main_note,
               velocity_for_accent(e->last_mod.effective_accent), 0u, s->slide, pass);
    if (e->segment_pulse_pos == 0u)
        emit_basic(e, MELLO_EVENT_SEGMENT_ENTER, e->now_tick, pe.step_index, pe.pulse_index, pe.total_pulses,
                   main_note, velocity_for_accent(e->last_mod.effective_accent), 0u, s->slide, pass);
    if (pe.pulse_index == 0u)
        emit_basic(e, MELLO_EVENT_STEP_ENTER, e->now_tick, pe.step_index, pe.pulse_index, pe.total_pulses,
                   main_note, velocity_for_accent(e->last_mod.effective_accent), 0u, s->slide, pass);
    generate_gate_events(e, s, pass, pe.pulse_index, pe.total_pulses, eff_gate, eff_ratchet,
                         e->last_mod.effective_accent, e->last_mod.tm_octave_mod);

    /* Browser sample layer lives inside triggerNote(): it only fires when the effective main gate
     * is not REST, only at pulseIdx 0, and its hold is the full logical step duration. */
    if (pe.pulse_index == 0u && e->sample.on && eff_gate != MELLO_GATE_REST) {
        uint8_t ss = e->sample.per_step ? s->sample_slot : (uint8_t)(e->sample.edit_slot + 1u);
        if (ss >= 1u && ss <= MELLO_SAMPLE_SLOT_COUNT) {
            uint32_t sample_off = e->now_tick + mello_pulse_ticks(&e->project) * (uint32_t)pe.total_pulses;
            emit_sample_pair(e, e->now_tick, sample_off, pe.step_index, pe.total_pulses, main_note,
                             velocity_for_accent(e->last_mod.effective_accent), (uint8_t)(ss - 1u), s->slide,
                             pass);
        }
    }

    /* BASS SEQ: a step every pulse (its own clock), or with FOLLOW at each of the sequence's steps; while on, the
     * sub's own MODE waits. */
    if (replay && e->bass.on && e->sub.on && e->bass.length) { /* REVERSE / REPEAT: the line's step of then */
        uint8_t real = e->bass_pos;
        e->bass_pos = pp.bass;
        if (!e->bass.follow || pe.pulse_index == 0u)
            bass_tick(e, e->bass.follow ? (uint8_t)pe.total_pulses : 1u, pe.step_index);
        e->bass_pos = (!e->bass.follow || real_pe.pulse_index == 0u) ? (uint8_t)((real + 1u) % e->bass.length)
                                                                      : real; /* its own time underneath */
    } else if (e->bass.on && e->sub.on && (!e->bass.follow || pe.pulse_index == 0u))
        bass_tick(e, e->bass.follow ? (uint8_t)pe.total_pulses : 1u, pe.step_index);
    /* Dedicated sub is independent of main probability/gate and evaluates only at step start. */
    if (pe.pulse_index == 0u && e->sub.on && !e->bass.on) {
        bool sub_should = false;
        uint8_t x_tone = MELLO_SUB_TONE_ROOT;
        int8_t x_oct = 0;
        if (e->sub.mode == MELLO_SUB_MODE_STEP)
            sub_should = s->sub;
        else if (e->sub.mode == MELLO_SUB_MODE_EUCLID) {
            uint32_t total = old_loop_ticks ? old_loop_ticks : 1u;
            uint32_t loop_tick = old_loop_ticks ? (e->absolute_pulse % old_loop_ticks) : 0u;
            sub_should = euclid_hit_u32(loop_tick, total, e->sub.euclid_pulses, e->sub.euclid_rotate);
        } else if (e->sub.mode == MELLO_SUB_MODE_TM)
            sub_should = tm_gate_sub;
        else
            sub_should = matrix_sub;

        if (e->sub.x_on) {
            uint8_t len = sanitize_sub_x_length(e->sub.x_length);
            uint8_t xi = (uint8_t)(e->sub_x_pos % len);
            MelloSubXStep *xs = &e->sub.x_steps[xi];
            e->sub_x_active_index = (int8_t)xi;
            sub_should = xs->on;
            x_tone = xs->tone;
            x_oct = xs->octave;
            e->sub_x_pos = (uint8_t)((e->sub_x_pos + 1u) % len);
        } else
            e->sub_x_active_index = -1;

        if (e->sub.mode == MELLO_SUB_MODE_DRONE) {
            /* DRONE: one note, never ended by the sequence. A new pitch (KEY, OCT, a 5SEQ tone) glides the same
             * voice there; the 5SEQ's off steps do not stop it. SUB MUTE fades it in mello_audio, as any bass. */
            uint8_t sub_note = resolve_sub_midi(e, x_tone, x_oct);
            if (!e->drone_open || sub_note != e->drone_note) {
                if (e->drone_open) /* MIDI out ends the old note; the voice is attacked again at once */
                    emit_basic(e, MELLO_EVENT_SUB_NOTE_OFF, e->now_tick, e->drone_step, 0u, 1u, e->drone_note,
                               0u, 0u, false, true);
                emit_basic(e, MELLO_EVENT_SUB_NOTE_ON, e->now_tick, pe.step_index, 0u, pe.total_pulses,
                           sub_note, 114u, 0u, false, pass);
                e->drone_open = true, e->drone_note = sub_note, e->drone_step = pe.step_index;
            }
        } else if (sub_should &&
                   !e->perf.sub_mute) { /* SUB MUTE: no new bass notes (mello_audio fades the sounding one) */
            uint32_t raw, dur;
            uint8_t sub_note = resolve_sub_midi(e, x_tone, x_oct);
            pulse_ticks = mello_pulse_ticks(&e->project);
            raw = (pulse_ticks * (uint32_t)pe.total_pulses * (uint32_t)e->sub.gate_amount) / 1000u;
            dur = clamp_duration_ms_as_ticks(&e->project, raw, 30u, 2500u);
            emit_sub_pair(e, e->now_tick, e->now_tick + dur, pe.step_index, pe.pulse_index, pe.total_pulses,
                          sub_note, pass);
        }
    }

    /* DRUMS: a step every pulse (its own clock: the sequence's skips, long steps and REVERSE / REPEAT do not stop
     * it), or with FOLLOW at each of the sequence's steps. */
    pulse_ticks = mello_pulse_ticks(&e->project);
    if (replay && e->drums.on && e->drums.length) { /* REVERSE / REPEAT: the drums' step of then */
        uint8_t real = e->drum_pos;
        e->drum_pos = pp.drum;
        if (!e->drums.follow || pe.pulse_index == 0u) drums_tick(e, pulse_ticks);
        e->drum_pos = (!e->drums.follow || real_pe.pulse_index == 0u) ? (uint8_t)((real + 1u) % e->drums.length)
                                                                       : real; /* its own time underneath */
    } else if (!e->drums.follow || pe.pulse_index == 0u)
        drums_tick(e, pulse_ticks);
    e->swing_off = 0u; /* the loop's and the slots' events stay on the grid */
    next_tick = e->now_tick + pulse_ticks;
    e->absolute_pulse++;
    if (e->perf.mute_req && e->absolute_pulse % 4u == 0u) /* PERFORM's mutes come in on the beat */
        e->perf.mix_mute ^= e->perf.mute_req, e->perf.mute_req = 0u;
    did_wrap = old_loop_ticks > 0u && (e->absolute_pulse % old_loop_ticks) == 0u;
    if (did_wrap) {
        /* Evolution waits for the loop's one: the registers and the matrices change between the loop's last pulse
         * and its first, as much as they would have over the loop pulse by pulse. */
        if (e->tm.on && e->modr.tm_on_loop) {
            uint32_t n = old_loop_ticks < 256u ? old_loop_ticks : 256u;
            while (n--) tm_mutate(e);
        }
        dot_evolve(e);
        e->loop_count++;
        emit_basic(e, MELLO_EVENT_PATTERN_WRAP, next_tick, pe.step_index, pe.pulse_index, pe.total_pulses,
                   main_note, 0u, 0u, s->slide, pass);
    }

    /* Exact source collision order: queued manual slot at wrap wins and returns before
     * Loop Cycle, Slot Sequencer, X reset, and normal traversal advancement for this boundary. */
    if (did_wrap && e->queued_slot >= 0) {
        uint8_t slot = (uint8_t)e->queued_slot;
        e->queued_slot = -1;
        e->now_tick = next_tick;
        (void)apply_slot(e, slot, false, next_tick);
        return true;
    }

    if (e->loop_cycle.active) {
        uint8_t len = e->loop_cycle.length;
        MelloLoopCycleStep *cur;
        uint16_t hold;
        if (len < 2u) len = 2u;
        if (len > MELLO_MAX_LOOP_CYCLE_STEPS) len = MELLO_MAX_LOOP_CYCLE_STEPS;
        e->loop_cycle_pos = (uint8_t)(e->loop_cycle_pos % len);
        cur = &e->loop_cycle.steps[e->loop_cycle_pos];
        hold = clamp_hold(cur->hold ? cur->hold : 16u);
        e->loop_cycle_tick++;
        if (e->loop_cycle_tick >= hold) {
            e->loop_cycle_tick = 0u;
            e->loop_cycle_count++;
            e->loop_cycle_pos = (uint8_t)((e->loop_cycle_pos + 1u) % len);
            emit_state_event(e, MELLO_EVENT_LOOP_CYCLE_CHANGE, next_tick);
        } else if (did_wrap && e->loop_cycle.sync && hold <= old_loop_ticks)
            e->loop_cycle_tick = 0u;
    }

    if (e->slot_seq.active) {
        uint8_t len = sanitize_slot_seq_length(e->slot_seq.length);
        uint8_t pos = (uint8_t)(e->slot_seq_pos % len);
        uint16_t hold = e->slot_seq_hold;
        MelloSlotSeqStep *cfg = &e->slot_seq.steps[pos];
        uint16_t need = clamp_hold(cfg->hold ? cfg->hold : 16u);
        if (hold >= need) {
            pos = (uint8_t)((pos + 1u) % len);
            hold = 0u;
            cfg = &e->slot_seq.steps[pos];
            e->slot_seq_pos = pos;
            emit_state_event(e, MELLO_EVENT_SLOT_SEQ_CHANGE, next_tick);
        }
        if (hold == 0u && cfg->slot >= 0 && cfg->slot < (int8_t)e->visible_slot_count &&
            e->slots[(uint8_t)cfg->slot].valid)
            auto_slot = cfg->slot;
        hold++;
        e->slot_seq_pos = pos;
        e->slot_seq_hold = hold;
    }

    if (did_wrap && e->sub.x_on && e->sub.x_reset) {
        e->sub_x_pos = 0u;
        e->sub_x_active_index = 0;
    }

    e->now_tick = next_tick;
    if (e->project.master_len > 0u && did_wrap) {
        e->active_runtime_segment = 0u;
        e->segment_pulse_pos = 0u;
        e->segment_repeat_done = 0u;
        /* RESET is a reset for every part (DECISIONS.md "RESET resets everything"): the drums, the bass line and
         * the chord chain start over with the sequence, so a 12-step bass under a 16-step lead lines up again at
         * every RESET; only FREE lets each part run its own length. */
        e->drum_pos = 0u;
        e->bass_pos = 0u;
        e->bass_glide_in = false;
#if MELLO_FEATURE_CHORD_MACHINE
        { /* the chord chain at the resets that meet its own length: a four-bar progression over a 2 BAR reset
           * starts over every second reset, not halfway (feedback 2026-11-01: "the same progression all the way") */
            uint32_t total = 0u, k;
            for (k = 0u; k < e->chord.length; ++k) total += e->chord.steps[k].hold;
            if (!total || e->absolute_pulse % total == 0u || total <= old_loop_ticks) {
                if (e->chord_chain_pos || e->chord_step_count) chord_drift(e); /* a pass round: DRIFT */
                e->chord_chain_pos = 0u;
                e->chord_chain_hold = 0u;
                e->chord_step_count = 0u; /* the clock chain (NEXT STEPS) too, so a chord lands on the one */
                e->chord_grid_tick = 0u;
            }
        }
#endif
    } else
        advance_segment_traversal(e);
    set_current_entry(e);

    /* React source applies Slot Seq state changes after the current callback. Rebuild here,
     * after old-pattern traversal has advanced, so the next pulse sees the new slot while
     * retaining arranger/main-sequence phase. Loop Cycle config/runtime remains untouched. */
    if (auto_slot >= 0) (void)apply_slot(e, (uint8_t)auto_slot, true, next_tick);
    return true;
}

const char *mello_event_type_name(MelloEventType type)
{
    switch (type) {
    case MELLO_EVENT_PULSE: return "PULSE";
    case MELLO_EVENT_STEP_ENTER: return "STEP_ENTER";
    case MELLO_EVENT_SEGMENT_ENTER: return "SEG_ENTER";
    case MELLO_EVENT_PATTERN_WRAP: return "PAT_WRAP";
    case MELLO_EVENT_NOTE_ON: return "NOTE_ON";
    case MELLO_EVENT_NOTE_OFF: return "NOTE_OFF";
    case MELLO_EVENT_SUB_NOTE_ON: return "SUB_NOTE_ON";
    case MELLO_EVENT_SUB_NOTE_OFF: return "SUB_NOTE_OFF";
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_EVENT_CHORD_NOTE_ON: return "CHORD_ON";
    case MELLO_EVENT_CHORD_NOTE_OFF: return "CHORD_OFF";
#endif
    case MELLO_EVENT_SAMPLE_NOTE_ON: return "SAMPLE_ON";
    case MELLO_EVENT_DRUM_HIT: return "DRUM_HIT";
    case MELLO_EVENT_SAMPLE_NOTE_OFF: return "SAMPLE_OFF";
    case MELLO_EVENT_LOOP_CYCLE_CHANGE: return "LOOP_CYCLE";
    case MELLO_EVENT_SLOT_CHANGE: return "SLOT_CHANGE";
    case MELLO_EVENT_SLOT_SEQ_CHANGE: return "SLOT_SEQ";
    default: return "?";
    }
}

uint8_t mello_scale_tones(MelloScale scale, const int8_t **tones)
{
    const ScaleDef *sd = &SCALES[scale < MELLO_SCALE_COUNT ? scale : MELLO_SCALE_MINOR];
    if (tones) *tones = sd->v;
    return sd->n;
}

uint8_t mello_scale_quantize(MelloScale scale, uint8_t root, uint8_t midi)
{
    const ScaleDef *sd = &SCALES[scale < MELLO_SCALE_COUNT ? scale : MELLO_SCALE_MINOR];
    int32_t rel = ((int32_t)midi - (int32_t)root) % 12, best = 0, best_d = 99;
    uint8_t i;
    if (sd->n >= 12u) return midi;
    if (rel < 0) rel += 12;
    for (i = 0u; i < sd->n; ++i) { /* the nearest tone, down on a tie (d <= 6 goes down) */
        int32_t d = rel - sd->v[i], up = 12 - d, dist = d <= 6 ? d : up;
        if (d < 0) d += 12, up = 12 - d, dist = d <= 6 ? d : up;
        if (dist < best_d || (dist == best_d && d <= 6 && best > 0)) {
            best_d = dist;
            best = d <= 6 ? -d : up; /* the move: down by d, or up by 12 - d */
        }
    }
    return clamp_u8_i32((int32_t)midi + best, 12u, 108u);
}

const char *mello_scale_name(MelloScale scale)
{
    if (scale >= MELLO_SCALE_COUNT) scale = MELLO_SCALE_MINOR;
    return SCALES[scale].name;
}

void mello_engine_stress(MelloEngine *e)
{
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloBassConfig bass;
    MelloDrumConfig d;
    MelloModRouting modr;
    MelloDotMatrixConfig dot;
    uint8_t i;
    if (!e) return;
    e->project.bpm = 140u;
    e->project.seq_len = 16u;
    for (i = 0u; i < 16u; ++i) {
        MelloStep *st = &e->project.steps[i];
        st->pitch = (int8_t)(i % 7u);
        st->pulses = 1u;
        st->ratchet = (uint8_t)(i & 1u ? 4u : 2u);
        st->gate = MELLO_GATE_MULTI;
        st->accent = (i & 1u) != 0u;
        st->slide = (i % 3u) == 0u;
        st->sub = true;
        st->skip = false;
        st->probability = 65535u;
    }
    (void)mello_engine_rebuild_sequence(e);
    s = e->sound;
    s.unison = MELLO_UNISON_MAX;
    s.detune = 600u;
    s.noise_level = 300u;
    s.noise_fm = 300u;
    s.osc2_wave = MELLO_WAVE_SQUARE;
    s.osc2_mix = 500u;
    s.osc2_sync = true;
    s.fm_amount = 500u;
    s.saturation = 500u;
    s.filter_res = 500u;
    s.lfo_on = true;
    s.lfo_target = MELLO_LFO_FILTER;
    s.lfo_depth = 500u;
    s.menv_amount = 500;
    s.follower_sub_on = true;
    s.follower_sub_level = 500u;
    mello_engine_sound_set_config(e, &s);
    fx = e->fx;
    fx.dist_amount = 600u;
    fx.delay_wet = 500u;
    fx.delay_feedback = 600u;
    fx.delay_lowcut = 300u;
    fx.reverb_wet = 500u;
    fx.reverb_size = 800u;
    fx.reverb_decay_ms = 8000u;
    fx.delay_pingpong = true; /* PING: the second tap's read and filters */
    mello_engine_fx_set_config(e, &fx);
    sub = e->sub;
    sub.on = true;
    sub.drive = 500u;
    mello_engine_sub_set_config(e, &sub);
    bass = e->bass;
    bass.on = true;
    bass.res = 600u;
    bass.env = 600u;
    bass.accent = 600u;
    for (i = 0u; i < MELLO_BASS_STEPS; ++i) {
        bass.steps[i].on = true;
        bass.steps[i].accent = (i & 1u) != 0u;
        bass.steps[i].glide = (i & 2u) != 0u;
        bass.steps[i].octave = (int8_t)((i & 4u) ? 1 : 0);
    }
    mello_engine_bass_set_config(e, &bass);
    d = e->drums;
    d.on = true;
    d.feed = true;
    d.feed_sound = true;
    d.roll = 500u;
    d.drive = 600u;
    d.fold = 500u;
    d.space = 500u;
    for (i = 0u; i < MELLO_DRUM_VOICES; ++i) {
        uint8_t k;
        d.hits[i] = 0xFFFFu;
        d.accents[i] = 0x5555u;
        for (k = 0u; k < MELLO_DRUM_STEPS; ++k) {
            d.lock[i][k].ratchet = (uint8_t)((k & 3u) == 3u ? 4u : 1u);
            d.lock[i][k].crush = (uint8_t)(k * 6u);
        }
    }
    d.voice[MELLO_DRUM_GLITCH].tone = 1000u; /* STUTTER: the memory read and written */
    d.voice[MELLO_DRUM_GLITCH].crush = 300u;
    mello_engine_drums_set_config(e, &d);
#if MELLO_FEATURE_CHORD_MACHINE
    {
        MelloChordConfig ch = e->chord;
#ifdef MELLO_STRESS_NO_CHORD
        ch.on = false; /* a measuring build: the worst case without the chord part, for its own price */
#else
        ch.on = true;
#endif
        ch.mode = MELLO_CHORD_HOLD;
        ch.poly = true; /* the costlier voice */
        ch.trigger = MELLO_CHORD_TRIGGER_STEP;
        ch.voices = 4u;
        ch.level = 600u;
        ch.chorus = 800u;
        ch.res = 500u;
        ch.fenv = 500u;
        ch.space = 800u;
        ch.strum_ms = 20u;
        mello_engine_chord_set_config(e, &ch);
    }
#endif
    modr = e->modr;
    modr.dot_target[0] = MELLO_DEST_CUTOFF;
    modr.dot_target[1] = MELLO_DEST_PITCH;
    modr.tm_dest[0] = MELLO_DEST_NOISE; /* EVOLVE's sound routes */
    modr.tm_dest[1] = MELLO_DEST_FM;
    modr.tm_depth[0] = modr.tm_depth[1] = 500;
    mello_engine_mod_routing_set(e, &modr);
    {
        MelloTmConfig tm = e->tm; /* EVOLVE on, every route */
        tm.on = true;
        tm.mutation_prob = 250u;
        tm.routes = MELLO_TM_ROUTE_LENGTH | MELLO_TM_ROUTE_FILTER | MELLO_TM_ROUTE_OCTAVE | MELLO_TM_ROUTE_DECAY |
                    MELLO_TM_ROUTE_RETRIG;
        tm.depth_gate = tm.depth_filter = tm.depth_octave = tm.depth_decay = 500u;
        mello_engine_tm_set_config(e, &tm);
    }
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) {
        dot = e->dot[i];
        dot.cells = 0xFFFFu;
        dot.amount = 500u;
        mello_engine_dot_set_config(e, (MelloDotTarget)i, &dot);
    }
#ifdef MELLO_STRESS_GROOVE /* a listening build: the same, the sequence rolled with GROOVE (a seed's own) */
    mello_engine_set_randomize_mask(e, MELLO_RAND_DEFAULT);
    mello_engine_randomize_steps_intensity(e, MELLO_RANDOMIZE_GROOVE);
#endif
}
