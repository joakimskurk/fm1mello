#include "mello_ui.h"

#define UI_RGB(r, g, b)                                                                                      \
    ((uint16_t)((((uint16_t)(r) & 0xF8u) << 8) | (((uint16_t)(g) & 0xFCu) << 3) | ((uint16_t)(b) >> 3)))
#define UI_BLACK   UI_RGB(4, 14, 8) /* the ground: the HTML's #040E08, not black */
#define UI_PANEL   UI_RGB(6, 24, 16)
#define UI_PANEL2  UI_RGB(13, 36, 24)
#define UI_DIM     UI_RGB(68, 112, 78)
#define UI_MID     UI_RGB(92, 166, 111)
#define UI_WHITE   UI_RGB(225, 255, 238)
#define UI_ACC     UI_RGB(0, 221, 136)
#define UI_CYAN    UI_RGB(0, 238, 220)
#define UI_GOLD    UI_RGB(255, 204, 0)
#define UI_PURPLE  UI_RGB(177, 92, 255)  /* generative / random */
#define UI_MAGENTA UI_RGB(255, 72, 184)  /* ratchet / repeat */
#define UI_CORAL   UI_RGB(255, 119, 85)  /* accent / emphasis */
#define UI_BLUE    UI_RGB(68, 187, 255)  /* movement / modulation */
#define UI_SOFT    UI_RGB(142, 184, 158) /* neutral edit selection */
#define UI_RED     UI_RGB(255, 68, 68)
#define UI_LINE    UI_RGB(23, 66, 45)    /* the HTML's borders and slider tracks' edge */
#define UI_TRACK   UI_RGB(13, 36, 24)    /* slider tracks, inactive pills */
#define UI_NOTE    UI_RGB(190, 255, 220) /* note names in the step bars */
#define UI_BRIGHT  UI_RGB(153, 255, 204) /* values, the playing step's top line */
#define UI_CHORD   UI_RGB(148, 126, 255) /* harmonic layer */

/* Structural colours: segment identity is deliberately softer than function colours. */
#define UI_SEG_A     UI_RGB(0, 221, 136) /* the HTML's green: one segment looks like the HTML */
#define UI_SEG_B     UI_RGB(225, 171, 72)
#define UI_SEG_C     UI_RGB(92, 146, 220)
#define UI_SEG_D     UI_RGB(174, 105, 210)
#define UI_SEG_E     UI_RGB(146, 184, 82)
#define UI_SEG_F     UI_RGB(222, 113, 76)
#define UI_SEG_A_DIM UI_RGB(28, 77, 49)
#define UI_SEG_B_DIM UI_RGB(84, 64, 28)
#define UI_SEG_C_DIM UI_RGB(35, 57, 84)
#define UI_SEG_D_DIM UI_RGB(66, 41, 80)
#define UI_SEG_E_DIM UI_RGB(55, 70, 32)
#define UI_SEG_F_DIM UI_RGB(82, 43, 30)

static const uint8_t white_key_notes[MELLO_UI_WHITE_KEYS] = {0u,  2u,  4u,  6u,  7u,  9u,  11u, 12u,
                                                             14u, 16u, 18u, 19u, 21u, 23u, 24u, 26u};
static const uint8_t tm_lengths[] = {3u,  4u,  5u,  6u,  7u,  8u,  9u,  10u, 11u,
                                     12u, 13u, 14u, 15u, 16u, 20u, 24u, 28u, 32u};
static const uint8_t slot_counts[] = {2u, 4u, 8u, 16u, 32u, 64u};
static const uint8_t sequence_lengths[] = {4u, 5u, 6u, 7u, 8u, 10u, 12u, 16u, 20u, 24u, 28u, 32u};
static const uint8_t sub_waveforms[] = {MELLO_WAVE_SINE, MELLO_WAVE_TRIANGLE, MELLO_WAVE_SQUARE,
                                        MELLO_WAVE_SAW};
static const uint16_t master_lengths[] = {0u, 16u, 32u, 64u, 128u, 256u};
static const uint16_t slot_holds[] = {1u, 2u, 4u, 8u, 16u, 32u, 64u};
static const char name_chars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.";

static int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static uint32_t bit_u32(uint8_t n)
{
    return n < 32u ? (1u << n) : 0u;
}

static uint8_t wrap_u8(int32_t v, uint8_t count)
{
    int32_t c = count;
    if (c <= 0) return 0u;
    while (v < 0) v += c;
    while (v >= c) v -= c;
    return (uint8_t)v;
}

static int32_t option_index_u8(const uint8_t *a, uint8_t n, uint8_t value)
{
    uint8_t i;
    for (i = 0u; i < n; ++i)
        if (a[i] == value) return (int32_t)i;
    return 0;
}

static int32_t option_index_u16(const uint16_t *a, uint8_t n, uint16_t value)
{
    uint8_t i;
    for (i = 0u; i < n; ++i)
        if (a[i] == value) return (int32_t)i;
    return 0;
}

static MelloWaveform step_sub_waveform(MelloWaveform current, int16_t delta)
{
    uint8_t i, count = (uint8_t)(sizeof(sub_waveforms) / sizeof(sub_waveforms[0]));
    for (i = 0u; i < count; ++i)
        if (sub_waveforms[i] == (uint8_t)current) break;
    if (i >= count) return (MelloWaveform)(delta > 0 ? sub_waveforms[0] : sub_waveforms[count - 1u]);
    return (MelloWaveform)sub_waveforms[wrap_u8((int32_t)i + (delta > 0 ? 1 : -1), count)];
}

static uint8_t random_target_count(void)
{
    return (uint8_t)(MELLO_FEATURE_CHORD_MACHINE ? 13u : 12u);
}

static uint16_t random_target_bit(uint8_t index);
/* A RANDOM pill's MELLO_RAND_* bit number (its rnd_amount index). */
static uint8_t random_target_bitno(uint8_t index)
{
    uint16_t bit = random_target_bit(index);
    uint8_t n = 0u;
    while (bit > 1u) bit >>= 1, ++n;
    return n;
}

static uint16_t random_target_bit(uint8_t index)
{
#if MELLO_FEATURE_CHORD_MACHINE
    static const uint16_t bits[13] = {
        MELLO_RAND_PITCH,  MELLO_RAND_PULSES, MELLO_RAND_RATCHET, MELLO_RAND_GATE, MELLO_RAND_OCTAVE,
        MELLO_RAND_ACCENT, MELLO_RAND_SUB,    MELLO_RAND_SLIDE,   MELLO_RAND_SKIP, MELLO_RAND_PROB,
        MELLO_RAND_SAMPLE, MELLO_RAND_CHORD,  MELLO_RAND_WAVE};
#else
    static const uint16_t bits[12] = {MELLO_RAND_PITCH, MELLO_RAND_PULSES, MELLO_RAND_RATCHET,
                                      MELLO_RAND_GATE,  MELLO_RAND_OCTAVE, MELLO_RAND_ACCENT,
                                      MELLO_RAND_SUB,   MELLO_RAND_SLIDE,  MELLO_RAND_SKIP,
                                      MELLO_RAND_PROB,  MELLO_RAND_SAMPLE, MELLO_RAND_WAVE};
#endif
    return index < random_target_count() ? bits[index] : 0u;
}

static uint8_t step_current(MelloUi *ui)
{
    if (!ui || !ui->transport) return 0u;
    if (ui->selected_step >= MELLO_MAX_STEPS) ui->selected_step = 0u;
    return ui->selected_step;
}

static MelloEngine *eng(MelloUi *ui)
{
    return ui && ui->transport ? &ui->transport->engine : 0;
}

static void mark_full(MelloUi *ui)
{
    ui->dirty |= MELLO_UI_DIRTY_ALL;
}

static void mark_edit(MelloUi *ui)
{
    ui->dirty |= MELLO_UI_DIRTY_BODY | MELLO_UI_DIRTY_FOOTER | MELLO_UI_DIRTY_STATUS | MELLO_UI_DIRTY_LEDS;
}

static void rebuild(MelloUi *ui)
{
    MelloEngine *e = eng(ui);
    if (e) (void)mello_engine_rebuild_sequence(e);
}

static void persistent_dirty(MelloUi *ui, uint32_t now_ms)
{
    if (ui->storage) mello_storage_mark_dirty(ui->storage, now_ms);
}

static void manual_save(MelloUi *ui, uint32_t now_ms)
{
    if (ui->storage) mello_storage_request_save(ui->storage, now_ms);
    ui->dirty |= MELLO_UI_DIRTY_STATUS;
}

static uint8_t page_button(MelloUi *ui)
{
    switch (ui->page) {
    case MELLO_UI_SEQ_STEP:
    case MELLO_UI_SEQ_SEGMENT:
    case MELLO_UI_SEQ_LOOP:
    case MELLO_UI_SEQ_SLOTSEQ: return MELLO_UI_BTN_SEQ;
    case MELLO_UI_RANDOM: return 0xFFu;
    case MELLO_UI_KEY_SCALE: return MELLO_UI_BTN_SEL;
    case MELLO_UI_SLOTS: return MELLO_UI_BTN_SAVE;
    case MELLO_UI_DOT:
    case MELLO_UI_TURING: return MELLO_UI_BTN_LFO;
    case MELLO_UI_SUB: return MELLO_UI_BTN_ARP;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: return MELLO_UI_BTN_ARP;
#endif
    case MELLO_UI_GLOBAL: return MELLO_UI_BTN_GLO;
    case MELLO_UI_SOUND_RESERVED: return ui->sound_subpage == 2u ? MELLO_UI_BTN_LFO : MELLO_UI_BTN_ENV;
    case MELLO_UI_AUDIO_FX: return MELLO_UI_BTN_FX;
    case MELLO_UI_SAMPLE_RESERVED: return MELLO_UI_BTN_GLO;
    case MELLO_UI_DRUMS: return MELLO_UI_BTN_ARP;
    case MELLO_UI_HOME:
    default: return MELLO_UI_BTN_HOME;
    }
}

const char *mello_ui_page_name(MelloUiPage page)
{
    switch (page) {
    case MELLO_UI_HOME: return "HOME";
    case MELLO_UI_SEQ_STEP: return "STEP";
    case MELLO_UI_SEQ_SEGMENT: return "SEGMENT";
    case MELLO_UI_SEQ_LOOP: return "LOOP CYCLE";
    case MELLO_UI_SEQ_SLOTSEQ: return "SLOT SEQ";
    case MELLO_UI_RANDOM: return "RANDOM";
    case MELLO_UI_KEY_SCALE: return "KEY / SCALE";
    case MELLO_UI_SLOTS: return "SLOTS";
    case MELLO_UI_DOT: return "DOT MATRIX";
    case MELLO_UI_TURING: return "EVOLVE";
    case MELLO_UI_SUB: return "SUB";
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: return "CHORD";
#endif
    case MELLO_UI_GLOBAL: return "GLOBAL";
    case MELLO_UI_SOUND_RESERVED: return "SOUND";
    case MELLO_UI_AUDIO_FX: return "AUDIO FX";
    case MELLO_UI_SAMPLE_RESERVED: return "SAMPLE";
    case MELLO_UI_DRUMS: return "DRUMS";
    default: return "MELLO";
    }
}

int8_t mello_ui_white_key_position(uint8_t note_index)
{
    uint8_t i;
    for (i = 0u; i < MELLO_UI_WHITE_KEYS; ++i)
        if (white_key_notes[i] == note_index) return (int8_t)i;
    return -1;
}

uint8_t mello_ui_white_key_note_index(uint8_t white_position)
{
    if (white_position >= MELLO_UI_WHITE_KEYS) return 0xFFu;
    return white_key_notes[white_position];
}

void mello_ui_init(MelloUi *ui, MelloTransport *transport, MelloStorage *storage)
{
    if (!ui) return;
    memset(ui, 0, sizeof *ui);
    ui->transport = transport;
    ui->storage = storage;
    ui->page = MELLO_UI_HOME;
    ui->random_intensity = MELLO_RANDOMIZE_MED;
    ui->selected_slot = 0u;
    ui->selected_tm_bit = 0u;
    ui->seen_step = 0xFFu;
    ui->seen_pulse = 0xFFu;
    ui->seen_segment = 0xFFu;
    ui->seen_loop_pos = 0xFFu;
    ui->seen_slot_seq_pos = 0xFFu;
    ui->seen_tm_pos = -2;
    ui->seen_dot_cell = -2;
    ui->dot_trail[0] = ui->dot_trail[1] = -1;
    ui->held_step = MELLO_UI_NO_STEP;
    ui->audition_key = 0xFFu;
    ui->seen_sub_x_pos = -2;
#if MELLO_FEATURE_CHORD_MACHINE
    ui->seen_chord_pos = 0xFFu;
    ui->seen_chord_grid = -2;
#endif
    ui->seen_running = 2u;
    ui->home_old_step = 0xFFu;
    ui->home_new_step = 0xFFu;
    ui->last_key_led_mask = 0xFFFFFFFFu;
    ui->last_button_led_mask = 0xFFFFFFFFu;
    ui->dirty = MELLO_UI_DIRTY_ALL;
}

void mello_ui_attach_sample_bank(MelloUi *ui, MelloSampleBank *bank)
{
    if (!ui) return;
    ui->sample_bank = bank;
    ui->dirty |= MELLO_UI_DIRTY_BODY | MELLO_UI_DIRTY_FOOTER | MELLO_UI_DIRTY_LEDS;
}

void mello_ui_set_page(MelloUi *ui, MelloUiPage page)
{
    if (!ui) return;
    if (page > MELLO_UI_DRUMS) page = MELLO_UI_HOME;
    if (ui->page == page) return;
    ui->page = page;
    ui->readout = 0u; /* it belonged to the old page */
    ui->held_step = MELLO_UI_NO_STEP;
    ui->keys_mode = ui->keys_rec = false;
    mark_full(ui);
}

/* A page on its first layer: page buttons open pages this way, so what a button shows never depends on history. */
static void open_page(MelloUi *ui, MelloUiPage page)
{
    switch (page) {
    case MELLO_UI_SEQ_STEP: ui->step_subpage = 0u; break;
    case MELLO_UI_SEQ_SEGMENT: ui->segment_subpage = 0u; break;
    case MELLO_UI_SEQ_LOOP: ui->loop_subpage = 0u; break;
    case MELLO_UI_SLOTS: ui->slots_subpage = 0u; break;
    case MELLO_UI_DOT:
        ui->selected_dot_target = 0u;
        ui->dot_shape = false;
        break;
    case MELLO_UI_TURING: ui->tm_subpage = 0u; break;
    case MELLO_UI_SUB: ui->sub_subpage = 0u; break;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: ui->chord_subpage = 0u; break;
#endif
    case MELLO_UI_GLOBAL: ui->global_subpage = 0u; break;
    case MELLO_UI_SOUND_RESERVED: ui->sound_subpage = 0u; break;
    case MELLO_UI_AUDIO_FX: ui->fx_subpage = 0u; break;
    case MELLO_UI_SAMPLE_RESERVED: ui->sample_subpage = 0u; break;
    case MELLO_UI_DRUMS: ui->drum_subpage = 0u; break;
    default: break;
    }
    mello_ui_set_page(ui, page);
    mark_full(ui);
}

static void seq_button(MelloUi *ui)
{
    switch (ui->page) {
    case MELLO_UI_SEQ_STEP: mello_ui_set_page(ui, MELLO_UI_SEQ_SEGMENT); break;
    case MELLO_UI_SEQ_SEGMENT: mello_ui_set_page(ui, MELLO_UI_SEQ_LOOP); break;
    case MELLO_UI_SEQ_LOOP: mello_ui_set_page(ui, MELLO_UI_SEQ_SLOTSEQ); break;
    case MELLO_UI_SEQ_SLOTSEQ: mello_ui_set_page(ui, MELLO_UI_SEQ_STEP); break;
    default: open_page(ui, MELLO_UI_SEQ_STEP); break;
    }
}

static uint8_t prob_pct(uint16_t p)
{
    return (uint8_t)(((uint32_t)p * 100u + 32767u) / 65535u);
}

static uint16_t pct_prob(uint8_t pct)
{
    return (uint16_t)(((uint32_t)pct * 65535u + 50u) / 100u);
}

static const char *wave_name(MelloWaveform w)
{
    switch (w) {
    case MELLO_WAVE_SAW: return "SAW";
    case MELLO_WAVE_SQUARE: return "SQUARE";
    case MELLO_WAVE_TRIANGLE: return "TRI";
    case MELLO_WAVE_SINE: return "SINE";
    case MELLO_WAVE_PULSE: return "PULSE";
    case MELLO_WAVE_OFF: return "OFF";
    default: return "?";
    }
}

static const char *lfo_target_name(MelloLfoTarget t)
{
    switch (t) {
    case MELLO_LFO_FILTER: return "FILTER";
    case MELLO_LFO_PITCH: return "PITCH";
    case MELLO_LFO_AMP: return "AMP";
    case MELLO_LFO_PWM: return "PWM";
    case MELLO_LFO_NOISE: return "NOISE";
    case MELLO_LFO_NOISE_FM: return "NOISE FM";
    case MELLO_LFO_DETUNE: return "DETUNE";
    case MELLO_LFO_RES: return "RES";
    case MELLO_LFO_OSC2: return "OSC 2";
    case MELLO_LFO_FM: return "FM";
    case MELLO_LFO_SAT: return "SAT";
    default: return "?";
    }
}

static const char *mod_dest_name(uint8_t d)
{
    static const char *n[MELLO_DEST_COUNT] = {"OFF", "CUTOFF", "RES",  "DECAY", "PITCH",  "OSC 2",
                                              "FM",  "NOISE",  "N.FM", "SAT",   "DETUNE", "PWM"};
    return d < MELLO_DEST_COUNT ? n[d] : "?";
}

/* DOT's layers in SELECT's order: each matrix, and after each MOD lane its SHAPE. */
static const uint8_t DOT_LAYERS[7][2] = {
    {MELLO_DOT_ACCENT, 0u}, {MELLO_DOT_MOD1, 0u},    {MELLO_DOT_MOD1, 1u}, {MELLO_DOT_MOD2, 0u},
    {MELLO_DOT_MOD2, 1u},   {MELLO_DOT_RATCHET, 0u}, {MELLO_DOT_SUB, 0u}};
static int8_t dot_lane(uint8_t matrix)
{
    return matrix == MELLO_DOT_MOD1 ? 0 : (matrix == MELLO_DOT_MOD2 ? 1 : -1);
}

static const char *menv_target_name(MelloModEnvTarget t)
{
    static const char *n[MELLO_MENV_TARGET_COUNT] = {"CUTOFF", "PITCH", "OSC 2", "FM", "NOISE", "RES"};
    return (unsigned)t < MELLO_MENV_TARGET_COUNT ? n[t] : "?";
}

static const char *filter_mode_name(MelloFilterMode m)
{
    static const char *n[MELLO_FILTER_MODE_COUNT] = {"LP", "HP", "BP", "NOTCH"};
    return (unsigned)m < MELLO_FILTER_MODE_COUNT ? n[m] : "?";
}

static const char *delay_div_name(uint8_t d)
{
    switch (d) {
    case 1u: return "16N";
    case 2u: return "4N";
    case 3u: return "8T";
    default: return "8N";
    }
}

static void set_selected_step_from_note(MelloUi *ui, uint8_t note_index)
{
    int8_t w = mello_ui_white_key_position(note_index);
    uint8_t idx;
    MelloEngine *e = eng(ui);
    if (w < 0 || !e) return;
    idx = (uint8_t)(ui->step_bank * 16u + (uint8_t)w);
    if (idx >= MELLO_MAX_STEPS) return;
    ui->selected_step = idx;
    if (idx >= e->project.seq_len) {
        uint8_t i, wanted = (uint8_t)(idx + 1u), was = e->project.seq_len;
        uint8_t count = (uint8_t)(sizeof(sequence_lengths) / sizeof(sequence_lengths[0]));
        for (i = 0u; i < count && sequence_lengths[i] < wanted; ++i) {
        }
        e->project.seq_len = i < count ? sequence_lengths[i] : sequence_lengths[count - 1u];
        mello_segment_rescale(&e->project, was);
        rebuild(ui);
    }
    mark_edit(ui);
}

static uint8_t scale_len(MelloScale s)
{
    switch (s) {
    case MELLO_SCALE_CHROMATIC: return 12u;
    case MELLO_SCALE_PENTATONIC:
    case MELLO_SCALE_MINOR_PENTATONIC: return 5u;
    case MELLO_SCALE_DIMINISHED: return 8u;
    case MELLO_SCALE_WHOLE_TONE: return 6u;
    case MELLO_SCALE_AUGMENTED: return 6u;
    default: return 7u;
    }
}

static void set_pitch_from_key(MelloUi *ui, uint8_t note_index)
{
    MelloEngine *e = eng(ui);
    MelloStep *st;
    uint8_t max_deg, d, best = 0u;
    int32_t want, best_diff = 10000;
    if (!e || note_index >= 27u) return;
    st = &e->project.steps[step_current(ui)];
    max_deg = (uint8_t)(e->project.pitch_span_oct * scale_len(e->project.scale));
    if (max_deg > 36u) max_deg = 36u;
    want = 53 + (int32_t)note_index;
    for (d = 0u; d < max_deg; ++d) {
        MelloStep temp = *st;
        int32_t got, diff;
        temp.pitch = (int8_t)d;
        got = (int32_t)mello_resolve_step_midi(&e->project, &temp);
        diff = got - want;
        if (diff < 0) diff = -diff;
        if (diff < best_diff) {
            best_diff = diff;
            best = d;
        }
    }
    st->pitch = (int8_t)best;
    rebuild(ui);
    mark_edit(ui);
}

static int8_t black_key_position(uint8_t note_index)
{
    uint8_t i;
    int8_t pos = 0;
    for (i = 0u; i < 27u; ++i) {
        if (mello_ui_white_key_position(i) >= 0) continue;
        if (i == note_index) return pos;
        ++pos;
    }
    return -1;
}

typedef enum {
    MELLO_CTX_NONE = 0,
    MELLO_CTX_OPEN_RANDOM,
    MELLO_CTX_STEP_SKIP,
    MELLO_CTX_LOOP_RESET,
    MELLO_CTX_RANDOM_REROLL,
    MELLO_CTX_RANDOM_CLEAR,
    MELLO_CTX_RANDOM_NONE,
    MELLO_CTX_RANDOM_ALL,
    MELLO_CTX_DOT_REROLL,
    MELLO_CTX_DOT_RESET,
    MELLO_CTX_DOT_FOLLOW,  /* the matrix reads only at step starts (was a rotary) */
    MELLO_CTX_DOT_REVERSE, /* the route backwards */
    MELLO_CTX_TURING_TOGGLE,
    MELLO_CTX_TURING_REROLL,
    MELLO_CTX_TURING_LOOP, /* mutate at the loop's one, or on every step */
    MELLO_CTX_SUB_TOGGLE,
    MELLO_CTX_SUB_STEP,
    MELLO_CTX_CHORD_TOGGLE,
    MELLO_CTX_GLOBAL_TAP,
    MELLO_CTX_GLOBAL_RESET,
    MELLO_CTX_SOUND_INIT, /* the sound back to the defaults (press twice; UNDO undoes it) */
    MELLO_CTX_SEG_A,      /* SEGMENT: B0-B5 pick segment A-F (keep the six in order) */
    MELLO_CTX_SEG_B,
    MELLO_CTX_SEG_C,
    MELLO_CTX_SEG_D,
    MELLO_CTX_SEG_E,
    MELLO_CTX_SEG_F,
    MELLO_CTX_SEG_EVEN, /* the segments as near equal as they go */
    MELLO_CTX_DRUM_KICK, /* DRUMS: B0-B3 pick the voice (keep the four in order) */
    MELLO_CTX_DRUM_SNARE,
    MELLO_CTX_DRUM_HAT,
    MELLO_CTX_DRUM_GLITCH,
    MELLO_CTX_DRUM_ON,
    MELLO_CTX_DRUM_ACCENT, /* the selected step's accent */
    MELLO_CTX_DRUM_ROLL,   /* roll the drums (undoable) */
    MELLO_CTX_DRUM_FEED,   /* RNDMZR rolls the drums too */
    MELLO_CTX_SAMPLE_TOGGLE,
    MELLO_CTX_HOME_RANDOM, /* reroll now, at the intensity ALGORITHM sets */
    MELLO_CTX_RNDMZR,      /* every page's B10, the last: tap = reroll (undoable), hold = the RANDOM page */
    MELLO_CTX_UNDO,        /* the pattern before the last RANDOM/CLEAR; again = redo */
    MELLO_CTX_MUTATE,      /* a LOW reroll of the pitches only */
    MELLO_CTX_TRANSPOSE_DOWN,
    MELLO_CTX_TRANSPOSE_UP,
    MELLO_CTX_PERF_FILL, /* HOME's performance keys: FILL .. REPEAT while held (perf_poll), RESTART on the press */
    MELLO_CTX_PERF_SUB_MUTE,
    MELLO_CTX_PERF_REVERSE,
    MELLO_CTX_PERF_REPEAT,
    MELLO_CTX_PERF_RESTART
} MelloContextVerb;

typedef struct {
    MelloContextVerb verb;
    const char *label;
} MelloContextAction;

/* WP-C5 authoritative contextual-action mapping. The physical B0-B10 slot,
 * rendered label and handler verb are defined together here. */
static const MelloContextAction context_actions[MELLO_UI_DRUMS + 1u][11] = {
    /* HOME (UI_ICEBERG.md): transpose and MUTATE; FILL, SUB MUTE; REVERSE, REPEAT, RESTART; CLR2, UNDO; RNDMZR.
     * Short labels so all eleven fit; the strip says a held key's full name (draw_control_hint). */
    [MELLO_UI_HOME] = {[0] = {MELLO_CTX_TRANSPOSE_DOWN, "-"},
                       [1] = {MELLO_CTX_TRANSPOSE_UP, "+"},
                       [2] = {MELLO_CTX_MUTATE, "MUT"},
                       [3] = {MELLO_CTX_PERF_FILL, "FL"},
                       [4] = {MELLO_CTX_PERF_SUB_MUTE, "SB"},
                       [5] = {MELLO_CTX_PERF_REVERSE, "<<"},
                       [6] = {MELLO_CTX_PERF_REPEAT, "RP"},
                       [7] = {MELLO_CTX_PERF_RESTART, "|<"},
                       [8] = {MELLO_CTX_RANDOM_CLEAR, "CL"},
                       [9] = {MELLO_CTX_UNDO, "UNDO"},
                       [10] = {MELLO_CTX_RNDMZR, "RNDMZR"}},
    [MELLO_UI_SEQ_STEP] = {[4] = {MELLO_CTX_STEP_SKIP, "SKIP"},
                           [10] = {MELLO_CTX_RNDMZR, "RNDMZR"},
                           [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_SEQ_SEGMENT] = {[0] = {MELLO_CTX_SEG_A, "A"},
                              [1] = {MELLO_CTX_SEG_B, "B"},
                              [2] = {MELLO_CTX_SEG_C, "C"},
                              [3] = {MELLO_CTX_SEG_D, "D"},
                              [4] = {MELLO_CTX_SEG_E, "E"},
                              [5] = {MELLO_CTX_SEG_F, "F"},
                              [7] = {MELLO_CTX_SEG_EVEN, "EVEN"},
                              [10] = {MELLO_CTX_RNDMZR, "RNDMZR"},
                              [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_SEQ_LOOP] = {[2] = {MELLO_CTX_LOOP_RESET, "RST"},
                           [10] = {MELLO_CTX_RNDMZR, "RNDMZR"},
                           [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_SEQ_SLOTSEQ] = {[10] = {MELLO_CTX_RNDMZR, "RNDMZR"}, [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_RANDOM] = {[1] = {MELLO_CTX_RANDOM_NONE, "NONE"},
                         [2] = {MELLO_CTX_RANDOM_ALL, "ALL"},
                         [3] = {MELLO_CTX_RANDOM_CLEAR, "CLR2"},
                         [10] = {MELLO_CTX_RNDMZR, "RNDMZR"},
                         [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_KEY_SCALE] = {[10] = {MELLO_CTX_RNDMZR, "RNDMZR"}, [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_SLOTS] = {[10] = {MELLO_CTX_RNDMZR, "RNDMZR"}, [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_DOT] = {[1] = {MELLO_CTX_DOT_REROLL, "RND"},
                      [2] = {MELLO_CTX_DOT_RESET, "RST"},
                      [3] = {MELLO_CTX_DOT_FOLLOW, "FLW"},
                      [4] = {MELLO_CTX_DOT_REVERSE, "REV"},
                      [10] = {MELLO_CTX_RNDMZR, "RNDMZR"},
                      [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_TURING] = {[0] = {MELLO_CTX_TURING_TOGGLE, "ON"},
                         [1] = {MELLO_CTX_TURING_REROLL, "RND"},
                         [2] = {MELLO_CTX_TURING_LOOP, "LOOP"},
                         [10] = {MELLO_CTX_RNDMZR, "RNDMZR"},
                         [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_SUB] = {[0] = {MELLO_CTX_SUB_TOGGLE, "ON"},
                      [4] = {MELLO_CTX_SUB_STEP, "STEP"},
                      [10] = {MELLO_CTX_RNDMZR, "RNDMZR"},
                      [9] = {MELLO_CTX_UNDO, "UNDO"}},
#if MELLO_FEATURE_CHORD_MACHINE
    [MELLO_UI_CHORD] = {[0] = {MELLO_CTX_CHORD_TOGGLE, "ON"},
                        [10] = {MELLO_CTX_RNDMZR, "RNDMZR"},
                        [9] = {MELLO_CTX_UNDO, "UNDO"}},
#endif
    [MELLO_UI_GLOBAL] = {[1] = {MELLO_CTX_GLOBAL_TAP, "TAP"},
                         [3] = {MELLO_CTX_GLOBAL_RESET, "RST2"},
                         [10] = {MELLO_CTX_RNDMZR, "RNDMZR"},
                         [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_SOUND_RESERVED] = {[0] = {MELLO_CTX_SOUND_INIT, "INIT"},
                                 [10] = {MELLO_CTX_RNDMZR, "RNDMZR"},
                                 [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_AUDIO_FX] = {[10] = {MELLO_CTX_RNDMZR, "RNDMZR"}, [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_DRUMS] = {[0] = {MELLO_CTX_DRUM_KICK, "K"},
                        [1] = {MELLO_CTX_DRUM_SNARE, "S"},
                        [2] = {MELLO_CTX_DRUM_HAT, "H"},
                        [3] = {MELLO_CTX_DRUM_GLITCH, "G"},
                        [4] = {MELLO_CTX_DRUM_ON, "ON"},
                        [5] = {MELLO_CTX_DRUM_ACCENT, "ACC"},
                        [6] = {MELLO_CTX_DRUM_ROLL, "ROLL"},
                        [7] = {MELLO_CTX_DRUM_FEED, "FEED"},
                        [10] = {MELLO_CTX_RNDMZR, "RNDMZR"},
                        [9] = {MELLO_CTX_UNDO, "UNDO"}},
    [MELLO_UI_SAMPLE_RESERVED] = {[0] = {MELLO_CTX_SAMPLE_TOGGLE, "ON"},
                                  [10] = {MELLO_CTX_RNDMZR, "RNDMZR"},
                                  [9] = {MELLO_CTX_UNDO, "UNDO"}}};

static const MelloContextAction *context_action(MelloUiPage page, uint8_t slot)
{
    if ((uint32_t)page > (uint32_t)MELLO_UI_DRUMS || slot >= 11u) return 0;
    return &context_actions[(uint32_t)page][slot];
}

static void handle_context_action(MelloUi *ui, uint32_t now_ms, uint8_t action);

/* The lead, played from the keys (KEYS mode, stopped): straight to the sound engine through the transport's sink. */
static void audition(MelloUi *ui, uint8_t note, bool on, uint32_t now_us)
{
    MelloEvent ev;
    MelloTransport *t = ui->transport;
    if (!t || !t->event_sink) return;
    memset(&ev, 0, sizeof ev);
    ev.type = on ? MELLO_EVENT_NOTE_ON : MELLO_EVENT_NOTE_OFF;
    ev.note = note;
    ev.velocity = on ? 100u : 0u;
    ev.step_index = 0xFFu; /* not a step: pairs this ON with its OFF only */
    ev.voice = MELLO_VOICE_LEAD;
    t->event_sink(t->event_sink_ctx, &ev, now_us);
}

/* The step (scale degree and octave, -2..+2) nearest to a MIDI note in the current key: step-record's inverse of
 * mello_resolve_step_midi, so a note lands where it was played, in any octave the step can reach. */
static void step_for_midi(MelloEngine *e, uint8_t midi, MelloStep *out)
{
    MelloStep probe;
    int32_t best_d = 1000;
    uint8_t maxp = (uint8_t)(e->project.pitch_span_oct * scale_len(e->project.scale)), d;
    int8_t oct;
    memset(&probe, 0, sizeof probe);
    out->pitch = 0;
    out->octave = 0;
    for (oct = -2; oct <= 2; ++oct)
        for (d = 0u; d < maxp; ++d) {
            int32_t diff;
            probe.pitch = (int8_t)d;
            probe.octave = oct;
            diff = (int32_t)mello_resolve_step_midi(&e->project, &probe) - (int32_t)midi;
            if (diff < 0) diff = -diff;
            if (diff < best_d ||
                (diff == best_d && (oct < 0 ? -oct : oct) < (out->octave < 0 ? -out->octave : out->octave))) {
                best_d = diff;
                out->pitch = (int8_t)d;
                out->octave = oct;
            }
        }
}

/* A key in KEYS mode. Note keys run F3 (MIDI 53) .. G5. */
static void keys_note(MelloUi *ui, uint8_t note_index, uint32_t now_us)
{
    MelloEngine *e = eng(ui);
    int32_t m = 53 + (int32_t)note_index + 12 * ui->keys_octave; /* OCT-/OCT+ move the keyboard */
    uint8_t midi = (uint8_t)(m < 12 ? 12 : m > 108 ? 108 : m);
    if (!e) return;
    if (ui->transport->running && !ui->keys_rec) {
        e->project.root_note = (uint8_t)(midi < 24u   ? 24u
                                         : midi > 83u ? 83u
                                                      : midi); /* the key is the new root */
        rebuild(ui);
        mark_edit(ui);
        ui->dirty |= MELLO_UI_DIRTY_HEADER;
        return;
    }
    if (ui->keys_rec && ui->keys_rec_pos < e->project.seq_len) {
        MelloStep *s = &e->project.steps[ui->keys_rec_pos], found;
        step_for_midi(e, midi, &found);
        s->pitch = found.pitch;
        s->octave = found.octave;
        s->skip = false;
        ui->selected_step = ui->keys_rec_pos;
        ui->keys_rec_pos = (uint8_t)((ui->keys_rec_pos + 1u) % e->project.seq_len);
        rebuild(ui);
        mark_edit(ui);
    }
    if (!ui->transport->running) {
        if (ui->audition_key != 0xFFu) audition(ui, ui->audition_note, false, now_us);
        ui->audition_key = note_index;
        ui->audition_note = midi;
        audition(ui, midi, true, now_us);
    }
}

static void handle_note_press(MelloUi *ui, uint32_t now_us, uint32_t now_ms, uint32_t held_buttons,
                              uint8_t note_index)
{
    MelloEngine *e = eng(ui);
    int8_t w;
    if (!e) return;

    if (ui->page == MELLO_UI_SEQ_STEP && (held_buttons & bit_u32(MELLO_UI_BTN_SEL))) {
        set_pitch_from_key(ui, note_index);
        return;
    }

    if (ui->page == MELLO_UI_HOME && ui->keys_mode) {
        keys_note(ui, note_index, now_us);
        return;
    }
    if (ui->page == MELLO_UI_HOME && ui->held_step != MELLO_UI_NO_STEP) {
        int8_t b = black_key_position(note_index); /* a held step: the first black keys are its flags */
        if (b >= 0 && b < 4) {
            MelloStep *st = &e->project.steps[ui->held_step];
            if (b == 0) st->skip = !st->skip;
            if (b == 1) st->accent = !st->accent;
            if (b == 2) st->slide = !st->slide;
            if (b == 3) st->sub = !st->sub;
            ui->held_step_edited = true;
            rebuild(ui);
            mark_edit(ui);
            return;
        }
    }
    {
        int8_t b = black_key_position(note_index);
        if (b >= 0) {
            const MelloContextAction *ca = context_action(ui->page, (uint8_t)b);
            if (ca && ca->verb == MELLO_CTX_RNDMZR) ui->rndmzr_key = note_index; /* to see it let go */
            handle_context_action(ui, now_ms, (uint8_t)b);
            return;
        }
    }

    w = mello_ui_white_key_position(note_index);
    if (w < 0) return;
    if (ui->page == MELLO_UI_HOME &&
        (ui->held_black & (1u << 6))) { /* REPEAT held: a white key is its length */
        ui->perf_repeat_len = (uint8_t)(w + 1);
        ui->dirty |= MELLO_UI_DIRTY_FOOTER;
        return;
    }

    switch (ui->page) {
    case MELLO_UI_HOME: {
        /* A step key: hold it to edit the step with K1-K4; let go without turning to switch the step on/off.
         * Keys past the sequence's length do nothing (they no longer stretch it unseen). */
        uint8_t idx = (uint8_t)(ui->step_bank * 16u + (uint8_t)w);
        if (idx >= e->project.seq_len) break;
        ui->selected_step = idx;
        ui->held_step = idx;
        ui->held_key = note_index;
        ui->held_step_edited = false;
        if (!ui->transport->running) { /* stopped: hear the step while it is held */
            uint8_t midi = mello_resolve_step_midi(&e->project, &e->project.steps[idx]);
            if (ui->audition_key != 0xFFu) audition(ui, ui->audition_note, false, now_us);
            ui->audition_key = note_index;
            ui->audition_note = midi;
            audition(ui, midi, true, now_us);
        }
        ui->dirty |= MELLO_UI_DIRTY_FOOTER | MELLO_UI_DIRTY_LEDS | MELLO_UI_DIRTY_BODY;
        break;
    }
    case MELLO_UI_SEQ_STEP: set_selected_step_from_note(ui, note_index); break;
    case MELLO_UI_SEQ_SEGMENT: {
        /* A step key cuts: a segment starts at that step, or, where one starts already, it joins the one before.
         * Step 1, and a seventh segment, only pick the segment the step is in. */
        uint8_t step = (uint8_t)(ui->step_bank * 16u + (uint8_t)w), b[MELLO_MAX_SEGMENTS + 1u], sc, j;
        int8_t r;
        if (step >= e->project.seq_len) break;
        r = mello_segment_toggle_start(&e->project, step);
        if (r < 0) {
            sc = mello_segment_bounds(&e->project, b);
            for (j = 0u; j + 1u < sc && b[j + 1u] <= step; ++j) {}
            r = (int8_t)j;
        } else
            rebuild(ui);
        ui->selected_segment = (uint8_t)r;
        ui->dirty |= MELLO_UI_DIRTY_HEADER; /* the segment's letter and colour */
        mark_edit(ui);
        break;
    }
    case MELLO_UI_SEQ_LOOP:
        if ((uint8_t)w < MELLO_MAX_LOOP_CYCLE_STEPS) {
            ui->selected_loop_step = (uint8_t)w;
            if (!e->loop_cycle.active) mello_engine_loop_cycle_select(e, (uint8_t)w);
            mark_edit(ui);
        }
        break;
    case MELLO_UI_SEQ_SLOTSEQ: {
        uint8_t pos = (uint8_t)(ui->slot_seq_bank * 16u + (uint8_t)w);
        if (pos < MELLO_MAX_SLOT_SEQ_STEPS) {
            ui->selected_slot_seq_step = pos;
            mark_edit(ui);
        }
        break;
    }
    case MELLO_UI_RANDOM:
        if ((uint8_t)w < random_target_count() && ui->random_edit) { /* EDIT: pick the target K1 sets */
            ui->random_sel = (uint8_t)w;
            mark_edit(ui);
        } else if ((uint8_t)w < random_target_count()) {
            uint16_t bit = random_target_bit((uint8_t)w);
            if (!ui->random_links)
                e->randomize.mask ^= bit;
            else if (bit &
                     MELLO_RAND_LINKABLE) { /* LINKS: free -> + (follows pitch) -> - (mirrors it) -> free */
                MelloModRouting m = e->modr;
                if (m.rnd_mirror & bit)
                    m.rnd_mirror &= (uint16_t)~bit;
                else if (m.rnd_follow & bit)
                    m.rnd_follow &= (uint16_t)~bit, m.rnd_mirror |= bit;
                else
                    m.rnd_follow |= bit;
                if ((m.rnd_follow | m.rnd_mirror) & bit)
                    e->randomize.mask |= bit; /* a link rolls its target */
                mello_engine_mod_routing_set(e, &m);
            }
            mark_edit(ui);
        }
        break;
    case MELLO_UI_SLOTS: {
        uint8_t slot = (uint8_t)(ui->slot_bank * 16u + (uint8_t)w);
        if (slot < e->visible_slot_count && slot < MELLO_MAX_SLOTS) {
            ui->selected_slot = slot;
            if (mello_engine_slot_valid(e, slot)) (void)mello_engine_request_slot(e, slot);
            mark_edit(ui);
        }
        break;
    }
    case MELLO_UI_DOT:
        if ((uint8_t)w < MELLO_DOT_CELL_COUNT) {
            MelloDotTarget t = (MelloDotTarget)ui->selected_dot_target;
            uint16_t cells = e->dot[t].cells ^ (uint16_t)(1u << (uint8_t)w);
            mello_engine_dot_set_cells(e, t, cells);
            mark_edit(ui);
        }
        break;
    case MELLO_UI_TURING: {
        uint8_t idx = (uint8_t)(ui->tm_bank * 16u + (uint8_t)w);
        if (idx < MELLO_TM_MAX_STEPS) {
            e->tm_bool_reg[idx] = e->tm_bool_reg[idx] ? 0u : 1u;
            if (idx >= e->tm_bool_len) e->tm_bool_len = (uint8_t)(idx + 1u);
            ui->selected_tm_bit = idx;
            mark_edit(ui);
        }
        break;
    }
    case MELLO_UI_SUB:
        if ((uint8_t)w < MELLO_SUB_X_MAX_STEPS) {
            ui->selected_sub_x_step = (uint8_t)w;
            mark_edit(ui);
        }
        break;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD:
        if (ui->chord_subpage == 3u && (uint8_t)w < 16u) {
            MelloChordConfig c = e->chord;
            c.grid_cells ^= (uint16_t)(1u << (uint8_t)w);
            mello_engine_chord_set_config(e, &c);
            mark_edit(ui);
        } else if (ui->chord_subpage == 4u && (uint8_t)w < e->chord.pattern_length) {
            ui->selected_chord_pattern = (uint8_t)w;
            mark_edit(ui);
        } else if ((uint8_t)w < e->chord.length) {
            ui->selected_chord_step = (uint8_t)w;
            mark_edit(ui);
        }
        break;
#endif
    case MELLO_UI_DRUMS:
        if ((uint8_t)w < MELLO_DRUM_STEPS) { /* a step key: the voice's hit there on or off, and that step picked */
            MelloDrumConfig d = e->drums;
            d.hits[ui->selected_drum_voice % MELLO_DRUM_VOICES] ^= (uint16_t)(1u << (uint8_t)w);
            ui->selected_drum_step = (uint8_t)w;
            mello_engine_drums_set_config(e, &d);
            mark_edit(ui);
        }
        break;
    case MELLO_UI_SAMPLE_RESERVED:
        if ((uint8_t)w < MELLO_SAMPLE_SLOT_COUNT) {
            MelloSampleConfig c = e->sample;
            c.edit_slot = (uint8_t)w;
            mello_engine_sample_set_config(e, &c);
            mark_edit(ui);
        }
        break;
    default: break;
    }

    if (ui->page == MELLO_UI_SEQ_SLOTSEQ || ui->page == MELLO_UI_SLOTS) persistent_dirty(ui, now_ms);
}

/* HOME's K1-K4 with no step held: CUTOFF, DECAY (release follows in proportion), SPACE (reverb wet, delay at half:
 * the defaults 24/12 sit on that line), DRIVE. Live while playing; each detent counts. */
static void adjust_home_macro(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    if (knob == 0u || knob == 1u) {
        MelloSoundConfig c = e->sound;
        if (knob == 0u)
            c.filter_cut = (uint16_t)clamp_i32((int32_t)c.filter_cut + delta * 10, 0, 1000);
        else {
            int32_t d = c.decay_ms, step = d / 20 > 5 ? d / 20 : 5, nd = clamp_i32(d + delta * step, 5, 1000);
            c.release_ms = (uint16_t)clamp_i32((int32_t)c.release_ms * nd / (d ? d : 1), 5, 2000);
            c.decay_ms = (uint16_t)nd;
        }
        mello_engine_sound_set_config(e, &c);
    } else {
        MelloFxConfig f = e->fx;
        if (knob == 2u) {
            f.reverb_wet = (uint16_t)clamp_i32((int32_t)f.reverb_wet + delta * 10, 0, 1000);
            f.delay_wet = (uint16_t)(f.reverb_wet / 2u);
        } else
            f.dist_amount = (uint16_t)clamp_i32((int32_t)f.dist_amount + delta * 10, 0, 1000);
        mello_engine_fx_set_config(e, &f);
    }
    mark_edit(ui);
}

static void adjust_home(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloStep *s;
    if (!e || !delta) return;
    if (ui->held_step == MELLO_UI_NO_STEP) {
        adjust_home_macro(ui, knob, delta);
        return;
    }
    ui->held_step_edited = true;
    if (ui->selected_step >= MELLO_MAX_STEPS) ui->selected_step = 0u;
    s = &e->project.steps[ui->selected_step];
    if (knob == 0u) {
        uint8_t maxp = (uint8_t)(e->project.pitch_span_oct * scale_len(e->project.scale) - 1u);
        s->pitch = (int8_t)clamp_i32((int32_t)s->pitch + delta, 0, maxp);
    } else if (knob == 1u)
        s->pulses = (uint8_t)clamp_i32((int32_t)s->pulses + delta, 1, 8);
    else if (knob == 2u)
        s->ratchet = (uint8_t)clamp_i32((int32_t)s->ratchet + delta, 1, 8);
    else if (knob == 3u)
        s->gate = (MelloGateMode)wrap_u8((int32_t)s->gate + (delta > 0 ? 1 : -1), 4u);
    rebuild(ui);
    mark_edit(ui);
}

static void adjust_step(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloStep *s;
    int32_t v;
    if (!e || !delta) return;
    s = &e->project.steps[step_current(ui)];
    switch (ui->step_subpage) {
    case 0u: /* TRIG */
        if (knob == 0u) {
            uint8_t maxp = (uint8_t)(e->project.pitch_span_oct * scale_len(e->project.scale) - 1u);
            s->pitch = (int8_t)clamp_i32((int32_t)s->pitch + delta, 0, maxp);
        } else if (knob == 1u)
            s->pulses = (uint8_t)clamp_i32((int32_t)s->pulses + delta, 1, 8);
        else if (knob == 2u)
            s->ratchet = (uint8_t)clamp_i32((int32_t)s->ratchet + delta, 1, 8);
        else if (knob == 3u)
            s->gate = (MelloGateMode)wrap_u8((int32_t)s->gate + (delta > 0 ? 1 : -1), 4u);
        break;
    case 1u: /* NOTE */
        if (knob == 0u)
            s->octave = (int8_t)clamp_i32((int32_t)s->octave + delta, -2, 2);
        else if (knob == 1u) {
            int32_t p = (int32_t)prob_pct(s->probability);
            p = clamp_i32(p + delta * 5, 0, 100);
            p -= p % 5;
            s->probability = pct_prob((uint8_t)p);
        } else if (knob == 2u)
            s->sample_slot = (uint8_t)clamp_i32((int32_t)s->sample_slot + (delta > 0 ? 1 : -1), 0, 4);
        else if (knob == 3u)
            s->accent = delta > 0;
        break;
    case 2u: /* FLAGS */
        if (knob == 0u)
            s->accent = delta > 0;
        else if (knob == 1u)
            s->sub = delta > 0;
        else if (knob == 2u)
            s->slide = delta > 0;
        else if (knob == 3u)
            s->skip = delta > 0;
        break;
    default: /* SEQ */
        if (knob == 0u) {
            uint8_t i, was = e->project.seq_len;
            uint8_t count = (uint8_t)(sizeof(sequence_lengths) / sizeof(sequence_lengths[0]));
            if (delta > 0) {
                for (i = 0u; i < count && sequence_lengths[i] <= e->project.seq_len; ++i) {
                }
                e->project.seq_len = i < count ? sequence_lengths[i] : sequence_lengths[count - 1u];
            } else {
                i = count;
                while (i > 0u && sequence_lengths[i - 1u] >= e->project.seq_len) --i;
                e->project.seq_len = i > 0u ? sequence_lengths[i - 1u] : sequence_lengths[0];
            }
            mello_segment_rescale(&e->project, was); /* the segments keep their proportions */
        } else if (knob == 1u) {
            int32_t idx = option_index_u16(master_lengths, 6u, e->project.master_len);
            idx = clamp_i32(idx + (delta > 0 ? 1 : -1), 0, 5);
            e->project.master_len = master_lengths[idx];
        } else if (knob == 2u) {
            uint8_t div = e->project.clock_div;
            if (delta > 0)
                div = div == 1u ? 2u : 4u;
            else
                div = div == 4u ? 2u : 1u;
            e->project.clock_div = div;
        } else if (knob == 3u) {
            e->project.pitch_span_oct = (uint8_t)clamp_i32((int32_t)e->project.pitch_span_oct + delta, 1, 3);
        }
        break;
    }
    rebuild(ui);
    mark_edit(ui);
    (void)v;
}

/* A segment's order: FWD, REV, P-P, RND (the HTML's DIR and MODE in one; P-P backwards still plays, shown "P-P R"). */
static uint8_t segment_order(const MelloProject *p, uint8_t s)
{
    if (p->seg_mode[s] == MELLO_SEG_RANDOM) return 3u;
    if (p->seg_mode[s] == MELLO_SEG_PINGPONG) return 2u;
    return p->seg_direction[s] == MELLO_SEG_REV ? 1u : 0u;
}

static void adjust_segment(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    uint8_t s, b[MELLO_MAX_SEGMENTS + 1u], sc;
    if (!e || !delta) return;
    sc = mello_segment_bounds(&e->project, b);
    s = ui->selected_segment;
    if (s >= sc) s = (uint8_t)(sc - 1u);
    if (s != ui->selected_segment) ui->dirty |= MELLO_UI_DIRTY_HEADER;
    ui->selected_segment = s;
    if (knob == 0u)
        e->project.seg_repeats[s] = (uint8_t)clamp_i32((int32_t)e->project.seg_repeats[s] + delta, 1, 4);
    else if (knob == 1u) {
        uint8_t o = (uint8_t)clamp_i32((int32_t)segment_order(&e->project, s) + (delta > 0 ? 1 : -1), 0, 3);
        e->project.seg_mode[s] = o == 3u ? MELLO_SEG_RANDOM : o == 2u ? MELLO_SEG_PINGPONG : MELLO_SEG_LINEAR;
        e->project.seg_direction[s] = o == 1u ? MELLO_SEG_REV : MELLO_SEG_FWD;
    } else if (knob == 2u)
        e->project.seg_skip[s] = delta > 0;
    else if (knob == 3u)
        mello_segment_set_length(&e->project, s,
                                 (uint8_t)clamp_i32((int32_t)(b[s + 1u] - b[s]) + delta, 1, MELLO_MAX_STEPS));
    rebuild(ui);
    mark_edit(ui);
}

static void adjust_loop(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloLoopCycleStep st;
    uint8_t i;
    if (!e || !delta) return;
    i = ui->selected_loop_step;
    if (i >= MELLO_MAX_LOOP_CYCLE_STEPS) i = 0u;
    st = e->loop_cycle.steps[i];
    if (ui->loop_subpage == 0u) {
        if (knob == 0u) {
            st.hold = (uint16_t)clamp_i32((int32_t)st.hold + delta, 1, 128);
        } else if (knob == 1u)
            st.root_offset = (int8_t)clamp_i32((int32_t)st.root_offset + delta, -12, 12);
        else if (knob == 2u)
            st.octave = (int8_t)clamp_i32((int32_t)st.octave + delta, -2, 2);
        else if (knob == 3u)
            st.pitch = (int8_t)clamp_i32((int32_t)st.pitch + delta, -12, 12);
        mello_engine_loop_cycle_set_step(e, i, &st);
    } else if (ui->loop_subpage == 1u) {
        uint8_t sub_tone = mello_loop_cycle_sub_tone(&st);
        int8_t sub_transpose = mello_loop_cycle_sub_transpose(&st);
        if (knob == 0u)
            st.root_note = (int8_t)clamp_i32((int32_t)st.root_note + (delta > 0 ? 1 : -1), -1, 11);
        else if (knob == 1u)
            st.scale_set = (int8_t)clamp_i32((int32_t)st.scale_set + (delta > 0 ? 1 : -1), -1,
                                             (int32_t)MELLO_SCALE_COUNT - 1);
        else if (knob == 2u)
            sub_tone = (uint8_t)wrap_u8((int32_t)sub_tone + (delta > 0 ? 1 : -1), 8u);
        else if (knob == 3u)
            sub_transpose = (int8_t)clamp_i32((int32_t)sub_transpose + delta, -12, 12);
        mello_loop_cycle_set_sub(&st, sub_tone, sub_transpose);
        mello_engine_loop_cycle_set_step(e, i, &st);
    } else {
        if (knob == 0u)
            mello_engine_loop_cycle_set_length(
                e, (uint8_t)clamp_i32((int32_t)e->loop_cycle.length + delta, 2, 8));
        else if (knob == 1u)
            mello_engine_loop_cycle_set_sync(e, delta > 0);
        else if (knob == 2u)
            mello_engine_loop_cycle_set_active(e, delta > 0);
        else if (knob == 3u)
            return;
    }
    mark_edit(ui);
}

static void adjust_slot_seq(MelloUi *ui, uint8_t knob, int16_t delta, uint32_t now_ms)
{
    MelloEngine *e = eng(ui);
    MelloSlotSeqStep *st;
    if (!e || !delta) return;
    st = &e->slot_seq.steps[ui->selected_slot_seq_step];
    if (knob == 0u) {
        int32_t v = (int32_t)st->slot + (delta > 0 ? 1 : -1);
        mello_engine_slot_seq_set_step(e, ui->selected_slot_seq_step, (int8_t)clamp_i32(v, -1, 63), st->hold);
    } else if (knob == 1u) {
        int32_t idx = option_index_u16(slot_holds, 7u, st->hold);
        idx = clamp_i32(idx + (delta > 0 ? 1 : -1), 0, 6);
        mello_engine_slot_seq_set_step(e, ui->selected_slot_seq_step, st->slot, slot_holds[idx]);
    } else if (knob == 2u)
        mello_engine_slot_seq_set_active(e, !e->slot_seq.active);
    else if (knob == 3u) {
        int32_t idx = option_index_u8(slot_counts, 6u, e->slot_seq.length);
        idx = clamp_i32(idx + (delta > 0 ? 1 : -1), 0, 5);
        mello_engine_slot_seq_set_length(e, slot_counts[idx]);
    }
    persistent_dirty(ui, now_ms);
    mark_edit(ui);
}

static void adjust_slots(MelloUi *ui, uint8_t knob, int16_t delta, uint32_t now_ms)
{
    MelloEngine *e = eng(ui);
    MelloPatternSlot *slot;
    if (!e || !delta) return;
    if (ui->slots_subpage == 0u) {
        if (knob == 1u) {
            int32_t idx = option_index_u8(slot_counts, 6u, e->visible_slot_count);
            idx = clamp_i32(idx + (delta > 0 ? 1 : -1), 0, 5);
            mello_engine_set_visible_slot_count(e, slot_counts[idx]);
            persistent_dirty(ui, now_ms);
        }
    } else {
        slot = &e->slots[ui->selected_slot];
        if (!slot->valid) return;
        if (knob == 0u)
            ui->slot_name_cursor =
                (uint8_t)clamp_i32((int32_t)ui->slot_name_cursor + delta, 0, MELLO_SLOT_NAME_CHARS - 1u);
        else if (knob == 1u) {
            char cur = slot->name[ui->slot_name_cursor];
            int32_t idx = 0, i;
            for (i = 0; name_chars[i]; ++i)
                if (name_chars[i] == cur) {
                    idx = i;
                    break;
                }
            idx += delta > 0 ? 1 : -1;
            if (idx < 0) idx = (int32_t)(sizeof(name_chars) - 1u) - 1;
            if (idx >= (int32_t)(sizeof(name_chars) - 1u)) idx = 0;
            slot->name[ui->slot_name_cursor] = name_chars[idx];
            slot->name[MELLO_SLOT_NAME_CHARS] = 0;
            persistent_dirty(ui, now_ms);
        }
    }
    mark_edit(ui);
}

static void adjust_dot(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloDotTarget t;
    MelloDotMatrixConfig c;
    if (!e || !delta) return;
    t = (MelloDotTarget)ui->selected_dot_target;
    c = e->dot[t];
    if (ui->dot_shape && dot_lane(t) >= 0) { /* SHAPE: where the lane goes, and how a lit cell moves it */
        MelloModRouting m = e->modr;
        uint8_t l = (uint8_t)dot_lane(t);
        if (knob == 0u) /* never OFF: DEPTH 0 is off */
            m.dot_target[l] = (uint8_t)(1u + wrap_u8((int32_t)m.dot_target[l] - 1 + (delta > 0 ? 1 : -1),
                                                     MELLO_DEST_COUNT - 1u));
        else if (knob == 1u)
            m.dot_attack_ms[l] = (uint16_t)clamp_i32((int32_t)m.dot_attack_ms[l] + delta * 5, 0, 2000);
        else if (knob == 2u)
            m.dot_release_ms[l] = (uint16_t)clamp_i32((int32_t)m.dot_release_ms[l] + delta * 10, 5, 4000);
        else
            m.dot_gate[l] = delta > 0;
        mello_engine_mod_routing_set(e, &m);
        mark_edit(ui);
        return;
    }
    if (knob == 0u)
        c.mode = (MelloDotMode)wrap_u8((int32_t)c.mode + (delta > 0 ? 1 : -1), MELLO_DOT_MODE_COUNT);
    else if (knob == 1u)
        c.length = (uint8_t)clamp_i32((int32_t)c.length + delta, 1, 16);
    else if (knob == 2u)
        c.amount = (uint16_t)clamp_i32((int32_t)c.amount + delta * 10, 0, 1000);
    else {
        MelloModRouting m = e->modr;
        m.dot_evolve[t] = (uint16_t)clamp_i32((int32_t)m.dot_evolve[t] + delta * 10, 0, 1000);
        mello_engine_mod_routing_set(e, &m);
    }
    mello_engine_dot_set_config(e, t, &c);
    mark_edit(ui);
}

static void adjust_turing(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloTmConfig c;
    if (!e || !delta) return;
    c = e->tm;
    if (ui->tm_subpage == 0u) {
        if (knob == 0u)
            c.mutation_prob = (uint16_t)clamp_i32((int32_t)c.mutation_prob + delta * 5, 0, 500);
        else if (knob == 1u) {
            int32_t idx =
                option_index_u8(tm_lengths, (uint8_t)(sizeof tm_lengths / sizeof tm_lengths[0]), c.length);
            idx = clamp_i32(idx + (delta > 0 ? 1 : -1), 0,
                            (int32_t)(sizeof tm_lengths / sizeof tm_lengths[0]) - 1);
            c.length = tm_lengths[idx];
        } else if (knob == 2u)
            c.rate = (MelloTmRate)wrap_u8((int32_t)c.rate + (delta > 0 ? 1 : -1), 3u);
        else if (knob == 3u)
            c.offset = (int8_t)clamp_i32((int32_t)c.offset + delta, -31, 31);
    } else if (ui->tm_subpage == 3u) { /* SOUND: two routes, destination and depth (+-) */
        MelloModRouting m = e->modr;
        uint8_t l = knob >> 1;
        if (!(knob & 1u))
            m.tm_dest[l] = wrap_u8((int32_t)m.tm_dest[l] + (delta > 0 ? 1 : -1), MELLO_DEST_COUNT);
        else
            m.tm_depth[l] = (int16_t)clamp_i32((int32_t)m.tm_depth[l] + delta * 10, -1000, 1000);
        mello_engine_mod_routing_set(e, &m);
        mark_edit(ui);
        return;
    } else if (ui->tm_subpage == 1u) {
        uint8_t mask = (uint8_t)(1u << knob);
        c.routes = delta > 0 ? (uint8_t)(c.routes | mask) : (uint8_t)(c.routes & (uint8_t)~mask);
    } else {
        uint16_t *d = knob == 0u   ? &c.depth_gate
                      : knob == 1u ? &c.depth_filter
                      : knob == 2u ? &c.depth_octave
                                   : &c.depth_decay;
        *d = (uint16_t)clamp_i32((int32_t)*d + delta * 10, 0, 1000);
    }
    mello_engine_tm_set_config(e, &c);
    mark_edit(ui);
}

static void adjust_sub(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloSubConfig c;
    MelloSubXStep *xs;
    if (!e || !delta) return;
    c = e->sub;
    xs = &c.x_steps[ui->selected_sub_x_step];
    if (ui->sub_subpage == 0u) {
        if (knob == 0u)
            c.mode = (MelloSubMode)wrap_u8((int32_t)c.mode + (delta > 0 ? 1 : -1), 5u);
        else if (knob == 1u)
            c.euclid_pulses = (uint8_t)clamp_i32((int32_t)c.euclid_pulses + delta, 1, 64);
        else if (knob == 2u)
            c.euclid_rotate = (uint8_t)clamp_i32((int32_t)c.euclid_rotate + delta, 0, 64);
        else if (knob == 3u)
            c.octave = (int8_t)clamp_i32((int32_t)c.octave + delta, -2, 1);
    } else if (ui->sub_subpage == 1u) {
        if (knob == 0u)
            c.x_on = delta > 0;
        else if (knob == 1u) {
            if (delta > 0)
                c.x_length = c.x_length == 2u ? 3u : 5u;
            else
                c.x_length = c.x_length == 5u ? 3u : 2u;
        } else if (knob == 2u)
            c.x_reset = delta > 0;
        else if (knob == 3u)
            c.gate_amount = (uint16_t)clamp_i32((int32_t)c.gate_amount + delta * 50, 150, 1200);
    } else if (ui->sub_subpage == 2u) {
        if (knob == 0u)
            xs->on = delta > 0;
        else if (knob == 1u)
            xs->tone = (uint8_t)wrap_u8((int32_t)xs->tone + (delta > 0 ? 1 : -1), 8u);
        else if (knob == 2u)
            xs->octave = (int8_t)clamp_i32((int32_t)xs->octave + delta, -2, 2);
        else if (knob == 3u)
            c.gate_amount = (uint16_t)clamp_i32((int32_t)c.gate_amount + delta * 50, 150, 1200);
    } else if (ui->sub_subpage == 3u) {
        if (knob == 0u)
            c.level = (uint16_t)clamp_i32((int32_t)c.level + delta * 10, 0, 1000);
        else if (knob == 1u)
            c.wave = step_sub_waveform(c.wave, delta);
        else if (knob == 2u)
            c.decay_ms = (uint16_t)clamp_i32((int32_t)c.decay_ms + delta * 10, 30, 700);
        else
            c.release_ms = (uint16_t)clamp_i32((int32_t)c.release_ms + delta * 10, 30, 1200);
    } else {
        if (knob == 0u)
            c.glide_ms = (uint16_t)clamp_i32((int32_t)c.glide_ms + delta * 5, 0, 200);
        else if (knob == 1u)
            c.cut = (uint16_t)clamp_i32((int32_t)c.cut + delta * 10, 0, 1000);
        else if (knob == 2u)
            c.drive = (uint16_t)clamp_i32((int32_t)c.drive + delta * 10, 0, 1000);
        else
            c.octave = (int8_t)clamp_i32((int32_t)c.octave + delta, -2, 1);
    }
    mello_engine_sub_set_config(e, &c);
    mark_edit(ui);
}

#if MELLO_FEATURE_CHORD_MACHINE
static const char *chord_shape_name(MelloChordShape s)
{
    switch (s) {
    case MELLO_CHORD_SEVENTH: return "7TH";
    case MELLO_CHORD_SUS2: return "SUS2";
    case MELLO_CHORD_SUS4: return "SUS4";
    case MELLO_CHORD_POWER: return "POWER";
    default: return "TRIAD";
    }
}
static const char *chord_mode_name(MelloChordMode m)
{
    switch (m) {
    case MELLO_CHORD_HOLD: return "HOLD";
    case MELLO_CHORD_ARP_UP: return "ARP UP";
    case MELLO_CHORD_ARP_DOWN: return "ARP DN";
    case MELLO_CHORD_ARP_UPDOWN: return "ARP UD";
    case MELLO_CHORD_PATTERN: return "PATTERN";
    default: return "STAB";
    }
}
/* CHORD's layers: PLAY, CHORD, VOICING, GRID (with TRIG GRID), PATTERN (with MODE PATTERN), TONE, FILTER, MOTION,
 * AMP. */
#define CHORD_LAYERS 9u
static bool chord_layer_shown(const MelloChordConfig *c, uint8_t l)
{
    return l == 3u ? c->trigger == MELLO_CHORD_TRIGGER_GRID : l == 4u ? c->mode == MELLO_CHORD_PATTERN : true;
}
static uint16_t chord_ms(uint16_t v, int16_t delta, uint16_t lo, uint16_t hi)
{
    int32_t step = v < 100u ? 2 : v < 500u ? 10 : 50; /* finer where short times differ most */
    return (uint16_t)clamp_i32((int32_t)v + delta * step, lo, hi);
}
static void adjust_chord(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloChordConfig c;
    MelloChordStep *s;
    if (!e || !delta) return;
    c = e->chord;
    s = &c.steps[ui->selected_chord_step % c.length];
    switch (ui->chord_subpage * 4u + knob) {
    case 0: c.mode = (MelloChordMode)wrap_u8((int32_t)c.mode + (delta > 0 ? 1 : -1), MELLO_CHORD_MODE_COUNT); break;
    case 1: c.trigger = (MelloChordTrigger)wrap_u8((int32_t)c.trigger + (delta > 0 ? 1 : -1), 2u); break;
    case 2: c.length = (uint8_t)clamp_i32((int32_t)c.length + delta, 1, MELLO_CHORD_MAX_STEPS); break;
    case 3: c.gate_amount = (uint16_t)clamp_i32((int32_t)c.gate_amount + delta * 10, 100, 1200); break;
    case 4: s->degree = (uint8_t)clamp_i32((int32_t)s->degree + delta, 0, 11); break;
    case 5:
        s->shape = (MelloChordShape)wrap_u8((int32_t)s->shape + (delta > 0 ? 1 : -1), MELLO_CHORD_SHAPE_COUNT);
        break;
    case 6: s->hold = (uint8_t)clamp_i32((int32_t)s->hold + delta, 1, 16); break;
    case 7: s->octave = (int8_t)clamp_i32((int32_t)s->octave + delta, -2, 2); break;
    case 8: s->inversion = (uint8_t)clamp_i32((int32_t)s->inversion + delta, 0, 3); break;
    case 9: s->spread = (uint8_t)clamp_i32((int32_t)s->spread + delta, 0, 3); break;
    case 10: c.voices = (uint8_t)clamp_i32((int32_t)c.voices + delta, 2, 4); break;
    case 11: c.voice_lead = delta > 0; break;
    case 12: c.grid_mode = (MelloDotMode)wrap_u8((int32_t)c.grid_mode + (delta > 0 ? 1 : -1), MELLO_DOT_MODE_COUNT); break;
    case 13: c.grid_length = (uint8_t)clamp_i32((int32_t)c.grid_length + delta, 1, 16); break;
    case 14: c.grid_amount = (uint16_t)clamp_i32((int32_t)c.grid_amount + delta * 10, 0, 1000); break;
    case 15: c.grid_follow = delta > 0; break;
    case 16: {
        uint8_t p = ui->selected_chord_pattern % c.pattern_length;
        c.pattern_masks[p] = (uint8_t)clamp_i32((int32_t)c.pattern_masks[p] + delta, 0, 15);
        break;
    }
    case 17:
        c.pattern_length = (uint8_t)clamp_i32((int32_t)c.pattern_length + delta, 1, MELLO_CHORD_PATTERN_STEPS);
        break;
    case 20: c.wave = step_sub_waveform(c.wave, delta); break;
    case 21: c.chorus = (uint16_t)clamp_i32((int32_t)c.chorus + delta * 10, 0, 1000); break;
    case 22: c.space = (uint16_t)clamp_i32((int32_t)c.space + delta * 10, 0, 1000); break;
    case 23: c.level = (uint16_t)clamp_i32((int32_t)c.level + delta * 10, 0, 1000); break;
    case 24: c.cut = (uint16_t)clamp_i32((int32_t)c.cut + delta * 10, 0, 1000); break;
    case 25: c.res = (uint16_t)clamp_i32((int32_t)c.res + delta * 10, 0, 1000); break;
    case 26: c.fenv = (uint16_t)clamp_i32((int32_t)c.fenv + delta * 10, 0, 1000); break;
    case 27: c.link = delta > 0; break;
    case 28: c.fenv_attack_ms = chord_ms(c.fenv_attack_ms, delta, 1u, 2000u); break;
    case 29: c.fenv_decay_ms = chord_ms(c.fenv_decay_ms, delta, 10u, 4000u); break;
    case 30: c.strum_ms = (uint8_t)clamp_i32((int32_t)c.strum_ms + delta, 0, 60); break;
    case 32: c.attack_ms = chord_ms(c.attack_ms, delta, 1u, 2000u); break;
    case 33: c.decay_ms = chord_ms(c.decay_ms, delta, 10u, 4000u); break;
    case 34: c.sustain = (uint16_t)clamp_i32((int32_t)c.sustain + delta * 10, 0, 1000); break;
    case 35: c.release_ms = chord_ms(c.release_ms, delta, 5u, 4000u); break;
    default: return;
    }
    mello_engine_chord_set_config(e, &c);
    mark_edit(ui);
}
#endif

/* DRUMS' layers: VOICE (the voice picked), STEP (its step picked), VARY, KIT. */
#define DRUM_LAYERS 4u
static void adjust_drums(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloDrumConfig d;
    MelloDrumVoice *v;
    MelloDrumLock *l;
    if (!e || !delta) return;
    d = e->drums;
    v = &d.voice[ui->selected_drum_voice % MELLO_DRUM_VOICES];
    l = &d.lock[ui->selected_drum_voice % MELLO_DRUM_VOICES][ui->selected_drum_step % MELLO_DRUM_STEPS];
    switch ((ui->drum_subpage % DRUM_LAYERS) * 4u + knob) {
    case 0: v->tune = (uint16_t)clamp_i32((int32_t)v->tune + delta * 10, 0, 1000); break;
    case 1: v->decay = (uint16_t)clamp_i32((int32_t)v->decay + delta * 10, 0, 1000); break;
    case 2: v->tone = (uint16_t)clamp_i32((int32_t)v->tone + delta * 10, 0, 1000); break;
    case 3: v->level = (uint16_t)clamp_i32((int32_t)v->level + delta * 10, 0, 1000); break;
    case 4: l->pitch = (int8_t)clamp_i32((int32_t)l->pitch + delta, -24, 24); break;
    case 5: l->decay = (int8_t)clamp_i32((int32_t)l->decay + delta * 5, -100, 100); break;
    case 6: l->crush = (uint8_t)clamp_i32((int32_t)l->crush + delta * 5, 0, 100); break;
    case 7: l->ratchet = (uint8_t)clamp_i32((int32_t)l->ratchet + delta, 1, 4); break;
    case 8: v->chance = (uint16_t)clamp_i32((int32_t)v->chance + delta * 10, 0, 1000); break;
    case 9: v->crush = (uint16_t)clamp_i32((int32_t)v->crush + delta * 10, 0, 1000); break;
    case 10: d.swing = (uint16_t)clamp_i32((int32_t)d.swing + delta * 10, 0, 1000); break;
    case 11: d.roll = (uint16_t)clamp_i32((int32_t)d.roll + delta * 10, 0, 1000); break;
    case 12: d.length = (uint8_t)clamp_i32((int32_t)d.length + delta, 1, MELLO_DRUM_STEPS); break;
    case 13: d.follow = delta > 0; break;
    case 14: d.space = (uint16_t)clamp_i32((int32_t)d.space + delta * 10, 0, 1000); break;
    case 15: d.level = (uint16_t)clamp_i32((int32_t)d.level + delta * 10, 0, 1000); break;
    default: return;
    }
    mello_engine_drums_set_config(e, &d);
    mark_edit(ui);
}

static void adjust_sound(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloSoundConfig c;
    if (!e || !delta) return;
    c = e->sound;
    if (ui->sound_subpage == 0u) {
        if (knob == 0u)
            c.osc_wave = (MelloWaveform)wrap_u8((int32_t)c.osc_wave + (delta > 0 ? 1 : -1), 6u);
        else if (knob == 1u)
            c.pwm_permille = (uint16_t)clamp_i32((int32_t)c.pwm_permille + delta * 10, 50, 950);
        else if (knob == 2u)
            c.filter_cut = (uint16_t)clamp_i32((int32_t)c.filter_cut + delta * 10, 0, 1000);
        else
            c.filter_res = (uint16_t)clamp_i32((int32_t)c.filter_res + delta * 10, 0, 1000);
    } else if (ui->sound_subpage == 1u) {
        if (knob == 0u)
            c.attack_ms = (uint16_t)clamp_i32((int32_t)c.attack_ms + delta * 5, 1, 500);
        else if (knob == 1u)
            c.decay_ms = (uint16_t)clamp_i32((int32_t)c.decay_ms + delta * 10, 5, 1000);
        else if (knob == 2u)
            c.sustain_permille = (uint16_t)clamp_i32((int32_t)c.sustain_permille + delta * 10, 0, 1000);
        else
            c.release_ms = (uint16_t)clamp_i32((int32_t)c.release_ms + delta * 10, 5, 2000);
    } else if (ui->sound_subpage == 2u) {
        if (knob == 0u)
            c.lfo_on = delta > 0;
        else if (knob ==
                 1u) { /* ~6% a detent: 0.008-20 Hz is a log range, as the HTML's slider (lfoRateToSlider) */
            int32_t r = c.lfo_rate_millihz, st = r / 16 > 1 ? r / 16 : 1;
            c.lfo_rate_millihz = (uint16_t)clamp_i32(r + delta * st, 8, 20000);
        } else if (knob == 2u)
            c.lfo_depth = (uint16_t)clamp_i32((int32_t)c.lfo_depth + delta * 10, 0, 1000);
        else
            c.lfo_target =
                (MelloLfoTarget)wrap_u8((int32_t)c.lfo_target + (delta > 0 ? 1 : -1), MELLO_LFO_TARGET_COUNT);
    } else if (ui->sound_subpage == 3u) {
        if (knob == 0u)
            c.follower_sub_on = delta > 0;
        else if (knob == 1u)
            c.follower_sub_level = (uint16_t)clamp_i32((int32_t)c.follower_sub_level + delta * 10, 0, 1000);
        else if (knob == 2u)
            c.follower_sub_octave = (int8_t)clamp_i32((int32_t)c.follower_sub_octave + delta, -2, 0);
        else
            c.follower_sub_wave = step_sub_waveform(c.follower_sub_wave, delta);
    } else if (ui->sound_subpage == 6u) { /* OSC 2 */
        if (knob == 0u) {                 /* OFF, SAW .. PULSE; turned on, it is heard at once */
            uint8_t w = c.osc2_wave == MELLO_WAVE_OFF ? 0u : (uint8_t)(c.osc2_wave + 1u);
            w = wrap_u8((int32_t)w + (delta > 0 ? 1 : -1), MELLO_WAVE_OFF + 1u);
            c.osc2_wave = w ? (MelloWaveform)(w - 1u) : MELLO_WAVE_OFF;
            if (c.osc2_wave != MELLO_WAVE_OFF && !c.osc2_mix) c.osc2_mix = 500u;
        } else if (knob == 1u)
            c.osc2_semi = (int8_t)clamp_i32((int32_t)c.osc2_semi + (delta > 0 ? 1 : -1), -24, 24);
        else if (knob == 2u)
            c.osc2_fine = (int8_t)clamp_i32((int32_t)c.osc2_fine + delta, -50, 50);
        else {
            c.osc2_mix = (uint16_t)clamp_i32((int32_t)c.osc2_mix + delta * 10, 0, 1000);
            if (c.osc2_mix && c.osc2_wave == MELLO_WAVE_OFF) c.osc2_wave = MELLO_WAVE_SAW; /* mixed in: on */
        }
    } else if (ui->sound_subpage == 7u) { /* CHARACTER */
        if (knob == 0u || knob == 1u) {
            if (knob == 0u)
                c.osc2_sync = delta > 0;
            else
                c.fm_amount = (uint16_t)clamp_i32((int32_t)c.fm_amount + delta * 10, 0, 1000);
            /* Sync and FM act on OSC 2: with OSC 2 not mixed in, bringing either in brings OSC 2 in, halfway. */
            if ((c.osc2_sync || c.fm_amount) && !c.osc2_mix) c.osc2_mix = 500u;
            if ((c.osc2_sync || c.fm_amount) && c.osc2_wave == MELLO_WAVE_OFF) c.osc2_wave = MELLO_WAVE_SAW;
        } else if (knob == 2u)
            c.saturation = (uint16_t)clamp_i32((int32_t)c.saturation + delta * 10, 0, 1000);
        else
            c.filter_mode = (MelloFilterMode)clamp_i32((int32_t)c.filter_mode + (delta > 0 ? 1 : -1), 0,
                                                       MELLO_FILTER_MODE_COUNT - 1);
    } else if (ui->sound_subpage == 8u) { /* MOD ENV */
        if (knob == 0u)
            c.menv_amount = (int16_t)clamp_i32((int32_t)c.menv_amount + delta * 10, -1000, 1000);
        else if (knob == 1u)
            c.menv_attack_ms = (uint16_t)clamp_i32((int32_t)c.menv_attack_ms + delta * 5, 0, 2000);
        else if (knob == 2u)
            c.menv_decay_ms = (uint16_t)clamp_i32((int32_t)c.menv_decay_ms + delta * 10, 5, 5000);
        else
            c.menv_target = (MelloModEnvTarget)wrap_u8((int32_t)c.menv_target + (delta > 0 ? 1 : -1),
                                                       MELLO_MENV_TARGET_COUNT);
    } else if (ui->sound_subpage == 9u) { /* TRACKING */
        if (knob == 0u)
            c.vel_cutoff = (int16_t)clamp_i32((int32_t)c.vel_cutoff + delta * 10, -1000, 1000);
        else if (knob == 1u)
            c.vel_level = (uint16_t)clamp_i32((int32_t)c.vel_level + delta * 10, 0, 2000);
        else if (knob == 2u)
            c.key_track = (uint16_t)clamp_i32((int32_t)c.key_track + delta * 10, 0, 1000);
        else
            c.vel_menv = (int16_t)clamp_i32((int32_t)c.vel_menv + delta * 10, -1000, 1000);
    } else if (ui->sound_subpage == 5u) { /* GRIT */
        if (knob == 0u)
            c.noise_level = (uint16_t)clamp_i32((int32_t)c.noise_level + delta * 10, 0, 1000);
        else if (knob == 1u)
            c.noise_fm = (uint16_t)clamp_i32((int32_t)c.noise_fm + delta * 10, 0, 1000);
        else if (knob == 2u)
            c.unison = (uint8_t)clamp_i32((int32_t)c.unison + (delta > 0 ? 1 : -1), 1, MELLO_UNISON_MAX);
        else
            c.detune = (uint16_t)clamp_i32((int32_t)c.detune + delta * 10, 0, 1000);
    } else {
        if (knob == 0u)
            c.slide_ms = (uint16_t)clamp_i32((int32_t)c.slide_ms + delta * 5, 0, 300);
        else if (knob == 1u)
            c.master_level = (uint16_t)clamp_i32((int32_t)c.master_level + delta * 10, 0, 1000);
    }
    mello_engine_sound_set_config(e, &c);
    mark_edit(ui);
}

static void adjust_audio_fx(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloFxConfig c;
    if (!e || !delta) return;
    c = e->fx;
    if (ui->fx_subpage == 0u) {
        if (knob == 0u)
            c.dist_amount = (uint16_t)clamp_i32((int32_t)c.dist_amount + delta * 10, 0, 1000);
        else if (knob == 1u)
            c.dist_mix = (uint16_t)clamp_i32((int32_t)c.dist_mix + delta * 10, 0, 1000);
        else if (knob == 2u)
            c.dist_character = (uint16_t)clamp_i32((int32_t)c.dist_character + delta * 10, 0, 1000);
        else
            c.dist_tone = (uint16_t)clamp_i32((int32_t)c.dist_tone + delta * 10, 0, 1000);
    } else if (ui->fx_subpage == 1u) {
        if (knob == 0u)
            c.delay_wet = (uint16_t)clamp_i32((int32_t)c.delay_wet + delta * 10, 0, 1000);
        else if (knob == 1u)
            c.delay_feedback = (uint16_t)clamp_i32((int32_t)c.delay_feedback + delta * 10, 0, 950);
        else if (knob == 2u)
            c.delay_lowcut = (uint16_t)clamp_i32((int32_t)c.delay_lowcut + delta * 10, 0, 1000);
        else
            c.delay_tone = (uint16_t)clamp_i32((int32_t)c.delay_tone + delta * 10, 0, 1000);
    } else if (ui->fx_subpage == 2u) {
        if (knob == 0u)
            c.delay_sync = delta > 0;
        else if (knob == 1u)
            c.delay_div = wrap_u8((int32_t)c.delay_div + (delta > 0 ? 1 : -1), 4u);
        else if (knob == 2u)
            c.delay_time_ms = (uint16_t)clamp_i32((int32_t)c.delay_time_ms + delta * 10, 30, 1500);
        else
            c.delay_level = (uint16_t)clamp_i32((int32_t)c.delay_level + delta * 10, 0, 1500);
    } else if (ui->fx_subpage == 3u) {
        if (knob == 0u)
            c.reverb_wet = (uint16_t)clamp_i32((int32_t)c.reverb_wet + delta * 10, 0, 1000);
        else if (knob == 1u)
            c.reverb_size = (uint16_t)clamp_i32((int32_t)c.reverb_size + delta * 10, 0, 1000);
        else if (knob == 2u)
            c.reverb_decay_ms = (uint16_t)clamp_i32((int32_t)c.reverb_decay_ms + delta * 100, 400, 60000);
        else
            c.reverb_tone = (uint16_t)clamp_i32((int32_t)c.reverb_tone + delta * 10, 0, 1000);
    } else {
        if (knob == 0u)
            c.reverb_pre_ms = (uint16_t)clamp_i32((int32_t)c.reverb_pre_ms + delta, 0, 120);
        else if (knob == 1u)
            c.reverb_level = (uint16_t)clamp_i32((int32_t)c.reverb_level + delta * 20, 0, 3000);
        else if (knob == 2u)
            c.dist_level = (uint16_t)clamp_i32((int32_t)c.dist_level + delta * 10, 200, 1400);
    }
    mello_engine_fx_set_config(e, &c);
    mark_edit(ui);
}

static void adjust_sample(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloSampleConfig c;
    MelloSampleSlotConfig *sl;
    if (!e || !delta) return;
    c = e->sample;
    if (c.edit_slot >= MELLO_SAMPLE_SLOT_COUNT) c.edit_slot = 0u;
    sl = &c.slots[c.edit_slot];
    if (ui->sample_subpage == 0u) {
        if (knob == 0u)
            sl->level = (uint16_t)clamp_i32((int32_t)sl->level + delta * 10, 0, 1500);
        else if (knob == 1u)
            sl->dist = (uint16_t)clamp_i32((int32_t)sl->dist + delta * 10, 0, 1000);
        else if (knob == 2u)
            sl->track = delta > 0;
        else
            sl->loop = delta > 0;
    } else if (ui->sample_subpage == 1u) {
        if (knob == 0u)
            sl->coarse_semitones = (int8_t)clamp_i32((int32_t)sl->coarse_semitones + delta, -24, 24);
        else if (knob == 1u)
            sl->fine_cents = (int8_t)clamp_i32((int32_t)sl->fine_cents + delta, -100, 100);
        else if (knob == 2u)
            sl->level = (uint16_t)clamp_i32((int32_t)sl->level + delta * 10, 0, 1500);
        else
            sl->dist = (uint16_t)clamp_i32((int32_t)sl->dist + delta * 10, 0, 1000);
    } else if (ui->sample_subpage == 2u) {
        if (knob == 0u)
            sl->trim_start =
                (uint16_t)clamp_i32((int32_t)sl->trim_start + delta * 5, 0, (int32_t)sl->trim_end - 1);
        else if (knob == 1u)
            sl->trim_end =
                (uint16_t)clamp_i32((int32_t)sl->trim_end + delta * 5, (int32_t)sl->trim_start + 1, 1000);
        else if (knob == 2u)
            sl->track = delta > 0;
        else
            sl->loop = delta > 0;
    } else {
        if (knob == 0u)
            sl->attack_ms = (uint16_t)clamp_i32((int32_t)sl->attack_ms + delta * 5, 1, 2000);
        else if (knob == 1u)
            sl->decay_ms = (uint16_t)clamp_i32((int32_t)sl->decay_ms + delta * 10, 5, 2000);
        else if (knob == 2u)
            sl->sustain_permille = (uint16_t)clamp_i32((int32_t)sl->sustain_permille + delta * 10, 0, 1000);
        else
            sl->release_ms = (uint16_t)clamp_i32((int32_t)sl->release_ms + delta * 10, 5, 3000);
    }
    mello_engine_sample_set_config(e, &c);
    mark_edit(ui);
}

static void adjust_key_scale(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    if (!e || !delta) return;
    if (knob == 0u)
        e->project.root_note = (uint8_t)clamp_i32((int32_t)e->project.root_note + delta, 24, 83);
    else if (knob == 1u)
        e->project.scale =
            (MelloScale)wrap_u8((int32_t)e->project.scale + (delta > 0 ? 1 : -1), (uint8_t)MELLO_SCALE_COUNT);
    else if (knob == 2u)
        e->project.global_octave = (int8_t)clamp_i32((int32_t)e->project.global_octave + delta, -2, 2);
    else if (knob == 3u)
        e->project.pitch_span_oct = (uint8_t)clamp_i32((int32_t)e->project.pitch_span_oct + delta, 1, 3);
    rebuild(ui);
    mark_edit(ui);
}

static void adjust_global(MelloUi *ui, uint8_t knob, int16_t delta, uint32_t now_us)
{
    MelloTransport *t = ui->transport;
    if (!t || !delta) return;
    if (ui->global_subpage == 0u) {
        if (knob == 0u && t->sync_mode != MELLO_SYNC_SLAVE)
            mello_transport_set_bpm(t, (uint16_t)clamp_i32((int32_t)t->engine.project.bpm + delta, 20, 250));
        else if (knob == 1u) {
            uint8_t d = t->engine.project.clock_div;
            if (delta > 0)
                d = d == 1u ? 2u : 4u;
            else
                d = d == 4u ? 2u : 1u;
            t->engine.project.clock_div = d;
            rebuild(ui);
        } else if (knob == 2u)
            mello_transport_set_sync(
                t, (MelloSyncMode)wrap_u8((int32_t)t->sync_mode + (delta > 0 ? 1 : -1), 3u), now_us);
        else if (knob == 3u)
            mello_transport_set_midi_note_out(t, delta > 0);
    } else {
        if (knob == 0u)
            mello_transport_set_midi_channel(t,
                                             (uint8_t)clamp_i32((int32_t)t->midi_channel + 1 + delta, 1, 16));
        else if (knob == 1u)
            mello_transport_set_midi_sub_channel(
                t, (uint8_t)clamp_i32((int32_t)t->midi_sub_channel + 1 + delta, 1, 16));
        else if (knob == 2u)
            mello_transport_set_midi_fixed_length(t, delta > 0, t->midi_fixed_len_ms);
        else if (knob == 3u)
            mello_transport_set_midi_fixed_length(
                t, t->midi_fixed_length,
                (uint16_t)clamp_i32((int32_t)t->midi_fixed_len_ms + delta * 10, 20, 2000));
    }
    mark_edit(ui);
}

static void adjust_algorithm(MelloUi *ui, int16_t delta)
{
    if (!delta) return;
    switch (ui->page) {
    case MELLO_UI_SEQ_STEP:
        ui->step_subpage = wrap_u8((int32_t)ui->step_subpage + (delta > 0 ? 1 : -1), 4u);
        break;
    case MELLO_UI_RANDOM: ui->random_links = !ui->random_links; break;
    case MELLO_UI_SEQ_LOOP:
        ui->loop_subpage = wrap_u8((int32_t)ui->loop_subpage + (delta > 0 ? 1 : -1), 3u);
        break;
    case MELLO_UI_SLOTS:
        ui->slots_subpage = wrap_u8((int32_t)ui->slots_subpage + (delta > 0 ? 1 : -1), 2u);
        break;
    case MELLO_UI_DOT: {
        uint8_t i;
        for (i = 0u;
             i < 7u && !(DOT_LAYERS[i][0] == ui->selected_dot_target && DOT_LAYERS[i][1] == ui->dot_shape);
             ++i) {
        }
        i = wrap_u8((int32_t)(i % 7u) + (delta > 0 ? 1 : -1), 7u);
        ui->selected_dot_target = DOT_LAYERS[i][0];
        ui->dot_shape = DOT_LAYERS[i][1] != 0u;
    } break;
    case MELLO_UI_TURING: ui->tm_subpage = wrap_u8((int32_t)ui->tm_subpage + (delta > 0 ? 1 : -1), 4u); break;
    case MELLO_UI_SUB: ui->sub_subpage = wrap_u8((int32_t)ui->sub_subpage + (delta > 0 ? 1 : -1), 5u); break;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: {
        uint8_t n = 0u, l = ui->chord_subpage;
        do /* past the layers not in use */
            l = wrap_u8((int32_t)l + (delta > 0 ? 1 : -1), CHORD_LAYERS);
        while (!chord_layer_shown(&eng(ui)->chord, l) && ++n < CHORD_LAYERS);
        ui->chord_subpage = l;
        break;
    }
#endif
    case MELLO_UI_GLOBAL:
        ui->global_subpage = wrap_u8((int32_t)ui->global_subpage + (delta > 0 ? 1 : -1), 2u);
        break;
    case MELLO_UI_DRUMS:
        ui->drum_subpage = wrap_u8((int32_t)ui->drum_subpage + (delta > 0 ? 1 : -1), DRUM_LAYERS);
        break;
    case MELLO_UI_SOUND_RESERVED:
        /* OSC, OSC 2, CHARACTER, GRIT, MOD ENV, AMP ENV, FOLLOWER, LEVEL: the LFO (subpage 2) is MOD's, reached
         * with the LFO button. Phase 5's layers (subpages 5-8) came later and sit after OSC, before the HTML's. */
        {
            static const uint8_t order[9] = {0u, 6u, 7u, 5u, 8u, 9u, 1u, 3u, 4u};
            uint8_t i;
            if (ui->sound_subpage == 2u) return;
            for (i = 0u; i < 9u && order[i] != ui->sound_subpage; ++i) {
            }
            ui->sound_subpage = order[wrap_u8((int32_t)(i % 9u) + (delta > 0 ? 1 : -1), 9u)];
        }
        break;
    case MELLO_UI_AUDIO_FX:
        ui->fx_subpage = wrap_u8((int32_t)ui->fx_subpage + (delta > 0 ? 1 : -1), 5u);
        break;
    case MELLO_UI_SAMPLE_RESERVED:
        ui->sample_subpage = wrap_u8((int32_t)ui->sample_subpage + (delta > 0 ? 1 : -1), 4u);
        break;
    default: break;
    }
    mark_edit(ui);
    ui->dirty |= MELLO_UI_DIRTY_HEADER; /* the header takes the layer's colour */
}

static void adjust_select(MelloUi *ui, int16_t delta)
{
    /* Direct positions live on the 16 white keys. SELECT changes the current
     * function layer instead of duplicating step/cell selection on a knob. */
    adjust_algorithm(ui, delta);
}

static void adjust_presets(MelloUi *ui, int16_t delta)
{
    if (!delta) return;
    ui->selected_slot = (uint8_t)clamp_i32((int32_t)ui->selected_slot + delta, 0, 63);
    ui->slot_bank = ui->selected_slot / 16u;
    if (ui->page == MELLO_UI_SLOTS) mark_edit(ui);
}

static const char *random_intensity_name(MelloRandomizeIntensity intensity)
{
    return intensity == MELLO_RANDOMIZE_LOW ? "LOW" : intensity == MELLO_RANDOMIZE_HIGH ? "HIGH" : "MED";
}

static void adjust_random(MelloUi *ui, uint8_t knob, int16_t delta)
{
    if (!ui || knob != 0u || !delta) return;
    if (ui->random_edit && eng(ui)) { /* EDIT: K1 is the picked target's amount, in 5% steps */
        MelloEngine *e = eng(ui);
        MelloModRouting m = e->modr;
        uint8_t b = random_target_bitno(ui->random_sel);
        m.rnd_amount[b] = (uint8_t)clamp_i32((int32_t)m.rnd_amount[b] + delta * 5, 0, 100);
        mello_engine_mod_routing_set(e, &m);
        mark_edit(ui);
        return;
    }
    ui->random_intensity = (MelloRandomizeIntensity)clamp_i32(
        (int32_t)ui->random_intensity + (delta > 0 ? 1 : -1), MELLO_RANDOMIZE_LOW, MELLO_RANDOMIZE_HIGH);
    mark_edit(ui);
}

static const char *layer_name(const MelloUi *ui);
static const char *domain_name(const MelloUi *ui);
static uint8_t save_state(const MelloUi *ui);
static void draw_param_panel(MelloUi *ui, const MelloUiDrawOps *o);
static const char *full_label(const char *l);
/* The delay's LO CUT in Hz: 20 x 100^(x / 1000), 2^f taken as 1 + f (0.657 + 0.343 f) (within 0.3%). */
static uint32_t delay_lowcut_hz(uint16_t x)
{
    uint32_t e = (uint32_t)x * 6644u / 1000u, f = e % 1000u;
    return (20u << (e / 1000u)) * (1000u + f * (657u + 343u * f / 1000u) / 1000u) / 1000u;
}
static void value_millihz(char *b, uint32_t mhz);
static void show_readout(MelloUi *ui, uint8_t what, uint32_t now_ms)
{
    ui->readout = what;
    ui->readout_until_ms = now_ms + MELLO_UI_READOUT_MS;
    ui->dirty |= MELLO_UI_DIRTY_BODY;
}

/* Acceleration: detents that come fast count more (x2 under 60 ms apart, x4 under 25 ms), so a sweep crosses a
 * range in a turn or two while slow turning stays exact. Handlers that step a list use only the sign, so lists
 * (waves, modes, scales) never run away. */
static int16_t accelerate(MelloUi *ui, uint8_t encoder, int16_t d, uint32_t now_ms)
{
    uint32_t gap;
    if (!d) return 0;
    gap = now_ms - ui->enc_last_ms[encoder];
    ui->enc_last_ms[encoder] = now_ms;
    if (gap == 0u) return d; /* same millisecond: one round's detents already arrive together as d */
    if (gap < 25u) return (int16_t)(d * 4);
    if (gap < 60u) return (int16_t)(d * 2);
    return d;
}

static void adjust_knobs(MelloUi *ui, const int16_t enc_in[MELLO_UI_ENCODER_COUNT], uint32_t now_us,
                         uint32_t now_ms)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT];
    uint8_t k;
    for (k = 0u; k < MELLO_UI_ENCODER_COUNT; ++k)
        enc[k] = k == MELLO_UI_ENC_SELECT || (k >= MELLO_UI_ENC_KNOB1 && k <= MELLO_UI_ENC_KNOB4)
                     ? accelerate(ui, k, enc_in[k], now_ms)
                     : enc_in[k];
    uint16_t before = ui->dirty;
    if (ui->page == MELLO_UI_HOME) {
        /* HOME's three encoders are fixed globals: TEMPO, RANDOM intensity, PATTERN (to the next saved slot,
         * switched on the next bar while playing). */
        MelloEngine *e = eng(ui);
        int16_t ds = (int16_t)(enc[MELLO_UI_ENC_SELECT]), da = enc[MELLO_UI_ENC_ALGORITHM],
                dp = enc[MELLO_UI_ENC_PRESETS];
        if (ds && e) {
            mello_transport_set_bpm(ui->transport,
                                    (uint16_t)clamp_i32((int32_t)e->project.bpm + ds, 20, 250));
            mark_edit(ui);
            ui->dirty |= MELLO_UI_DIRTY_HEADER; /* the header shows the tempo */
            show_readout(ui, MELLO_UI_READOUT_TEMPO, now_ms);
        }
        if (da) {
            adjust_random(ui, 0u, da);
            show_readout(ui, MELLO_UI_READOUT_INTENSITY, now_ms);
        }
        if (dp && e) {
            int32_t i = ui->selected_slot,
                    n = e->visible_slot_count < MELLO_MAX_SLOTS ? e->visible_slot_count : MELLO_MAX_SLOTS;
            int32_t step = dp > 0 ? 1 : -1, tries;
            for (tries = 0; tries < n; ++tries) {
                i += step;
                if (i < 0 || i >= n) break;
                if (mello_engine_slot_valid(e, (uint8_t)i)) {
                    ui->selected_slot = (uint8_t)i;
                    (void)mello_engine_request_slot(e, (uint8_t)i);
                    mark_edit(ui);
                    break;
                }
            }
            show_readout(ui, MELLO_UI_READOUT_PATTERN, now_ms);
        }
        before = ui->dirty; /* keep what HOME's encoders marked */
    }
    /* The readout shows only what a turn changed: an inert knob, or a page without layers, stays quiet. */
    ui->dirty = 0u;
    if (ui->page != MELLO_UI_HOME) {
        adjust_select(ui, enc[MELLO_UI_ENC_SELECT]);
        adjust_algorithm(ui, enc[MELLO_UI_ENC_ALGORITHM]);
        if (ui->dirty && layer_name(ui)) show_readout(ui, MELLO_UI_READOUT_LAYER, now_ms);
        ui->dirty |= before;
        adjust_presets(ui, enc[MELLO_UI_ENC_PRESETS]);
    } else
        ui->dirty |= before;
    for (k = 0u; k < 4u; ++k) {
        int16_t d = enc[MELLO_UI_ENC_KNOB1 + k];
        if (!d) continue;
        before = ui->dirty;
        ui->dirty = 0u;
        switch (ui->page) {
        case MELLO_UI_HOME: adjust_home(ui, k, d); break;
        case MELLO_UI_SEQ_STEP: adjust_step(ui, k, d); break;
        case MELLO_UI_SEQ_SEGMENT: adjust_segment(ui, k, d); break;
        case MELLO_UI_SEQ_LOOP: adjust_loop(ui, k, d); break;
        case MELLO_UI_SEQ_SLOTSEQ: adjust_slot_seq(ui, k, d, now_ms); break;
        case MELLO_UI_RANDOM: adjust_random(ui, k, d); break;
        case MELLO_UI_KEY_SCALE: adjust_key_scale(ui, k, d); break;
        case MELLO_UI_SLOTS: adjust_slots(ui, k, d, now_ms); break;
        case MELLO_UI_DOT: adjust_dot(ui, k, d); break;
        case MELLO_UI_TURING: adjust_turing(ui, k, d); break;
        case MELLO_UI_SUB: adjust_sub(ui, k, d); break;
#if MELLO_FEATURE_CHORD_MACHINE
        case MELLO_UI_CHORD: adjust_chord(ui, k, d); break;
#endif
        case MELLO_UI_GLOBAL: adjust_global(ui, k, d, now_us); break;
        case MELLO_UI_SOUND_RESERVED: adjust_sound(ui, k, d); break;
        case MELLO_UI_AUDIO_FX: adjust_audio_fx(ui, k, d); break;
        case MELLO_UI_SAMPLE_RESERVED: adjust_sample(ui, k, d); break;
        case MELLO_UI_DRUMS: adjust_drums(ui, k, d); break;
        default: break;
        }
        if (ui->dirty) show_readout(ui, (uint8_t)(k + 1u), now_ms);
        ui->dirty |= before;
    }
}

static void save_selected_slot(MelloUi *ui, uint32_t now_ms)
{
    MelloEngine *e = eng(ui);
    char name[MELLO_SLOT_NAME_CHARS + 1u];
    if (!e) return;
    if (e->slots[ui->selected_slot].valid && e->slots[ui->selected_slot].name[0]) {
        memcpy(name, e->slots[ui->selected_slot].name, sizeof name);
    } else {
        memset(name, 0, sizeof name);
        name[0] = 'S';
        name[1] = 'L';
        name[2] = 'O';
        name[3] = 'T';
        name[4] = ' ';
        name[5] = (char)('0' + ((ui->selected_slot + 1u) / 10u) % 10u);
        name[6] = (char)('0' + (ui->selected_slot + 1u) % 10u);
    }
    (void)mello_engine_save_slot(e, ui->selected_slot, name);
    persistent_dirty(ui, now_ms);
    mark_edit(ui);
}

static void tap_tempo(MelloUi *ui, uint32_t now_ms)
{
    uint32_t fresh[8], sum = 0u, numerator;
    uint8_t count = 0u, i;
    uint16_t bpm;
    if (!ui || !ui->transport) return;

    /* Exact source policy: discard taps 3000 ms or older and keep the latest 8. */
    for (i = 0u; i < ui->tap_count && i < 8u; ++i) {
        if ((uint32_t)(now_ms - ui->tap_times_ms[i]) < 3000u) fresh[count++] = ui->tap_times_ms[i];
    }
    if (count == 8u) {
        for (i = 1u; i < 8u; ++i) fresh[i - 1u] = fresh[i];
        count = 7u;
    }
    fresh[count++] = now_ms;
    memcpy(ui->tap_times_ms, fresh, count * sizeof(fresh[0]));
    ui->tap_count = count;
    if (count < 2u) return;

    for (i = 1u; i < count; ++i) sum += (uint32_t)(fresh[i] - fresh[i - 1u]);
    if (!sum) return;
    numerator = 60000u * (uint32_t)(count - 1u);
    bpm = (uint16_t)((numerator + sum / 2u) / sum);
    if (bpm >= 20u && bpm <= 250u) {
        mello_transport_set_bpm(ui->transport, bpm);
        persistent_dirty(ui, now_ms);
        mark_edit(ui);
    }
}

static void factory_reset_ui(MelloUi *ui, uint32_t now_ms)
{
    MelloTransport *transport;
    MelloStorage *storage;
    MelloSampleBank *sample_bank;
    if (!ui || !ui->transport) return;
    transport = ui->transport;
    storage = ui->storage;
    sample_bank = ui->sample_bank;

    /* Browser RESET ALL stops transport, clears snapshot/slots/slot-seq and reloads
     * defaults. On FM-1 sample audio lives in a separate flash store, so those bytes
     * are deliberately preserved while the project/sample configuration resets. */
    mello_transport_factory_reset(transport, 0x4D454C4Cu);
    mello_ui_init(ui, transport, storage);
    mello_ui_attach_sample_bank(ui, sample_bank);
    manual_save(ui, now_ms);
}

static void handle_context_action(MelloUi *ui, uint32_t now_ms, uint8_t action)
{
    MelloEngine *e = eng(ui);
    const MelloContextAction *a = context_action(ui->page, action);
    if (!e || !a || a->verb == MELLO_CTX_NONE) return;

    switch (a->verb) {
    case MELLO_CTX_OPEN_RANDOM: mello_ui_set_page(ui, MELLO_UI_RANDOM); break;
    case MELLO_CTX_STEP_SKIP: {
        MelloStep *st = &e->project.steps[ui->selected_step];
        st->skip = !st->skip;
        rebuild(ui);
        mark_edit(ui);
        break;
    }
    case MELLO_CTX_SEG_A:
    case MELLO_CTX_SEG_B:
    case MELLO_CTX_SEG_C:
    case MELLO_CTX_SEG_D:
    case MELLO_CTX_SEG_E:
    case MELLO_CTX_SEG_F: {
        uint8_t b[MELLO_MAX_SEGMENTS + 1u], n = (uint8_t)(a->verb - MELLO_CTX_SEG_A);
        if (n < mello_segment_bounds(&e->project, b)) ui->selected_segment = n;
        ui->dirty |= MELLO_UI_DIRTY_HEADER;
        mark_edit(ui);
        break;
    }
    case MELLO_CTX_DRUM_KICK:
    case MELLO_CTX_DRUM_SNARE:
    case MELLO_CTX_DRUM_HAT:
    case MELLO_CTX_DRUM_GLITCH:
        ui->selected_drum_voice = (uint8_t)(a->verb - MELLO_CTX_DRUM_KICK);
        mark_edit(ui);
        break;
    case MELLO_CTX_DRUM_ON:
    case MELLO_CTX_DRUM_ACCENT:
    case MELLO_CTX_DRUM_FEED: {
        MelloDrumConfig d = e->drums;
        if (a->verb == MELLO_CTX_DRUM_ON)
            d.on = !d.on;
        else if (a->verb == MELLO_CTX_DRUM_FEED)
            d.feed = !d.feed;
        else
            d.accents[ui->selected_drum_voice % MELLO_DRUM_VOICES] ^=
                (uint16_t)(1u << (ui->selected_drum_step % MELLO_DRUM_STEPS));
        mello_engine_drums_set_config(e, &d);
        persistent_dirty(ui, now_ms);
        mark_edit(ui);
        break;
    }
    case MELLO_CTX_DRUM_ROLL:
        mello_engine_undo_snapshot(e);
        mello_engine_drums_roll(e);
        persistent_dirty(ui, now_ms);
        mark_edit(ui);
        break;
    case MELLO_CTX_SEG_EVEN:
        mello_segment_even(&e->project);
        rebuild(ui);
        mark_edit(ui);
        break;
    case MELLO_CTX_LOOP_RESET:
        mello_engine_loop_cycle_reset_position(e);
        mark_edit(ui);
        break;
    case MELLO_CTX_RANDOM_REROLL:
        mello_engine_randomize_steps_intensity(e, ui->random_intensity);
        rebuild(ui);
        persistent_dirty(ui, now_ms);
        mark_edit(ui);
        break;
    case MELLO_CTX_PERF_RESTART: mello_engine_perf_restart(e); break;
    case MELLO_CTX_PERF_FILL:
    case MELLO_CTX_PERF_SUB_MUTE:
    case MELLO_CTX_PERF_REVERSE:
    case MELLO_CTX_PERF_REPEAT: break; /* held, not pressed: perf_poll */
    case MELLO_CTX_RNDMZR:
        ui->rndmzr_down = true; /* tap or hold: rndmzr_poll decides */
        ui->rndmzr_down_ms = now_ms;
        break;
    case MELLO_CTX_HOME_RANDOM:
        mello_engine_randomize_steps_intensity(e, ui->random_intensity);
        rebuild(ui);
        persistent_dirty(ui, now_ms);
        mark_edit(ui);
        break;
    case MELLO_CTX_UNDO:
        if (mello_engine_undo_swap(e)) {
            persistent_dirty(ui, now_ms);
            mark_edit(ui);
        }
        break;
    case MELLO_CTX_MUTATE: {
        uint16_t mask = e->randomize.mask;
        mello_engine_set_randomize_mask(e, MELLO_RAND_PITCH);
        mello_engine_randomize_steps_intensity(e, MELLO_RANDOMIZE_LOW);
        mello_engine_set_randomize_mask(e, mask);
        rebuild(ui);
        persistent_dirty(ui, now_ms);
        mark_edit(ui);
        break;
    }
    case MELLO_CTX_TRANSPOSE_DOWN:
    case MELLO_CTX_TRANSPOSE_UP:
        e->project.root_note = (uint8_t)clamp_i32(
            (int32_t)e->project.root_note + (a->verb == MELLO_CTX_TRANSPOSE_UP ? 1 : -1), 24, 83);
        rebuild(ui);
        mark_edit(ui);
        break;
    case MELLO_CTX_RANDOM_CLEAR:
        if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) <= 0) {
            ui->reset_armed = false;
            mello_engine_undo_snapshot(e); /* CLEAR can be undone */
            mello_engine_clear_pattern(e);
            persistent_dirty(ui, now_ms);
            mark_edit(ui);
        } else {
            ui->reset_armed = true;
            ui->reset_confirm_until_ms = now_ms + 3000u;
            mark_edit(ui);
        }
        break;
    case MELLO_CTX_RANDOM_NONE:
        mello_engine_set_randomize_mask(e, 0u);
        mark_edit(ui);
        break;
    case MELLO_CTX_RANDOM_ALL:
        mello_engine_set_randomize_mask(e, MELLO_RAND_ALL);
        mark_edit(ui);
        break;
    case MELLO_CTX_DOT_REROLL:
        mello_engine_dot_randomize_cells(e, (MelloDotTarget)ui->selected_dot_target);
        mark_edit(ui);
        break;
    case MELLO_CTX_DOT_RESET: {
        MelloDotTarget t = (MelloDotTarget)ui->selected_dot_target;
        MelloDotMatrixConfig c = e->dot[t];
        c.reset = !c.reset;
        mello_engine_dot_set_config(e, t, &c);
        mark_edit(ui);
        break;
    }
    case MELLO_CTX_DOT_FOLLOW: {
        MelloDotTarget t = (MelloDotTarget)ui->selected_dot_target;
        MelloDotMatrixConfig c = e->dot[t];
        c.follow = !c.follow;
        mello_engine_dot_set_config(e, t, &c);
        mark_edit(ui);
        break;
    }
    case MELLO_CTX_DOT_REVERSE: {
        MelloDotTarget t = (MelloDotTarget)ui->selected_dot_target;
        MelloDotMatrixConfig c = e->dot[t];
        c.reverse = !c.reverse;
        mello_engine_dot_set_config(e, t, &c);
        mark_edit(ui);
        break;
    }
    case MELLO_CTX_TURING_LOOP: {
        MelloModRouting m = e->modr;
        m.tm_on_loop = !m.tm_on_loop;
        mello_engine_mod_routing_set(e, &m);
        mark_edit(ui);
        break;
    }
    case MELLO_CTX_TURING_TOGGLE: {
        MelloTmConfig c = e->tm;
        c.on = !c.on;
        mello_engine_tm_set_config(e, &c);
        mark_edit(ui);
        break;
    }
    case MELLO_CTX_TURING_REROLL:
        mello_engine_tm_reroll(e);
        mark_edit(ui);
        break;
    case MELLO_CTX_SUB_TOGGLE: {
        MelloSubConfig c = e->sub;
        c.on = !c.on;
        mello_engine_sub_set_config(e, &c);
        mark_edit(ui);
        break;
    }
    case MELLO_CTX_SUB_STEP:
        if (ui->selected_sub_x_step < MELLO_SUB_X_MAX_STEPS) {
            e->sub.x_steps[ui->selected_sub_x_step].on = !e->sub.x_steps[ui->selected_sub_x_step].on;
            mark_edit(ui);
        }
        break;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_CTX_CHORD_TOGGLE: {
        MelloChordConfig c = e->chord;
        c.on = !c.on;
        mello_engine_chord_set_config(e, &c);
        mark_edit(ui);
        break;
    }
#endif
    case MELLO_CTX_GLOBAL_TAP: tap_tempo(ui, now_ms); break;
    case MELLO_CTX_SOUND_INIT:
        /* The way back to the plain sound (feedback 2026-10-30): pressed twice within 3 s, as CLR2; undoable. */
        if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) <= 0) {
            MelloSoundConfig d;
            ui->reset_armed = false;
            mello_engine_undo_snapshot(e);
            mello_sound_defaults(&d);
            mello_engine_sound_set_config(e, &d);
            persistent_dirty(ui, now_ms);
            mark_edit(ui);
        } else {
            ui->reset_armed = true;
            ui->reset_confirm_until_ms = now_ms + 3000u;
            mark_edit(ui);
        }
        break;
    case MELLO_CTX_GLOBAL_RESET:
        if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) <= 0) {
            factory_reset_ui(ui, now_ms);
        } else {
            ui->reset_armed = true;
            ui->reset_confirm_until_ms = now_ms + 3000u;
            mark_edit(ui);
        }
        break;
    case MELLO_CTX_SAMPLE_TOGGLE: {
        MelloSampleConfig c = e->sample;
        c.on = !c.on;
        mello_engine_sample_set_config(e, &c);
        mark_edit(ui);
        break;
    }
    case MELLO_CTX_NONE:
    default: break;
    }
}

static void handle_buttons(MelloUi *ui, uint32_t now_us, uint32_t now_ms, uint32_t pressed, uint32_t held)
{
    MelloEngine *e = eng(ui);
    if (!e) return;

    if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) > 0) {
        ui->reset_armed = false;
        mark_edit(ui);
    }
    if (ui->reset_armed && pressed) {
        ui->reset_armed = false;
        mark_edit(ui);
    }

    if (pressed & bit_u32(MELLO_UI_BTN_PLAY)) {
        if (ui->transport->running)
            mello_transport_stop(ui->transport);
        else
            /* Start one lookahead out, so the first step reaches the audio before its block, like the rest. */
            mello_transport_start(ui->transport, now_us + MELLO_TRANSPORT_LOOKAHEAD_US);
        ui->dirty |= MELLO_UI_DIRTY_HEADER | MELLO_UI_DIRTY_STATUS | MELLO_UI_DIRTY_LEDS;
    }
    if (pressed & bit_u32(MELLO_UI_BTN_HOME)) {
        if (held & bit_u32(MELLO_UI_BTN_SAVE))
            manual_save(ui, now_ms);
        else
            mello_ui_set_page(ui, MELLO_UI_HOME);
    }
    if (pressed & bit_u32(MELLO_UI_BTN_SEQ)) seq_button(ui);
    if (pressed & bit_u32(MELLO_UI_BTN_EDIT)) {
        if (ui->page == MELLO_UI_TURING) {
            MelloTmConfig c = e->tm;
            c.locked = !c.locked;
            mello_engine_tm_set_config(e, &c);
            mark_edit(ui);
        } else if (ui->page == MELLO_UI_SAMPLE_RESERVED) {
            MelloSampleConfig c = e->sample;
            c.per_step = !c.per_step;
            mello_engine_sample_set_config(e, &c);
            mark_edit(ui);
        } else if (ui->page == MELLO_UI_RANDOM) {
            ui->random_edit = !ui->random_edit; /* EDIT: how much each target is rolled */
            mark_full(ui);
        } else if (ui->page == MELLO_UI_HOME) {
            ui->keys_mode = !ui->keys_mode; /* KEYS: the keys are a keyboard */
            ui->keys_rec = false;
            ui->keys_rec_pos = 0u;
            mark_full(ui);
        }
    }
    if ((pressed & bit_u32(MELLO_UI_BTN_REC)) && ui->page == MELLO_UI_HOME && ui->keys_mode) {
        ui->keys_rec = !ui->keys_rec; /* step-record from step 1 */
        ui->keys_rec_pos = 0u;
        mark_full(ui);
    }
    /* Page buttons (UI_ICEBERG.md): from elsewhere a button opens its page on the first layer, so the screen never
     * depends on history; pressed again on its own page it goes to the next layer. */
    if (pressed & bit_u32(MELLO_UI_BTN_SEL)) open_page(ui, MELLO_UI_KEY_SCALE);
    if (pressed & bit_u32(MELLO_UI_BTN_FX)) {
        if (ui->page == MELLO_UI_AUDIO_FX)
            adjust_algorithm(ui, 1);
        else
            open_page(ui, MELLO_UI_AUDIO_FX);
    }
    if (pressed & bit_u32(MELLO_UI_BTN_LFO)) { /* MOD: LFO -> DOT -> EVOLVE -> LFO */
        if (ui->page == MELLO_UI_SOUND_RESERVED && ui->sound_subpage == 2u)
            open_page(ui, MELLO_UI_DOT);
        else if (ui->page == MELLO_UI_DOT)
            open_page(ui, MELLO_UI_TURING);
        else {
            open_page(ui, MELLO_UI_SOUND_RESERVED);
            ui->sound_subpage = 2u;
        }
    }
    if (pressed & bit_u32(MELLO_UI_BTN_ARP)) { /* BASS: SUB -> CHORD -> DRUMS -> SUB */
#if MELLO_FEATURE_CHORD_MACHINE
        open_page(ui, ui->page == MELLO_UI_SUB ? MELLO_UI_CHORD : ui->page == MELLO_UI_CHORD ? MELLO_UI_DRUMS : MELLO_UI_SUB);
#else
        if (ui->page == MELLO_UI_SUB) {
            MelloSubConfig c = e->sub;
            c.on = !c.on;
            mello_engine_sub_set_config(e, &c);
            mark_edit(ui);
        } else
            open_page(ui, MELLO_UI_SUB);
#endif
    }
    if (pressed & bit_u32(MELLO_UI_BTN_GLO)) /* GLOBAL -> SAMPLE -> GLOBAL */
        open_page(ui, ui->page == MELLO_UI_GLOBAL ? MELLO_UI_SAMPLE_RESERVED : MELLO_UI_GLOBAL);
    if (pressed &
        bit_u32(MELLO_UI_BTN_ENV)) { /* SOUND: its layers in adjust_algorithm's order (the LFO is MOD's) */
        if (ui->page == MELLO_UI_SOUND_RESERVED && ui->sound_subpage != 2u)
            adjust_algorithm(ui, 1);
        else
            open_page(ui, MELLO_UI_SOUND_RESERVED);
    }
    if (pressed & bit_u32(MELLO_UI_BTN_SAVE)) {
        if (ui->page == MELLO_UI_SLOTS)
            save_selected_slot(ui, now_ms);
        else {
            ui->save_down = true; /* short = save now, held = SLOTS: decided in save_button_poll */
            ui->save_down_ms = now_ms;
        }
    }
    /* REC is reserved for actual record/capture. No capture path exists yet. */
    if ((pressed & (bit_u32(MELLO_UI_BTN_OCT_DOWN) | bit_u32(MELLO_UI_BTN_OCT_UP))) &&
        ui->page == MELLO_UI_HOME && ui->keys_mode) { /* KEYS: OCT-/OCT+ move the keyboard an octave */
        ui->keys_octave =
            (int8_t)clamp_i32(ui->keys_octave + ((pressed & bit_u32(MELLO_UI_BTN_OCT_UP)) ? 1 : -1), -2, 2);
        ui->dirty |= MELLO_UI_DIRTY_FOOTER | MELLO_UI_DIRTY_LEDS;
        pressed &= ~(bit_u32(MELLO_UI_BTN_OCT_DOWN) | bit_u32(MELLO_UI_BTN_OCT_UP));
    }
    if (pressed & bit_u32(MELLO_UI_BTN_OCT_DOWN)) {
        if (ui->page == MELLO_UI_HOME || ui->page == MELLO_UI_SEQ_STEP) {
            ui->step_bank = 0u;
            mark_edit(ui);
        } else if (ui->page == MELLO_UI_SEQ_SLOTSEQ && ui->slot_seq_bank > 0u) {
            --ui->slot_seq_bank;
            mark_edit(ui);
        } else if (ui->page == MELLO_UI_SLOTS && ui->slot_bank > 0u) {
            --ui->slot_bank;
            mark_edit(ui);
        } else if (ui->page == MELLO_UI_TURING) {
            ui->tm_bank = 0u;
            mark_edit(ui);
        }
    }
    if (pressed & bit_u32(MELLO_UI_BTN_OCT_UP)) {
        if (ui->page == MELLO_UI_HOME || ui->page == MELLO_UI_SEQ_STEP) {
            ui->step_bank = 1u;
            mark_edit(ui);
        } else if (ui->page == MELLO_UI_SEQ_SLOTSEQ && ui->slot_seq_bank < 3u) {
            ++ui->slot_seq_bank;
            mark_edit(ui);
        } else if (ui->page == MELLO_UI_SLOTS && ui->slot_bank < 3u) {
            ++ui->slot_bank;
            mark_edit(ui);
        } else if (ui->page == MELLO_UI_TURING) {
            ui->tm_bank = 1u;
            mark_edit(ui);
        }
    }
}

/* The black keys' note indices by strip position (0 = F#3 .. 10 = F#5): the 27 keys from F3 less the white ones. A
 * table, as perf_poll reads them on every input (a search there cost most of the main loop: 0.5-k1 .. f2). */
static const uint8_t BLACK_KEY_NOTES[11] = {1u, 3u, 5u, 8u, 10u, 13u, 15u, 17u, 20u, 22u, 25u};

/* HOME's performance keys act while held: which are held now goes to the engine (mello_engine_set_perf) on every
 * input; anywhere else, in KEYS or with a step held, none are. The strip shows held keys by name. */
static void perf_poll(MelloUi *ui)
{
    MelloEngine *e = eng(ui);
    MelloPerf p;
    uint16_t held = 0u;
    uint8_t b;
    if (!e) return;
    if (ui->page == MELLO_UI_HOME && !ui->keys_mode && ui->held_step == MELLO_UI_NO_STEP && ui->held_notes)
        for (b = 0u; b < 11u; ++b)
            if (ui->held_notes & bit_u32(BLACK_KEY_NOTES[b])) held |= (uint16_t)(1u << b);
    if (!held && !ui->held_black) return; /* nothing held, nothing was: the engine's perf is already off */
    if (held != ui->held_black) ui->dirty |= MELLO_UI_DIRTY_FOOTER;
    ui->held_black = held;
    memset(&p, 0, sizeof p);
    p.fill = (held & (1u << 3)) != 0u;
    p.sub_mute = (held & (1u << 4)) != 0u;
    p.reverse = (held & (1u << 5)) != 0u;
    p.repeat = (held & (1u << 6)) != 0u;
    p.repeat_len = ui->perf_repeat_len ? ui->perf_repeat_len : 4u;
    mello_engine_set_perf(e, &p);
}

void mello_ui_set_held_notes(MelloUi *ui, uint32_t notes)
{
    MelloEngine *e;
    if (!ui) return;
    ui->held_notes = notes;
    if (ui->audition_key != 0xFFu && !(notes & bit_u32(ui->audition_key)))
        ui->audition_release = true; /* let go: mello_ui_input ends the note, with its time */
    if (ui->held_step == MELLO_UI_NO_STEP || (notes & bit_u32(ui->held_key))) return;
    e = eng(ui);
    (void)e;
    ui->held_step = MELLO_UI_NO_STEP; /* let go: K1-K4 are the macros again, the strip the rail */
    ui->dirty |= MELLO_UI_DIRTY_FOOTER | MELLO_UI_DIRTY_BODY | MELLO_UI_DIRTY_LEDS;
}

void mello_ui_input(MelloUi *ui, uint32_t now_us, uint32_t now_ms, uint32_t pressed_buttons,
                    uint32_t released_buttons, uint32_t held_buttons, uint32_t note_pressed,
                    const int16_t encoder_delta[MELLO_UI_ENCODER_COUNT])
{
    uint8_t i;
    if (!ui || !ui->transport) return;
    if (ui->readout && (int32_t)(now_ms - ui->readout_until_ms) >= 0) { /* its time is up: the body again */
        ui->readout = 0u;
        ui->dirty |= MELLO_UI_DIRTY_BODY;
    }
    if (ui->audition_release) {
        ui->audition_release = false;
        audition(ui, ui->audition_note, false, now_us);
        ui->audition_key = 0xFFu;
    }
    handle_buttons(ui, now_us, now_ms, pressed_buttons, held_buttons);
    if (ui->save_down) {
        if (!(held_buttons & bit_u32(MELLO_UI_BTN_SAVE)) || (released_buttons & bit_u32(MELLO_UI_BTN_SAVE))) {
            ui->save_down = false; /* let go early: save the project now */
            manual_save(ui, now_ms);
        } else if ((int32_t)(now_ms - ui->save_down_ms) >= 700) {
            ui->save_down = false; /* held: the pattern slots */
            mello_ui_set_page(ui, MELLO_UI_SLOTS);
        }
    }
    perf_poll(ui); /* before the presses: REPEAT held + a white key */
    for (i = 0u; i < 27u; ++i)
        if (note_pressed & bit_u32(i)) handle_note_press(ui, now_us, now_ms, held_buttons, i);
    perf_poll(ui);
    if (encoder_delta) adjust_knobs(ui, encoder_delta, now_us, now_ms);
    if (ui->rndmzr_down) { /* RNDMZR: let go = roll now; held half a second = the RANDOM page */
        MelloEngine *e = eng(ui);
        if (!(ui->held_notes & bit_u32(ui->rndmzr_key))) {
            ui->rndmzr_down = false;
            if (e) {
                mello_engine_randomize_steps_intensity(e, ui->random_intensity);
                rebuild(ui);
                persistent_dirty(ui, now_ms);
                mark_edit(ui);
            }
        } else if ((int32_t)(now_ms - ui->rndmzr_down_ms) >= 500) {
            ui->rndmzr_down = false;
            if (ui->page != MELLO_UI_RANDOM) mello_ui_set_page(ui, MELLO_UI_RANDOM);
        }
    }
}

void mello_ui_tick(MelloUi *ui)
{
    MelloEngine *e;
    uint8_t run, step_changed, other_changed, dynamic_body = 1u;
    if (!ui || !ui->transport) return;
    e = &ui->transport->engine;
    run = ui->transport->running ? 1u : 0u;
    step_changed = ui->seen_step != ui->transport->display_step;
    other_changed = ui->seen_pulse != ui->transport->display_pulse ||
                    ui->seen_segment != ui->transport->display_segment ||
                    ui->seen_loop_pos != e->loop_cycle_pos || ui->seen_slot_seq_pos != e->slot_seq_pos ||
                    ui->seen_tm_pos != e->tm_active_index || ui->seen_sub_x_pos != e->sub_x_active_index ||
#if MELLO_FEATURE_CHORD_MACHINE
                    ui->seen_chord_pos != e->chord_chain_pos ||
                    ui->seen_chord_grid != e->chord_grid_active_cell ||
                    (ui->page == MELLO_UI_CHORD && ui->seen_chord_lit != ui->transport->display_chord_lit) ||
#endif
                    (ui->page == MELLO_UI_DRUMS && ui->seen_drum_step != e->drum_step) ||
                    ui->seen_running != run;

    /* Static/edit pages do not depend on the transport cursor. Do not redraw
       their 142-pixel body merely because playback advanced elsewhere. */
    switch (ui->page) {
    case MELLO_UI_HOME:
    case MELLO_UI_SEQ_STEP:
    case MELLO_UI_RANDOM:
    case MELLO_UI_KEY_SCALE:
    case MELLO_UI_SLOTS:
    case MELLO_UI_DOT:
    case MELLO_UI_GLOBAL:
    case MELLO_UI_SOUND_RESERVED:
    case MELLO_UI_AUDIO_FX:
    case MELLO_UI_SAMPLE_RESERVED: dynamic_body = 0u; break;
    default: break;
    }

    if (save_state(ui) != ui->seen_save_state) ui->dirty |= MELLO_UI_DIRTY_STATUS;
    /* The DOT page is otherwise static, but its read position moves with the transport: redraw on that alone. */
    if (ui->page == MELLO_UI_DOT && ui->seen_dot_cell != e->dot_active_cell[ui->selected_dot_target]) {
        ui->dot_trail[1] = ui->dot_trail[0]; /* where it was: drawn fading */
        ui->dot_trail[0] = ui->seen_dot_cell;
        ui->seen_dot_cell = e->dot_active_cell[ui->selected_dot_target];
        ui->dirty |= MELLO_UI_DIRTY_BODY | MELLO_UI_DIRTY_LEDS;
    }
    if (step_changed || other_changed) {
        if (step_changed) {
            ui->home_old_step = ui->seen_step;
            ui->home_new_step = ui->transport->display_step;
            if (ui->page == MELLO_UI_HOME)
                ui->dirty |= MELLO_UI_DIRTY_HOME_PLAYHEAD;
            else if (ui->page == MELLO_UI_SEQ_STEP || ui->page == MELLO_UI_RANDOM)
                ui->dirty |= MELLO_UI_DIRTY_SEQ_PLAYHEAD;
            else if (dynamic_body)
                ui->dirty |= MELLO_UI_DIRTY_BODY;
        }
        if (other_changed && dynamic_body) ui->dirty |= MELLO_UI_DIRTY_BODY;
        ui->seen_step = ui->transport->display_step;
        ui->seen_pulse = ui->transport->display_pulse;
        ui->seen_segment = ui->transport->display_segment;
        ui->seen_loop_pos = e->loop_cycle_pos;
        ui->seen_slot_seq_pos = e->slot_seq_pos;
        ui->seen_tm_pos = e->tm_active_index;
        ui->seen_sub_x_pos = e->sub_x_active_index;
#if MELLO_FEATURE_CHORD_MACHINE
        if (ui->page == MELLO_UI_HOME && e->chord.on && ui->seen_chord_pos != e->chord_chain_pos)
            ui->dirty |= MELLO_UI_DIRTY_HEADER; /* the chord's name */
        ui->seen_chord_pos = e->chord_chain_pos;
        ui->seen_chord_grid = e->chord_grid_active_cell;
        ui->seen_chord_lit = ui->transport->display_chord_lit;
#endif
        ui->seen_drum_step = e->drum_step;
        ui->seen_running = run;
        ui->dirty |= MELLO_UI_DIRTY_STATUS | MELLO_UI_DIRTY_LEDS;
    }
}

static char *put_u32(char *p, uint32_t v)
{
    char tmp[10];
    uint8_t n = 0u;
    do {
        tmp[n++] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v && n < 10u);
    while (n) *p++ = tmp[--n];
    *p = 0;
    return p;
}

static char *put_i32(char *p, int32_t v)
{
    if (v < 0) {
        *p++ = '-';
        v = -v;
    } else
        *p++ = '+';
    return put_u32(p, (uint32_t)v);
}

static char *copy_s(char *p, const char *s)
{
    while (*s) *p++ = *s++;
    *p = 0;
    return p;
}

static void value_u(char *b, const char *label, uint32_t v)
{
    char *p = copy_s(b, label);
    if (*label) *p++ = ' ';
    put_u32(p, v);
}

static void value_i(char *b, const char *label, int32_t v)
{
    char *p = copy_s(b, label);
    if (*label) *p++ = ' ';
    put_i32(p, v);
}

static void main_reset_name(char *b, uint16_t master_len)
{
    char *p;
    if (master_len == 0u) {
        copy_s(b, "FREE");
        return;
    }
    p = put_u32(b, (uint32_t)master_len / 16u);
    copy_s(p, " BAR");
}

static void midi_note_name(char *b, uint8_t midi)
{
    static const char *n[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    char *p = copy_s(b, n[midi % 12u]);
    put_i32(p, (int32_t)(midi / 12u) - 1);
    /* note octaves read conventionally without a leading '+' */
    p = b;
    while (*p && *p != '+') ++p;
    if (*p == '+') {
        while (*p) {
            *p = *(p + 1);
            ++p;
        }
    }
}

static const char *gate_name(MelloGateMode g)
{
    return g == MELLO_GATE_HOLD    ? "HOLD"
           : g == MELLO_GATE_REST  ? "REST"
           : g == MELLO_GATE_MULTI ? "MULTI"
                                   : "SINGLE";
}

static const char *dot_mode_name(MelloDotMode m)
{
    static const char *n[MELLO_DOT_MODE_COUNT] = {"HORIZ",  "VERT", "SNAKE",  "RANDOM", "P-PONG",
                                                  "SPIRAL", "DIAG", "BOUNCE", "WALK"};
    return (uint32_t)m < MELLO_DOT_MODE_COUNT ? n[m] : "?";
}

static const char *sub_mode_name(MelloSubMode m)
{
    return m == MELLO_SUB_MODE_EUCLID   ? "EUCLID"
           : m == MELLO_SUB_MODE_MATRIX ? "MATRIX"
           : m == MELLO_SUB_MODE_TM     ? "TM"
           : m == MELLO_SUB_MODE_DRONE  ? "DRONE"
                                        : "STEP";
}

static const char *sync_name(MelloSyncMode m)
{
    return m == MELLO_SYNC_SLAVE ? "SLAVE" : m == MELLO_SYNC_MASTER ? "MASTER" : "OFF";
}

/* Every frame with the HTML's rounded corners (a corner pixel left out) once it is big enough to show them. */
static void ui_frame(const MelloUiDrawOps *o, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour)
{
    int16_t r = w >= 6 && h >= 6 ? 1 : 0;
    if (w < 2 || h < 2) return;
    o->fill(o->ctx, (int16_t)(x + r), y, (int16_t)(w - 2 * r), 1, colour);
    o->fill(o->ctx, (int16_t)(x + r), (int16_t)(y + h - 1), (int16_t)(w - 2 * r), 1, colour);
    o->fill(o->ctx, x, (int16_t)(y + r), 1, (int16_t)(h - 2 * r), colour);
    o->fill(o->ctx, (int16_t)(x + w - 1), (int16_t)(y + r), 1, (int16_t)(h - 2 * r), colour);
}

/* The HTML's drawing vocabulary, from rectangles: rounded frames and fills (corners cut by a pixel), vertical
 * gradients, ring-thumb sliders, letter-spaced labels. */
static void ui_round_frame(const MelloUiDrawOps *o, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c)
{
    if (w < 4 || h < 4) {
        ui_frame(o, x, y, w, h, c);
        return;
    }
    o->fill(o->ctx, (int16_t)(x + 1), y, (int16_t)(w - 2), 1, c);
    o->fill(o->ctx, (int16_t)(x + 1), (int16_t)(y + h - 1), (int16_t)(w - 2), 1, c);
    o->fill(o->ctx, x, (int16_t)(y + 1), 1, (int16_t)(h - 2), c);
    o->fill(o->ctx, (int16_t)(x + w - 1), (int16_t)(y + 1), 1, (int16_t)(h - 2), c);
}
static void ui_round_fill(const MelloUiDrawOps *o, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c)
{
    if (w < 4 || h < 4) {
        o->fill(o->ctx, x, y, w, h, c);
        return;
    }
    o->fill(o->ctx, (int16_t)(x + 1), y, (int16_t)(w - 2), h, c);
    o->fill(o->ctx, x, (int16_t)(y + 1), 1, (int16_t)(h - 2), c);
    o->fill(o->ctx, (int16_t)(x + w - 1), (int16_t)(y + 1), 1, (int16_t)(h - 2), c);
}
/* a + (b - a) * t / 256, per RGB565 channel. */
static uint16_t ui_mix(uint16_t a, uint16_t b, uint16_t t)
{
    int32_t ra = a >> 11, ga = (a >> 5) & 63, ba = a & 31, rb = b >> 11, gb = (b >> 5) & 63, bb = b & 31;
    int32_t r = ra + ((rb - ra) * (int32_t)t) / 256, g = ga + ((gb - ga) * (int32_t)t) / 256,
            bl = ba + ((bb - ba) * (int32_t)t) / 256;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}
/* Top to bottom in four bands, as the HTML's step bars. */
static void ui_vgrad(const MelloUiDrawOps *o, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t top,
                     uint16_t bottom)
{
    int16_t i, y0 = y;
    for (i = 0; i < 4; ++i) {
        int16_t y1 = (int16_t)(y + (h * (i + 1)) / 4);
        if (y1 > y0) o->fill(o->ctx, x, y0, w, (int16_t)(y1 - y0), ui_mix(top, bottom, (uint16_t)(i * 85)));
        y0 = y1;
    }
}
/* The HTML's slider: a dark rounded track, a ring thumb (6 x 6 here, outline in the colour). */
static void ui_slider(const MelloUiDrawOps *o, int16_t x, int16_t y, int16_t w, uint16_t value,
                      uint16_t max_value, uint16_t colour)
{
    int16_t tx;
    (void)colour; /* the HTML's thumbs are all its green ring; the colour is the value's text */
    ui_round_fill(o, x, (int16_t)(y + 1), w, 4, UI_TRACK);
    o->fill(o->ctx, (int16_t)(x + 1), (int16_t)(y + 1), (int16_t)(w - 2), 1, UI_LINE); /* the track's edge */
    if (!max_value) return;
    if (value > max_value) value = max_value;
    tx = (int16_t)(x + ((int32_t)(w - 6) * value) / max_value);
    o->fill(o->ctx, (int16_t)(tx + 1), y, 4, 6, UI_ACC);
    o->fill(o->ctx, tx, (int16_t)(y + 1), 6, 4, UI_ACC);
    o->fill(o->ctx, (int16_t)(tx + 2), (int16_t)(y + 2), 2, 2, UI_PANEL);
}
/* A label as the HTML sets them: uppercase, a pixel between letters (9 px a character; plain when it does not fit). */
static void ui_spaced(const MelloUiDrawOps *o, int16_t x, int16_t y, int16_t w, const char *t, uint16_t c,
                      uint8_t align)
{
    o->text(o->ctx, x, y, w, MELLO_UI_TEXT_SPACED, t, c, align);
}

static void ui_hbar(const MelloUiDrawOps *o, int16_t x, int16_t y, int16_t w, uint16_t value,
                    uint16_t max_value, uint16_t colour)
{
    int16_t fillw;
    o->fill(o->ctx, x, y, w, 3, UI_PANEL2);
    if (!max_value) return;
    if (value > max_value) value = max_value;
    fillw = (int16_t)(((int32_t)w * value) / max_value);
    if (fillw > 0) o->fill(o->ctx, x, y, fillw, 3, colour);
}

static void ui_env_graph(const MelloUiDrawOps *o, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t attack,
                         uint16_t decay, uint16_t sustain, uint16_t release, uint16_t colour)
{
    uint32_t total = (uint32_t)attack + decay + release + 500u;
    int16_t ax = (int16_t)(4 + ((uint32_t)(w - 8) * attack) / total);
    int16_t dx = (int16_t)(4 + ((uint32_t)(w - 8) * decay) / total);
    int16_t sx = (int16_t)(4 + ((uint32_t)(w - 8) * 500u) / total);
    int16_t rx = (int16_t)(w - 8 - ax - dx - sx);
    int16_t peak = (int16_t)(y + 4), base = (int16_t)(y + h - 5), sus;
    uint8_t i;
    if (rx < 2) rx = 2;
    sus = (int16_t)(base - ((int32_t)(base - peak) * (sustain > 1000u ? 1000u : sustain)) / 1000);
    o->fill(o->ctx, x, y, w, h, UI_PANEL);
    ui_frame(o, x, y, w, h, UI_PANEL2);
    /* Sparse 32-point trace: enough for a 240 px display without a costly per-pixel redraw. */
    for (i = 0u; i < 32u; ++i) {
        int16_t col = (int16_t)(((int32_t)i * (w - 9)) / 31);
        int16_t py;
        if (col < ax)
            py = (int16_t)(base - ((int32_t)(base - peak) * col) / (ax ? ax : 1));
        else if (col < ax + dx)
            py = (int16_t)(peak + ((int32_t)(sus - peak) * (col - ax)) / (dx ? dx : 1));
        else if (col < ax + dx + sx)
            py = sus;
        else
            py = (int16_t)(sus + ((int32_t)(base - sus) * (col - ax - dx - sx)) / (rx ? rx : 1));
        if (py < peak) py = peak;
        if (py > base) py = base;
        o->fill(o->ctx, (int16_t)(x + 4 + col), py, 3, 2, colour);
    }
}

static uint8_t ui_segment_bounds(const MelloProject *p, uint8_t bounds[MELLO_MAX_SEGMENTS + 1u])
{
    uint8_t n = p->seq_len, sc = p->seg_count, i;
    if (n < 1u) n = 1u;
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

static uint8_t ui_segment_for_step(const MelloProject *p, uint8_t step)
{
    uint8_t bounds[MELLO_MAX_SEGMENTS + 1u], sc, i;
    sc = ui_segment_bounds(p, bounds);
    for (i = 0u; i < sc; ++i)
        if (step >= bounds[i] && step < bounds[i + 1u]) return i;
    return 0u;
}

static uint16_t ui_segment_colour(uint8_t segment)
{
    static const uint16_t c[MELLO_MAX_SEGMENTS] = {UI_SEG_A, UI_SEG_B, UI_SEG_C,
                                                   UI_SEG_D, UI_SEG_E, UI_SEG_F};
    return c[segment < MELLO_MAX_SEGMENTS ? segment : 0u];
}

static uint16_t ui_segment_colour_dim(uint8_t segment)
{
    static const uint16_t c[MELLO_MAX_SEGMENTS] = {UI_SEG_A_DIM, UI_SEG_B_DIM, UI_SEG_C_DIM,
                                                   UI_SEG_D_DIM, UI_SEG_E_DIM, UI_SEG_F_DIM};
    return c[segment < MELLO_MAX_SEGMENTS ? segment : 0u];
}

static uint16_t ui_dot_colour(uint8_t target)
{
    static const uint16_t c[MELLO_DOT_MATRIX_COUNT] = {UI_CORAL, UI_CYAN, UI_MAGENTA, UI_GOLD, UI_BLUE};
    return c[target < MELLO_DOT_MATRIX_COUNT ? target : 0u];
}

static uint16_t ui_page_accent(MelloUi *ui)
{
    MelloEngine *e = eng(ui);
    switch (ui->page) {
    case MELLO_UI_SEQ_SEGMENT: return ui_segment_colour(ui->selected_segment);
    case MELLO_UI_SEQ_LOOP:
    case MELLO_UI_SUB: return UI_GOLD;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: return UI_CHORD;
#endif
    case MELLO_UI_SEQ_SLOTSEQ:
    case MELLO_UI_SLOTS: return UI_BLUE;
    case MELLO_UI_RANDOM: return UI_PURPLE;
    case MELLO_UI_DOT: return ui_dot_colour(ui->selected_dot_target);
    case MELLO_UI_TURING: return UI_PURPLE; /* the MOD family, as the HTML's LFO */
    case MELLO_UI_AUDIO_FX:
        /* As the HTML colours them: distortion coral, delay turquoise, reverb green. */
        return ui->fx_subpage == 0u ? UI_CORAL : (ui->fx_subpage <= 2u ? UI_CYAN : UI_ACC);
    case MELLO_UI_SOUND_RESERVED:
        switch (ui->sound_subpage) {
        case 0u: return UI_CYAN;
        case 2u: return UI_PURPLE;
        case 3u: return UI_GOLD;
        case 5u:
        case 7u: return UI_CORAL;  /* grit and character, as DRIVE */
        case 6u: return UI_CYAN;   /* an oscillator, as OSC */
        case 8u: return UI_PURPLE; /* modulation, as the LFO */
        default: return UI_ACC;
        }
    case MELLO_UI_SAMPLE_RESERVED: return UI_BLUE;
    case MELLO_UI_DRUMS: return UI_CORAL;
    case MELLO_UI_HOME:
    case MELLO_UI_SEQ_STEP:
        if (e) return ui_segment_colour(ui_segment_for_step(&e->project, ui->selected_step));
        return UI_ACC;
    case MELLO_UI_GLOBAL: return UI_ACC; /* the HTML's own green, as its title bar */
    default: return UI_ACC;
    }
}

static uint8_t ui_step_overview_count(const MelloProject *p)
{
    uint8_t n = p && p->seq_len ? p->seq_len : 16u;
    if (n < 16u) n = 16u;
    if (n > MELLO_MAX_STEPS) n = MELLO_MAX_STEPS;
    return n;
}

static int16_t ui_step_overview_x(const MelloProject *p, uint8_t index)
{
    uint8_t n = ui_step_overview_count(p);
    return (int16_t)(8 + ((uint32_t)index * 224u) / n);
}

static int16_t ui_step_overview_w(const MelloProject *p, uint8_t index)
{
    uint8_t n = ui_step_overview_count(p);
    int16_t x0 = ui_step_overview_x(p, index);
    int16_t x1 = (int16_t)(8 + ((uint32_t)(index + 1u) * 224u) / n);
    int16_t w = (int16_t)(x1 - x0 - 1);
    return w < 2 ? 2 : w;
}

static void draw_step_overview_cell(MelloUi *ui, const MelloUiDrawOps *o, uint8_t i, uint8_t clear_bg)
{
    MelloEngine *e = eng(ui);
    int16_t x, cw;
    uint16_t c;
    if (!e || i >= ui_step_overview_count(&e->project)) return;
    cw = ui_step_overview_w(&e->project, i);
    x = ui_step_overview_x(&e->project, i);
    if (clear_bg) o->fill(o->ctx, x, 35, cw, 12, UI_BLACK);
    c = i < e->project.seq_len ? UI_PANEL2 : UI_BLACK;
    o->fill(o->ctx, x, 36, cw, 10, c);
    if (i < e->project.seq_len) {
        uint8_t seg = ui_segment_for_step(&e->project, i);
        o->fill(o->ctx, x, 42, cw, 2,
                e->project.steps[i].skip ? ui_segment_colour_dim(seg) : ui_segment_colour(seg));
    }
    if (i == ui->selected_step) ui_frame(o, x, 35, cw, 12, UI_SOFT);
    if (ui->transport->display_step == i) ui_frame(o, x, 36, cw, 10, UI_WHITE);
}

static void home_pitch_range(const MelloProject *p, uint8_t *lo, uint8_t *hi)
{
    uint8_t i, n = p->seq_len > MELLO_MAX_STEPS ? MELLO_MAX_STEPS : p->seq_len;
    uint8_t mn = 108u, mx = 12u, desired;
    int16_t center, vlo, vhi;
    if (!n) {
        *lo = 48u;
        *hi = 72u;
        return;
    }
    for (i = 0u; i < n; ++i) {
        uint8_t m = mello_resolve_step_midi(p, &p->steps[i]);
        if (m < mn) mn = m;
        if (m > mx) mx = m;
    }
    desired =
        (uint8_t)((p->pitch_span_oct < 1u ? 1u : (p->pitch_span_oct > 3u ? 3u : p->pitch_span_oct)) * 12u);
    if ((uint8_t)(mx - mn) < desired) {
        center = (int16_t)(((uint16_t)mn + (uint16_t)mx) / 2u);
        vlo = (int16_t)(center - desired / 2);
        vhi = (int16_t)(vlo + desired);
        if (vlo < 12) {
            vhi += (int16_t)(12 - vlo);
            vlo = 12;
        }
        if (vhi > 108) {
            vlo -= (int16_t)(vhi - 108);
            vhi = 108;
            if (vlo < 12) vlo = 12;
        }
        mn = (uint8_t)vlo;
        mx = (uint8_t)vhi;
    } else {
        if (mn > 12u) --mn;
        if (mx < 108u) ++mx;
    }
    if (mx <= mn) {
        mn = 48u;
        mx = 72u;
    }
    *lo = mn;
    *hi = mx;
}

static uint8_t random_skyline_count(const MelloProject *p)
{
    uint8_t n = p && p->seq_len ? p->seq_len : 4u;
    if (n < 4u) n = 4u;
    if (n > MELLO_MAX_STEPS) n = MELLO_MAX_STEPS;
    return n;
}

static int16_t random_skyline_x(const MelloProject *p, uint8_t i)
{
    uint8_t n = random_skyline_count(p);
    return (int16_t)(8 + ((uint32_t)i * 224u) / n);
}

static int16_t random_skyline_w(const MelloProject *p, uint8_t i)
{
    uint8_t n = random_skyline_count(p);
    int16_t x0 = random_skyline_x(p, i);
    int16_t x1 = (int16_t)(8 + ((uint32_t)(i + 1u) * 224u) / n);
    int16_t w = (int16_t)(x1 - x0 - 1);
    return w < 2 ? 2 : w;
}

static void draw_random_playhead_marker(MelloUi *ui, const MelloUiDrawOps *o, uint8_t i)
{
    MelloEngine *e = eng(ui);
    int16_t x, w;
    if (!e || i >= random_skyline_count(&e->project)) return;
    x = random_skyline_x(&e->project, i);
    w = random_skyline_w(&e->project, i);
    o->fill(o->ctx, x, 129, w, 2, UI_BLACK);
    if (ui->transport->display_step == i)
        o->fill(o->ctx, x, 129, w, 2, UI_WHITE);
    else if (ui->selected_step == i)
        o->fill(o->ctx, x, 129, w, 1, UI_SOFT);
}

static void draw_random_skyline(MelloUi *ui, const MelloUiDrawOps *o)
{
    MelloEngine *e = eng(ui);
    uint8_t i, n, lo, hi;
    if (!e) return;
    n = random_skyline_count(&e->project);
    home_pitch_range(&e->project, &lo, &hi);
    o->fill(o->ctx, 8, 129, 224, 39, UI_PANEL2);
    for (i = 0u; i < n; ++i) {
        MelloStep *st = &e->project.steps[i];
        int16_t x = random_skyline_x(&e->project, i), w = random_skyline_w(&e->project, i),
                barw = w > 3 ? (int16_t)(w - 2) : w;
        uint8_t midi = mello_resolve_step_midi(&e->project, st);
        int16_t h = (int16_t)(5 + ((int32_t)(midi - lo) * 22) / (hi > lo ? (int32_t)(hi - lo) : 1));
        int16_t top;
        uint8_t seg = ui_segment_for_step(&e->project, i);
        uint16_t col = st->skip ? ui_segment_colour_dim(seg) : ui_segment_colour(seg);
        int16_t pw;
        if (h < 5) h = 5;
        if (h > 27) h = 27;
        top = (int16_t)(158 - h);
        o->fill(o->ctx, x, 132, w, 27, UI_PANEL);
        o->fill(o->ctx, x + (w - barw) / 2, top, barw, h, col);
        pw = (int16_t)(((uint32_t)(st->pulses > 8u ? 8u : st->pulses) * (uint32_t)w + 7u) / 8u);
        if (pw < 1) pw = 1;
        if (pw > w) pw = w;
        o->fill(o->ctx, x, 161, pw, 1, UI_SOFT);
        if (st->ratchet > 1u) o->fill(o->ctx, x, 163, w, 1, UI_MAGENTA);
        if (st->accent) o->fill(o->ctx, x, 165, w > 2 ? 2 : w, 1, UI_CORAL);
        if (st->sub) o->fill(o->ctx, (int16_t)(x + (w > 3 ? 2 : 0)), 165, w > 4 ? 2 : 1, 1, UI_GOLD);
        if (st->slide) o->fill(o->ctx, (int16_t)(x + (w > 5 ? 4 : (w - 1))), 165, 1, 1, UI_BLUE);
        if (st->skip) o->fill(o->ctx, x, 168, w, 1, UI_RED);
        draw_random_playhead_marker(ui, o, i);
    }
}

/* One HOME lane, as the HTML's step lanes: a dark panel with a cap in the segment's colour, the bar rising by pitch
 * in a gradient with the note's letter in it (a mark beside it for a sharp), the playing step brighter, the
 * selected one framed, three dots under it (accent, sub, slide / ratchet). */
static void draw_home_step(MelloUi *ui, const MelloUiDrawOps *o, uint8_t i)
{
    static const char letters[12] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
    static const uint8_t sharp[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
    MelloEngine *e = eng(ui);
    MelloStep *s;
    uint8_t lo, hi, midi, active, selected, segment;
    int16_t x, w, lane_y = 50, lane_h = 98, base_y = 146, top_y, h;
    uint16_t seg, top, bottom;
    if (!e || i >= ui_step_overview_count(&e->project)) return;
    x = ui_step_overview_x(&e->project, i);
    w = ui_step_overview_w(&e->project, i);
    o->fill(o->ctx, x, 46, w, 108, UI_BLACK);
    if (i >= e->project.seq_len) {
        ui_round_frame(o, x, lane_y, w, lane_h, UI_TRACK);
        return;
    }
    s = &e->project.steps[i];
    segment = ui_segment_for_step(&e->project, i);
    seg = ui_segment_colour(segment);
    active = ui->transport->running && ui->transport->display_step == i;
    selected = ui->selected_step == i && (ui->held_step != MELLO_UI_NO_STEP || !ui->transport->running);
    o->fill(o->ctx, x, 46, w, 2, active ? seg : ui_mix(seg, UI_BLACK, 150)); /* the cap */
    ui_round_fill(o, x, lane_y, w, lane_h, selected ? ui_mix(UI_PANEL, seg, 24) : UI_PANEL);
    ui_round_frame(o, x, lane_y, w, lane_h, selected ? seg : UI_LINE);
    home_pitch_range(&e->project, &lo, &hi);
    midi = mello_resolve_step_midi(&e->project, s);
    h = (int16_t)(6 + ((int32_t)(midi - lo) * 82) / (hi > lo ? (int32_t)(hi - lo) : 1));
    if (h > 92) h = 92;
    top_y = (int16_t)(base_y - h);
    if (s->skip) { /* off: an outline where the bar would be */
        ui_frame(o, (int16_t)(x + 1), top_y, (int16_t)(w - 2), h, ui_mix(seg, UI_BLACK, 190));
    } else {
        top = active ? seg : ui_mix(seg, UI_BLACK, 125);
        bottom = active ? ui_mix(seg, UI_BLACK, 150) : ui_mix(seg, UI_BLACK, 195);
        if (h >= 30 &&
            w >= 10) { /* the letter's band solid, so it sits on its own colour; the gradient below */
            o->fill(o->ctx, (int16_t)(x + 1), top_y, (int16_t)(w - 2), 19, top);
            ui_vgrad(o, (int16_t)(x + 1), (int16_t)(top_y + 19), (int16_t)(w - 2), (int16_t)(h - 19),
                     ui_mix(top, bottom, 60), bottom);
        } else
            ui_vgrad(o, (int16_t)(x + 1), top_y, (int16_t)(w - 2), h, top, bottom);
        o->fill(o->ctx, (int16_t)(x + 1), top_y, (int16_t)(w - 2), 1, active ? UI_BRIGHT : seg);
    }
    if (w >= 10) { /* the note's letter, in the bar when it is tall enough, else over it */
        char t[2] = {letters[midi % 12u], 0};
        int16_t ty = h >= 30 ? (int16_t)(top_y + 2) : (int16_t)(top_y - 17);
        uint16_t tc = s->skip ? UI_DIM : (h >= 30 ? UI_NOTE : UI_MID);
        if (ty < lane_y + 1) ty = (int16_t)(lane_y + 1);
        o->text(o->ctx, (int16_t)(x + (w - 8) / 2), ty, 8, 0, t, tc, 0);
        if (sharp[midi % 12u]) o->fill(o->ctx, (int16_t)(x + (w - 8) / 2 + 8), (int16_t)(ty + 3), 2, 2, tc);
    }
    { /* the HTML's three dots: accent, sub, slide (ratchet magenta) */
        int16_t dx = (int16_t)(x + w / 2 - 5);
        o->fill(o->ctx, dx, 150, 2, 2, s->accent ? UI_CORAL : UI_LINE);
        o->fill(o->ctx, (int16_t)(dx + 4), 150, 2, 2, s->sub ? UI_GOLD : UI_LINE);
        o->fill(o->ctx, (int16_t)(dx + 8), 150, 2, 2,
                s->ratchet > 1u ? UI_MAGENTA : (s->slide ? UI_BLUE : UI_LINE));
    }
}

static void draw_home(MelloUi *ui, const MelloUiDrawOps *o)
{
    MelloEngine *e = eng(ui);
    uint8_t i, bounds[MELLO_MAX_SEGMENTS + 1u], sc, focus_seg;
    char b[24], *p;
    if (!e) return;
    o->fill(o->ctx, 0, 30, 240, 142, UI_BLACK);
    sc = ui_segment_bounds(&e->project, bounds);
    focus_seg = ui_segment_for_step(&e->project, ui->selected_step);
    for (i = 0u; i < sc; ++i) {
        uint8_t bs = bounds[i], be = bounds[i + 1u], vn = ui_step_overview_count(&e->project);
        int16_t x, x2;
        uint16_t c;
        if (bs >= vn) continue;
        if (be > vn) be = vn;
        x = (int16_t)(8 + ((uint32_t)bs * 224u) / vn);
        x2 = (int16_t)(8 + ((uint32_t)be * 224u) / vn - 2);
        c = ui_segment_colour(i);
        /* the segment as the HTML's segment boxes: a thin rounded bracket, its letter in the middle */
        ui_round_frame(o, x, 37, (int16_t)(x2 - x + 1), 6, i == focus_seg ? c : ui_mix(c, UI_BLACK, 140));
        b[0] = (char)('A' + i);
        b[1] = 0;
        o->text(o->ctx, x, 30, (int16_t)(x2 - x + 1), 0, b, i == focus_seg ? UI_BRIGHT : c, 1);
    }
    for (i = 0u; i < ui_step_overview_count(&e->project); ++i) draw_home_step(ui, o, i);
    {
        uint8_t si = ui->selected_step < e->project.seq_len ? ui->selected_step : 0u;
        MelloStep *s = &e->project.steps[si];
        uint32_t prob = ((uint32_t)s->probability * 100u + 32767u) / 65535u;
        const char *gate_short = s->gate == MELLO_GATE_HOLD    ? "HLD"
                                 : s->gate == MELLO_GATE_REST  ? "RST"
                                 : s->gate == MELLO_GATE_MULTI ? "MUL"
                                                               : "SGL";
        p = copy_s(b, "S");
        p = put_u32(p, (uint32_t)si + 1u);
        p = copy_s(p, " ");
        midi_note_name(p, mello_resolve_step_midi(&e->project, s));
        o->text(o->ctx, 8, 156, 96, 0, b, UI_BRIGHT, 0);
        p = copy_s(b, "P");
        p = put_u32(p, s->pulses);
        p = copy_s(p, " R");
        p = put_u32(p, s->ratchet);
        p = copy_s(p, " ");
        p = copy_s(p, gate_short);
        p = copy_s(p, " Q");
        put_u32(p, prob);
        o->text(o->ctx, 108, 156, 124, 0, b, UI_MID, 2);
    }
}

static void draw_home_playhead(MelloUi *ui, const MelloUiDrawOps *o)
{
    if (ui->home_old_step < MELLO_MAX_STEPS) draw_home_step(ui, o, ui->home_old_step);
    if (ui->home_new_step < MELLO_MAX_STEPS && ui->home_new_step != ui->home_old_step)
        draw_home_step(ui, o, ui->home_new_step);
}

static void draw_sequence_playhead(MelloUi *ui, const MelloUiDrawOps *o)
{
    uint8_t old_step = ui->home_old_step, new_step = ui->home_new_step;
    if (ui->page == MELLO_UI_SEQ_STEP) {
        if (old_step < MELLO_MAX_STEPS) draw_step_overview_cell(ui, o, old_step, 1u);
        if (new_step < MELLO_MAX_STEPS && new_step != old_step) draw_step_overview_cell(ui, o, new_step, 1u);
    } else if (ui->page == MELLO_UI_RANDOM) {
        if (old_step < MELLO_MAX_STEPS) draw_random_playhead_marker(ui, o, old_step);
        if (new_step < MELLO_MAX_STEPS && new_step != old_step) draw_random_playhead_marker(ui, o, new_step);
    }
}

static void draw_header(MelloUi *ui, const MelloUiDrawOps *o)
{
    MelloEngine *e = eng(ui);
    char b[32], status[12], *p;
    uint16_t accent = ui_page_accent(ui);
    o->fill(o->ctx, 0, 0, 240, 30, UI_PANEL);
    if (e) { /* the key, where the wordmark was: the HTML's title line ("C minor") */
        char key[24], *q;
        static const char *n[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        static const char *sc[MELLO_SCALE_COUNT] = {"CHROM", "MINOR", "DORIAN", "PHRYG", "LYDIAN", "MIXO",
                                                    "PENTA", "mPENT", "DIM",    "WHOLE", "AUG",    "MAJOR"};
        q = copy_s(key, n[e->project.root_note % 12u]);
        q = copy_s(q, " ");
        (void)copy_s(q, sc[e->project.scale < MELLO_SCALE_COUNT ? e->project.scale : MELLO_SCALE_MINOR]);
        o->text(o->ctx, 4, 7, 84, 0, key, UI_GOLD, 0);
    }
    p = copy_s(b, ui->page == MELLO_UI_HOME && ui->keys_mode ? (ui->keys_rec ? "HOME REC" : "HOME KEYS")
                                                             : domain_name(ui));
#if MELLO_FEATURE_CHORD_MACHINE
    if (e && ui->page == MELLO_UI_HOME && !ui->keys_mode && e->chord.on && ui->transport->running &&
        e->chord_voiced_slot < e->chord.length) { /* HOME: the chord sounding, after the page's name */
        char cn[8];
        mello_engine_chord_name(e, e->chord_voiced_slot, cn);
        *p++ = ' ';
        p = copy_s(p, cn);
    } else
#endif
    if (e && !ui->keys_mode &&
        (ui->page == MELLO_UI_HOME || ui->page == MELLO_UI_SEQ_STEP || ui->page == MELLO_UI_SEQ_SEGMENT)) {
        uint8_t seg = ui->page == MELLO_UI_SEQ_SEGMENT ? ui->selected_segment
                                                       : ui_segment_for_step(&e->project, ui->selected_step);
        *p++ = ' ';
        *p++ = (char)('A' + (seg < MELLO_MAX_SEGMENTS ? seg : 0u));
        *p = 0;
    }
    ui_spaced(o, 88, 7, 100, b, accent, 1);
    /* the HTML's "● PLAYING": a dot lit while playing, then the tempo */
    o->fill(o->ctx, 200, 13, 4, 4, ui->transport->running ? UI_ACC : UI_LINE);
    o->fill(o->ctx, 201, 12, 2, 6, ui->transport->running ? UI_ACC : UI_LINE);
    o->fill(o->ctx, 199, 14, 6, 2, ui->transport->running ? UI_ACC : UI_LINE);
    p = status;
    put_u32(p, mello_transport_effective_bpm(ui->transport));
    o->text(o->ctx, 208, 7, 28, 0, status, ui->transport->running ? UI_ACC : UI_SOFT, 2);
    o->fill(o->ctx, 7, 28, 226, 1, accent);
}

static void footer_texts(MelloUi *ui, const char *l[4], char v[4][24]);
static bool knob_norm(MelloUi *ui, uint8_t k, uint16_t *val, uint16_t *max);
static uint16_t knob_colour(MelloUi *ui, uint8_t k);
static uint16_t ui_text_len(const char *s);

/* The HTML's section: a rounded panel from y to the strip, its title spaced in the page's colour, the layer on the
 * right. */
static void ui_section(MelloUi *ui, const MelloUiDrawOps *o, int16_t y, const char *title, uint16_t accent)
{
    const char *layer = layer_name(ui);
    ui_round_fill(o, 4, y, 232, (int16_t)(169 - y), UI_PANEL);
    ui_round_frame(o, 4, y, 232, (int16_t)(169 - y), ui_mix(accent, UI_BLACK, 150));
    ui_spaced(o, 10, (int16_t)(y + 3), 124, title, accent, 0);
    if (layer) ui_spaced(o, 136, (int16_t)(y + 3), 96, layer, UI_DIM, 2);
}

/* A pill as the HTML's buttons: on = filled in a dark shade of its colour with a bright edge, off = a dim edge. */
static void ui_pill(const MelloUiDrawOps *o, int16_t x, int16_t y, int16_t w, const char *t, bool on,
                    uint16_t c)
{
    /* 18 high from a pixel above the text: the text's own 16 px background leaves the edges standing */
    if (on) ui_round_fill(o, x, (int16_t)(y - 1), w, 18, ui_mix(c, UI_BLACK, 150));
    ui_round_frame(o, x, (int16_t)(y - 1), w, 18, on ? c : UI_LINE);
    o->text(o->ctx, (int16_t)(x + 2), y, (int16_t)(w - 4), 0, t, on ? UI_BRIGHT : UI_DIM, 1);
}

/* K1-K4 as the HTML's controls, two by two under the page's picture (rows at y and y + 26): the label left and the
 * value right, a slider under it where the value has a range, the value in a pill where it is a word. The same texts
 * as the footer and the readout (footer_texts). */
static void draw_knob_grid(MelloUi *ui, const MelloUiDrawOps *o, int16_t y)
{
    char v[4][24];
    const char *l[4];
    uint8_t k;
    footer_texts(ui, l, v);
    for (k = 0u; k < 4u; ++k) {
        int16_t x = (int16_t)(10 + (k & 1u) * 114u), cy = (int16_t)(y + (k >> 1) * 26), w = 106,
                vw = (int16_t)(ui_text_len(v[k]) * 8u);
        uint16_t c = knob_colour(ui, k), nv, nm;
        if (!v[k][0] || (v[k][0] == '-' && !v[k][1])) continue; /* an empty knob */
        if (knob_norm(ui, k, &nv, &nm)) {
            ui_spaced(o, x, cy, (int16_t)(w - vw - 4), l[k], UI_DIM, 0);
            o->text(o->ctx, (int16_t)(x + w - vw), cy, vw, 0, v[k], c, 2);
            ui_slider(o, x, (int16_t)(cy + 17), w, nv, nm, c);
        } else {
            int16_t pw = (int16_t)(vw + 8);
            if (pw > w - 30) pw = (int16_t)(w - 30);
            ui_spaced(o, x, cy, (int16_t)(w - pw - 2), l[k], UI_DIM, 0);
            ui_round_frame(o, (int16_t)(x + w - pw), (int16_t)(cy - 1), pw, 18, ui_mix(c, UI_BLACK, 90));
            o->text(o->ctx, (int16_t)(x + w - pw + 4), cy, (int16_t)(pw - 8), 0, v[k], c, 1);
        }
    }
}

static void draw_body(MelloUi *ui, const MelloUiDrawOps *o)
{
    MelloEngine *e = eng(ui);
    char a[48], b[48];
    if (!e) return;
    o->fill(o->ctx, 0, 30, 240, 142, UI_BLACK);
    switch (ui->page) {
    case MELLO_UI_HOME: draw_home(ui, o); break;
    case MELLO_UI_SEQ_STEP: {
        /* The HTML's STEP DETAIL: the sequence strip, then a panel with the step's note in its segment's colour, its
         * flags as pills (lit when on), its pulses, ratchets, gate, chance and sample, and K1-K4 below. */
        MelloStep *s = &e->project.steps[ui->selected_step];
        uint8_t i, vis = ui_step_overview_count(&e->project),
                   seg = ui_segment_for_step(&e->project, ui->selected_step);
        uint16_t sc = ui_segment_colour(seg);
        char *q;
        for (i = 0u; i < vis; ++i) draw_step_overview_cell(ui, o, i, 0u);
        q = copy_s(a, "STEP ");
        q = put_u32(q, (uint32_t)ui->selected_step + 1u);
        q = copy_s(q, "  ");
        midi_note_name(q, mello_resolve_step_midi(&e->project, s));
        ui_section(ui, o, 50, a, sc);
        {
            static const char *fl[4] = {"SLIDE", "ACC", "SUB", "SKIP"};
            static const uint16_t fc[4] = {UI_BLUE, UI_CORAL, UI_GOLD, UI_RED};
            bool on[4];
            on[0] = s->slide, on[1] = s->accent, on[2] = s->sub, on[3] = s->skip;
            for (i = 0u; i < 4u; ++i) ui_pill(o, (int16_t)(10 + i * 56u), 71, 52, fl[i], on[i], fc[i]);
        }
        {
            /* pulses and ratchets as the HTML's 1-8 rows, in small: lit up to the value */
            for (i = 0u; i < 8u; ++i) {
                ui_round_fill(o, (int16_t)(10 + i * 9u), 93, 7, 12, i < s->pulses ? UI_ACC : UI_TRACK);
                ui_round_fill(o, (int16_t)(88 + i * 9u), 93, 7, 12, i < s->ratchet ? UI_MAGENTA : UI_TRACK);
            }
            q = copy_s(b, s->sample_slot ? "SMP " : "SMP -");
            if (s->sample_slot) *q++ = (char)('A' + s->sample_slot - 1u), *q = 0;
            o->text(o->ctx, 164, 91, 66, 0, b, s->sample_slot ? UI_BLUE : UI_DIM, 2);
        }
        draw_knob_grid(ui, o, 118);
        break;
    }
    case MELLO_UI_SEQ_SEGMENT: {
        /* Segments A-F over the sequence, in their colours: each one's letter (and its order, where it is not
         * forwards) over its steps, a cut where one starts, the playing step framed, a skipped one struck through;
         * under them each segment as a pill with its length (x its repeats); K1-K4 below. */
        static const char *mark[4] = {"", "<", "<>", "?"};
        uint8_t s = ui->selected_segment, i, bounds[MELLO_MAX_SEGMENTS + 1u], sc, vis;
        char *q;
        sc = ui_segment_bounds(&e->project, bounds);
        if (s >= sc) s = 0u;
        vis = ui_step_overview_count(&e->project);
        q = copy_s(a, "SEGMENT ");
        *q++ = (char)('A' + s);
        *q++ = ' ';
        q = put_u32(q, (uint32_t)bounds[s] + 1u);
        q = copy_s(q, "-");
        (void)put_u32(q, bounds[s + 1u]);
        ui_section(ui, o, 32, a, ui_segment_colour(s));
        for (i = 0u; i < vis; ++i) {
            uint8_t seg = 0xFFu, j;
            int16_t cw = ui_step_overview_w(&e->project, i), x = ui_step_overview_x(&e->project, i);
            for (j = 0u; j < sc; ++j)
                if (i >= bounds[j] && i < bounds[j + 1u]) seg = j;
            if (i >= e->project.seq_len || seg == 0xFFu) continue;
            o->fill(o->ctx, x, 70, (int16_t)(cw - 1), 12,
                    e->project.seg_skip[seg] ? UI_PANEL2
                    : seg == s               ? ui_mix(ui_segment_colour(seg), UI_PANEL, 120)
                                             : ui_segment_colour_dim(seg));
            if (e->project.seg_skip[seg]) o->fill(o->ctx, x, 75, (int16_t)(cw - 1), 2, UI_RED);
            if (seg > 0u && i == bounds[seg]) o->fill(o->ctx, (int16_t)(x - 1), 66, 1, 20, UI_BRIGHT); /* a cut */
            if (ui->transport->display_step == i) ui_frame(o, x, 70, (int16_t)(cw - 1), 12, UI_WHITE);
        }
        for (i = 0u; i < sc; ++i) {
            int16_t bx = (int16_t)(8 + ((uint32_t)bounds[i] * 224u) / vis),
                    bx2 =
                        (int16_t)(8 + ((uint32_t)(bounds[i + 1u] > vis ? vis : bounds[i + 1u]) * 224u) / vis);
            if (bounds[i] >= vis) continue;
            a[0] = (char)('A' + i), a[1] = 0;
            (void)copy_s(a + 1, mark[segment_order(&e->project, i)]);
            o->text(o->ctx, bx, 52, (int16_t)(bx2 - bx), 0, a, i == s ? UI_BRIGHT : ui_segment_colour(i), 1);
        }
        for (i = 0u; i < sc; ++i) {
            a[0] = (char)('A' + i);
            q = put_u32(a + 1, (uint32_t)(bounds[i + 1u] - bounds[i]));
            if (e->project.seg_repeats[i] > 1u) {
                *q++ = 'x';
                (void)put_u32(q, e->project.seg_repeats[i]);
            }
            ui_pill(o, (int16_t)(4 + i * 39u), 88, 36, a, i == s, ui_segment_colour(i));
        }
        draw_knob_grid(ui, o, 118);
        break;
    }
    case MELLO_UI_SEQ_LOOP: {
        /* The loop cycle's eight slots as the HTML's SLOT cards, in small: each one's hold as a bar, its root
         * offset as a gold line, an octave mark; the playing one lit, the selected one framed. */
        uint8_t i, len = e->loop_cycle.length;
        uint16_t lc = e->loop_cycle.active ? UI_GOLD : UI_BLUE;
        if (len < 2u) len = 2u;
        if (len > MELLO_MAX_LOOP_CYCLE_STEPS) len = MELLO_MAX_LOOP_CYCLE_STEPS;
        ui_section(ui, o, 32, e->loop_cycle.active ? "LOOP CYCLE" : "LOOP: AUDITION", lc);
        for (i = 0u; i < MELLO_MAX_LOOP_CYCLE_STEPS; ++i) {
            MelloLoopCycleStep *ls = &e->loop_cycle.steps[i];
            int16_t x = (int16_t)(10 + i * 28u),
                    hold_h = (int16_t)(3 + ((uint32_t)(ls->hold > 128u ? 128u : ls->hold) * 28u) / 128u),
                    root_y = (int16_t)(70 - ls->root_offset);
            bool in_len = i < len, active = i == e->loop_cycle_pos, selected = i == ui->selected_loop_step;
            ui_round_fill(o, x, 52, 24, 38, in_len ? UI_PANEL2 : UI_PANEL);
            ui_round_frame(o, x, 52, 24, 38,
                           active ? lc : (selected ? UI_BRIGHT : (in_len ? UI_LINE : UI_TRACK)));
            if (in_len) {
                ui_vgrad(o, (int16_t)(x + 3), (int16_t)(87 - hold_h), 18, hold_h, active ? lc : UI_MID,
                         ui_mix(active ? lc : UI_MID, UI_PANEL2, 160));
                if (root_y < 54) root_y = 54;
                if (root_y > 86) root_y = 86;
                o->fill(o->ctx, (int16_t)(x + 3), root_y, 18, 2, UI_GOLD);
                if (ls->octave) o->fill(o->ctx, (int16_t)(x + 3), ls->octave > 0 ? 54 : 86, 18, 2, UI_BLUE);
            }
            a[0] = (char)('1' + i), a[1] = 0;
            o->text(o->ctx, x, 91, 24, 0, a, active ? lc : (selected ? UI_BRIGHT : UI_DIM), 1);
        }
        draw_knob_grid(ui, o, 118);
        break;
    }
    case MELLO_UI_SEQ_SLOTSEQ: {
        /* Sixteen positions of the slot sequence (this bank), two rows: the slot each plays, in its colour, and its
         * hold as a bar; the playing one framed bright. */
        uint8_t i, start = (uint8_t)(ui->slot_seq_bank * 16u), len = e->slot_seq.length;
        ui_section(ui, o, 32, e->slot_seq.active ? "SLOT SEQ ON" : "SLOT SEQ OFF",
                   e->slot_seq.active ? UI_BLUE : UI_DIM);
        for (i = 0u; i < 16u; ++i) {
            uint8_t idx = (uint8_t)(start + i);
            MelloSlotSeqStep *ss = &e->slot_seq.steps[idx];
            int16_t x = (int16_t)(10 + (i % 8u) * 28u), y = (int16_t)(52 + (i / 8u) * 27u);
            bool in_len = idx < len, active = e->slot_seq.active && idx == e->slot_seq_pos,
                 selected = idx == ui->selected_slot_seq_step;
            static const uint16_t shade[4] = {UI_ACC, UI_BLUE, UI_PURPLE, UI_GOLD};
            uint16_t c = ss->slot >= 0 ? shade[(uint8_t)ss->slot % 4u] : UI_LINE;
            ui_round_fill(o, x, y, 24, 25,
                          in_len && ss->slot >= 0 ? ui_mix(c, UI_PANEL, 150)
                                                  : (in_len ? UI_PANEL2 : UI_PANEL));
            ui_round_frame(o, x, y, 24, 25,
                           active ? UI_WHITE : (selected ? UI_BRIGHT : (in_len ? UI_LINE : UI_TRACK)));
            if (in_len && ss->slot >= 0) {
                value_u(a, "", (uint32_t)ss->slot + 1u);
                o->text(o->ctx, x, (int16_t)(y + 3), 24, 0, a, UI_BRIGHT, 1);
            }
            if (in_len)
                o->fill(o->ctx, (int16_t)(x + 3), (int16_t)(y + 20),
                        (int16_t)(2 + ((uint32_t)(ss->hold > 256u ? 256u : ss->hold) * 16u) / 256u), 2, c);
        }
        draw_knob_grid(ui, o, 118);
        break;
    }
    case MELLO_UI_RANDOM: {
#if MELLO_FEATURE_CHORD_MACHINE
        static const char *rn[13] = {"PIT", "PUL", "RAT", "GAT", "OCT", "ACC", "SUB",
                                     "SLD", "SKP", "PRB", "SMP", "CHD", "WAVE"};
        static const uint16_t rc[13] = {UI_ACC,  UI_ACC, UI_MAGENTA, UI_ACC,  UI_BLUE,  UI_CORAL, UI_GOLD,
                                        UI_BLUE, UI_RED, UI_PURPLE,  UI_SOFT, UI_CHORD, UI_BLUE};
#else
        static const char *rn[12] = {"PIT", "PUL", "RAT", "GAT", "OCT", "ACC",
                                     "SUB", "SLD", "SKP", "PRB", "SMP", "WAVE"};
        static const uint16_t rc[12] = {UI_ACC,  UI_ACC,  UI_MAGENTA, UI_ACC,    UI_BLUE, UI_CORAL,
                                        UI_GOLD, UI_BLUE, UI_RED,     UI_PURPLE, UI_SOFT, UI_BLUE};
#endif
        /* The RANDOMIZER as a panel: its targets as pills. MASK: lit = rolled. LINKS: PIT is the source (the skyline
         * below is the pitches), and a target that follows it is filled with "+", one that mirrors it with "-". */
        uint8_t i, enabled = 0u, rn_count = random_target_count();
        int16_t x, y, w;
        ui_section(ui, o, 32,
                   ui->random_edit ? "AMOUNTS" : (ui->random_links ? "LINKS TO PITCH" : "RANDOMIZER"),
                   UI_PURPLE);
        for (i = 0u; i < rn_count; ++i) {
            uint16_t bit = random_target_bit(i);
            bool on = (e->randomize.mask & bit) != 0u, fol = (e->modr.rnd_follow & bit) != 0u,
                 mir = (e->modr.rnd_mirror & bit) != 0u, linkable = (bit & MELLO_RAND_LINKABLE) != 0u;
            uint8_t row = i < 7u ? 0u : 1u, col = row == 0u ? i : (uint8_t)(i - 7u);
            char lab[5];
            w = (i == (uint8_t)(rn_count - 1u)) ? 38 : 30; /* WAVE: four letters */
            x = (int16_t)(10 + col * 31);
            y = (int16_t)(53 + row * 21);
            if (on) ++enabled;
            copy_s(lab, rn[i]);
            if (ui->random_edit) { /* the picked target lit; each one's amount as a bar under it */
                uint8_t amt = e->modr.rnd_amount[random_target_bitno(i)];
                ui_pill(o, x, y, w, lab, i == ui->random_sel, on ? rc[i] : UI_LINE);
                o->fill(o->ctx, x, (int16_t)(y + 18), w, 2, UI_TRACK);
                o->fill(o->ctx, x, (int16_t)(y + 18), (int16_t)(w * amt / 100), 2, on ? rc[i] : UI_LINE);
            } else if (!ui->random_links) {
                ui_pill(o, x, y, w, lab, on, rc[i]);
            } else if (i == 0u) { /* PIT: the source */
                ui_pill(o, x, y, w, lab, true, UI_PURPLE);
            } else if (linkable) {
                if (fol || mir) lab[2] = fol ? '+' : '-', lab[3] = 0;
                ui_pill(o, x, y, w, lab, fol || mir, mir ? UI_CORAL : rc[i]);
            } else
                o->text(o->ctx, (int16_t)(x + 2), y, (int16_t)(w - 4), 0, lab, UI_TRACK, 1); /* cannot link */
        }
        if (ui->random_edit)
            o->text(o->ctx, 10, 95, 220, 0, "KEY PICKS  K1 HOW MUCH", UI_DIM, 0);
        else if (ui->random_links)
            o->text(o->ctx, 10, 95, 220, 0, "+ FOLLOWS  - MIRRORS PITCH", UI_DIM, 0);
        else {
            value_u(a, "ROLLS", enabled);
            o->text(o->ctx, 10, 95, 80, 0, a, UI_MID, 0);
            o->text(o->ctx, 92, 95, 138, 0, random_intensity_name(ui->random_intensity), UI_PURPLE, 2);
        }
        draw_random_skyline(ui, o);
        break;
    }
    case MELLO_UI_KEY_SCALE: {
        uint8_t i, rootpc = (uint8_t)(e->project.root_note % 12u);
        o->text(o->ctx, 8, 33, 104, 0, mello_scale_name(e->project.scale), UI_PURPLE, 0);
        value_u(a, "ROOT", e->project.root_note);
        o->text(o->ctx, 116, 33, 56, 0, a, UI_ACC, 1);
        value_i(a, "OCT", e->project.global_octave);
        o->text(o->ctx, 176, 33, 56, 0, a, UI_GOLD, 2);
        /* Chromatic root locator: black/white key geometry without duplicating engine scale tables. */
        for (i = 0u; i < 12u; ++i) {
            static const uint8_t black[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
            int16_t x = (int16_t)(8 + i * 18u);
            int16_t h = black[i] ? 40 : 62;
            uint8_t root = i == rootpc;
            o->fill(o->ctx, x, 58, 15, h, root ? UI_ACC : (black[i] ? UI_PANEL2 : UI_MID));
            ui_frame(o, x, 58, 15, h, root ? UI_WHITE : (black[i] ? UI_DIM : UI_PANEL2));
        }
        value_u(a, "SPAN", e->project.pitch_span_oct);
        o->text(o->ctx, 8, 132, 58, 0, a, UI_BLUE, 0);
        break;
    }
    case MELLO_UI_SLOTS: {
        /* This bank's sixteen slots, two rows: saved ones filled, the playing one green, the queued one blue; the
         * selected slot's name as the title (where RENAME's cursor runs). */
        MelloPatternSlot *s = &e->slots[ui->selected_slot];
        uint8_t i, start = (uint8_t)(ui->slot_bank * 16u);
        ui_section(ui, o, 32, "", UI_ACC);
        value_u(a, "", (uint32_t)ui->selected_slot + 1u);
        o->text(o->ctx, 10, 35, 24, 0, a, UI_DIM, 0);
        o->text(o->ctx, 34, 35, 100, 0, s->valid ? s->name : "EMPTY", s->valid ? UI_ACC : UI_DIM, 0);
        if (ui->slots_subpage && s->valid)
            o->fill(o->ctx, (int16_t)(34 + ui->slot_name_cursor * 8u), 51, 8, 2, UI_GOLD);
        for (i = 0u; i < 16u; ++i) {
            uint8_t idx = (uint8_t)(start + i);
            MelloPatternSlot *ps = &e->slots[idx];
            int16_t x = (int16_t)(10 + (i % 8u) * 28u), y = (int16_t)(54 + (i / 8u) * 26u);
            bool selected = idx == ui->selected_slot, active = e->active_slot == (int8_t)idx,
                 queued = e->queued_slot == (int8_t)idx, visible = idx < e->visible_slot_count;
            uint16_t c = active ? UI_ACC : (queued ? UI_BLUE : UI_MID);
            ui_round_fill(o, x, y, 24, 23,
                          ps->valid ? ui_mix(c, UI_PANEL, active || queued ? 110 : 170)
                                    : (visible ? UI_PANEL2 : UI_PANEL));
            ui_round_frame(o, x, y, 24, 23,
                           selected ? UI_BRIGHT : (ps->valid ? c : (visible ? UI_LINE : UI_TRACK)));
            value_u(a, "", idx + 1u);
            o->text(o->ctx, x, (int16_t)(y + 3), 24, 0, a,
                    ps->valid ? UI_BRIGHT : (visible ? UI_DIM : UI_TRACK), 1);
        }
        draw_knob_grid(ui, o, 118);
        break;
    }
    case MELLO_UI_DOT: {
        /* The HTML's dot matrix, and a step past it: the four matrices as pills; a 4 x 4 of rounded cells with a
         * dot in each, the cells that are on glowing in the matrix's colour, the cells past its length dark; the
         * read position framed bright with a fading trail of where it was, so its direction shows; the run mode
         * as pills under it. */
        uint8_t sel_id = ui->selected_dot_target % MELLO_DOT_MATRIX_COUNT;
        MelloDotMatrixConfig *c = &e->dot[sel_id];
        static const uint8_t tid[MELLO_DOT_MATRIX_COUNT] = {MELLO_DOT_ACCENT, MELLO_DOT_MOD1, MELLO_DOT_MOD2,
                                                            MELLO_DOT_RATCHET, MELLO_DOT_SUB};
        static const char *tn[MELLO_DOT_MATRIX_COUNT] = {"ACC", "MOD 1", "MOD 2", "RATCH", "SUB"};
        int8_t lane = dot_lane(sel_id);
        uint8_t i, r, col, len = c->length < 1u ? 1u : c->length, route[MELLO_DOT_ROUTE_MAX], route_pos[16],
                           route_n;
        uint16_t colour = ui_dot_colour(sel_id), route_cells = 0u;
        /* The route the matrix reads, numbered in its cells (the first time it comes to each); RANDOM and WALK have
         * none fixed, so their cells only show they are in play. */
        memset(route_pos, 0, sizeof route_pos);
        route_n = mello_dot_route(c->mode, len, route);
        for (i = 0u; i < route_n; ++i) {
            /* forwards the first time the route reaches the cell; with REV, backwards: its last time, from the end */
            uint8_t k = c->reverse ? (uint8_t)(route_n - 1u - i) : i;
            route_cells |= (uint16_t)(1u << route[k]);
            if (!route_pos[route[k]] && c->mode != MELLO_DOT_RANDOM && c->mode != MELLO_DOT_WALK)
                route_pos[route[k]] = (uint8_t)(i + 1u);
        }
        int8_t at = e->dot_active_cell[sel_id];
        for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) {
            int16_t tx = (int16_t)(4 + i * 47u);
            bool sel = tid[i] == sel_id;
            uint16_t tc = ui_dot_colour(tid[i]);
            if (sel) ui_round_fill(o, tx, 32, 45, 17, ui_mix(tc, UI_BLACK, 170));
            ui_round_frame(o, tx, 32, 45, 17, sel ? tc : UI_LINE);
            o->text(o->ctx, (int16_t)(tx + 2), 33, 41, 0, tn[i], sel ? UI_BRIGHT : ui_mix(tc, UI_BLACK, 110),
                    1);
        }
        for (r = 0u; r < 4u; ++r)
            for (col = 0u; col < 4u; ++col) {
                uint8_t idx = (uint8_t)(r * 4u + col);
                int16_t x = (int16_t)(8 + col * 57u), y = (int16_t)(53 + r * 23u);
                bool on = (c->cells & (uint16_t)(1u << idx)) != 0u,
                     inside = (route_cells & (1u << idx)) != 0u;
                bool here = at == (int8_t)idx, trail1 = ui->dot_trail[0] == (int8_t)idx,
                     trail2 = ui->dot_trail[1] == (int8_t)idx;
                uint16_t fillc = !inside ? UI_BLACK
                                 : on    ? ui_mix(colour, UI_PANEL, here ? 40 : 150)
                                         : UI_PANEL;
                uint16_t edge = here     ? colour
                                : trail1 ? ui_mix(colour, UI_PANEL, 110)
                                : trail2 ? ui_mix(colour, UI_PANEL, 170)
                                : on     ? ui_mix(colour, UI_PANEL, 90)
                                         : (inside ? UI_LINE : UI_TRACK);
                ui_round_fill(o, x, y, 52, 20, fillc);
                ui_round_frame(o, x, y, 52, 20, edge);
                if (here)
                    ui_round_frame(o, (int16_t)(x + 1), (int16_t)(y + 1), 50, 18,
                                   colour); /* a thicker frame */
                if (inside)                 /* the HTML's dot in each cell: bright where on or read */
                    o->fill(o->ctx, (int16_t)(x + 34), (int16_t)(y + 8), here ? 4 : 2, here ? 4 : 2,
                            here ? UI_BRIGHT : (on ? UI_BRIGHT : UI_LINE));
                if (route_pos[idx]) { /* the route: where this cell comes in it (REV counts from the end) */
                    value_u(a, "", route_pos[idx]);
                    o->text(o->ctx, (int16_t)(x + 4), (int16_t)(y + 2), 20, 0, a,
                            here ? UI_BRIGHT : (on ? ui_mix(UI_BRIGHT, colour, 100) : UI_DIM), 0);
                }
            }
        if (ui->dot_shape && lane >= 0) {
            /* SHAPE: what a lit cell sends, drawn: up in ATK, down in REL; GATE holds a flat top between them
             * (lit cells in a row), HIT goes straight down. Times share the width by their length. */
            const MelloModRouting *m = &e->modr;
            uint32_t at_ms = m->dot_attack_ms[lane], rl = m->dot_release_ms[lane],
                     hold = m->dot_gate[lane] ? (at_ms + rl) / 2u + 20u : 0u, tot = at_ms + hold + rl;
            int16_t x0 = 6, w = 120, wa = (int16_t)(at_ms * (uint32_t)w / tot),
                    wh = (int16_t)(hold * (uint32_t)w / tot), k;
            ui_round_fill(o, 4, 146, 124, 19, UI_PANEL);
            for (k = 0; k < w; ++k) {
                int16_t hgt = k < wa ? (int16_t)(1 + k * 14 / (wa ? wa : 1))
                              : k < wa + wh
                                  ? 15
                                  : (int16_t)(15 - (k - wa - wh) * 14 / (w - wa - wh > 0 ? w - wa - wh : 1));
                o->fill(o->ctx, (int16_t)(x0 + k), (int16_t)(163 - hgt), 1, 2, colour);
            }
            o->text(o->ctx, 132, 148, 104, 0, mod_dest_name(m->dot_target[lane]), colour, 2);
            break;
        }
        /* the route by name, as a pill (and REV beside it when on) */
        ui_round_fill(o, 4, 147, 72, 17, ui_mix(colour, UI_BLACK, 170));
        ui_round_frame(o, 4, 147, 72, 17, colour);
        o->text(o->ctx, 8, 148, 64, 0, dot_mode_name(c->mode), UI_BRIGHT, 1);
        if (c->reverse) {
            ui_round_frame(o, 80, 147, 34, 17, colour);
            o->text(o->ctx, 84, 148, 26, 0, "REV", colour, 1);
        }
        if (lane >= 0) /* where the lane goes, so the matrix says what it does */
            o->text(o->ctx, 140, 148, 96, 0, mod_dest_name(e->modr.dot_target[lane]), colour, 2);
        else {
            (void)copy_s(put_u32(a, c->amount / 10u), "%");
            o->text(o->ctx, 144, 148, 40, 0, a, colour, 2);
            o->text(o->ctx, 188, 148, 46, 0, c->follow ? "FOLLOW" : (c->reset ? "RESET" : "FREE"),
                    c->follow ? UI_BLUE : UI_DIM, 2);
        }
        break;
    }
    case MELLO_UI_TURING: {
        uint8_t i;
        uint8_t len = e->tm.length < 3u ? 3u : e->tm.length;
        int16_t lw = (int16_t)(224 / len); /* every lane in view, as wide as the length lets them */
        o->text(o->ctx, 8, 33, 92, 0, e->tm.on ? (e->tm.locked ? "EVOLVE LOCK" : "EVOLVE ON") : "EVOLVE OFF",
                e->tm.on ? UI_BLUE : UI_DIM, 0);
        value_u(a, "LEN", len);
        o->text(o->ctx, 100, 33, 48, 0, a, UI_MID, 1); /* "LEN 32" fills it */
        o->text(o->ctx, 148, 33, 84, 0,
                e->tm.rate == MELLO_TM_RATE_HALF     ? "RATE 1/2X"
                : e->tm.rate == MELLO_TM_RATE_DOUBLE ? "RATE 2X"
                                                     : "RATE 1X",
                UI_MID, 2);
        /* The whole register (feedback 2026-10-30: past 8 steps the playing one went out of sight): a lane a step,
         * its value as a bar and its gate as a pad under it; the playing lane framed white, the selected one soft;
         * step numbers where the lanes are wide enough, else every fourth. */
        for (i = 0u; i < len; ++i) {
            int16_t x = (int16_t)(8 + i * lw), cw = (int16_t)(lw - (lw > 6 ? 2 : 1));
            int16_t h = (int16_t)(((uint32_t)e->tm_float_reg[i] * 69u) / 65535u);
            uint8_t gate = e->tm_bool_reg[i];
            uint8_t active = e->tm_active_index == (int8_t)i;
            o->fill(o->ctx, x, 52, cw, 84, UI_PANEL);
            if (h > 0) o->fill(o->ctx, (int16_t)(x + (cw > 8 ? 2 : 0)), (int16_t)(128 - h), (int16_t)(cw > 8 ? cw - 4 : cw), h, UI_BLUE);
            o->fill(o->ctx, (int16_t)(x + (cw > 8 ? 2 : 0)), 130, (int16_t)(cw > 8 ? cw - 4 : cw), 4, gate ? UI_ACC : UI_PANEL2);
            if (active || i == ui->selected_tm_bit) ui_frame(o, x, 52, cw, 84, active ? UI_WHITE : UI_SOFT);
            if (lw >= 20 || (i % 4u) == 0u) {
                a[0] = (char)('0' + ((i + 1u) / 10u)), a[1] = (char)('0' + ((i + 1u) % 10u)), a[2] = 0;
                o->text(o->ctx, x, 137, (int16_t)(lw >= 20 ? cw : 22), 0, i < 9u ? a + 1 : a, active ? UI_WHITE : UI_DIM, 0);
            }
        }
        o->text(o->ctx, 8, 154, 36, 0, "MUT", UI_DIM, 0);
        ui_hbar(o, 45, 157, 92, e->tm.mutation_prob, 500u, UI_PURPLE);
        value_u(a, "", e->tm.mutation_prob / 10u);
        o->text(o->ctx, 141, 154, 28, 0, a, UI_PURPLE, 0);
        value_i(a, "OFF", e->tm.offset);
        o->text(o->ctx, 173, 154, 59, 0, a, UI_MID, 2);
        break;
    }
    case MELLO_UI_SUB: {
        /* The dedicated sub's 5-SEQ as yellow cards: each step's tone as a bar (off = a line), its octave mark; the
         * playing one gold, the selected one framed; then K1-K4. */
        uint8_t i, xlen = e->sub.x_length;
        if (xlen != 2u && xlen != 3u && xlen != 5u) xlen = 5u;
        (void)copy_s(copy_s(a, e->sub.on ? "SUB  " : "SUB OFF  "), sub_mode_name(e->sub.mode));
        ui_section(ui, o, 32, a, e->sub.on ? UI_GOLD : UI_DIM);
        ui_spaced(o, 10, 52, 100, "SUB 5-SEQ", e->sub.x_on ? UI_GOLD : UI_DIM, 0);
        o->text(o->ctx, 110, 52, 120, 0, !e->sub.x_on ? "OFF" : (e->sub.x_reset ? "RESET LOOP" : "FREE RUN"),
                e->sub.x_on ? UI_GOLD : UI_DIM, 2);
        for (i = 0u; i < MELLO_SUB_X_MAX_STEPS; ++i) {
            MelloSubXStep *xs = &e->sub.x_steps[i];
            int16_t x = (int16_t)(10 + i * 45u), tone_h = (int16_t)(4 + xs->tone * 3u);
            bool in_len = i < xlen, active = e->sub.x_on && e->sub_x_active_index == (int8_t)i,
                 selected = i == ui->selected_sub_x_step;
            uint16_t bar = e->sub.x_on ? UI_GOLD : UI_MID;
            ui_round_fill(o, x, 70, 40, 28, in_len ? UI_PANEL2 : UI_PANEL);
            ui_round_frame(o, x, 70, 40, 28,
                           active ? UI_GOLD : (selected ? UI_BRIGHT : (in_len ? UI_LINE : UI_TRACK)));
            if (in_len && xs->on) {
                if (tone_h > 20) tone_h = 20;
                ui_vgrad(o, (int16_t)(x + 4), (int16_t)(94 - tone_h), 32, tone_h, bar,
                         ui_mix(bar, UI_PANEL2, 150));
            } else if (in_len)
                o->fill(o->ctx, (int16_t)(x + 4), 92, 32, 2, UI_LINE);
            if (in_len && xs->octave)
                o->fill(o->ctx, (int16_t)(x + 4), xs->octave > 0 ? 73 : 95, 32, 2, UI_BLUE);
            a[0] = (char)('1' + i), a[1] = 0;
            o->text(o->ctx, x, 99, 40, 0, a, active ? UI_GOLD : (selected ? UI_BRIGHT : UI_DIM), 1);
        }
        draw_knob_grid(ui, o, 118);
        break;
    }
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: {
        /* The chord layer in its violet: the chain of chords (the selected one's notes as pills), or its trigger grid,
         * or its pattern's voice masks; then K1-K4. */
        MelloChordConfig *c = &e->chord;
        uint8_t i;
        uint16_t col = UI_CHORD;
        (void)copy_s(copy_s(a, c->on ? "CHORD  " : "CHORD OFF  "), chord_mode_name(c->mode));
        ui_section(ui, o, 32, a, c->on ? col : UI_DIM);
        if (ui->chord_subpage == 3u) {
            for (i = 0u; i < 16u; ++i) {
                int16_t x = (int16_t)(10 + (i % 4u) * 56u), y = (int16_t)(52 + (i / 4u) * 13u);
                bool on = (c->grid_cells & (uint16_t)(1u << i)) != 0u,
                     here = e->chord_grid_active_cell == (int8_t)i;
                ui_round_fill(o, x, y, 52, 11, on ? ui_mix(col, UI_PANEL, here ? 40 : 130) : UI_PANEL2);
                ui_round_frame(o, x, y, 52, 11, here ? UI_BRIGHT : (on ? col : UI_LINE));
            }
        } else if (ui->chord_subpage == 4u) {
            /* the voice masks, the one playing framed and its voices lit as they sound */
            uint8_t pl = c->pattern_length ? c->pattern_length : 1u,
                    at = ui->transport->running ? (uint8_t)((e->chord_pattern_pos + pl - 1u) % pl) : 0xFFu,
                    lit = ui->transport->display_chord_lit;
            for (i = 0u; i < pl; ++i) {
                int16_t x = (int16_t)(10 + i * 28u);
                uint8_t j, m = c->pattern_masks[i];
                ui_round_frame(o, x, 52, 24, 52,
                               i == at ? UI_BRIGHT : i == ui->selected_chord_pattern ? col : UI_LINE);
                for (j = 0u; j < 4u; ++j) {
                    bool on = (m & (1u << j)) != 0u;
                    ui_round_fill(o, (int16_t)(x + 4), (int16_t)(91 - j * 12u), 16, 9,
                                  !on                         ? UI_TRACK
                                  : i == at && (lit >> j) & 1u ? UI_BRIGHT
                                                              : col);
                }
            }
        } else {
            /* The chain as chord names, the one sounding underlined; under it the chord's voices, lowest left,
             * each lit while it sounds (or, on AMP and MOTION, the envelope). */
            uint8_t notes[4], sel = ui->selected_chord_step < c->length ? ui->selected_chord_step : 0u, n,
                              sounding = ui->transport->running ? e->chord_voiced_slot : 0xFFu,
                              lit = ui->transport->display_chord_lit;
            int16_t w = (int16_t)(220 / c->length);
            for (i = 0u; i < c->length; ++i) {
                mello_engine_chord_name(e, i, b);
                ui_pill(o, (int16_t)(10 + i * w), 53, (int16_t)(w - 3), b, i == sel, col);
                if (i == sounding) o->fill(o->ctx, (int16_t)(14 + i * w), 71, (int16_t)(w - 11), 2, UI_BRIGHT);
            }
            if (ui->chord_subpage == 8u)
                ui_env_graph(o, 10, 78, 220, 32, c->attack_ms, c->decay_ms, c->sustain, c->release_ms, col);
            else if (ui->chord_subpage == 7u)
                ui_env_graph(o, 10, 78, 220, 32, c->fenv_attack_ms, c->fenv_decay_ms, 0u, 0u, col);
            else {
                if (sounding != 0xFFu && e->chord_voiced_n) {
                    n = e->chord_voiced_n;
                    for (i = 0u; i < n; ++i) notes[i] = e->chord_voiced[i];
                } else
                    n = mello_engine_resolve_chord_notes(e, sel, notes);
                for (i = 0u; i < n && i < 4u; ++i) {
                    int16_t x = (int16_t)(10 + i * 56u);
                    bool on = (lit >> i) & 1u;
                    midi_note_name(b, notes[i]);
                    ui_round_fill(o, x, 77, 52, 34, on ? ui_mix(col, UI_WHITE, 200) : ui_mix(col, UI_PANEL, 170));
                    ui_round_frame(o, x, 77, 52, 34, on ? UI_WHITE : col);
                    o->text(o->ctx, (int16_t)(x + 1), 78, 50, 1, b, on ? UI_BLACK : UI_BRIGHT, 1);
                }
            }
        }
        draw_knob_grid(ui, o, 118);
        break;
    }
#endif
    case MELLO_UI_GLOBAL: draw_param_panel(ui, o); break;
    case MELLO_UI_SOUND_RESERVED: draw_param_panel(ui, o); break;
    case MELLO_UI_AUDIO_FX: draw_param_panel(ui, o); break;
    case MELLO_UI_DRUMS: {
        /* The four voices' rows over the 16 steps, each in its colour: a hit filled (an accent brighter, its rolls as
         * dots, a step with a lock marked under it), the picked voice's row lit and its picked step framed, the playing
         * step's column framed, steps past LENGTH dim; then K1-K4. */
        static const char *names[MELLO_DRUM_VOICES] = {"KICK", "SNARE", "HAT", "GLITCH"};
        static const uint16_t col[MELLO_DRUM_VOICES] = {UI_CORAL, UI_GOLD, UI_CYAN, UI_PURPLE};
        const MelloDrumConfig *d = &e->drums;
        uint8_t vsel = ui->selected_drum_voice % MELLO_DRUM_VOICES, i, v;
        int8_t at = ui->transport->running && d->on ? e->drum_step : -1;
        (void)copy_s(copy_s(a, d->on ? "DRUMS  " : "DRUMS OFF  "), names[vsel]);
        ui_section(ui, o, 32, a, d->on ? col[vsel] : UI_DIM);
        for (v = 0u; v < MELLO_DRUM_VOICES; ++v)
            for (i = 0u; i < MELLO_DRUM_STEPS; ++i) {
                int16_t x = (int16_t)(8 + i * 14u), y = (int16_t)(52 + v * 14u);
                const MelloDrumLock *lk = &d->lock[v][i];
                bool hit = (d->hits[v] >> i) & 1u, acc = (d->accents[v] >> i) & 1u, in = i < d->length;
                uint16_t c = !in       ? UI_PANEL
                             : hit     ? (acc ? ui_mix(col[v], UI_WHITE, 150) : (v == vsel ? col[v] : ui_mix(col[v], UI_PANEL, 110)))
                                       : (v == vsel ? UI_TRACK : ui_mix(col[v], UI_PANEL, 225));
                o->fill(o->ctx, x, y, 12, 12, c);
                if (hit && lk->ratchet > 1u) {
                    uint8_t r;
                    for (r = 0u; r < lk->ratchet; ++r) o->fill(o->ctx, (int16_t)(x + 1 + r * 3), (int16_t)(y + 1), 2, 2, UI_BLACK);
                }
                if (hit && (lk->pitch || lk->decay || lk->crush)) o->fill(o->ctx, (int16_t)(x + 3), (int16_t)(y + 9), 6, 2, UI_BLACK);
                if (v == vsel && i == ui->selected_drum_step) ui_frame(o, x, y, 12, 12, UI_BRIGHT);
            }
        if (at >= 0) ui_frame(o, (int16_t)(7 + at * 14), 51, 14, 58, UI_WHITE);
        draw_knob_grid(ui, o, 118);
        break;
    }
    case MELLO_UI_SAMPLE_RESERVED: {
        /* The HTML's SAMPLE LAYER: slots A-D as pills (filled = loaded), the layer's state; the trim as a bar with
         * gold edges (PITCH adds the transpose mark), or the envelope's curve; then K1-K4. */
        uint8_t si = e->sample.edit_slot < MELLO_SAMPLE_SLOT_COUNT ? e->sample.edit_slot : 0u, i;
        MelloSampleSlotConfig *sl = &e->sample.slots[si];
        const MelloSampleSlotMeta *sm = ui->sample_bank ? &ui->sample_bank->meta[si] : 0;
        bool loaded = sm && sm->loaded;
        ui_section(ui, o, 32, e->sample.on ? (e->sample.per_step ? "PER STEP" : "FIXED SLOT") : "LAYER OFF",
                   e->sample.on ? UI_BLUE : UI_DIM);
        for (i = 0u; i < MELLO_SAMPLE_SLOT_COUNT; ++i) {
            const MelloSampleSlotMeta *mi = ui->sample_bank ? &ui->sample_bank->meta[i] : 0;
            a[0] = (char)('A' + i), a[1] = 0;
            ui_pill(o, (int16_t)(10 + i * 34u), 53, 30, a, i == si, mi && mi->loaded ? UI_ACC : UI_BLUE);
        }
        o->text(o->ctx, 146, 53, 84, 0, loaded ? sm->name : "EMPTY", loaded ? UI_ACC : UI_DIM, 2);
        if (ui->sample_subpage == 3u)
            ui_env_graph(o, 10, 76, 220, 32, sl->attack_ms, sl->decay_ms, sl->sustain_permille,
                         sl->release_ms, UI_BLUE);
        else {
            int16_t tx0 = (int16_t)(10 + ((uint32_t)220u * sl->trim_start) / 1000u),
                    tx1 = (int16_t)(10 + ((uint32_t)220u * sl->trim_end) / 1000u);
            ui_round_fill(o, 10, 76, 220, 30, UI_PANEL2);
            if (tx1 < tx0 + 2) tx1 = (int16_t)(tx0 + 2);
            if (tx1 > 228) tx1 = 228;
            ui_vgrad(o, tx0, 81, (int16_t)(tx1 - tx0), 20, loaded ? UI_ACC : UI_MID,
                     ui_mix(loaded ? UI_ACC : UI_MID, UI_PANEL2, 160));
            o->fill(o->ctx, tx0, 76, 2, 30, UI_GOLD);
            o->fill(o->ctx, tx1, 76, 2, 30, UI_GOLD);
            if (ui->sample_subpage == 1u) {
                int16_t center = (int16_t)(120 + sl->coarse_semitones * 4);
                if (center < 12) center = 12;
                if (center > 226) center = 226;
                o->fill(o->ctx, center, 74, 2, 34, UI_BLUE);
            }
        }
        draw_knob_grid(ui, o, 118);
        break;
    }
    }
}

static uint16_t ui_text_len(const char *s)
{
    uint16_t n = 0u;
    if (!s) return 0u;
    while (s[n]) ++n;
    return n;
}

static void footer_pair(const MelloUiDrawOps *o, uint8_t k, const char *label, const char *value,
                        uint16_t colour)
{
    int16_t x = (int16_t)(k * 60u);
    char compact[8];
    const char *shown = value ? value : "";
    uint16_t n = ui_text_len(shown), i;
    if (n > 7u) {
        for (i = 0u; i < 6u; ++i) compact[i] = shown[i];
        compact[6] = '~';
        compact[7] = 0;
        shown = compact;
    }
    o->fill(o->ctx, x, 188, 60, 32, UI_PANEL);
    if (k) o->fill(o->ctx, x, 192, 1, 24, UI_TRACK); /* a quiet divider, as the HTML's columns */
    ui_spaced(o, (int16_t)(x + 2), 188, 56, label, UI_DIM, 1);
    o->text(o->ctx, x + 2, 204, 56, 0, shown, colour, 1);
}

/* A black key that switches something: 1 on, 0 off; -1 for a key that acts (no state to show). */
static int8_t ctx_lit(MelloUi *ui, MelloContextVerb v)
{
    MelloEngine *e = eng(ui);
    if (!e) return -1;
    switch (v) {
    case MELLO_CTX_DOT_FOLLOW: return e->dot[ui->selected_dot_target % MELLO_DOT_MATRIX_COUNT].follow ? 1 : 0;
    case MELLO_CTX_DOT_RESET: return e->dot[ui->selected_dot_target % MELLO_DOT_MATRIX_COUNT].reset ? 1 : 0;
    case MELLO_CTX_DOT_REVERSE:
        return e->dot[ui->selected_dot_target % MELLO_DOT_MATRIX_COUNT].reverse ? 1 : 0;
    case MELLO_CTX_TURING_TOGGLE: return e->tm.on ? 1 : 0;
    case MELLO_CTX_TURING_LOOP: return e->modr.tm_on_loop ? 1 : 0;
    case MELLO_CTX_SUB_TOGGLE: return e->sub.on ? 1 : 0;
    case MELLO_CTX_PERF_FILL: return e->perf.fill ? 1 : 0;
    case MELLO_CTX_PERF_SUB_MUTE: return e->perf.sub_mute ? 1 : 0;
    case MELLO_CTX_PERF_REVERSE: return e->perf.reverse ? 1 : 0;
    case MELLO_CTX_PERF_REPEAT: return e->perf.repeat ? 1 : 0;
    case MELLO_CTX_UNDO: return mello_engine_undo_state(e) == MELLO_UNDO_NONE ? 0 : -1; /* dim: nothing to undo */
    case MELLO_CTX_DRUM_KICK:
    case MELLO_CTX_DRUM_SNARE:
    case MELLO_CTX_DRUM_HAT:
    case MELLO_CTX_DRUM_GLITCH: return (uint8_t)(v - MELLO_CTX_DRUM_KICK) == ui->selected_drum_voice ? 1 : -1;
    case MELLO_CTX_DRUM_ON: return e->drums.on ? 1 : 0;
    case MELLO_CTX_DRUM_FEED: return e->drums.feed ? 1 : 0;
    case MELLO_CTX_DRUM_ACCENT:
        return (e->drums.accents[ui->selected_drum_voice % MELLO_DRUM_VOICES] >>
                (ui->selected_drum_step % MELLO_DRUM_STEPS)) & 1u
                   ? 1
                   : 0;
    case MELLO_CTX_SEG_A:
    case MELLO_CTX_SEG_B:
    case MELLO_CTX_SEG_C:
    case MELLO_CTX_SEG_D:
    case MELLO_CTX_SEG_E:
    case MELLO_CTX_SEG_F: { /* the one picked lit, the others there plain, the ones not there dim */
        uint8_t b[MELLO_MAX_SEGMENTS + 1u], n = (uint8_t)(v - MELLO_CTX_SEG_A);
        if (n >= mello_segment_bounds(&e->project, b)) return 0;
        return n == ui->selected_segment ? 1 : -1;
    }
    default: return -1;
    }
}

/* A label's colour when the strip has no room for pills (HOME): by what the key does. */
static uint16_t strip_colour(const MelloUi *ui, MelloContextVerb v)
{
    switch (v) {
    case MELLO_CTX_PERF_FILL: return UI_CORAL;
    case MELLO_CTX_PERF_SUB_MUTE: return UI_GOLD;
    case MELLO_CTX_PERF_REVERSE: return UI_BLUE;
    case MELLO_CTX_PERF_REPEAT: return UI_PURPLE;
    case MELLO_CTX_PERF_RESTART: return UI_CYAN;
    case MELLO_CTX_RANDOM_CLEAR: return ui->reset_armed ? UI_RED : ui_mix(UI_RED, UI_PANEL2, 110);
    case MELLO_CTX_MUTATE:
    case MELLO_CTX_UNDO: return UI_ACC;
    default: return UI_MID;
    }
}

/* The black-key strip: one cell per black key, in the keyboard's own groups (3 2 3 2 1 from F#3), so a key is found
 * by its group and place rather than by a number. A labelled key is a framed cell; a free one a dim dot. */
static void draw_control_hint(MelloUi *ui, const MelloUiDrawOps *o)
{
    static const uint8_t group_end[5] = {3u, 5u, 8u, 10u, 11u};
    uint16_t accent = ui_page_accent(ui);
    int16_t x = 4;
    uint8_t i, g = 0u;
    o->fill(o->ctx, 0, 172, 240, 16, UI_PANEL2);
    if (ui->page == MELLO_UI_HOME && ui->held_step != MELLO_UI_NO_STEP && eng(ui)) {
        /* a held step: the first group's keys and the next are its flags, lit when on (the HTML's per-step pills) */
        static const char *fl[4] = {"SKIP", "ACC", "SLD", "SUB"};
        static const uint16_t fc[4] = {UI_RED, UI_CORAL, UI_BLUE, UI_GOLD};
        const MelloStep *st = &eng(ui)->project.steps[ui->held_step];
        bool on[4];
        uint8_t f;
        int16_t px = 2;
        on[0] = st->skip, on[1] = st->accent, on[2] = st->slide, on[3] = st->sub;
        for (f = 0u; f < 4u; ++f) {
            int16_t w = (int16_t)(ui_text_len(fl[f]) * 8u + 6u);
            if (f == 3u) px = (int16_t)(px + 6); /* SUB is the next group's first key */
            if (on[f]) ui_round_fill(o, px, 172, w, 16, ui_mix(fc[f], UI_BLACK, 150));
            ui_round_frame(o, px, 172, w, 16, on[f] ? fc[f] : UI_LINE);
            o->text(o->ctx, (int16_t)(px + 3), 172, (int16_t)(w - 6), 0, fl[f], on[f] ? UI_BRIGHT : UI_DIM,
                    1);
            px = (int16_t)(px + w + 2);
        }
        ui_spaced(o, (int16_t)(px + 6), 172, (int16_t)(236 - px - 6), "STEP", UI_DIM, 2);
        return;
    }
    if (ui->page == MELLO_UI_HOME && ui->held_black) { /* HOME's keys held: their full names, lit */
        static const char *full[11] = {"TRANSPOSE -1", "TRANSPOSE +1", "MUTATE", "FILL",
                                       "SUB MUTE",     "REVERSE",      "REPEAT", "RESTART",
                                       "CLEAR",        "UNDO",         "RNDMZR"};
        char b[48], *q = b;
        uint8_t k;
        *q = 0;
        for (k = 0u; k < 11u; ++k)
            if (ui->held_black & (1u << k)) {
                if (q != b) q = copy_s(q, " + ");
                q = copy_s(q, full[k]);
                if (k == 6u) { /* REPEAT: its length, and how to set it */
                    q = copy_s(q, " ");
                    q = put_u32(q, ui->perf_repeat_len ? ui->perf_repeat_len : 4u);
                    q = copy_s(q, "  WHITE KEY = LEN");
                }
            }
        ui_round_fill(o, 2, 172, 236, 16, ui_mix(accent, UI_BLACK, 170));
        o->text(o->ctx, 4, 172, 232, 0, b, UI_BRIGHT, 1);
        return;
    }
    if (ui->page == MELLO_UI_HOME && ui->keys_mode) { /* every key is a note now: say what a key does */
        char b[40], *q;
        q = copy_s(b, ui->keys_rec ? "REC: STEP " : ui->transport->running ? "KEYS: ROOT " : "KEYS: PLAY ");
        if (ui->keys_rec) q = put_u32(q, (uint32_t)ui->keys_rec_pos + 1u);
        q = copy_s(q, "  OCT ");
        (void)put_i32(q, ui->keys_octave);
        o->text(o->ctx, 4, 172, 232, 0, b, ui->keys_rec ? UI_RED : UI_GOLD, 1);
        return;
    }
    {
        /* Eleven cells across the whole width, one a key in the keys' order, the keyboard's groups apart: a labelled
         * key as wide as its label, a free one a short dim cell, and what room is left shared out, so a label sits
         * where its key is. Where the labels need all the room (HOME, DRUMS) the cells close up. */
        int16_t w[11], need = 0, gap_in, gap_group, extra, rem;
        const char *lab[11];
        uint8_t labelled = 0u;
        bool compact = false;
        for (i = 0u; i < 11u; ++i) {
            const MelloContextAction *a = context_action(ui->page, i);
            lab[i] = a && a->verb != MELLO_CTX_NONE ? a->label : 0;
            if (lab[i] && a->verb == MELLO_CTX_UNDO && eng(ui) &&
                mello_engine_undo_state(eng(ui)) == MELLO_UNDO_REDO)
                lab[i] = "REDO";
            w[i] = lab[i] ? (int16_t)(ui_text_len(lab[i]) * 8u + 4u) : 6;
            need = (int16_t)(need + w[i]);
            if (lab[i]) ++labelled;
        }
        gap_in = 2, gap_group = 6;
        if (need + 6 * gap_in + 4 * gap_group > 236) {
            /* No room for framed cells: plain ones, each just holding its label, a pixel apart */
            compact = true;
            gap_in = 0, gap_group = 2;
            need = 0;
            for (i = 0u; i < 11u; ++i) {
                w[i] = lab[i] ? (int16_t)(ui_text_len(lab[i]) * 8u + 1u) : 2;
                need = (int16_t)(need + w[i]);
            }
        }
        extra = (int16_t)(236 - need - 6 * gap_in - 4 * gap_group);
        if (extra < 0) extra = 0;
        rem = labelled ? (int16_t)(extra % labelled) : 0;
        x = 2;
        for (i = 0u; i < 11u; ++i) {
            const MelloContextAction *a = context_action(ui->page, i);
            int16_t cw = w[i];
            if (i && i == group_end[g]) {
                x = (int16_t)(x + gap_group); /* between the keyboard's groups */
                ++g;
            } else if (i)
                x = (int16_t)(x + gap_in);
            if (lab[i] && labelled) cw = (int16_t)(cw + extra / labelled + (rem-- > 0 ? 1 : 0));
            if (!lab[i]) {
                ui_round_fill(o, x, 178, cw, 4, UI_LINE); /* a key with nothing on it */
            } else if (compact) { /* RNDMZR filled purple, as on every page */
                int8_t lit = a->verb == MELLO_CTX_RNDMZR ? -1 : ctx_lit(ui, a->verb);
                uint16_t c = a->verb == MELLO_CTX_RNDMZR   ? UI_WHITE
                             : ui->page == MELLO_UI_HOME ? strip_colour(ui, a->verb)
                                                         : (lit == 0 ? UI_DIM : accent);
                o->fill(o->ctx, x, 172, (int16_t)(cw - 1), 16,
                        a->verb == MELLO_CTX_RNDMZR ? ui_mix(UI_PURPLE, UI_BLACK, 110)
                        : lit > 0                   ? ui_mix(c, UI_BLACK, 130)
                                                    : ui_mix(c, UI_PANEL2, 205));
                o->text(o->ctx, x, 172, (int16_t)(cw - 1), 0, lab[i], lit > 0 ? UI_BRIGHT : c, 1);
            } else if (a->verb == MELLO_CTX_RNDMZR) { /* the same key on every page, so it looks the same: filled */
                ui_round_fill(o, x, 172, cw, 16, ui_mix(UI_PURPLE, UI_BLACK, 110));
                o->text(o->ctx, (int16_t)(x + 1), 172, (int16_t)(cw - 2), 0, lab[i], UI_WHITE, 1);
            } else {
                int8_t lit = ctx_lit(ui, a->verb);
                uint16_t c = ui->page == MELLO_UI_HOME ? strip_colour(ui, a->verb)
                             : a->verb == MELLO_CTX_RANDOM_CLEAR || a->verb == MELLO_CTX_GLOBAL_RESET ||
                                     a->verb == MELLO_CTX_SOUND_INIT
                                 ? (ui->reset_armed ? UI_RED : UI_DIM)
                                 : (lit == 0 ? UI_DIM : accent);
                /* a cell a key: tinted with its colour, filled when on */
                ui_round_fill(o, x, 172, cw, 16, lit > 0 ? ui_mix(c, UI_BLACK, 130) : ui_mix(c, UI_PANEL2, 215));
                ui_round_frame(o, x, 172, cw, 16, lit > 0 ? c : ui_mix(c, UI_PANEL2, 150));
                o->text(o->ctx, (int16_t)(x + 1), 172, (int16_t)(cw - 2), 0, lab[i], lit > 0 ? UI_BRIGHT : c, 1);
            }
            x = (int16_t)(x + cw);
        }
    }
}

static const uint16_t CUTOFF_HZ[101] = {
    /* 80 * 200^(i / 100): logFilterHz, the HTML's cutoff in Hz */
    80,   84,   89,    94,    99,    104,   110,   116,   122,   129,   136,   143,  151,  159,  168,
    177,  187,  197,   208,   219,   231,   243,   257,   271,   285,   301,   317,  334,  353,  372,
    392,  413,  436,   460,   485,   511,   539,   568,   599,   632,   666,   702,  740,  781,  823,
    868,  915,  965,   1018,  1073,  1131,  1193,  1258,  1326,  1398,  1475,  1555, 1639, 1729, 1823,
    1922, 2026, 2137,  2253,  2375,  2505,  2641,  2785,  2936,  3096,  3264,  3442, 3629, 3827, 4035,
    4255, 4486, 4730,  4988,  5259,  5545,  5847,  6165,  6500,  6854,  7227,  7620, 8035, 8472, 8933,
    9419, 9932, 10472, 11042, 11643, 12276, 12944, 13649, 14391, 15174, 16000,
};

/* Values in music units, as the HTML shows them: 62%, 180MS / 1.2S, 3264HZ / 2.00HZ, D#4. */
static void value_pct(char *b, uint32_t permille)
{
    (void)copy_s(put_u32(b, (permille + 5u) / 10u), "%");
}
static void value_ms(char *b, uint32_t ms)
{
    char *p;
    if (ms < 1000u) {
        (void)copy_s(put_u32(b, ms), "MS");
        return;
    }
    p = put_u32(b, ms / 1000u);
    *p++ = '.';
    *p++ = (char)('0' + (ms % 1000u) / 100u);
    (void)copy_s(p, "S");
}
static void value_cutoff(char *b, uint16_t cut)
{
    uint32_t i = cut >= 1000u ? 100u : cut / 10u, f = cut % 10u, hz = CUTOFF_HZ[i];
    if (i < 100u) hz += ((uint32_t)(CUTOFF_HZ[i + 1u] - CUTOFF_HZ[i]) * f) / 10u;
    (void)copy_s(put_u32(b, hz), "HZ");
}
static void value_millihz(char *b, uint32_t mhz)
{
    char *p = put_u32(b, mhz / 1000u);
    *p++ = '.';
    if (mhz < 10000u) {
        *p++ = (char)('0' + (mhz % 1000u) / 100u);
        *p++ = (char)('0' + (mhz % 100u) / 10u);
    } else
        *p++ = (char)('0' + (mhz % 1000u) / 100u);
    (void)copy_s(p, "HZ");
}

/* Where K1-K4's value sits in its range, for the readout's slider: false where the control is a list or switch. */
static bool knob_norm(MelloUi *ui, uint8_t k, uint16_t *val, uint16_t *max)
{
    MelloEngine *e = eng(ui);
    uint32_t v = 0u, m = 1000u;
    if (!e) return false;
    if (ui->page == MELLO_UI_HOME) {
        if (ui->held_step == MELLO_UI_NO_STEP) {
            v = k == 0u   ? e->sound.filter_cut
                : k == 1u ? e->sound.decay_ms
                : k == 2u ? e->fx.reverb_wet
                          : e->fx.dist_amount;
        } else {
            MelloStep *s = &e->project.steps[ui->selected_step % MELLO_MAX_STEPS];
            if (k == 0u) {
                v = (uint32_t)(s->pitch < 0 ? 0 : s->pitch);
                m = (uint32_t)e->project.pitch_span_oct * scale_len(e->project.scale) - 1u;
            } else if (k == 1u || k == 2u) {
                v = (uint32_t)(k == 1u ? s->pulses : s->ratchet) - 1u;
                m = 7u;
            } else
                return false;
        }
    } else if (ui->page == MELLO_UI_SOUND_RESERVED) {
        const MelloSoundConfig *c = &e->sound;
        switch (ui->sound_subpage * 4u + k) {
        case 1: v = c->pwm_permille; break;
        case 2: v = c->filter_cut; break;
        case 3: v = c->filter_res; break;
        case 4: v = c->attack_ms, m = 500u; break;
        case 5: v = c->decay_ms; break;
        case 6: v = c->sustain_permille; break;
        case 7: v = c->release_ms, m = 2000u; break;
        case 9: v = c->lfo_rate_millihz, m = 20000u; break;
        case 10: v = c->lfo_depth; break;
        case 13: v = c->follower_sub_level; break;
        case 16: v = c->slide_ms, m = 300u; break;
        case 17: v = c->master_level; break;
        case 20: v = c->noise_level; break;
        case 21: v = c->noise_fm; break;
        case 22: v = (uint32_t)c->unison - 1u, m = MELLO_UNISON_MAX - 1u; break;
        case 23: v = c->detune; break;
        case 25: v = (uint32_t)(c->osc2_semi + 24), m = 48u; break;
        case 26: v = (uint32_t)(c->osc2_fine + 50), m = 100u; break;
        case 27: v = c->osc2_mix; break;
        case 29: v = c->fm_amount; break;
        case 30: v = c->saturation; break;
        case 32: v = (uint32_t)(c->menv_amount + 1000), m = 2000u; break;
        case 33: v = c->menv_attack_ms, m = 2000u; break;
        case 34: v = c->menv_decay_ms, m = 5000u; break;
        case 36: v = (uint32_t)(c->vel_cutoff + 1000), m = 2000u; break;
        case 37: v = c->vel_level, m = 2000u; break;
        case 38: v = c->key_track; break;
        case 39: v = (uint32_t)(c->vel_menv + 1000), m = 2000u; break;
        default: return false;
        }
    } else if (ui->page == MELLO_UI_AUDIO_FX) {
        const MelloFxConfig *f = &e->fx;
        switch (ui->fx_subpage * 4u + k) {
        case 0: v = f->dist_amount; break;
        case 1: v = f->dist_mix; break;
        case 2: v = f->dist_character; break;
        case 3: v = f->dist_tone; break;
        case 4: v = f->delay_wet; break;
        case 5: v = f->delay_feedback; break;
        case 6: v = f->delay_lowcut; break;
        case 7: v = f->delay_tone; break;
        case 10: v = f->delay_time_ms, m = 2000u; break;
        case 11: v = f->delay_level, m = 1500u; break;
        case 12: v = f->reverb_wet; break;
        case 13: v = f->reverb_size; break;
        case 14: v = f->reverb_decay_ms, m = 60000u; break;
        case 15: v = f->reverb_tone; break;
        case 16: v = f->reverb_pre_ms, m = 120u; break;
        case 17: v = f->reverb_level, m = 3000u; break;
        case 18: v = f->dist_level, m = 1400u; break;
        default: return false;
        }
    } else if (ui->page == MELLO_UI_SEQ_STEP) { /* the ranges as each adjust_* clamps them */
        const MelloStep *s = &e->project.steps[ui->selected_step % MELLO_MAX_STEPS];
        switch (ui->step_subpage * 4u + k) {
        case 0:
            v = (uint32_t)(s->pitch < 0 ? 0 : s->pitch);
            m = (uint32_t)e->project.pitch_span_oct * scale_len(e->project.scale) - 1u;
            break;
        case 1: v = (uint32_t)s->pulses - 1u, m = 7u; break;
        case 2: v = (uint32_t)s->ratchet - 1u, m = 7u; break;
        case 5: v = prob_pct(s->probability), m = 100u; break;
        case 12: v = e->project.seq_len, m = MELLO_MAX_STEPS; break;
        default: return false;
        }
    } else if (ui->page == MELLO_UI_SEQ_LOOP && ui->loop_subpage == 0u) {
        const MelloLoopCycleStep *ls =
            &e->loop_cycle.steps[ui->selected_loop_step % MELLO_MAX_LOOP_CYCLE_STEPS];
        int32_t sv = k == 1u ? ls->root_offset + 12 : (k == 2u ? ls->octave + 2 : ls->pitch + 12);
        v = k == 0u ? ls->hold : (uint32_t)(sv < 0 ? 0 : sv);
        m = k == 0u ? 256u : (k == 2u ? 4u : 24u);
    } else if (ui->page == MELLO_UI_SEQ_SLOTSEQ && (k == 1u || k == 3u)) {
        v = k == 1u ? e->slot_seq.steps[ui->selected_slot_seq_step % MELLO_MAX_SLOT_SEQ_STEPS].hold
                    : e->slot_seq.length;
        m = k == 1u ? 256u : MELLO_MAX_SLOT_SEQ_STEPS;
    } else if (ui->page == MELLO_UI_SLOTS && ui->slots_subpage == 0u && k == 1u) {
        v = e->visible_slot_count, m = MELLO_MAX_SLOTS;
    } else if (ui->page == MELLO_UI_SUB) {
        const MelloSubConfig *c = &e->sub;
        switch (ui->sub_subpage * 4u + k) {
        case 1: v = c->euclid_pulses, m = 64u; break;
        case 2: v = c->euclid_rotate, m = 64u; break;
        case 7:
        case 11: v = c->gate_amount, m = 1200u; break;
        case 12: v = c->level; break;
        case 14: v = c->decay_ms, m = 700u; break;
        case 15: v = c->release_ms, m = 1200u; break;
        case 16: v = c->glide_ms, m = 200u; break;
        case 17: v = c->cut; break;
        case 18: v = c->drive; break;
        default: return false;
        }
#if MELLO_FEATURE_CHORD_MACHINE
    } else if (ui->page == MELLO_UI_CHORD) {
        const MelloChordConfig *c = &e->chord;
        switch (ui->chord_subpage * 4u + k) {
        case 3: v = c->gate_amount, m = 1200u; break;
        case 13: v = c->grid_length, m = 16u; break;
        case 14: v = c->grid_amount; break;
        case 21: v = c->chorus; break;
        case 22: v = c->space; break;
        case 23: v = c->level; break;
        case 24: v = c->cut; break;
        case 25: v = c->res; break;
        case 26: v = c->fenv; break;
        case 28: v = c->fenv_attack_ms, m = 2000u; break;
        case 29: v = c->fenv_decay_ms, m = 4000u; break;
        case 30: v = c->strum_ms, m = 60u; break;
        case 32: v = c->attack_ms, m = 2000u; break;
        case 33: v = c->decay_ms, m = 4000u; break;
        case 34: v = c->sustain; break;
        case 35: v = c->release_ms, m = 4000u; break;
        default: return false;
        }
#endif
    } else if (ui->page == MELLO_UI_DRUMS) {
        const MelloDrumConfig *d = &e->drums;
        const MelloDrumVoice *dv = &d->voice[ui->selected_drum_voice % MELLO_DRUM_VOICES];
        const MelloDrumLock *dl =
            &d->lock[ui->selected_drum_voice % MELLO_DRUM_VOICES][ui->selected_drum_step % MELLO_DRUM_STEPS];
        switch ((ui->drum_subpage % DRUM_LAYERS) * 4u + k) {
        case 0: v = dv->tune; break;
        case 1: v = dv->decay; break;
        case 2: v = dv->tone; break;
        case 3: v = dv->level; break;
        case 4: v = (uint32_t)(dl->pitch + 24), m = 48u; break;
        case 5: v = (uint32_t)(dl->decay + 100), m = 200u; break;
        case 6: v = dl->crush, m = 100u; break;
        case 7: v = dl->ratchet - 1u, m = 3u; break;
        case 8: v = dv->chance; break;
        case 9: v = dv->crush; break;
        case 10: v = d->swing; break;
        case 11: v = d->roll; break;
        case 12: v = d->length - 1u, m = 15u; break;
        case 14: v = d->space; break;
        case 15: v = d->level; break;
        default: return false;
        }
    } else if (ui->page == MELLO_UI_SAMPLE_RESERVED) {
        const MelloSampleSlotConfig *sl = &e->sample.slots[e->sample.edit_slot % MELLO_SAMPLE_SLOT_COUNT];
        switch (ui->sample_subpage * 4u + k) {
        case 0:
        case 6: v = sl->level, m = 1500u; break;
        case 1:
        case 7: v = sl->dist; break;
        case 4: v = (uint32_t)(sl->coarse_semitones + 24), m = 48u; break;
        case 5: v = (uint32_t)(sl->fine_cents + 100), m = 200u; break;
        case 8: v = sl->trim_start; break;
        case 9: v = sl->trim_end; break;
        case 12: v = sl->attack_ms, m = 2000u; break;
        case 13: v = sl->decay_ms, m = 2000u; break;
        case 14: v = sl->sustain_permille; break;
        case 15: v = sl->release_ms, m = 3000u; break;
        default: return false;
        }
    } else if (ui->page == MELLO_UI_RANDOM && ui->random_edit && k == 0u) {
        v = e->modr.rnd_amount[random_target_bitno(ui->random_sel)], m = 100u;
    } else if (ui->page == MELLO_UI_GLOBAL && ui->global_subpage == 0u && k == 0u) {
        v = e->project.bpm > 20u ? e->project.bpm - 20u : 0u, m = 230u; /* 20..250 BPM */
    } else
        return false;
    *val = (uint16_t)(v > m ? m : v);
    *max = (uint16_t)m;
    return true;
}

/* K1-K4's labels and values on the current page and layer: the footer and the readout both show these. */
static void footer_texts(MelloUi *ui, const char *l[4], char v[4][24])
{
    MelloEngine *e = eng(ui);
    uint8_t k;
    for (k = 0u; k < 4u; ++k) {
        static const char *kn[4] = {"K1", "K2", "K3", "K4"};
        l[k] = kn[k];
        v[k][0] = 0;
    }
    if (!e) return;
    switch (ui->page) {
    case MELLO_UI_HOME: {
        uint8_t si = ui->selected_step < MELLO_MAX_STEPS ? ui->selected_step : 0u;
        MelloStep *s = &e->project.steps[si];
        if (ui->held_step == MELLO_UI_NO_STEP) {
            l[0] = "CUTOFF";
            value_cutoff(v[0], e->sound.filter_cut);
            l[1] = "DECAY";
            value_ms(v[1], e->sound.decay_ms);
            l[2] = "SPACE";
            value_pct(v[2], e->fx.reverb_wet);
            l[3] = "DRIVE";
            value_pct(v[3], e->fx.dist_amount);
            break;
        }
        l[0] = "PITCH";
        midi_note_name(v[0], mello_resolve_step_midi(&e->project, s));
        l[1] = "PULSE";
        value_u(v[1], "", s->pulses);
        l[2] = "RATCH";
        value_u(v[2], "", s->ratchet);
        l[3] = "GATE";
        copy_s(v[3], gate_name(s->gate));
        break;
    }
    case MELLO_UI_SEQ_STEP: {
        MelloStep *s = &e->project.steps[ui->selected_step];
        if (ui->step_subpage == 0u) {
            l[0] = "PITCH";
            value_i(v[0], "", s->pitch);
            l[1] = "PULSE";
            value_u(v[1], "", s->pulses);
            l[2] = "RATCH";
            value_u(v[2], "", s->ratchet);
            l[3] = "GATE";
            copy_s(v[3], gate_name(s->gate));
        } else if (ui->step_subpage == 1u) {
            l[0] = "OCT";
            value_i(v[0], "", s->octave);
            l[1] = "PROB";
            value_u(v[1], "", prob_pct(s->probability));
            l[2] = "SAMP";
            value_u(v[2], "", s->sample_slot);
            l[3] = "ACC";
            copy_s(v[3], s->accent ? "ON" : "OFF");
        } else if (ui->step_subpage == 2u) {
            l[0] = "ACC";
            copy_s(v[0], s->accent ? "ON" : "OFF");
            l[1] = "SUB";
            copy_s(v[1], s->sub ? "ON" : "OFF");
            l[2] = "SLIDE";
            copy_s(v[2], s->slide ? "ON" : "OFF");
            l[3] = "SKIP";
            copy_s(v[3], s->skip ? "ON" : "OFF");
        } else {
            l[0] = "LEN";
            value_u(v[0], "", e->project.seq_len);
            l[1] = "RESET";
            main_reset_name(v[1], e->project.master_len);
            l[2] = "DIV";
            value_u(v[2], "", e->project.clock_div);
            l[3] = "SPAN";
            value_u(v[3], "", e->project.pitch_span_oct);
        }
        break;
    }
    case MELLO_UI_SEQ_SEGMENT: {
        static const char *order[4] = {"FWD", "REV", "P-P", "RND"};
        uint8_t b[MELLO_MAX_SEGMENTS + 1u], sc = mello_segment_bounds(&e->project, b),
                sg = ui->selected_segment < sc ? ui->selected_segment : 0u;
        l[0] = "REPEAT";
        value_u(v[0], "", e->project.seg_repeats[sg]);
        l[1] = "ORDER";
        copy_s(v[1], e->project.seg_mode[sg] == MELLO_SEG_PINGPONG && e->project.seg_direction[sg] == MELLO_SEG_REV
                         ? "P-P R"
                         : order[segment_order(&e->project, sg)]);
        l[2] = "SKIP";
        copy_s(v[2], e->project.seg_skip[sg] ? "ON" : "OFF");
        l[3] = "LENGTH";
        value_u(v[3], "", (uint32_t)(b[sg + 1u] - b[sg]));
        break;
    }
    case MELLO_UI_SEQ_LOOP: {
        MelloLoopCycleStep *ls =
            &e->loop_cycle
                 .steps[ui->selected_loop_step < MELLO_MAX_LOOP_CYCLE_STEPS ? ui->selected_loop_step : 0u];
        if (ui->loop_subpage == 0u) {
            l[0] = "HOLD";
            value_u(v[0], "", ls->hold);
            l[1] = "ROOT+";
            value_i(v[1], "", ls->root_offset);
            l[2] = "OCT";
            value_i(v[2], "", ls->octave);
            l[3] = "TRANS";
            value_i(v[3], "", ls->pitch);
        } else if (ui->loop_subpage == 1u) {
            l[0] = "KEY";
            if (ls->root_note < 0)
                copy_s(v[0], "FOLLOW");
            else
                value_u(v[0], "", (uint32_t)ls->root_note);
            l[1] = "SCALE";
            if (ls->scale_set < 0)
                copy_s(v[1], "FOLLOW");
            else
                copy_s(v[1], mello_scale_name((MelloScale)ls->scale_set));
            l[2] = "SUB";
            value_u(v[2], "", mello_loop_cycle_sub_tone(ls));
            l[3] = "BASS TR";
            value_i(v[3], "", mello_loop_cycle_sub_transpose(ls));
        } else {
            l[0] = "LEN";
            value_u(v[0], "", e->loop_cycle.length);
            l[1] = "SYNC";
            copy_s(v[1], e->loop_cycle.sync ? "SYNC" : "FREE");
            l[2] = "ON";
            copy_s(v[2], e->loop_cycle.active ? "ON" : "OFF");
            l[3] = "K4";
            copy_s(v[3], "-");
        }
        break;
    }
    case MELLO_UI_SEQ_SLOTSEQ: {
        MelloSlotSeqStep *ss = &e->slot_seq.steps[ui->selected_slot_seq_step];
        l[0] = "SLOT";
        if (ss->slot < 0)
            copy_s(v[0], "OFF");
        else
            value_u(v[0], "", (uint32_t)ss->slot + 1u);
        l[1] = "HOLD";
        value_u(v[1], "", ss->hold);
        l[2] = "ON";
        copy_s(v[2], e->slot_seq.active ? "ON" : "OFF");
        l[3] = "LEN";
        value_u(v[3], "", e->slot_seq.length);
        break;
    }
    case MELLO_UI_RANDOM:
        if (ui->random_edit) {
            static const char *rl[13] = {"PIT", "PUL", "RAT", "GAT", "OCT", "ACC", "SUB",
                                         "SLD", "SKP", "PRB", "SMP", "CHD", "WAV"};
            char *q = copy_s(v[0], rl[ui->random_sel % 13u]); /* "PIT 60%" */
            l[0] = "AMOUNT";
            *q++ = ' ';
            copy_s(put_u32(q, e->modr.rnd_amount[random_target_bitno(ui->random_sel)]), "%");
            copy_s(v[1], "-");
            copy_s(v[2], "-");
            copy_s(v[3], "-");
            break;
        }
        l[0] = "INT";
        copy_s(v[0], random_intensity_name(ui->random_intensity));
        copy_s(v[1], "-");
        copy_s(v[2], "-");
        copy_s(v[3], "-");
        break;
    case MELLO_UI_SLOTS:
        if (ui->slots_subpage == 0u) {
            copy_s(v[0], "-");
            l[1] = "VISIBLE";
            value_u(v[1], "", e->visible_slot_count);
            copy_s(v[2], "-");
            copy_s(v[3], "-");
        } else {
            MelloPatternSlot *sl = &e->slots[ui->selected_slot];
            l[0] = "CURSOR";
            value_u(v[0], "", ui->slot_name_cursor + 1u);
            l[1] = "CHAR";
            if (sl->valid && sl->name[ui->slot_name_cursor]) {
                v[1][0] = sl->name[ui->slot_name_cursor];
                v[1][1] = 0;
            } else
                copy_s(v[1], "-");
            copy_s(v[2], "-");
            copy_s(v[3], "-");
        }
        break;
    case MELLO_UI_DOT: {
        MelloDotMatrixConfig *c = &e->dot[ui->selected_dot_target % MELLO_DOT_MATRIX_COUNT];
        int8_t lane = dot_lane(ui->selected_dot_target);
        if (ui->dot_shape && lane >= 0) {
            l[0] = "TARGET";
            copy_s(v[0], mod_dest_name(e->modr.dot_target[lane]));
            l[1] = "ATK";
            value_ms(v[1], e->modr.dot_attack_ms[lane]);
            l[2] = "REL";
            value_ms(v[2], e->modr.dot_release_ms[lane]);
            l[3] = "HOLD";
            copy_s(v[3], e->modr.dot_gate[lane] ? "GATE" : "HIT");
            break;
        }
        l[0] = "MODE";
        copy_s(v[0], dot_mode_name(c->mode));
        l[1] = "LEN";
        value_u(v[1], "", c->length);
        l[2] = lane >= 0 ? "DEPTH"
                         : "CHANCE"; /* a MOD lane's amount moves its target; the others' is a chance */
        value_pct(v[2], c->amount);
        l[3] = "EVOLVE";
        value_pct(v[3], e->modr.dot_evolve[ui->selected_dot_target % MELLO_DOT_MATRIX_COUNT]);
        break;
    }
    case MELLO_UI_TURING:
        if (ui->tm_subpage == 0u) {
            l[0] = "MUT";
            value_u(v[0], "", e->tm.mutation_prob / 10u);
            l[1] = "LEN";
            value_u(v[1], "", e->tm.length);
            l[2] = "RATE";
            copy_s(v[2], e->tm.rate == MELLO_TM_RATE_HALF     ? "1/2X"
                         : e->tm.rate == MELLO_TM_RATE_DOUBLE ? "2X"
                                                              : "1X");
            l[3] = "OFFSET";
            value_i(v[3], "", e->tm.offset);
        } else if (ui->tm_subpage == 3u) {
            uint8_t r;
            for (r = 0u; r < 2u; ++r) {
                l[r * 2u] = r ? "DEST B" : "DEST A";
                copy_s(v[r * 2u], mod_dest_name(e->modr.tm_dest[r]));
                l[r * 2u + 1u] = r ? "DEPTH B" : "DEPTH A";
                copy_s(
                    put_i32(v[r * 2u + 1u], (e->modr.tm_depth[r] + (e->modr.tm_depth[r] < 0 ? -5 : 5)) / 10),
                    "%");
            }
        } else if (ui->tm_subpage == 1u) {
            l[0] = "GATE";
            copy_s(v[0], e->tm.routes & MELLO_TM_ROUTE_GATE ? "ON" : "OFF");
            l[1] = "FILTER";
            copy_s(v[1], e->tm.routes & MELLO_TM_ROUTE_FILTER ? "ON" : "OFF");
            l[2] = "OCT";
            copy_s(v[2], e->tm.routes & MELLO_TM_ROUTE_OCTAVE ? "ON" : "OFF");
            l[3] = "DECAY";
            copy_s(v[3], e->tm.routes & MELLO_TM_ROUTE_DECAY ? "ON" : "OFF");
        } else {
            l[0] = "GATE";
            value_u(v[0], "", e->tm.depth_gate / 10u);
            l[1] = "FILTER";
            value_u(v[1], "", e->tm.depth_filter / 10u);
            l[2] = "OCT";
            value_u(v[2], "", e->tm.depth_octave / 10u);
            l[3] = "DECAY";
            value_u(v[3], "", e->tm.depth_decay / 10u);
        }
        break;
    case MELLO_UI_SUB:
        if (ui->sub_subpage == 0u) {
            l[0] = "MODE";
            copy_s(v[0], sub_mode_name(e->sub.mode));
            l[1] = "PULSES";
            value_u(v[1], "", e->sub.euclid_pulses);
            l[2] = "ROT";
            value_u(v[2], "", e->sub.euclid_rotate);
            l[3] = "OCT";
            value_i(v[3], "", e->sub.octave);
        } else if (ui->sub_subpage == 1u) {
            l[0] = "5SEQ ON";
            copy_s(v[0], e->sub.x_on ? "ON" : "OFF");
            l[1] = "5SEQLEN";
            value_u(v[1], "", e->sub.x_length);
            l[2] = "RESET";
            copy_s(v[2], e->sub.x_reset ? "ON" : "OFF");
            l[3] = "GATE";
            value_u(v[3], "", e->sub.gate_amount / 10u);
        } else if (ui->sub_subpage == 2u) {
            MelloSubXStep *xs = &e->sub.x_steps[ui->selected_sub_x_step];
            l[0] = "ON";
            copy_s(v[0], xs->on ? "ON" : "OFF");
            l[1] = "TONE";
            value_u(v[1], "", xs->tone);
            l[2] = "OCT";
            value_i(v[2], "", xs->octave);
            l[3] = "GATE";
            value_u(v[3], "", e->sub.gate_amount / 10u);
        } else if (ui->sub_subpage == 3u) {
            l[0] = "LEVEL";
            value_u(v[0], "", e->sub.level / 10u);
            l[1] = "WAVE";
            copy_s(v[1], wave_name(e->sub.wave));
            l[2] = "DEC";
            value_ms(v[2], e->sub.decay_ms);
            l[3] = "REL";
            value_ms(v[3], e->sub.release_ms);
        } else {
            l[0] = "GLIDE";
            value_ms(v[0], e->sub.glide_ms);
            l[1] = "CUT";
            value_u(v[1], "", e->sub.cut / 10u);
            l[2] = "DRIVE";
            value_u(v[2], "", e->sub.drive / 10u);
            l[3] = "OCT";
            value_i(v[3], "", e->sub.octave);
        }
        break;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: {
        static const char *lab[CHORD_LAYERS][4] = {
            {"MODE", "TRIG", "CHAIN", "GATE"},      {"DEG", "SHAPE", "HOLD", "OCT"},
            {"INV", "SPREAD", "VOICES", "VLEAD"},   {"MODE", "LEN", "DEPTH", "FOLLOW"},
            {"MASK", "LEN", "K3", "K4"},              {"WAVE", "CHORUS", "SPACE", "LEVEL"},
            {"CUT", "RES", "ENV", "LINK"},          {"F ATK", "F DEC", "STRUM", "K4"},
            {"ATTACK", "DECAY", "SUSTAIN", "RELEASE"}};
        MelloChordConfig *c = &e->chord;
        MelloChordStep *s = &c->steps[ui->selected_chord_step % c->length];
        uint8_t L = ui->chord_subpage % CHORD_LAYERS;
        for (k = 0u; k < 4u; ++k) {
            l[k] = lab[L][k];
            copy_s(v[k], "-");
        }
        switch (L) {
        case 0:
            copy_s(v[0], chord_mode_name(c->mode));
            copy_s(v[1], c->trigger == MELLO_CHORD_TRIGGER_GRID ? "GRID" : "STEP");
            value_u(v[2], "", c->length);
            value_u(v[3], "", c->gate_amount / 10u);
            break;
        case 1:
            value_u(v[0], "", s->degree + 1u);
            copy_s(v[1], chord_shape_name(s->shape));
            value_u(v[2], "", s->hold);
            value_i(v[3], "", s->octave);
            break;
        case 2:
            value_u(v[0], "", s->inversion);
            value_u(v[1], "", s->spread);
            value_u(v[2], "", c->voices);
            copy_s(v[3], c->voice_lead ? "ON" : "OFF");
            break;
        case 3:
            copy_s(v[0], dot_mode_name(c->grid_mode));
            value_u(v[1], "", c->grid_length);
            value_u(v[2], "", c->grid_amount / 10u);
            copy_s(v[3], c->grid_follow ? "ON" : "OFF");
            break;
        case 4:
            value_u(v[0], "", c->pattern_masks[ui->selected_chord_pattern % c->pattern_length]);
            value_u(v[1], "", c->pattern_length);
            break;
        case 5:
            copy_s(v[0], wave_name(c->wave));
            value_u(v[1], "", c->chorus / 10u);
            value_u(v[2], "", c->space / 10u);
            value_u(v[3], "", c->level / 10u);
            break;
        case 6:
            value_u(v[0], "", c->cut / 10u);
            value_u(v[1], "", c->res / 10u);
            value_u(v[2], "", c->fenv / 10u);
            copy_s(v[3], c->link ? "ON" : "OFF");
            break;
        case 7:
            value_ms(v[0], c->fenv_attack_ms);
            value_ms(v[1], c->fenv_decay_ms);
            value_ms(v[2], c->strum_ms);
            break;
        default:
            value_ms(v[0], c->attack_ms);
            value_ms(v[1], c->decay_ms);
            value_u(v[2], "", c->sustain / 10u);
            value_ms(v[3], c->release_ms);
            break;
        }
        break;
    }
#endif
    case MELLO_UI_KEY_SCALE:
        l[0] = "ROOT";
        value_u(v[0], "", e->project.root_note);
        l[1] = "SCALE";
        copy_s(v[1], mello_scale_name(e->project.scale));
        l[2] = "OCT";
        value_i(v[2], "", e->project.global_octave);
        l[3] = "SPAN";
        value_u(v[3], "", e->project.pitch_span_oct);
        break;
    case MELLO_UI_GLOBAL:
        if (ui->global_subpage == 0u) {
            l[0] = "BPM";
            value_u(v[0], "", mello_transport_effective_bpm(ui->transport));
            l[1] = "DIV";
            value_u(v[1], "", e->project.clock_div);
            l[2] = "SYNC";
            copy_s(v[2], sync_name(ui->transport->sync_mode));
            l[3] = "NOTEOUT";
            copy_s(v[3], ui->transport->midi_note_out_enabled ? "ON" : "OFF");
        } else {
            l[0] = "LEAD";
            value_u(v[0], "", (uint32_t)ui->transport->midi_channel + 1u);
            l[1] = "SUB";
            value_u(v[1], "", (uint32_t)ui->transport->midi_sub_channel + 1u);
            l[2] = "LEN";
            copy_s(v[2], ui->transport->midi_fixed_length ? "FIXED" : "STEP");
            l[3] = "MS";
            value_u(v[3], "", ui->transport->midi_fixed_len_ms);
        }
        break;
    case MELLO_UI_SOUND_RESERVED:
        if (ui->sound_subpage == 0u) {
            l[0] = "WAVE";
            copy_s(v[0], wave_name(e->sound.osc_wave));
            l[1] = "PWM";
            value_pct(v[1], e->sound.pwm_permille);
            l[2] = "CUT";
            value_cutoff(v[2], e->sound.filter_cut);
            l[3] = "RES";
            value_pct(v[3], e->sound.filter_res);
        } else if (ui->sound_subpage == 1u) {
            l[0] = "ATK";
            value_ms(v[0], e->sound.attack_ms);
            l[1] = "DEC";
            value_ms(v[1], e->sound.decay_ms);
            l[2] = "SUS";
            value_pct(v[2], e->sound.sustain_permille);
            l[3] = "REL";
            value_ms(v[3], e->sound.release_ms);
        } else if (ui->sound_subpage == 2u) {
            l[0] = "LFO";
            copy_s(v[0], e->sound.lfo_on ? "ON" : "OFF");
            l[1] = "RATE";
            value_millihz(v[1], e->sound.lfo_rate_millihz);
            l[2] = "DEPTH";
            value_pct(v[2], e->sound.lfo_depth);
            l[3] = "TARGET";
            copy_s(v[3], lfo_target_name(e->sound.lfo_target));
        } else if (ui->sound_subpage == 3u) {
            l[0] = "SUBOSC";
            copy_s(v[0], e->sound.follower_sub_on ? "ON" : "OFF");
            l[1] = "LEVEL";
            value_pct(v[1], e->sound.follower_sub_level);
            l[2] = "OCT";
            value_i(v[2], "", e->sound.follower_sub_octave);
            l[3] = "WAVE";
            copy_s(v[3], wave_name(e->sound.follower_sub_wave));
        } else if (ui->sound_subpage == 6u) {
            const MelloSoundConfig *c = &e->sound;
            l[0] = "WAVE";
            copy_s(v[0], wave_name(c->osc2_wave));
            l[1] = "SEMI";
            copy_s(put_i32(v[1], c->osc2_semi), " ST");
            l[2] = "FINE";
            copy_s(put_i32(v[2], c->osc2_fine), " CT");
            l[3] = "MIX";
            value_pct(v[3], c->osc2_mix);
        } else if (ui->sound_subpage == 7u) {
            const MelloSoundConfig *c = &e->sound;
            l[0] = "SYNC";
            copy_s(v[0], c->osc2_sync ? "ON" : "OFF");
            l[1] = "FM";
            value_pct(v[1], c->fm_amount);
            l[2] = "SAT";
            value_pct(v[2], c->saturation);
            l[3] = "FILTER";
            copy_s(v[3], filter_mode_name(c->filter_mode));
        } else if (ui->sound_subpage == 8u) {
            const MelloSoundConfig *c = &e->sound;
            l[0] = "AMT";
            copy_s(put_i32(v[0], (c->menv_amount + (c->menv_amount < 0 ? -5 : 5)) / 10), "%");
            l[1] = "ATK";
            value_ms(v[1], c->menv_attack_ms);
            l[2] = "DEC";
            value_ms(v[2], c->menv_decay_ms);
            l[3] = "TARGET";
            copy_s(v[3], menv_target_name(c->menv_target));
        } else if (ui->sound_subpage == 9u) {
            const MelloSoundConfig *c = &e->sound;
            l[0] = "V.CUT";
            copy_s(put_i32(v[0], (c->vel_cutoff + (c->vel_cutoff < 0 ? -5 : 5)) / 10), "%");
            l[1] = "V.LVL";
            value_pct(v[1], c->vel_level);
            l[2] = "KEYTRK";
            value_pct(v[2], c->key_track);
            l[3] = "V.ENV";
            copy_s(put_i32(v[3], (c->vel_menv + (c->vel_menv < 0 ? -5 : 5)) / 10), "%");
        } else if (ui->sound_subpage == 5u) {
            uint32_t d = e->sound.detune;
            l[0] = "NOISE";
            value_pct(v[0], e->sound.noise_level);
            l[1] = "N.FM";
            value_pct(v[1], e->sound.noise_fm);
            l[2] = "UNISON";
            value_u(v[2], "", e->sound.unison);
            l[3] = "DETUNE"; /* the outer oscillators' offset, as mello_audio: 60 cents * (d / 1000)^2 */
            value_u(v[3], "", d * d / 1000u * 60u / 1000u);
            copy_s(v[3] + ui_text_len(v[3]), " CT");
        } else {
            l[0] = "SLIDE";
            value_ms(v[0], e->sound.slide_ms);
            l[1] = "MASTER";
            value_pct(v[1], e->sound.master_level);
            copy_s(v[2], "-");
            copy_s(v[3], "-");
        }
        break;
    case MELLO_UI_AUDIO_FX:
        if (ui->fx_subpage == 0u) {
            l[0] = "AMT";
            value_pct(v[0], e->fx.dist_amount);
            l[1] = "MIX";
            value_pct(v[1], e->fx.dist_mix);
            l[2] = "CHAR";
            value_pct(v[2], e->fx.dist_character);
            l[3] = "TONE";
            value_pct(v[3], e->fx.dist_tone);
        } else if (ui->fx_subpage == 1u) {
            l[0] = "WET";
            value_pct(v[0], e->fx.delay_wet);
            l[1] = "FDBK";
            value_pct(v[1], e->fx.delay_feedback);
            l[2] = "LO CUT";
            if (e->fx.delay_lowcut)
                (void)copy_s(put_u32(v[2], delay_lowcut_hz(e->fx.delay_lowcut)), "HZ");
            else
                copy_s(v[2], "OFF");
            l[3] = "HI CUT";
            value_pct(v[3], e->fx.delay_tone);
        } else if (ui->fx_subpage == 2u) {
            l[0] = "SYNC";
            copy_s(v[0], e->fx.delay_sync ? "ON" : "OFF");
            l[1] = "DIV";
            copy_s(v[1], delay_div_name(e->fx.delay_div));
            l[2] = "TIME";
            value_ms(v[2], e->fx.delay_time_ms);
            l[3] = "LEVEL";
            value_pct(v[3], e->fx.delay_level);
        } else if (ui->fx_subpage == 3u) {
            l[0] = "WET";
            value_pct(v[0], e->fx.reverb_wet);
            l[1] = "SIZE";
            value_pct(v[1], e->fx.reverb_size);
            l[2] = "DECAY";
            value_ms(v[2], e->fx.reverb_decay_ms);
            l[3] = "TONE";
            value_pct(v[3], e->fx.reverb_tone);
        } else {
            l[0] = "PRE";
            value_ms(v[0], e->fx.reverb_pre_ms);
            l[1] = "LEVEL";
            value_pct(v[1], e->fx.reverb_level);
            l[2] = "D.LEVEL";
            value_pct(v[2], e->fx.dist_level);
            copy_s(v[3], "-");
        }
        break;
    case MELLO_UI_DRUMS: {
        static const char *lab[DRUM_LAYERS][4] = {{"TUNE", "DECAY", "TONE", "LEVEL"},
                                                  {"PITCH", "DECAY", "CRUSH", "RATCH"},
                                                  {"CHANCE", "CRUSH", "SWING", "ROLL"},
                                                  {"LENGTH", "FOLLOW", "SPACE", "LEVEL"}};
        const MelloDrumConfig *d = &e->drums;
        const MelloDrumVoice *dv = &d->voice[ui->selected_drum_voice % MELLO_DRUM_VOICES];
        const MelloDrumLock *dl =
            &d->lock[ui->selected_drum_voice % MELLO_DRUM_VOICES][ui->selected_drum_step % MELLO_DRUM_STEPS];
        uint8_t L = ui->drum_subpage % DRUM_LAYERS;
        for (k = 0u; k < 4u; ++k) l[k] = lab[L][k];
        if (L == 0u) {
            value_pct(v[0], dv->tune);
            value_pct(v[1], dv->decay);
            value_pct(v[2], dv->tone);
            value_pct(v[3], dv->level);
        } else if (L == 1u) {
            value_i(v[0], "", dl->pitch);
            value_i(v[1], "", dl->decay);
            value_u(v[2], "", dl->crush);
            value_u(v[3], "", dl->ratchet);
        } else if (L == 2u) {
            value_pct(v[0], dv->chance);
            value_pct(v[1], dv->crush);
            value_pct(v[2], d->swing);
            value_pct(v[3], d->roll);
        } else {
            value_u(v[0], "", d->length);
            copy_s(v[1], d->follow ? "ON" : "OFF");
            value_pct(v[2], d->space);
            value_pct(v[3], d->level);
        }
        break;
    }
    case MELLO_UI_SAMPLE_RESERVED: {
        MelloSampleSlotConfig *sl =
            &e->sample.slots[e->sample.edit_slot < MELLO_SAMPLE_SLOT_COUNT ? e->sample.edit_slot : 0u];
        if (ui->sample_subpage == 0u) {
            l[0] = "LEVEL";
            value_u(v[0], "", sl->level / 10u);
            l[1] = "DIST";
            value_u(v[1], "", sl->dist / 10u);
            l[2] = "TRACK";
            copy_s(v[2], sl->track ? "ON" : "OFF");
            l[3] = "LOOP";
            copy_s(v[3], sl->loop ? "ON" : "OFF");
        } else if (ui->sample_subpage == 1u) {
            l[0] = "COARSE";
            value_i(v[0], "", sl->coarse_semitones);
            l[1] = "FINE";
            value_i(v[1], "", sl->fine_cents);
            l[2] = "LEVEL";
            value_u(v[2], "", sl->level / 10u);
            l[3] = "DIST";
            value_u(v[3], "", sl->dist / 10u);
        } else if (ui->sample_subpage == 2u) {
            l[0] = "START";
            value_u(v[0], "", sl->trim_start / 10u);
            l[1] = "END";
            value_u(v[1], "", sl->trim_end / 10u);
            l[2] = "TRACK";
            copy_s(v[2], sl->track ? "ON" : "OFF");
            l[3] = "LOOP";
            copy_s(v[3], sl->loop ? "ON" : "OFF");
        } else {
            l[0] = "ATK";
            value_ms(v[0], sl->attack_ms);
            l[1] = "DEC";
            value_ms(v[1], sl->decay_ms);
            l[2] = "SUS";
            value_u(v[2], "", sl->sustain_permille / 10u);
            l[3] = "REL";
            value_ms(v[3], sl->release_ms);
        }
        break;
    }
    default:
        copy_s(v[0], "-");
        copy_s(v[1], "-");
        copy_s(v[2], "-");
        copy_s(v[3], "-");
        break;
    }
}

/* The colour of K1-K4's value: the page's, except where the HTML colours each control: HOME's macros by domain and
 * the envelope's A D S R letters. */
static uint16_t knob_colour(MelloUi *ui, uint8_t k)
{
    static const uint16_t macro[4] = {UI_CYAN, UI_GOLD, UI_ACC, UI_CORAL};
    static const uint16_t adsr[4] = {UI_GOLD, UI_CORAL, UI_CYAN, UI_PURPLE};
    static const uint16_t route[4] = {UI_ACC, UI_CYAN, UI_BLUE, UI_CORAL};
    k &= 3u;
    if (ui->page == MELLO_UI_HOME && ui->held_step == MELLO_UI_NO_STEP) return macro[k];
    if (ui->page == MELLO_UI_SOUND_RESERVED && ui->sound_subpage == 1u) return adsr[k];
    if (ui->page == MELLO_UI_TURING && ui->tm_subpage >= 1u) return route[k];
    return ui_page_accent(ui);
}

static const uint16_t EXP_NEG_Q15[65] = {
    /* 32767 * e^(-i / 8): the envelope curve's exponential parts */
    32767, 28917, 25519, 22520, 19874, 17539, 15478, 13659, 12054, 10638, 9388, 8285, 7311,
    6452,  5694,  5025,  4435,  3913,  3454,  3048,  2690,  2374,  2095,  1849, 1631, 1440,
    1271,  1121,  989,   873,   771,   680,   600,   530,   467,   412,   364,  321,  283,
    250,   221,   195,   172,   152,   134,   118,   104,   92,    81,    72,   63,   56,
    49,    43,    38,    34,    30,    26,    23,    21,    18,    16,    14,   12,   11,
};

/* e^(-t / tau) in Q15 for t / tau in 1/8 steps (0 past 8). */
static int32_t exp_neg_q15(int32_t eighths)
{
    return eighths >= 64 ? 0 : EXP_NEG_Q15[eighths < 0 ? 0 : eighths];
}

/* The amp envelope's curve over the panel's top: attack (linear), decay and release (Tone's exponential approach,
 * tau = D / ln 200 roughly, as mello_audio's TONE_ENV) and a held sustain, each in its letter's colour. The times
 * share 3/4 of the width by their length, the sustain hold the rest. */
static void draw_env_curve(const MelloUiDrawOps *o, const MelloSoundConfig *c, int16_t x0, int16_t y0,
                           int16_t w, int16_t h)
{
    static const uint16_t col[4] = {UI_GOLD, UI_CORAL, UI_CYAN, UI_PURPLE};
    uint32_t a = c->attack_ms, d = c->decay_ms, r = c->release_ms, sum = a + d + r, s = c->sustain_permille;
    int16_t wa, wd, ws, wr, x;
    if (sum == 0u) sum = 1u;
    wa = (int16_t)((uint32_t)(w * 3 / 4) * a / sum);
    wd = (int16_t)((uint32_t)(w * 3 / 4) * d / sum);
    wr = (int16_t)((uint32_t)(w * 3 / 4) * r / sum);
    if (wa < 2) wa = 2;
    if (wd < 3) wd = 3;
    if (wr < 3) wr = 3;
    ws = (int16_t)(w - wa - wd - wr);
    o->fill(o->ctx, x0, (int16_t)(y0 + h - 1), w, 1, UI_LINE); /* the floor */
    for (x = 0; x < w; ++x) {
        int32_t lv; /* 0..1000 */
        uint8_t seg;
        int16_t y;
        if (x < wa) {
            seg = 0u, lv = (int32_t)(1000 * (x + 1) / wa);
        } else if (x < wa + wd) {
            int32_t t8 =
                (int32_t)((x - wa) * 8 * 5 / wd); /* five time constants over the decay (ln 200 ~ 5.3) */
            seg = 1u, lv = (int32_t)s + ((1000 - (int32_t)s) * exp_neg_q15(t8)) / 32767;
        } else if (x < wa + wd + ws) {
            seg = 2u, lv = (int32_t)s;
        } else {
            int32_t t8 = (int32_t)((x - wa - wd - ws) * 8 * 5 / wr);
            seg = 3u, lv = ((int32_t)s * exp_neg_q15(t8)) / 32767;
        }
        y = (int16_t)(y0 + h - 2 - (lv * (h - 3)) / 1000);
        if (y0 + h - 1 - y - 2 > 0)
            o->fill(o->ctx, (int16_t)(x0 + x), (int16_t)(y + 2), 1, (int16_t)(y0 + h - 1 - y - 2),
                    ui_mix(col[seg], UI_PANEL, 200));          /* the area, faint */
        o->fill(o->ctx, (int16_t)(x0 + x), y, 1, 2, col[seg]); /* the line */
    }
}

/* AMP ENVELOPE as the HTML's: the curve, then A D S R rows of letter, slider and value. */
static void draw_env_panel(MelloUi *ui, const MelloUiDrawOps *o)
{
    static const char *letter[4] = {"A", "D", "S", "R"};
    MelloEngine *e = eng(ui);
    char v[4][24];
    const char *l[4];
    uint8_t k;
    if (!e) return;
    footer_texts(ui, l, v);
    ui_round_fill(o, 4, 31, 232, 138, UI_PANEL);
    ui_round_frame(o, 4, 31, 232, 138, ui_mix(UI_GOLD, UI_BLACK, 150));
    ui_spaced(o, 10, 34, 120, "ENVELOPE", UI_GOLD, 0);
    draw_env_curve(o, &e->sound, 12, 52, 216, 44);
    for (k = 0u; k < 4u; ++k) {
        int16_t y = (int16_t)(100 + k * 17);
        uint16_t c = knob_colour(ui, k), nv, nm;
        o->text(o->ctx, 10, y, 10, 0, letter[k], c, 0);
        if (knob_norm(ui, k, &nv, &nm)) ui_slider(o, 24, (int16_t)(y + 5), 146, nv, nm, c);
        o->text(o->ctx, 174, y, 56, 0, v[k], c, 2);
    }
}

/* A layer as the HTML's synth panel: its title spaced in the domain's colour, then K1-K4 as rows, each a label and
 * its value, with the HTML's slider under a continuous value and a pill around a list's or a switch's. The texts are
 * the footer's (footer_texts), the colours knob_colour's (the envelope's A D S R as the HTML's letters). */
static void draw_param_panel(MelloUi *ui, const MelloUiDrawOps *o)
{
    char v[4][24];
    const char *l[4], *title = layer_name(ui);
    uint16_t accent = ui_page_accent(ui);
    uint8_t k;
    if (ui->page == MELLO_UI_SOUND_RESERVED && ui->sound_subpage == 1u) {
        draw_env_panel(ui, o);
        return;
    }
    footer_texts(ui, l, v);
    ui_round_fill(o, 4, 31, 232, 138, UI_PANEL);
    ui_round_frame(o, 4, 31, 232, 138, ui_mix(accent, UI_BLACK, 150));
    ui_spaced(o, 10, 34, 220, title ? title : "AUDIO LFO", accent, 0);
    for (k = 0u; k < 4u; ++k) {
        int16_t y = (int16_t)(56 + k * 28);
        uint16_t c = knob_colour(ui, k), nv, nm;
        if (!v[k][0] || (v[k][0] == '-' && !v[k][1])) continue; /* an empty knob: no row */
        ui_spaced(o, 10, y, 128, full_label(l[k]), c == accent ? UI_DIM : c, 0);
        if (knob_norm(ui, k, &nv, &nm)) {
            o->text(o->ctx, 138, y, 92, 0, v[k], c, 2);
            ui_slider(o, 10, (int16_t)(y + 18), 220, nv, nm, c);
        } else {
            int16_t w = (int16_t)(ui_text_len(v[k]) * 8u + 8u);
            ui_round_frame(o, (int16_t)(230 - w), (int16_t)(y - 1), w, 18, c);
            o->text(o->ctx, (int16_t)(234 - w), y, (int16_t)(w - 8), 0, v[k], c, 1);
        }
    }
}

static void draw_footer(MelloUi *ui, const MelloUiDrawOps *o)
{
    char v[4][24];
    const char *l[4];
    uint8_t k;
    footer_texts(ui, l, v);
    draw_control_hint(ui, o);
    for (k = 0u; k < 4u; ++k) footer_pair(o, k, l[k], v[k], knob_colour(ui, k));
}

/* The save word on the status line: 0 none (no storage), 1 SAVED, 2 UNSAVED (written when stopped and silent),
 * 3 SAVE OFF (the flash holds a newer firmware's project: nothing is written). */
static uint8_t save_state(const MelloUi *ui)
{
    const MelloStorage *st = ui->storage;
    if (!st || !st->io.read) return 0u;
    if (st->last_save_result == MELLO_STORAGE_SAVE_PROTECTED) return 3u;
    return st->dirty || st->manual_pending ? 2u : 1u;
}

static void draw_status(MelloUi *ui, const MelloUiDrawOps *o)
{
    char b[48], *p;
    MelloEngine *e = eng(ui);
    uint8_t sv = save_state(ui);
    static const char *sw[4] = {"", "SAVED", "UNSAVED", "SAVE OFF"};
    static const uint16_t sc[4] = {UI_DIM, UI_ACC, UI_CORAL, UI_RED};
    o->fill(o->ctx, 0, 220, 240, 20, UI_PANEL2);
    if (ui->transport->running && ui->transport->display_step != 0xFFu) {
        p = copy_s(b, "STEP ");
        p = put_u32(p, (uint32_t)ui->transport->display_step + 1u);
        if (e) {
            p = copy_s(p, "/");
            p = put_u32(p, e->project.seq_len);
        }
    } else { /* stopped: just the length */
        p = put_u32(b, e ? e->project.seq_len : 0u);
        p = copy_s(p, " STEPS");
    }
    if (e && e->active_slot >= 0) {
        p = copy_s(p, "  SLOT ");
        put_u32(p, (uint32_t)e->active_slot + 1u);
    }
    o->text(o->ctx, 4, 222, 140, 0, b, UI_MID, 0);
    o->text(o->ctx, 148, 222, 88, 0, sw[sv], sc[sv], 2);
    ui->seen_save_state = sv;
}

/* The render's draw ops wrap the platform's: they keep the last fills, so each text is drawn on the colour it
 * sits on (text_on) rather than on a black box, without every text call naming its background. */
#define FIT_FILLS 24u
typedef struct {
    const MelloUiDrawOps *raw;
    int16_t fx[FIT_FILLS], fy[FIT_FILLS], fw[FIT_FILLS], fh[FIT_FILLS];
    uint16_t fc[FIT_FILLS];
    uint8_t fn, fhead;
} MelloUiFitCtx;

static void fit_fill(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour)
{
    MelloUiFitCtx *f = (MelloUiFitCtx *)ctx;
    uint8_t i = f->fhead;
    f->raw->fill(f->raw->ctx, x, y, w, h, colour);
    if (w < 8 || h < 8) return; /* lines and dots are not backgrounds */
    f->fx[i] = x, f->fy[i] = y, f->fw[i] = w, f->fh[i] = h, f->fc[i] = colour;
    f->fhead = (uint8_t)((i + 1u) % FIT_FILLS);
    if (f->fn < FIT_FILLS) ++f->fn;
}
/* The colour of the latest fill that holds the box, else the ground. */
static uint16_t fit_bg(const MelloUiFitCtx *f, int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint8_t n;
    for (n = 1u; n <= f->fn; ++n) {
        uint8_t i = (uint8_t)((f->fhead + FIT_FILLS - n) % FIT_FILLS);
        if (x >= f->fx[i] && y >= f->fy[i] && x + w <= f->fx[i] + f->fw[i] && y + h <= f->fy[i] + f->fh[i])
            return f->fc[i];
    }
    return UI_BLACK;
}

static void fit_text(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large, const char *text,
                     uint16_t colour, uint8_t align)
{
    MelloUiFitCtx *f = (MelloUiFitCtx *)ctx;
    uint16_t n = ui_text_len(text);
    if (large == MELLO_UI_TEXT_SPACED && (uint32_t)n * 9u > (uint32_t)(w > 0 ? w : 0) + 1u) large = 0u;
    if (large == 1u && (uint32_t)n * 16u > (uint32_t)(w > 0 ? w : 0)) large = 0u;
    if (f->raw->text_on)
        f->raw->text_on(f->raw->ctx, x, y, w, large, text, colour,
                        fit_bg(f, x, y, w, (int16_t)(large == 1u ? 32 : 16)), align);
    else
        f->raw->text(f->raw->ctx, x, y, w, large == 1u ? 1u : 0u, text, colour, align);
}

/* The page's name as the header and the readout show it: LFO, DOT and EVOLVE are one domain, MOD (the LFO
 * button), whatever page holds them. */
static const char *domain_name(const MelloUi *ui)
{
    if (ui->page == MELLO_UI_SOUND_RESERVED && ui->sound_subpage == 2u) return "MOD LFO";
    if (ui->page == MELLO_UI_DOT) return "MOD DOT";
    if (ui->page == MELLO_UI_TURING) return "MOD EVOLVE";
    return mello_ui_page_name(ui->page);
}

/* The name of the layer SELECT shows on the current page, or 0 where a page has none. */
static const char *layer_name(const MelloUi *ui)
{
    static const char *step[4] = {"TRIG", "NOTE", "FLAGS", "SEQ"}, *loop[3] = {"STEP", "HARM", "LOOP"}, *slots[2] = {"LOAD", "RENAME"},
                      *dot[5] = {"ACCENT", "MOD 1", "RATCHET", "SUB", "MOD 2"},
                      *dsh[2] = {"MOD 1 SHAPE", "MOD 2 SHAPE"}, *tm[4] = {"MAIN", "ROUTES", "DEPTH", "SOUND"},
                      *sub[5] = {"TRIG", "5-SEQ", "STEP", "VOICE", "TONE"}, *glo[2] = {"CLOCK", "MIDI"},
                      *snd[10] = {"OSC / FILTER", "AMP ENVELOPE", "AUDIO LFO", "FOLLOWER SUB", "PLAY / LEVEL",
                                  "GRIT",         "OSC 2",        "CHARACTER", "MOD ENV",      "TRACKING"},
                      *fx[5] = {"DISTORTION", "DELAY", "DELAY CONFIG", "REVERB", "RETURNS"},
                      *smp[4] = {"SAMPLE MAIN", "SAMPLE PITCH", "TRIM / LOOP", "ENVELOPE"};
    switch (ui->page) {
    case MELLO_UI_SEQ_STEP: return step[ui->step_subpage % 4u];
    case MELLO_UI_SEQ_LOOP: return loop[ui->loop_subpage % 3u];
    case MELLO_UI_SLOTS: return slots[ui->slots_subpage % 2u];
    case MELLO_UI_RANDOM: return ui->random_edit ? "EDIT" : (ui->random_links ? "LINKS" : "MASK");
    case MELLO_UI_DOT:
        return ui->dot_shape && dot_lane(ui->selected_dot_target) >= 0
                   ? dsh[dot_lane(ui->selected_dot_target)]
                   : dot[ui->selected_dot_target % MELLO_DOT_MATRIX_COUNT];
    case MELLO_UI_TURING: return tm[ui->tm_subpage % 4u];
    case MELLO_UI_SUB: return sub[ui->sub_subpage % 5u];
    case MELLO_UI_GLOBAL: return glo[ui->global_subpage % 2u];
    case MELLO_UI_SOUND_RESERVED: return ui->sound_subpage == 2u ? 0 : snd[ui->sound_subpage % 10u];
    case MELLO_UI_AUDIO_FX: return fx[ui->fx_subpage % 5u];
    case MELLO_UI_SAMPLE_RESERVED: return smp[ui->sample_subpage % 4u];
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: {
        static const char *chd[CHORD_LAYERS] = {"PLAY",    "CHORD", "VOICING", "GRID", "PATTERN",
                                                "TONE",    "FILTER", "MOTION", "AMP"};
        return chd[ui->chord_subpage % CHORD_LAYERS];
    }
#endif
    case MELLO_UI_DRUMS: {
        static const char *drm[DRUM_LAYERS] = {"VOICE", "STEP", "VARY", "KIT"};
        return drm[ui->drum_subpage % DRUM_LAYERS];
    }
    case MELLO_UI_HOME: return ui->held_step == MELLO_UI_NO_STEP ? 0 : "HELD STEP";
    default: return 0;
    }
}

/* The footer's short labels in full, for the readout's large line (13 characters fit). */
static const char *full_label(const char *l)
{
    static const char *map[][2] = {
        {"CUT", "CUTOFF"},        {"RES", "RESONANCE"},    {"ATK", "ATTACK"},
        {"DEC", "DECAY"},         {"SUS", "SUSTAIN"},      {"REL", "RELEASE"},
        {"PWM", "PULSE WIDTH"},   {"FDBK", "FEEDBACK"},    {"AMT", "AMOUNT"},
        {"CHAR", "CHARACTER"},    {"RATCH", "RATCHET"},    {"OCT", "OCTAVE"},
        {"PRE", "PRE-DELAY"},     {"DIV", "DIVISION"},     {"D.LEVEL", "DIST LEVEL"},
        {"SUBOSC", "SUB OSC"},    {"PROB", "PROBABILITY"}, {"LEN", "LENGTH"},
        {"DEP", "DEPTH"},         {"ACC", "ACCENT"},       {"V.CUT", "VEL > CUTOFF"},
        {"V.LVL", "VEL > LEVEL"}, {"KEYTRK", "KEY TRACK"}, {"V.ENV", "VEL > MOD ENV"},
        {"N.FM", "NOISE FM"},     {"SAT", "SATURATION"},   {"FINE", "FINE TUNE"},
        {"SEMI", "SEMITONES"}};
    uint8_t i;
    for (i = 0u; i < sizeof map / sizeof map[0]; ++i) {
        const char *a = l, *b = map[i][0]; /* no strcmp on the target's libc */
        while (*a && *a == *b) ++a, ++b;
        if (*a == *b) return map[i][1];
    }
    return l;
}

/* What you are turning, large over the body: where you are (page, layer), the control's name and its value. The
 * same texts as the footer (footer_texts), so the two never disagree. */
static void draw_readout(MelloUi *ui, const MelloUiDrawOps *o)
{
    char v[4][24], where[40], *p = where;
    const char *l[4], *layer = layer_name(ui);
    uint16_t accent = ui_page_accent(ui);
    footer_texts(ui, l, v);
    p = copy_s(p, domain_name(ui));
    if (layer) {
        p = copy_s(p, " / ");
        (void)copy_s(p, layer);
    }
    o->fill(o->ctx, 0, 30, 240, 142,
            UI_BLACK); /* in place of the body, not over it: no overdraw, no flicker */
    ui_round_fill(o, 6, 40, 228, 124, UI_PANEL);
    ui_round_frame(o, 6, 40, 228, 124, ui_mix(accent, UI_BLACK, 90));
    ui_spaced(o, 12, 46, 216, where, UI_DIM, 1);
    if (ui->readout >= MELLO_UI_READOUT_TEMPO) {
        MelloEngine *e = eng(ui);
        char val[24], *q = val;
        const char *label = "TEMPO";
        val[0] = 0;
        if (ui->readout == MELLO_UI_READOUT_TEMPO && e) {
            q = put_u32(val, e->project.bpm);
            (void)copy_s(q, " BPM");
        } else if (ui->readout == MELLO_UI_READOUT_INTENSITY) {
            label = "RANDOM";
            (void)copy_s(val, random_intensity_name(ui->random_intensity));
        } else if (e) {
            label = "PATTERN";
            q = copy_s(val, "SLOT ");
            q = put_u32(q, (uint32_t)ui->selected_slot + 1u);
            if (!mello_engine_slot_valid(e, ui->selected_slot)) (void)copy_s(val, "NO SLOTS");
        }
        o->text(o->ctx, 12, 68, 216, 1, label, accent, 1);
        o->text(o->ctx, 12, 102, 216, 1, val, UI_BRIGHT, 1);
    } else if (ui->readout == MELLO_UI_READOUT_LAYER) {
        o->text(o->ctx, 12, 80, 216, 1, layer ? layer : domain_name(ui), accent, 1);
        ui_spaced(o, 12, 124, 216, "SELECT: LAYER", UI_DIM, 1);
    } else {
        uint8_t k = (uint8_t)(ui->readout - 1u);
        uint16_t nv, nm;
        accent = knob_colour(ui, k);
        o->text(o->ctx, 12, 68, 216, 1, full_label(l[k]), accent, 1);
        o->text(o->ctx, 12, 102, 216, 1, v[k][0] ? v[k] : "-", UI_BRIGHT, 1);
        if (knob_norm(ui, k, &nv, &nm)) ui_slider(o, 20, 146, 200, nv, nm, accent); /* the HTML's slider */
    }
}

void mello_ui_render(MelloUi *ui, const MelloUiDrawOps *ops)
{
    uint16_t d;
    MelloUiFitCtx fit_ctx;
    MelloUiDrawOps fitted;
    if (!ui || !ops || !ops->fill || !ops->text) return;
    fit_ctx.raw = ops;
    fit_ctx.fn = 0u;
    fit_ctx.fhead = 0u;
    fitted.fill = fit_fill;
    fitted.text = fit_text;
    fitted.key_led = 0;
    fitted.button_led = 0;
    fitted.ctx = &fit_ctx;
    fitted.text_on = 0;
    d = ui->dirty;
    if (d & MELLO_UI_DIRTY_FULL) {
        fitted.fill(fitted.ctx, 0, 0, 240, 240, UI_BLACK);
        d |= MELLO_UI_DIRTY_HEADER | MELLO_UI_DIRTY_BODY | MELLO_UI_DIRTY_FOOTER | MELLO_UI_DIRTY_STATUS;
    }
    if (d & MELLO_UI_DIRTY_HEADER) draw_header(ui, &fitted);
    if (ui->readout) {
        /* The readout takes the body's place; the partial playhead redraws wait for the full redraw when it
         * goes. */
        if (d & MELLO_UI_DIRTY_BODY) draw_readout(ui, &fitted);
    } else if (d & MELLO_UI_DIRTY_BODY)
        draw_body(ui, &fitted);
    else if ((d & MELLO_UI_DIRTY_HOME_PLAYHEAD) && ui->page == MELLO_UI_HOME)
        draw_home_playhead(ui, &fitted);
    else if ((d & MELLO_UI_DIRTY_SEQ_PLAYHEAD) &&
             (ui->page == MELLO_UI_SEQ_STEP || ui->page == MELLO_UI_RANDOM))
        draw_sequence_playhead(ui, &fitted);
    if (d & MELLO_UI_DIRTY_FOOTER) draw_footer(ui, &fitted);
    if (d & MELLO_UI_DIRTY_STATUS) draw_status(ui, &fitted);
    ui->dirty &= MELLO_UI_DIRTY_LEDS;
}

static uint32_t calc_key_leds(MelloUi *ui)
{
    MelloEngine *e = eng(ui);
    uint32_t mask = 0u;
    uint8_t i;
    if (!e) return 0u;
    if (ui->page == MELLO_UI_HOME && ui->keys_mode) {
        /* KEYS: the root lit on the keyboard (F3 = MIDI 53 .. G5), in every octave the keys reach. */
        for (i = 0u; i < 27u; ++i)
            if ((53u + i) % 12u == e->project.root_note % 12u) mask |= bit_u32(i);
    } else if (ui->page == MELLO_UI_HOME) {
        /* The steps of this bank: lit = on; the playhead flips its key, so it runs across them. */
        uint8_t base = (uint8_t)(ui->step_bank * 16u), at = ui->transport->display_step;
        for (i = 0u; i < 16u; ++i)
            if (base + i < e->project.seq_len && !e->project.steps[base + i].skip)
                mask |= bit_u32(white_key_notes[i]);
        if (ui->transport->running && at >= base && at < base + 16u)
            mask ^= bit_u32(white_key_notes[at - base]);
    } else if (ui->page == MELLO_UI_SEQ_STEP) {
        uint8_t p = (uint8_t)(ui->selected_step % 16u);
        if (ui->selected_step / 16u == ui->step_bank) mask |= bit_u32(white_key_notes[p]);
    } else if (ui->page == MELLO_UI_SEQ_SEGMENT) {
        /* where each segment starts, lit; the playhead flips its key */
        uint8_t base = (uint8_t)(ui->step_bank * 16u), at = ui->transport->display_step, b[MELLO_MAX_SEGMENTS + 1u],
                sc = mello_segment_bounds(&e->project, b);
        for (i = 0u; i < sc; ++i)
            if (b[i] >= base && b[i] < base + 16u) mask |= bit_u32(white_key_notes[b[i] - base]);
        if (ui->transport->running && at >= base && at < base + 16u)
            mask ^= bit_u32(white_key_notes[at - base]);
    } else if (ui->page == MELLO_UI_SEQ_LOOP) {
        mask |= bit_u32(white_key_notes[ui->selected_loop_step]);
    } else if (ui->page == MELLO_UI_SEQ_SLOTSEQ) {
        mask |= bit_u32(white_key_notes[ui->selected_slot_seq_step % 16u]);
    } else if (ui->page == MELLO_UI_RANDOM) {
        for (i = 0u; i < random_target_count(); ++i)
            if (e->randomize.mask & random_target_bit(i)) mask |= bit_u32(white_key_notes[i]);
    } else if (ui->page == MELLO_UI_SLOTS) {
        uint8_t base = (uint8_t)(ui->slot_bank * 16u);
        for (i = 0u; i < 16u; ++i)
            if (base + i < MELLO_MAX_SLOTS && e->slots[base + i].valid) mask |= bit_u32(white_key_notes[i]);
        mask |= bit_u32(white_key_notes[ui->selected_slot % 16u]);
    } else if (ui->page == MELLO_UI_DOT) {
        uint16_t bits = e->dot[ui->selected_dot_target].cells;
        int8_t at = e->dot_active_cell[ui->selected_dot_target];
        for (i = 0u; i < 16u; ++i)
            if (bits & (uint16_t)(1u << i)) mask |= bit_u32(white_key_notes[i]);
        if (at >= 0 && at < 16)
            mask ^= bit_u32(white_key_notes[at]); /* the read position: inverted, so it shows */
    } else if (ui->page == MELLO_UI_TURING) {
        uint8_t base = (uint8_t)(ui->tm_bank * 16u);
        for (i = 0u; i < 16u; ++i)
            if (e->tm_bool_reg[base + i]) mask |= bit_u32(white_key_notes[i]);
    } else if (ui->page == MELLO_UI_SUB) {
        for (i = 0u; i < MELLO_SUB_X_MAX_STEPS; ++i)
            if (e->sub.x_steps[i].on) mask |= bit_u32(white_key_notes[i]);
        mask |= bit_u32(white_key_notes[ui->selected_sub_x_step]);
#if MELLO_FEATURE_CHORD_MACHINE
    } else if (ui->page == MELLO_UI_CHORD) {
        if (ui->chord_subpage == 3u) {
            uint16_t bits = e->chord.grid_cells;
            for (i = 0u; i < 16u; ++i)
                if (bits & (uint16_t)(1u << i)) mask |= bit_u32(white_key_notes[i]);
        } else if (ui->chord_subpage == 4u) {
            if (ui->selected_chord_pattern < 16u)
                mask |= bit_u32(white_key_notes[ui->selected_chord_pattern]);
        } else {
            for (i = 0u; i < e->chord.length && i < 16u; ++i) mask |= bit_u32(white_key_notes[i]);
            if (ui->selected_chord_step < 16u) mask |= bit_u32(white_key_notes[ui->selected_chord_step]);
        }
#endif
    } else if (ui->page == MELLO_UI_DRUMS) {
        /* the picked voice's hits lit; the drums' playhead flips its key */
        uint16_t h = e->drums.hits[ui->selected_drum_voice % MELLO_DRUM_VOICES];
        for (i = 0u; i < e->drums.length; ++i)
            if (h & (1u << i)) mask |= bit_u32(white_key_notes[i]);
        if (ui->transport->running && e->drums.on && e->drum_step >= 0)
            mask ^= bit_u32(white_key_notes[(uint8_t)e->drum_step]);
    } else if (ui->page == MELLO_UI_SAMPLE_RESERVED) {
        for (i = 0u; i < MELLO_SAMPLE_SLOT_COUNT; ++i)
            if (ui->sample_bank && ui->sample_bank->meta[i].loaded) mask |= bit_u32(white_key_notes[i]);
        if (e->sample.edit_slot < MELLO_SAMPLE_SLOT_COUNT)
            mask |= bit_u32(white_key_notes[e->sample.edit_slot]);
    }
    return mask;
}

static uint32_t calc_button_leds(MelloUi *ui)
{
    uint8_t pb = page_button(ui);
    uint32_t m = pb <= MELLO_UI_BTN_REC ? bit_u32(pb) : 0u;
    MelloEngine *e = eng(ui);
    if (ui->transport && ui->transport->running) m |= bit_u32(MELLO_UI_BTN_PLAY);
    if (ui->page == MELLO_UI_HOME && ui->keys_mode) m |= bit_u32(MELLO_UI_BTN_EDIT);
    if (ui->page == MELLO_UI_RANDOM && ui->random_edit) m |= bit_u32(MELLO_UI_BTN_EDIT);
    if (ui->page == MELLO_UI_HOME && ui->keys_rec) m |= bit_u32(MELLO_UI_BTN_REC);
    if (ui->page == MELLO_UI_SUB && e && e->sub.on) m |= bit_u32(MELLO_UI_BTN_ARP);
#if MELLO_FEATURE_CHORD_MACHINE
    if (ui->page == MELLO_UI_CHORD && e && e->chord.on) m |= bit_u32(MELLO_UI_BTN_ARP);
#endif
    return m;
}

void mello_ui_sync_leds(MelloUi *ui, const MelloUiDrawOps *ops)
{
    uint32_t km, bm, ch;
    uint8_t i;
    if (!ui || !ops) return;
    km = calc_key_leds(ui);
    bm = calc_button_leds(ui);
    if (ops->key_led) {
        ch = km ^ ui->last_key_led_mask;
        for (i = 0u; i < 27u; ++i)
            if (ch & bit_u32(i)) ops->key_led(ops->ctx, i, (km & bit_u32(i)) != 0u);
    }
    if (ops->button_led) {
        ch = bm ^ ui->last_button_led_mask;
        for (i = 0u; i < MELLO_UI_BUTTON_COUNT; ++i)
            if (ch & bit_u32(i)) ops->button_led(ops->ctx, i, (bm & bit_u32(i)) != 0u);
    }
    ui->last_key_led_mask = km;
    ui->last_button_led_mask = bm;
    ui->dirty &= (uint16_t)~MELLO_UI_DIRTY_LEDS;
}

uint16_t mello_ui_dirty(const MelloUi *ui)
{
    return ui ? ui->dirty : 0u;
}
