#include "mello_audio.h"

#define Q15_ONE                    32767
#define Q24_ONE                    (32767 << 8)
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
    return clamp32((int32_t)base + lfo * (int32_t)depth / 32767, 0, 1000);
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
static void sub_attack(MelloAudio *a, uint8_t note, uint8_t vel)
{
    uint32_t attack_n;
    a->sub.note = note;
    a->sub.velocity = vel;
    a->sub.gate = true;
    voice_glide_to(&a->sub.inc, &a->sub.target_inc, &a->sub.pitch_q16, &a->sub.slide_step, &a->sub.slide_left,
                   note, a->sub_config.glide_ms, a->sub.env_q24);
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
    if (c->dist) y = soft_drive(y, c->dist, 500u);
    y = y * (int32_t)c->level / 1000;
    v->pos_q16 += v->step_q16;
    return y;
}

#if MELLO_FEATURE_CHORD_MACHINE
static void chord_release(MelloAudio *a)
{
    if (!a) return;
    a->chord.env_stage = MELLO_ENV_RELEASE;
    env_set_segment(&a->chord.env_step_q24, &a->chord.env_target_q24, a->chord.env_q24, 0,
                    ms_samples(a->chord_config.release_ms));
}
static void chord_note_on(MelloAudio *a, uint8_t note)
{
    uint8_t i, free_slot = 0xFFu;
    if (!a || !a->chord_config.on) return;
    for (i = 0u; i < MELLO_CHORD_MAX_VOICES; ++i) {
        if (a->chord.active[i] && a->chord.note[i] == note) return;
        if (!a->chord.active[i] && free_slot == 0xFFu) free_slot = i;
    }
    if (free_slot == 0xFFu) free_slot = 0u;
    a->chord.note[free_slot] = note;
    a->chord.inc[free_slot] = note_inc(note);
    a->chord.active[free_slot] = 1u;
    a->chord.active_count = 0u;
    for (i = 0u; i < MELLO_CHORD_MAX_VOICES; ++i)
        if (a->chord.active[i]) a->chord.active_count++;
    if (a->chord.env_stage == MELLO_ENV_OFF || a->chord.env_stage == MELLO_ENV_RELEASE) {
        a->chord.env_stage = MELLO_ENV_ATTACK;
        env_set_segment(&a->chord.env_step_q24, &a->chord.env_target_q24, a->chord.env_q24, Q24_ONE,
                        ms_samples(a->chord_config.attack_ms));
    }
}
static void chord_note_off(MelloAudio *a, uint8_t note)
{
    uint8_t i;
    if (!a) return;
    for (i = 0u; i < MELLO_CHORD_MAX_VOICES; ++i)
        if (a->chord.active[i] && a->chord.note[i] == note) {
            a->chord.active[i] = 0u;
            break;
        }
    a->chord.active_count = 0u;
    for (i = 0u; i < MELLO_CHORD_MAX_VOICES; ++i)
        if (a->chord.active[i]) a->chord.active_count++;
    if (!a->chord.active_count) chord_release(a);
}
static void chord_kill(MelloAudio *a)
{
    uint8_t i;
    if (!a) return;
    for (i = 0u; i < MELLO_CHORD_MAX_VOICES; ++i) a->chord.active[i] = 0u;
    a->chord.active_count = 0u;
    a->chord.env_q24 = 0;
    a->chord.env_stage = MELLO_ENV_OFF;
    a->chord.tone_lp = 0;
    a->chord.hp_x1 = 0;
    a->chord.hp_y1 = 0;
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
        for (l = 0u; l < MELLO_DOT_MOD_LANES; ++l) {
            /* DOT's MOD lanes. HIT: a lit cell starts an attack (from where the lane is, so hits in a row do not
             * click) and then the release. GATE: lit cells in a row hold, an unlit one releases. (The HTML's filter
             * dot also snapped to 0 on every unlit pulse.) */
            MelloDotLane *ln = &a->dot_lane[l];
            uint16_t lv = e->dot_mod[l];
            bool gate = a->modr.dot_gate[l];
            if (lv == MELLO_DOT_NOT_READ) continue;
            if (lv && (!gate || ln->stage == 0u || ln->stage == 3u || ln->peak != (int32_t)lv << 16))
                dot_lane_attack(a, ln, l, (int32_t)lv << 16);
            else if (!lv && gate && ln->stage && ln->stage != 3u)
                dot_lane_release(a, ln, l);
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
        lead_attack(a, e->note, e->velocity, e->slide);
    } else if (e->type == MELLO_EVENT_NOTE_OFF) {
        if (event_tag(e) == a->lead.on_tag) lead_release(a);
    } else if (e->type == MELLO_EVENT_SUB_NOTE_ON) {
        a->sub.on_tag = event_tag(e);
        sub_attack(a, e->note, e->velocity);
    } else if (e->type == MELLO_EVENT_SUB_NOTE_OFF) {
        if (event_tag(e) == a->sub.on_tag) sub_release(a);
    }
#if MELLO_FEATURE_CHORD_MACHINE
    else if (e->type == MELLO_EVENT_CHORD_NOTE_ON)
        chord_note_on(a, e->note);
    else if (e->type == MELLO_EVENT_CHORD_NOTE_OFF)
        chord_note_off(a, e->note);
#endif
    else if (e->type == MELLO_EVENT_SAMPLE_NOTE_ON)
        sample_attack_voice(a, e);
    else if (e->type == MELLO_EVENT_SAMPLE_NOTE_OFF)
        sample_release_voice(a, e->sample_slot);
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

/* Tone.Reverb's RT60 in ms for DECAY = i * 100 ms: its noise IR fades with exponentialApproachValueAtTime, tau =
 * ln(decay+1)/ln 200, so RT60 = 6.908 tau. 2.54 s at DECAY 6 s. */
static const uint16_t TONE_REVERB_RT60_MS[601] = {
    0,    124,  238,  342,  439,  529,  613,  692,  766,  837,  904,  967,  1028, 1086, 1141, 1195, 1246,
    1295, 1342, 1388, 1432, 1475, 1516, 1557, 1596, 1633, 1670, 1706, 1741, 1774, 1807, 1840, 1871, 1902,
    1932, 1961, 1990, 2018, 2045, 2072, 2098, 2124, 2149, 2174, 2199, 2223, 2246, 2269, 2292, 2314, 2336,
    2358, 2379, 2400, 2420, 2440, 2460, 2480, 2499, 2518, 2537, 2556, 2574, 2592, 2609, 2627, 2644, 2661,
    2678, 2695, 2711, 2727, 2743, 2759, 2775, 2790, 2805, 2820, 2835, 2850, 2865, 2879, 2893, 2907, 2921,
    2935, 2949, 2962, 2976, 2989, 3002, 3015, 3028, 3041, 3053, 3066, 3078, 3090, 3102, 3114, 3126, 3138,
    3150, 3161, 3173, 3184, 3196, 3207, 3218, 3229, 3240, 3251, 3261, 3272, 3282, 3293, 3303, 3314, 3324,
    3334, 3344, 3354, 3364, 3374, 3384, 3393, 3403, 3412, 3422, 3431, 3441, 3450, 3459, 3468, 3477, 3486,
    3495, 3504, 3513, 3522, 3531, 3539, 3548, 3556, 3565, 3573, 3582, 3590, 3598, 3607, 3615, 3623, 3631,
    3639, 3647, 3655, 3663, 3671, 3678, 3686, 3694, 3701, 3709, 3717, 3724, 3732, 3739, 3746, 3754, 3761,
    3768, 3776, 3783, 3790, 3797, 3804, 3811, 3818, 3825, 3832, 3839, 3846, 3853, 3859, 3866, 3873, 3879,
    3886, 3893, 3899, 3906, 3912, 3919, 3925, 3932, 3938, 3944, 3951, 3957, 3963, 3969, 3976, 3982, 3988,
    3994, 4000, 4006, 4012, 4018, 4024, 4030, 4036, 4042, 4048, 4053, 4059, 4065, 4071, 4077, 4082, 4088,
    4094, 4099, 4105, 4110, 4116, 4122, 4127, 4133, 4138, 4143, 4149, 4154, 4160, 4165, 4170, 4176, 4181,
    4186, 4191, 4197, 4202, 4207, 4212, 4217, 4222, 4228, 4233, 4238, 4243, 4248, 4253, 4258, 4263, 4268,
    4273, 4278, 4282, 4287, 4292, 4297, 4302, 4307, 4311, 4316, 4321, 4326, 4330, 4335, 4340, 4344, 4349,
    4354, 4358, 4363, 4367, 4372, 4377, 4381, 4386, 4390, 4395, 4399, 4404, 4408, 4412, 4417, 4421, 4426,
    4430, 4434, 4439, 4443, 4447, 4452, 4456, 4460, 4464, 4469, 4473, 4477, 4481, 4485, 4490, 4494, 4498,
    4502, 4506, 4510, 4514, 4519, 4523, 4527, 4531, 4535, 4539, 4543, 4547, 4551, 4555, 4559, 4563, 4566,
    4570, 4574, 4578, 4582, 4586, 4590, 4594, 4598, 4601, 4605, 4609, 4613, 4617, 4620, 4624, 4628, 4632,
    4635, 4639, 4643, 4646, 4650, 4654, 4657, 4661, 4665, 4668, 4672, 4676, 4679, 4683, 4686, 4690, 4694,
    4697, 4701, 4704, 4708, 4711, 4715, 4718, 4722, 4725, 4729, 4732, 4736, 4739, 4743, 4746, 4749, 4753,
    4756, 4760, 4763, 4766, 4770, 4773, 4776, 4780, 4783, 4786, 4790, 4793, 4796, 4800, 4803, 4806, 4809,
    4813, 4816, 4819, 4822, 4826, 4829, 4832, 4835, 4838, 4842, 4845, 4848, 4851, 4854, 4857, 4861, 4864,
    4867, 4870, 4873, 4876, 4879, 4882, 4885, 4888, 4892, 4895, 4898, 4901, 4904, 4907, 4910, 4913, 4916,
    4919, 4922, 4925, 4928, 4931, 4934, 4937, 4940, 4943, 4945, 4948, 4951, 4954, 4957, 4960, 4963, 4966,
    4969, 4972, 4975, 4977, 4980, 4983, 4986, 4989, 4992, 4994, 4997, 5000, 5003, 5006, 5009, 5011, 5014,
    5017, 5020, 5022, 5025, 5028, 5031, 5033, 5036, 5039, 5042, 5044, 5047, 5050, 5053, 5055, 5058, 5061,
    5063, 5066, 5069, 5071, 5074, 5077, 5079, 5082, 5085, 5087, 5090, 5093, 5095, 5098, 5100, 5103, 5106,
    5108, 5111, 5113, 5116, 5118, 5121, 5124, 5126, 5129, 5131, 5134, 5136, 5139, 5141, 5144, 5146, 5149,
    5151, 5154, 5156, 5159, 5161, 5164, 5166, 5169, 5171, 5174, 5176, 5179, 5181, 5184, 5186, 5189, 5191,
    5193, 5196, 5198, 5201, 5203, 5206, 5208, 5210, 5213, 5215, 5217, 5220, 5222, 5225, 5227, 5229, 5232,
    5234, 5236, 5239, 5241, 5243, 5246, 5248, 5250, 5253, 5255, 5257, 5260, 5262, 5264, 5267, 5269, 5271,
    5273, 5276, 5278, 5280, 5283, 5285, 5287, 5289, 5292, 5294, 5296, 5298, 5301, 5303, 5305, 5307, 5309,
    5312, 5314, 5316, 5318, 5321, 5323, 5325, 5327, 5329, 5332, 5334, 5336, 5338, 5340, 5342, 5345, 5347,
    5349, 5351, 5353, 5355, 5357, 5360,
};
/* Per-line gain for a 60 dB decay in Tone.Reverb's RT60: g = exp(-6.908 L / (fs RT60)), as a Pade approximant. */
static uint16_t reverb_feedback_q15(uint32_t loop_samples, uint16_t decay_ms)
{
    uint32_t raw = decay_ms < 400u ? 400u : decay_ms > 60000u ? 60000u : decay_ms;
    uint32_t i = raw / 100u, f = raw % 100u;
    uint32_t d = TONE_REVERB_RT60_MS[i] +
                 (i < 600u ? ((TONE_REVERB_RT60_MS[i + 1u] - TONE_REVERB_RT60_MS[i]) * f) / 100u : 0u);
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
    if (off > a->reverb_capacity) {
        a->reverb_pre_cap = 0u;
        return;
    }

    pre = ms_samples(a->fx.reverb_pre_ms) / RV3_RATE_DIV;
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
    a->reverb_feedback_q15 = (uint16_t)(sum / 8u);
    a->reverb_delay_a = a->reverb_line_delay[0];
    a->reverb_delay_b = a->reverb_line_delay[7];
}
static int32_t rv3_allpass(MelloAudio *a, uint16_t which, int32_t x, uint16_t g)
{
    uint16_t cap, delay, pos, rp, off;
    int32_t z, y, w;
    cap = a->reverb_ap_cap[which];
    delay = a->reverb_ap_delay[which];
    pos = a->reverb_ap_pos[which];
    off = a->reverb_ap_offset[which];
    if (cap < 4u || delay < 1u) return x;
    rp = pos >= delay ? (uint16_t)(pos - delay) : (uint16_t)(pos + cap - delay);
    z = a->reverb_buffer[(uint32_t)off + rp];
    y = z - q15mul(x, g);
    y = clamp32(y, -32767, 32767);
    w = x + q15mul(y, g);
    a->reverb_buffer[(uint32_t)off + pos] = sat16(w);
    pos++;
    if (pos >= cap) pos = 0u;
    a->reverb_ap_pos[which] = pos;
    return y;
}
static void reverb_network_step(MelloAudio *a, int32_t send, int32_t *left, int32_t *right)
{
    static const int8_t inject_sign[8] = {1, 1, -1, -1, 1, -1, 1, -1};
    int32_t x, y[8], d[8], h[8], w, sum = 0, inj, rv_l, rv_r;
    uint16_t i, pos, rp, cap, off;
    uint32_t p;
    if (left) *left = 0;
    if (right) *right = 0;
    if (!a || !a->reverb_buffer || a->reverb_pre_cap < 2u) return;

    p = a->reverb_pos;
    if (a->reverb_pre_samples) {
        uint32_t pr = p >= a->reverb_pre_samples ? p - a->reverb_pre_samples
                                                 : p + a->reverb_pre_cap - a->reverb_pre_samples;
        x = a->reverb_buffer[pr];
    } else
        x = send;
    a->reverb_buffer[p] = sat16(send);
    p++;
    if (p >= a->reverb_pre_cap) p = 0u;
    a->reverb_pos = p;

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
            dq8 = (uint32_t)((int32_t)((uint32_t)a->reverb_line_delay[i] << 8) +
                             ((sn * RV3_MOD_DEPTH_Q8) >> 15)) -
                  128u;
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
        y0 = a->reverb_buffer[(uint32_t)off + rp];
        y1 = a->reverb_buffer[(uint32_t)off + rp1];
        /* First-order allpass interpolation, (eta + z^-1) / (1 + eta z^-1): flat in level, where linear
         * interpolation would dull the tail. */
        y[i] = clamp32(q15mul(eta, y0) + y1 - q15mul(eta, a->reverb_mod_y[i]), -32767, 32767);
        a->reverb_mod_y[i] = y[i];
        d[i] = y[i];
        sum += d[i];
    }

    /* 8x8 Householder reflection H = I - 2/N * 11^T.  It is orthogonal but
     * costs one sum and one subtraction per line instead of a dense matrix. */
    for (i = 0u; i < 8u; ++i) h[i] = clamp32(d[i] - sum / 4, -32767, 32767);

    inj = q15mul(x, 11585); /* ~1/sqrt(8): energy-controlled injection. */
    for (i = 0u; i < 8u; ++i) {
        w = (inject_sign[i] > 0 ? inj : -inj) + q15mul(h[i], a->reverb_line_feedback_q15[i]);
        pos = a->reverb_line_pos[i];
        off = a->reverb_line_offset[i];
        a->reverb_buffer[(uint32_t)off + pos] = sat16(w);
        pos++;
        if (pos >= a->reverb_line_cap[i]) pos = 0u;
        a->reverb_line_pos[i] = pos;
    }

    /* Two orthogonal sign rows expose a real stereo late field from shared FDN state. */
    rv_l = (y[0] + y[1] + y[2] + y[3] - y[4] - y[5] - y[6] - y[7]) / 4;
    rv_r = (y[0] - y[1] + y[2] - y[3] + y[4] - y[5] + y[6] - y[7]) / 4;
    if (left) *left = clamp32(rv_l, -32767, 32767);
    if (right) *right = clamp32(rv_r, -32767, 32767);
}
/* One audio-rate sample of the reverb, the network at half the rate: the input averaged over two samples, the output
 * interpolated back. */
static void reverb_step(MelloAudio *a, int32_t send, int32_t *left, int32_t *right)
{
    a->reverb_in_acc += send;
    if (a->reverb_odd) {
        a->reverb_prev_l = a->reverb_cur_l;
        a->reverb_prev_r = a->reverb_cur_r;
        reverb_network_step(a, a->reverb_in_acc / 2, &a->reverb_cur_l, &a->reverb_cur_r);
        a->reverb_in_acc = 0;
        *left = (a->reverb_prev_l + a->reverb_cur_l) / 2;
        *right = (a->reverb_prev_r + a->reverb_cur_r) / 2;
    } else {
        *left = a->reverb_cur_l;
        *right = a->reverb_cur_r;
    }
    a->reverb_odd = !a->reverb_odd;
}

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
#endif

/* Tone.Limiter(-1): a compressor at -1 dBFS (29204), ratio 20, attack 3 ms, release 10 ms, on the stereo peak. Above
 * the threshold the gain is threshold / envelope (ratio 20 is within 0.3 dB of that at 6 dB over). Unity below it,
 * so it only acts when the returns pile up; it replaces a hard clip at -1 dBFS. */
#define LIMITER_THRESHOLD   29204
#define LIMITER_ATTACK_Q15  247 /* 1 - exp(-1 / (0.003 * 44100)) */
#define LIMITER_RELEASE_Q15 74  /* 1 - exp(-1 / (0.010 * 44100)) */
static void limiter_step(MelloAudio *a, int32_t *l, int32_t *r)
{
    int32_t al = *l < 0 ? -*l : *l, ar = *r < 0 ? -*r : *r, pk = (al > ar ? al : ar), d, g;
    if (pk > 262143) pk = 262143;
    d = (pk << 4) - a->limiter_env_q4; /* Q4: a 1/16 LSB floor, and (d * coeff) fits 32 bits */
    a->limiter_env_q4 += (d * (d > 0 ? LIMITER_ATTACK_Q15 : LIMITER_RELEASE_Q15)) >> 15;
    if (a->limiter_env_q4 <= (LIMITER_THRESHOLD << 4)) return;
    g = (int32_t)(((uint32_t)LIMITER_THRESHOLD << 15) / (uint32_t)(a->limiter_env_q4 >> 4));
    *l = (int32_t)(((int64_t)*l * g) >> 15);
    *r = (int32_t)(((int64_t)*r * g) >> 15);
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
    bool sub_drive_changed, lfo_rate_changed, delay_changed, reverb_changed, dist_changed, reverb_wet_changed;
    if (!a) return;
    g = a->config_generation;
    /* Odd means main-loop publication is in progress. Keep the last complete
     * audio snapshot for this block instead of consuming a torn config. */
    if ((g & 1u) || g == a->applied_generation) return;
    old_decay = a->config.decay_ms;
    sub_drive_changed = a->sub_config.drive != a->pending_sub_config.drive;
    lfo_rate_changed = a->config.lfo_rate_millihz != a->pending_config.lfo_rate_millihz;
    delay_changed =
        a->fx.delay_time_ms != a->pending_fx.delay_time_ms || a->fx.delay_sync != a->pending_fx.delay_sync;
    reverb_changed = a->fx.reverb_pre_ms != a->pending_fx.reverb_pre_ms ||
                     a->fx.reverb_size != a->pending_fx.reverb_size ||
                     a->fx.reverb_decay_ms != a->pending_fx.reverb_decay_ms;
    dist_changed = a->fx.dist_amount != a->pending_fx.dist_amount ||
                   a->fx.dist_character != a->pending_fx.dist_character;
    reverb_wet_changed = a->fx.reverb_wet != a->pending_fx.reverb_wet;
    a->config = a->pending_config;
    a->modr = a->pending_modr;
    a->fx = a->pending_fx;
    a->sub_config = a->pending_sub_config;
    a->sample_config = a->pending_sample_config;
#if MELLO_FEATURE_CHORD_MACHINE
    {
        bool was_on = a->chord_config.on;
        a->chord_config = a->pending_chord_config;
        if (was_on && !a->chord_config.on) chord_kill(a);
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
        if (a->delay_samples < 1u) a->delay_samples = 1u;
        if (a->delay_samples >= a->delay_capacity - 1u) a->delay_samples = a->delay_capacity - 2u;
    }
    if (reverb_changed || a->applied_generation == 0u) {
        reverb_configure(a);
#if defined(MELLO_LIVE_EDIT_TEST_HOOK) && MELLO_LIVE_EDIT_TEST_HOOK
        a->test_reverb_reconfigs++;
#endif
    }
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
        ramp_to(&a->rp_master, a->config.master_level, 50u, j);
        ramp_to(&a->rp_rev_send, a->reverb_send_permille, 80u, j);
        ramp_to(&a->rp_rev_level, a->fx.reverb_level, 80u, j);
        ramp_to(&a->rp_dly_send, a->fx.delay_wet, 100u, j);
        ramp_to(&a->rp_dly_level, a->fx.delay_level, 80u, j);
        ramp_to(&a->rp_dly_fb, a->fx.delay_feedback, 50u, j);
        ramp_to(&a->rp_dist_level, a->fx.dist_level, 80u, j);
        ramp_to(&a->rp_sub_level, a->sub_config.level, 50u, j);
        ramp_to(&a->rp_fsub_level, a->config.follower_sub_on ? a->config.follower_sub_level : 0u, 50u, j);
        ramp_to(&a->rp_delay, (int32_t)a->delay_samples, 50u, j);
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
    a->sub_gain = 32767;
    /* Unison oscillators start spread over the cycle, so that they never begin in phase (a flanged attack). */
    a->lead.uni_phase[0] = 0x55555555u;
    a->lead.uni_phase[1] = 0xAAAAAAAAu;
    a->lead.uni_phase[2] = 0x2AAAAAAAu;
    mello_sound_defaults(&a->pending_config);
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
    a->delay_buffer = db;
    a->delay_capacity = dc;
    a->reverb_buffer = rb;
    a->reverb_capacity = rc;
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
    config_update_end(a, update);
}
void mello_audio_set_sub_mute(MelloAudio *a, bool mute)
{
    if (a) a->sub_mute = mute;
}
void mello_audio_set_mod_routing(MelloAudio *a, const MelloModRouting *m)
{
    uint32_t update;
    if (!a || !m) return;
    update = config_update_begin(a);
    a->pending_modr = *m;
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
static bool audio_pop_due(MelloAudio *a, uint32_t now, MelloAudioQueuedEvent *out)
{
    uint8_t lane, best = 255u;
    uint32_t best_due = 0u;
    for (lane = 0u; lane < MELLO_AUDIO_EVENT_LANES; ++lane) {
        uint8_t head = a->q_head[lane];
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
        uint8_t head = a->q_head[best];
        uint8_t slot = audio_lane_slot(best, head);
        *out = a->queue[slot];
        a->q_head[best] = (uint8_t)((head + 1u) % MELLO_AUDIO_EVENT_LANE_CAP);
    }
    return true;
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
void mello_audio_render(MelloAudio *a, int32_t *out, uint32_t frames, uint32_t start_us, uint16_t bpm)
{
    uint32_t i, t = start_us, terr = 0u;
    if (!a || !out) return;
    apply_pending(a);
    /* Bound the IMA decode work per call whatever the sample settings (P1-4). Normal playback needs about
     * one block per voice per 256 frames; four voices at +24 semitones need 16. */
    a->sample_decode_budget = MELLO_SAMPLE_DECODE_BUDGET;
    if (a->silent && a->q_head == a->q_tail && !a->all_notes_off_pending) {
        a->rendered_frames += frames;
        return;
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
        switch (a->fx.delay_div) {
        case 1: ms = qms / 4u; break;
        case 2: ms = qms; break;
        case 3: ms = qms / 3u; break;
        default: ms = qms / 2u; break;
        }
        a->delay_samples = (ms * 441u + 5u) / 10u;
        if (a->delay_samples < 1u) a->delay_samples = 1u;
        if (a->delay_samples >= a->delay_capacity - 1u) a->delay_samples = a->delay_capacity - 2u;
        ramp_to(&a->rp_delay, (int32_t)a->delay_samples, 50u, a->rendered_frames == 0u);
    }
    for (i = 0u; i < frames; ++i) {
        int32_t lead = 0, sub = 0, sample_mix = 0, mix, mix_l = 0, mix_r = 0, lfo = 0, ampmod = 32767,
                mod_env, dmod[MELLO_DEST_COUNT];
        bool mods = false;
#if MELLO_FEATURE_CHORD_MACHINE
        int32_t chord_mix = 0;
#endif
        uint16_t cut = (uint16_t)(ramp_q8(&a->rp_cut) >> 8), res = (uint16_t)(ramp_q8(&a->rp_res) >> 8);
        int32_t master = ramp_q8(&a->rp_master) >> 8, rev_send = ramp_q8(&a->rp_rev_send) >> 8,
                rev_level = ramp_q8(&a->rp_rev_level) >> 8, dly_send = ramp_q8(&a->rp_dly_send) >> 8,
                dly_level = ramp_q8(&a->rp_dly_level) >> 8, dly_fb = ramp_q8(&a->rp_dly_fb) >> 8,
                dist_level = ramp_q8(&a->rp_dist_level) >> 8, sub_level = ramp_q8(&a->rp_sub_level) >> 8,
                fsub_level = ramp_q8(&a->rp_fsub_level) >> 8, delay_q8 = ramp_q8(&a->rp_delay);
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
#if MELLO_FEATURE_CHORD_MACHINE
        if (a->chord.env_stage == MELLO_ENV_ATTACK || a->chord.env_stage == MELLO_ENV_RELEASE)
            (void)env_tick(&a->chord.env_q24, a->chord.env_step_q24, a->chord.env_target_q24,
                           &a->chord.env_stage);
#endif
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
        if (a->config.lfo_on) {
            uint32_t ph = a->lfo_phase;
            int32_t s0 = SINE_Q15[(uint8_t)(ph >> 24)], s1 = SINE_Q15[(uint8_t)((ph >> 24) + 1u)];
            a->lfo_phase = ph + a->lfo_inc;
            lfo = s0 + (((s1 - s0) * (int32_t)((ph >> 16) & 0xFFu)) >> 8);
            a->last_lfo = (int16_t)lfo;
            if (a->config.lfo_target == MELLO_LFO_AMP)
                /* HTML: mix.gain = 0.58 + 0.58 * depth * sin, on the whole mix (applied below). */
                ampmod = 32767 + (lfo * (int32_t)a->config.lfo_depth / 1000);
            else if (a->config.lfo_target == MELLO_LFO_FILTER) {
                /* HTML: f = base * (1 + sin * (0.08 + 0.92 * depth)), linear in Hz: on the log cutoff that is
                 * + 1000 * log200(r), r = 0..2. */
                int32_t span = 80 + (int32_t)a->config.lfo_depth * 920 / 1000;       /* permille */
                int32_t m = lfo * span / 1000;                                       /* (r - 1) in Q15 */
                int32_t r64 = clamp32(64 + (m >> 9), 0, 127), rem = m & 511, lo, hi; /* r in 1/64 + rem/512 */
                lo = LFO_LOG200_PERMILLE[r64];
                hi = LFO_LOG200_PERMILLE[r64 + 1];
                cut = (uint16_t)clamp32((int32_t)cut + lo + ((hi - lo) * rem) / 512, 0, 1000);
            }
        }
        if (a->tm_filter_mod_q15) {
            int32_t tv = clamp32(a->tm_filter_mod_q15, -8192, 8192);
            uint32_t ti = (uint32_t)(tv + 8192) * 100u / 16384u;
            int32_t cv = (int32_t)cut + TM_FILTER_OFFSET_PERMILLE[ti > 100u ? 100u : ti];
            cut = (uint16_t)clamp32(cv, 0, 1000);
        }
        if (a->sub_mute ? a->sub_gain > 0 : a->sub_gain < 32767) /* SUB MUTE: ~10 ms either way */
            a->sub_gain = clamp32(a->sub_gain + (a->sub_mute ? -75 : 75), 0, 32767);
        if (a->dot_lane[0].stage || a->dot_lane[1].stage || a->tm_value) {
            /* DOT's MOD lanes and EVOLVE's sound routes, summed per destination in permille of its range. */
            uint8_t l;
            memset(dmod, 0, sizeof dmod);
            for (l = 0u; l < MELLO_DOT_MOD_LANES; ++l) {
                dot_lane_tick(a, &a->dot_lane[l], l);
                dmod[a->modr.dot_target[l] % MELLO_DEST_COUNT] += a->dot_lane[l].v >> 16;
                dmod[a->modr.tm_dest[l] % MELLO_DEST_COUNT] += a->tm_value * a->modr.tm_depth[l] / 32768;
            }
            dmod[MELLO_DEST_OFF] = 0;
            mods = true;
            if (dmod[MELLO_DEST_CUTOFF])
                cut = (uint16_t)clamp32((int32_t)cut + dmod[MELLO_DEST_CUTOFF], 0, 1000);
        }
        if (a->menv_stage == 1u) {
            a->menv_q24 += a->menv_step;
            if (a->menv_q24 >= 1 << 24) a->menv_q24 = 1 << 24, a->menv_stage = 2u;
        } else if (a->menv_stage == 2u) {
            a->menv_q24 -= ((a->menv_q24 >> 8) * a->menv_k_q16) >> 8;
            if (a->menv_q24 < 1 << 10) a->menv_q24 = 0, a->menv_stage = 0u;
        }
        mod_env = 0;
        if (a->config
                .menv_amount) { /* +-32767; VEL > MOD ENV scales it by up to x2 (or to none) for an accent */
            int32_t amt = a->config.menv_amount;
            if (a->config.vel_menv)
                amt = clamp32(amt + amt * (a->lead.vel_q15 * a->config.vel_menv / 1000) / 32767, -1000, 1000);
            mod_env = (a->menv_q24 >> 9) * amt / 1000;
        }
        if (a->config.vel_cutoff) /* VEL > CUTOFF: up to 30% of the range for an accent */
            cut = (uint16_t)clamp32(
                (int32_t)cut + a->lead.vel_q15 * a->config.vel_cutoff / 1000 * 300 / 32767, 0, 1000);
        if (a->config.key_track) {
            /* KEY TRACK from C4: 1000 * log200(2) / 12 = 10.9 permille of the cutoff a semitone, at 1000 */
            int32_t semis_q8 = (a->lead.pitch_q16 >> 8) - 60 * 256;
            cut = (uint16_t)clamp32(
                (int32_t)cut + semis_q8 * (int32_t)a->config.key_track / 1000 * 109 / 2560, 0, 1000);
        }
        if (mod_env && a->config.menv_target == MELLO_MENV_CUTOFF)
            cut = (uint16_t)clamp32((int32_t)cut + mod_env * 1000 / 32767, 0, 1000);
        res = (uint16_t)lfo_around(res, lfo, a->config.lfo_depth, MELLO_LFO_RES, a);
        if (mods && dmod[MELLO_DEST_RES])
            res = (uint16_t)clamp32((int32_t)res + dmod[MELLO_DEST_RES], 0, 1000);
        if (mod_env && a->config.menv_target == MELLO_MENV_RES)
            res = (uint16_t)clamp32((int32_t)res + mod_env * 1000 / 32767, 0, 1000);
        if (a->lead.env_q24 > 0 && a->config.osc_wave != MELLO_WAVE_OFF) {
            uint32_t inc = a->lead.inc;
            uint16_t pwm = a->config.pwm_permille;
            if (a->config.lfo_on && a->config.lfo_target == MELLO_LFO_PITCH) {
                /* HTML: Tone.LFO into synth.detune, +-(80 + 620 * depth) cents (the follower sub is not detuned).
                 * c is in 1/16 cent; the table steps 25 cents (400). */
                int32_t c = (lfo * (80 + (int32_t)a->config.lfo_depth * 620 / 1000)) >> 11;
                uint32_t k = (uint32_t)(c + 700 * 16), idx = k / 400u, rem = k % 400u, r;
                if (idx > 55u) idx = 55u, rem = 400u;
                r = LFO_CENTS_RATIO_Q16[idx] +
                    (uint32_t)(((int32_t)(LFO_CENTS_RATIO_Q16[idx + 1u] - LFO_CENTS_RATIO_Q16[idx]) *
                                (int32_t)rem) /
                               400);
                inc = (uint32_t)(((uint64_t)inc * r) >> 16);
            }
            if (a->config.lfo_on && a->config.lfo_target == MELLO_LFO_PWM) {
                /* HTML: width + sin * swing, swing = min(0.42 * depth, width - 0.05, 0.95 - width): symmetric. */
                int32_t sw = (int32_t)a->config.lfo_depth * 420 / 1000, pb = clamp32((int32_t)pwm, 50, 950);
                if (sw > pb - 50) sw = pb - 50;
                if (sw > 950 - pb) sw = 950 - pb;
                pwm = (uint16_t)(pb + (lfo * sw) / 32767);
            }
            if (mod_env && a->config.menv_target == MELLO_MENV_PITCH) /* +-24 semitones */
                inc = inc_times(inc, semi_ratio_q16(mod_env * 6144 / 32767));
            if (mods && dmod[MELLO_DEST_PITCH]) /* +-12 semitones */
                inc = inc_times(inc,
                                semi_ratio_q16(clamp32(dmod[MELLO_DEST_PITCH], -1000, 1000) * 3072 / 1000));
            if (mods && dmod[MELLO_DEST_PWM])
                pwm = (uint16_t)clamp32((int32_t)pwm + dmod[MELLO_DEST_PWM] * 450 / 1000, 50, 950);
            {
                /* Grit and the Virus layer. Each part costs nothing while it is off, and the defaults are off:
                 * then this is the HTML's plain saw. */
                const MelloSoundConfig *c = &a->config;
                int32_t noise_amt = lfo_around(c->noise_level, lfo, c->lfo_depth, MELLO_LFO_NOISE, a);
                int32_t nfm_amt = lfo_around(c->noise_fm, lfo, c->lfo_depth, MELLO_LFO_NOISE_FM, a);
                int32_t sat = lfo_around(c->saturation, lfo, c->lfo_depth, MELLO_LFO_SAT, a);
                uint8_t uni = c->unison, k;
                int32_t white = 0, dev = 0, o1 = 0;
                uint32_t inc2 = 0u, before = a->lead.phase;
                if (mod_env && c->menv_target == MELLO_MENV_NOISE)
                    noise_amt = clamp32(noise_amt + mod_env * 1000 / 32767, 0, 1000);
                if (mods) {
                    noise_amt = clamp32(noise_amt + dmod[MELLO_DEST_NOISE], 0, 1000);
                    nfm_amt = clamp32(nfm_amt + dmod[MELLO_DEST_NOISE_FM], 0, 1000);
                    sat = clamp32(sat + dmod[MELLO_DEST_SAT], 0, 1000);
                }
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
                    dev = ((a->nfm_lp * (nfm_amt * nfm_amt / 1000 * 737 / 1000)) >> 15) * 58000;
                }
                if (uni > 1u) {
                    /* Unison: detune spreads the oscillators up to +-60 cents (squared). Near 1 the ratio is
                     * 1 + c * ln2 / 1200 to within a cent, so no table: 2^-18 per unit, c * 151.4. */
                    int32_t det = clamp32(lfo_around(c->detune, lfo, c->lfo_depth, MELLO_LFO_DETUNE, a) +
                                              (mods ? dmod[MELLO_DEST_DETUNE] : 0),
                                          0, 1000);
                    int32_t d_q18 = det * det / 1000 * 60 * 1514 / 10000; /* cents * 151.4 */
                    int32_t step =
                        (int32_t)(((inc >> 14) * (uint32_t)d_q18) >> 4) / 3; /* a third of the spread */
                    int32_t sum = 0;
                    for (k = 0u; k < uni; ++k) {
                        uint32_t vi =
                            (uint32_t)((int32_t)inc + step * UNISON_SPREAD_THIRDS[uni - 1u][k] + dev);
                        uint32_t *ph = k ? &a->lead.uni_phase[k - 1u] : &a->lead.phase;
                        int32_t y;
                        *ph += vi;
                        y = tone_osc_sample(c->osc_wave, *ph, inc_size(vi), pwm);
                        if (!k) o1 = y;
                        sum += y;
                    }
                    lead = q15mul(clamp32(sum, -65535, 65535), UNISON_GAIN_Q15[uni - 1u]);
                } else {
                    uint32_t vi = inc + (uint32_t)dev;
                    a->lead.phase += vi;
                    lead = o1 = tone_osc_sample(c->osc_wave, a->lead.phase, inc_size(vi), pwm);
                }
                if (c->osc2_mix && c->osc2_wave != MELLO_WAVE_OFF) {
                    /* OSC 2: SEMI and FINE from OSC 1, hard sync (it restarts where OSC 1 wraps, at the same
                     * fraction of its own cycle) and FM from OSC 1 (linear, through zero, up to 4x its frequency;
                     * squared). The Virus's oscillator pair. */
                    int32_t q8 = c->osc2_semi * 256 + c->osc2_fine * 256 / 100, fm, o2;
                    uint32_t r2, v2;
                    if (c->lfo_on && c->lfo_target == MELLO_LFO_OSC2)
                        q8 += lfo * (int32_t)c->lfo_depth / 10667; /* +-12 */
                    if (mod_env && c->menv_target == MELLO_MENV_OSC2)
                        q8 += mod_env * 12288 / 32767;                                         /* +-48 */
                    if (mods) q8 += clamp32(dmod[MELLO_DEST_OSC2], -1000, 1000) * 6144 / 1000; /* +-24 */
                    r2 = semi_ratio_q16(q8);
                    inc2 = inc_times(inc, r2);
                    fm = lfo_around(c->fm_amount, lfo, c->lfo_depth, MELLO_LFO_FM, a);
                    if (mod_env && c->menv_target == MELLO_MENV_FM)
                        fm = clamp32(fm + mod_env * 1000 / 32767, 0, 1000);
                    if (mods) fm = clamp32(fm + dmod[MELLO_DEST_FM], 0, 1000);
                    v2 = inc2 + (uint32_t)dev;
                    if (fm) {
                        int64_t d = ((int64_t)inc2 * q15mul(o1, fm * fm / 1000 * 32767 / 1000)) >> 13;
                        v2 = (uint32_t)(int32_t)(d > 0x7FFFFFFF ? 0x7FFFFFFF
                                                                : (d < -0x7FFFFFFF ? -0x7FFFFFFF : d)) +
                             v2;
                    }
                    if (c->osc2_sync && (int32_t)(inc + (uint32_t)dev) > 0 && a->lead.phase < before)
                        a->lead.osc2_phase = (uint32_t)(((uint64_t)a->lead.phase * r2) >> 16);
                    else
                        a->lead.osc2_phase += v2;
                    o2 = tone_osc_sample(c->osc2_wave, a->lead.osc2_phase, inc_size(v2), pwm);
                    lead = (lead * (1000 - (int32_t)c->osc2_mix) + o2 * (int32_t)c->osc2_mix) / 1000;
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
                    lead += q15mul(clamp32(hp, -65535, 65535), noise_amt * 16683 / 1000);
                }
                if (sat) lead = saturate(clamp32(lead, -65535, 65535), sat);
            }
            if (fsub_level) {
                a->lead.sub_phase += a->lead.sub_inc;
                /* HTML: subOsc is triggered at vel * level and then passes Gain(level): level squared. */
                int32_t lv = fsub_level,
                        fs = q15mul(tone_osc_sample(a->config.follower_sub_wave, a->lead.sub_phase,
                                                    a->lead.sub_inc, 500u),
                                    lv * lv / 1000 * 32767 / 1000);
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
            /* HTML: Tone.Filter rolloff=-24 => two cascaded biquads, both Q=.8. */
            filtered = sub_biquad_step(sub, bc, &a->sub.biquad1_x1, &a->sub.biquad1_x2, &a->sub.biquad1_y1,
                                       &a->sub.biquad1_y2);
            filtered = sub_biquad_step(filtered, bc, &a->sub.biquad2_x1, &a->sub.biquad2_x2,
                                       &a->sub.biquad2_y1, &a->sub.biquad2_y2);
            if (a->sub_config.drive) {
                /* Lightweight 2x approximation around Tone.WaveShaper oversample=2x. */
                int32_t mid = a->sub.drive_prev + ((filtered - a->sub.drive_prev) >> 1);
                filtered = (sub_tone_distort(a, mid) + sub_tone_distort(a, filtered)) / 2;
            }
            a->sub.drive_prev = filtered;
            sub = filtered * sub_level / 1000;
            if (a->sub_gain != 32767) sub = q15mul(clamp32(sub, -65535, 65535), a->sub_gain); /* SUB MUTE */
        }
#if MELLO_FEATURE_CHORD_MACHINE
        if (a->chord_config.on && a->chord.active_count && a->chord.env_q24 > 0 &&
            a->chord_config.wave != MELLO_WAVE_OFF) {
            uint8_t cv;
            int32_t sum = 0;
            static const int16_t norm[5] = {0, 32767, 18500, 13200, 10400};
            for (cv = 0u; cv < MELLO_CHORD_MAX_VOICES; ++cv)
                if (a->chord.active[cv]) {
                    int32_t base, soft;
                    a->chord.phase[cv] += a->chord.inc[cv];
                    base = osc_sample(a->chord_config.wave, a->chord.phase[cv], a->chord.inc[cv], 500u);
                    /* A quiet sine fundamental thickens the layer without adding the lead's bright edge. */
                    soft = SINE_Q15[(uint8_t)(a->chord.phase[cv] >> 24)];
                    sum += q15mul(base, 22000) + q15mul(soft, 10700);
                }
            sum = q15mul(sum, norm[a->chord.active_count <= 4u ? a->chord.active_count : 4u]);
            sum = q15mul(sum, a->chord.env_q24 >> 8);
            /* Fixed ~3.5 kHz low-pass followed by ~120 Hz high-pass: a mid-bed, not a second lead/sub. */
            a->chord.tone_lp += q15mul(sum - a->chord.tone_lp, 12880);
            {
                const int32_t hp_a = 32210;
                int32_t hp = q15mul(hp_a, a->chord.hp_y1 + a->chord.tone_lp - a->chord.hp_x1);
                a->chord.hp_x1 = a->chord.tone_lp;
                a->chord.hp_y1 = hp;
                sum = hp;
            }
            /* Automatic ~19% duck at full lead envelope leaves the melodic transient in front. */
            {
                int32_t lead_env = a->lead.env_q24 >> 8;
                int32_t duck = 32767 - ((lead_env * 3) >> 4);
                if (duck < 24500) duck = 24500;
                sum = q15mul(sum, duck);
            }
            chord_mix = sum * (int32_t)a->chord_config.level / 1000;
        }
#endif
        {
            uint8_t sv;
            for (sv = 0u; sv < MELLO_SAMPLE_SLOT_COUNT; ++sv)
                sample_mix += sample_render_voice(a, &a->sample_voice[sv]);
        }
        mix = (lead + sub + sample_mix) * 580 / 1000;
        if (ampmod != 32767) mix = q15mul_wide(clamp32(mix, -65535, 65535), ampmod); /* reaches 65534: x2 */
        {
            /* HTML: Tone.Filter lowpass -24 = two identical biquads, Q = res*15 dB, f = 80*200^cut. */
            int32_t y;
            main_filter_coeffs(a, cut, res);
            y = main_biquad_step(a->mf_coeff, a->mf_state[0], clamp32(mix, -65535, 65535));
            y = main_biquad_step(a->mf_coeff, a->mf_state[1], y);
            mix = clamp32(y, -65535, 65535);
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
            mix = mix * dist_level / 1000;
        }
#if MELLO_FEATURE_CHORD_MACHINE
        /* Chord keeps its own spectral identity and bypasses lead distortion, but shares delay/reverb space. */
        mix += chord_mix * 800 / 1000;
#endif
        {
            int32_t fx_in = mix;
            mix = fx_in * 840 / 1000;
            if (a->delay_buffer && a->delay_capacity > 2u && a->delay_samples) {
                /* The delay time ramps (Tone's delayTime.rampTo): read between samples while it moves. */
                uint32_t di = (uint32_t)delay_q8 >> 8, fr = (uint32_t)delay_q8 & 0xFFu, rp, rp1;
                int32_t d, send = fx_in * dly_send / 1000, dl;
                if (di < 1u) di = 1u;
                if (di > a->delay_capacity - 2u) di = a->delay_capacity - 2u;
                rp = a->delay_pos >= di ? a->delay_pos - di : a->delay_pos + a->delay_capacity - di;
                d = a->delay_buffer[rp];
                if (fr) {
                    rp1 = rp ? rp - 1u : a->delay_capacity - 1u;
                    d += ((a->delay_buffer[rp1] - d) * (int32_t)fr) >> 8;
                }
                dl = tone_lp24(&DELAY_TONE_BIQUAD_Q28[a->fx.delay_tone / 10u], a->delay_tone_s, d);
                a->delay_buffer[a->delay_pos] = sat16(send + d * dly_fb / 1000);
                if (++a->delay_pos >= a->delay_capacity) a->delay_pos = 0u;
                mix += (dl * dly_level) / 1000;
            }
            mix_l = mix;
            mix_r = mix;
            if (a->reverb_buffer && a->reverb_capacity > 1024u &&
                (a->fx.reverb_wet || a->tail_guard_frames)) {
                int32_t send = fx_in * rev_send / 1000, rv_l = 0, rv_r = 0;
                const ToneSubBiquadCoeff *rt = &REVERB_TONE_BIQUAD_Q28[a->fx.reverb_tone / 10u];
                int32_t tl, tr;
                reverb_step(a, send, &rv_l, &rv_r);
                tl = tone_lp24(rt, a->reverb_tone_s[0], rv_l);
                tr = tone_lp24(rt, a->reverb_tone_s[1], rv_r);
                {
                    /* DC blocker, ~27 Hz: reverb_hp_y1 integrates the DC in Q8 and is subtracted. The old
                     * q15mul one-pole high-pass floored to a dead band and passed about -150 of DC forever,
                     * which kept the tail guard armed, so the unit never counted as silent (P1-1). */
                    int32_t hp_l = tl - (a->reverb_hp_y1 >> 8);
                    int32_t hp_r = tr - (a->reverb_hp_y1_r >> 8);
                    a->reverb_hp_y1 += hp_l;
                    a->reverb_hp_y1_r += hp_r;
                    mix_l += ((hp_l * rev_level) / 1000) * 1050 / 1000; /* reverbReturnGain */
                    mix_r += ((hp_r * rev_level) / 1000) * 1050 / 1000;
                }
            }
            limiter_step(a, &mix_l, &mix_r);
            mix_l = clamp32(mix_l * master / 1000, -32767, 32767);
            mix_r = clamp32(mix_r * master / 1000, -32767, 32767);
        }
        if ((mix_l > 8 || mix_l < -8 || mix_r > 8 || mix_r < -8) && (a->fx.delay_wet || a->fx.reverb_wet)) {
            uint32_t guard = 0u;
            if (a->fx.delay_wet) guard = a->delay_samples + 512u;
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
                    && samples_silent && a->tail_guard_frames == 0u;
    }
}
