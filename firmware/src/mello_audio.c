/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
#include "mello_audio.h"

#define Q15_ONE 32767
#define Q24_ONE (32767 << 8)
/* The bass bus (ROADMAP E11): the level the lead's path (x 0.58, the distortion's x 1/3 at AMT 0, its LEVEL 90%, the
 * lead's filter at its default CUTOFF) gave the bass, measured so a project sounds as it did */
#define BASS_BUS_Q12               713
#define MELLO_SAMPLE_DECODE_BUDGET 20u /* IMA blocks per render call (256 frames): 5 per sample voice */
#define AUDIO_US_BASE              (1000000u / MELLO_AUDIO_FS)
#define AUDIO_US_REM               (1000000u % MELLO_AUDIO_FS)

static const uint32_t MIDI_INC[128] = {
    796254,     843601,     893765,    946911,    1003217,   1062871,   1126073,   1193033,    1263974,
    1339134,    1418763,    1503127,   1592507,   1687203,   1787529,   1893821,   2006434,    2125742,
    2252146,    2386065,    2527948,   2678268,   2837526,   3006254,   3185015,   3374406,    3575058,
    3787642,    4012867,    4251485,   4504291,   4772130,   5055896,   5356535,   5675051,    6012507,
    6370030,    6748811,    7150117,   7575285,   8025735,   8502970,   9008582,   9544261,    10111792,
    10713070,   11350103,   12025015,  12740059,  13497623,  14300233,  15150569,  16051469,   17005939,
    18017165,   19088521,   20223584,  21426141,  22700205,  24050030,  25480119,  26995246,   28600467,
    30301139,   32102938,   34011878,  36034330,  38177043,  40447168,  42852281,  45400411,   48100060,
    50960238,   53990491,   57200933,  60602278,  64205876,  68023757,  72068660,  76354085,   80894335,
    85704563,   90800821,   96200119,  101920476, 107980983, 114401866, 121204555, 128411753,  136047513,
    144137319,  152708170,  161788671, 171409126, 181601643, 192400238, 203840952, 215961966,  228803732,
    242409110,  256823506,  272095026, 288274639, 305416341, 323577341, 342818251, 363203285,  384800477,
    407681904,  431923931,  457607465, 484818220, 513647012, 544190053, 576549277, 610832681,  647154683,
    685636503,  726406571,  769600953, 815363807, 863847862, 915214929, 969636441, 1027294024, 1088380105,
    1153098554, 1221665363,
};

static const int16_t SINE_Q15[256] = {
    0,      804,    1608,   2410,   3212,   4011,   4808,   5602,   6393,   7179,   7962,   8739,   9512,
    10278,  11039,  11793,  12539,  13279,  14010,  14732,  15446,  16151,  16846,  17530,  18204,  18868,
    19519,  20159,  20787,  21403,  22005,  22594,  23170,  23731,  24279,  24811,  25329,  25832,  26319,
    26790,  27245,  27683,  28105,  28510,  28898,  29268,  29621,  29956,  30273,  30571,  30852,  31113,
    31356,  31580,  31785,  31971,  32137,  32285,  32412,  32521,  32609,  32678,  32728,  32757,  32767,
    32757,  32728,  32678,  32609,  32521,  32412,  32285,  32137,  31971,  31785,  31580,  31356,  31113,
    30852,  30571,  30273,  29956,  29621,  29268,  28898,  28510,  28105,  27683,  27245,  26790,  26319,
    25832,  25329,  24811,  24279,  23731,  23170,  22594,  22005,  21403,  20787,  20159,  19519,  18868,
    18204,  17530,  16846,  16151,  15446,  14732,  14010,  13279,  12539,  11793,  11039,  10278,  9512,
    8739,   7962,   7179,   6393,   5602,   4808,   4011,   3212,   2410,   1608,   804,    0,      -804,
    -1608,  -2410,  -3212,  -4011,  -4808,  -5602,  -6393,  -7179,  -7962,  -8739,  -9512,  -10278, -11039,
    -11793, -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530, -18204, -18868, -19519, -20159,
    -20787, -21403, -22005, -22594, -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790, -27245,
    -27683, -28105, -28510, -28898, -29268, -29621, -29956, -30273, -30571, -30852, -31113, -31356, -31580,
    -31785, -31971, -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757, -32767, -32757, -32728,
    -32678, -32609, -32521, -32412, -32285, -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
    -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683, -27245, -26790, -26319, -25832, -25329,
    -24811, -24279, -23731, -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868, -18204, -17530,
    -16846, -16151, -15446, -14732, -14010, -13279, -12539, -11793, -11039, -10278, -9512,  -8739,  -7962,
    -7179,  -6393,  -5602,  -4808,  -4011,  -3212,  -2410,  -1608,  -804,
};

static const uint32_t SAMPLE_SEMI_Q16[121] = {
    2048,    2170,    2299,    2435,    2580,    2734,    2896,    3069,    3251,    3444,    3649,
    3866,    4096,    4340,    4598,    4871,    5161,    5468,    5793,    6137,    6502,    6889,
    7298,    7732,    8192,    8679,    9195,    9742,    10321,   10935,   11585,   12274,   13004,
    13777,   14596,   15464,   16384,   17358,   18390,   19484,   20643,   21870,   23170,   24548,
    26008,   27554,   29193,   30929,   32768,   34716,   36781,   38968,   41285,   43740,   46341,
    49097,   52016,   55109,   58386,   61858,   65536,   69433,   73562,   77936,   82570,   87480,
    92682,   98193,   104032,  110218,  116772,  123715,  131072,  138866,  147123,  155872,  165140,
    174960,  185364,  196386,  208064,  220436,  233544,  247431,  262144,  277732,  294247,  311744,
    330281,  349920,  370728,  392772,  416128,  440872,  467088,  494862,  524288,  555464,  588493,
    623487,  660561,  699841,  741455,  785544,  832255,  881744,  934175,  989724,  1048576, 1110928,
    1176987, 1246974, 1321123, 1399681, 1482910, 1571089, 1664511, 1763488, 1868350, 1979448, 2097152,
};

static const uint16_t SAMPLE_FINE_Q15[201] = {
    30929, 30947, 30965, 30983, 31000, 31018, 31036, 31054, 31072, 31090, 31108, 31126, 31144, 31162, 31180,
    31198, 31216, 31234, 31252, 31270, 31288, 31306, 31324, 31343, 31361, 31379, 31397, 31415, 31433, 31451,
    31470, 31488, 31506, 31524, 31542, 31561, 31579, 31597, 31615, 31634, 31652, 31670, 31688, 31707, 31725,
    31743, 31762, 31780, 31798, 31817, 31835, 31854, 31872, 31890, 31909, 31927, 31946, 31964, 31983, 32001,
    32020, 32038, 32057, 32075, 32094, 32112, 32131, 32149, 32168, 32186, 32205, 32224, 32242, 32261, 32280,
    32298, 32317, 32336, 32354, 32373, 32392, 32410, 32429, 32448, 32467, 32485, 32504, 32523, 32542, 32560,
    32579, 32598, 32617, 32636, 32655, 32673, 32692, 32711, 32730, 32749, 32768, 32787, 32806, 32825, 32844,
    32863, 32882, 32901, 32920, 32939, 32958, 32977, 32996, 33015, 33034, 33053, 33072, 33091, 33110, 33130,
    33149, 33168, 33187, 33206, 33225, 33245, 33264, 33283, 33302, 33322, 33341, 33360, 33379, 33399, 33418,
    33437, 33457, 33476, 33495, 33515, 33534, 33553, 33573, 33592, 33611, 33631, 33650, 33670, 33689, 33709,
    33728, 33748, 33767, 33787, 33806, 33826, 33845, 33865, 33884, 33904, 33924, 33943, 33963, 33982, 34002,
    34022, 34041, 34061, 34081, 34100, 34120, 34140, 34160, 34179, 34199, 34219, 34239, 34258, 34278, 34298,
    34318, 34338, 34357, 34377, 34397, 34417, 34437, 34457, 34477, 34497, 34517, 34536, 34556, 34576, 34596,
    34616, 34636, 34656, 34676, 34696, 34716,
};

/* Browser-parity control curves. */

typedef struct {
    int32_t b0, b1, b2, a1, a2;
} ToneSubBiquadCoeff;
#include "mello_filter_table.h"
#include "mello_lfo_table.h"

#include "mello_env_table.h"

static const uint16_t REVERB_SEND_PERMILLE[101] = {
    0,   34,  39,  44,  50,  56,  62,  69,  76,  83,  90,  97,  105, 112, 120, 128, 135, 143, 151, 159, 168,
    176, 184, 193, 201, 210, 219, 227, 236, 245, 254, 263, 272, 281, 290, 300, 309, 318, 327, 337, 346, 356,
    365, 375, 385, 394, 404, 414, 424, 434, 444, 453, 463, 473, 484, 494, 504, 514, 524, 534, 545, 555, 565,
    576, 586, 597, 607, 618, 628, 639, 649, 660, 671, 681, 692, 703, 713, 724, 735, 746, 757, 768, 779, 790,
    801, 812, 823, 834, 845, 856, 867, 878, 890, 901, 912, 923, 935, 946, 957, 969, 980,
};

static const int16_t TM_FILTER_OFFSET_PERMILLE[101] = {
    -869, -738, -608, -531, -477, -435, -400, -371, -346, -324, -304, -286, -269, -254, -240, -227, -215,
    -204, -193, -183, -173, -164, -155, -147, -139, -131, -123, -116, -109, -103, -96,  -90,  -84,  -78,
    -73,  -67,  -62,  -57,  -52,  -47,  -42,  -37,  -33,  -28,  -24,  -20,  -16,  -12,  -8,   -4,   0,
    4,    7,    11,   15,   18,   21,   25,   28,   31,   34,   38,   41,   44,   47,   50,   52,   55,
    58,   61,   64,   66,   69,   71,   74,   77,   79,   81,   84,   86,   89,   91,   93,   96,   98,
    100,  102,  105,  107,  109,  111,  113,  115,  117,  119,  121,  123,  125,  127,  129,  131,
};

static int32_t clamp32(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
static bool audio_time_due(uint32_t now, uint32_t due)
{
    return (int32_t)(now - due) >= 0;
}
static uint32_t ms_samples(uint16_t ms)
{
    return ((uint32_t)ms * 441u + 5u) / 10u;
}
/* Starts a ramp to v over ms (or jumps there): Tone's linearRampTo from the value it is at. */
static void ramp_to(MelloRamp *r, int32_t v, uint16_t ms, bool jump)
{
    uint32_t n = ((uint32_t)ms * 441u + 5u) / 10u;
    v <<= 8;
    if (!jump && v == r->target) return;
    r->target = v;
    if (jump || n == 0u) {
        r->cur = v;
        r->left = 0u;
    } else {
        r->left = n;
        r->step = (v - r->cur) / (int32_t)n;
    }
}
/* the ramp's value this sample: moved only while any ramp moves (MelloAudio.ramps_moving), else as it stands */
#define RAMP(a, moving, r) ((moving) ? ramp_q8(&(a)->r) : (a)->r.cur)
static inline int32_t ramp_q8(MelloRamp *r)
{
    if (r->left) {
        if (--r->left == 0u)
            r->cur = r->target;
        else
            r->cur += r->step;
    }
    return r->cur;
}
static uint32_t note_inc(uint8_t note);
/* PERFORM's OCT- / OCT+: a note's pitch moved (its tag and its voice's note stay as played, so the NOTE_OFF that
 * belongs to it still finds it whatever is held by then) */
static uint8_t perf_note(const MelloAudio *a, uint8_t note)
{
    int32_t n = (int32_t)note + a->pfx_semis;
    return (uint8_t)(n < 12 ? 12 : n > 120 ? 120 : n);
}
static uint32_t note_inc(uint8_t note)
{
    return MIDI_INC[note & 127u];
}
/* The increment for a pitch between notes (semitones << 16), interpolated between MIDI notes: within 0.7 cents of
 * 2^(p/12), which is what Tone's exponentialRampTo on the frequency follows. */
static uint32_t pitch_inc(int32_t p_q16)
{
    int32_t semi = p_q16 >> 16;
    uint32_t base, f = ((uint32_t)p_q16 >> 8) & 0xFFu;
    if (semi < 0) return MIDI_INC[0];
    if (semi >= 127) return MIDI_INC[127];
    base = MIDI_INC[semi];
    return base + ((MIDI_INC[semi + 1] - base) >> 8) * f;
}
/* Tone.Monophonic.setNote: with portamento, and the voice still above 0.05, the frequency ramps exponentially (a
 * straight line in pitch) to the note over the portamento time; otherwise it jumps. */
static void voice_glide_to(uint32_t *inc, uint32_t *target_inc, int32_t *pitch_q16, int32_t *step,
                           uint32_t *left, uint8_t note, uint16_t glide_ms, int32_t env_q24)
{
    int32_t to = (int32_t)note << 16;
    uint32_t n;
    *target_inc = note_inc(note);
    if (glide_ms && *inc && env_q24 > (Q24_ONE / 20) && *pitch_q16 != to) {
        n = ms_samples(glide_ms);
        if (n < 1u) n = 1u;
        *left = n;
        *step = (to - *pitch_q16) / (int32_t)n;
    } else {
        *pitch_q16 = to;
        *inc = *target_inc;
        *left = 0u;
    }
}
static void voice_slide_tick(uint32_t *inc, uint32_t target_inc, int32_t *pitch_q16, int32_t step,
                             uint32_t *left)
{
    if (--*left == 0u) {
        *inc = target_inc;
        *pitch_q16 = (*pitch_q16 + step + 0x8000) & ~0xFFFF; /* lands on the note */
    } else {
        *pitch_q16 += step;
        *inc = pitch_inc(*pitch_q16);
    }
}
static uint32_t shifted_note_inc(uint8_t note, int8_t oct)
{
    int32_t n = (int32_t)note + (int32_t)oct * 12;
    if (n < 0) n = 0;
    if (n > 127) n = 127;
    return MIDI_INC[(uint8_t)n];
}
static int16_t sat16(int32_t x)
{
    if (x > 32767) return 32767;
    if (x < -32767) return -32767;
    return (int16_t)x;
}
/* PolyBLEP: the band-limited step's residual for a jump of 2 (Q15, 1.0 = 32768) at phase 0, for a phase
 * increment inc. Non-zero only within one sample of the edge, where a 32-bit division finds the position. */
static int32_t poly_blep_q15(uint32_t phase, uint32_t inc)
{
    uint32_t d, x;
    if (inc < (1u << 15)) return 0; /* below ~0.3 Hz: nothing to band-limit */
    d = inc >> 15;
    if (phase < inc) { /* just after the edge: x = t/dt, x + x - x^2 - 1 */
        x = phase / d;
        if (x > 32768u) x = 32768u;
        return (int32_t)(2u * x) - (int32_t)((x * x) >> 15) - 32768;
    }
    if (phase > 0u - inc) { /* just before it: x = (1-t)/dt, x^2 - 2x + 1 */
        x = (0u - phase) / d;
        if (x > 32768u) x = 32768u;
        return (int32_t)((x * x) >> 15) - (int32_t)(2u * x) + 32768;
    }
    return 0;
}
/* The HTML's oscillators are band-limited (Web Audio PeriodicWaves): saw, square and pulse get PolyBLEP,
 * the sine table is interpolated. inc is the voice's phase increment this sample. */
static int16_t osc_sample(MelloWaveform wave, uint32_t phase, uint32_t inc, uint16_t pwm)
{
    uint16_t x = (uint16_t)(phase >> 16);
    switch (wave) {
    case MELLO_WAVE_SAW: return sat16((int32_t)x - 32768 - poly_blep_q15(phase, inc));
    case MELLO_WAVE_SQUARE:
        return sat16((phase < 0x80000000u ? 32767 : -32767) + poly_blep_q15(phase, inc) -
                     poly_blep_q15(phase - 0x80000000u, inc));
    case MELLO_WAVE_TRIANGLE:
        return (int16_t)(x < 32768u ? (-32767 + (int32_t)x * 2) : (32767 - (int32_t)(x - 32768u) * 2));
    case MELLO_WAVE_SINE: {
        int32_t s0 = SINE_Q15[(uint8_t)(phase >> 24)], s1 = SINE_Q15[(uint8_t)((phase >> 24) + 1u)];
        return (int16_t)(s0 + (((s1 - s0) * (int32_t)((phase >> 16) & 0xFFu)) >> 8));
    }
    case MELLO_WAVE_PULSE: {
        /* Tone's PulseOscillator thresholds a triangle at -width: the duty is (1 + width) / 2, so the HTML's width
         * 0.5 (PWM 50) is a 75% pulse, and 0 would be square. */
        uint32_t th = ((1000u + (uint32_t)pwm) / 2u) * 4294967u;
        return sat16((phase < th ? 32767 : -32767) + poly_blep_q15(phase, inc) -
                     poly_blep_q15(phase - th, inc));
    }
    case MELLO_WAVE_OFF:
    default: return 0;
    }
}
/* A Tone oscillator as the HTML plays it: Web Audio normalizes a PeriodicWave to peak 1, and Tone's saw and square
 * (2047 partials) overshoot to 1.179 at the jump (Gibbs), so Chrome plays them at 0.848 (-1.43 dB). Triangle and sine
 * peak at 1; Tone's pulse is a thresholded triangle, not a PeriodicWave. The chord layer is not in the HTML and keeps
 * osc_sample's levels. */
static int32_t tone_osc_sample(MelloWaveform wave, uint32_t phase, uint32_t inc, uint16_t pwm)
{
    int32_t y = osc_sample(wave, phase, inc, pwm);
    if (wave == MELLO_WAVE_SAW) return (y * 27805) >> 15;
    if (wave == MELLO_WAVE_SQUARE) return (y * 27794) >> 15;
    return y;
}
/* |a * b| must stay below 2^31: |a| <= 65535 with |b| <= 32767 is safe. */
static int32_t q15mul(int32_t a, int32_t b)
{
    return (a * b) >> 15;
}
/* The reverb's loops: a x b in Q15 towards zero, so no error holds a tail up (a floor's -1 LSB, kept by a gain near 1,
 * was V3's floor). For a and b under 2^15, and for any a (in 64 bits). Shifts, no divide. */
static inline int32_t q15mul_tz(int32_t a, int32_t b)
{
    int32_t p = a * b;
    return (p + ((p >> 31) & 32767)) >> 15;
}
static inline int32_t q15mul_tz64(int32_t a, int32_t b)
{
    int64_t p = (int64_t)a * b;
    return (int32_t)((p + ((p >> 63) & 32767)) >> 15);
}
static inline int32_t shr_tz(int32_t x, uint32_t k) /* x / 2^k towards zero, as C's divide */
{
    return (x + ((x >> 31) & (int32_t)((1u << k) - 1u))) >> k;
}
/* x by a gain in Q12 (4096 = 1) or Q15, through the chip's widening multiply: any x, any gain, no divide. */
static inline int32_t mul_q12(int32_t x, int32_t g)
{
    /* towards zero, as a divide: a floor held a -1 LSB in every tail for ever, and put -1/2 LSB of DC into the
     * reverb (+4 dB at 62 Hz in its tail); rounding biased the tail's last bits the other way (+2 dB) */
    int64_t p = (int64_t)x * g;
    return (int32_t)(p >= 0 ? p >> 12 : -((-p) >> 12));
}
static inline int32_t mul_q15w(int32_t x, int32_t g)
{
    int64_t p = (int64_t)x * g;
    return (int32_t)(p >= 0 ? p >> 15 : -((-p) >> 15));
}
#define mul_q12_fb mul_q12
static inline int32_t permille_q12(int32_t p)
{
    return p * 4096 / 1000;
}
/* For |a| up to 131071 (a difference of two 17-bit values) with 0 <= b <= 32767, or |a| <= 65535 with b up to
 * 65535: halving a first keeps the product in int32 at the cost of one LSB. No 64-bit multiply (docs/CPU.md). */
static int32_t q15mul_wide(int32_t a, int32_t b)
{
    return ((a >> 1) * b) >> 14;
}
/* Grit (DECISIONS.md "Grit"): a LFO target that moves a grit amount swings it around its base, as PWM. */
static int32_t lfo_around(uint16_t base, int32_t lfo, uint16_t depth, MelloLfoTarget target,
                          const MelloAudio *a)
{
    if (!a->config.lfo_on || a->config.lfo_target != target) return base;
    return clamp32((int32_t)base + ((lfo * (int32_t)depth) >> 15), 0, 1000);
}
/* 2^(q8 / 3072): a pitch offset in 1/256 semitone as a Q16 ratio, -48..+48 semitones. */
static uint32_t semi_ratio_q16(int32_t q8)
{
    int32_t i, f;
    q8 = clamp32(q8 + 48 * 256, 0, 96 * 256 - 1);
    i = q8 >> 8, f = q8 & 255;
    return SEMI_RATIO_Q16[i] + (uint32_t)(((int32_t)(SEMI_RATIO_Q16[i + 1] - SEMI_RATIO_Q16[i]) * f) >> 8);
}
static uint32_t inc_times(uint32_t inc, uint32_t ratio_q16)
{
    uint64_t v = ((uint64_t)inc * ratio_q16) >> 16;
    return v > 0x7FFFFFFFu ? 0x7FFFFFFFu : (uint32_t)v; /* below Nyquist's 2^31 */
}
/* An increment that FM has pushed through zero runs the phase backwards; the band-limiting wants its size. */
static uint32_t inc_size(uint32_t inc)
{
    return (int32_t)inc < 0 ? 0u - inc : inc;
}
/* SAT: soft clipping (3t - t^3) / 2 of the oscillators driven up to 16x (+24 dB, squared knob), the Virus's hot
 * oscillator volume into the filter. x and the result are +-65535. The curve's slope at 0 is 1.5 and a saw clips
 * from about 2x on, so the make-up 2 / 3g, at least 0.32, keeps the level near the clean one across the knob. */
static int32_t saturate(int32_t x, int32_t sat)
{
    int32_t g_q8 = 256 + sat * sat / 1000 * 15 * 256 / 1000,
            t = clamp32((x * (g_q8 >> 4)) >> 5, -32767, 32767), y, makeup = 2 * 32767 * 256 / (3 * g_q8);
    if (makeup < 10486) makeup = 10486;
    y = (3 * t - (((t * t) >> 15) * t >> 15)) >> 1;
    return (y * 2 * makeup) >> 15;
}
/* The unison gain, 1/sqrt(n): detuned oscillators add up in power, not in amplitude. */
static const int16_t UNISON_GAIN_Q15[MELLO_UNISON_MAX] = {32767, 23170, 18919, 16384};
/* Each unison oscillator's place in the spread, in thirds: n = 2 is +-1, 3 is -1 0 +1, 4 is -1 -1/3 +1/3 +1. */
static const int8_t UNISON_SPREAD_THIRDS[MELLO_UNISON_MAX][MELLO_UNISON_MAX] = {
    {0, 0, 0, 0}, {-3, 3, 0, 0}, {-3, 0, 3, 0}, {-3, -1, 1, 3}};

static int32_t permille_q24(uint16_t p)
{
    uint32_t q = (uint32_t)Q24_ONE / 1000u;
    uint32_t r = (uint32_t)Q24_ONE % 1000u;
    if (p > 1000u) p = 1000u;
    return (int32_t)(q * (uint32_t)p + (r * (uint32_t)p) / 1000u);
}
static void env_set_segment(int32_t *step, int32_t *target, int32_t current, int32_t targetv,
                            uint32_t samples)
{
    int32_t d = targetv - current;
    if (samples < 1u) samples = 1u;
    *target = targetv;
    *step = d / (int32_t)samples;
    if (*step == 0 && d != 0) *step = d > 0 ? 1 : -1;
}
static int32_t env_tick(int32_t *level, int32_t step, int32_t target, MelloEnvStage *stage)
{
    int32_t n = *level + step;
    if ((step >= 0 && n >= target) || (step < 0 && n <= target)) {
        n = target;
        if (*stage == MELLO_ENV_ATTACK)
            *stage = MELLO_ENV_DECAY;
        else if (*stage == MELLO_ENV_DECAY)
            *stage = MELLO_ENV_SUSTAIN;
        else if (*stage == MELLO_ENV_RELEASE)
            *stage = MELLO_ENV_OFF;
    }
    *level = n;
    return n;
}
/* Tone.Envelope's exponential decay/release (exponentialApproachValueAtTime): an exponential approach to the
 * target with tau = ln(D+1)/ln 200 for 90 % of D, then a linear ramp onto the target. Shared by every voice
 * through TONE_ENV(v), which names the voice's envelope fields. */
#define TONE_ENV(v)                                                                                          \
    &(v)->env_q24, &(v)->env_step_q24, &(v)->env_target_q24, &(v)->env_curve_left, &(v)->env_linear_left,    \
        &(v)->env_curve_coeff_q31, &(v)->env_stage
static void tone_curve_begin(int32_t *level, int32_t *step, int32_t *target, uint32_t *curve_left,
                             uint32_t *linear_left, uint32_t *coeff, MelloEnvStage *stage, int32_t to,
                             uint16_t ms)
{
    uint32_t n, curve;
    (void)level;
    (void)stage;
    if (ms > TONE_ENV_MAX_MS) ms = TONE_ENV_MAX_MS;
    n = ms_samples(ms);
    if (n < 2u) n = 2u;
    curve = (n * 9u) / 10u;
    if (curve < 1u) curve = 1u;
    if (curve >= n) curve = n - 1u;
    *target = to;
    *curve_left = curve;
    *linear_left = n - curve;
    *coeff = TONE_ENV_COEFF_Q31[ms];
    *step = 0;
}
static void tone_curve_tick(int32_t *level, int32_t *step, int32_t *target, uint32_t *curve_left,
                            uint32_t *linear_left, uint32_t *coeff, MelloEnvStage *stage)
{
    if (*curve_left) {
        int32_t d = *level - *target;
        *level = *target + (int32_t)(((int64_t)d * (int64_t)*coeff) >> 31);
        if (--*curve_left == 0u) {
            uint32_t n = *linear_left ? *linear_left : 1u;
            *step = (*target - *level) / (int32_t)n;
        }
    } else if (*linear_left) {
        if (--*linear_left == 0u) {
            *level = *target;
            if (*stage == MELLO_ENV_DECAY)
                *stage = MELLO_ENV_SUSTAIN;
            else if (*stage == MELLO_ENV_RELEASE)
                *stage = MELLO_ENV_OFF;
        } else
            *level += *step;
    }
}
/* Tone.Envelope's linear attack, shortened on a retrigger by the level already reached. */
static void tone_attack_begin(int32_t *level, int32_t *step, int32_t *target, uint32_t *curve_left,
                              uint32_t *linear_left, uint32_t *coeff, MelloEnvStage *stage, uint16_t ms)
{
    uint32_t n = ms_samples(ms);
    (void)coeff;
    if (*level > 0 && *level < Q24_ONE)
        n = (uint32_t)(((uint64_t)n * (uint32_t)(Q24_ONE - *level)) / Q24_ONE);
    if (n < 1u) n = 1u;
    *curve_left = 0u;
    *linear_left = 0u;
    *stage = MELLO_ENV_ATTACK;
    env_set_segment(step, target, *level, Q24_ONE, n);
}
static void lead_begin_decay(MelloAudio *a)
{
    tone_curve_begin(TONE_ENV(&a->lead), permille_q24(a->config.sustain_permille),
                     a->effective_decay_ms ? a->effective_decay_ms : a->config.decay_ms);
}
/* What the note's velocity does to the cutoff and the MOD envelope (tracking), for the per-sample path. */
static void lead_tracking(MelloAudio *a)
{
    int32_t amt = a->config.menv_amount;
    a->lead.vel_cut = (int16_t)(a->lead.vel_q15 * a->config.vel_cutoff / 1000 * 300 / 32767);
    if (a->config.vel_menv) /* VEL > MOD ENV scales it by up to x2 (or to none) for an accent */
        amt = clamp32(amt + amt * (a->lead.vel_q15 * a->config.vel_menv / 1000) / 32767, -1000, 1000);
    a->lead.menv_amt_q15 = amt * 32768 / 1000;
}
static void lead_attack(MelloAudio *a, uint8_t note, uint8_t vel, bool slide)
{
    a->lead.note = note;
    a->lead.velocity = vel;
    a->lead.gate = true;
    {
        /* Tracking: velocity from the HTML's plain step (91) to its accent (121) is 0..1. */
        int32_t dv = ((int32_t)vel - 91) * 32767 / 30;
        a->lead.vel_q15 = (int16_t)clamp32(dv, -32767, 32767);
        a->lead.vel_level =
            (uint8_t)clamp32(91 + ((int32_t)vel - 91) * (int32_t)a->config.vel_level / 1000, 1, 127);
        lead_tracking(a);
    }
    /* HTML: portamento is set per step, slideTime on SLIDE steps and 0 otherwise. */
    voice_glide_to(&a->lead.inc, &a->lead.target_inc, &a->lead.pitch_q16, &a->lead.slide_step,
                   &a->lead.slide_left, note, slide ? a->config.slide_ms : 0u, a->lead.env_q24);
    /* HTML: the follower sub (subOsc) never glides; only the lead's portamento is set per step. */
    a->lead.sub_target_inc = shifted_note_inc(note, a->config.follower_sub_octave);
    a->lead.sub_inc = a->lead.sub_target_inc;
    a->lead.sub_slide_left = 0u;
    tone_attack_begin(TONE_ENV(&a->lead), a->config.attack_ms);
    if (a->config.menv_amount) { /* the MOD envelope restarts with every note, as the amp envelope */
        uint32_t att = (uint32_t)a->config.menv_attack_ms * 441u / 10u,
                 dec = (uint32_t)a->config.menv_decay_ms * 441u / 10u;
        /* The decay falls to 1/1000 (-60 dB) in its time: ln 1000 = 6.9 time constants. */
        a->menv_k_q16 = (int32_t)(452710u / (dec ? dec : 1u));
        if (a->menv_k_q16 < 1) a->menv_k_q16 = 1;
        if (att) {
            a->menv_stage = 1u;
            a->menv_step = (int32_t)((1u << 24) / att);
            if (a->menv_step < 1) a->menv_step = 1;
        } else {
            a->menv_stage = 2u;
            a->menv_q24 = 1 << 24;
        }
    }
}
static void lead_release(MelloAudio *a)
{
    a->lead.gate = false;
    a->lead.env_stage = MELLO_ENV_RELEASE;
    tone_curve_begin(TONE_ENV(&a->lead), 0, a->config.release_ms);
}
static void sub_curve_begin(MelloAudio *a, int32_t target, uint16_t ms)
{
    tone_curve_begin(TONE_ENV(&a->sub), target, ms);
}
static void sub_curve_tick(MelloAudio *a)
{
    tone_curve_tick(TONE_ENV(&a->sub));
}
static void sub_begin_decay(MelloAudio *a)
{
    /* DRONE holds the attack's full level: its LEVEL is the drone's level. */
    sub_curve_begin(a, permille_q24(a->sub_config.mode == MELLO_SUB_MODE_DRONE ? 1000u : 240u),
                    a->sub_config.decay_ms);
}
/* seq: a BASS SEQ note, which slides (GLIDE, or 60 ms at 0) only when its step is glided into, and then, the last
 * note still held, without a new attack (legato); others always glide GLIDE, as the HTML's. */
static void sub_attack(MelloAudio *a, uint8_t note, uint8_t vel, bool seq, bool slide)
{
    uint32_t attack_n;
    uint16_t glide = a->sub_config.glide_ms;
    bool legato = seq && slide && a->sub.gate && a->sub.env_q24 > 0;
    if (seq) glide = slide ? (glide ? glide : 60u) : 0u;
    a->sub.note = note;
    a->sub.velocity = vel;
    a->sub.gate = true;
    a->sub.accent = seq && vel >= 127u;
    voice_glide_to(&a->sub.inc, &a->sub.target_inc, &a->sub.pitch_q16, &a->sub.slide_step, &a->sub.slide_left,
                   note, glide, a->sub.env_q24);
    if (legato) return; /* the envelopes go on */
    a->sub.fenv = 1 << 24;
    /* Tone.Envelope shortens a retrigger attack by the already-reached level. */
    attack_n = ms_samples(3u);
    if (a->sub.env_q24 > 0 && a->sub.env_q24 < Q24_ONE)
        attack_n = (uint32_t)(((uint64_t)attack_n * (uint32_t)(Q24_ONE - a->sub.env_q24)) / Q24_ONE);
    if (attack_n < 1u) attack_n = 1u;
    a->sub.env_curve_left = 0u;
    a->sub.env_linear_left = 0u;
    a->sub.env_stage = MELLO_ENV_ATTACK;
    env_set_segment(&a->sub.env_step_q24, &a->sub.env_target_q24, a->sub.env_q24, Q24_ONE, attack_n);
}
static void sub_release(MelloAudio *a)
{
    a->sub.gate = false;
    a->sub.env_stage = MELLO_ENV_RELEASE;
    sub_curve_begin(a, 0, a->sub_config.release_ms);
}
static int32_t soft_drive(int32_t x, uint16_t amount, uint16_t character);
/* DRUMS (2026-10-31: "the drums sound very 16-bit FM; I want something modern and glitchy: Richard Devine, Aphex
 * Twin, glitch music"). Built from what that music is built from: noise through resonant filters that sweep, micro-
 * sounds and clicks, the music itself cut into loops, every hit placed somewhere else. Each voice in a few operations
 * a sample, moved by each hit's PITCH, DECAY and CRUSH:
 * - KICK: an interpolated sine whose pitch falls twice, a click (from 7x in ~6 ms) and a body (from 1.7x in ~60 ms);
 *   its level held up to 12 ms, so it punches; noise above ~2 kHz on the click; TONE drive.
 * - SNARE: noise through a state-variable band-pass that sweeps down from 4x its TUNE (0.9 .. 3 kHz) in ~18 ms, a
 *   short falling body under it, driven; TONE from a broad crack (Q 0.7) to a resonant zap (Q 12). Its tail is gated:
 *   at -18 dB it falls four times as fast.
 * - HAT: noise high-passed and ringing at 5 .. 9 kHz (TUNE), crossed by TONE with a tick: a sine burst there, gone in
 *   ~4 ms. Each hit lands somewhere else in the stereo field.
 * - GLITCH, its TONE in quarters: BLEEP (FM, its index falling fast), CHIRP (a sine falling from 8x in ~12 ms, ring-
 *   modulated), CRACKLE (noise held for longer and longer), STUTTER (the last 6 .. 46 ms of the music and the drums,
 *   looped: TUNE the loop's length). Scattered as the hat.
 * Kick and snare are rounded together by a soft cubic saturation (drum_glue). CRUSH stays, at 0 by default. */
static uint32_t hz_inc(uint32_t hz)
{
    return hz * 97392u; /* 2^32 / 44100 a Hz */
}
/* A one-pole fall to -60 dB (6.9 time constants) in ms, Q24 a sample. */
static int32_t drum_rate(uint32_t ms)
{
    return (int32_t)(2625000u / (ms < 2u ? 2u : ms));
}
static int32_t drum_noise(MelloAudio *a)
{
    uint32_t x = a->drum_noise ? a->drum_noise : 0x9E3779B9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    a->drum_noise = x;
    return (int32_t)(x >> 16) - 32768;
}
/* The phase increment pushed up by an envelope: inc x (1 + m), m Q24. */
static uint32_t drum_bend(uint32_t inc, int32_t m)
{
    return inc + (uint32_t)(((uint64_t)inc * (uint32_t)m) >> 24);
}
/* A Hz as the state-variable filter's f, Q15 (2 pi fc / fs), within its stable range. */
static int32_t drum_svf_f(uint32_t hz, uint32_t ratio_q16)
{
    uint32_t f = inc_times(hz * 4669u / 1000u, ratio_q16);
    return (int32_t)(f > 36000u ? 36000u : f);
}
/* Chamberlin's state-variable filter: the band-pass out, lp and bp kept; *hp the high-pass. */
static int32_t drum_svf(MelloAudioDrumVoice *d, int32_t x, int32_t f, int32_t *hp)
{
    int32_t h;
    d->lp = clamp32(d->lp + (int32_t)(((int64_t)f * d->bp) >> 15), -524288, 524287);
    h = x - d->lp - (int32_t)(((int64_t)d->damp * d->bp) >> 15);
    d->bp = clamp32(d->bp + (int32_t)(((int64_t)f * h) >> 15), -524288, 524287);
    if (hp) *hp = h;
    return d->bp;
}
/* Where a scattered hit sits: up to 60% to either side. */
static void drum_scatter(MelloAudio *a, MelloAudioDrumVoice *d)
{
    int32_t r = (drum_noise(a) & 1023) - 512;
    d->gl = 32767 - (r > 0 ? r * 38 : 0);
    d->gr = 32767 - (r < 0 ? -r * 38 : 0);
}
static void drum_hit(MelloAudio *a, const MelloEvent *e)
{
    /* Weight (2026-11-01, DECISIONS.md "A kick with weight"): the kick decays up to 1.5 s and the snare's body
     * lasts; the kick's pitch falls from 3.5x over 90 ms with a 7x click, its sine always a little driven. */
    static const uint16_t base_ms[MELLO_DRUM_VOICES] = {80u, 70u, 15u, 10u},
                          span_ms[MELLO_DRUM_VOICES] = {1420u, 630u, 785u, 390u};
    uint8_t v = e->sample_slot;
    MelloAudioDrumVoice *d;
    const MelloDrumVoice *c;
    uint32_t ms, ratio, crush, hold_ms = 0u;
    if (v >= MELLO_DRUM_VOICES || !a->drum_config.on) return;
    d = &a->drum[v];
    c = &a->drum_config.voice[v];
    ms = base_ms[v] + (uint32_t)c->decay * span_ms[v] / 1000u;
    if (e->drum_decay || a->drum_mod[1]) /* the step's DECAY, and the modulation's (up to 4x either way) */
        ms = inc_times(ms << 8, semi_ratio_q16((int32_t)e->drum_decay * 24 * 256 / 100 +
                                               (int32_t)a->drum_mod[1] * 6144 / 1000)) >>
             8;
    ratio = semi_ratio_q16(((int32_t)e->note - 64) * 256 +
                           (int32_t)a->drum_mod[0] * 3072 / 1000); /* PITCH, +-12 */
    d->env = 1 << 24;
    d->penv = d->cenv = 1 << 24;
    d->pk = d->ck = 0;
    d->gated = 0u;
    d->gl = d->gr = 32767;
    d->gain = (int32_t)c->level * 32767 / 1000 * e->velocity / 127;
    switch (v) {
    case MELLO_DRUM_KICK:
        d->inc[0] = inc_times(hz_inc(38u + c->tune * 52u / 1000u), ratio); /* 38 .. 90 Hz */
        d->ph[0] = 0u;
        d->ck = drum_rate(6u);
        d->pk = drum_rate(90u); /* the punch: the body's fall, 90 ms (was 60) (DUCK: apply_event's) */
        hold_ms = ms / 8u > 14u ? 14u : ms / 8u;
        break;
    case MELLO_DRUM_SNARE:
        d->f = drum_svf_f(600u + c->tune * 2400u / 1000u, ratio); /* 0.6 .. 3 kHz: lower at low TUNE */
        d->damp = 45875 - (int32_t)c->tone * 43254 / 1000;        /* 1/Q: 1.4 .. 0.08 */
        d->inc[0] = inc_times(hz_inc(150u + c->tune * 90u / 1000u), ratio);
        d->ph[0] = 0u;
        d->pk = drum_rate(ms / 2u + 10u); /* the body lasts half the hit (was a quarter) */
        d->ck = drum_rate(18u);
        hold_ms = 1u;
        break;
    case MELLO_DRUM_HAT: {
        uint32_t hz = 5000u + c->tune * 4000u / 1000u;
        d->f = drum_svf_f(hz, ratio);
        d->damp = 16384; /* Q 2: a ring */
        d->inc[0] = inc_times(hz_inc(hz), ratio);
        if (d->inc[0] > hz_inc(16000u)) d->inc[0] = hz_inc(16000u); /* the tick stays below Nyquist */
        d->ph[0] = 0u;
        d->ck = drum_rate(4u);
        hold_ms = 1u;
        drum_scatter(a, d);
        break;
    }
    default: { /* GLITCH */
        uint32_t frac = c->tone % 250u;
        d->kind = MELLO_GLITCH_KIND(c->tone);
        drum_scatter(a, d);
        d->hp1 = 0;
        switch (d->kind) {
        case MELLO_GLITCH_BLEEP:
            d->inc[0] =
                inc_times(inc_times(hz_inc(550u), semi_ratio_q16((int32_t)c->tune * 59 * 256 / 1000 - 7552)),
                          ratio);                                     /* 100 Hz .. 3 kHz */
            d->inc[1] = inc_times(d->inc[0], 131072u + frac * 1310u); /* x 2 .. 7 */
            d->pk = drum_rate(ms / 4u + 2u);
            break;
        case MELLO_GLITCH_CHIRP:
            d->inc[0] = inc_times(hz_inc(300u + c->tune * 1700u / 1000u), ratio);
            d->inc[1] = inc_times(d->inc[0], 92406u + frac * 400u); /* x 1.41 .. 2.9 */
            d->ck = drum_rate(12u + ms / 10u);
            break;
        case MELLO_GLITCH_CRACKLE:
            d->sh_n = (uint16_t)(1u + (1000u - c->tune) / 100u +
                                 (inc_times(64u << 8, ratio) < (64u << 8) ? 4u : 0u));
            d->sh_left = 0u;
            d->ck = drum_rate(ms / 2u + 2u);
            break;
        default: { /* STUTTER: the last loop_len samples, from here (the job's count of the memory) */
            uint32_t len = 256u + (1000u - c->tune) * 1792u / 1000u;
            len = len * 65536u / (ratio < 16384u ? 16384u : ratio); /* PITCH: a shorter loop is higher */
            if (len < 32u) len = 32u;
            if (len > MELLO_DRUM_BUF - 1u) len = MELLO_DRUM_BUF - 1u;
            d->loop_len = (uint16_t)len;
            d->loop_start = (uint16_t)((a->parts_job.buf_pos - len) & (MELLO_DRUM_BUF - 1u));
            d->loop_pos = 0u;
            break;
        }
        }
        break;
    }
    }
    d->k = drum_rate(ms > hold_ms + 2u ? ms - hold_ms : 2u);
    d->hold_s = (uint16_t)(hold_ms * 44u);
    crush =
        (uint32_t)c->crush / 10u + e->drum_crush + (a->drum_mod[2] > 0 ? (uint32_t)a->drum_mod[2] / 10u : 0u);
    if (crush > 100u) crush = 100u;
    d->hold_n = (uint8_t)(1u + crush * 15u / 100u);
    d->mask = ~((1 << (crush * 11u / 100u)) - 1);
    d->hold = 0u;
    d->on = 1u;
}
/* The kit's soft saturation: x - x^3 / 3 (full scale 65536) on 1.5 x the sum, back down by 1.5. Unity for quiet
 * hits; a kick's peak ~1.4 dB rounder; never past full scale. */
static int32_t drum_glue(int32_t s)
{
    int32_t x = clamp32(s * 3 / 2, -65535, 65535), q = (int32_t)(((int64_t)x * x) >> 16);
    return (x - (int32_t)(((int64_t)q * x) >> 16) / 3) * 2 / 3;
}
/* The kit's DRIVE (up to 8x, and FOLD's 4x more) into a soft clip, crossed by FOLD with a fold: past full scale the
 * wave turns back (a triangle, no loop). Its level is held by drum_makeup_q12 (drums_makeup), not here. */
static int32_t drum_drive(int32_t x, uint16_t drive, uint16_t fold, int32_t fold_q12)
{
    int32_t y = (int32_t)(((int64_t)x * (1024 + (int32_t)drive * 7 + (int32_t)fold * 4)) >> 10), c, q, u, out;
    c = clamp32(y, -65535, 65535);
    q = (int32_t)(((int64_t)c * c) >> 16);
    out = c - (int32_t)(((int64_t)q * c) >> 16) / 3; /* the soft clip, to +-43690 */
    if (fold) {
        u = (y + 32768) & 0x1FFFF; /* 0 .. 131071: up, then back down */
        if (u >= 65536) u = 131071 - u;
        out += mul_q12((u - 32768) * 4 / 3 - out, fold_q12);
    }
    return out;
}
/* STUTTER's memory at, as it stands at the chunk's sample s: written this chunk before s (the render's, beside),
 * it waits until the render has written it; anything else is as it was (the render writes a sample only after the
 * job has done it). One core: the render has written every sample before s. */
static int32_t drum_memory(MelloAudio *a, uint32_t s, uint32_t at)
{
    uint32_t sp =
        (at - a->parts_job.buf_pos0) & (MELLO_DRUM_BUF - 1u); /* the chunk's sample that writes it */
    if (sp < s && a->heard_done <= sp) {
        while (a->heard_done <= sp) a->cores->sync();
        a->cores->sync();
    }
    return a->drum_buf[at];
}
/* One sample of the drums, in stereo (before LEVEL), the chunk's sample s: out[0] left, out[1] right, out[2] their
 * share of STUTTER's memory (the render adds the music, as loud after LEVEL as itself, and writes it). */
static void drum_voices(MelloAudio *a, uint32_t s, int32_t out[3])
{
    int32_t mid = 0, l = 0, r = 0;
    uint8_t v;
    for (v = 0u; v < MELLO_DRUM_VOICES; ++v) {
        MelloAudioDrumVoice *d = &a->drum[v];
        const MelloDrumVoice *c = &a->drum_config.voice[v];
        int32_t y, g;
        if (!d->on) continue;
        if (d->hold_s)
            --d->hold_s;
        else
            d->env -= (int32_t)(((int64_t)d->env * d->k) >> 24);
        if (v == MELLO_DRUM_SNARE && !d->gated && d->env < (1 << 21))
            d->k <<= 2, d->gated = 1u; /* the gate */
        if (d->pk) d->penv -= (int32_t)(((int64_t)d->penv * d->pk) >> 24);
        if (d->ck) {
            d->cenv -= (int32_t)(((int64_t)d->cenv * d->ck) >> 24);
            if (d->cenv < 16384) d->cenv = 0, d->ck = 0; /* -60 dB: done */
        }
        if (d->env < 2048) {
            d->on = 0u;
            continue;
        }
        g = d->env >> 9;
        switch (v) {
        case MELLO_DRUM_KICK:
            /* the pitch falls from 3.5x over 90 ms (the punch) under a 7x click; the sine always driven a little,
             * so the fundamental has the harmonics a small speaker can carry, TONE adds more */
            d->ph[0] += drum_bend(d->inc[0], d->cenv * 6 + d->penv * 5 / 2);
            y = osc_sample(MELLO_WAVE_SINE, d->ph[0], 0u, 0u);
            y = soft_drive(y, (uint16_t)(220u + c->tone * 780u / 1000u), 500u); /* TONE: drive, from 22% */
            if (d->ck) { /* the click's noise, above ~2 kHz */
                int32_t n = drum_noise(a);
                d->hp1 += (n - d->hp1) >> 2;
                y += q15mul(n - d->hp1, d->cenv >> 9) / 3;
            }
            y = q15mul(clamp32(y, -32767, 32767), g);
            break;
        case MELLO_DRUM_SNARE: {
            int32_t f = d->f + (int32_t)(((int64_t)d->f * 3 * d->cenv) >> 24),
                    fmax = (65536 - d->damp) * 3 / 4;
            int32_t zap = drum_svf(d, drum_noise(a), f < fmax ? f : fmax, 0);
            /* the 1/Q + 9000 passes 32767 below TONE ~50%: with the noise at 65535, past a q15mul's range */
            y = (int32_t)(((int64_t)clamp32(zap, -65535, 65535) * (d->damp + 9000)) >> 15);
            if (d->pk) { /* the body, until it is down 60 dB; twice the weight it had */
                d->ph[0] += drum_bend(d->inc[0], d->cenv >> 1);
                y += q15mul(osc_sample(MELLO_WAVE_SINE, d->ph[0], 0u, 0u), d->penv >> 9);
                if (d->penv < 16384) d->pk = 0;
            }
            y = soft_drive(y, 450u, 600u); /* driven: the crunch */
            y = q15mul(y, g);
            break;
        }
        case MELLO_DRUM_HAT: {
            int32_t hp, bp = drum_svf(d, drum_noise(a), d->f, &hp), tick = 0;
            y = clamp32(hp + bp / 2, -32767, 32767);
            if (d->ck) {
                d->ph[0] += d->inc[0];
                tick = q15mul(osc_sample(MELLO_WAVE_SINE, d->ph[0], 0u, 0u), d->cenv >> 9);
            }
            /* TONE: noise .. tick; tick * 2 - y reaches 98301, past a q15mul's range: widened, floored as one */
            y = y + (int32_t)(((int64_t)(tick * 2 - y) * ((int32_t)c->tone * 32)) >> 15);
            y = q15mul(clamp32(y, -32767, 32767), g);
            break;
        }
        default:
            switch (d->kind) {
            case MELLO_GLITCH_BLEEP: {
                int32_t m;
                d->ph[1] += d->inc[1];
                m = SINE_Q15[(uint8_t)(d->ph[1] >> 24)];
                d->ph[0] += d->inc[0];
                y = SINE_Q15[(uint8_t)((d->ph[0] + (uint32_t)((int64_t)m * (d->penv >> 8))) >> 24)];
                break;
            }
            case MELLO_GLITCH_CHIRP: {
                int32_t s0;
                d->ph[0] += drum_bend(d->inc[0], d->cenv * 7);
                d->ph[1] += drum_bend(d->inc[1], d->cenv * 7);
                s0 = osc_sample(MELLO_WAVE_SINE, d->ph[0], 0u, 0u);
                y = (s0 + ((d->ph[1] & 0x80000000u) ? s0 : -s0)) / 2 +
                    s0 / 2; /* ring-modulated, and itself */
                break;
            }
            case MELLO_GLITCH_CRACKLE:
                if (!d->sh_left) { /* held longer as the hit goes on */
                    d->sh = drum_noise(a);
                    d->sh_left = (uint16_t)(d->sh_n + ((16777216 - d->cenv) >> 19));
                } else
                    --d->sh_left;
                d->hp1 += (d->sh - d->hp1) >> 4;
                y = clamp32((d->sh - d->hp1) * 3 / 2, -32767, 32767);
                break;
            default: /* STUTTER */
                y = clamp32(drum_memory(a, s, (d->loop_start + d->loop_pos) & (MELLO_DRUM_BUF - 1u)) * 4,
                            -32767, 32767);
                if (++d->loop_pos >= d->loop_len) d->loop_pos = 0u;
                break;
            }
            y = q15mul(y, g);
            break;
        }
        if (d->hold_n > 1u || d->mask != -1) { /* CRUSH: held for hold_n samples, its low bits dropped */
            if (!d->hold--) {
                d->held = y & d->mask;
                d->hold = (uint8_t)(d->hold_n - 1u);
            }
            y = d->held;
        }
        y = q15mul(y, d->gain);
        if (v <= MELLO_DRUM_SNARE)
            mid += y;
        else
            l += q15mul(y, d->gl), r += q15mul(y, d->gr);
    }
    if (mid) mid = drum_glue(mid);
    out[2] =
        mid + (l + r) / 2; /* STUTTER's memory: the drums' share (the render's: the music, then a quarter) */
    l += mid, r += mid;
    {
        uint16_t drive = (uint16_t)clamp32((int32_t)a->drum_config.drive + a->drum_mod[3], 0, 1000);
        if (drive |
            a->drum_config.fold) { /* the kit's own DRIVE (modulated) and FOLD, at the level it had without */
            int32_t pos = ((int32_t)drive * 67109) >> 10, i = pos >> 12, f = pos & 4095,
                    m; /* drive * 16 / 1000, Q12 */
            m = a->drum_makeup_q12[i];
            if (i < 16) m += ((a->drum_makeup_q12[i + 1] - m) * f) >> 12;
            l = mul_q12(drum_drive(l, drive, a->drum_config.fold, a->drum_fold_q12), m);
            r = mul_q12(drum_drive(r, drive, a->drum_config.fold, a->drum_fold_q12), m);
        }
    }
    out[0] = clamp32(l, -65535, 65535);
    out[1] = clamp32(r, -65535, 65535);
}
static uint32_t sample_rate_q16(const MelloSampleSlotConfig *c, uint8_t note)
{
    int32_t semi;
    uint32_t base, hi, lo, scaled;
    uint16_t fq;
    if (!c) return 32768u;
    semi = (c->track ? ((int32_t)note - 60) : 0) + (int32_t)c->coarse_semitones;
    if (semi < -60) semi = -60;
    if (semi > 60) semi = 60;
    base = SAMPLE_SEMI_Q16[(uint32_t)(semi + 60)];
    fq = SAMPLE_FINE_Q15[(uint32_t)((int32_t)c->fine_cents + 100)];
    hi = base >> 16;
    lo = base & 0xFFFFu;
    scaled = hi * (uint32_t)fq * 2u + ((lo * (uint32_t)fq) >> 15);
    if (scaled < 2048u) scaled = 2048u;
    if (scaled > 2097152u) scaled = 2097152u;
    return scaled >> 1;
}
static void sample_begin_decay(MelloAudio *a, MelloAudioSampleVoice *v)
{
    const MelloSampleSlotConfig *c = &a->sample_config.slots[v->slot];
    /* HTML: each slot's Tone.AmplitudeEnvelope, exponential decay and release as the lead's. */
    tone_curve_begin(TONE_ENV(v), permille_q24(c->sustain_permille), c->decay_ms);
}
static void sample_release_voice(MelloAudio *a, uint8_t slot)
{
    MelloAudioSampleVoice *v;
    const MelloSampleSlotConfig *c;
    if (!a || slot >= MELLO_SAMPLE_SLOT_COUNT) return;
    v = &a->sample_voice[slot];
    if (!v->active) return;
    c = &a->sample_config.slots[slot];
    v->gate = false;
    v->env_stage = MELLO_ENV_RELEASE;
    tone_curve_begin(TONE_ENV(v), 0, c->release_ms);
}
static void sample_attack_voice(MelloAudio *a, const MelloEvent *e)
{
    MelloAudioSampleVoice *v;
    const MelloSampleSlotConfig *c;
    const MelloSampleSlotMeta *m;
    uint32_t n, start, end, target, start_step;
    uint8_t slot;
    if (!a || !e || !a->sample_bank || !a->sample_config.on) return;
    slot = e->sample_slot;
    if (slot >= MELLO_SAMPLE_SLOT_COUNT) return;
    m = &a->sample_bank->meta[slot];
    if (!m->loaded || m->sample_count < 2u) return;
    c = &a->sample_config.slots[slot];
    v = &a->sample_voice[slot];
    {
        int32_t prev_env = v->active ? v->env_q24 : 0;
        start = ((uint32_t)c->trim_start * m->sample_count) / 1000u;
        end = ((uint32_t)c->trim_end * m->sample_count) / 1000u;
        if (start >= m->sample_count) start = m->sample_count - 1u;
        if (end > m->sample_count) end = m->sample_count;
        if (end <= start + 1u) end = start + 1u;
        target = sample_rate_q16(c, e->note);
        start_step = a->sample_last_target_step_q16[slot] ? a->sample_last_target_step_q16[slot] : target;
        memset(v, 0, sizeof(*v));
        v->active = true;
        v->gate = true;
        v->loop = c->loop;
        v->slot = slot;
        v->velocity = e->velocity;
        v->trim_start_sample = start;
        v->trim_end_sample = end;
        v->pos_q16 = start << 16;
        v->target_step_q16 = target;
        if (e->slide && a->config.slide_ms) {
            n = ms_samples(a->config.slide_ms);
            if (n < 1u) n = 1u;
            v->step_q16 = start_step;
            v->slide_left = n;
            v->slide_step_q16 = ((int32_t)target - (int32_t)start_step) / (int32_t)n;
        } else
            v->step_q16 = target;
        a->sample_last_target_step_q16[slot] = target;
        v->env_stage = MELLO_ENV_ATTACK;
        v->env_q24 = prev_env;
        v->env_curve_left = v->env_linear_left = 0u;
        env_set_segment(&v->env_step_q24, &v->env_target_q24, v->env_q24, Q24_ONE, ms_samples(c->attack_ms));
    }
}
/* 1: the block holding sample_index is decoded; 0: it cannot be (stop the voice); -1: this render call's
 * decode budget is spent (mute this frame). */
static int sample_decode_for(MelloAudio *a, MelloAudioSampleVoice *v, uint32_t sample_index)
{
    uint16_t block = (uint16_t)(sample_index / MELLO_SAMPLE_BLOCK_SAMPLES), valid = 0u;
    if (v->decoded_ready && v->decoded_block == block) return 1;
    if (!a->sample_decode_budget) return -1;
    a->sample_decode_budget--;
    a->sample_decodes++;
    if (!mello_sample_decode_block(a->sample_bank, v->slot, block, a->sample_decode[v->slot], &valid))
        return 0;
    v->decoded_block = block;
    v->decoded_valid = valid;
    v->decoded_ready = true;
    return 1;
}
static int32_t sample_render_voice(MelloAudio *a, MelloAudioSampleVoice *v)
{
    const MelloSampleSlotConfig *c;
    uint32_t idx, frac, off;
    int32_t s0, s1, y;
    if (!a || !v || !v->active || v->env_stage == MELLO_ENV_OFF) return 0;
    c = &a->sample_config.slots[v->slot];
    if (v->slide_left) {
        v->step_q16 = (uint32_t)((int32_t)v->step_q16 + v->slide_step_q16);
        if (--v->slide_left == 0u) v->step_q16 = v->target_step_q16;
    }
    if (v->env_stage == MELLO_ENV_ATTACK) {
        (void)env_tick(&v->env_q24, v->env_step_q24, v->env_target_q24, &v->env_stage);
        if (v->env_stage == MELLO_ENV_DECAY) sample_begin_decay(a, v);
    } else if (v->env_stage == MELLO_ENV_DECAY || v->env_stage == MELLO_ENV_RELEASE) {
        tone_curve_tick(TONE_ENV(v));
        if (v->env_stage == MELLO_ENV_OFF) {
            v->active = false;
            return 0;
        }
    }
    idx = v->pos_q16 >> 16;
    if (idx >= v->trim_end_sample) {
        if (v->loop && v->env_stage != MELLO_ENV_OFF) {
            uint32_t span = v->trim_end_sample - v->trim_start_sample;
            if (span < 1u) span = 1u;
            idx = v->trim_start_sample + (idx - v->trim_start_sample) % span;
            v->pos_q16 = (idx << 16) | (v->pos_q16 & 0xFFFFu);
            /* The cached block stays valid: sample_decode_for only decodes when the block changes. */
        } else {
            v->active = false;
            v->env_stage = MELLO_ENV_OFF;
            v->env_q24 = 0;
            return 0;
        }
    }
    {
        int d = sample_decode_for(a, v, idx);
        if (d == 0) {
            v->active = false;
            return 0;
        }
        if (d < 0) { /* over budget (extreme pitch or a loop across a block edge): drop the frame */
            a->sample_decode_skips++;
            v->pos_q16 += v->step_q16;
            return 0;
        }
    }
    off = idx % MELLO_SAMPLE_BLOCK_SAMPLES;
    if (off >= v->decoded_valid) return 0;
    s0 = a->sample_decode[v->slot][off];
    s1 = s0;
    if (off + 1u < v->decoded_valid) s1 = a->sample_decode[v->slot][off + 1u];
    frac = v->pos_q16 & 0xFFFFu;
    y = s0 + ((s1 - s0) * (int32_t)(frac >> 8)) / 256;
    y = q15mul(y, v->env_q24 >> 8);
    y = q15mul(y, (int32_t)v->velocity * 258);
    if (c->dist)
        y = mul_q12(soft_drive(y, c->dist, 500u), a->sample_makeup_q12[v->slot]); /* as loud as without */
    y = y * (int32_t)c->level / 1000;
    v->pos_q16 += v->step_q16;
    return y;
}

#if MELLO_FEATURE_CHORD_MACHINE
#define CHORD_OVERSHOOT 21810381 /* 1.3 in Q24: an attack aims past 1 and stops there, as an analog one */
/* A one-pole rate for 16 samples, Q24, that covers ms in about tau time constants (x10). */
static int32_t chord_rate(uint16_t ms, uint32_t tau_x10)
{
    uint32_t n = (uint32_t)(ms ? ms : 1u) * 441u, k = (tau_x10 << 24) / n; /* n: samples x 10 */
    return k >= (1u << 20) ? (1 << 24) : (int32_t)(k * 16u);
}
/* The config's envelope rates and sustain, worked out once when it changes. */
static void chord_coeffs(MelloAudio *a)
{
    const MelloChordConfig *c = &a->chord_config;
    MelloAudioChordVoice *v = &a->chord;
    v->k_att = chord_rate(c->attack_ms, 15u); /* aims at 1.3, so it reaches 1 in about ATTACK */
    v->k_dec = chord_rate(c->decay_ms, 40u);
    v->k_rel = chord_rate(c->release_ms, 40u);
    v->k_fatt = chord_rate(c->fenv_attack_ms, 15u);
    v->k_fdec = chord_rate(c->fenv_decay_ms, 40u);
    v->sus_q24 = (int32_t)((uint32_t)c->sustain * 16777u);
    v->chorus_depth = (int32_t)c->chorus * 80 / 1000;
    v->chorus_mix_q15 = (int32_t)c->chorus * 16384 / 1000;
}
static void chord_release_voice(MelloAudioChordVoice *v, uint8_t i)
{
    if (v->active[i]) {
        v->active[i] = 0u;
        v->active_count--;
    }
    if (v->vstage[i]) v->vstage[i] = 3u;
}
static void chord_note_on(MelloAudio *a, uint8_t note)
{
    MelloAudioChordVoice *v = &a->chord;
    uint8_t i, pick = 0xFFu;
    uint8_t nv = a && a->chord_config.poly ? MELLO_CHORD_AUDIO_VOICES : MELLO_SUB_MELLO_VOICES;
    if (!a || !a->chord_config.on) return;
    for (i = 0u; i < nv; ++i) /* the same note again: that voice, from where it is */
        if (v->vstage[i] && v->note[i] == note) pick = i;
    for (i = 0u; pick == 0xFFu && i < nv; ++i)
        if (!v->vstage[i]) pick = i;
    if (pick == 0xFFu) { /* none free: the quietest released one, or else the oldest */
        int32_t low = 0x7FFFFFFF;
        uint32_t oldest = 0u;
        for (i = 0u; i < nv; ++i)
            if (v->vstage[i] == 3u && v->venv[i] < low) low = v->venv[i], pick = i;
        for (i = 0u; pick == 0xFFu && i < nv; ++i)
            if (v->age - v->vage[i] >= oldest) oldest = v->age - v->vage[i], pick = i;
        if (pick == 0xFFu) pick = 0u;
    }
    v->note[pick] = note; /* (the note as played: its NOTE_OFF finds the voice by it) */
    v->inc[pick] = note_inc(perf_note(a, note)); /* OCT- / OCT+ */
    if (!v->active[pick]) {
        v->active[pick] = 1u;
        v->active_count++;
    }
    v->vstage[pick] = 1u;
    v->vage[pick] = ++v->age;
    v->fstage = 1u; /* the filter envelope with every note, from where it is */
    v->env_stage = MELLO_ENV_ATTACK;
}
static void chord_note_off(MelloAudio *a, uint8_t note)
{
    uint8_t i;
    if (!a) return;
    for (i = 0u; i < MELLO_CHORD_AUDIO_VOICES; ++i)
        if (a->chord.active[i] && a->chord.note[i] == note) {
            chord_release_voice(&a->chord, i);
            break;
        }
}
static void chord_kill(MelloAudio *a)
{
    MelloAudioChordVoice *v;
    if (!a) return;
    v = &a->chord;
    memset(v->active, 0, sizeof v->active);
    memset(v->vstage, 0, sizeof v->vstage);
    memset(v->venv, 0, sizeof v->venv);
    memset(v->vgain, 0, sizeof v->vgain);
    memset(v->vstep, 0, sizeof v->vstep);
    memset(v->chorus_buf, 0, sizeof v->chorus_buf);
    v->active_count = 0u;
    v->env_q24 = 0;
    v->env_stage = MELLO_ENV_OFF;
    v->fenv = 0;
    v->lp = v->bp = v->hp_x1 = v->hp_y1 = 0;
}
/* Every 16 samples: the envelopes (each voice's gain then ramps to its new level over the 16) and the filter. mod_cut:
 * the routes on C.CUT now, permille (chord_block's, so it can run beside the render). */
static void chord_control(MelloAudio *a, int32_t mod_cut)
{
    MelloAudioChordVoice *v = &a->chord;
    const MelloChordConfig *c = &a->chord_config;
    int32_t peak = 0, q8, w;
    uint8_t i;
    for (i = 0u; i < MELLO_CHORD_AUDIO_VOICES; ++i) {
        int32_t e = v->venv[i];
        if (v->vstage[i] == 1u) {
            e += (int32_t)(((int64_t)(CHORD_OVERSHOOT - e) * v->k_att) >> 24);
            if (e >= 1 << 24) e = 1 << 24, v->vstage[i] = 2u;
        } else if (v->vstage[i] == 2u)
            e += (int32_t)(((int64_t)(v->sus_q24 - e) * v->k_dec) >> 24);
        else if (v->vstage[i] == 3u) {
            e -= (int32_t)(((int64_t)e * v->k_rel) >> 24);
            if (e < 4096) e = 0, v->vstage[i] = 0u;
        } else
            e = 0;
        v->venv[i] = e;
        v->vstep[i] = (e - v->vgain[i]) / 16;
        if (e > peak) peak = e;
        if (v->vgain[i] > peak) peak = v->vgain[i];
    }
    v->env_q24 = peak;
    if (!peak) v->env_stage = MELLO_ENV_OFF;
    if (v->fstage == 1u) {
        v->fenv += (int32_t)(((int64_t)(CHORD_OVERSHOOT - v->fenv) * v->k_fatt) >> 24);
        if (v->fenv >= 1 << 24) v->fenv = 1 << 24, v->fstage = 2u;
    } else
        v->fenv -= (int32_t)(((int64_t)v->fenv * v->k_fdec) >> 24);
    /* CUT 0..1000 = 60 Hz .. 6.4 kHz (81 semitones), the envelope up to 48 more, LINK as far as the lead's CUTOFF
     * from its middle; counted from 960 Hz (semi_ratio_q16 reaches +-48 semitones) */
    q8 = (int32_t)c->cut * 20736 / 1000 +
         (int32_t)(((int64_t)v->fenv * ((int32_t)c->fenv * 12288 / 1000)) >> 24) - 48 * 256;
    if (c->link) q8 += ((int32_t)a->config.filter_cut - 500) * 235 / 10;
    q8 += mod_cut * 20736 / 1000; /* DOT's MOD and EVOLVE's routes on C.CUT, as CUT's range */
    if (q8 > 33 * 256) q8 = 33 * 256;
    w = (int32_t)(((int64_t)semi_ratio_q16(q8) * 4482) >> 16); /* 2 pi fc / fs, Q15 */
    w -= (int32_t)((((int64_t)w * w >> 15) * w) >> 15) / 24;   /* 2 sin(w / 2) */
    v->f_q15 = clamp32(w, 40, 29000);
    v->damp_q15 =
        65536 - (int32_t)c->res * 62000 / 1000; /* damping 2 (RES 0) .. 0.1: just short of whistling */
}
/* One sample of the chord layer, left and right (before LEVEL). */
static void chord_render(MelloAudio *a, int32_t mod_cut, int32_t *out_l, int32_t *out_r)
{
    static const int16_t norm[5] = {0, 32767, 18500, 13200, 10400};
    MelloAudioChordVoice *v = &a->chord;
    const MelloChordConfig *c = &a->chord_config;
    int32_t x = 0, hp, wet_l, wet_r, m;
    uint8_t i;
    if (!v->k_att) chord_coeffs(a); /* before the first config */
    if (!(v->ctl++ & 15u)) chord_control(a, mod_cut);
    for (i = 0u; i < MELLO_CHORD_AUDIO_VOICES; ++i) {
        if (!v->vstage[i] && !v->vgain[i]) continue;
        v->vgain[i] += v->vstep[i];
        v->phase[i] += v->inc[i];
        x += q15mul(osc_sample(c->wave, v->phase[i], v->inc[i], 500u), v->vgain[i] >> 9);
    }
    if (c->poly)
        x = q15mul(clamp32(x, -131071, 131071) >> 1, norm[c->voices <= 4u ? c->voices : 4u]) * 2;
    else /* SUB MELLO: a voice as loud as one of a triad's, so LEVEL means the same for both */
        x = q15mul(clamp32(x, -65535, 65535), norm[3]);
    /* the filter: a state variable low-pass (Chamberlin), 12 dB */
    x = clamp32(x, -65535, 65535);
    v->lp = clamp32(v->lp + (int32_t)(((int64_t)v->f_q15 * v->bp) >> 15), -262143, 262143);
    hp = x - v->lp - (int32_t)(((int64_t)v->damp_q15 * v->bp) >> 15);
    v->bp = clamp32(v->bp + (int32_t)(((int64_t)v->f_q15 * hp) >> 15), -262143, 262143);
    x = clamp32(v->lp, -65535, 65535);
    /* ~120 Hz high-pass: the chords leave the bass to the sub */
    hp = q15mul(32210, clamp32(v->hp_y1 + x - v->hp_x1, -65535, 65535));
    v->hp_x1 = x;
    v->hp_y1 = hp;
    x = clamp32(hp, -32767, 32767);
    if (!c->poly || !c->chorus) { /* SUB MELLO has no chorus: the delay and the reverb give it its space */
        *out_l = *out_r = x;
        return;
    }
    {
        /* CHORUS, as a Juno's: one delay line read at 3.5 ms +- up to 1.8 ms, left and right swung against each
         * other by a triangle at 0.45 .. 0.85 Hz, mixed up to half and half with the dry signal. */
        uint32_t tri, d, pos, f;
        int32_t depth = v->chorus_depth, s0, s1; /* samples */
        v->chorus_buf[v->chorus_pos] = (int16_t)x;
        v->chorus_lfo += 43826u + (uint32_t)c->chorus * 39u;
        tri = v->chorus_lfo >> 16;
        tri = tri < 32768u ? tri * 2u : (65535u - tri) * 2u;                  /* 0..65535 */
        d = (uint32_t)((154 << 8) + ((((int32_t)tri - 32768) * depth) >> 7)); /* Q8 samples */
        pos = (uint32_t)v->chorus_pos + MELLO_CHORD_CHORUS_LEN * 2u - (d >> 8);
        f = d & 0xFFu;
        s0 = v->chorus_buf[pos % MELLO_CHORD_CHORUS_LEN];
        s1 = v->chorus_buf[(pos - 1u) % MELLO_CHORD_CHORUS_LEN];
        wet_l = s0 + (((s1 - s0) * (int32_t)f) >> 8);
        d = (uint32_t)((154 << 8) - ((((int32_t)tri - 32768) * depth) >> 7));
        pos = (uint32_t)v->chorus_pos + MELLO_CHORD_CHORUS_LEN * 2u - (d >> 8);
        f = d & 0xFFu;
        s0 = v->chorus_buf[pos % MELLO_CHORD_CHORUS_LEN];
        s1 = v->chorus_buf[(pos - 1u) % MELLO_CHORD_CHORUS_LEN];
        wet_r = s0 + (((s1 - s0) * (int32_t)f) >> 8);
        v->chorus_pos = (uint16_t)((v->chorus_pos + 1u) % MELLO_CHORD_CHORUS_LEN);
        m = v->chorus_mix_q15;
        *out_l = x + q15mul(wet_l - x, m);
        *out_r = x + q15mul(wet_r - x, m);
    }
}
#endif

static void dot_lane_release(MelloAudio *a, MelloDotLane *ln, uint8_t l)
{
    uint32_t n = ms_samples(a->modr.dot_release_ms[l]);
    if (n < 1u) n = 1u;
    ln->stage = 3u;
    ln->left = n;
    ln->step = -(ln->v / (int32_t)n);
    if (ln->step == 0) ln->step = -1;
}
static void dot_lane_attack(MelloAudio *a, MelloDotLane *ln, uint8_t l, int32_t peak)
{
    uint32_t n = ms_samples(a->modr.dot_attack_ms[l]);
    ln->peak = peak;
    if (n < 1u) {
        ln->v = peak;
        if (a->modr.dot_gate[l])
            ln->stage = 2u;
        else
            dot_lane_release(a, ln, l);
        return;
    }
    ln->stage = 1u;
    ln->left = n;
    ln->step = (peak - ln->v) / (int32_t)n;
}
/* A pulse's cell on a MOD lane. HIT: a lit cell starts an attack (from where the lane is, so hits in a row do not
 * click) and then the release. GATE: lit cells in a row hold, an unlit one releases. (The HTML's filter dot also
 * snapped to 0 on every unlit pulse.) */
static void dot_lane_pulse(MelloAudio *a, MelloDotLane *ln, uint8_t l, uint16_t lv)
{
    bool gate = a->modr.dot_gate[l];
    if (lv && (!gate || ln->stage == 0u || ln->stage == 3u || ln->peak != (int32_t)lv << 16))
        dot_lane_attack(a, ln, l, (int32_t)lv << 16);
    else if (!lv && gate && ln->stage && ln->stage != 3u)
        dot_lane_release(a, ln, l);
}
static void dot_lane_tick(MelloAudio *a, MelloDotLane *ln, uint8_t l)
{
    if (ln->stage == 1u) {
        ln->v += ln->step;
        if (--ln->left == 0u) {
            ln->v = ln->peak;
            if (a->modr.dot_gate[l])
                ln->stage = 2u;
            else
                dot_lane_release(a, ln, l);
        }
    } else if (ln->stage == 3u) {
        ln->v += ln->step;
        if (--ln->left == 0u || ln->v <= 0) ln->v = 0, ln->stage = 0u;
    }
}

/* Which note an ON/OFF pair belongs to. Tone's triggerAttack cancels the envelope's scheduled release, so in the HTML a
 * note that starts before the last one's gate ends plays legato: that NOTE_OFF must not release the new note. */
static uint32_t event_tag(const MelloEvent *e)
{
    return (uint32_t)e->step_index | (uint32_t)e->pulse_index << 8 | (uint32_t)e->ratchet_index << 16 |
           (uint32_t)e->note << 24;
}
static void apply_event(MelloAudio *a, const MelloEvent *e)
{
    if (!a || !e) return;
    if (e->type == MELLO_EVENT_PULSE) {
        uint8_t l;
        int32_t dm = 0;
        a->tm_filter_mod_q15 = e->tm_filter_mod_q15;
        a->tm_decay_mod_q15 = e->tm_decay_mod_q15;
        a->tm_value = e->tm_value;
        for (l = 0u; l < MELLO_DOT_MOD_LANES; ++l) { /* DOT's MOD lanes (dot_lane_pulse) */
            MelloDotLane *ln = &a->dot_lane[l];
            uint16_t lv = e->dot_mod[l];
            if (lv == MELLO_DOT_NOT_READ) continue;
            dot_lane_pulse(a, ln, l, lv);
            if (a->modr.dot_target[l] == MELLO_DEST_DECAY) dm += lv ? lv : ln->v >> 16;
        }
        for (l = 0u; l < MELLO_DOT_MOD_LANES; ++l)
            if (a->modr.tm_dest[l] == MELLO_DEST_DECAY) dm += a->tm_value * a->modr.tm_depth[l] / 32768;
        {
            /* The amp decay for this pulse's notes: EVOLVE's DECAY route (as the HTML, +-1 s), then the DECAY
             * destination, up to 4x longer or shorter at +-100%. */
            int32_t d = (int32_t)a->config.decay_ms;
            if (e->tm_decay_mod_q15) d = clamp32(d + ((int32_t)e->tm_decay_mod_q15 * 1000) / 32767, 5, 800);
            if (dm)
                d = (int32_t)inc_times((uint32_t)d, semi_ratio_q16(clamp32(dm, -1000, 1000) * 6144 / 1000));
            a->effective_decay_ms = (e->tm_decay_mod_q15 || dm) ? (uint16_t)clamp32(d, 5, 5000) : 0u;
        }
    } else if (e->type == MELLO_EVENT_NOTE_ON) {
        a->tm_filter_mod_q15 = e->tm_filter_mod_q15;
        a->tm_decay_mod_q15 = e->tm_decay_mod_q15;
        a->lead.on_tag = event_tag(e);
        lead_attack(a, perf_note(a, e->note), e->velocity,
                    e->slide); /* (OCT- / OCT+: the pitch, not the tag) */
        if (a->pfx & MELLO_PERF_FX2_PLUCK) a->pluck_up = true;
        a->lead.wave = e->wave;
    } else if (e->type == MELLO_EVENT_NOTE_OFF) {
        if (event_tag(e) == a->lead.on_tag) lead_release(a);
    } else if (e->type == MELLO_EVENT_SUB_NOTE_ON) {
        a->sub.on_tag = event_tag(e);
        sub_attack(a, perf_note(a, e->note), e->velocity, e->ratchet_index != 0u,
                   e->slide); /* ratchet_index: BASS SEQ's step */
        if (a->pfx & MELLO_PERF_FX2_PLUCK) a->pluck_up = true;
    } else if (e->type == MELLO_EVENT_SUB_NOTE_OFF) {
        if (event_tag(e) == a->sub.on_tag) sub_release(a);
    }
#if MELLO_FEATURE_CHORD_MACHINE
    else if (e->type == MELLO_EVENT_CHORD_NOTE_ON) { /* the note itself: chord_block, at the same sample */
        if (a->pfx & MELLO_PERF_FX2_PLUCK) a->pluck_up = true;
    }
#endif
    else if (e->type == MELLO_EVENT_DRUM_HIT) { /* the hit itself: parts_step, at the same sample */
        if (e->sample_slot == MELLO_DRUM_KICK && a->drum_config.on)
            a->duck_env = 1 << 24; /* DUCK: the music dips under this kick */
        if (a->pfx & MELLO_PERF_FX2_PLUCK) a->pluck_up = true;
    } else if (e->type == MELLO_EVENT_SAMPLE_NOTE_ON)
        sample_attack_voice(a, e);
    else if (e->type == MELLO_EVENT_SAMPLE_NOTE_OFF)
        sample_release_voice(a, e->sample_slot);
}

/* BASS SEQ's filter, used once RES or ENV is above 0: CUT as the HTML's (40 x 55^cut Hz), up to 4 octaves more by
 * ENV x the filter envelope (to -60 dB in ENV DECAY), 2 more by an accent's ACCENT; two state-variable low-passes,
 * the second with RES (Q 0.7 .. ~17). The frequency is worked out every 16 samples. */
static int32_t sub_svf(MelloAudio *a, int32_t x)
{
    MelloAudioSubVoice *s = &a->sub;
    const MelloAudioBassVoice *b = &a->bass_config;
    int32_t h;
    s->fenv -= (int32_t)(((int64_t)s->fenv * s->fenv_k) >> 24);
    if (!s->svf_ctl--) {
        int32_t q8 = (int32_t)a->sub_config.cut * 17766 / 1000 /* 69.4 semitones over CUT */
                     + (int32_t)(((int64_t)((int32_t)b->env * 12288 / 1000) * s->fenv) >> 24);
        uint32_t hz;
        if (s->accent) q8 += (int32_t)(((int64_t)((int32_t)b->accent * 6144 / 1000) * s->fenv) >> 24);
        s->fenv_k = (int32_t)(2625000u / (b->env_decay_ms < 20u ? 20u : b->env_decay_ms));
        if (q8 > 96 * 256) q8 = 96 * 256;
        hz = (640u * semi_ratio_q16(q8 - 48 * 256)) >> 16; /* 40 Hz at 0 */
        s->svf_f = (int32_t)(hz * 4669u / 1000u);
        if (s->svf_f > 30000) s->svf_f = 30000;
        s->svf_damp = 45875 - (int32_t)b->res * 44000 / 1000;
        s->svf_ctl = 15u;
    }
    s->svf_lp[0] =
        clamp32(s->svf_lp[0] + (int32_t)(((int64_t)s->svf_f * s->svf_bp[0]) >> 15), -1048576, 1048575);
    h = x - s->svf_lp[0] - (int32_t)(((int64_t)45875 * s->svf_bp[0]) >> 15);
    s->svf_bp[0] = clamp32(s->svf_bp[0] + (int32_t)(((int64_t)s->svf_f * h) >> 15), -1048576, 1048575);
    s->svf_lp[1] =
        clamp32(s->svf_lp[1] + (int32_t)(((int64_t)s->svf_f * s->svf_bp[1]) >> 15), -1048576, 1048575);
    h = s->svf_lp[0] - s->svf_lp[1] - (int32_t)(((int64_t)s->svf_damp * s->svf_bp[1]) >> 15);
    s->svf_bp[1] = clamp32(s->svf_bp[1] + (int32_t)(((int64_t)s->svf_f * h) >> 15), -1048576, 1048575);
    return clamp32(s->svf_lp[1], -65535, 65535);
}

static int32_t sub_biquad_step(int32_t x, const ToneSubBiquadCoeff *c, int32_t *x1, int32_t *x2, int32_t *y1,
                               int32_t *y2)
{
    int64_t acc;
    int32_t y;
    acc = (int64_t)c->b0 * x + (int64_t)c->b1 * (*x1) + (int64_t)c->b2 * (*x2) - (int64_t)c->a1 * (*y1) -
          (int64_t)c->a2 * (*y2);
    y = (int32_t)(acc >> 28);
    y = clamp32(y, -131071, 131071);
    *x2 = *x1;
    *x1 = x;
    *y2 = *y1;
    *y1 = y;
    return y;
}
/* The main filter's coefficients for cut and res (permille), interpolated between MAIN_FILTER_BIQUAD_Q28's
 * 0.01-cut and 0.05-res steps. Only recomputed when cut or res changes (per sample under LFO/TM/matrix). */
static void main_filter_coeffs(MelloAudio *a, uint16_t cut, uint16_t res)
{
    uint16_t ci, ri, ci1, ri1;
    int32_t cf, rf;
    const ToneSubBiquadCoeff *t00, *t01, *t10, *t11;
    int64_t v[5][4];
    uint8_t k;
    if (a->mf_coeff_valid && a->mf_cut == cut && a->mf_res == res && a->mf_mode == a->config.filter_mode)
        return;
    if (cut > 1000u) cut = 1000u;
    if (res > 1000u) res = 1000u;
    ci = (uint16_t)(cut / 10u);
    cf = (int32_t)(cut % 10u); /* tenths of a step */
    ri = (uint16_t)(res / 50u);
    rf = (int32_t)(res % 50u); /* fiftieths of a step */
    ci1 = ci + 1u < MAIN_FILTER_CUTS ? (uint16_t)(ci + 1u) : ci;
    ri1 = ri + 1u < MAIN_FILTER_RESES ? (uint16_t)(ri + 1u) : ri;
    t00 = &MAIN_FILTER_BIQUAD_Q28[ri][ci];
    t01 = &MAIN_FILTER_BIQUAD_Q28[ri][ci1];
    t10 = &MAIN_FILTER_BIQUAD_Q28[ri1][ci];
    t11 = &MAIN_FILTER_BIQUAD_Q28[ri1][ci1];
    v[0][0] = t00->b0, v[0][1] = t01->b0, v[0][2] = t10->b0, v[0][3] = t11->b0;
    v[1][0] = t00->b1, v[1][1] = t01->b1, v[1][2] = t10->b1, v[1][3] = t11->b1;
    v[2][0] = t00->b2, v[2][1] = t01->b2, v[2][2] = t10->b2, v[2][3] = t11->b2;
    v[3][0] = t00->a1, v[3][1] = t01->a1, v[3][2] = t10->a1, v[3][3] = t11->a1;
    v[4][0] = t00->a2, v[4][1] = t01->a2, v[4][2] = t10->a2, v[4][3] = t11->a2;
    for (k = 0u; k < 5u; ++k) {
        /* Only multiplies: no 64-bit division on the target. 1/10 and 1/50 as 6554/65536 and 1311/65536. */
        int64_t lo = v[k][0] + (((v[k][1] - v[k][0]) * cf * 6554) >> 16);
        int64_t hi = v[k][2] + (((v[k][3] - v[k][2]) * cf * 6554) >> 16);
        a->mf_coeff[k] = (int32_t)(lo + (((hi - lo) * rf * 1311) >> 16));
    }
    if (a->config.filter_mode != MELLO_FILTER_LP) {
        /* The other modes share the lowpass's poles (a1, a2) and so its cutoff and resonance; only the zeros
         * differ. With 1/a0 = (1 + a2) / 2, cos/a0 = -a1 / 2 and alpha/a0 = (1 - a2) / 2 (Q28):
         * HP b = (1 + cos)/2a0 [1 -2 1], BP b = alpha/a0 [1 0 -1], notch b = 1/a0 [1 -2cos 1]. */
        int32_t one = 1 << 28, a1 = a->mf_coeff[3], a2 = a->mf_coeff[4];
        if (a->config.filter_mode == MELLO_FILTER_HP) {
            int32_t b0 = (int32_t)(((int64_t)one + a2 - a1) / 4);
            a->mf_coeff[0] = b0, a->mf_coeff[1] = -2 * b0, a->mf_coeff[2] = b0;
        } else if (a->config.filter_mode == MELLO_FILTER_BP) {
            int32_t b0 = (one - a2) / 2;
            a->mf_coeff[0] = b0, a->mf_coeff[1] = 0, a->mf_coeff[2] = -b0;
        } else {
            int32_t b0 = (int32_t)(((int64_t)one + a2) / 2);
            a->mf_coeff[0] = b0, a->mf_coeff[1] = a1, a->mf_coeff[2] = b0;
        }
    }
    a->mf_mode = (uint8_t)a->config.filter_mode;
    a->mf_cut = cut;
    a->mf_res = res;
    a->mf_coeff_valid = true;
}
/* One biquad of the main filter, direct form I. States may exceed full scale (both stages together peak at
 * +30 dB at res 1: 65535 x 31.6 < 2^21), so they are kept within +-2^21. (Not 2^22: -2^22 is a ROM address
 * to tools/build.py's check.) */
static int32_t main_biquad_step(const int32_t *c, int32_t *s, int32_t x)
{
    int64_t acc = (int64_t)c[0] * x + (int64_t)c[1] * s[0] + (int64_t)c[2] * s[1] - (int64_t)c[3] * s[2] -
                  (int64_t)c[4] * s[3];
    /* Rounded, not floored: a floored >> 28 leaves a DC limit cycle of a few LSB in the tails. */
    int32_t y = clamp32((int32_t)((acc + (1 << 27)) >> 28), -(1 << 21), 1 << 21);
    s[1] = s[0];
    s[0] = x;
    s[3] = s[2];
    s[2] = y;
    return y;
}
/* Tone.Filter lowpass -24: two identical biquads (the FX tone filters, Q 0.2 dB). */
static int32_t tone_lp24(const ToneSubBiquadCoeff *c, int32_t s[2][4], int32_t x)
{
    const int32_t k[5] = {c->b0, c->b1, c->b2, c->a1, c->a2};
    return main_biquad_step(k, s[1], main_biquad_step(k, s[0], x));
}
static int16_t tone_distortion_point(uint16_t amount, uint16_t index)
{
    /* Tone.Distortion waveshaper from Tone.js 14. amount is permille here. */
    const uint32_t PI_Q16 = 205887u, DEG20_Q16 = 22876u;
    uint32_t k = (uint32_t)amount / 10u;
    uint32_t x = index >= 256u ? 32767u : (uint32_t)index * 128u;
    uint32_t den = PI_Q16 + k * x * 2u;
    uint64_t num = (uint64_t)(3u + k) * x * DEG20_Q16;
    uint32_t y = den ? (uint32_t)((num + den / 2u) / den) : 0u;
    if (y > 32767u) y = 32767u;
    return (int16_t)y;
}
static void build_sub_dist_curve_for_drive(int16_t curve[257], uint16_t drive)
{
    uint16_t i;
    if (!curve) return;
    for (i = 0u; i <= 256u; ++i) curve[i] = tone_distortion_point(drive, i);
}
/* A Tone.Distortion curve (odd, 257 points over 0..1) applied to x; the input is clamped to +-1 as a
 * WaveShaperNode clamps it. */
static int32_t tone_shape(const int16_t curve[257], int32_t x)
{
    uint32_t ax, idx, frac;
    int32_t y0, y1, y;
    x = clamp32(x, -32767, 32767);
    ax = (uint32_t)(x < 0 ? -x : x);
    idx = ax >> 7;
    if (idx > 255u) idx = 255u;
    frac = ax & 127u;
    y0 = curve[idx];
    y1 = curve[idx + 1u];
    y = y0 + ((y1 - y0) * (int32_t)frac) / 128;
    return x < 0 ? -y : y;
}
static int32_t sub_tone_distort(const MelloAudio *a, int32_t x)
{
    if (!a || !a->sub_config.drive) return x;
    return tone_shape(a->sub_dist_curve, x);
}
/* The level the distortion's gain compensation holds: a sine at 0.4 (-8 dBFS), about a full lead note there. */
#define MAIN_DIST_MAKEUP_REF 13107
static const uint16_t AMT_POW19_PERMILLE[101] = {
    /* 1000 * (i/100)^1.9: distDriveFromAmount's amt^1.9 */
    0,   0,   1,   1,   2,   3,   5,   6,   8,   10,  13,  15,  18,  21,  24,  27,  31,   35,  38,  43,  47,
    52,  56,  61,  66,  72,  77,  83,  89,  95,  102, 108, 115, 122, 129, 136, 144, 151,  159, 167, 175, 184,
    192, 201, 210, 219, 229, 238, 248, 258, 268, 278, 289, 299, 310, 321, 332, 344, 355,  367, 379, 391, 403,
    416, 428, 441, 454, 467, 481, 494, 508, 522, 536, 550, 564, 579, 594, 609, 624, 639,  654, 670, 686, 702,
    718, 734, 751, 768, 784, 801, 819, 836, 853, 871, 889, 907, 925, 944, 962, 981, 1000,
};
/* The HTML's main distortion: Tone.Distortion(distDriveFromAmount(amt, char)), where
 * distDriveFromAmount = clamp(amt^1.9 * (0.28 + 0.52 char), 0, 0.8). Built on the main loop, not in audio. */
static void build_main_dist_curve(int16_t curve[257], uint16_t amount, uint16_t character)
{
    uint32_t a = amount > 1000u ? 1000u : amount, c = character > 1000u ? 1000u : character, i = a / 10u,
             f = a % 10u;
    uint32_t p = AMT_POW19_PERMILLE[i];
    uint32_t drive;
    if (f && i < 100u) p += ((uint32_t)(AMT_POW19_PERMILLE[i + 1u] - p) * f) / 10u;
    drive = (p * (280u + (520u * c) / 1000u)) / 1000u;
    if (drive > 800u) drive = 800u;
    build_sub_dist_curve_for_drive(curve, (uint16_t)drive);
    if (drive) {
        /* Gain compensation (DECISIONS.md, not in the HTML): the curve is scaled so a sine at the reference
         * level keeps the mean |y| it has at AMT 0 (y = x/3). Without it AMT 100 is ~11 dB louder there and
         * up to ~16 dB in quiet tails. Built in the main loop, so the render pays nothing. */
        uint32_t ref = 0u, got = 0u, g, k;
        for (k = 0u; k < 64u; ++k) {
            int32_t x = (SINE_Q15[k * 4u + 2u] * MAIN_DIST_MAKEUP_REF) >> 15;
            int32_t y = tone_shape(curve, x);
            ref += (uint32_t)(x < 0 ? -x : x);
            got += (uint32_t)(y < 0 ? -y : y);
        }
        ref /= 3u;
        if (got > ref) {
            g = (ref << 10) / (got >> 5); /* Q15; ref < 2^19, got >> 5 keeps ~13 bits */
            for (k = 0u; k <= 256u; ++k) curve[k] = (int16_t)(((int32_t)curve[k] * (int32_t)g + 16384) >> 15);
        }
    }
}

/* Distortion keeps its level (DECISIONS "Distortion keeps its level", 2026-11-03): each drive scaled so a sine at a
 * reference level keeps the mean |y| it has without the drive, as the main DIST's curve is (above). Built on the main
 * loop when the config changes; the render only multiplies. Q12, held to 1/8 .. 2x. */
static int32_t makeup_from(uint32_t want, uint32_t got)
{
    uint32_t g;
    want >>= 4, got >>= 4; /* sums of 64 |samples| up to 2^22: Q12 of them fits 32 bits */
    if (!got) return 4096;
    g = (want << 12) / got;
    return (int32_t)(g < 512u ? 512u : (g > 8192u ? 8192u : g));
}
/* soft_drive at `amount` against no drive, on a sine at `ref` */
static int32_t soft_drive_makeup_q12(uint16_t amount, int32_t ref)
{
    uint32_t want = 0u, got = 0u, k;
    for (k = 0u; k < 64u; ++k) {
        int32_t x = (SINE_Q15[k * 4u + 2u] * ref) >> 15, w = x, y = soft_drive(x, amount, 500u);
        want += (uint32_t)(w < 0 ? -w : w);
        got += (uint32_t)(y < 0 ? -y : y);
    }
    return makeup_from(want, got);
}
/* The kit's DRIVE at 0, 62.5 .. 1000 with its FOLD, against no drive, on a sine at about a hit's level */
#define DRUM_MAKEUP_DECAY                                                                                    \
    80 /* the hit's decay a step: -0.2 .. -1.2 dB over DRIVE, with or without FOLD (drive_makeup) */
/* The kit's DRIVE at 0, 62.5 .. 1000 with its FOLD, against no drive, on a hit: a sine dying away from about a
 * kick's peak, so the attack and the tail the drive lifts both count */
static void drums_makeup(int16_t t[17], uint16_t fold)
{
    int32_t fold_q12 = permille_q12(fold);
    uint32_t i, k;
    for (i = 0u; i <= 16u; ++i) {
        uint32_t want = 0u, got = 0u;
        uint16_t drive = (uint16_t)(i * 1000u / 16u);
        int32_t env = 30000;
        for (k = 0u; k < 256u; ++k) {
            int32_t x = (SINE_Q15[(k * 8u + 2u) & 255u] * env) >> 15,
                    y = drum_drive(x, drive, fold, fold_q12);
            want += (uint32_t)(x < 0 ? -x : x);
            got += (uint32_t)(y < 0 ? -y : y);
            env -= env / DRUM_MAKEUP_DECAY;
        }
        t[i] = (int16_t)(drive | fold ? makeup_from(want, got) : 4096);
    }
}
static void drives_makeup(MelloAudio *a)
{
    uint8_t s;
    drums_makeup(a->pending_drum_makeup_q12, a->pending_drum_config.fold);
    for (s = 0u; s < MELLO_SAMPLE_SLOT_COUNT; ++s)
        a->pending_sample_makeup_q12[s] =
            (int16_t)(a->pending_sample_config.slots[s].dist
                          ? soft_drive_makeup_q12(a->pending_sample_config.slots[s].dist, 16384)
                          : 4096);
}

/* A loop's gain for a 60 dB decay in DECAY's time, g = exp(-6.908 L / (fs DECAY)), as a Pade approximant. DECAY is the
 * time to -60 dB (DECISIONS "DECAY in seconds"): Tone.Reverb's (the HTML's) was far shorter than its value. */
static uint16_t reverb_feedback_q15(uint32_t loop_samples, uint16_t decay_ms)
{
    uint32_t d = decay_ms < 400u ? 400u : decay_ms > 60000u ? 60000u : decay_ms;
    uint32_t x = (uint32_t)(((uint64_t)5133u * loop_samples + d / 2u) / d);
    uint32_t x2 = (uint32_t)(((uint64_t)x * x + 32768u) / (2u * 32768u));
    uint32_t den = 32768u + x + x2;
    uint32_t fb = (uint32_t)(((uint64_t)32768u * 32768u + den / 2u) / den);
    if (fb > 32720u) fb = 32720u;
    if (fb < 4096u) fb = 4096u;
    return (uint16_t)fb;
}

/*
 * Reverb V3 candidate A: dense 8-line FDN inside the existing target ring.
 *
 * Design goals are perceptual first: faster early diffusion, substantially more
 * late-field modes, frequency-dependent feedback loss and a decorrelated stereo
 * return.  The network deliberately reuses the existing reverb ring and avoids
 * time-varying delay modulation in candidate A; modulation is only added later if
 * controlled listening still exposes stationary metallic modes.
 */
/* The network runs at fs / 2: the reverb's tone filter cuts at 4.2 kHz (12.6 kHz at most), so nothing it passes is
 * lost, and the network was half the sound's cost (32.7 of 68 M instructions a second on the FM-1). */
#define RV3_RATE_DIV    2u
#define RV3_LAYOUT_FULL 24166u
/* network samples: the pre-delay a chunk or more, so the network can run a chunk ahead (parts_reverb; Oliverb's) */
#define OLIVERB_PRE_MIN (MELLO_PARTS_CHUNK / RV3_RATE_DIV)
#define RV3_PRE_FULL    5293u
#define RV3_AP1_FULL    353u
#define RV3_AP2_FULL    521u
#define RV3_AP3_FULL    997u
#define RV3_L0_FULL     1151u
#define RV3_L1_FULL     1429u
#define RV3_L2_FULL     1699u
#define RV3_L3_FULL     1999u
#define RV3_L4_FULL     2243u
#define RV3_L5_FULL     2503u
#define RV3_L6_FULL     2791u
#define RV3_L7_FULL     3187u

/* The lines' delay modulation: depth in Q8 samples, rates 0.31..1.27 Hz (2^32 / 44100 per Hz), none a multiple
 * of another. */
#define RV3_MOD_DEPTH_Q8 (20 * 256 / RV3_RATE_DIV)
/* Thiran's first-order allpass coefficient for a fractional delay of 0.5 + k / 256 samples: (1 - d) / (1 + d), Q15. */
static const int16_t RV3_ETA_Q15[256] = {
    10923, 10809, 10696, 10584, 10472, 10361, 10251, 10140, 10031, 9922,  9814,  9706,  9599,  9492,  9386,
    9280,  9175,  9070,  8966,  8863,  8760,  8657,  8555,  8454,  8353,  8252,  8152,  8052,  7953,  7855,
    7757,  7659,  7562,  7465,  7369,  7273,  7178,  7083,  6988,  6894,  6801,  6708,  6615,  6523,  6431,
    6340,  6249,  6158,  6068,  5978,  5889,  5800,  5712,  5624,  5536,  5449,  5362,  5276,  5190,  5104,
    5019,  4934,  4849,  4765,  4681,  4598,  4515,  4432,  4350,  4268,  4186,  4105,  4024,  3944,  3863,
    3784,  3704,  3625,  3546,  3468,  3390,  3312,  3235,  3158,  3081,  3004,  2928,  2852,  2777,  2702,
    2627,  2552,  2478,  2404,  2331,  2258,  2185,  2112,  2040,  1967,  1896,  1824,  1753,  1682,  1612,
    1541,  1471,  1401,  1332,  1263,  1194,  1125,  1057,  989,   921,   854,   786,   719,   653,   586,
    520,   454,   389,   323,   258,   193,   129,   64,    0,     -64,   -128,  -191,  -254,  -317,  -380,
    -442,  -504,  -566,  -628,  -689,  -750,  -811,  -872,  -933,  -993,  -1053, -1113, -1172, -1232, -1291,
    -1350, -1409, -1467, -1526, -1584, -1641, -1699, -1757, -1814, -1871, -1928, -1984, -2040, -2097, -2153,
    -2208, -2264, -2319, -2374, -2429, -2484, -2539, -2593, -2647, -2701, -2755, -2809, -2862, -2915, -2968,
    -3021, -3074, -3126, -3179, -3231, -3283, -3334, -3386, -3437, -3488, -3539, -3590, -3641, -3691, -3742,
    -3792, -3842, -3892, -3941, -3991, -4040, -4089, -4138, -4187, -4235, -4284, -4332, -4380, -4428, -4476,
    -4524, -4571, -4618, -4665, -4712, -4759, -4806, -4852, -4899, -4945, -4991, -5037, -5083, -5128, -5174,
    -5219, -5264, -5309, -5354, -5399, -5444, -5488, -5532, -5576, -5620, -5664, -5708, -5752, -5795, -5838,
    -5881, -5924, -5967, -6010, -6053, -6095, -6137, -6180, -6222, -6264, -6306, -6347, -6389, -6430, -6471,
    -6513,
};
#define RV3_MOD_EVERY 16u
#define RV3_GLIDE_Q8  32u /* a line's length moves 32/256 of a sample every RV3_MOD_EVERY network samples */
static const uint32_t RV3_MOD_INC[8] = {30190u, 40904u, 50643u, 60383u, 71096u, 83758u, 98367u, 123690u};

static uint16_t rv3_scaled_cap(uint32_t total, uint16_t full)
{
    uint32_t scale, c;
    if (total >= RV3_LAYOUT_FULL) return full;
    scale = (total * 1000u) / RV3_LAYOUT_FULL;
    c = ((uint32_t)full * scale) / 1000u;
    if (c < 8u) c = 8u;
    return (uint16_t)c;
}
static uint16_t rv3_tap(uint16_t cap, uint16_t lo, uint16_t hi, uint16_t size)
{
    uint32_t pm, d;
    if (cap < 4u) return 1u;
    if (size > 1000u) size = 1000u;
    pm = (uint32_t)lo + (((uint32_t)(hi - lo) * size + 500u) / 1000u);
    d = ((uint32_t)cap * pm + 500u) / 1000u;
    if (d < 3u) d = 3u;
    if (d >= cap) d = cap - 1u;
    if ((d & 1u) == 0u) {
        if (d + 1u < cap)
            d += 1u;
        else if (d > 3u)
            d -= 1u;
    }
    return (uint16_t)d;
}
static void reverb_configure(MelloAudio *a)
{
    static const uint16_t full_line[8] = {RV3_L0_FULL, RV3_L1_FULL, RV3_L2_FULL, RV3_L3_FULL,
                                          RV3_L4_FULL, RV3_L5_FULL, RV3_L6_FULL, RV3_L7_FULL};
    static const uint16_t lo_pm[8] = {300u, 320u, 340u, 360u, 380u, 400u, 420u, 440u};
    static const uint16_t hi_pm[8] = {870u, 890u, 905u, 920u, 935u, 945u, 955u, 965u};
    uint16_t i;
    uint32_t off, sum = 0u, pre;
    if (!a || !a->reverb_capacity) return;
    /* The network runs at half the rate (RV3_RATE_DIV): the same delay times in half the samples. */
    a->reverb_pre_cap = rv3_scaled_cap(a->reverb_capacity / RV3_RATE_DIV, RV3_PRE_FULL);
    a->reverb_ap_cap[0] = rv3_scaled_cap(a->reverb_capacity / RV3_RATE_DIV, RV3_AP1_FULL);
    a->reverb_ap_cap[1] = rv3_scaled_cap(a->reverb_capacity / RV3_RATE_DIV, RV3_AP2_FULL);
    a->reverb_ap_cap[2] = rv3_scaled_cap(a->reverb_capacity / RV3_RATE_DIV, RV3_AP3_FULL);
    for (i = 0u; i < 8u; ++i)
        a->reverb_line_cap[i] = rv3_scaled_cap(a->reverb_capacity / RV3_RATE_DIV, full_line[i]);

    off = a->reverb_pre_cap;
    for (i = 0u; i < 3u; ++i) {
        a->reverb_ap_offset[i] = (uint16_t)off;
        off += a->reverb_ap_cap[i];
    }
    for (i = 0u; i < 8u; ++i) {
        a->reverb_line_offset[i] = (uint16_t)off;
        off += a->reverb_line_cap[i];
    }
    if (off + (off + 1u) / 2u > a->reverb_room) { /* (and the 8 bits under each sample: reverb_lo) */
        a->reverb_pre_cap = 0u;
        return;
    }
    a->reverb_lo = (uint8_t *)(a->reverb_buffer + off);

    pre = ms_samples(a->fx.reverb_pre_ms) / RV3_RATE_DIV;
    if (a->oliverb_on && pre < OLIVERB_PRE_MIN)
        pre = OLIVERB_PRE_MIN; /* (a chunk ahead on the second core) */
    if (pre >= a->reverb_pre_cap) pre = a->reverb_pre_cap ? a->reverb_pre_cap - 1u : 0u;
    a->reverb_pre_samples = (uint16_t)pre;
    a->reverb_ap_delay[0] = rv3_tap(a->reverb_ap_cap[0], 430u, 790u, a->fx.reverb_size);
    a->reverb_ap_delay[1] = rv3_tap(a->reverb_ap_cap[1], 370u, 730u, a->fx.reverb_size);
    a->reverb_ap_delay[2] = rv3_tap(a->reverb_ap_cap[2], 310u, 690u, a->fx.reverb_size);
    for (i = 0u; i < 8u; ++i) {
        a->reverb_line_delay[i] = rv3_tap(a->reverb_line_cap[i], lo_pm[i], hi_pm[i], a->fx.reverb_size);
        a->reverb_line_feedback_q15[i] =
            reverb_feedback_q15((uint32_t)a->reverb_line_delay[i] * RV3_RATE_DIV, a->fx.reverb_decay_ms);
        sum += a->reverb_line_feedback_q15[i];
        if (a->reverb_line_pos[i] >= a->reverb_line_cap[i]) a->reverb_line_pos[i] = 0u;
    }
    for (i = 0u; i < 3u; ++i)
        if (a->reverb_ap_pos[i] >= a->reverb_ap_cap[i]) a->reverb_ap_pos[i] = 0u;
    if (a->reverb_pos >= a->reverb_pre_cap) a->reverb_pos = 0u;
    if (a->reverb_out_pos >= a->reverb_pre_cap) a->reverb_out_pos = a->reverb_pos; /* (the same step count) */
    a->reverb_feedback_q15 = (uint16_t)(sum / 8u);
    a->reverb_delay_a = a->reverb_line_delay[0];
    a->reverb_delay_b = a->reverb_line_delay[7];
    /* A SIZE changed while the tail rings (a STYLE roll, a knob) moved every read at once: the tail broke, a click
     * that then went round the lines (DECISIONS "The reverb glides"). Now the lines glide to their new lengths
     * (reverb_network) and the diffusers take theirs once the tail has died away; silent, at once. */
    for (i = 0u; i < 8u; ++i)
        if (!a->reverb_live || !a->reverb_line_dq8[i])
            a->reverb_line_dq8[i] = (uint32_t)a->reverb_line_delay[i] << 8;
    for (i = 0u; i < 3u; ++i)
        if (!a->reverb_live || !a->reverb_ap_now[i]) a->reverb_ap_now[i] = a->reverb_ap_delay[i];
}
/* Oliverb's settings (DECISIONS "Oliverb", "Oliverb's knobs"): SIZE as Parasites' (0.05 + 0.94 x SIZE), DECAY as V3's
 * lines (the loop's gain a half for DECAY's time to -60 dB: a half is, on average, its two allpasses and its delay at
 * SIZE), its own knobs as Parasites' Oliverb mode turns them (DIFFUSE its STEREO SPREAD, SWING its REVERB, RATE its
 * FEEDBACK, SHIMMER its PITCH), the rest Parasites' at rest (TEXTURE at noon); TONE after it, as V3's. A new project's
 * knobs give Parasites' Init: diffusion 0.625, the swing 30 samples at 32 kHz, its rate 0.5, no shimmer. */
#define OLIVERB_IN_GAIN                                                                                      \
    8118 /* the input's gain: Parasites' 0.5, -6.1 dB to V3's level at the default project's */
#define OLIVERB_LP 32077 /* lp 0.93 at 32 kHz, the same corner here (1 - 0.07^(32000 / 22050)) */
#define OLIVERB_HP 93    /* 10 Hz, to keep DC out (Parasites' 0.01, 51 Hz, cut the bass's tail short) */
#define OLIVERB_SWING_Q16                                                                                    \
    1354752u /* SWING 10%: 30 samples at 32 kHz (REVERB at 0.1), 20.7 here, Q16 (300 at 100%) */
#define OLIVERB_SLOPE_MAX                                                                                    \
    121739u /* RATE 100%: the LFOs' step, FEEDBACK^6 / 200 at 32 kHz, x 32000 / 22050 here, Q24 */
#define OLIVERB_HALF                                                                                         \
    4844u /* a half of the loop at SIZE 1, network samples: (863+1198+2350+1043+939+3295) / 2 */
#define OLIVERB_MAKEUP 32995u /* 10^(0.06 / 20), Q15 */
static void oliverb_settings(MelloAudio *a)
{
    const MelloFxConfig *f = &a->fx;
    uint32_t size = f->reverb_size > 1000u ? 1000u : f->reverb_size, size_q16, half, g,
             dif = f->olv_diffuse > 1000u ? 1000u : f->olv_diffuse,
             sw = f->olv_swing > 1000u ? 1000u : f->olv_swing, r = f->olv_rate > 1000u ? 1000u : f->olv_rate,
             r3, q30, slope;
    int32_t st = f->olv_shimmer < -24 ? -24 : f->olv_shimmer > 24 ? 24 : f->olv_shimmer;
    if (!a->oliverb_on) return;
    size_q16 = 3277u + size * 61604u / 1000u;
    half = (OLIVERB_HALF * size_q16) >> 16;
    /* the loop loses about 0.06 dB a half besides its gain (the low-pass, the Hermite reads): made up, under 1.0 */
    g = (reverb_feedback_q15(half * RV3_RATE_DIV, f->reverb_decay_ms) * OLIVERB_MAKEUP) >> 15;
    /* RATE^6 without a 64-bit divide: RATE^3 in Q30 (2^30 / 10^9 = 1125900 / 2^20), squared */
    r3 = r * r * r;
    q30 = (uint32_t)(((uint64_t)r3 * 1125900u) >> 20);
    slope = (uint32_t)(((((uint64_t)q30 * q30) >> 30) * OLIVERB_SLOPE_MAX) >> 30);
    mello_oliverb_set(&a->oliverb, (int32_t)size_q16, (int32_t)((dif * 16384u + 9830400u) / 1000u),
                      (int32_t)(g > 32740u ? 32740u : g), OLIVERB_IN_GAIN, OLIVERB_LP, OLIVERB_HP,
                      (int32_t)(sw * OLIVERB_SWING_Q16 / 100u), slope, st ? semi_ratio_q16(st * 256) : 65536u,
                      st ? 32768 : 0); /* (Parasites': the shimmer whole from a semitone) */
}
/* Which reverb (at a chunk's start): Oliverb while the second core takes the parts' jobs, V3 with one core, once the
 * second is lost, or with Oliverb off. They share the memory after the pre-delay line, so a change empties it: the
 * tail goes (once, at power-on or on a fault), the input runs on. */
static void reverb_choose(MelloAudio *a)
{
    bool on = a->cores && !a->oliverb_off && a->reverb_buffer && a->reverb_capacity > 1024u &&
              a->reverb_pre_cap > OLIVERB_PRE_MIN &&
              a->reverb_pre_cap + MELLO_OLIVERB_WORDS <= a->reverb_room;
    uint8_t k;
    if (on == a->oliverb_on) return;
    a->oliverb_on = on;
    if (on)
        mello_oliverb_init(&a->oliverb, a->reverb_buffer + a->reverb_pre_cap, 0u);
    else if (a->reverb_buffer && a->reverb_room > a->reverb_pre_cap) {
        memset(a->reverb_buffer + a->reverb_pre_cap, 0,
               (a->reverb_room - a->reverb_pre_cap) * sizeof a->reverb_buffer[0]); /* (not the delay's) */
        memset(a->reverb_mod_y, 0, sizeof a->reverb_mod_y);
        for (k = 0u; k < 8u; ++k) a->reverb_line_dq8[k] = (uint32_t)a->reverb_line_delay[k] << 8;
        for (k = 0u; k < 3u; ++k) a->reverb_ap_now[k] = a->reverb_ap_delay[k];
    }
    reverb_configure(a); /* (PRE: Oliverb's at least a chunk) */
    oliverb_settings(a);
}
/* V3's eight lines carry 8 bits under each sample (reverb_lo), as their arithmetic does (Q8, RV3_Q8_MAX): in 16 bits
 * a quiet tail went round on its rounding, a floor of 6 samples at a 2.5 s decay and more as the gain nears 1. */
#define RV3_Q8_MAX (32767 * 256)
static inline int32_t rv3_rd(const MelloAudio *a, uint32_t i)
{
    return (int32_t)a->reverb_buffer[i] * 256 + a->reverb_lo[i];
}
static inline void rv3_wr(MelloAudio *a, uint32_t i, int32_t v)
{
    int32_t h;
    v = clamp32(v, -32768 * 256, RV3_Q8_MAX + 255);
    h = v >> 8; /* (floor: hi x 256 + lo is v) */
    a->reverb_buffer[i] = (int16_t)h, a->reverb_lo[i] = (uint8_t)(v - h * 256);
}
static int32_t rv3_allpass(MelloAudio *a, uint16_t which, int32_t x, uint16_t g)
{
    uint16_t cap, delay, pos, rp, off;
    int32_t z, y, w;
    cap = a->reverb_ap_cap[which];
    delay = a->reverb_ap_now[which];
    pos = a->reverb_ap_pos[which];
    off = a->reverb_ap_offset[which];
    if (cap < 4u || delay < 1u) return x;
    rp = pos >= delay ? (uint16_t)(pos - delay) : (uint16_t)(pos + cap - delay);
    z = a->reverb_buffer[(uint32_t)off + rp];
    y = z - q15mul_tz(x, g); /* (towards zero: a floor kept a sample going round) */
    y = clamp32(y, -32767, 32767);
    w = x + q15mul_tz(y, g);
    a->reverb_buffer[(uint32_t)off + pos] = sat16(w);
    pos++;
    if (pos >= cap) pos = 0u;
    a->reverb_ap_pos[which] = pos;
    return y;
}
/* The network's input: the pre-delay line written at reverb_pos (the render's, reverb_in). */
static void reverb_pre_write(MelloAudio *a, int32_t send)
{
    uint32_t p = a->reverb_pos;
    a->reverb_buffer[p] = sat16(send);
    if (++p >= a->reverb_pre_cap) p = 0u;
    a->reverb_pos = p;
}
/* The network from its pre-delay on: the line read PRE behind its write (reverb_out_pos: the same step count), or
 * with PRE 0 this step's input itself. With PRE at least a chunk, what it reads the render wrote a chunk before or
 * earlier, so it can run a chunk ahead on the second core (parts_step). */
static void reverb_network(MelloAudio *a, int32_t send, int32_t *left, int32_t *right)
{
    static const int8_t inject_sign[8] = {1, 1, -1, -1, 1, -1, 1, -1};
    int32_t x, y[8], d[8], h[8], w, sum = 0, inj, rv_l, rv_r;
    uint16_t i, pos, rp, cap, off;
    uint32_t p;
    if (left) *left = 0;
    if (right) *right = 0;
    if (!a || !a->reverb_buffer || a->reverb_pre_cap < 2u) return;

    p = a->reverb_out_pos;
    if (a->reverb_pre_samples) {
        uint32_t pr = p >= a->reverb_pre_samples ? p - a->reverb_pre_samples
                                                 : p + a->reverb_pre_cap - a->reverb_pre_samples;
        x = a->reverb_buffer[pr];
    } else
        x = send;
    p++;
    if (p >= a->reverb_pre_cap) p = 0u;
    a->reverb_out_pos = p;
    if (a->oliverb_on) { /* Oliverb, from the same line (reverb_choose) */
        int32_t ol, orr;
        mello_oliverb_step(&a->oliverb, x, &ol, &orr, SINE_Q15);
        if (left) *left = ol;
        if (right) *right = orr;
        return;
    }

    x = rv3_allpass(a, 0u, x, 18022u);
    x = rv3_allpass(a, 1u, x, 20890u);
    x = rv3_allpass(a, 2u, x, 15729u);

    /* No damping in the loop: Tone.Reverb's IR is white noise, its colour comes only from revTone and revHPF
     * after it. */
    /* Each line's delay swings slowly by up to RV3_MOD_DEPTH_Q8 / 256 samples, each at its own rate: that moves the
     * FDN's resonances, which are narrower than their spacing, so no note keeps landing on (or between) them and
     * gets much more (or less) reverb than its neighbours. Tone.Reverb's noise IR has no such modes. The swing
     * detunes the tail by up to ~6 cents. The delays are updated every RV3_MOD_EVERY samples (it moves < 1/256 of a
     * sample in between); the reads run every sample. */
    if (a->reverb_mod_tick == 0u) {
        a->reverb_mod_tick = RV3_MOD_EVERY;
        for (i = 0u; i < 8u; ++i) {
            uint32_t ph = a->reverb_mod_phase[i], dq8;
            int32_t s0 = SINE_Q15[(uint8_t)(ph >> 24)], s1 = SINE_Q15[(uint8_t)((ph >> 24) + 1u)], sn;
            a->reverb_mod_phase[i] = ph + RV3_MOD_INC[i] * RV3_MOD_EVERY * RV3_RATE_DIV;
            sn = s0 + (((s1 - s0) * (int32_t)((ph >> 16) & 0xFFu)) >> 8);
            cap = a->reverb_line_cap[i];
            { /* the line's length gliding to where SIZE put it: 1/8 sample per update, a bend of ~13 cents */
                uint32_t to = (uint32_t)a->reverb_line_delay[i] << 8, now = a->reverb_line_dq8[i];
                now = now + RV3_GLIDE_Q8 < to   ? now + RV3_GLIDE_Q8
                      : now > to + RV3_GLIDE_Q8 ? now - RV3_GLIDE_Q8
                                                : to;
                a->reverb_line_dq8[i] = now;
                /* (int32_t): RV3_RATE_DIV is unsigned, so the depth was, and so was the product: with the sine below
                 * zero the swing came out near +512 samples and the line jumped to its longest, and back as the sine
                 * rose; twice a cycle on each of the eight lines, a click going round (DECISIONS "The reverb
                 * glides"; ROADMAP F16's noise) */
                dq8 = (uint32_t)((int32_t)now + ((sn * (int32_t)RV3_MOD_DEPTH_Q8) >> 15)) - 128u;
            }
            if (dq8 < 256u) dq8 = 256u;
            if (cap > 3u && dq8 > ((uint32_t)cap - 2u) << 8) dq8 = ((uint32_t)cap - 2u) << 8;
            /* the delay is di + 0.5 + fr / 256: Thiran's sweet spot, 0.5..1.5 samples past di */
            a->reverb_mod_di[i] = (uint16_t)(dq8 >> 8);
            a->reverb_mod_eta[i] = RV3_ETA_Q15[dq8 & 0xFFu];
        }
    }
    a->reverb_mod_tick--;
    for (i = 0u; i < 8u; ++i) {
        int32_t y0, y1, eta = a->reverb_mod_eta[i];
        uint16_t di = a->reverb_mod_di[i], rp1;
        cap = a->reverb_line_cap[i];
        pos = a->reverb_line_pos[i];
        off = a->reverb_line_offset[i];
        rp = pos >= di ? (uint16_t)(pos - di) : (uint16_t)(pos + cap - di);
        rp1 = rp ? (uint16_t)(rp - 1u) : (uint16_t)(cap - 1u);
        y0 = rv3_rd(a, (uint32_t)off + rp);
        y1 = rv3_rd(a, (uint32_t)off + rp1);
        /* First-order allpass interpolation, (eta + z^-1) / (1 + eta z^-1): flat in level, where linear
         * interpolation would dull the tail. */
        y[i] = clamp32(q15mul_tz64(eta, y0) + y1 - q15mul_tz64(eta, a->reverb_mod_y[i]), -RV3_Q8_MAX,
                       RV3_Q8_MAX);
        a->reverb_mod_y[i] = y[i];
        d[i] = y[i];
        sum += d[i];
    }

    /* 8x8 Householder reflection H = I - 2/N * 11^T.  It is orthogonal but
     * costs one sum and one subtraction per line instead of a dense matrix. */
    for (i = 0u; i < 8u; ++i) h[i] = clamp32(d[i] - shr_tz(sum, 2u), -RV3_Q8_MAX, RV3_Q8_MAX);

    inj = q15mul_tz64(x * 256, 11585); /* ~1/sqrt(8): energy-controlled injection. */
    for (i = 0u; i < 8u; ++i) {
        w = (inject_sign[i] > 0 ? inj : -inj) + q15mul_tz64(h[i], a->reverb_line_feedback_q15[i]);
        pos = a->reverb_line_pos[i];
        off = a->reverb_line_offset[i];
        rv3_wr(a, (uint32_t)off + pos, w);
        pos++;
        if (pos >= a->reverb_line_cap[i]) pos = 0u;
        a->reverb_line_pos[i] = pos;
    }

    /* Two orthogonal sign rows expose a real stereo late field from shared FDN state. */
    rv_l = shr_tz(y[0] + y[1] + y[2] + y[3] - y[4] - y[5] - y[6] - y[7], 10u); /* (/ 4, and Q8 to samples) */
    rv_r = shr_tz(y[0] - y[1] + y[2] - y[3] + y[4] - y[5] + y[6] - y[7], 10u);
    if (left) *left = clamp32(rv_l, -32767, 32767);
    if (right) *right = clamp32(rv_r, -32767, 32767);
}
/* One audio-rate sample of the reverb, the network at half the rate: the input averaged over two samples, the output
 * interpolated back. */
/* One sample of the delay: in goes in (with the feedback), and its return comes out through LO CUT and HI CUT. The time
 * ramps (Tone's delayTime.rampTo): delay_q8 is read between samples while it moves. */
/* The line's reach: past it, HALF RATE (twice as far); past that, the longest it holds. */
/* PING reads the line at twice the time as well, so the line must hold that. */
static void delay_fit(MelloAudio *a)
{
    uint32_t lim = a->delay_capacity - 2u, k = a->fx.delay_pingpong ? 2u : 1u;
    uint8_t hr;
    if (a->delay_samples < 1u) a->delay_samples = 1u;
    hr = a->delay_samples * k > lim ? 1u : 0u;
    if (a->delay_samples * k > 2u * lim - 2u) a->delay_samples = (2u * lim - 2u) / k;
    if (hr != a->delay_hr) a->delay_hr = hr, a->delay_hr_phase = 0u;
}
/* GLIDE (DECISIONS "The looping delay"): the time this sample, gliding to the ramp's target as Clouds' looping delay
 * does (its 0.0005 a sample at 32 kHz is GLIDE 16%), the step's remainder kept. */
static inline int32_t delay_glide(MelloAudio *a)
{
    int64_t p = (int64_t)(a->rp_delay.target - a->dly_glide_q8) * a->dly_glide_k + a->dly_glide_rem;
    a->dly_glide_q8 += (int32_t)(p >> 24);
    a->dly_glide_rem = (uint32_t)(p & 0xFFFFFF);
    return a->dly_glide_q8;
}
/* GLIDE turned: its step (2^24 / (its time constant in samples), a divide only here); turned on, it starts where the
 * ramp is; turned off, the ramp takes it from where the glide is, the rest in 50 ms */
static void delay_glide_set(MelloAudio *a)
{
    uint32_t ms = mello_delay_glide_ms(a->fx.delay_glide), k = ms ? 167772160u / (ms * 441u) : 0u;
    if (k && !a->dly_glide_k) a->dly_glide_q8 = a->rp_delay.cur, a->dly_glide_rem = 0u;
    if (!k && a->dly_glide_k) {
        int32_t t = a->rp_delay.target;
        a->rp_delay.cur = a->dly_glide_q8, a->rp_delay.target = t + 1; /* (so ramp_to moves it) */
        ramp_to(&a->rp_delay, t >> 8, 50u, false);
        a->ramps_moving = true;
    }
    a->dly_glide_k = k;
}
/* The line at delay_q8 (a time in samples, Q8), between two slots, through LO CUT (states lc). */
static int32_t delay_read(MelloAudio *a, int32_t delay_q8, int32_t lc[2])
{
    uint32_t di, fr, rp, rp1;
    int32_t d;
    if (a->delay_hr) /* HALF RATE: the line's slots are two samples each; the read point counted in half slots */
        delay_q8 = (int32_t)(((uint32_t)delay_q8 - ((uint32_t)a->delay_hr_phase << 8)) >> 1);
    di = (uint32_t)delay_q8 >> 8, fr = (uint32_t)delay_q8 & 0xFFu;
    if (di < 1u) di = 1u;
    if (di > a->delay_capacity - 2u) di = a->delay_capacity - 2u;
    rp = a->delay_pos >= di ? a->delay_pos - di : a->delay_pos + a->delay_capacity - di;
    d = a->delay_buffer[rp];
    if (fr) {
        rp1 = rp ? rp - 1u : a->delay_capacity - 1u;
        d += ((a->delay_buffer[rp1] - d) * (int32_t)fr) >> 8;
    }
    if (a->fx.delay_lowcut) {
        /* LO CUT, 12 dB an octave, 20 Hz .. 2 kHz: on the return and in the feedback, so each repeat is thinner than
         * the last (delay_lc_k, from apply_pending). */
        /* The two one-poles' states in Q8, their output rounded: in Q0 the shift floored, a few LSB stayed in the
         * feedback for ever and the unit never counted as silent (autosave never ran; audio_silence_regression). */
        int32_t k = a->delay_lc_k;
        lc[0] += (int32_t)(((int64_t)(d * 256 - lc[0]) * k) >>
                           15); /* (* 256: a shift of a negative is undefined) */
        d -= (lc[0] + 128) >> 8;
        lc[1] += (int32_t)(((int64_t)(d * 256 - lc[1]) * k) >> 15);
        d -= (lc[1] + 128) >> 8;
    }
    return d;
}
/* DIFFUSE and PITCH (DECISIONS "The looping delay"): Clouds' looping delay puts its diffuser and its pitch shifter on
 * what it plays and feeds back, so each repeat is smeared and shifted further than the last. In the delay's memory
 * (dlyfx): the shifter's line, then the diffuser's four. The diffuser is Clouds' left one (126, 180, 269 and 444
 * samples at 32 kHz, here x 44.1 / 32), allpasses of 0.625; the shifter Clouds' (two grains over its line, a raised
 * cosine between them; each read between two points, not Clouds' four), its window fixed at 45 ms (Clouds' takes it
 * from SIZE: at 32 kHz 4 .. 64 ms). Each output worked out whole and then cut towards zero once, so it is never larger
 * than the exact one: an allpass's y - g v with g v cut alone, or the grains' sum cut a grain at a time, came out
 * larger, and a quiet repeat went round for ever (PITCH -12 with DIFFUSE: 4 samples 39 s on). */
static inline int32_t tz15_64(int64_t p) /* p / 2^15 towards zero */
{
    return (int32_t)((p + ((p >> 63) & 32767)) >> 15);
}
#define DLYFX_PS_LEN 2048u /* the shifter's line, a power of two */
#define DLYFX_PS_WIN 2000  /* its window, samples */
static const uint16_t DLYFX_AP_LEN[4] = {174u, 248u, 371u, 612u};
/* The allpasses' 0.625 is 5 / 8, so y g and y - v g, Q15 and cut towards zero, are 5 y and 8 y - 5 v cut by 2^3: the
 * same to the bit in 32 bits (0.5-f127; the core's 64-bit products cost it a third more). */
static inline int32_t dlyfx_diffuse(MelloAudio *a, int32_t x)
{
    int16_t *m = a->dlyfx + DLYFX_PS_LEN;
    int32_t acc = x;
    uint8_t k;
    for (k = 0u; k < 4u; ++k) {
        uint16_t p = a->dfx_ap_pos[k];
        int32_t y = m[p], v = acc + shr_tz(5 * y, 3u); /* (the line's oldest: its length back) */
        m[p] = sat16(v);
        acc = shr_tz(8 * y - 5 * v, 3u);
        a->dfx_ap_pos[k] = (uint16_t)(p + 1u >= DLYFX_AP_LEN[k] ? 0u : p + 1u);
        m += DLYFX_AP_LEN[k];
    }
    return tz15_64((int64_t)x * (32768 - a->dfx_amount_q15) + (int64_t)acc * a->dfx_amount_q15);
}
/* the shifter's line at offset off (Q16 samples back), between two points (0.5-f128: Clouds reads four, Hermite's,
 * which took the core half the shifter's time). The line is 16-bit, so x0 (1 - t) + x1 t is under 2^31 and its cut
 * one towards zero, never past either point. */
static inline int32_t dlyfx_linear(const MelloAudio *a, int32_t off_q16)
{
    const int16_t *r = a->dlyfx;
    uint32_t i = (a->ps_wp + ((uint32_t)off_q16 >> 16)) & (DLYFX_PS_LEN - 1u);
    int32_t t = (off_q16 & 0xFFFF) >> 1, x0 = r[i], x1 = r[(i + 1u) & (DLYFX_PS_LEN - 1u)]; /* (t Q15) */
    int32_t p = x0 * (32768 - t) + x1 * t;
    return (p + ((p >> 31) & 32767)) >> 15;
}
static inline int32_t dlyfx_shift(MelloAudio *a, int32_t x)
{
    uint32_t ph, w = a->ps_wp = (uint16_t)((a->ps_wp - 1u) & (DLYFX_PS_LEN - 1u));
    int32_t tri, phase, half;
    a->dlyfx[w] = sat16(x);
    ph = a->ps_phase = (a->ps_phase + (uint32_t)a->ps_step) & 0xFFFFFFu; /* Q24 of the window */
    tri = olv_raised_cos((int32_t)((ph >= (1u << 23) ? (1u << 24) - ph : ph) >> 7), SINE_Q15);
    phase =
        (int32_t)((ph * (uint32_t)(DLYFX_PS_WIN / 16)) >> 4); /* Q16 samples (ph x 2000 / 2^8, in 32 bits) */
    half = phase + (DLYFX_PS_WIN << 15);
    if (half >= DLYFX_PS_WIN << 16) half -= DLYFX_PS_WIN << 16;
    { /* (two 16-bit grains by tri and 1 - tri, at most 2^30: 32 bits) */
        int32_t p = dlyfx_linear(a, phase) * tri + dlyfx_linear(a, half) * (32768 - tri);
        return (p + ((p >> 31) & 32767)) >> 15;
    }
}
static inline int32_t delay_loop_fx(MelloAudio *a, int32_t d)
{
    if (a->dlyfx_diffuse) d = dlyfx_diffuse(a, d);
    if (a->dlyfx_shift) d = dlyfx_shift(a, d);
    return d;
}
/* DIFFUSE or PITCH turned: the amount and the shifter's step (1 - ratio) / window, Q24 (a divide only here); either
 * turned on starts from an empty line */
static void dlyfx_set(MelloAudio *a)
{
    int32_t st = a->fx.delay_pitch < -24 ? -24 : a->fx.delay_pitch > 24 ? 24 : a->fx.delay_pitch;
    uint32_t dif = a->fx.delay_diffuse > 1000u ? 1000u : a->fx.delay_diffuse;
    bool diffuse = a->dlyfx && dif, shift = a->dlyfx && st;
    if (diffuse && !a->dlyfx_diffuse) {
        memset(a->dlyfx + DLYFX_PS_LEN, 0, (MELLO_AUDIO_DLYFX_WORDS - DLYFX_PS_LEN) * sizeof a->dlyfx[0]);
        memset(a->dfx_ap_pos, 0, sizeof a->dfx_ap_pos);
    }
    if (shift && !a->dlyfx_shift) {
        memset(a->dlyfx, 0, DLYFX_PS_LEN * sizeof a->dlyfx[0]);
        a->ps_wp = 0u, a->ps_phase = 0u;
    }
    a->dfx_amount_q15 = (int32_t)(dif * 32768u / 1000u);
    a->ps_step = shift ? (65536 - (int32_t)semi_ratio_q16(st * 256)) * 256 / DLYFX_PS_WIN : 0;
    a->dlyfx_diffuse = diffuse, a->dlyfx_shift = shift;
}
/* FREEZE (DECISIONS "The looping delay"): Clouds' looping delay, frozen, stops writing and plays a loop of what it
 * holds; here the loop is the delay's own time (its DIV), the last of it before a crossfade's worth of the newest, so
 * a pass may run on past either end while the next fades in (Clouds' tail). REV plays it backwards. Each pass takes
 * the time anew, so a DIV turned while frozen sizes the next. Its output goes through DIFFUSE and PITCH and HI CUT as
 * the repeats do; nothing is fed back. Turned off, the tap (and the writing) come back over a crossfade. */
#define FRZ_XF 256u /* the crossfades, samples (5.8 ms) */
static inline uint32_t frz_wrap(const MelloAudio *a, int32_t pos_q8)
{
    int32_t c = (int32_t)a->delay_capacity << 8;
    if (pos_q8 < 0) pos_q8 += c;
    if (pos_q8 >= c) pos_q8 -= c;
    return (uint32_t)pos_q8;
}
static inline int32_t frz_at(const MelloAudio *a,
                             int32_t pos_q8) /* the line at a place, Q8 slots, between two */
{
    uint32_t p = frz_wrap(a, pos_q8), i = p >> 8, j = i + 1u >= a->delay_capacity ? 0u : i + 1u;
    int32_t d = a->delay_buffer[i];
    return d + (((a->delay_buffer[j] - d) * (int32_t)(p & 0xFFu)) >> 8);
}
/* the loop's length from the time: its slots (the rate frozen at), and the pass in samples */
static void frz_size(MelloAudio *a)
{
    uint32_t slots = a->frz_step == 256u ? a->delay_samples : a->delay_samples / 2u,
             most = a->delay_capacity - 4u * FRZ_XF - 4u;
    if (slots < FRZ_XF) slots = FRZ_XF;
    if (slots > most) slots = most;
    a->frz_len = slots;
    a->frz_len_n = a->frz_step == 256u ? slots : slots * 2u;
}
static void frz_set(MelloAudio *a)
{
    uint8_t m = a->fx.delay_freeze > 2u ? 0u : a->fx.delay_freeze;
    if (!a->delay_buffer || a->delay_capacity <= 8u * FRZ_XF) m = 0u;
    if (m && !a->frz_play) { /* frozen: the loop ends a crossfade's worth before the newest */
        a->frz_step = a->delay_hr ? 128u : 256u;
        a->frz_end =
            (uint32_t)(frz_wrap(a, (int32_t)(a->delay_pos << 8) - (int32_t)(FRZ_XF * a->frz_step)) >> 8);
        frz_size(a);
        a->frz_n = 0u, a->frz_first = true, a->frz_out = 0u, a->frz_play = true;
    } else if (m && a->frz_play) {
        if (a->frz_out) a->frz_out = 0u; /* (frozen again while it fades out: on with the loop) */
        if (m != a->frz_mode && a->frz_mode)
            a->frz_n = a->frz_len_n - 1u - a->frz_n; /* ON <-> REV, where it is */
    } else if (!m && a->frz_play && !a->frz_out)
        a->frz_out = FRZ_XF;
    a->frz_mode = m ? m : a->frz_mode;
}
/* this sample of the loop; the tap's (tap) where the first pass fades in from it */
static int32_t frz_loop(MelloAudio *a, int32_t tap)
{
    int32_t o = (int32_t)(a->frz_n * a->frz_step), end = (int32_t)(a->frz_end << 8),
            start = end - (int32_t)(a->frz_len << 8), pos, tail, x;
    bool rev = a->frz_mode == 2u;
    pos = rev ? end - 256 - o : start + o;
    x = frz_at(a, pos);
    if (a->frz_n < FRZ_XF) { /* the pass fades in over the last one running on (or the tap) */
        int32_t g = (int32_t)(a->frz_n << 7), t;
        tail = rev ? start - 256 - o : end + o;
        t = a->frz_first ? tap : frz_at(a, tail);
        x = tz15_64((int64_t)x * g + (int64_t)t * (32768 - g));
    }
    if (++a->frz_n >= a->frz_len_n) a->frz_n = 0u, a->frz_first = false, frz_size(a);
    return x;
}
/* PING (ROADMAP F25): the line is read at the time (left) and at twice it (right, at FDBK), and fed back from the
 * second at FDBK squared, so the echoes come left, right, left ... at 1, FDBK, FDBK^2 ...: the same series as without,
 * taking turns. Without PING the right is the left. */
static int32_t delay_tick(MelloAudio *a, int32_t send, int32_t dly_fb, int32_t delay_q8, int32_t *right)
{
    int32_t d, fb_in, dl;
    *right = 0;
    if (!a->delay_buffer || a->delay_capacity <= 2u || !a->delay_samples) return 0;
    if (a->frz_play && !a->frz_out) { /* FREEZE: the loop, nothing written */
        d = frz_loop(a, a->frz_first ? delay_read(a, delay_q8, a->delay_lc) : 0);
        if (a->dlyfx_diffuse | a->dlyfx_shift) d = delay_loop_fx(a, d);
        dl = tone_lp24(&DELAY_TONE_BIQUAD_Q28[a->fx.delay_tone / 10u], a->delay_tone_s, d);
        *right = dl;
        return dl;
    }
    d = delay_read(a, delay_q8, a->delay_lc);
    if (a->frz_play) { /* FREEZE off: the tap back over the loop, the writing with it */
        int32_t g = (int32_t)(FRZ_XF - a->frz_out) << 7;
        d = tz15_64((int64_t)d * g + (int64_t)frz_loop(a, 0) * (32768 - g));
        if (--a->frz_out == 0u) a->frz_play = false;
    }
    if (!a->fx.delay_pingpong && (a->dlyfx_diffuse | a->dlyfx_shift)) d = delay_loop_fx(a, d);
    dl = tone_lp24(&DELAY_TONE_BIQUAD_Q28[a->fx.delay_tone / 10u], a->delay_tone_s, d);
    if (a->fx.delay_pingpong) {
        int32_t d2 = delay_read(a, delay_q8 * 2, a->delay_lc_r);
        if (a->dlyfx_diffuse | a->dlyfx_shift)
            d2 = delay_loop_fx(a, d2); /* (PING: the tap that feeds back) */
        fb_in = mul_q12_fb(d2, mul_q12(dly_fb, dly_fb));
        *right =
            mul_q12(tone_lp24(&DELAY_TONE_BIQUAD_Q28[a->fx.delay_tone / 10u], a->delay_tone_r_s, d2), dly_fb);
    } else {
        fb_in = mul_q12_fb(d, dly_fb);
        *right = dl;
    }
    if (a->delay_hr) { /* HALF RATE: two samples averaged into a slot */
        int32_t in = send + fb_in;
        if (!a->delay_hr_phase) {
            a->delay_hr_acc = in;
            a->delay_hr_phase = 1u;
            return dl;
        }
        a->delay_hr_phase = 0u;
        a->delay_buffer[a->delay_pos] = sat16((a->delay_hr_acc + in) / 2);
    } else
        a->delay_buffer[a->delay_pos] = sat16(send + fb_in);
    if (++a->delay_pos >= a->delay_capacity) a->delay_pos = 0u;
    return dl;
}
/* THROW raises the delay's feedback towards 70% as it slides in */
static int32_t perf_fb(const MelloAudio *a, int32_t fb)
{
    return a->throw_q12 && fb < 2867 ? fb + (((2867 - fb) * a->throw_q12) >> 12) : fb;
}
/* The half-band filter (31 taps, Kaiser 7.5): flat to 8 kHz within 0.03 dB, -48 dB from 14 kHz. Its centre tap is
 * 1/2 and every other tap 0; these are the odd ones, Q15, nearest the centre first. */
static const int16_t RV_HB[8] = {10269, -3020, 1401, -668, 291, -105, 27, -3};
/* The reverb's way in, a sample (the render's, always): the input filtered down, every other sample into the
 * pre-delay line. */
static void reverb_in(MelloAudio *a, int32_t send)
{
    uint8_t ip = a->reverb_hb_in_pos, j;
    a->reverb_hb_in[ip] = sat16(send);
    a->reverb_hb_in_pos = (uint8_t)((ip + 1u) & 31u);
    if (a->reverb_odd) {
        /* Down: the input filtered, centred 15 samples back, every other sample into the network. */
        int32_t acc = (int32_t)a->reverb_hb_in[(ip - 15u) & 31u] * 16384;
        for (j = 0u; j < 8u; ++j)
            acc += RV_HB[j] * ((int32_t)a->reverb_hb_in[(ip - 15u - (2u * j + 1u)) & 31u] +
                               a->reverb_hb_in[(ip - 15u + (2u * j + 1u)) & 31u]);
        a->reverb_down = acc >> 15;
        if (a->reverb_buffer && a->reverb_pre_cap >= 2u) reverb_pre_write(a, a->reverb_down);
    }
    a->reverb_odd = !a->reverb_odd;
}
/* And its way out, the same sample: the network every other sample, the filter back up. */
static void reverb_out(MelloAudio *a, int32_t *left, int32_t *right)
{
    uint8_t op, j;
    if (a->reverb_odd_out) {
        int32_t l, r;
        reverb_network(a, a->reverb_down, &l, &r);
        op = a->reverb_hb_out_pos;
        a->reverb_hb_out[0][op] = sat16(l);
        a->reverb_hb_out[1][op] = sat16(r);
        a->reverb_hb_out_pos = (uint8_t)((op + 1u) & 15u);
        /* Up, the even phase: the network's output 8 of its samples back (the filter's centre). */
        *left = a->reverb_hb_out[0][(op - 8u) & 15u];
        *right = a->reverb_hb_out[1][(op - 8u) & 15u];
    } else {
        /* Up, the odd phase: half-way between that sample and the next, by the filter's odd taps (doubled). */
        int32_t al = 0, ar = 0;
        op = (uint8_t)((a->reverb_hb_out_pos - 1u) & 15u); /* the newest */
        for (j = 0u; j < 8u; ++j) {
            uint8_t lo = (uint8_t)((op - 8u - j) & 15u), hi = (uint8_t)((op - 7u + j) & 15u);
            al += RV_HB[j] * ((int32_t)a->reverb_hb_out[0][lo] + a->reverb_hb_out[0][hi]);
            ar += RV_HB[j] * ((int32_t)a->reverb_hb_out[1][lo] + a->reverb_hb_out[1][hi]);
        }
        *left = al >> 14;
        *right = ar >> 14;
    }
    a->reverb_odd_out = !a->reverb_odd_out;
}
/* The reverb's return: revTone, then a DC blocker, ~27 Hz: reverb_hp_y1 integrates the DC in Q8 and is subtracted.
 * The old q15mul one-pole high-pass floored to a dead band and passed about -150 of DC forever, which kept the tail
 * guard armed, so the unit never counted as silent (P1-1). */
static void reverb_return(MelloAudio *a, int32_t rv_l, int32_t rv_r, int32_t *hp_l, int32_t *hp_r)
{
    const ToneSubBiquadCoeff *rt = &REVERB_TONE_BIQUAD_Q28[a->fx.reverb_tone / 10u];
    int32_t tl = tone_lp24(rt, a->reverb_tone_s[0], rv_l), tr = tone_lp24(rt, a->reverb_tone_s[1], rv_r);
    *hp_l = tl - (a->reverb_hp_y1 >> 8);
    *hp_r = tr - (a->reverb_hp_y1_r >> 8);
    a->reverb_hp_y1 += *hp_l;
    a->reverb_hp_y1_r += *hp_r;
}
#if defined(MELLO_REVERB_TEST_HOOK) && MELLO_REVERB_TEST_HOOK
/* One audio-rate sample of the reverb, the network at half the rate: the input averaged over two samples, the output
 * interpolated back (the way in, then the way out, as the render does them: the probes' below) */
static void reverb_step(MelloAudio *a, int32_t send, int32_t *left, int32_t *right)
{
    reverb_in(a, send);
    reverb_out(a, left, right);
}
#endif

#if defined(MELLO_REVERB_TEST_HOOK) && MELLO_REVERB_TEST_HOOK
/* Host-only probes. Never compiled into target firmware. */
int32_t mello_audio_reverb_ir_probe(MelloAudio *a, int32_t send)
{
    int32_t l = 0, r = 0;
    reverb_step(a, send, &l, &r);
    return l;
}
void mello_audio_reverb_stereo_probe(MelloAudio *a, int32_t send, int32_t *left, int32_t *right)
{
    reverb_step(a, send, left, right);
}
/* the reverb chosen now, as the render does at each chunk (Oliverb with the cores set) */
void mello_audio_reverb_choose_probe(MelloAudio *a)
{
    reverb_choose(a);
}
/* one sample of the delay alone, at its time and FDBK (Q12) as given, as the render calls it */
int32_t mello_audio_delay_probe(MelloAudio *a, int32_t send, int32_t fb_q12, int32_t *right)
{
    return delay_tick(a, send, fb_q12, (int32_t)a->delay_samples << 8, right);
}
#endif

/* The output stage (DECISIONS "The output level", "The level, all of it"): MASTER, a fixed gain, a bus compressor,
 * a look-ahead limiter at -1 dBFS, and a soft top for what is left. `make parity` and the tests that measure a part
 * alone render with the gain at 0 dB (-DMELLO_OUTPUT_GAIN_Q12=4096): then the stage is out, the engine as the HTML. */
#ifndef MELLO_OUTPUT_GAIN_TUNE
#define MELLO_OUTPUT_GAIN_TUNE 22958 /* 10^(15/20): +15 dB (+11 before the look-ahead limiter) */
#endif
#ifndef MELLO_OUTPUT_GAIN_Q12
#define MELLO_OUTPUT_GAIN_Q12 MELLO_OUTPUT_GAIN_TUNE
#endif
#define MELLO_OUTPUT_STAGE (MELLO_OUTPUT_GAIN_Q12 != 4096)
/* A tail still sounding: over 8 LSB before the output's gain (the reverb's last bits stay a few LSB for ever, which
 * the gain must not turn into a tail that never ends: then nothing would ever be silent, and nothing saved) */
#define TAIL_LSB          ((8 * MELLO_OUTPUT_GAIN_Q12) >> 12)
#define LIMITER_THRESHOLD 29204 /* -1 dBFS */
/* What still gets past -1 dBFS bends towards full scale and never reaches it: x over the threshold by e comes out at
 * T + e * H / (e + H) (H the headroom to full scale), not clipped hard. A divide only then. */
static int32_t soft_top(int32_t x)
{
    int32_t ax = x < 0 ? -x : x, e, y;
    const int32_t h = 32767 - LIMITER_THRESHOLD;
    if (ax <= LIMITER_THRESHOLD) return x;
    e = ax - LIMITER_THRESHOLD;
    if (e > 1 << 20) e = 1 << 20;
    y = LIMITER_THRESHOLD + (int32_t)((uint32_t)e * (uint32_t)h / (uint32_t)(e + h)); /* < 2^32 */
    return x < 0 ? -y : y;
}
/* log2 in Q8 (x > 0) and 2^(-y/256) in Q15 (y >= 0), each by a 17-point table, within 0.1 dB */
static const uint8_t LOG2_FRAC[17] = {0,   22,  44,  63,  82,  100, 118, 134, 150,
                                      165, 179, 193, 207, 220, 232, 244, 255};
static const uint16_t EXP2N_Q15[17] = {32768, 31379, 30048, 28774, 27554, 26386, 25268, 24196, 23170,
                                       22188, 21247, 20347, 19484, 18658, 17867, 17109, 16384};
static int32_t log2_q8(uint32_t x)
{
    int32_t n = 31 - __builtin_clz(x);
    uint32_t f = n >= 8 ? (x >> (n - 8)) & 255u : (x << (8 - n)) & 255u, i = f >> 4, r = f & 15u;
    return n * 256 + LOG2_FRAC[i] + (int32_t)(((LOG2_FRAC[i + 1] - LOG2_FRAC[i]) * r) >> 4);
}
static int32_t exp2n_q15(int32_t y)
{
    uint32_t f = (uint32_t)y & 255u, i = f >> 4, r = f & 15u;
    int32_t g = EXP2N_Q15[i] - (int32_t)(((EXP2N_Q15[i] - EXP2N_Q15[i + 1]) * r) >> 4);
    return y >= 15 * 256 ? 0 : g >> (y >> 8);
}
/* The bus compressor: glue, as OP-1's master has. Its ear is the mix's middle, high-passed at ~100 Hz (the bass and
 * kick alone do not pump it), followed at 10 ms up and 150 ms down. Above COMP_T it turns down at COMP_R:1, a knee an
 * octave (6 dB) wide; the gain is worked out every 16 samples and ramped. Loud rolls, a stutter or a dense lead come
 * down; what sits under it is left as it is. */
#ifndef COMP_T
#define COMP_T                                                                                               \
    10100 /* the follower's |x| (mean, not RMS), after the output's gain: ~-10 dBFS, the loud ones only */
#endif
#ifndef COMP_SLOPE_Q8
#define COMP_SLOPE_Q8 154 /* 1 - 1/R: 2.5:1 */
#endif
#define COMP_KNEE    256  /* Q8 octaves */
#define COMP_HP_Q15  467  /* 2 pi 100 / 44100 */
#define COMP_ATT_Q20 2375 /* 1 - exp(-1 / (0.010 * 44100)) */
#define COMP_REL_Q20 158  /* 1 - exp(-1 / (0.150 * 44100)) */
#define LA_REL_Q15   12   /* the limiter's release, ~60 ms */
static void comp_step(MelloAudio *a, int32_t *l, int32_t *r)
{
    int32_t m = (*l + *r) / 2, d;
    m = clamp32(m, -131071, 131071); /* (4x full scale is plenty for its ear; and x16 stays clear of the ROM's
                                      * addresses, which tools/build.py's checker rejects as constants) */
    a->comp_hp_q4 += (int32_t)(((int64_t)(m * 16 - a->comp_hp_q4) * COMP_HP_Q15) >> 15);
    m = m * 16 - a->comp_hp_q4; /* Q4, high-passed */
    if (m < 0) m = -m;
    d = m - a->comp_env_q4;
    a->comp_env_q4 += (int32_t)(((int64_t)d * (d > 0 ? COMP_ATT_Q20 : COMP_REL_Q20)) >> 20);
    if (!a->comp_n) { /* the block's gain */
        int32_t x = (a->comp_env_q4 > 16 ? log2_q8((uint32_t)a->comp_env_q4) : 0) - log2_q8(COMP_T << 4) +
                    COMP_KNEE / 2,
                red = x <= 0          ? 0
                      : x < COMP_KNEE ? COMP_SLOPE_Q8 * x / 2 * x / COMP_KNEE >> 8
                                      : COMP_SLOPE_Q8 * (x - COMP_KNEE / 2) >> 8;
        a->comp_step = (exp2n_q15(red) - a->comp_g) / 16; /* there by the block's end */
    }
    a->comp_n = (uint8_t)((a->comp_n + 1u) & 15u);
    a->comp_g += a->comp_step;
    *l = (int32_t)(((int64_t)*l * a->comp_g) >> 15);
    *r = (int32_t)(((int64_t)*r * a->comp_g) >> 15);
}
/* The look-ahead limiter: a sample's gain to stay under -1 dBFS is held for the look-ahead and averaged over it, so
 * the gain is down by the time the sample (delayed as long) goes out: no attack's edge, no clip. Then it lets go
 * at ~60 ms. */
static void limiter_step(MelloAudio *a, int32_t *l, int32_t *r)
{
    int32_t al = *l < 0 ? -*l : *l, ar = *r < 0 ? -*r : *r, pk = (al > ar ? al : ar), g = 32767, ol, or_;
    uint8_t p = a->la_pos;
    if (pk > LIMITER_THRESHOLD)
        g = (int32_t)(((uint32_t)LIMITER_THRESHOLD << 15) / (uint32_t)(pk > 262143 ? 262143 : pk));
    if (g <= a->la_min)
        a->la_min = g, a->la_hold = MELLO_LA_N + 1u;
    else if (a->la_hold)
        a->la_hold--;
    else
        a->la_min += ((32767 - a->la_min) * LA_REL_Q15 >> 15) + 1, a->la_min = a->la_min > g ? g : a->la_min;
    a->la_sum += a->la_min - a->la_g[p];
    a->la_g[p] = (int16_t)a->la_min;
    ol = a->la_line[0][p], or_ = a->la_line[1][p];
    a->la_line[0][p] = *l, a->la_line[1][p] = *r;
    a->la_pos = (uint8_t)((p + 1u) & (MELLO_LA_N - 1u));
    g = a->la_sum / (int32_t)MELLO_LA_N;
    *l = (int32_t)(((int64_t)ol * g) >> 15);
    *r = (int32_t)(((int64_t)or_ * g) >> 15);
}
static uint32_t config_update_begin(MelloAudio *a)
{
    uint32_t g;
    if (!a) return 1u;
    g = a->config_generation + 1u;
    if ((g & 1u) == 0u) g += 1u;
    if (g == 0u) g = 1u;
    a->config_generation = g;
    return g;
}
static void config_update_end(MelloAudio *a, uint32_t begin)
{
    uint32_t g;
    if (!a) return;
    g = begin + 1u;
    if (g == 0u) g = 2u;
    a->config_generation = g;
}
static void apply_pending(MelloAudio *a)
{
    uint32_t g;
    uint16_t old_decay;
    bool sub_drive_changed, lfo_rate_changed, delay_changed, reverb_changed, dist_changed, reverb_wet_changed,
        oliverb_changed, glide_changed, dlyfx_changed, freeze_changed;
    if (!a) return;
    g = a->config_generation;
    /* Odd means main-loop publication is in progress. Keep the last complete
     * audio snapshot for this block instead of consuming a torn config. */
    if ((g & 1u) || g == a->applied_generation) return;
    old_decay = a->config.decay_ms;
    sub_drive_changed = a->sub_config.drive != a->pending_sub_config.drive;
    lfo_rate_changed = a->config.lfo_rate_millihz != a->pending_config.lfo_rate_millihz;
    delay_changed =
        a->fx.delay_time_ms != a->pending_fx.delay_time_ms || a->fx.delay_sync != a->pending_fx.delay_sync ||
        a->fx.delay_pingpong != a->pending_fx.delay_pingpong; /* PING: the line holds twice the time */
    glide_changed = a->fx.delay_glide != a->pending_fx.delay_glide;
    dlyfx_changed =
        a->fx.delay_diffuse != a->pending_fx.delay_diffuse || a->fx.delay_pitch != a->pending_fx.delay_pitch;
    freeze_changed = a->fx.delay_freeze != a->pending_fx.delay_freeze;
    reverb_changed = a->fx.reverb_pre_ms != a->pending_fx.reverb_pre_ms ||
                     a->fx.reverb_size != a->pending_fx.reverb_size ||
                     a->fx.reverb_decay_ms != a->pending_fx.reverb_decay_ms;
    dist_changed = a->fx.dist_amount != a->pending_fx.dist_amount ||
                   a->fx.dist_character != a->pending_fx.dist_character;
    reverb_wet_changed = a->fx.reverb_wet != a->pending_fx.reverb_wet;
    oliverb_changed =
        a->fx.olv_diffuse != a->pending_fx.olv_diffuse || a->fx.olv_swing != a->pending_fx.olv_swing ||
        a->fx.olv_rate != a->pending_fx.olv_rate || a->fx.olv_shimmer != a->pending_fx.olv_shimmer;
    a->config = a->pending_config;
    a->modr = a->pending_modr;
    a->fx = a->pending_fx;
    a->sub_config = a->pending_sub_config;
    a->sample_config = a->pending_sample_config;
    a->drum_config = a->pending_drum_config;
    a->bass_config = a->pending_bass_config;
    memcpy(a->drum_makeup_q12, a->pending_drum_makeup_q12, sizeof a->drum_makeup_q12);
    memcpy(a->sample_makeup_q12, a->pending_sample_makeup_q12, sizeof a->sample_makeup_q12);
#if MELLO_FEATURE_CHORD_MACHINE
    {
        bool was_on = a->chord_config.on;
        a->chord_config = a->pending_chord_config;
        if (was_on && !a->chord_config.on) chord_kill(a);
        chord_coeffs(a);
    }
#endif
    if (sub_drive_changed) {
        memcpy(a->sub_dist_curve, a->pending_sub_dist_curve, sizeof a->sub_dist_curve);
#if defined(MELLO_LIVE_EDIT_TEST_HOOK) && MELLO_LIVE_EDIT_TEST_HOOK
        a->test_sub_curve_applies++;
#endif
    }
    if (a->effective_decay_ms == 0u || a->config.decay_ms != old_decay)
        a->effective_decay_ms = a->config.decay_ms;
    if (lfo_rate_changed || a->applied_generation == 0u)
        a->lfo_inc = ((uint32_t)a->config.lfo_rate_millihz * 974u + 5u) / 10u;
    if (a->delay_capacity && (delay_changed || a->applied_generation == 0u)) {
        uint32_t ms = a->fx.delay_time_ms;
        a->delay_samples = ((uint32_t)ms * 441u + 5u) / 10u;
        delay_fit(a);
    }
    if (reverb_changed || a->applied_generation == 0u) {
        reverb_configure(a);
        oliverb_settings(a);
#if defined(MELLO_LIVE_EDIT_TEST_HOOK) && MELLO_LIVE_EDIT_TEST_HOOK
        a->test_reverb_reconfigs++;
#endif
    } else if (oliverb_changed)
        oliverb_settings(a); /* (its knobs: no line moves) */
    if (dist_changed || a->applied_generation == 0u)
        memcpy(a->main_dist_curve, a->pending_main_dist_curve, sizeof a->main_dist_curve);
    if (reverb_wet_changed || a->applied_generation == 0u)
        a->reverb_send_permille =
            REVERB_SEND_PERMILLE[(a->fx.reverb_wet > 1000u ? 1000u : a->fx.reverb_wet) / 10u];
    {
        /* Before the first sample the config is where the sound starts: no ramp. */
        bool j = a->applied_generation == 0u || a->rendered_frames == 0u;
        ramp_to(&a->rp_cut, a->config.filter_cut, 50u, j);
        ramp_to(&a->rp_res, a->config.filter_res, 50u, j);
        /* the gains in Q12 (the reverb return's 1050/1000, reverbReturnGain, folded into its level) */
        { /* MASTER with TRIM (-6..+6 dB, 10^(dB/20) in Q12) */
            static const int16_t TRIM_Q12[13] = {2053, 2304, 2585, 2900, 3254, 3651, 4096,
                                                 4596, 5157, 5786, 6492, 7284, 8173};
            int32_t t = a->fx.trim_db < -6 ? -6 : a->fx.trim_db > 6 ? 6 : a->fx.trim_db;
            ramp_to(&a->rp_master, (permille_q12(a->config.master_level) * TRIM_Q12[t + 6]) >> 12, 50u, j);
        }
        ramp_to(&a->rp_rev_send, permille_q12(a->reverb_send_permille), 80u, j);
        ramp_to(&a->rp_rev_level, permille_q12((int32_t)a->fx.reverb_level * 1050 / 1000), 80u, j);
        ramp_to(&a->rp_dly_send, permille_q12(a->fx.delay_wet), 100u, j);
        ramp_to(&a->rp_dly_level, permille_q12(a->fx.delay_level), 80u, j);
        ramp_to(&a->rp_dly_rev, permille_q12(a->fx.delay_to_reverb), 80u, j);
        ramp_to(&a->rp_dly_fb, permille_q12(a->fx.delay_feedback), 50u, j);
        ramp_to(&a->rp_dist_level, permille_q12(a->fx.dist_level), 80u, j);
        ramp_to(&a->rp_sub_level, permille_q12(a->sub_config.level), 50u, j);
        ramp_to(&a->rp_lead_level, permille_q12(a->config.lead_level), 50u, j);
        ramp_to(&a->rp_fsub_level,
                permille_q12(a->config.follower_sub_on ? a->config.follower_sub_level : 0u), 50u, j);
        ramp_to(&a->rp_delay, (int32_t)a->delay_samples, 50u, j);
        a->ramps_moving = true;
        if (j) a->dly_glide_q8 = a->rp_delay.cur, a->dly_glide_rem = 0u; /* (a jump: the glide too) */
    }
    if (glide_changed || a->applied_generation == 0u) delay_glide_set(a);
    if (dlyfx_changed || a->applied_generation == 0u) dlyfx_set(a);
    if (freeze_changed) frz_set(a);
    {
        /* the rest the per-sample path reads ready-made (mello_audio.h: "no divide") */
        const MelloSoundConfig *s = &a->config;
        a->lfo_depth_q15 = (int32_t)s->lfo_depth * 32768 / 1000;
        a->lfo_filter_span_q15 = (80 + (int32_t)s->lfo_depth * 920 / 1000) * 32768 / 1000;
        a->lfo_pitch_cents = 80 + (int32_t)s->lfo_depth * 620 / 1000;
        a->lfo_pwm_swing = (int32_t)s->lfo_depth * 420 / 1000;
        a->key_track_q15 = (int32_t)(((int64_t)s->key_track * 109 * 32768) / 2560000);
        a->osc2_q8_base = s->osc2_semi * 256 + s->osc2_fine * 256 / 100;
        a->osc2_mix_q15 = (int32_t)s->osc2_mix * 32768 / 1000;
        a->lead.osc2_r2 = 0u; /* taken again at the next sample */
        /* 2 pi f / fs, Q15: f = 20 Hz x 100^(lowcut / 1000), counted from 320 Hz (48 semitones up) */
        a->delay_lc_k =
            (int32_t)(((int64_t)semi_ratio_q16((int32_t)a->fx.delay_lowcut * 797 * 256 / 10000 - 48 * 256) *
                       1494) >>
                      16);
        a->drum_level_q12 =
            (int32_t)a->drum_config.level * 4096 / 3000; /* LEVEL 70%: a little over the lead */
        a->drum_fold_q12 = permille_q12(a->drum_config.fold);
        {
            uint8_t p;
            for (p = 0u; p < MELLO_SEND_PARTS; ++p) {
                a->send_dly_q12[p] = permille_q12(a->fx.send_dly[p]);
                a->send_rev_q12[p] = permille_q12(a->fx.send_rev[p]);
            }
            /* the chords send 800 / 1000 of their share, as their SPACE did */
            a->send_dly_q12[MELLO_SEND_CHORD] = (int32_t)a->fx.send_dly[MELLO_SEND_CHORD] * 3277 / 1000;
            a->send_rev_q12[MELLO_SEND_CHORD] = (int32_t)a->fx.send_rev[MELLO_SEND_CHORD] * 3277 / 1000;
        }
#if MELLO_FEATURE_CHORD_MACHINE
        a->chord_level_q12 = permille_q12(a->chord_config.level);
#endif
        lead_tracking(a);
    }
    a->applied_generation = g;
#if defined(MELLO_LIVE_EDIT_TEST_HOOK) && MELLO_LIVE_EDIT_TEST_HOOK
    a->test_config_applies++;
#endif
}
void mello_audio_init(MelloAudio *a, int16_t *db, uint32_t dc, int16_t *rb, uint32_t rc)
{
    if (!a) return;
    memset(a, 0, sizeof(*a));
    a->noise_seed = 0x2545F491u;
    a->comp_g = 32768;
    for (uint8_t k = 0u; k < MELLO_LA_N; ++k) a->la_g[k] = 32767; /* the limiter's window open */
    a->la_min = 32767, a->la_sum = 32767 * (int32_t)MELLO_LA_N;
    a->sub_gain = 32767;
    a->mix_gain[0] = a->mix_gain[1] = a->mix_gain[2] = a->mix_gain[3] = 32767;
    /* Unison oscillators start spread over the cycle, so that they never begin in phase (a flanged attack). */
    a->lead.uni_phase[0] = 0x55555555u;
    a->lead.uni_phase[1] = 0xAAAAAAAAu;
    a->lead.uni_phase[2] = 0x2AAAAAAAu;
    mello_sound_defaults(&a->pending_config);
    {
        MelloDrumConfig d;
        MelloBassConfig b;
        mello_drum_defaults(&d);
        mello_bass_defaults(&b);
        mello_audio_drum_kit(&d, &a->pending_drum_config);
        mello_audio_bass_voice(&b, &a->pending_bass_config);
    }
    mello_mod_routing_defaults(&a->pending_modr);
    a->modr = a->pending_modr;
    mello_fx_defaults(&a->pending_fx);
    mello_sub_defaults(&a->pending_sub_config);
    mello_sample_defaults(&a->pending_sample_config);
#if MELLO_FEATURE_CHORD_MACHINE
    mello_chord_defaults(&a->pending_chord_config);
#endif
    build_sub_dist_curve_for_drive(a->pending_sub_dist_curve, a->pending_sub_config.drive);
    build_main_dist_curve(a->pending_main_dist_curve, a->pending_fx.dist_amount,
                          a->pending_fx.dist_character);
    drives_makeup(a);
    a->delay_buffer = db;
    a->delay_capacity = dc;
    a->reverb_buffer = rb;
    a->reverb_capacity = rc;
    /* the delay's diffuser and pitch shifter after what either reverb uses, in a reverb memory of the FM-1's size */
    a->dlyfx = rb && rc == MELLO_AUDIO_REVERB_SAMPLES ? rb + MELLO_AUDIO_DLYFX_AT : 0;
    a->reverb_room = a->dlyfx ? MELLO_AUDIO_DLYFX_AT : rc;
    a->silent = true;
    a->config_generation = 2u;
    apply_pending(a);
    if (db && dc) memset(db, 0, dc * sizeof(*db));
    if (rb && rc) memset(rb, 0, rc * sizeof(*rb));
}
void mello_audio_set_config(MelloAudio *a, const MelloSoundConfig *s, const MelloFxConfig *f,
                            const MelloSubConfig *sub, const MelloSampleConfig *sample)
{
    uint32_t update;
    bool drive_changed;
    if (!a || !s || !f || !sub || !sample) return;
    update = config_update_begin(a);
    drive_changed = a->pending_sub_config.drive != sub->drive;
    if (drive_changed) {
        build_sub_dist_curve_for_drive(a->pending_sub_dist_curve, sub->drive);
#if defined(MELLO_LIVE_EDIT_TEST_HOOK) && MELLO_LIVE_EDIT_TEST_HOOK
        a->test_sub_curve_builds++;
#endif
    }
    if (a->pending_fx.dist_amount != f->dist_amount || a->pending_fx.dist_character != f->dist_character)
        build_main_dist_curve(a->pending_main_dist_curve, f->dist_amount, f->dist_character);
    a->pending_config = *s;
    a->pending_fx = *f;
    a->pending_sub_config = *sub;
    a->pending_sample_config = *sample;
    drives_makeup(a);
    config_update_end(a, update);
}
void mello_audio_set_sub_mute(MelloAudio *a, bool mute)
{
    if (a && a->sub_mute != mute) a->sub_mute = mute, a->gains_moving = true;
}
void mello_audio_set_perf_fx(MelloAudio *a, uint8_t fx)
{
    if (a) a->perf_fx = fx;
}
void mello_audio_set_perf_fx2(MelloAudio *a, uint8_t fx2, const uint16_t amt[8])
{
    uint8_t i;
    int8_t semis = 0;
    uint32_t ms;
    if (!a) return;
    for (i = 0u; i < 8u; ++i) a->pfx_amt[i] = amt ? (amt[i] > 1000u ? 1000u : amt[i]) : 0u;
    if (fx2 & MELLO_PERF_FX2_OCTDN) semis = (int8_t)(semis - (a->pfx_amt[5] >= 500u ? 24 : 12));
    if (fx2 & MELLO_PERF_FX2_OCTUP) semis = (int8_t)(semis + (a->pfx_amt[6] >= 500u ? 24 : 12));
    a->pfx_semis = semis;
    { /* PLUCK: each note dies in 400 .. 30 ms (mello_perf_pluck_ms), and its level made up: the dying notes cost
       * 1 .. 6.4 dB (measured over eight styles), 80% of it given back, so it stays near the level it took away
       * (DECISIONS "The level, all of it") */
        static const uint16_t PLUCK_MK[11] = {4491u, 4720u, 4961u, 5214u, 5480u, 5759u,
                                              6053u, 6362u, 6686u, 7027u, 7385u};
        uint32_t i = a->pfx_amt[2] / 100u, f = a->pfx_amt[2] % 100u;
        ms = mello_perf_pluck_ms(a->pfx_amt[2]);
        a->pluck_mk = i >= 10u ? 7385 - 4096
                               : (int32_t)(PLUCK_MK[i] + (PLUCK_MK[i + 1u] - PLUCK_MK[i]) * f / 100u) - 4096;
    }
    a->tape_frames = mello_perf_tape_ms(a->pfx_amt[1]) * 22u; /* (frames at half the rate) */
    a->pluck_k = (int32_t)(167772160u / (ms * 441u)); /* (the step towards silence a sample, of 2^24) */
    a->pfx = fx2;
}

/* PERFORM's black keys on the mix (DECISIONS "PERFORM's black keys"), after the sweeps. STUT loops what came in the
 * held division after the press (heard as it is the first time round); TAPE reads the record slowing to a stop and is
 * gone the moment it is let go (a 3 ms fade); WARP reads it 15 ms back, the delay swinging. Then CRUSH, CHOP, PLUCK. */
#define PB_Q (MELLO_PERF_BUF << 16)
static int32_t pb_read(const MelloAudio *a, uint8_t c, uint32_t pos_q16)
{
    uint32_t i = pos_q16 >> 16, j = i + 1u >= MELLO_PERF_BUF ? 0u : i + 1u, f = (pos_q16 >> 8) & 0xFFu;
    int32_t x0 = a->pbuf[c][i], x1 = a->pbuf[c][j];
    return x0 + (((x1 - x0) * (int32_t)f) >> 8);
}
static uint32_t pb_wrap(uint32_t q)
{
    return q >= PB_Q ? q - PB_Q : q;
}
static void perf2_step(MelloAudio *a, int32_t *l, int32_t *r)
{
    uint8_t fx = a->pfx, rise = (uint8_t)(fx & ~a->pfx_was);
    uint8_t mode = (fx & MELLO_PERF_FX2_TAPE)   ? 2u
                   : (fx & MELLO_PERF_FX2_STUT) ? 1u
                   : (fx & MELLO_PERF_FX2_WARP) ? 3u
                                                : 0u;
    int32_t dl = *l, dr = *r;
    uint32_t bpm = a->render_bpm ? a->render_bpm : 120u;
    a->pfx_was = fx;
    if (!(fx & MELLO_PERF_FX2_CRUSH)) a->crush_in = 0u; /* (their fades in start again at the next press) */
    if (!(fx & MELLO_PERF_FX2_WARP)) a->warp_in = 0u;
    if (mode && (mode != a->pb_mode || (rise & (MELLO_PERF_FX2_STUT | MELLO_PERF_FX2_TAPE)))) {
        a->pb_mode = mode, a->pb_t = 0u, a->pb_k = 0u;
        if (mode == 1u) { /* STUT: a division's length from now, at half the rate */
            static const uint8_t den[4] = {32u, 16u, 8u, 4u};
            /* a loop of a short division is a hit over and over: louder than the music by 4 dB at 1/32, 3 at 1/16,
             * 1 at 1/8 and 1/4 (measured); 80% of that taken back */
            static const uint16_t gain[4] = {2801u, 3072u, 3736u, 3779u};
            uint32_t len = 5292000u / (bpm * den[a->pfx_amt[0] * 3u / 1000u]);
            a->stut_q12 = gain[a->pfx_amt[0] * 3u / 1000u];
            a->pb_len = len < 64u ? 64u : len > MELLO_PERF_BUF - 64u ? MELLO_PERF_BUF - 64u : len;
            a->pb_pos_q16 = (uint32_t)a->pb_w << 16;
        } else if (mode == 2u) { /* TAPE: from now, a stop in 100 .. 500 ms */
            uint32_t frames = a->tape_frames ? a->tape_frames : 600u * 22u;
            a->pb_pos_q16 = pb_wrap(((uint32_t)a->pb_w << 16) + PB_Q - (2u << 16));
            a->pb_speed_q16 = 65536u, a->pb_dec_q16 = 65536u / frames;
        }
    } else if (!mode && !a->pb_mix_q12)
        a->pb_mode = 0u;
    if (a->pb_mode) {
        int32_t wl = dl, wr = dr, g = 4096;
        bool record = a->pb_mode != 1u || a->pb_t < a->pb_len * 2u; /* STUT records its first time round */
        /* TAPE: the reader falls behind half the stop's length; a buffer behind (~0.28 s), the record stops and the
         * reader runs on into what it holds, so a stop of up to ~1.2 s fits */
        if (a->pb_mode == 2u &&
            ((a->pb_w + MELLO_PERF_BUF - (a->pb_pos_q16 >> 16)) % MELLO_PERF_BUF) >= MELLO_PERF_BUF - 64u)
            record = false;
        if (record) { /* the record, two samples a frame */
            a->pb_acc[0] += dl, a->pb_acc[1] += dr;
            if (a->pb_half) {
                a->pbuf[0][a->pb_w] = (int16_t)clamp32(a->pb_acc[0] / 2, -32767, 32767);
                a->pbuf[1][a->pb_w] = (int16_t)clamp32(a->pb_acc[1] / 2, -32767, 32767);
                a->pb_acc[0] = a->pb_acc[1] = 0;
                if (++a->pb_w >= MELLO_PERF_BUF) a->pb_w = 0u;
            }
            a->pb_half ^= 1u;
        }
        if (a->pb_mode == 1u) {
            uint32_t n = a->pb_len * 2u, k = a->pb_k;
            if (a->pb_t >= n) { /* round and round, a 64-sample fade either side of the seam */
                wl = pb_read(a, 0u, pb_wrap(a->pb_pos_q16 + (k << 15)));
                wr = pb_read(a, 1u, pb_wrap(a->pb_pos_q16 + (k << 15)));
                if (k < 64u) g = (int32_t)k * 64;
                if (n - k < 64u) g = (int32_t)(n - k) * 64;
                g = (g * a->stut_q12) >> 12;
                if (++a->pb_k >= n) a->pb_k = 0u;
            } else
                a->pb_t++;
        } else if (a->pb_mode == 2u) {
            wl = pb_read(a, 0u, a->pb_pos_q16), wr = pb_read(a, 1u, a->pb_pos_q16);
            a->pb_pos_q16 = pb_wrap(a->pb_pos_q16 + (a->pb_speed_q16 >> 1));
            a->pb_speed_q16 =
                a->pb_speed_q16 > (a->pb_dec_q16 >> 1) ? a->pb_speed_q16 - (a->pb_dec_q16 >> 1) : 0u;
            g = a->pb_speed_q16 >= 16384u ? 4096 : (int32_t)(a->pb_speed_q16 >> 2); /* (silent as it stops) */
        } else { /* WARP: 15 ms back, swinging up to 13.6 ms either way, slow and fast */
            uint32_t d;
            int32_t sw;
            a->warp_ph[0] += 126600u, a->warp_ph[1] += 633000u;
            sw = (SINE_Q15[a->warp_ph[0] >> 24] * 7 + SINE_Q15[a->warp_ph[1] >> 24] * 3) / 10;
            if (a->warp_in < 13230u) a->warp_in++; /* its depth in over 300 ms, from none */
            d = (uint32_t)(330 * 65536 +
                           (int32_t)((uint32_t)a->pfx_amt[7] * a->warp_in / 13230u) * 300 / 1000 * sw * 2);
            wl = pb_read(a, 0u, pb_wrap(((uint32_t)a->pb_w << 16) + PB_Q - d));
            wr = pb_read(a, 1u, pb_wrap(((uint32_t)a->pb_w << 16) + PB_Q - d));
        }
        wl = (wl * g) >> 12, wr = (wr * g) >> 12;
        /* in over 10 ms; out in 3 ms, at once as near as clicks allow */
        a->pb_mix_q12 = mode ? (a->pb_mix_q12 + 9 > 4096 ? 4096 : a->pb_mix_q12 + 9)
                             : (a->pb_mix_q12 > 31 ? a->pb_mix_q12 - 31 : 0);
        *l = dl + (((wl - dl) * a->pb_mix_q12) >> 12);
        *r = dr + (((wr - dr) * a->pb_mix_q12) >> 12);
    }
    if (fx &
        MELLO_PERF_FX2_CRUSH) { /* rate held 1 .. 16 samples, 0 .. 10 bits gone; in over 200 ms, from none */
        uint32_t amt = a->pfx_amt[3] * (a->crush_in < 8820u ? ++a->crush_in : 8820u) / 8820u;
        uint8_t hold = (uint8_t)(1u + amt * 15u / 1000u);
        int32_t mask = ~((1 << (amt * 10u / 1000u)) - 1);
        if (++a->crush_n >= hold) a->crush_n = 0u, a->crush_h[0] = *l & mask, a->crush_h[1] = *r & mask;
        *l = a->crush_h[0], *r = a->crush_h[1];
    }
    if (fx & MELLO_PERF_FX2_CHOP) { /* on for the first 60% of each 1/8, 1/16, 1/32 from the press */
        static const uint8_t den[3] = {8u, 16u, 32u};
        if (rise & MELLO_PERF_FX2_CHOP)
            a->chop_n = 0u, a->chop_len = 10584000u / (bpm * den[a->pfx_amt[4] * 2u / 1000u]);
        if (a->chop_len < 2u) a->chop_len = 2u;
        if (++a->chop_n >= a->chop_len) a->chop_n = 0u;
        a->chop_g = a->chop_n * 5u < a->chop_len * 3u ? (a->chop_g > 93 ? a->chop_g - 93 : 0)
                                                      : (a->chop_g + 93 > 4096 ? 4096 : a->chop_g + 93);
    } else
        a->chop_g = a->chop_g > 93 ? a->chop_g - 93 : 0; /* (chop_g: how far cut, 0 open) */
    /* CHOP's open part 1.2 dB up while held (it cuts 40% of the time: 1.1 .. 1.5 dB less, measured), and PLUCK's
     * makeup: each slides there and back over ~6 ms */
    {
        int32_t to = (fx & MELLO_PERF_FX2_CHOP) ? 607 : 0;
        a->chop_mk += a->chop_mk < to ? 2 : a->chop_mk > to ? -2 : 0;
        to = (fx & MELLO_PERF_FX2_PLUCK) ? a->pluck_mk : 0;
        a->pluck_mk_now += a->pluck_mk_now < to - 11   ? 12
                           : a->pluck_mk_now > to + 11 ? -12
                                                       : to - a->pluck_mk_now;
    }
    if (a->chop_g | a->chop_mk) {
        int32_t g = ((4096 - a->chop_g) * (4096 + a->chop_mk)) >> 12;
        *l = (*l * g) >> 12, *r = (*r * g) >> 12;
    }
    /* (pluck_q24: how far down, 0 open; Q24, so 400 ms is a step of 951 and not lost to rounding as it was in Q12,
     * where every amount under ~80% died alike in 93 ms) */
    if ((fx & MELLO_PERF_FX2_PLUCK) && !a->pluck_up)
        a->pluck_q24 += (int32_t)(((int64_t)((1 << 24) - a->pluck_q24) * a->pluck_k) >> 24) + 1;
    else if (a->pluck_q24) {
        a->pluck_q24 = a->pluck_q24 > (62 << 12) ? a->pluck_q24 - (62 << 12) : 0; /* a note: up in 1.5 ms */
        if (!a->pluck_q24) a->pluck_up = false;
    } else
        a->pluck_up = false;
    if (a->pluck_q24 > (1 << 24)) a->pluck_q24 = 1 << 24;
    if (a->pluck_q24 | a->pluck_mk_now) {
        int32_t g = ((4096 - (a->pluck_q24 >> 12)) * (4096 + a->pluck_mk_now)) >> 12;
        *l = (*l * g) >> 12, *r = (*r * g) >> 12;
    }
}
void mello_audio_set_mix_mute(MelloAudio *a, uint8_t mute_bits)
{
    if (a && a->mix_mute != mute_bits) a->mix_mute = mute_bits, a->gains_moving = true;
}
void mello_audio_set_mod_routing(MelloAudio *a, const MelloModRouting *m)
{
    uint32_t update;
    if (!a || !m) return;
    update = config_update_begin(a);
    a->pending_modr = *m;
    config_update_end(a, update);
}
void mello_audio_drum_kit(const MelloDrumConfig *c, MelloAudioDrumKit *k)
{
    memset(k, 0, sizeof *k);
    k->on = c->on;
    k->level = c->level, k->space = c->space, k->drive = c->drive, k->fold = c->fold, k->duck = c->duck;
    memcpy(k->voice, c->voice, sizeof k->voice);
}
void mello_audio_bass_voice(const MelloBassConfig *c, MelloAudioBassVoice *v)
{
    v->res = c->res, v->env = c->env, v->env_decay_ms = c->env_decay_ms, v->accent = c->accent;
}
void mello_audio_set_bass_config(MelloAudio *a, const MelloBassConfig *c)
{
    uint32_t update;
    if (!a || !c) return;
    update = config_update_begin(a);
    mello_audio_bass_voice(c, &a->pending_bass_config);
    config_update_end(a, update);
}
void mello_audio_set_drum_config(MelloAudio *a, const MelloDrumConfig *c)
{
    uint32_t update;
    if (!a || !c) return;
    update = config_update_begin(a);
    mello_audio_drum_kit(c, &a->pending_drum_config);
    drives_makeup(a);
    config_update_end(a, update);
}
#if MELLO_FEATURE_CHORD_MACHINE
void mello_audio_set_chord_config(MelloAudio *a, const MelloChordConfig *c)
{
    uint32_t update;
    if (!a || !c) return;
    update = config_update_begin(a);
    a->pending_chord_config = *c;
    config_update_end(a, update);
}
#endif
void mello_audio_attach_sample_bank(MelloAudio *a, MelloSampleBank *bank)
{
    if (a) a->sample_bank = bank;
}
static bool audio_due_before(uint32_t a, uint32_t b)
{
    return (int32_t)(a - b) < 0;
}
static uint8_t audio_lane_slot(uint8_t lane, uint8_t pos)
{
    return (uint8_t)(lane * MELLO_AUDIO_EVENT_LANE_CAP + pos);
}
static bool audio_lane_empty(const MelloAudio *a, uint8_t lane)
{
    return a->q_head[lane] == a->q_tail[lane];
}
bool mello_audio_push_engine_event(MelloAudio *a, const MelloEvent *e, uint32_t due)
{
    uint8_t lane, best = 255u;
    if (!a || !e) return false;
    /* Choose a lane whose published due times stay monotonic. Empty lanes may restart at
     * any timestamp because the ISR has already consumed every older entry in that lane.
     * Producer never rewrites a published entry, so this remains safe against app_audio(). */
    for (lane = 0u; lane < MELLO_AUDIO_EVENT_LANES; ++lane) {
        uint8_t tail = a->q_tail[lane], next = (uint8_t)((tail + 1u) % MELLO_AUDIO_EVENT_LANE_CAP);
        if (next == a->q_head[lane]) continue;
        if (audio_lane_empty(a, lane)) {
            best = lane;
            break;
        }
        if (!audio_due_before(due, a->q_last_due[lane])) {
            if (best == 255u || audio_due_before(a->q_last_due[best], a->q_last_due[lane])) best = lane;
        }
    }
    if (best == 255u) {
        a->queue_overflow = true;
        return false;
    }
    {
        uint8_t tail = a->q_tail[best];
        uint8_t slot = audio_lane_slot(best, tail);
        a->queue[slot].due_us = due;
        a->queue[slot].event = *e;
        a->q_last_due[best] = due;
        a->q_tail[best] = (uint8_t)((tail + 1u) % MELLO_AUDIO_EVENT_LANE_CAP);
    }
    return true;
}
static bool queue_pop_due(const MelloAudio *a, volatile uint8_t *heads, uint32_t now,
                          MelloAudioQueuedEvent *out)
{
    uint8_t lane, best = 255u;
    uint32_t best_due = 0u;
    for (lane = 0u; lane < MELLO_AUDIO_EVENT_LANES; ++lane) {
        uint8_t head = heads[lane];
        uint8_t slot;
        uint32_t due;
        if (head == a->q_tail[lane]) continue;
        slot = audio_lane_slot(lane, head);
        due = a->queue[slot].due_us;
        if (!audio_time_due(now, due)) continue;
        if (best == 255u || audio_due_before(due, best_due)) {
            best = lane;
            best_due = due;
        }
    }
    if (best == 255u) return false;
    {
        uint8_t head = heads[best];
        uint8_t slot = audio_lane_slot(best, head);
        *out = a->queue[slot];
        heads[best] = (uint8_t)((head + 1u) % MELLO_AUDIO_EVENT_LANE_CAP);
    }
    return true;
}
static bool audio_pop_due(MelloAudio *a, uint32_t now, MelloAudioQueuedEvent *out)
{
    return queue_pop_due(a, a->q_head, now, out);
}

/* The chords and the drums a chunk at a time (ROADMAP H2-H3, DECISIONS "The second core"): on the second core a
 * chunk ahead, beside the rest of the render, or here a sample at a time where the render needs it: the same samples
 * either way, and the same as when they were rendered inline. parts_step reads the queue on its own read positions
 * with the render's own times, so a chord note or a drum hit lands on its sample; it keeps its own copy of the MOD
 * lanes and EVOLVE's value for C.CUT and the drums, and of the LFO's control steps and the sample count for the
 * drums' share; it writes only the chord and drum state and the chunk, and counts each sample in parts_done.
 * STUTTER's memory needs the music, so the render writes it from the drums' share (counting in heard_done), and
 * the drums read it through drum_memory. */
static MelloAudio
    *parts_job_audio; /* (the job's: a pointer does not fit the platform's 32-bit argument on the host) */
/* the first due time at the job's read positions: it looks into the queue only once that comes (the render pops at
 * every sample; nothing is queued while the render runs) */
static void parts_job_next(const MelloAudio *a, MelloPartsJob *j)
{
    uint8_t lane;
    j->queued = false;
    for (lane = 0u; lane < MELLO_AUDIO_EVENT_LANES; ++lane) {
        uint32_t due;
        if (j->q_head[lane] == a->q_tail[lane]) continue;
        due = a->queue[audio_lane_slot(lane, j->q_head[lane])].due_us;
        if (!j->queued || audio_due_before(due, j->next_due)) j->next_due = due;
        j->queued = true;
    }
}
static bool parts_dest(uint8_t d) /* a destination the job works out itself */
{
    d %= MELLO_DEST_COUNT;
    return d == MELLO_DEST_CHORD_CUT || (d >= MELLO_DEST_DRUM_TUNE && d <= MELLO_DEST_DRUM_DRIVE);
}
/* the chunk's next sample (parts_done), as the render's loop had it: its events, the routes, the drums' share, the
 * chords, the drums */
static void parts_step(MelloAudio *a)
{
    MelloPartsJob *j = &a->parts_job;
    const MelloAudioCores *c = a->cores;
    uint32_t s = a->parts_done;
    MelloAudioQueuedEvent q;
    int32_t dm[5] = {0, 0, 0, 0, 0}; /* the routes on C.CUT, DRUM TUNE .. DRUM DRIVE, as the render's dmod */
    bool mods = false;
    uint8_t k;
    while (j->queued && audio_time_due(j->t, j->next_due) && queue_pop_due(a, j->q_head, j->t, &q)) {
        const MelloEvent *e = &q.event;
#if MELLO_FEATURE_CHORD_MACHINE
        if (e->type == MELLO_EVENT_CHORD_NOTE_ON && j->chords)
            chord_note_on(a, e->note);
        else if (e->type == MELLO_EVENT_CHORD_NOTE_OFF && j->chords)
            chord_note_off(a, e->note);
        else
#endif
            if (e->type == MELLO_EVENT_DRUM_HIT && j->drums)
            drum_hit(a, e);
        else if (e->type == MELLO_EVENT_PULSE && j->routed) { /* as apply_event: EVOLVE's value, the lanes */
            j->tm_value = e->tm_value;
            for (k = 0u; k < MELLO_DOT_MOD_LANES; ++k)
                if (e->dot_mod[k] != MELLO_DOT_NOT_READ) dot_lane_pulse(a, &j->dot[k], k, e->dot_mod[k]);
        }
        parts_job_next(a, j);
    }
    if (j->routed && (j->dot[0].stage || j->dot[1].stage || j->tm_value)) { /* as the render's dmod */
        mods = true;
        for (k = 0u; k < MELLO_DOT_MOD_LANES; ++k) {
            uint8_t dd = a->modr.dot_target[k] % MELLO_DEST_COUNT, td = a->modr.tm_dest[k] % MELLO_DEST_COUNT;
            dot_lane_tick(a, &j->dot[k], k);
            if (dd == MELLO_DEST_CHORD_CUT) dm[0] += j->dot[k].v >> 16;
            if (dd >= MELLO_DEST_DRUM_TUNE && dd <= MELLO_DEST_DRUM_DRIVE)
                dm[1 + dd - MELLO_DEST_DRUM_TUNE] += j->dot[k].v >> 16;
            if (td == MELLO_DEST_CHORD_CUT) dm[0] += j->tm_value * a->modr.tm_depth[k] / 32768;
            if (td >= MELLO_DEST_DRUM_TUNE && td <= MELLO_DEST_DRUM_DRIVE)
                dm[1 + td - MELLO_DEST_DRUM_TUNE] += j->tm_value * a->modr.tm_depth[k] / 32768;
        }
    }
    if (j->lfo_drums && !(j->ctl_phase++ & 3u)) { /* as the render's control step: the LFO */
        uint32_t ph = j->lfo_phase;
        int32_t s0 = SINE_Q15[(uint8_t)(ph >> 24)], s1 = SINE_Q15[(uint8_t)((ph >> 24) + 1u)];
        j->lfo_phase = ph + a->lfo_inc * 4u;
        j->c_lfo = s0 + (((s1 - s0) * (int32_t)((ph >> 16) & 0xFFu)) >> 8);
    }
    if (j->drums &&
        !(j->rendered & 15u)) { /* the drums' share, every 16 samples: read at each hit and by DRIVE */
        int32_t lt = (int32_t)a->config.lfo_target - (int32_t)MELLO_LFO_DRUM_TUNE;
        for (k = 0u; k < 4u; ++k) {
            int32_t m = mods ? dm[1 + k] : 0;
            if (a->config.lfo_on && lt == (int32_t)k) m += (j->c_lfo * (int32_t)a->config.lfo_depth) >> 15;
            a->drum_mod[k] = (int16_t)clamp32(m, -1000, 1000);
        }
    }
#if MELLO_FEATURE_CHORD_MACHINE
    if (j->chords) {
        int32_t l = 0, r = 0;
        if ((a->chord.env_stage != MELLO_ENV_OFF || a->chord.active_count) &&
            a->chord_config.wave != MELLO_WAVE_OFF)
            chord_render(a, clamp32(mods ? dm[0] : 0, -1000, 1000), &l, &r);
        a->chord_out[0][s] = l;
        a->chord_out[1][s] = r;
    }
#endif
    if (j->reverb) { /* the reverb's network and its return, PRE behind what the render writes */
        int32_t l = 0, r = 0;
        reverb_out(a, &l, &r);
        reverb_return(a, l, r, &a->rev_out[0][s], &a->rev_out[1][s]);
    }
    if (j->drums) {
        int32_t o[3];
        drum_voices(a, s, o);
        a->drum_out[0][s] = o[0], a->drum_out[1][s] = o[1], a->drum_out[2][s] = o[2];
        j->buf_pos = (uint16_t)((j->buf_pos + 1u) & (MELLO_DRUM_BUF - 1u));
    }
    j->rendered++;
    j->t += AUDIO_US_BASE;
    j->terr += AUDIO_US_REM;
    if (j->terr >= MELLO_AUDIO_FS) {
        j->terr -= MELLO_AUDIO_FS;
        j->t++;
    }
    if (c && c->sync) c->sync(); /* the sample, then its count */
    a->parts_done = s + 1u;
}
static void parts_job_entry(uint32_t arg)
{
    MelloAudio *a = parts_job_audio;
    uint32_t (*micros)(void) = a->cores->micros, t0 = micros ? micros() : 0u;
    (void)arg;
    while (a->parts_done < a->parts_job.n) parts_step(a);
    if (micros) a->parts_busy += micros() - t0; /* (the render reads it once the job is back) */
}
/* The reverb's network and return go with the chunk on two cores when nothing in them needs this chunk's input: WET
 * up (the reverb runs every sample, whatever its tail), PRE at least a chunk (the network reads what the render wrote
 * a chunk or more before), and no diffuser waiting to move (at the tail's end, the render's to see). Else here. */
static bool parts_reverb(const MelloAudio *a)
{
    uint8_t k;
    if (!a->cores || !a->reverb_buffer || a->reverb_capacity <= 1024u || a->reverb_pre_cap < 2u ||
        a->reverb_pre_samples < OLIVERB_PRE_MIN)
        return false;
    /* Oliverb: there, whenever the reverb runs (its tail, WASH; the render then feeds it the whole chunk) */
    if (a->oliverb_on) return a->fx.reverb_wet || a->reverb_live || a->wash_q12;
    if (!a->fx.reverb_wet) return false;
    for (k = 0u; k < 3u; ++k)
        if (a->reverb_ap_now[k] != a->reverb_ap_delay[k]) return false;
    return true;
}
/* a chunk of n samples from the render's sample at time t: to the second core, or here a sample at a time */
static void parts_start(MelloAudio *a, uint32_t t, uint32_t terr, uint32_t n)
{
    MelloPartsJob *j = &a->parts_job;
    int32_t lt = (int32_t)a->config.lfo_target - (int32_t)MELLO_LFO_DRUM_TUNE;
    uint8_t k;
    j->t = t, j->terr = terr, j->n = n;
    for (k = 0u; k < MELLO_AUDIO_EVENT_LANES; ++k) j->q_head[k] = a->q_head[k];
    parts_job_next(a, j);
    memcpy(j->dot, a->dot_lane, sizeof j->dot);
    j->tm_value = a->tm_value;
    j->routed = false;
    for (k = 0u; k < MELLO_DOT_MOD_LANES; ++k)
        if (parts_dest(a->modr.dot_target[k]) || parts_dest(a->modr.tm_dest[k])) j->routed = true;
    j->drums = a->drum_config.on;
#if MELLO_FEATURE_CHORD_MACHINE
    j->chords = a->chord_config.on;
#else
    j->chords = false;
#endif
    j->reverb = parts_reverb(a);
    j->lfo_drums = j->drums && a->config.lfo_on && lt >= 0 && lt < 4;
    j->ctl_phase = a->ctl_phase, j->lfo_phase = a->lfo_phase, j->c_lfo = a->c_lfo;
    j->rendered = a->rendered_frames;
    j->buf_pos = j->buf_pos0 = a->drum_buf_pos;
    a->parts_done = a->heard_done = 0u;
    parts_job_audio = a;
    a->parts_on_core1 = false;
    if (a->cores) {
        a->cores->sync();
        a->parts_on_core1 = a->cores->run(parts_job_entry, 0u) != 0;
        if (!a->parts_on_core1) { /* refused: the second core is off until power-off, so one core from here */
            a->cores = 0;
            reverb_choose(a); /* V3 before this chunk's first sample */
            j->reverb = parts_reverb(a);
        }
    }
}
/* the job given up (late, or its core read it wrong and is held): from its last sample, here */
static void parts_take_back(MelloAudio *a)
{
    if (a->cores->wait() < 0) a->cores = 0; /* lost: the core held until power-off; V3 from the next chunk */
    a->parts_on_core1 = false;
}
/* the render at the chunk's sample s: waits for it while the second core is behind, or does it here */
static void parts_at(MelloAudio *a, uint32_t s)
{
    if (a->parts_on_core1) {
        uint32_t spins = 0u;
        while (a->parts_done <= s) {
            a->cores->sync();
            if (++spins == 1u << 20) { /* far past a sample: wait for the job, or give it up */
                parts_take_back(a);
                break;
            }
        }
        if (a->parts_on_core1) {
            a->cores->sync();
            return;
        }
    }
    while (a->parts_done <= s) parts_step(a);
}
/* STUTTER's memory for the chunk's sample s: the music and the drums, a quarter of full scale; counted for the job */
static void parts_heard(MelloAudio *a, uint32_t s, int32_t music)
{
    a->drum_buf[a->drum_buf_pos] = (int16_t)clamp32((music * 4 + a->drum_out[2][s]) >> 2, -32767, 32767);
    a->drum_buf_pos = (uint16_t)((a->drum_buf_pos + 1u) & (MELLO_DRUM_BUF - 1u));
    if (a->parts_on_core1) a->cores->sync();
    a->heard_done = s + 1u;
}
/* the chunk done: the second core free for the next */
static void parts_finish(MelloAudio *a)
{
    if (a->parts_on_core1) parts_take_back(a);
}
void mello_audio_set_cores(MelloAudio *a, const MelloAudioCores *cores)
{
    if (a) a->cores = cores && cores->run && cores->wait && cores->sync ? cores : 0;
}
void mello_audio_set_oliverb(MelloAudio *a, bool on)
{
    if (a) a->oliverb_off = !on; /* (taken at the next chunk: reverb_choose) */
}
void mello_audio_event_sink(void *ctx, const MelloEvent *e, uint32_t due)
{
    (void)mello_audio_push_engine_event((MelloAudio *)ctx, e, due);
}
void mello_audio_all_notes_off(MelloAudio *a)
{
    if (!a) return;
    a->all_notes_off_pending = true;
}
bool mello_audio_queue_overflowed(const MelloAudio *a)
{
    return a ? a->queue_overflow : false;
}
bool mello_audio_is_silent(const MelloAudio *a)
{
    return !a || a->silent;
}
static int32_t soft_drive(int32_t x, uint16_t amount, uint16_t character)
{
    int32_t gain = 32768 + (int32_t)amount * (12 + (int32_t)character / 40);
    if (gain > 65535) gain = 65535;
    x = clamp32(x, -32767, 32767);
    int32_t y = q15mul(x, gain);
    int32_t lim = 32767 - (int32_t)amount * 8;
    if (lim < 12000) lim = 12000;
    if (y > lim) y = lim + ((y - lim) >> 3);
    if (y < -lim) y = -lim + ((y + lim) >> 3);
    return clamp32(y, -32767, 32767);
}
/* PERFORM's sweeps (DECISIONS "PERFORM: performance only"): a state variable filter on the whole mix, its cutoff
 * gliding while the key is held, low-pass from 7.6 kHz down to 200 Hz over 1.5 s, high-pass from 30 Hz up to 2 kHz,
 * and back in a quarter of a second when let go; open, it is out of the path. Its coefficient, 2 sin(pi fc / fs) in
 * Q15, on an exponential 33-point table; the first eighth of the way it fades in, so it starts without a step. */
static const int32_t SWEEP_F_Q15[2][33] = {
    {33774, 30451, 27398, 24610, 22078, 19785, 17716, 15853, 14178, 12675, 11328,
     10121, 9041,  8074,  7211,  6438,  5748,  5132,  4582,  4090,  3651,  3259,
     2909,  2597,  2318,  2069,  1847,  1648,  1471,  1313,  1172,  1046,  934},
    {140,  160,  182,  208,  237,  270,  308,  351,  400,  456,  520,  593,  677,  771,  880,  1003, 1144,
     1304, 1487, 1695, 1933, 2204, 2513, 2865, 3266, 3724, 4246, 4840, 5517, 6289, 7167, 8168, 9306}};
#define SWEEP_FULL     (1u << 20)
#define SWEEP_UP       16u  /* 1.5 s to close */
#define SWEEP_DOWN     95u  /* a quarter of a second back */
#define SWEEP_DAMP_Q12 3277 /* 0.8: a little resonance at the edge */
static void sweep_step(MelloAudio *a, int32_t *l, int32_t *r)
{
    uint8_t k;
    /* Both held (DECISIONS "LP and HP together"): the two would close on each other into silence; instead LP SWP's
     * filter gives its band (a resonant band gliding down) and HP SWP opens out of the way. The change is faded
     * over ~10 ms each way, so pressing the second key does not click. */
    bool band = (a->perf_fx & 3u) == 3u;
    a->sweep_band = band ? (a->sweep_band < 4096 - 8 ? a->sweep_band + 8 : 4096)
                         : (a->sweep_band > 8 ? a->sweep_band - 8 : 0);
    for (k = 0u; k < 2u; ++k) {
        bool held = ((a->perf_fx >> k) & 1u) && !(band && k == 1u);
        uint32_t pos = a->sweep_pos[k], i, fr;
        int32_t f, mixq, c;
        pos = held ? (pos + SWEEP_UP < SWEEP_FULL ? pos + SWEEP_UP : SWEEP_FULL)
                   : (pos > SWEEP_DOWN ? pos - SWEEP_DOWN : 0u);
        a->sweep_pos[k] = pos;
        if (!pos) {
            if (a->sweep_mix[k]) memset(a->sweep_s[k], 0, sizeof a->sweep_s[k]), a->sweep_mix[k] = 0;
            continue;
        }
        if (!(a->rendered_frames & 15u) || !a->sweep_mix[k]) { /* the coefficient every 16 samples */
            i = pos >> 15, fr = pos & 0x7FFFu;                 /* 32 steps of 2^15 */
            a->sweep_f[k] =
                SWEEP_F_Q15[k][i] +
                (i < 32u ? (int32_t)(((SWEEP_F_Q15[k][i + 1u] - SWEEP_F_Q15[k][i]) * (int32_t)fr) >> 15) : 0);
            a->sweep_mix[k] =
                pos >= (SWEEP_FULL >> 3) ? 4096 : (int32_t)(pos >> 5) | 1; /* faded in, the first eighth */
        }
        f = a->sweep_f[k], mixq = a->sweep_mix[k];
        for (c = 0; c < 2; ++c) {
            int32_t *x = c ? r : l, *s = a->sweep_s[k][c], hp, y;
            s[0] += (int32_t)(((int64_t)f * s[1]) >> 15);                       /* low */
            hp = *x - s[0] - (int32_t)(((int64_t)SWEEP_DAMP_Q12 * s[1]) >> 12); /* high */
            s[1] += (int32_t)(((int64_t)f * hp) >> 15);                         /* band */
            s[0] = clamp32(s[0], -262143, 262143), s[1] = clamp32(s[1], -262143, 262143);
            y = k ? hp : s[0];
            if (!k && a->sweep_band) /* towards the band (with the damping's 1.25 back out of it) */
                y += (int32_t)(((int64_t)(s[1] * 4 / 5 - y) * a->sweep_band) >> 12);
            *x += (int32_t)(((int64_t)(y - *x) * mixq) >> 12);
        }
    }
}
void mello_audio_render(MelloAudio *a, int32_t *out, uint32_t frames, uint32_t start_us, uint16_t bpm)
{
    uint32_t i, t = start_us, terr = 0u, parts_at0 = 0u; /* the sample the parts' chunk began at */
    bool parts_on = false;
    if (!a || !out) return;
    a->render_bpm = bpm;
    apply_pending(a);
    /* Bound the IMA decode work per call whatever the sample settings (P1-4). Normal playback needs about
     * one block per voice per 256 frames; four voices at +24 semitones need 16. */
    a->sample_decode_budget = MELLO_SAMPLE_DECODE_BUDGET;
    if (a->silent && !a->all_notes_off_pending && !a->dot_lane[0].stage && !a->dot_lane[1].stage) {
        /* Nothing sounds and nothing is due: the block stays zero. (This compared the two lane arrays' addresses
         * before, never equal, so the whole loop ran through the silence: 55 M instructions/s on the chip.) */
        uint8_t ql;
        for (ql = 0u; ql < MELLO_AUDIO_EVENT_LANES; ++ql)
            if (!audio_lane_empty(a, ql)) break;
        if (ql == MELLO_AUDIO_EVENT_LANES) {
            /* the mutes' gains still settle, as they would have a sample at a time (ui_perf_keys), and the LFO
             * runs on through the silence as the HTML's does (lfo_amp's parity) */
            int32_t step = 75 * (int32_t)frames;
            uint8_t m;
            if (a->config.lfo_on) a->lfo_phase += a->lfo_inc * frames;
            a->sub_gain = clamp32(a->sub_gain + (a->sub_mute ? -step : step), 0, 32767);
            for (m = 0u; m < 4u; ++m)
                a->mix_gain[m] =
                    clamp32(a->mix_gain[m] + (((a->mix_mute >> m) & 1u) ? -step : step), 0, 32767);
            /* GLIDE: nothing is heard, so a new time is there at once (not a glide at the next PLAY) */
            a->dly_glide_q8 = a->rp_delay.target, a->dly_glide_rem = 0u;
            a->rendered_frames += frames;
            return;
        }
    }
    if (a->all_notes_off_pending) {
        {
            uint8_t ql;
            for (ql = 0u; ql < MELLO_AUDIO_EVENT_LANES; ++ql) a->q_head[ql] = a->q_tail[ql];
        }
        lead_release(a);
        sub_release(a);
#if MELLO_FEATURE_CHORD_MACHINE
        chord_kill(a);
#endif
        {
            uint8_t sv;
            for (sv = 0u; sv < MELLO_SAMPLE_SLOT_COUNT; ++sv) sample_release_voice(a, sv);
        }
        a->all_notes_off_pending = false;
    }
    if (a->fx.delay_sync && a->delay_capacity > 1u) {
        uint32_t qms = 60000u / (bpm ? bpm : 80u);
        uint32_t ms;
        ms = mello_delay_div_ms(a->fx.delay_div, qms);
        a->delay_samples = (ms * 441u + 5u) / 10u;
        delay_fit(a);
        ramp_to(&a->rp_delay, (int32_t)a->delay_samples, 50u, a->rendered_frames == 0u);
        if (a->rp_delay.left) a->ramps_moving = true;
        if (a->rendered_frames == 0u) a->dly_glide_q8 = a->rp_delay.cur, a->dly_glide_rem = 0u;
    }
    /* The routes' sums per destination, zeroed once a block: a sample clears only the entries the routes write (the
     * block's routes, apply_pending above), as zeroing all of it each sample costs this chip a 72-byte loop. */
    int32_t dmod[MELLO_DEST_COUNT] = {0};
    uint8_t mod_at[2u * MELLO_DOT_MOD_LANES];
    for (i = 0u; i < MELLO_DOT_MOD_LANES; ++i) {
        mod_at[2u * i] = (uint8_t)(a->modr.dot_target[i] % MELLO_DEST_COUNT);
        mod_at[2u * i + 1u] = (uint8_t)(a->modr.tm_dest[i] % MELLO_DEST_COUNT);
    }
    for (i = 0u; i < frames; ++i) {
        int32_t lead = 0, sub = 0, sample_mix = 0, mix, mix_l = 0, mix_r = 0, lfo = 0, ampmod = 32767,
                mod_env;
        bool mods = false;
#if MELLO_FEATURE_CHORD_MACHINE
        int32_t chord_mix = 0, chord_l = 0, chord_r = 0;
#endif
        if (!(i % MELLO_PARTS_CHUNK)) { /* the parts' next chunk, from here, before this sample's events */
            parts_finish(a);
            reverb_choose(a);
            parts_on = a->drum_config.on || parts_reverb(a);
#if MELLO_FEATURE_CHORD_MACHINE
            parts_on = parts_on || a->chord_config.on;
#endif
            parts_at0 = i;
            if (parts_on)
                parts_start(a, t, terr, frames - i < MELLO_PARTS_CHUNK ? frames - i : MELLO_PARTS_CHUNK);
        }
        int32_t drums = 0, drums_l = 0, drums_r = 0, bass = 0, dly_heard = 0;
        bool rm = a->ramps_moving;
        uint16_t cut = (uint16_t)(RAMP(a, rm, rp_cut) >> 8), res = (uint16_t)(RAMP(a, rm, rp_res) >> 8);
        int32_t master = RAMP(a, rm, rp_master) >> 8, rev_send = RAMP(a, rm, rp_rev_send) >> 8,
                rev_level = RAMP(a, rm, rp_rev_level) >> 8, dly_send = RAMP(a, rm, rp_dly_send) >> 8,
                dly_level = RAMP(a, rm, rp_dly_level) >> 8, dly_fb = RAMP(a, rm, rp_dly_fb) >> 8,
                dist_level = RAMP(a, rm, rp_dist_level) >> 8, sub_level = RAMP(a, rm, rp_sub_level) >> 8,
                fsub_level = RAMP(a, rm, rp_fsub_level) >> 8, delay_q8 = RAMP(a, rm, rp_delay),
                lead_level = RAMP(a, rm, rp_lead_level) >> 8, dly_rev = RAMP(a, rm, rp_dly_rev) >> 8;
        if (rm && !(a->rp_cut.left | a->rp_res.left | a->rp_master.left | a->rp_rev_send.left |
                    a->rp_rev_level.left | a->rp_dly_send.left | a->rp_dly_level.left | a->rp_dly_fb.left |
                    a->rp_dist_level.left | a->rp_sub_level.left | a->rp_fsub_level.left | a->rp_delay.left |
                    a->rp_lead_level.left | a->rp_dly_rev.left))
            a->ramps_moving = false; /* all arrived: the ramps are read as they stand until the next change */
        if (a->dly_glide_k)
            delay_q8 = delay_glide(a); /* GLIDE: the time a tape's way (the ramp runs on under it) */
        while (1) {
            MelloAudioQueuedEvent q;
            if (!audio_pop_due(a, t, &q)) break;
            apply_event(a, &q.event);
        }
        if (a->lead.env_stage == MELLO_ENV_ATTACK) {
            (void)env_tick(&a->lead.env_q24, a->lead.env_step_q24, a->lead.env_target_q24,
                           &a->lead.env_stage);
            if (a->lead.env_stage == MELLO_ENV_DECAY) lead_begin_decay(a);
        } else if (a->lead.env_stage == MELLO_ENV_DECAY || a->lead.env_stage == MELLO_ENV_RELEASE)
            tone_curve_tick(TONE_ENV(&a->lead));
        if (a->sub.env_stage == MELLO_ENV_ATTACK) {
            MelloEnvStage before = a->sub.env_stage;
            (void)env_tick(&a->sub.env_q24, a->sub.env_step_q24, a->sub.env_target_q24, &a->sub.env_stage);
            if (before == MELLO_ENV_ATTACK && a->sub.env_stage == MELLO_ENV_DECAY) sub_begin_decay(a);
        } else if (a->sub.env_stage == MELLO_ENV_DECAY || a->sub.env_stage == MELLO_ENV_RELEASE)
            sub_curve_tick(a);
        if (a->lead.slide_left)
            voice_slide_tick(&a->lead.inc, a->lead.target_inc, &a->lead.pitch_q16, a->lead.slide_step,
                             &a->lead.slide_left);
        if (a->lead.sub_slide_left) {
            a->lead.sub_inc = (uint32_t)((int32_t)a->lead.sub_inc + a->lead.sub_slide_step);
            if (--a->lead.sub_slide_left == 0u) a->lead.sub_inc = a->lead.sub_target_inc;
        }
        if (a->sub.slide_left)
            voice_slide_tick(&a->sub.inc, a->sub.target_inc, &a->sub.pitch_q16, a->sub.slide_step,
                             &a->sub.slide_left);
        if (a->gains_moving) { /* SUB MUTE and MIX's mutes, ~10 ms either way; nothing to do once they sit */
            uint8_t m;
            bool still = false;
            if (a->sub_mute ? a->sub_gain > 0 : a->sub_gain < 32767)
                a->sub_gain = clamp32(a->sub_gain + (a->sub_mute ? -75 : 75), 0, 32767), still = true;
            for (m = 0u; m < 4u; ++m) {
                bool off = (a->mix_mute >> m) & 1u;
                if (off ? a->mix_gain[m] > 0 : a->mix_gain[m] < 32767)
                    a->mix_gain[m] = clamp32(a->mix_gain[m] + (off ? -75 : 75), 0, 32767), still = true;
            }
            a->gains_moving = still;
        }
        if (a->dot_lane[0].stage || a->dot_lane[1].stage || a->tm_value) {
            /* DOT's MOD lanes and EVOLVE's sound routes, summed per destination in permille of its range. */
            uint8_t l;
            for (l = 0u; l < 2u * MELLO_DOT_MOD_LANES; ++l) dmod[mod_at[l]] = 0;
            for (l = 0u; l < MELLO_DOT_MOD_LANES; ++l) {
                dot_lane_tick(a, &a->dot_lane[l], l);
                dmod[mod_at[2u * l]] += a->dot_lane[l].v >> 16;
                dmod[mod_at[2u * l + 1u]] += shr_tz(a->tm_value * a->modr.tm_depth[l], 15u);
            }
            dmod[MELLO_DEST_OFF] = 0;
            mods = true;
        }
        /* the chords' share (DECISIONS.md "The chords move too"): their CUT and LEVEL, read by chord_coeffs and the
         * mix; a route with a negative depth pulls them the other way from the lead */
        a->chord_mod_cut = (int16_t)(mods ? clamp32(dmod[MELLO_DEST_CHORD_CUT], -1000, 1000) : 0);
        a->chord_mod_level = (int16_t)(mods ? clamp32(dmod[MELLO_DEST_CHORD_LEVEL], -1000, 1000) : 0);
        if (i + 1u == frames) { /* the block's last sample: what the screen shows (F14) */
            uint8_t d;
            for (d = 0u; d < MELLO_DEST_COUNT; ++d)
                a->mod_view[d] = (int16_t)(mods ? clamp32(dmod[d], -1000, 1000) : 0);
            a->mod_view[MELLO_DEST_CUTOFF] = (int16_t)clamp32(a->c_cut_off, -1000, 1000);
            a->mod_view[MELLO_DEST_RES] = (int16_t)clamp32(a->c_res_off, -1000, 1000);
        }
        if (a->menv_stage == 1u) {
            a->menv_q24 += a->menv_step;
            if (a->menv_q24 >= 1 << 24) a->menv_q24 = 1 << 24, a->menv_stage = 2u;
        } else if (a->menv_stage == 2u) {
            a->menv_q24 -= ((a->menv_q24 >> 8) * a->menv_k_q16) >> 8;
            if (a->menv_q24 < 1 << 10) a->menv_q24 = 0, a->menv_stage = 0u;
        }
        mod_env = 0;
        if (a->config.menv_amount) /* +-32767, the note's amount (lead_tracking: VEL > MOD ENV) */
            mod_env = ((a->menv_q24 >> 9) * a->lead.menv_amt_q15) >> 15;
        if (!(a->ctl_phase++ & 3u)) {
            /* The control step, every 4 samples (11 kHz): the LFO, and every offset the modulators put on the
             * cutoff, the resonance, the lead's pitch and pulse width, the grit's amounts, OSC 2's pitch and FM;
             * the rest of the sample reads them ready-made. What moves a sample at a time (the envelopes, the
             * slides, the MOD lanes, the MOD envelope) stays per sample. */
            const MelloSoundConfig *c = &a->config;
            int32_t cut_off = 0, res_off = 0, pwm = c->pwm_permille, q8, noise, nfm, sat, det, fm;
            uint32_t ratio = 65536u;
            lfo = 0;
            if (c->lfo_on) {
                uint32_t ph = a->lfo_phase;
                int32_t s0 = SINE_Q15[(uint8_t)(ph >> 24)], s1 = SINE_Q15[(uint8_t)((ph >> 24) + 1u)];
                a->lfo_phase = ph + a->lfo_inc * 4u;
                lfo = s0 + (((s1 - s0) * (int32_t)((ph >> 16) & 0xFFu)) >> 8);
                a->last_lfo = (int16_t)lfo;
                if (c->lfo_target == MELLO_LFO_FILTER) {
                    /* HTML: f = base * (1 + sin * (0.08 + 0.92 * depth)), linear in Hz: on the log cutoff that is
                     * + 1000 * log200(r), r = 0..2. */
                    int32_t m = (lfo * a->lfo_filter_span_q15) >> 15; /* (r - 1) in Q15 */
                    int32_t r64 = clamp32(64 + (m >> 9), 0, 127), rem = m & 511, lo,
                            hi; /* r in 1/64 + rem/512 */
                    lo = LFO_LOG200_PERMILLE[r64];
                    hi = LFO_LOG200_PERMILLE[r64 + 1];
                    cut_off += lo + (((hi - lo) * rem) >> 9);
                } else if (c->lfo_target == MELLO_LFO_PITCH) {
                    /* HTML: Tone.LFO into synth.detune, +-(80 + 620 * depth) cents (the follower sub is not
                     * detuned). cc is in 1/16 cent; the table steps 25 cents (400). */
                    int32_t cc = (lfo * a->lfo_pitch_cents) >> 11;
                    uint32_t k = (uint32_t)(cc + 700 * 16), idx = k / 400u, rem = k % 400u;
                    if (idx > 55u) idx = 55u, rem = 400u;
                    ratio = LFO_CENTS_RATIO_Q16[idx] +
                            (uint32_t)(((int32_t)(LFO_CENTS_RATIO_Q16[idx + 1u] - LFO_CENTS_RATIO_Q16[idx]) *
                                        (int32_t)rem) /
                                       400);
                } else if (c->lfo_target == MELLO_LFO_PWM) {
                    /* HTML: width + sin * swing, swing = min(0.42 * depth, width - 0.05, 0.95 - width): symmetric */
                    int32_t sw = a->lfo_pwm_swing, pb = clamp32(pwm, 50, 950);
                    if (sw > pb - 50) sw = pb - 50;
                    if (sw > 950 - pb) sw = 950 - pb;
                    pwm = pb + ((lfo * sw) >> 15);
                }
            }
            a->c_lfo = lfo;
            /* HTML: mix.gain = 0.58 + 0.58 * depth * sin, on the whole mix (applied below). */
            a->c_ampmod = c->lfo_on && c->lfo_target == MELLO_LFO_AMP
                              ? 32767 + ((lfo * a->lfo_depth_q15) >> 15)
                              : 32767;
            if (a->tm_filter_mod_q15) {
                int32_t tv = clamp32(a->tm_filter_mod_q15, -8192, 8192);
                uint32_t ti = (uint32_t)(tv + 8192) * 100u / 16384u;
                cut_off += TM_FILTER_OFFSET_PERMILLE[ti > 100u ? 100u : ti];
            }
            if (mods) cut_off += dmod[MELLO_DEST_CUTOFF];
            if (c->vel_cutoff)
                cut_off += a->lead.vel_cut; /* VEL > CUTOFF: up to 30% of the range for an accent */
            if (c->key_track) {
                /* KEY TRACK from C4: 1000 * log200(2) / 12 = 10.9 permille of the cutoff a semitone, at 1000 */
                int32_t semis_q8 = (a->lead.pitch_q16 >> 8) - 60 * 256;
                cut_off += (semis_q8 * a->key_track_q15) >> 15;
            }
            if (mod_env && c->menv_target == MELLO_MENV_CUTOFF) cut_off += (mod_env * 1000) >> 15;
            if (c->lfo_on && c->lfo_target == MELLO_LFO_RES) res_off += (lfo * (int32_t)c->lfo_depth) >> 15;
            if (mods) res_off += dmod[MELLO_DEST_RES];
            if (mod_env && c->menv_target == MELLO_MENV_RES) res_off += (mod_env * 1000) >> 15;
            a->c_cut_off = cut_off;
            a->c_res_off = res_off;
            /* the lead's pitch (a Q16 ratio on its increment) and pulse width */
            if (mod_env && c->menv_target == MELLO_MENV_PITCH) /* +-24 semitones */
                ratio = (uint32_t)(((uint64_t)ratio * semi_ratio_q16((mod_env * 6144) >> 15)) >> 16);
            if (mods && dmod[MELLO_DEST_PITCH]) /* +-12 semitones */
                ratio = (uint32_t)(((uint64_t)ratio *
                                    semi_ratio_q16((clamp32(dmod[MELLO_DEST_PITCH], -1000, 1000) * 3146) >>
                                                   10)) >>
                                   16);
            if (mods && dmod[MELLO_DEST_PWM])
                pwm = clamp32(pwm + ((dmod[MELLO_DEST_PWM] * 461) >> 10), 50, 950);
            a->c_lead_ratio = ratio;
            a->c_pwm = (uint16_t)pwm;
            /* grit and the Virus layer's amounts */
            noise = lfo_around(c->noise_level, lfo, c->lfo_depth, MELLO_LFO_NOISE, a);
            nfm = lfo_around(c->noise_fm, lfo, c->lfo_depth, MELLO_LFO_NOISE_FM, a);
            sat = lfo_around(c->saturation, lfo, c->lfo_depth, MELLO_LFO_SAT, a);
            det = lfo_around(c->detune, lfo, c->lfo_depth, MELLO_LFO_DETUNE, a);
            fm = lfo_around(c->fm_amount, lfo, c->lfo_depth, MELLO_LFO_FM, a);
            if (mod_env && c->menv_target == MELLO_MENV_NOISE) noise += (mod_env * 1000) >> 15;
            if (mod_env && c->menv_target == MELLO_MENV_FM) fm += (mod_env * 1000) >> 15;
            if (mods) {
                noise += dmod[MELLO_DEST_NOISE];
                nfm += dmod[MELLO_DEST_NOISE_FM];
                sat += dmod[MELLO_DEST_SAT];
                det += dmod[MELLO_DEST_DETUNE];
                fm += dmod[MELLO_DEST_FM];
            }
            a->c_noise = clamp32(noise, 0, 1000);
            a->c_nfm = clamp32(nfm, 0, 1000);
            a->c_sat = clamp32(sat, 0, 1000);
            a->c_det = clamp32(det, 0, 1000);
            a->c_fm = clamp32(fm, 0, 1000);
            /* OSC 2's pitch offset */
            q8 = a->osc2_q8_base;
            if (c->lfo_on && c->lfo_target == MELLO_LFO_OSC2)
                q8 += (int32_t)(((int64_t)lfo * c->lfo_depth * 393) >> 22);                  /* +-12 */
            if (mod_env && c->menv_target == MELLO_MENV_OSC2) q8 += (mod_env * 12288) >> 15; /* +-48 */
            if (mods) q8 += (clamp32(dmod[MELLO_DEST_OSC2], -1000, 1000) * 6291) >> 10;      /* +-24 */
            a->c_osc2_q8 = q8;
        }
        lfo = a->c_lfo;
        ampmod = a->c_ampmod;
        if (a->c_cut_off) cut = (uint16_t)clamp32((int32_t)cut + a->c_cut_off, 0, 1000);
        if (a->c_res_off) res = (uint16_t)clamp32((int32_t)res + a->c_res_off, 0, 1000);
        /* (the drums' share, every 16 samples: parts_step) */
        if (a->lead.env_q24 > 0 && a->config.osc_wave != MELLO_WAVE_OFF) {
            uint32_t inc = a->lead.inc;
            uint16_t pwm = a->c_pwm;
            /* the step's own wave (RANDOM's WAVE), else the sound's */
            MelloWaveform w1 = a->lead.wave ? (MelloWaveform)(a->lead.wave - 1u) : a->config.osc_wave;
            if (a->c_lead_ratio != 65536u) inc = (uint32_t)(((uint64_t)inc * a->c_lead_ratio) >> 16);
            {
                /* Grit and the Virus layer. Each part costs nothing while it is off, and the defaults are off:
                 * then this is the HTML's plain saw. */
                const MelloSoundConfig *c = &a->config;
                int32_t noise_amt = a->c_noise, nfm_amt = a->c_nfm, sat = a->c_sat;
                uint8_t uni = c->unison, k;
                int32_t white = 0, dev = 0, o1 = 0;
                uint32_t inc2 = 0u, before = a->lead.phase;
                if (noise_amt || nfm_amt) {
                    a->noise_seed = a->noise_seed * 1664525u + 1013904223u;
                    white = (int32_t)a->noise_seed >> 16; /* +-32768, uniform */
                }
                if (nfm_amt) {
                    /* Noise FM, as the Virus's FM source "noise": lowpassed noise (~900 Hz) moves the frequency
                     * by up to ~70 Hz RMS at full, through zero. An absolute deviation, so that a bass note gets
                     * as rough as a high one; squared, so the low half of the knob is fine-grained. */
                    a->nfm_lp += (white - a->nfm_lp) >> 3;
                    /* 100% is the first version's 15% (feedback 2026-10-30: usable only up to there) */
                    dev = ((a->nfm_lp * (int32_t)(((int64_t)nfm_amt * nfm_amt * 1546) >> 21)) >> 15) * 58000;
                }
                if (uni > 1u) {
                    /* Unison: detune spreads the oscillators up to +-60 cents (squared). Near 1 the ratio is
                     * 1 + c * ln2 / 1200 to within a cent, so no table: 2^-18 per unit, c * 151.4. */
                    int32_t det = a->c_det;
                    int32_t d_q18 = (int32_t)(((int64_t)det * det * 9525) >> 20); /* cents * 151.4 */
                    int32_t step =
                        (int32_t)(((inc >> 14) * (uint32_t)d_q18) >> 4) / 3; /* a third of the spread */
                    int32_t sum = 0;
                    for (k = 0u; k < uni; ++k) {
                        uint32_t vi =
                            (uint32_t)((int32_t)inc + step * UNISON_SPREAD_THIRDS[uni - 1u][k] + dev);
                        uint32_t *ph = k ? &a->lead.uni_phase[k - 1u] : &a->lead.phase;
                        int32_t y;
                        *ph += vi;
                        y = tone_osc_sample(w1, *ph, inc_size(vi), pwm);
                        if (!k) o1 = y;
                        sum += y;
                    }
                    lead = q15mul(clamp32(sum, -65535, 65535), UNISON_GAIN_Q15[uni - 1u]);
                } else {
                    uint32_t vi = inc + (uint32_t)dev;
                    a->lead.phase += vi;
                    lead = o1 = tone_osc_sample(w1, a->lead.phase, inc_size(vi), pwm);
                }
                if (c->osc2_mix && c->osc2_wave != MELLO_WAVE_OFF) {
                    /* OSC 2: SEMI and FINE from OSC 1, hard sync (it restarts where OSC 1 wraps, at the same
                     * fraction of its own cycle) and FM from OSC 1 (linear, through zero, up to 4x its frequency;
                     * squared). The Virus's oscillator pair. */
                    int32_t q8 = a->c_osc2_q8, fm, o2;
                    uint32_t r2, v2;
                    if (q8 != a->lead.osc2_q8_last ||
                        !a->lead.osc2_r2) { /* the ratio, when the offset moves */
                        a->lead.osc2_r2 = semi_ratio_q16(q8);
                        a->lead.osc2_q8_last = q8;
                    }
                    r2 = a->lead.osc2_r2;
                    inc2 = inc_times(inc, r2);
                    fm = a->c_fm;
                    v2 = inc2 + (uint32_t)dev;
                    if (fm) {
                        int64_t d =
                            ((int64_t)inc2 * q15mul(o1, (int32_t)(((int64_t)fm * fm * 34359) >> 20))) >> 13;
                        v2 = (uint32_t)(int32_t)(d > 0x7FFFFFFF ? 0x7FFFFFFF
                                                                : (d < -0x7FFFFFFF ? -0x7FFFFFFF : d)) +
                             v2;
                    }
                    if (c->osc2_sync && (int32_t)(inc + (uint32_t)dev) > 0 && a->lead.phase < before)
                        a->lead.osc2_phase = (uint32_t)(((uint64_t)a->lead.phase * r2) >> 16);
                    else
                        a->lead.osc2_phase += v2;
                    o2 = tone_osc_sample(c->osc2_wave, a->lead.osc2_phase, inc_size(v2), pwm);
                    lead += mul_q15w(o2 - lead, a->osc2_mix_q15);
                }
                if (noise_amt) {
                    /* White noise into the filter, through a low cut (two one-pole highpasses, ~450 Hz, 12 dB an
                     * octave) so it stays out of the low register; linear, at full about half the saw's RMS (the
                     * first version's 30%: feedback 2026-10-30, too much from there). */
                    int32_t hp = white;
                    a->noise_lc[0] += (hp - a->noise_lc[0]) >> 4;
                    hp -= a->noise_lc[0];
                    a->noise_lc[1] += (hp - a->noise_lc[1]) >> 4;
                    hp -= a->noise_lc[1];
                    lead += q15mul(clamp32(hp, -65535, 65535), (noise_amt * 17083) >> 10);
                }
                if (sat) lead = saturate(clamp32(lead, -65535, 65535), sat);
            }
            if (fsub_level) {
                a->lead.sub_phase += a->lead.sub_inc;
                /* HTML: subOsc is triggered at vel * level and then passes Gain(level): level squared (Q12 x Q12
                 * = Q24, to Q15). */
                int32_t lv = fsub_level,
                        fs = q15mul(tone_osc_sample(a->config.follower_sub_wave, a->lead.sub_phase,
                                                    a->lead.sub_inc, 500u),
                                    (lv * lv) >> 9);
                lead += a->sub_gain == 32767 ? fs : q15mul(fs, a->sub_gain); /* SUB MUTE */
            }
            lead = q15mul(clamp32(lead, -65535, 65535), (a->lead.env_q24 >> 8));
            lead = q15mul(lead, (int32_t)a->lead.vel_level * 258);
        }
        if (a->sub.env_q24 > 0 && a->sub_config.on && a->sub_config.wave != MELLO_WAVE_OFF) {
            const ToneSubBiquadCoeff *bc = &SUB_FILTER_BIQUAD_Q28[a->sub_config.cut / 10u];
            int32_t filtered;
            a->sub.phase += a->sub.inc;
            sub = tone_osc_sample(a->sub_config.wave, a->sub.phase, a->sub.inc, 500u);
            sub = q15mul(sub, a->sub.env_q24 >> 8);
            sub = q15mul(sub, (int32_t)a->sub.velocity * 258);
            if (a->bass_config.res | a->bass_config.env)
                filtered = sub_svf(a, sub);
            else { /* HTML: Tone.Filter rolloff=-24 => two cascaded biquads, both Q=.8. */
                filtered = sub_biquad_step(sub, bc, &a->sub.biquad1_x1, &a->sub.biquad1_x2,
                                           &a->sub.biquad1_y1, &a->sub.biquad1_y2);
                filtered = sub_biquad_step(filtered, bc, &a->sub.biquad2_x1, &a->sub.biquad2_x2,
                                           &a->sub.biquad2_y1, &a->sub.biquad2_y2);
            }
            if (a->sub.accent && a->bass_config.accent) /* ACCENT: up to half again as loud */
                filtered += (int32_t)(((int64_t)filtered * a->bass_config.accent) >> 11);
            if (a->sub_config.drive) {
                /* Lightweight 2x approximation around Tone.WaveShaper oversample=2x. */
                int32_t mid = a->sub.drive_prev + ((filtered - a->sub.drive_prev) >> 1);
                filtered = (sub_tone_distort(a, mid) + sub_tone_distort(a, filtered)) / 2;
            }
            a->sub.drive_prev = filtered;
            sub = mul_q12(filtered, sub_level);
            if (a->sub_gain != 32767) sub = q15mul(clamp32(sub, -65535, 65535), a->sub_gain); /* SUB MUTE */
        }
        if (parts_on) parts_at(a, i - parts_at0); /* this sample's chords and drums (parts_step) */
#if MELLO_FEATURE_CHORD_MACHINE
        if (a->parts_job.chords && parts_on) { /* silent ones are 0, and all below keeps them 0 */
            chord_l = a->chord_out[0][i - parts_at0];
            chord_r = a->chord_out[1][i - parts_at0];
        }
        if (chord_l | chord_r) {
            int32_t duck = 32767 - (((a->lead.env_q24 >> 8) * 3) >> 4);
            /* Automatic ~19% duck at full lead envelope leaves the melodic transient in front. */
            if (duck < 24500) duck = 24500;
            {
                int32_t lv = a->chord_level_q12; /* C.LEVEL: the routes scale the level, 0 .. twice */
                if (a->chord_mod_level) lv = lv * (1000 + (int32_t)a->chord_mod_level) / 1000;
                chord_l = mul_q12(q15mul(chord_l, duck), lv);
                chord_r = mul_q12(q15mul(chord_r, duck), lv);
            }
            if (a->mix_gain[2] != 32767) /* MIX's CHORD mute */
                chord_l = q15mul(clamp32(chord_l, -65535, 65535), a->mix_gain[2]),
                chord_r = q15mul(clamp32(chord_r, -65535, 65535), a->mix_gain[2]);
            chord_mix = (chord_l + chord_r) / 2;
        }
#endif
        {
            uint8_t sv;
            for (sv = 0u; sv < MELLO_SAMPLE_SLOT_COUNT; ++sv)
                if (a->sample_voice[sv].active) sample_mix += sample_render_voice(a, &a->sample_voice[sv]);
        }
        if (lead_level < 4096) lead = mul_q12(lead, lead_level); /* MIX's LEAD */
        if (a->mix_gain[0] != 32767)
            lead = q15mul(clamp32(lead, -65535, 65535), a->mix_gain[0]);                        /* its mute */
        if (a->mix_gain[1] != 32767) sub = q15mul(clamp32(sub, -65535, 65535), a->mix_gain[1]); /* SUB's */
        mix = mul_q12(lead + sample_mix, 2376);                                                 /* 0.58 */
        /* The bass leaves the lead's filter and distortion (ROADMAP E11): it has its own filter and DRIVE; at the
         * level it reached the effects with before (the lead's 0.58, the distortion's x 1/3 at AMT 0, its LEVEL) */
        bass = mul_q12(sub, BASS_BUS_Q12);
        if (ampmod != 32767) { /* the LFO's AMP: the whole mix, as the HTML's */
            mix = q15mul_wide(clamp32(mix, -65535, 65535), ampmod); /* reaches 65534: x2 */
            bass = q15mul_wide(clamp32(bass, -65535, 65535), ampmod);
        }
        {
            /* HTML: Tone.Filter lowpass -24 = two identical biquads, Q = res*15 dB, f = 80*200^cut. */
            int32_t y;
            main_filter_coeffs(a, cut, res);
            y = main_biquad_step(a->mf_coeff, a->mf_state[0], clamp32(mix, -65535, 65535));
            y = main_biquad_step(a->mf_coeff, a->mf_state[1], y);
            mix = clamp32(y, -65535, 65535);
        }
        if (a->duck_env) { /* DUCK (the kick's sidechain): the music dips at each kick, back over ~160 ms */
            int32_t g = 32767 - (int32_t)(((int64_t)(a->duck_env >> 9) * a->drum_config.duck) / 1000);
            a->duck_env -= (int32_t)(((int64_t)a->duck_env * 16406) >> 24);
            if (a->duck_env < 4096) a->duck_env = 0;
            if (a->drum_config.duck) {
                mix = q15mul(mix, g);
                bass = q15mul(bass, g);
#if MELLO_FEATURE_CHORD_MACHINE
                chord_l = q15mul(chord_l, g);
                chord_r = q15mul(chord_r, g);
#endif
            }
        }
        /* DRUMS: past the lead's filter and distortion, their own LEVEL and SPACE */
        if (a->drum_config.on) { /* every sample while ON: STUTTER keeps listening */
            drums_l = a->drum_out[0][i - parts_at0];
            drums_r = a->drum_out[1][i - parts_at0];
            parts_heard(a, i - parts_at0, mix); /* STUTTER's memory: the music, as it is now, and the drums */
            drums_l = mul_q12(drums_l, a->drum_level_q12); /* LEVEL 70%: a little over the lead */
            drums_r = mul_q12(drums_r, a->drum_level_q12);
            if (a->mix_gain[3] != 32767) /* MIX's PERC mute */
                drums_l = q15mul(clamp32(drums_l, -65535, 65535), a->mix_gain[3]),
                drums_r = q15mul(clamp32(drums_r, -65535, 65535), a->mix_gain[3]);
            drums = (drums_l + drums_r) / 2; /* what they send */
        }
        if ((a->perf_fx & 12u) ||
            a->throw_q12 | a->wash_q12) { /* THROW and WASH slide in and out over ~10 ms */
            a->throw_q12 = clamp32(a->throw_q12 + ((a->perf_fx & MELLO_PERF_FX_THROW) ? 10 : -10), 0, 4096);
            a->wash_q12 = clamp32(a->wash_q12 + ((a->perf_fx & MELLO_PERF_FX_WASH) ? 10 : -10), 0, 4096);
            a->perf_tail = 3u * MELLO_AUDIO_FS; /* their tails ring on after: 3 s */
        } else if (a->perf_tail)
            a->perf_tail--;
        if (a->fx.delay_pre_dist) {
            /* PRE DIST: the delay before the distortion, so its filtered echoes are driven too (the HTML has it
             * after); what the chords and drums send to it goes there with them. */
            int32_t in = mul_q12(mix, a->send_dly_q12[MELLO_SEND_LEAD]) +
                         mul_q12(bass, a->send_dly_q12[MELLO_SEND_BASS]) +
                         mul_q12(drums, a->send_dly_q12[MELLO_SEND_DRUMS]);
#if MELLO_FEATURE_CHORD_MACHINE
            in += mul_q12(chord_mix, a->send_dly_q12[MELLO_SEND_CHORD]);
#endif
            int32_t dr, dl;
            in = mul_q12(in, dly_send);
            if (a->throw_q12) in += mul_q12(mix + bass + drums, a->throw_q12) / 2; /* THROW */
            dl = delay_tick(a, in, perf_fb(a, dly_fb), delay_q8, &dr);
            /* the distortion is one channel: PING's two taps go in together, its echoes in the middle */
            dly_heard = mul_q12(a->fx.delay_pingpong ? dl + dr : dl, dly_level);
            mix += dly_heard;
        }
        {
            /* HTML: Tone.Distortion is always in the path (wet = distMix, 1 by default). At AMT 0 its curve is
             * y = x/3 with a hard clip at +-1: the HTML's gain staging. 2x oversampling as for the sub: it aliases
             * about as much as the HTML's 4x (drive_high, -34.5 vs -33.8 dB). */
            int32_t x = clamp32(mix, -32767, 32767), wet, mid;
            mid = a->main_dist_prev + ((x - a->main_dist_prev) >> 1);
            wet = (tone_shape(a->main_dist_curve, mid) + tone_shape(a->main_dist_curve, x)) / 2;
            a->main_dist_prev = x;
            if (a->fx.dist_mix < 1000u) {
                /* Tone's dry/wet CrossFade is equal power: dry cos, wet sin of mix * 90 degrees. */
                uint32_t q = ((uint32_t)a->fx.dist_mix * 64u + 500u) / 1000u;
                mix = q15mul(mix, SINE_Q15[64u - q]) + q15mul(wet, SINE_Q15[q]);
            } else
                mix = wet;
            mix = tone_lp24(&DIST_TONE_BIQUAD_Q28[a->fx.dist_tone / 10u], a->dist_tone_s, mix);
            mix = mul_q12(mix, dist_level);
        }
#if MELLO_FEATURE_CHORD_MACHINE
#endif
        {
            /* MIX's sends (DECISIONS "A send per part"): each part's share to the delay and to the reverb, then the
             * effect's WET. The chords and drums bypass the lead's distortion, their width their own (added after
             * the delay, below). */
            int32_t fx_all,
                fx_dly = mul_q12(mix, a->send_dly_q12[MELLO_SEND_LEAD]) +
                         mul_q12(bass, a->send_dly_q12[MELLO_SEND_BASS]) +
                         mul_q12(drums, a->send_dly_q12[MELLO_SEND_DRUMS]),
                fx_rev = mul_q12(mix, a->send_rev_q12[MELLO_SEND_LEAD]) +
                         mul_q12(bass, a->send_rev_q12[MELLO_SEND_BASS]) +
                         mul_q12(drums, a->send_rev_q12[MELLO_SEND_DRUMS]);
#if MELLO_FEATURE_CHORD_MACHINE
            fx_dly += mul_q12(chord_mix, a->send_dly_q12[MELLO_SEND_CHORD]);
            fx_rev += mul_q12(chord_mix, a->send_rev_q12[MELLO_SEND_CHORD]);
#endif
            fx_all = mix + bass + drums;
#if MELLO_FEATURE_CHORD_MACHINE
            fx_all += chord_mix;
#endif
            mix = mul_q12(mix + bass, 3441); /* 0.84 */
            mix_l = mix;
            mix_r = mix;
            if (!a->fx.delay_pre_dist) {
                int32_t dr, dl, in = mul_q12(fx_dly, dly_send);
                if (a->throw_q12) in += mul_q12(fx_all, a->throw_q12) / 2; /* THROW: everything, at half */
                dl = delay_tick(a, in, perf_fb(a, dly_fb), delay_q8, &dr);
                dl = mul_q12(dl, dly_level);
                mix_l += dl;
                dr = a->fx.delay_pingpong ? mul_q12(dr, dly_level) : dl; /* without PING the same */
                mix_r += dr;
                dly_heard = a->fx.delay_pingpong ? dl + dr : dl; /* (PING: each echo once, left or right) */
            }
            /* REVERB (FX > DELAY ROUTE): the delay as it is heard, with the parts' sends into the reverb (with PRE
             * DIST the lead's send carries it too, in the lead) */
            if (dly_rev) fx_rev += mul_q12(dly_heard, dly_rev);
#if MELLO_FEATURE_CHORD_MACHINE
            mix_l += mul_q12(chord_l, 2753); /* 0.672: 800 x 840 as the lead's dry */
            mix_r += mul_q12(chord_r, 2753);
#endif
            mix_l += mul_q12(drums_l, 3441);
            mix_r += mul_q12(drums_r, 3441);
            if (a->reverb_buffer && a->reverb_capacity > 1024u &&
                ((parts_on && a->parts_job.reverb) || a->fx.reverb_wet || a->reverb_live || a->wash_q12)) {
                int32_t send =
                            mul_q12(fx_rev, rev_send) + (a->wash_q12 ? mul_q12(fx_all, a->wash_q12) / 2 : 0),
                        rv_l = 0, rv_r = 0, hp_l, hp_r; /* WASH: everything, at half */
                reverb_in(a, send);
                if (parts_on && a->parts_job.reverb) { /* its network and return: the parts' chunk */
                    hp_l = a->rev_out[0][i - parts_at0];
                    hp_r = a->rev_out[1][i - parts_at0];
                } else {
                    reverb_out(a, &rv_l, &rv_r);
                    reverb_return(a, rv_l, rv_r, &hp_l, &hp_r);
                }
                mix_l += mul_q12(hp_l, rev_level); /* reverbReturnGain's 1050/1000 is in it */
                mix_r += mul_q12(hp_r, rev_level);
                /* Its own tail: with WET at 0 the network runs on only until what it holds has died away (the
                 * delay's tail guard used to keep it running for nothing). */
                if (send > 8 || send < -8)
                    a->reverb_live = ms_samples(a->fx.reverb_decay_ms) + 2048u;
                else if (a->reverb_live &&
                         !--a->reverb_live) { /* the tail gone: the diffusers where SIZE put them */
                    uint8_t k;
                    for (k = 0u; k < 3u; ++k) /* (split, they are already: parts_reverb) */
                        if (a->reverb_ap_now[k] != a->reverb_ap_delay[k])
                            a->reverb_ap_now[k] = a->reverb_ap_delay[k];
                }
            }
            if (a->perf_fx & 3u || a->sweep_pos[0] | a->sweep_pos[1])
                sweep_step(a, &mix_l, &mix_r); /* the sweeps */
            if (a->pfx | a->pfx_was || a->pb_mode || a->chop_g || a->pluck_q24 ||
                a->chop_mk | a->pluck_mk_now)
                perf2_step(a, &mix_l, &mix_r); /* PERFORM's black keys */
            /* MASTER and the output's gain, then the limiter: nothing leaves above -1 dBFS but a 3 ms attack's edge */
            mix_l = mul_q12(mul_q12(mix_l, master), MELLO_OUTPUT_GAIN_Q12);
            mix_r = mul_q12(mul_q12(mix_r, master), MELLO_OUTPUT_GAIN_Q12);
            if (MELLO_OUTPUT_STAGE) comp_step(a, &mix_l, &mix_r);
            limiter_step(a, &mix_l, &mix_r);
            mix_l = soft_top(mix_l);
            mix_r = soft_top(mix_r);
        }
        if ((mix_l > TAIL_LSB || mix_l < -TAIL_LSB || mix_r > TAIL_LSB || mix_r < -TAIL_LSB) &&
            (a->fx.delay_wet || a->fx.reverb_wet || a->perf_tail)) {
            uint32_t guard = a->perf_tail;
            if (a->fx.delay_wet) guard = a->delay_samples * (a->fx.delay_pingpong ? 2u : 1u) + 512u;
            if (a->fx.reverb_wet) {
                uint32_t rg = ms_samples(a->fx.reverb_decay_ms);
                if (rg > guard) guard = rg;
            }
            a->tail_guard_frames = guard;
        } else if (a->tail_guard_frames)
            a->tail_guard_frames--;
        out[i * 2u] += mix_l;
        out[i * 2u + 1u] += mix_r;
        {
            int32_t pk_l = mix_l < 0 ? -mix_l : mix_l, pk_r = mix_r < 0 ? -mix_r : mix_r,
                    pk = pk_l > pk_r ? pk_l : pk_r;
            if (pk > a->peak) a->peak = (int16_t)pk;
        }
        t += AUDIO_US_BASE;
        terr += AUDIO_US_REM;
        if (terr >= MELLO_AUDIO_FS) {
            terr -= MELLO_AUDIO_FS;
            t++;
        }
        a->rendered_frames++;
    }
    parts_finish(a); /* (the chord and drum state is the render's again) */
    {
        bool samples_silent = true;
        uint8_t sv;
        for (sv = 0u; sv < MELLO_SAMPLE_SLOT_COUNT; ++sv)
            if (a->sample_voice[sv].active && a->sample_voice[sv].env_stage != MELLO_ENV_OFF &&
                a->sample_voice[sv].env_q24 > 0) {
                samples_silent = false;
                break;
            }
        a->silent = (a->lead.env_stage == MELLO_ENV_OFF || a->lead.env_q24 <= 0) &&
                    (a->sub.env_stage == MELLO_ENV_OFF || a->sub.env_q24 <= 0)
#if MELLO_FEATURE_CHORD_MACHINE
                    && (a->chord.env_stage == MELLO_ENV_OFF || a->chord.env_q24 <= 0)
#endif
                    && !(a->drum[0].on | a->drum[1].on | a->drum[2].on | a->drum[3].on) && samples_silent &&
                    a->tail_guard_frames == 0u;
    }
}
