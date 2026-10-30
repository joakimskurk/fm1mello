#ifndef MELLO_ENGINE_H
#define MELLO_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* On the FM-1, dinghy.c includes libc.c (memset, memcpy) before the Mello sources; the host tests use the C library. */
#ifndef __PI32V2__
void *(memset)(void *d, int c, size_t n);
void *(memcpy)(void *d, const void *s, size_t n);
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MELLO_MAX_STEPS            32u
#define MELLO_MAX_SEGMENTS         6u
#define MELLO_MAX_BASE_PULSES      (MELLO_MAX_STEPS * 8u)
#define MELLO_MAX_RUNTIME_PULSES   (MELLO_MAX_BASE_PULSES * 2u)
#define MELLO_EVENT_QUEUE_CAP      64u
#define MELLO_MAX_LOOP_CYCLE_STEPS 8u
#define MELLO_MAX_SLOTS            64u
#define MELLO_SLOT_NAME_CHARS      18u
#define MELLO_MAX_SLOT_SEQ_STEPS   64u
#define MELLO_DOT_MATRIX_COUNT     5u
#define MELLO_DOT_WIRE_COUNT       4u /* SEC_WORKING / SEC_SLOTS hold the first four; MOD 2 is in SEC_MOD */
#define MELLO_DOT_MOD_LANES        2u
#define MELLO_DOT_CELL_COUNT       16u
#ifndef MELLO_FEATURE_CHORD_MACHINE
#define MELLO_FEATURE_CHORD_MACHINE 1
#endif
#if MELLO_FEATURE_CHORD_MACHINE
#define MELLO_CHORD_MAX_VOICES    4u
#define MELLO_CHORD_MAX_STEPS     8u
#define MELLO_CHORD_PATTERN_STEPS 8u
#endif
#define MELLO_TM_MAX_STEPS      32u
#define MELLO_SUB_X_MAX_STEPS   5u
#define MELLO_SAMPLE_SLOT_COUNT 4u

/* Internal representation only. 16800 PPQN is divisible by MIDI 24 PPQN,
 * 1/16, ratchets 1..8, and source gate fractions 0.40/0.72/0.82/0.88. */
#define MELLO_PPQN            16800u
#define MELLO_QUARTER_TICKS   MELLO_PPQN
#define MELLO_SIXTEENTH_TICKS (MELLO_PPQN / 4u)

/* UI-normalized amounts use integer permille to keep target arithmetic deterministic.
 * Source controls have 0.01 resolution except TM mutation probability (0.005), so permille
 * preserves every source UI value exactly with integer-only arithmetic. */
#define MELLO_AMOUNT_MAX  1000u
#define MELLO_TM_PROB_MAX 500u

/* Randomize mask bits match the current HTML checkbox set. */
#define MELLO_RAND_PITCH   (1u << 0)
#define MELLO_RAND_PULSES  (1u << 1)
#define MELLO_RAND_RATCHET (1u << 2)
#define MELLO_RAND_GATE    (1u << 3)
#define MELLO_RAND_OCTAVE  (1u << 4)
#define MELLO_RAND_ACCENT  (1u << 5)
#define MELLO_RAND_SUB     (1u << 6)
#define MELLO_RAND_SLIDE   (1u << 7)
#define MELLO_RAND_SKIP    (1u << 8)
#define MELLO_RAND_PROB    (1u << 9)
#define MELLO_RAND_SAMPLE  (1u << 10)
/* Bit 11 stays reserved for CHORD so existing masks never change meaning. */
#define MELLO_RAND_CHORD   (1u << 11)
#define MELLO_RAND_WAVE    (1u << 12)
#define MELLO_RAND_TARGETS 13u /* MELLO_RAND_* bits, 0..12 */
/* What RNDMZR rolls on a new project: the first nine, the pattern; not the sound (WAVE, CHORD), samples or chance. */
#define MELLO_RAND_DEFAULT                                                                                   \
    (MELLO_RAND_PITCH | MELLO_RAND_PULSES | MELLO_RAND_RATCHET | MELLO_RAND_GATE | MELLO_RAND_OCTAVE |       \
     MELLO_RAND_ACCENT | MELLO_RAND_SUB | MELLO_RAND_SLIDE | MELLO_RAND_SKIP)
#define MELLO_RAND_BASE_ALL ((1u << 11) - 1u)
#if MELLO_FEATURE_CHORD_MACHINE
#define MELLO_RAND_ALL (MELLO_RAND_BASE_ALL | MELLO_RAND_CHORD | MELLO_RAND_WAVE)
#else
#define MELLO_RAND_ALL (MELLO_RAND_BASE_ALL | MELLO_RAND_WAVE)
#endif

#define MELLO_TM_ROUTE_GATE   (1u << 0)
#define MELLO_TM_ROUTE_FILTER (1u << 1)
#define MELLO_TM_ROUTE_OCTAVE (1u << 2)
#define MELLO_TM_ROUTE_DECAY  (1u << 3)

typedef enum {
    MELLO_GATE_SINGLE = 0,
    MELLO_GATE_HOLD = 1,
    MELLO_GATE_REST = 2,
    MELLO_GATE_MULTI = 3
} MelloGateMode;

typedef enum {
    MELLO_SCALE_CHROMATIC = 0,
    MELLO_SCALE_MINOR,
    MELLO_SCALE_DORIAN,
    MELLO_SCALE_PHRYGIAN,
    MELLO_SCALE_LYDIAN,
    MELLO_SCALE_MIXOLYDIAN,
    MELLO_SCALE_PENTATONIC,
    MELLO_SCALE_MINOR_PENTATONIC,
    MELLO_SCALE_DIMINISHED,
    MELLO_SCALE_WHOLE_TONE,
    MELLO_SCALE_AUGMENTED,
    MELLO_SCALE_MAJOR,
    MELLO_SCALE_COUNT
} MelloScale;

typedef enum { MELLO_SEG_FWD = 0, MELLO_SEG_REV = 1 } MelloSegmentDirection;

typedef enum { MELLO_SEG_LINEAR = 0, MELLO_SEG_PINGPONG = 1, MELLO_SEG_RANDOM = 2 } MelloSegmentMode;

typedef enum {
    MELLO_SUB_TONE_ROOT = 0,
    MELLO_SUB_TONE_SECOND,
    MELLO_SUB_TONE_THIRD,
    MELLO_SUB_TONE_FOURTH,
    MELLO_SUB_TONE_FIFTH,
    MELLO_SUB_TONE_SIXTH,
    MELLO_SUB_TONE_SEVENTH,
    MELLO_SUB_TONE_OCTAVE
} MelloSubTone;

/* MOD 1 is the HTML's FILTER matrix, with its target and shape now chosen (MelloModRouting); MOD 2 is new. */
typedef enum {
    MELLO_DOT_ACCENT = 0,
    MELLO_DOT_MOD1,
    MELLO_DOT_RATCHET,
    MELLO_DOT_SUB,
    MELLO_DOT_MOD2
} MelloDotTarget;

/* The order a matrix reads its cells in. The first four are the HTML's; the rest are Phase 5's (DECISIONS.md "DOT
 * orders"). REV (MelloDotMatrixConfig.reverse) runs any of them backwards. */
typedef enum {
    MELLO_DOT_HORIZONTAL = 0,
    MELLO_DOT_VERTICAL,
    MELLO_DOT_SNAKE,
    MELLO_DOT_RANDOM,
    MELLO_DOT_PINGPONG, /* there and back, the ends once: 2 x LEN - 2 long */
    MELLO_DOT_SPIRAL,   /* from the outside in, clockwise */
    MELLO_DOT_DIAGONAL, /* the diagonals, one more cell each, then fewer */
    MELLO_DOT_BOUNCE,   /* down the rows while bouncing side to side: 12 long, against the steps' 16 */
    MELLO_DOT_WALK      /* each read to a random neighbour */
} MelloDotMode;
#define MELLO_DOT_MODE_COUNT 9u
#define MELLO_DOT_ROUTE_MAX  32u

typedef enum { MELLO_TM_RATE_HALF = 0, MELLO_TM_RATE_ONE, MELLO_TM_RATE_DOUBLE } MelloTmRate;

typedef enum {
    MELLO_SUB_MODE_STEP = 0,
    MELLO_SUB_MODE_EUCLID,
    MELLO_SUB_MODE_MATRIX,
    MELLO_SUB_MODE_TM,
    MELLO_SUB_MODE_DRONE /* one note held from START to STOP; the pitch moves with KEY, OCT and the 5SEQ's tones */
} MelloSubMode;

typedef enum {
    MELLO_VOICE_LEAD = 0,
    MELLO_VOICE_SUB = 1,
#if MELLO_FEATURE_CHORD_MACHINE
    MELLO_VOICE_CHORD = 2
#endif
} MelloVoice;

/* Phase 9 sound model. Integer units mirror the browser controls without
 * introducing floating point into target code. */
typedef enum {
    MELLO_WAVE_SAW = 0,
    MELLO_WAVE_SQUARE,
    MELLO_WAVE_TRIANGLE,
    MELLO_WAVE_SINE,
    MELLO_WAVE_PULSE,
    MELLO_WAVE_OFF
} MelloWaveform;

/* The first four are the HTML's; the rest move what Phase 5 added (DECISIONS.md "Grit", "Virus layer"). */
typedef enum {
    MELLO_LFO_FILTER = 0,
    MELLO_LFO_PITCH,
    MELLO_LFO_AMP,
    MELLO_LFO_PWM,
    MELLO_LFO_NOISE,
    MELLO_LFO_NOISE_FM,
    MELLO_LFO_DETUNE,
    MELLO_LFO_RES,
    MELLO_LFO_OSC2,
    MELLO_LFO_FM,
    MELLO_LFO_SAT
} MelloLfoTarget;
#define MELLO_LFO_TARGET_COUNT 11u
/* The MOD envelope's destinations. */
typedef enum {
    MELLO_MENV_CUTOFF = 0,
    MELLO_MENV_PITCH,
    MELLO_MENV_OSC2,
    MELLO_MENV_FM,
    MELLO_MENV_NOISE,
    MELLO_MENV_RES
} MelloModEnvTarget;
#define MELLO_MENV_TARGET_COUNT 6u
/* Where DOT's MOD lanes and EVOLVE's sound routes go (Phase 5, DECISIONS.md "DOT and EVOLVE routes"). */
typedef enum {
    MELLO_DEST_OFF = 0,
    MELLO_DEST_CUTOFF,
    MELLO_DEST_RES,
    MELLO_DEST_DECAY,
    MELLO_DEST_PITCH,
    MELLO_DEST_OSC2,
    MELLO_DEST_FM,
    MELLO_DEST_NOISE,
    MELLO_DEST_NOISE_FM,
    MELLO_DEST_SAT,
    MELLO_DEST_DETUNE,
    MELLO_DEST_PWM
} MelloModDest;
#define MELLO_DEST_COUNT 12u
typedef enum { MELLO_FILTER_LP = 0, MELLO_FILTER_HP, MELLO_FILTER_BP, MELLO_FILTER_NOTCH } MelloFilterMode;
#define MELLO_FILTER_MODE_COUNT 4u
#define MELLO_UNISON_MAX        4u

typedef struct {
    MelloWaveform osc_wave;
    uint16_t pwm_permille;     /* source 0.05..0.95 => 50..950 */
    uint16_t attack_ms;        /* 1..500 */
    uint16_t decay_ms;         /* 5..1000 */
    uint16_t sustain_permille; /* 0..1000 */
    uint16_t release_ms;       /* 5..2000 */
    uint16_t slide_ms;         /* 0..300 */
    uint16_t filter_cut;       /* 0..1000 */
    uint16_t filter_res;       /* 0..1000 */
    bool lfo_on;
    uint16_t lfo_rate_millihz; /* 8..20000 */
    uint16_t lfo_depth;        /* 0..1000 */
    MelloLfoTarget lfo_target;
    bool follower_sub_on;
    uint16_t follower_sub_level; /* 0..1000 */
    int8_t follower_sub_octave;  /* -2..0 */
    MelloWaveform follower_sub_wave;
    uint16_t master_level; /* source masterVol 0..1 => 0..1000 */
    /* Grit, beyond the HTML (DECISIONS.md "Grit"); all off by default, so the default is the HTML's plain saw. */
    uint16_t noise_level; /* 0..1000: white noise mixed with the oscillator, before the filter */
    uint16_t noise_fm;    /* 0..1000: noise frequency-modulating the oscillator */
    uint8_t unison;       /* 1..MELLO_UNISON_MAX oscillators */
    uint16_t detune;      /* 0..1000: the unison spread, up to +-60 cents */
    /* The Virus layer (DECISIONS.md "Virus layer"); off by default (OSC 2 mix 0, FM 0, SAT 0, LP, MOD ENV 0). */
    MelloWaveform osc2_wave; /* OSC 2: free-running, or hard-synced to OSC 1; OFF by default */
    int8_t osc2_semi;        /* -24..+24 semitones from OSC 1 */
    int8_t osc2_fine;        /* -50..+50 cents */
    uint16_t osc2_mix;       /* 0..1000: 0 = OSC 1 only, 1000 = OSC 2 only */
    bool osc2_sync;          /* OSC 2 restarts with every OSC 1 cycle */
    uint16_t fm_amount;      /* 0..1000: OSC 1 frequency-modulates OSC 2 */
    uint16_t saturation;     /* 0..1000: soft clipping into the filter */
    MelloFilterMode filter_mode;
    int16_t menv_amount;     /* -1000..1000 */
    uint16_t menv_attack_ms; /* 0..2000 */
    uint16_t menv_decay_ms;  /* 5..5000 */
    MelloModEnvTarget menv_target;
    /* Tracking (DECISIONS.md "Velocity and key tracking"): how a note's velocity and pitch shape it. Velocity counts
     * from the HTML's plain step (91; an accent is 121), so plain steps sound as they did. */
    int16_t vel_cutoff; /* -1000..1000: an accent opens (or closes) the filter by up to 30% of its range */
    uint16_t vel_level; /* 0..2000: how much velocity changes the level; 1000 = the HTML's */
    uint16_t key_track; /* 0..1000: the cutoff follows the note from C4; 1000 = one to one */
    int16_t vel_menv;   /* -1000..1000: an accent's MOD envelope, up to twice (or none) */
} MelloSoundConfig;

typedef struct {
    uint16_t dist_amount;    /* 0..1000 */
    uint16_t dist_mix;       /* 0..1000 */
    uint16_t dist_character; /* 0..1000 */
    uint16_t dist_tone;      /* 0..1000 */
    uint16_t dist_level;     /* 200..1400 */
    uint16_t delay_wet;      /* 0..1000 */
    bool delay_sync;
    uint8_t delay_div;        /* 0=8n, 1=16n, 2=4n, 3=8t */
    uint16_t delay_time_ms;   /* 30..1500 */
    uint16_t delay_feedback;  /* 0..950 */
    uint16_t delay_tone;      /* 0..1000: HI CUT on the return */
    uint16_t delay_lowcut;    /* 0..1000: LO CUT on the return and in the feedback, 20 Hz .. 2 kHz; 0 off */
    uint16_t delay_level;     /* 0..1500 */
    uint16_t reverb_wet;      /* 0..1000 */
    uint16_t reverb_size;     /* 0..1000 */
    uint16_t reverb_decay_ms; /* 400..60000 */
    uint16_t reverb_pre_ms;   /* 0..120 */
    uint16_t reverb_tone;     /* 0..1000 */
    uint16_t reverb_level;    /* 0..3000 */
} MelloFxConfig;

typedef struct {
    int8_t coarse_semitones; /* source -24..+24 */
    int8_t fine_cents;       /* source -100..+100 */
    bool track;              /* true: C4-root pitch tracking */
    bool loop;
    uint16_t level;            /* source 0..1.5 => 0..1500 */
    uint16_t dist;             /* source 0..1 => 0..1000 */
    uint16_t trim_start;       /* normalized 0..1000 */
    uint16_t trim_end;         /* normalized 0..1000, > trim_start */
    uint16_t attack_ms;        /* 1..2000 */
    uint16_t decay_ms;         /* 5..2000 */
    uint16_t sustain_permille; /* 0..1000 */
    uint16_t release_ms;       /* 5..3000 */
} MelloSampleSlotConfig;

typedef struct {
    bool on;
    bool per_step;     /* source sampleSeq: true=step chooses A-D, false=fixed edit slot */
    uint8_t edit_slot; /* 0..3 */
    MelloSampleSlotConfig slots[MELLO_SAMPLE_SLOT_COUNT];
} MelloSampleConfig;

typedef struct {
    int8_t pitch;
    uint8_t pulses;
    uint8_t ratchet;
    MelloGateMode gate;
    int8_t octave;
    bool slide;
    bool skip;
    bool accent;
    bool sub;
    uint8_t sample_slot;  /* source samp: 0=off, 1..4=A..D */
    uint16_t probability; /* 0..65535 => 0..1 */
} MelloStep;

typedef struct {
    uint8_t seq_len;
    uint16_t bpm;
    uint8_t clock_div;
    uint8_t root_note;
    MelloScale scale;
    int8_t global_octave;
    uint8_t pitch_span_oct; /* source pitchSpanOct: 1..3 */
    MelloStep steps[MELLO_MAX_STEPS];

    uint8_t seg_count;
    uint8_t seg_splits[MELLO_MAX_SEGMENTS - 1u];
    uint8_t seg_repeats[MELLO_MAX_SEGMENTS];
    MelloSegmentDirection seg_direction[MELLO_MAX_SEGMENTS];
    MelloSegmentMode seg_mode[MELLO_MAX_SEGMENTS];
    bool seg_skip[MELLO_MAX_SEGMENTS];

    /* 0=FREE. Non-zero is musical length in 1/16-note units (16 = 1 bar of 4/4). */
    uint16_t master_len;
} MelloProject;

typedef struct {
    uint16_t hold;      /* processed-pulse count; runtime clamps 1..256 */
    int8_t root_offset; /* semitones, UI -12..12 */
    int8_t root_note;   /* -1=follow, otherwise pitch class 0..11 */
    int8_t scale_set;   /* -1=follow, otherwise MelloScale */
    int8_t octave;      /* UI -2..2 */
    int8_t pitch;       /* extra semitone offset, UI -12..12 */
    uint8_t sub_tone;   /* packed: bits 0..2 tone, bits 3..7 signed bass transpose (-12..12) */
} MelloLoopCycleStep;

typedef struct {
    bool active;
    bool sync;
    uint8_t length; /* source UI 2..8 */
    MelloLoopCycleStep steps[MELLO_MAX_LOOP_CYCLE_STEPS];
} MelloLoopCycleConfig;

typedef struct {
    int8_t slot;   /* -1 none, 0..63 saved slot */
    uint16_t hold; /* processed-pulse count; runtime clamps 1..256 */
} MelloSlotSeqStep;

typedef struct {
    bool active;
    uint8_t length; /* 2/4/8/16/32/64 in UI */
    MelloSlotSeqStep steps[MELLO_MAX_SLOT_SEQ_STEPS];
} MelloSlotSeqConfig;

typedef struct {
    uint16_t cells; /* row-major 4x4 bitset, bit index r*4+c */
    MelloDotMode mode;
    uint8_t length;  /* 1..16 */
    bool reset;      /* source reset=true wraps linear domain at seqLen */
    bool follow;     /* true: evaluate only on pulseIdx==0 */
    uint16_t amount; /* 0..1000 */
    bool reverse;    /* REV: the route backwards (saved in bit 7 of the mode byte) */
} MelloDotMatrixConfig;

/* HOME's performance keys, held (DECISIONS.md "Performance keys"). */
#define MELLO_PERF_HISTORY 64u
typedef struct {
    bool fill;          /* every step plays (chance and rests aside), ratchets doubled */
    bool sub_mute;      /* the bass under the lead (dedicated and follower sub) */
    bool reverse;       /* what was heard, backwards */
    bool repeat;        /* the last repeat_len pulses, round and round */
    uint8_t repeat_len; /* 1..16 pulses */
} MelloPerf;

/* DOT's MOD lanes, DOT's evolve and EVOLVE's sound routes and timing. Saved in SEC_MOD (not in the snapshot's own
 * wire format), with DOT's fifth matrix. */
typedef struct {
    uint8_t dot_target[MELLO_DOT_MOD_LANES];      /* MelloModDest, never OFF */
    uint16_t dot_attack_ms[MELLO_DOT_MOD_LANES];  /* 0..2000 */
    uint16_t dot_release_ms[MELLO_DOT_MOD_LANES]; /* 5..4000 */
    bool
        dot_gate[MELLO_DOT_MOD_LANES]; /* false HIT: each lit cell is a hit (AR); true GATE: lit cells hold */
    uint16_t dot_evolve[MELLO_DOT_MATRIX_COUNT]; /* 0..1000: lit cells move at the loop's one */
    uint8_t tm_dest[MELLO_DOT_MOD_LANES];        /* MelloModDest */
    int16_t tm_depth[MELLO_DOT_MOD_LANES];       /* -1000..1000 */
    bool tm_on_loop;                             /* EVOLVE mutates at the loop's one, not on every step */
    /* The randomizer's links (DECISIONS.md "Randomizer links"): rolled targets that follow each step's pitch (+)
     * or mirror it (-), MELLO_RAND_* bits of MELLO_RAND_LINKABLE; a target in both mirrors. */
    uint16_t rnd_follow;
    uint16_t rnd_mirror;
    /* How much each target is rolled (RANDOM's EDIT): the chance, 0..100%, that a roll changes a step's value (or,
     * for WAVE and CHORD, the sound's); by MELLO_RAND_* bit number. 100 by default: every step, as before. */
    uint8_t rnd_amount[MELLO_RAND_TARGETS];
} MelloModRouting;
#define MELLO_RAND_LINKABLE                                                                                  \
    (MELLO_RAND_PULSES | MELLO_RAND_RATCHET | MELLO_RAND_ACCENT | MELLO_RAND_SUB | MELLO_RAND_SLIDE |        \
     MELLO_RAND_SKIP | MELLO_RAND_PROB)

typedef struct {
    bool on;
    bool locked;
    uint16_t mutation_prob; /* 0..500 permille */
    uint8_t length;         /* 3..32 */
    MelloTmRate rate;
    int8_t offset;       /* -31..31 */
    uint8_t routes;      /* MELLO_TM_ROUTE_* bitmask */
    uint16_t depth_gate; /* 0..1000 */
    uint16_t depth_filter;
    uint16_t depth_octave;
    uint16_t depth_decay;
} MelloTmConfig;

typedef struct {
    bool on;
    uint8_t tone; /* MelloSubTone */
    int8_t octave;
} MelloSubXStep;

typedef struct {
    bool on;
    MelloSubMode mode;
    uint8_t euclid_pulses; /* 1..64, clamped to current loop length at evaluation */
    uint8_t euclid_rotate; /* 0..64 */
    int8_t octave;         /* source -2..+1 */
    uint16_t gate_amount;  /* source subGate 0.15..1.20 => 150..1200 permille */
    uint16_t level;        /* source subLevel 0..1 => 0..1000 */
    MelloWaveform wave;
    uint16_t decay_ms;   /* 30..700 */
    uint16_t release_ms; /* 30..1200 */
    uint16_t glide_ms;   /* 0..200 */
    uint16_t cut;        /* 0..1000 */
    uint16_t drive;      /* 0..1000 */
    bool x_on;
    bool x_reset;
    uint8_t x_length; /* source UI: 2, 3, or 5 */
    MelloSubXStep x_steps[MELLO_SUB_X_MAX_STEPS];
} MelloSubConfig;

/* DRUMS (2026-10-30): four synthesized voices on a 16-step pattern with its own clock. The voices are plain; what
 * makes them move is per-step: each hit's PITCH, DECAY, CRUSH and RATCH, and the rolls (ROLL, and RNDMZR with FEED). */
#define MELLO_DRUM_VOICES 4u
#define MELLO_DRUM_STEPS  16u
typedef enum { MELLO_DRUM_KICK = 0, MELLO_DRUM_SNARE, MELLO_DRUM_HAT, MELLO_DRUM_GLITCH } MelloDrumVoiceId;
typedef struct {
    int8_t pitch;    /* -24..24 semitones */
    int8_t decay;    /* -100..100: up to 4x shorter or longer */
    uint8_t crush;   /* 0..100: added to the voice's CRUSH */
    uint8_t ratchet; /* 1..8 hits in the step */
} MelloDrumLock;
typedef struct {
    uint16_t tune;   /* 0..1000 */
    uint16_t decay;  /* 0..1000 */
    uint16_t tone;   /* 0..1000: the voice's character (kick drive, snare snap, hat metal, glitch FM ratio) */
    uint16_t level;  /* 0..1000 */
    uint16_t chance; /* 0..1000: the chance a hit plays */
    uint16_t crush;  /* 0..1000: sample-rate and bit reduction */
} MelloDrumVoice;
typedef struct {
    bool on;
    bool follow;     /* step with the main sequence's steps (its skips and long steps stop it), not every pulse */
    bool feed;       /* RNDMZR rolls the drums too */
    uint8_t length;  /* 1..16 */
    uint16_t swing;  /* 0..1000: the off steps up to half a step late */
    uint16_t level;  /* 0..1000 */
    uint16_t space;  /* 0..1000: the send to the delay and reverb */
    uint16_t roll;   /* 0..1000: how much a roll changes */
    MelloDrumVoice voice[MELLO_DRUM_VOICES];
    uint16_t hits[MELLO_DRUM_VOICES];    /* bit = step */
    uint16_t accents[MELLO_DRUM_VOICES]; /* bit = step */
    MelloDrumLock lock[MELLO_DRUM_VOICES][MELLO_DRUM_STEPS];
} MelloDrumConfig;

#if MELLO_FEATURE_CHORD_MACHINE
typedef enum {
    MELLO_CHORD_TRIAD = 0,
    MELLO_CHORD_SEVENTH,
    MELLO_CHORD_SUS2,
    MELLO_CHORD_SUS4,
    MELLO_CHORD_POWER,
    MELLO_CHORD_SHAPE_COUNT
} MelloChordShape;

typedef enum {
    MELLO_CHORD_STAB = 0,
    MELLO_CHORD_HOLD,
    MELLO_CHORD_ARP_UP,
    MELLO_CHORD_ARP_DOWN,
    MELLO_CHORD_ARP_UPDOWN,
    MELLO_CHORD_PATTERN,
    MELLO_CHORD_MODE_COUNT
} MelloChordMode;

typedef enum { MELLO_CHORD_TRIGGER_STEP = 0, MELLO_CHORD_TRIGGER_GRID } MelloChordTrigger;

typedef struct {
    uint8_t degree; /* scale degree 0..11; wrapped to current scale */
    MelloChordShape shape;
    uint8_t inversion; /* 0..3 */
    int8_t octave;     /* -2..2 */
    uint8_t spread;    /* 0=close, 1..3 progressively wider */
    uint8_t hold;      /* number of chord triggers before chain advances */
} MelloChordStep;

typedef struct {
    bool on;
    MelloChordTrigger trigger;
    MelloChordMode mode;
    uint8_t length;
    uint8_t voices; /* 2..4 */
    uint16_t level; /* 0..1000 */
    MelloWaveform wave;
    uint16_t gate_amount; /* 100..1200 permille of current chord clock unit */
    uint16_t attack_ms;   /* 1..2000 */
    uint16_t release_ms;  /* 5..4000 */
    MelloChordStep steps[MELLO_CHORD_MAX_STEPS];
    uint16_t grid_cells;
    MelloDotMode grid_mode;
    uint8_t grid_length;
    bool grid_reset;
    bool grid_follow;
    uint16_t grid_amount;
    uint8_t pattern_length;
    uint8_t pattern_masks[MELLO_CHORD_PATTERN_STEPS]; /* low 4 bits choose chord voices */
    /* The voice (2026-10-30), as a Juno's: ADSR per voice, a resonant 12 dB low-pass with its own envelope, a
     * stereo chorus, its own send to the delay and reverb. */
    uint16_t cut;            /* 0..1000: 60 Hz .. 6.4 kHz */
    uint16_t res;            /* 0..1000 */
    uint16_t fenv;           /* 0..1000: the filter envelope opens the filter up to 48 semitones */
    uint16_t fenv_attack_ms; /* 1..2000 */
    uint16_t fenv_decay_ms;  /* 10..4000 */
    uint16_t decay_ms;       /* 10..4000, to SUSTAIN */
    uint16_t sustain;        /* 0..1000 */
    uint16_t chorus;         /* 0..1000: off .. a deep, wide ensemble */
    uint16_t space;          /* 0..1000: the send to the delay and reverb (1000 = the lead's) */
    uint8_t strum_ms;        /* 0..60: each voice of a chord this much after the one under it */
    bool link;               /* HOME's CUTOFF (the lead's) moves the chord filter as far */
    bool voice_lead;         /* a new chord in the inversion nearest the last one */
} MelloChordConfig;
#endif

typedef enum {
    MELLO_RANDOMIZE_LOW = 0,
    MELLO_RANDOMIZE_MED = 1,
    MELLO_RANDOMIZE_HIGH = 2
} MelloRandomizeIntensity;

typedef struct {
    uint16_t mask; /* MELLO_RAND_* */
} MelloRandomizeConfig;

/* Compact slot copy. Explicit bytes rather than C bitfields keeps the representation bounded. */
typedef struct {
    int8_t pitch;
    uint8_t pulse_ratchet; /* bits 0..2 pulses-1, bits 3..5 ratchet-1 */
    uint8_t flags;         /* gate[1:0], octave+2[4:2], slide bit5, skip bit6, accent bit7 */
    uint8_t extras;        /* sub bit0, sample slot bits1..3 */
    uint8_t probability_lo;
    uint8_t probability_hi;
} MelloStoredStep;

typedef struct {
    uint8_t seq_len;
    uint8_t bpm_lo;
    uint8_t bpm_hi;
    uint8_t clock_div;
    uint8_t root_note;
    uint8_t scale;
    int8_t global_octave;
    uint8_t pitch_span_oct;
    MelloStoredStep steps[MELLO_MAX_STEPS];
    uint8_t seg_count;
    uint8_t seg_splits[MELLO_MAX_SEGMENTS - 1u];
    uint8_t seg_repeats[MELLO_MAX_SEGMENTS];
    uint8_t seg_direction[MELLO_MAX_SEGMENTS];
    uint8_t seg_mode[MELLO_MAX_SEGMENTS];
    uint8_t seg_skip_mask;
    uint8_t master_len_lo;
    uint8_t master_len_hi;
} MelloStoredProject;

typedef struct {
    bool valid;
    char name[MELLO_SLOT_NAME_CHARS + 1u];
    MelloStoredProject project;
    MelloLoopCycleConfig loop_cycle;
    MelloDotMatrixConfig dot[MELLO_DOT_MATRIX_COUNT];
    MelloTmConfig tm;
    MelloModRouting modr;
    uint8_t tm_bool_len;
    uint8_t
        tm_bool_reg[MELLO_TM_MAX_STEPS]; /* snapshot saves current bool-array length; load expands to 32 */
    MelloSubConfig sub;
    MelloRandomizeConfig randomize;
    MelloSoundConfig sound;
    MelloSampleConfig sample;
    MelloFxConfig fx; /* working snapshots persist FX; saved-slot loads intentionally ignore it */
} MelloPatternSlot;

typedef enum {
    MELLO_EVENT_PULSE = 0,
    MELLO_EVENT_STEP_ENTER,
    MELLO_EVENT_SEGMENT_ENTER,
    MELLO_EVENT_PATTERN_WRAP,
    MELLO_EVENT_NOTE_ON,
    MELLO_EVENT_NOTE_OFF,
    MELLO_EVENT_SUB_NOTE_ON,
    MELLO_EVENT_SUB_NOTE_OFF,
#if MELLO_FEATURE_CHORD_MACHINE
    MELLO_EVENT_CHORD_NOTE_ON,
    MELLO_EVENT_CHORD_NOTE_OFF,
#endif
    MELLO_EVENT_SAMPLE_NOTE_ON,
    MELLO_EVENT_SAMPLE_NOTE_OFF,
    MELLO_EVENT_LOOP_CYCLE_CHANGE,
    MELLO_EVENT_SLOT_CHANGE,
    MELLO_EVENT_SLOT_SEQ_CHANGE,
    MELLO_EVENT_DRUM_HIT /* sample_slot: the voice; note: 64 + PITCH; velocity; drum_decay, drum_crush */
} MelloEventType;

typedef struct {
    uint32_t tick;
    MelloEventType type;
    uint8_t step_index;
    uint8_t pulse_index;
    uint8_t total_pulses;
    uint8_t segment_index;
    uint8_t segment_repeat;
    uint8_t note;
    uint8_t velocity;
    uint8_t ratchet_index;
    bool slide;
    bool probability_pass;
    int8_t slot_index;
    uint8_t loop_cycle_index;
    uint8_t slot_seq_index;
    MelloVoice voice;
    uint8_t sample_slot; /* 0..3 for SAMPLE events */
    MelloGateMode effective_gate;
    uint8_t effective_ratchet;
    bool effective_accent;
    int8_t tm_index;
    int8_t tm_octave_mod;
    int16_t tm_filter_mod_q15; /* normalized roughly -0.25..+0.25 */
    int16_t tm_decay_mod_q15;  /* normalized roughly -0.10..+0.10 */
    uint16_t
        dot_mod[MELLO_DOT_MOD_LANES]; /* MOD 1 / MOD 2: 0..1000 lit, 0 unlit, MELLO_DOT_NOT_READ not read */
    int16_t tm_value;                 /* EVOLVE's register value, centred (+-32767); 0 while off */
    int8_t dot_cell[MELLO_DOT_MATRIX_COUNT];
    int8_t drum_decay;  /* DRUM_HIT: the step's DECAY lock */
    uint8_t drum_crush; /* DRUM_HIT: the step's CRUSH lock */
} MelloEvent;
#define MELLO_DOT_NOT_READ 0xFFFFu

typedef struct {
    uint32_t state;
} MelloPrng;

typedef struct {
    uint8_t step_index;
    uint8_t pulse_index;
    uint8_t total_pulses;
} MelloPulseEntry;

typedef struct {
    uint16_t offset;
    uint16_t length;
    uint8_t project_segment;
} MelloRuntimeSegment;

typedef struct {
    MelloDotMode mode;
    uint8_t length;
    int32_t cycle;
    uint8_t order[MELLO_DOT_ROUTE_MAX]; /* the route, period long */
    uint8_t period;
    uint8_t walk; /* WALK: the cell it is on */
} MelloDotRuntime;

typedef struct {
    MelloGateMode effective_gate;
    uint8_t effective_ratchet;
    bool effective_accent;
    bool matrix_sub;
    uint16_t dot_mod[MELLO_DOT_MOD_LANES];
    int8_t tm_index;
    bool tm_bool;
    uint16_t tm_float;
    int8_t tm_octave_mod;
    int16_t tm_filter_mod_q15;
    int16_t tm_decay_mod_q15;
    int8_t dot_cell[MELLO_DOT_MATRIX_COUNT];
} MelloPulseModState;

typedef struct {
    MelloProject project;
    bool playing;
    bool has_active_step;
    uint8_t step_index;
    uint8_t pulse_index;
    uint32_t now_tick;
    uint32_t loop_count;
    uint32_t absolute_pulse;
    MelloPrng rng;     /* Phase 2/3 traversal + per-pulse probability stream */
    MelloPrng gen_rng; /* Phase 6 generative stream; isolated to keep earlier traces stable */

    MelloPulseEntry runtime_pulses[MELLO_MAX_RUNTIME_PULSES];
    MelloPulseEntry build_scratch[MELLO_MAX_BASE_PULSES];
    MelloRuntimeSegment runtime_segments[MELLO_MAX_SEGMENTS];
    uint8_t runtime_segment_count;
    uint16_t runtime_pulse_count;
    uint8_t active_runtime_segment;
    uint16_t segment_pulse_pos;
    uint8_t segment_repeat_done;
    uint32_t loop_pulse_length;
    bool runtime_built;

    /* Phase 5+ long-form hierarchy retained in Phase 6. */
    MelloLoopCycleConfig loop_cycle;
    uint8_t loop_cycle_pos;
    uint16_t loop_cycle_tick;
    uint32_t loop_cycle_count;

    MelloPatternSlot slots[MELLO_MAX_SLOTS];
    uint8_t visible_slot_count;
    int8_t active_slot;
    int8_t queued_slot;

    MelloSlotSeqConfig slot_seq;
    uint8_t slot_seq_pos;
    uint16_t slot_seq_hold;

    /* Phase 6 generative layers. */
    MelloDotMatrixConfig dot[MELLO_DOT_MATRIX_COUNT];
    MelloDotRuntime dot_runtime[MELLO_DOT_MATRIX_COUNT];
    uint32_t dot_tick;
    uint32_t dot_follow;
    int8_t dot_active_cell[MELLO_DOT_MATRIX_COUNT];

    MelloTmConfig tm;
    MelloModRouting modr;
    uint8_t tm_bool_reg[MELLO_TM_MAX_STEPS];
    uint16_t tm_float_reg[MELLO_TM_MAX_STEPS];
    uint8_t tm_bool_len;
    uint8_t tm_float_len;
    int8_t tm_active_index;

    MelloSubConfig sub;
    uint8_t sub_x_pos;
    int8_t sub_x_active_index;
#if MELLO_FEATURE_CHORD_MACHINE
    MelloChordConfig chord;
    MelloDotRuntime chord_grid_runtime;
    uint32_t chord_grid_tick;
    uint32_t chord_grid_follow;
    int8_t chord_grid_active_cell;
    uint8_t chord_chain_pos;
    uint8_t chord_chain_hold;
    uint8_t chord_arp_pos;
    int8_t chord_arp_dir;
    uint8_t chord_pattern_pos;
    uint8_t chord_last_notes[MELLO_CHORD_MAX_VOICES];
    uint8_t chord_last_count;
    uint8_t chord_voiced[MELLO_CHORD_MAX_VOICES]; /* the chord sounding, as voiced (voice leading) */
    uint8_t chord_voiced_n;
    uint8_t chord_voiced_slot; /* which chain chord it is (0xFF: none yet) */
    uint8_t chord_prev[MELLO_CHORD_MAX_VOICES]; /* the chord before it, as voiced: voice leading's reference */
    uint8_t chord_prev_n;
#endif

    MelloRandomizeConfig randomize;
    MelloSoundConfig sound;
    MelloSampleConfig sample;
    MelloFxConfig fx;
    MelloPulseModState last_mod;

    /* Undo for RANDOM and CLEAR: what the pattern was before the last one (mello_engine_undo_swap). */
    MelloProject undo_project;
    MelloSoundConfig undo_sound;
#if MELLO_FEATURE_CHORD_MACHINE
    MelloChordConfig undo_chord;
#endif
    MelloDrumConfig undo_drums;
    bool undo_valid;
    bool undo_is_redo;  /* the last press was UNDO: the next puts the roll back */
    uint32_t undo_hash; /* the state as UNDO left it: an edit since means there is nothing to redo */

    /* HOME's performance keys (mello_engine_set_perf): runtime only, never saved or undone. The sequence keeps
     * its own place all along; REVERSE and REPEAT play from perf_buf instead, so letting go lands in time. */
    MelloPerf perf;
    MelloPulseEntry perf_hist[MELLO_PERF_HISTORY]; /* the last pulses of the sequence, oldest overwritten */
    uint8_t perf_hist_w;
    uint8_t perf_hist_n;
    MelloPulseEntry perf_buf[MELLO_PERF_HISTORY]; /* what REVERSE / REPEAT play, in order, round and round */
    uint8_t perf_buf_n;
    uint8_t perf_buf_pos;
    uint8_t perf_mode; /* 0 the sequence, 1 REVERSE, 2 REPEAT */
    uint8_t perf_mode_len;
    bool perf_restart; /* to step 1 at the next pulse */
    /* A HOLD note sounding (its ON sent, its OFF to come at the step's last pulse), so that REVERSE / REPEAT, which
     * may never play that pulse, can end it. */
    bool hold_open;
    uint8_t hold_note;
    uint8_t hold_step;
    uint8_t hold_total;
    /* The DRONE sub's note sounding (its ON sent, its OFF at STOP, or when the mode or SUB changes). */
    MelloDrumConfig drums;
    MelloPrng drum_rng; /* chance and rolls: the other streams stay as they were */
    uint8_t drum_pos;   /* the drum step playing next */
    int8_t drum_step;   /* the one playing, -1 stopped (the DRUMS page) */
    bool drone_open;
    uint8_t drone_note;
    uint8_t drone_step;

    MelloEvent queue[MELLO_EVENT_QUEUE_CAP];
    uint8_t q_head;
    uint8_t q_tail;
    uint8_t q_count;
    bool queue_overflow;
} MelloEngine;

void mello_project_defaults(MelloProject *project);
/* Segments, as the SEGMENT page edits them. Each works on the segments as they play (the splits clamped to the
 * sequence) and writes them back in order. bounds[] gets each segment's first step and, last, seq_len. */
uint8_t mello_segment_bounds(const MelloProject *project, uint8_t bounds[MELLO_MAX_SEGMENTS + 1u]);
/* A segment starts at step (it is cut in two, the new half like the old), or, where one already starts, it joins the
 * one before. Returns the segment that now holds step, or -1 when nothing changed (step 1, a seventh segment). */
int8_t mello_segment_toggle_start(MelloProject *project, uint8_t step);
/* The sequence in seg_count segments as near equal as they go (16 in 3: 5, 5, 6). */
void mello_segment_even(MelloProject *project);
/* Segment seg is len steps long; the ones after it keep their lengths as far as the sequence lets them (the last gives
 * way). The last segment's length moves its start. */
void mello_segment_set_length(MelloProject *project, uint8_t seg, uint8_t len);
/* seq_len went from old_len to what it is now: the segments keep their proportions. */
void mello_segment_rescale(MelloProject *project, uint8_t old_len);
void mello_loop_cycle_defaults(MelloLoopCycleConfig *config);
uint8_t mello_loop_cycle_sub_tone(const MelloLoopCycleStep *step);
int8_t mello_loop_cycle_sub_transpose(const MelloLoopCycleStep *step);
void mello_loop_cycle_set_sub(MelloLoopCycleStep *step, uint8_t tone, int8_t transpose);
void mello_slot_seq_defaults(MelloSlotSeqConfig *config);
void mello_dot_matrix_defaults(MelloDotMatrixConfig *config);
void mello_tm_defaults(MelloTmConfig *config);
void mello_mod_routing_defaults(MelloModRouting *config);
/* DOT's defaults for matrix id (the MOD lanes' amount is their depth: 24%, the HTML's filter dot). */
void mello_dot_defaults_for(MelloDotMatrixConfig *config, uint8_t id);
void mello_sub_defaults(MelloSubConfig *config);
void mello_drum_defaults(MelloDrumConfig *config);
void mello_engine_drums_set_config(MelloEngine *engine, const MelloDrumConfig *config);
/* A roll of the drums: hits, accents and per-step locks, as much as roll says, the kick kept on the one. */
void mello_engine_drums_roll(MelloEngine *engine);
#if MELLO_FEATURE_CHORD_MACHINE
void mello_chord_defaults(MelloChordConfig *config);
#endif
void mello_randomize_defaults(MelloRandomizeConfig *config);
void mello_sound_defaults(MelloSoundConfig *config);
void mello_sample_defaults(MelloSampleConfig *config);
void mello_fx_defaults(MelloFxConfig *config);
void mello_stored_project_encode(MelloStoredProject *dst, const MelloProject *src);
void mello_stored_project_decode(MelloProject *dst, const MelloStoredProject *src);
void mello_engine_init(MelloEngine *engine, const MelloProject *project, uint32_t seed);
void mello_engine_start(MelloEngine *engine);
void mello_engine_stop(MelloEngine *engine);
void mello_engine_reset_transport(MelloEngine *engine);
bool mello_engine_rebuild_sequence(MelloEngine *engine);

/* Process exactly one current-source processed pulse entry. */
bool mello_engine_advance_pulse(MelloEngine *engine);

/* Loop Cycle controls. Setters match source reset/no-reset distinctions. */
void mello_engine_loop_cycle_set_active(MelloEngine *engine, bool active);
void mello_engine_loop_cycle_set_sync(MelloEngine *engine, bool sync);
void mello_engine_loop_cycle_set_length(MelloEngine *engine, uint8_t length);
void mello_engine_loop_cycle_set_step(MelloEngine *engine, uint8_t index, const MelloLoopCycleStep *step);
void mello_engine_loop_cycle_select(MelloEngine *engine, uint8_t index);
void mello_engine_loop_cycle_reset_position(MelloEngine *engine);

/* Saved-slot RAM model. Manual request toggles queue while playing, immediate-loads while stopped. */
void mello_engine_capture_working(const MelloEngine *engine, MelloPatternSlot *out);
bool mello_engine_restore_working(MelloEngine *engine, const MelloPatternSlot *snapshot);
bool mello_engine_import_slot(MelloEngine *engine, uint8_t index, const MelloPatternSlot *slot);
void mello_engine_clear_pattern(MelloEngine *engine);
void mello_engine_clear_slots(MelloEngine *engine);
bool mello_engine_save_slot(MelloEngine *engine, uint8_t index, const char *name);
bool mello_engine_request_slot(MelloEngine *engine, uint8_t index);
bool mello_engine_slot_valid(const MelloEngine *engine, uint8_t index);
void mello_engine_set_visible_slot_count(MelloEngine *engine, uint8_t count);

/* Slot Sequencer controls. Runtime position persists across STOP/START like source. */
void mello_engine_slot_seq_set_active(MelloEngine *engine, bool active);
void mello_engine_slot_seq_set_length(MelloEngine *engine, uint8_t length);
void mello_engine_slot_seq_set_step(MelloEngine *engine, uint8_t index, int8_t slot, uint16_t hold);
void mello_engine_slot_seq_reset_position(MelloEngine *engine);

/* Dot Matrix: changing config resets both matrix counters/runtime, like the React cfg effect.
 * Changing cells alone does not reset traversal. */
void mello_engine_dot_set_config(MelloEngine *engine, MelloDotTarget target,
                                 const MelloDotMatrixConfig *config);
void mello_engine_dot_set_cells(MelloEngine *engine, MelloDotTarget target, uint16_t cells);
void mello_engine_dot_randomize_cells(MelloEngine *engine, MelloDotTarget target);
void mello_engine_dot_reset_runtime(MelloEngine *engine);
/* A route as the engine reads it (out: cells in order); returns its length. RANDOM and WALK have none fixed. */
uint8_t mello_dot_route(MelloDotMode mode, uint8_t length, uint8_t out[MELLO_DOT_ROUTE_MAX]);

/* Turing / Evolve controls. */
void mello_engine_tm_set_config(MelloEngine *engine, const MelloTmConfig *config);
void mello_engine_mod_routing_set(MelloEngine *engine, const MelloModRouting *config);
/* The performance keys as held now; call it whenever they change (and harmlessly when they do not). */
void mello_engine_set_perf(MelloEngine *engine, const MelloPerf *perf);
/* RESTART: the sequence from its first step at the next pulse (16th), the loop's one with it. */
void mello_engine_perf_restart(MelloEngine *engine);
void mello_engine_tm_set_length(MelloEngine *engine, uint8_t length);
void mello_engine_tm_reroll(MelloEngine *engine);
void mello_engine_tm_set_bool_register(MelloEngine *engine, const uint8_t *values, uint8_t count);

/* Dedicated sub-trigger model + X Sub override. */
void mello_engine_sub_set_config(MelloEngine *engine, const MelloSubConfig *config);
void mello_engine_sub_reset_x(MelloEngine *engine);
#if MELLO_FEATURE_CHORD_MACHINE
void mello_engine_chord_set_config(MelloEngine *engine, const MelloChordConfig *config);
void mello_engine_chord_reset(MelloEngine *engine);
uint8_t mello_engine_resolve_chord_notes(const MelloEngine *engine, uint8_t chord_step,
                                         uint8_t out_notes[MELLO_CHORD_MAX_VOICES]);
/* A chain chord's name in the key and scale now: "Am", "F", "Dm7", "CM7", "Bo" (diminished), "C+" (augmented),
 * "Csus4", "E5". out holds 8. */
void mello_engine_chord_name(const MelloEngine *engine, uint8_t chord_step, char out[8]);
#endif
void mello_engine_sound_set_config(MelloEngine *engine, const MelloSoundConfig *config);
void mello_engine_sample_set_config(MelloEngine *engine, const MelloSampleConfig *config);
void mello_engine_fx_set_config(MelloEngine *engine, const MelloFxConfig *config);

/* Source RANDOM button. Mutates all 32 backing steps, conditional on mask. */
void mello_engine_set_randomize_mask(MelloEngine *engine, uint16_t mask);
void mello_engine_randomize_steps(MelloEngine *engine); /* exact MED compatibility path */
void mello_engine_randomize_steps_intensity(MelloEngine *engine, MelloRandomizeIntensity intensity);
/* Undo: keep what a destructive edit is about to replace (randomize_steps_intensity does so itself), and swap it
 * back in. A second swap redoes. Returns false when there is nothing to undo. */
void mello_engine_undo_snapshot(MelloEngine *engine);
bool mello_engine_undo_swap(MelloEngine *engine);
/* What UNDO's key does now: MELLO_UNDO_NONE, MELLO_UNDO_UNDO, or MELLO_UNDO_REDO. After an UNDO, an edit leaves nothing
 * to redo, and nothing to undo either (the state before the roll is now the one edited). */
enum { MELLO_UNDO_NONE = 0, MELLO_UNDO_UNDO = 1, MELLO_UNDO_REDO = 2 };
uint8_t mello_engine_undo_state(MelloEngine *engine);

bool mello_event_pop(MelloEngine *engine, MelloEvent *out);
void mello_event_clear(MelloEngine *engine);
bool mello_event_overflowed(const MelloEngine *engine);

uint32_t mello_prng_next_u32(MelloPrng *rng);
void mello_prng_seed(MelloPrng *rng, uint32_t seed);

uint32_t mello_pulse_ticks(const MelloProject *project);
uint8_t mello_resolve_step_midi(const MelloProject *project, const MelloStep *step);
const char *mello_event_type_name(MelloEventType type);
const char *mello_scale_name(MelloScale scale);

#ifdef __cplusplus
}
#endif
#endif
