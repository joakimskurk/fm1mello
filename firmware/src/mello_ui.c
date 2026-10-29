#include "mello_ui.h"

#define UI_RGB(r, g, b)                                                                                      \
    ((uint16_t)((((uint16_t)(r) & 0xF8u) << 8) | (((uint16_t)(g) & 0xFCu) << 3) | ((uint16_t)(b) >> 3)))
#define UI_BLACK   UI_RGB(4, 14, 8)   /* the ground: the HTML's #040E08, not black */
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
    if (page > MELLO_UI_SAMPLE_RESERVED) page = MELLO_UI_HOME;
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
    case MELLO_UI_DOT: ui->selected_dot_target = 0u; break;
    case MELLO_UI_TURING: ui->tm_subpage = 0u; break;
    case MELLO_UI_SUB: ui->sub_subpage = 0u; break;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: ui->chord_subpage = 0u; break;
#endif
    case MELLO_UI_GLOBAL: ui->global_subpage = 0u; break;
    case MELLO_UI_SOUND_RESERVED: ui->sound_subpage = 0u; break;
    case MELLO_UI_AUDIO_FX: ui->fx_subpage = 0u; break;
    case MELLO_UI_SAMPLE_RESERVED: ui->sample_subpage = 0u; break;
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
    default: return "?";
    }
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
        uint8_t i, wanted = (uint8_t)(idx + 1u);
        uint8_t count = (uint8_t)(sizeof(sequence_lengths) / sizeof(sequence_lengths[0]));
        for (i = 0u; i < count && sequence_lengths[i] < wanted; ++i) {
        }
        e->project.seq_len = i < count ? sequence_lengths[i] : sequence_lengths[count - 1u];
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
    MELLO_CTX_TURING_TOGGLE,
    MELLO_CTX_TURING_REROLL,
    MELLO_CTX_SUB_TOGGLE,
    MELLO_CTX_SUB_STEP,
    MELLO_CTX_CHORD_TOGGLE,
    MELLO_CTX_GLOBAL_TAP,
    MELLO_CTX_GLOBAL_RESET,
    MELLO_CTX_SAMPLE_TOGGLE,
    MELLO_CTX_HOME_RANDOM, /* reroll now, at the intensity ALGORITHM sets */
    MELLO_CTX_UNDO,        /* the pattern before the last RANDOM/CLEAR; again = redo */
    MELLO_CTX_MUTATE,      /* a LOW reroll of the pitches only */
    MELLO_CTX_TRANSPOSE_DOWN,
    MELLO_CTX_TRANSPOSE_UP
} MelloContextVerb;

typedef struct {
    MelloContextVerb verb;
    const char *label;
} MelloContextAction;

/* WP-C5 authoritative contextual-action mapping. The physical B0-B7 slot,
 * rendered label and handler verb are defined together here. */
static const MelloContextAction context_actions[MELLO_UI_SAMPLE_RESERVED + 1u][8] = {
    [MELLO_UI_HOME] = {[0] = {MELLO_CTX_HOME_RANDOM, "RND"},
                       [1] = {MELLO_CTX_UNDO, "UNDO"},
                       [2] = {MELLO_CTX_MUTATE, "MUT"},
                       [3] = {MELLO_CTX_TRANSPOSE_DOWN, "-1"},
                       [4] = {MELLO_CTX_TRANSPOSE_UP, "+1"},
                       [5] = {MELLO_CTX_OPEN_RANDOM, "MASK"},
                       [7] = {MELLO_CTX_RANDOM_CLEAR, "CLR2"}},
    [MELLO_UI_SEQ_STEP] = {[4] = {MELLO_CTX_STEP_SKIP, "SKIP"}, [5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_SEQ_SEGMENT] = {[5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_SEQ_LOOP] = {[2] = {MELLO_CTX_LOOP_RESET, "RST"}, [5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_SEQ_SLOTSEQ] = {[5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_RANDOM] = {[1] = {MELLO_CTX_RANDOM_REROLL, "RND"},
                         [2] = {MELLO_CTX_UNDO, "UNDO"},
                         [3] = {MELLO_CTX_RANDOM_CLEAR, "CLR2"},
                         [6] = {MELLO_CTX_RANDOM_NONE, "NONE"},
                         [7] = {MELLO_CTX_RANDOM_ALL, "ALL"}},
    [MELLO_UI_KEY_SCALE] = {[5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_SLOTS] = {[5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_DOT] = {[1] = {MELLO_CTX_DOT_REROLL, "RND"},
                      [2] = {MELLO_CTX_DOT_RESET, "RST"},
                      [5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_TURING] = {[0] = {MELLO_CTX_TURING_TOGGLE, "ON"},
                         [1] = {MELLO_CTX_TURING_REROLL, "RND"},
                         [5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_SUB] = {[0] = {MELLO_CTX_SUB_TOGGLE, "ON"},
                      [4] = {MELLO_CTX_SUB_STEP, "STEP"},
                      [5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
#if MELLO_FEATURE_CHORD_MACHINE
    [MELLO_UI_CHORD] = {[0] = {MELLO_CTX_CHORD_TOGGLE, "ON"}, [5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
#endif
    [MELLO_UI_GLOBAL] = {[1] = {MELLO_CTX_GLOBAL_TAP, "TAP"},
                         [3] = {MELLO_CTX_GLOBAL_RESET, "RST2"},
                         [5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_SOUND_RESERVED] = {[5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_AUDIO_FX] = {[5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_SAMPLE_RESERVED] = {[0] = {MELLO_CTX_SAMPLE_TOGGLE, "ON"},
                                  [5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}}};

static const MelloContextAction *context_action(MelloUiPage page, uint8_t slot)
{
    if ((uint32_t)page > (uint32_t)MELLO_UI_SAMPLE_RESERVED || slot >= 8u) return 0;
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

/* The step pitch (scale degree, octave 0) nearest to a MIDI note in the current key: step-record's inverse of
 * mello_resolve_step_midi. */
static int8_t degree_for_midi(MelloEngine *e, uint8_t midi)
{
    MelloStep probe;
    int32_t best = 0, best_d = 1000;
    uint8_t maxp = (uint8_t)(e->project.pitch_span_oct * scale_len(e->project.scale)), d;
    memset(&probe, 0, sizeof probe);
    for (d = 0u; d < maxp; ++d) {
        int32_t diff;
        probe.pitch = (int8_t)d;
        diff = (int32_t)mello_resolve_step_midi(&e->project, &probe) - (int32_t)midi;
        if (diff < 0) diff = -diff;
        if (diff < best_d) best_d = diff, best = d;
    }
    return (int8_t)best;
}

/* A key in KEYS mode. Note keys run F3 (MIDI 53) .. G5. */
static void keys_note(MelloUi *ui, uint8_t note_index, uint32_t now_us)
{
    MelloEngine *e = eng(ui);
    uint8_t midi = (uint8_t)(53u + note_index);
    if (!e) return;
    if (ui->transport->running && !ui->keys_rec) {
        e->project.root_note = midi; /* the key you press is the new root: the sequence follows at its next step */
        rebuild(ui);
        mark_edit(ui);
        ui->dirty |= MELLO_UI_DIRTY_HEADER;
        return;
    }
    if (ui->keys_rec && ui->keys_rec_pos < e->project.seq_len) {
        MelloStep *s = &e->project.steps[ui->keys_rec_pos];
        s->pitch = degree_for_midi(e, midi);
        s->octave = 0;
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
            handle_context_action(ui, now_ms, (uint8_t)b);
            return;
        }
    }

    w = mello_ui_white_key_position(note_index);
    if (w < 0) return;

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
    case MELLO_UI_SEQ_SEGMENT:
        if ((uint8_t)w < MELLO_MAX_SEGMENTS) {
            ui->selected_segment = (uint8_t)w;
            if (ui->selected_segment >= e->project.seg_count)
                ui->selected_segment = (uint8_t)(e->project.seg_count - 1u);
            mark_edit(ui);
        }
        break;
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
        if ((uint8_t)w < random_target_count()) {
            e->randomize.mask ^= random_target_bit((uint8_t)w);
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
            uint8_t i;
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

static void adjust_segment(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    uint8_t s;
    if (!e || !delta) return;
    s = ui->selected_segment;
    if (s >= e->project.seg_count) s = (uint8_t)(e->project.seg_count - 1u);
    ui->selected_segment = s;
    if (ui->segment_subpage == 0u) {
        if (knob == 0u)
            e->project.seg_repeats[s] = (uint8_t)clamp_i32((int32_t)e->project.seg_repeats[s] + delta, 1, 4);
        else if (knob == 1u)
            e->project.seg_direction[s] =
                e->project.seg_direction[s] == MELLO_SEG_FWD ? MELLO_SEG_REV : MELLO_SEG_FWD;
        else if (knob == 2u)
            e->project.seg_mode[s] =
                (MelloSegmentMode)wrap_u8((int32_t)e->project.seg_mode[s] + (delta > 0 ? 1 : -1), 3u);
        else if (knob == 3u)
            e->project.seg_skip[s] = delta > 0;
    } else {
        if (knob == 0u) {
            e->project.seg_count = (uint8_t)clamp_i32((int32_t)e->project.seg_count + delta, 1, 6);
            if (ui->selected_segment >= e->project.seg_count)
                ui->selected_segment = (uint8_t)(e->project.seg_count - 1u);
        } else if (knob == 1u && s > 0u) {
            uint8_t i = (uint8_t)(s - 1u);
            int32_t lo = (i == 0u) ? 1 : (int32_t)e->project.seg_splits[i - 1u] + 1;
            int32_t hi = (int32_t)e->project.seq_len - ((int32_t)e->project.seg_count - 1 - i);
            e->project.seg_splits[i] = (uint8_t)clamp_i32((int32_t)e->project.seg_splits[i] + delta, lo, hi);
        } else if (knob == 2u) {
            e->project.seg_repeats[s] = (uint8_t)clamp_i32((int32_t)e->project.seg_repeats[s] + delta, 1, 4);
        } else if (knob == 3u)
            e->project.seg_skip[s] = delta > 0;
    }
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
    if (knob == 0u)
        c.mode = (MelloDotMode)wrap_u8((int32_t)c.mode + (delta > 0 ? 1 : -1), 4u);
    else if (knob == 1u)
        c.length = (uint8_t)clamp_i32((int32_t)c.length + delta, 1, 16);
    else if (knob == 2u)
        c.amount = (uint16_t)clamp_i32((int32_t)c.amount + delta * 10, 0, 1000);
    else if (knob == 3u)
        c.follow = delta > 0;
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
            c.mode = (MelloSubMode)wrap_u8((int32_t)c.mode + (delta > 0 ? 1 : -1), 4u);
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
static void adjust_chord(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloChordConfig c;
    MelloChordStep *s;
    if (!e || !delta) return;
    c = e->chord;
    if (ui->chord_subpage == 0u) {
        if (knob == 0u)
            c.mode = (MelloChordMode)wrap_u8((int32_t)c.mode + (delta > 0 ? 1 : -1), MELLO_CHORD_MODE_COUNT);
        else if (knob == 1u)
            c.trigger = (MelloChordTrigger)wrap_u8((int32_t)c.trigger + (delta > 0 ? 1 : -1), 2u);
        else if (knob == 2u)
            c.length = (uint8_t)clamp_i32((int32_t)c.length + delta, 1, MELLO_CHORD_MAX_STEPS);
        else
            c.level = (uint16_t)clamp_i32((int32_t)c.level + delta * 10, 0, 1000);
    } else if (ui->chord_subpage == 1u) {
        s = &c.steps[ui->selected_chord_step % c.length];
        if (knob == 0u)
            s->degree = (uint8_t)clamp_i32((int32_t)s->degree + delta, 0, 11);
        else if (knob == 1u)
            s->shape =
                (MelloChordShape)wrap_u8((int32_t)s->shape + (delta > 0 ? 1 : -1), MELLO_CHORD_SHAPE_COUNT);
        else if (knob == 2u)
            s->hold = (uint8_t)clamp_i32((int32_t)s->hold + delta, 1, 16);
        else
            s->octave = (int8_t)clamp_i32((int32_t)s->octave + delta, -2, 2);
    } else if (ui->chord_subpage == 2u) {
        s = &c.steps[ui->selected_chord_step % c.length];
        if (knob == 0u)
            s->inversion = (uint8_t)clamp_i32((int32_t)s->inversion + delta, 0, 3);
        else if (knob == 1u)
            s->spread = (uint8_t)clamp_i32((int32_t)s->spread + delta, 0, 3);
        else if (knob == 2u)
            c.wave = step_sub_waveform(c.wave, delta);
        else
            c.gate_amount = (uint16_t)clamp_i32((int32_t)c.gate_amount + delta * 10, 100, 1200);
    } else if (ui->chord_subpage == 3u) {
        if (knob == 0u)
            c.grid_mode = (MelloDotMode)wrap_u8((int32_t)c.grid_mode + (delta > 0 ? 1 : -1), 4u);
        else if (knob == 1u)
            c.grid_length = (uint8_t)clamp_i32((int32_t)c.grid_length + delta, 1, 16);
        else if (knob == 2u)
            c.grid_amount = (uint16_t)clamp_i32((int32_t)c.grid_amount + delta * 10, 0, 1000);
        else
            c.grid_follow = delta > 0;
    } else {
        uint8_t p = ui->selected_chord_pattern % c.pattern_length;
        if (knob == 0u)
            c.pattern_masks[p] = (uint8_t)clamp_i32((int32_t)c.pattern_masks[p] + delta, 0, 15);
        else if (knob == 1u)
            c.pattern_length =
                (uint8_t)clamp_i32((int32_t)c.pattern_length + delta, 1, MELLO_CHORD_PATTERN_STEPS);
        else if (knob == 2u)
            c.voices = (uint8_t)clamp_i32((int32_t)c.voices + delta, 2, 4);
        else
            c.release_ms = (uint16_t)clamp_i32((int32_t)c.release_ms + delta * 10, 5, 4000);
    }
    mello_engine_chord_set_config(e, &c);
    mark_edit(ui);
}
#endif

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
        else if (knob == 1u)
        { /* ~6% a detent: 0.008-20 Hz is a log range, as the HTML's slider (lfoRateToSlider) */
            int32_t r = c.lfo_rate_millihz, st = r / 16 > 1 ? r / 16 : 1;
            c.lfo_rate_millihz = (uint16_t)clamp_i32(r + delta * st, 8, 20000);
        }
        else if (knob == 2u)
            c.lfo_depth = (uint16_t)clamp_i32((int32_t)c.lfo_depth + delta * 10, 0, 1000);
        else
            c.lfo_target = (MelloLfoTarget)wrap_u8((int32_t)c.lfo_target + (delta > 0 ? 1 : -1), 4u);
    } else if (ui->sound_subpage == 3u) {
        if (knob == 0u)
            c.follower_sub_on = delta > 0;
        else if (knob == 1u)
            c.follower_sub_level = (uint16_t)clamp_i32((int32_t)c.follower_sub_level + delta * 10, 0, 1000);
        else if (knob == 2u)
            c.follower_sub_octave = (int8_t)clamp_i32((int32_t)c.follower_sub_octave + delta, -2, 0);
        else
            c.follower_sub_wave = step_sub_waveform(c.follower_sub_wave, delta);
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
            c.delay_tone = (uint16_t)clamp_i32((int32_t)c.delay_tone + delta * 10, 0, 1000);
        else
            c.delay_level = (uint16_t)clamp_i32((int32_t)c.delay_level + delta * 10, 0, 1500);
    } else if (ui->fx_subpage == 2u) {
        if (knob == 0u)
            c.delay_sync = delta > 0;
        else if (knob == 1u)
            c.delay_div = wrap_u8((int32_t)c.delay_div + (delta > 0 ? 1 : -1), 4u);
        else if (knob == 2u)
            c.delay_time_ms = (uint16_t)clamp_i32((int32_t)c.delay_time_ms + delta * 10, 30, 1500);
        else
            c.dist_level = (uint16_t)clamp_i32((int32_t)c.dist_level + delta * 10, 200, 1400);
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
    case MELLO_UI_SEQ_SEGMENT:
        ui->segment_subpage = wrap_u8((int32_t)ui->segment_subpage + (delta > 0 ? 1 : -1), 2u);
        break;
    case MELLO_UI_SEQ_LOOP:
        ui->loop_subpage = wrap_u8((int32_t)ui->loop_subpage + (delta > 0 ? 1 : -1), 3u);
        break;
    case MELLO_UI_SLOTS:
        ui->slots_subpage = wrap_u8((int32_t)ui->slots_subpage + (delta > 0 ? 1 : -1), 2u);
        break;
    case MELLO_UI_DOT:
        ui->selected_dot_target = wrap_u8((int32_t)ui->selected_dot_target + (delta > 0 ? 1 : -1), 4u);
        break;
    case MELLO_UI_TURING: ui->tm_subpage = wrap_u8((int32_t)ui->tm_subpage + (delta > 0 ? 1 : -1), 3u); break;
    case MELLO_UI_SUB: ui->sub_subpage = wrap_u8((int32_t)ui->sub_subpage + (delta > 0 ? 1 : -1), 5u); break;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD:
        ui->chord_subpage = wrap_u8((int32_t)ui->chord_subpage + (delta > 0 ? 1 : -1), 5u);
        break;
#endif
    case MELLO_UI_GLOBAL:
        ui->global_subpage = wrap_u8((int32_t)ui->global_subpage + (delta > 0 ? 1 : -1), 2u);
        break;
    case MELLO_UI_SOUND_RESERVED:
        /* OSC, ENV, FOLLOWER, LEVEL: the LFO (subpage 2) is MOD's, reached with the LFO button. */
        if (ui->sound_subpage == 2u) return;
        do
            ui->sound_subpage = wrap_u8((int32_t)ui->sound_subpage + (delta > 0 ? 1 : -1), 5u);
        while (ui->sound_subpage == 2u);
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
    ui->random_intensity = (MelloRandomizeIntensity)clamp_i32(
        (int32_t)ui->random_intensity + (delta > 0 ? 1 : -1), MELLO_RANDOMIZE_LOW, MELLO_RANDOMIZE_HIGH);
    mark_edit(ui);
}

static const char *layer_name(const MelloUi *ui);
static const char *domain_name(const MelloUi *ui);
static uint8_t save_state(const MelloUi *ui);
static void draw_param_panel(MelloUi *ui, const MelloUiDrawOps *o);
static const char *full_label(const char *l);
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
        int16_t ds = (int16_t)(enc[MELLO_UI_ENC_SELECT]), da = enc[MELLO_UI_ENC_ALGORITHM], dp = enc[MELLO_UI_ENC_PRESETS];
        if (ds && e) {
            mello_transport_set_bpm(ui->transport, (uint16_t)clamp_i32((int32_t)e->project.bpm + ds, 20, 250));
            mark_edit(ui);
            ui->dirty |= MELLO_UI_DIRTY_HEADER; /* the header shows the tempo */
            show_readout(ui, MELLO_UI_READOUT_TEMPO, now_ms);
        }
        if (da) {
            adjust_random(ui, 0u, da);
            show_readout(ui, MELLO_UI_READOUT_INTENSITY, now_ms);
        }
        if (dp && e) {
            int32_t i = ui->selected_slot, n = e->visible_slot_count < MELLO_MAX_SLOTS ? e->visible_slot_count
                                                                                       : MELLO_MAX_SLOTS;
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
    if (pressed & bit_u32(MELLO_UI_BTN_ARP)) { /* BASS: SUB -> CHORD -> SUB */
#if MELLO_FEATURE_CHORD_MACHINE
        open_page(ui, ui->page == MELLO_UI_SUB ? MELLO_UI_CHORD : MELLO_UI_SUB);
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
    if (pressed & bit_u32(MELLO_UI_BTN_ENV)) { /* SOUND: OSC -> ENV -> FOLLOWER -> LEVEL (the LFO is MOD's) */
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
    for (i = 0u; i < 27u; ++i)
        if (note_pressed & bit_u32(i)) handle_note_press(ui, now_us, now_ms, held_buttons, i);
    if (encoder_delta) adjust_knobs(ui, encoder_delta, now_us, now_ms);
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
#endif
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
        ui->seen_chord_pos = e->chord_chain_pos;
        ui->seen_chord_grid = e->chord_grid_active_cell;
#endif
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
    return m == MELLO_DOT_VERTICAL ? "VERT"
           : m == MELLO_DOT_SNAKE  ? "SNAKE"
           : m == MELLO_DOT_RANDOM ? "RANDOM"
                                   : "HORIZ";
}

static const char *sub_mode_name(MelloSubMode m)
{
    return m == MELLO_SUB_MODE_EUCLID   ? "EUCLID"
           : m == MELLO_SUB_MODE_MATRIX ? "MATRIX"
           : m == MELLO_SUB_MODE_TM     ? "TM"
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
static void ui_slider(const MelloUiDrawOps *o, int16_t x, int16_t y, int16_t w, uint16_t value, uint16_t max_value,
                      uint16_t colour)
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
    static const uint16_t c[4] = {UI_CORAL, UI_CYAN, UI_MAGENTA, UI_GOLD};
    return c[target < 4u ? target : 0u];
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
        return ui->sound_subpage == 0u
                   ? UI_CYAN
                   : (ui->sound_subpage == 2u ? UI_PURPLE : (ui->sound_subpage == 3u ? UI_GOLD : UI_ACC));
    case MELLO_UI_SAMPLE_RESERVED: return UI_BLUE;
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
    o->fill(o->ctx, 8, 129, 224, 41, UI_BLACK);
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
        if (h >= 30 && w >= 10) { /* the letter's band solid, so it sits on its own colour; the gradient below */
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
                                                    "PENTA", "mPENT", "DIM",   "WHOLE", "AUG",    "MAJOR"};
        q = copy_s(key, n[e->project.root_note % 12u]);
        q = copy_s(q, " ");
        (void)copy_s(q, sc[e->project.scale < MELLO_SCALE_COUNT ? e->project.scale : MELLO_SCALE_MINOR]);
        o->text(o->ctx, 4, 7, 84, 0, key, UI_GOLD, 0);
    }
    p = copy_s(b, ui->page == MELLO_UI_HOME && ui->keys_mode ? (ui->keys_rec ? "HOME REC" : "HOME KEYS")
                                                              : domain_name(ui));
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

static void draw_body(MelloUi *ui, const MelloUiDrawOps *o)
{
    MelloEngine *e = eng(ui);
    char a[48], b[48];
    if (!e) return;
    o->fill(o->ctx, 0, 30, 240, 142, UI_BLACK);
    switch (ui->page) {
    case MELLO_UI_HOME: draw_home(ui, o); break;
    case MELLO_UI_SEQ_STEP: {
        MelloStep *s = &e->project.steps[ui->selected_step];
        uint8_t i, vis = ui_step_overview_count(&e->project);
        uint8_t midi = mello_resolve_step_midi(&e->project, s);
        int16_t x;
        /* Full-sequence strip: always show every main step; OCT only changes which 16 keys edit. */
        for (i = 0u; i < vis; ++i) draw_step_overview_cell(ui, o, i, 0u);
        {
            uint8_t bs = (uint8_t)(ui->step_bank * 16u), be = (uint8_t)(bs + 16u);
            int16_t bx, bx2;
            if (be > vis) be = vis;
            if (bs < vis) {
                bx = (int16_t)(8 + ((uint32_t)bs * 224u) / vis);
                bx2 = (int16_t)(8 + ((uint32_t)be * 224u) / vis - 1);
                o->fill(o->ctx, bx, 47, (int16_t)(bx2 - bx + 1), 1, UI_BLUE);
            }
        }
        {
            static const char *tab[4] = {"TRIG", "NOTE", "FLAGS", "SEQ"};
            uint16_t tc[4] = {ui_segment_colour(ui_segment_for_step(&e->project, ui->selected_step)), UI_BLUE,
                              UI_CORAL, UI_SOFT};
            uint8_t seg = ui_segment_for_step(&e->project, ui->selected_step);
            value_u(a, "STEP", (uint32_t)ui->selected_step + 1u);
            midi_note_name(b, midi);
            o->text(o->ctx, 8, 48, 52, 0, a, UI_WHITE, 0);
            o->text(o->ctx, 64, 48, 42, 0, b, ui_segment_colour(seg), 1);
            value_i(a, "OCT", s->octave);
            o->text(o->ctx, 108, 48, 48, 0, a, UI_MID, 0);
            value_u(a, "P", s->pulses);
            o->text(o->ctx, 160, 48, 24, 0, a, UI_MID, 1);
            o->text(o->ctx, 188, 48, 44, 0, tab[ui->step_subpage < 4u ? ui->step_subpage : 0u],
                    tc[ui->step_subpage < 4u ? ui->step_subpage : 0u], 2);

            for (i = 0u; i < 4u; ++i) {
                x = (int16_t)(8 + i * 56u);
                ui_frame(o, x, 66, 52, 18, i == ui->step_subpage ? tc[i] : UI_PANEL2);
                o->text(o->ctx, x + 2, 67, 48, 0, tab[i], i == ui->step_subpage ? tc[i] : UI_DIM, 1);
            }
        }

        o->text(o->ctx, 8, 87, 48, 0, "PULSES", UI_DIM, 0);
        for (i = 0u; i < 8u; ++i) {
            uint16_t c = i < s->pulses ? UI_ACC : UI_PANEL2;
            o->fill(o->ctx, (int16_t)(58 + i * 10u), 93, 7, 4, c);
        }
        o->text(o->ctx, 145, 87, 46, 0, "RATCH", UI_DIM, 0);
        for (i = 0u; i < 8u; ++i) {
            uint16_t c = i < s->ratchet ? UI_MAGENTA : UI_PANEL2;
            o->fill(o->ctx, (int16_t)(191 + i * 5u), 91, 3, 8, c);
        }

        o->text(o->ctx, 8, 106, 38, 0, "GATE", UI_DIM, 0);
        for (i = 0u; i < 4u; ++i) {
            int16_t gx = (int16_t)(50 + i * 25u);
            uint16_t c = i == (uint8_t)s->gate ? UI_ACC : UI_PANEL2;
            ui_frame(o, gx, 104, 20, 13, c);
            if (i == 0u)
                o->fill(o->ctx, gx + 4, 110, 12, 1, c);
            else if (i == 1u)
                o->fill(o->ctx, gx + 4, 108, 12, 5, c);
            else if (i == 2u) {
                ui_frame(o, gx + 7, 107, 6, 7, c);
            } else {
                o->fill(o->ctx, gx + 4, 107, 12, 1, c);
                o->fill(o->ctx, gx + 4, 110, 12, 1, c);
                o->fill(o->ctx, gx + 4, 113, 12, 1, c);
            }
        }

        {
            const char *fl[4] = {"SLD", "ACC", "SUB", "SKIP"};
            uint8_t on[4] = {s->slide ? 1u : 0u, s->accent ? 1u : 0u, s->sub ? 1u : 0u, s->skip ? 1u : 0u};
            uint16_t fc[4] = {UI_BLUE, UI_CORAL, UI_GOLD, UI_RED};
            for (i = 0u; i < 4u; ++i) {
                x = (int16_t)(8 + i * 55u);
                ui_frame(o, x, 124, 49, 18, on[i] ? fc[i] : UI_PANEL2);
                o->text(o->ctx, x + 2, 125, 45, 0, fl[i], on[i] ? fc[i] : UI_DIM, 1);
            }
        }
        value_u(a, "PROB", prob_pct(s->probability));
        o->text(o->ctx, 8, 146, 64, 0, a, UI_PURPLE, 0);
        ui_hbar(o, 76, 150, 88, s->probability, 65535u, UI_PURPLE);
        if (s->sample_slot == 0u)
            copy_s(b, "SAMP OFF");
        else {
            char *q = copy_s(b, "SAMP ");
            *q++ = (char)('A' + s->sample_slot - 1u);
            *q = 0;
        }
        o->text(o->ctx, 168, 146, 64, 0, b, s->sample_slot ? UI_BLUE : UI_DIM, 2);
        break;
    }
    case MELLO_UI_SEQ_SEGMENT: {
        uint8_t s = ui->selected_segment, i, bounds[MELLO_MAX_SEGMENTS + 1u], sc;
        int16_t x;
        if (s >= e->project.seg_count) s = 0u;
        sc = ui_segment_bounds(&e->project, bounds);
        /* A-F brackets over the same 16-cell sequence language used by HOME. */
        {
            uint8_t vis = ui_step_overview_count(&e->project);
            for (i = 0u; i < sc; ++i) {
                uint8_t bs = bounds[i], be = bounds[i + 1u];
                int16_t bx, bx2;
                uint16_t c = ui_segment_colour(i);
                if (bs >= vis) continue;
                if (be > vis) be = vis;
                bx = (int16_t)(8 + ((uint32_t)bs * 224u) / vis);
                bx2 = (int16_t)(8 + ((uint32_t)be * 224u) / vis - 2);
                o->fill(o->ctx, bx, 48, (int16_t)(bx2 - bx + 1), 1, c);
                o->fill(o->ctx, bx, 48, 1, 4, c);
                o->fill(o->ctx, bx2, 48, 1, 4, c);
                a[0] = (char)('A' + i);
                a[1] = 0;
                o->text(o->ctx, bx, 31, (int16_t)(bx2 - bx + 1), 0, a, c, 1);
            }
            for (i = 0u; i < vis; ++i) {
                uint8_t seg = 0xFFu, j;
                int16_t cw = ui_step_overview_w(&e->project, i);
                x = ui_step_overview_x(&e->project, i);
                for (j = 0u; j < sc; ++j)
                    if (i >= bounds[j] && i < bounds[j + 1u]) {
                        seg = j;
                        break;
                    }
                o->fill(o->ctx, x, 52, cw, 14, i < e->project.seq_len ? UI_PANEL2 : UI_BLACK);
                if (i < e->project.seq_len && seg != 0xFFu)
                    o->fill(o->ctx, x, 61, cw, 2, ui_segment_colour(seg));
                if (seg == s) o->fill(o->ctx, x, 53, cw, 2, ui_segment_colour(s));
                if (ui->transport->display_step == i) ui_frame(o, x, 52, cw, 14, UI_WHITE);
            }
            if (s < sc && bounds[s] < vis) {
                uint8_t be = bounds[s + 1u] > vis ? vis : bounds[s + 1u];
                int16_t sx = (int16_t)(8 + ((uint32_t)bounds[s] * 224u) / vis);
                int16_t sx2 = (int16_t)(8 + ((uint32_t)be * 224u) / vis - 1);
                if (sx2 > sx) ui_frame(o, sx, 50, (int16_t)(sx2 - sx), 18, UI_SOFT);
            }
        }
        value_u(a, "SEG", (uint32_t)s + 1u);
        b[0] = (char)('A' + s);
        b[1] = 0;
        o->text(o->ctx, 8, 70, 46, 0, a, UI_WHITE, 0);
        o->text(o->ctx, 55, 70, 20, 0, b, ui_segment_colour(s), 1);
        value_u(a, "x", e->project.seg_repeats[s]);
        o->text(o->ctx, 84, 70, 40, 0, a, UI_MAGENTA, 0);
        o->text(o->ctx, 130, 70, 44, 0, e->project.seg_direction[s] == MELLO_SEG_REV ? "REV" : "FWD", UI_BLUE,
                1);
        o->text(o->ctx, 179, 70, 53, 0,
                e->project.seg_mode[s] == MELLO_SEG_PINGPONG ? "P-P"
                : e->project.seg_mode[s] == MELLO_SEG_RANDOM ? "RND"
                                                             : "LIN",
                e->project.seg_mode[s] == MELLO_SEG_RANDOM ? UI_PURPLE : UI_BLUE, 1);

        if (ui->segment_subpage == 0u) {
            o->text(o->ctx, 8, 90, 42, 0, "RPT", UI_DIM, 0);
            for (i = 0u; i < 4u; ++i) {
                x = (int16_t)(55 + i * 24u);
                ui_frame(o, x, 87, 20, 15, i + 1u == e->project.seg_repeats[s] ? UI_MAGENTA : UI_PANEL2);
                a[0] = 'x';
                a[1] = (char)('1' + i);
                a[2] = 0;
                o->text(o->ctx, x + 1, 91, 18, 0, a,
                        i + 1u == e->project.seg_repeats[s] ? UI_MAGENTA : UI_DIM, 1);
            }
            ui_frame(o, 157, 87, 34, 15, e->project.seg_direction[s] == MELLO_SEG_FWD ? UI_BLUE : UI_PANEL2);
            ui_frame(o, 195, 87, 37, 15, e->project.seg_direction[s] == MELLO_SEG_REV ? UI_BLUE : UI_PANEL2);
            o->text(o->ctx, 158, 91, 32, 0, "FWD",
                    e->project.seg_direction[s] == MELLO_SEG_FWD ? UI_BLUE : UI_DIM, 1);
            o->text(o->ctx, 196, 91, 35, 0, "REV",
                    e->project.seg_direction[s] == MELLO_SEG_REV ? UI_BLUE : UI_DIM, 1);

            o->text(o->ctx, 8, 113, 40, 0, "MODE", UI_DIM, 0);
            {
                const char *mn[3] = {"LIN", "P-P", "RND"};
                for (i = 0u; i < 3u; ++i) {
                    x = (int16_t)(55 + i * 44u);
                    uint16_t mc = i == 2u ? UI_PURPLE : UI_BLUE;
                    ui_frame(o, x, 109, 38, 16, i == (uint8_t)e->project.seg_mode[s] ? mc : UI_PANEL2);
                    o->text(o->ctx, x + 2, 113, 34, 0, mn[i],
                            i == (uint8_t)e->project.seg_mode[s] ? mc : UI_DIM, 1);
                }
            }
            ui_frame(o, 193, 109, 39, 16, e->project.seg_skip[s] ? UI_RED : UI_PANEL2);
            o->text(o->ctx, 195, 113, 35, 0, "SKIP", e->project.seg_skip[s] ? UI_RED : UI_DIM, 1);
        } else {
            o->text(o->ctx, 8, 89, 50, 0, "COUNT", UI_DIM, 0);
            for (i = 0u; i < MELLO_MAX_SEGMENTS; ++i) {
                x = (int16_t)(60 + i * 28u);
                ui_frame(o, x, 86, 24, 16, i + 1u == e->project.seg_count ? ui_segment_colour(i) : UI_PANEL2);
                a[0] = (char)('1' + i);
                a[1] = 0;
                o->text(o->ctx, x + 2, 90, 20, 0, a,
                        i + 1u == e->project.seg_count ? ui_segment_colour(i) : UI_DIM, 1);
            }

            o->text(o->ctx, 8, 113, 50, 0, "SELECT", UI_DIM, 0);
            for (i = 0u; i < e->project.seg_count; ++i) {
                x = (int16_t)(60 + i * 28u);
                ui_frame(o, x, 109, 24, 16, i == s ? UI_SOFT : ui_segment_colour_dim(i));
                a[0] = (char)('A' + i);
                a[1] = 0;
                o->text(o->ctx, x + 2, 113, 20, 0, a, i == s ? UI_WHITE : ui_segment_colour(i), 1);
            }
            if (s > 0u) {
                value_u(a, "SPLIT", e->project.seg_splits[s - 1u]);
                o->text(o->ctx, 8, 132, 70, 0, a, UI_GOLD, 0);
            } else
                o->text(o->ctx, 8, 132, 70, 0, "SPLIT --", UI_DIM, 0);
            ui_frame(o, 193, 129, 39, 16, e->project.seg_skip[s] ? UI_RED : UI_PANEL2);
            o->text(o->ctx, 195, 133, 35, 0, "SKIP", e->project.seg_skip[s] ? UI_RED : UI_DIM, 1);
        }

        value_u(a, "START", (uint32_t)bounds[s] + 1u);
        value_u(b, "END", bounds[s + 1u]);
        o->text(o->ctx, 82, 154, 60, 0, a, UI_MID, 0);
        o->text(o->ctx, 145, 154, 52, 0, b, UI_MID, 0);
        o->text(o->ctx, 8, 154, 68, 0, ui->segment_subpage ? "STRUCT" : "PLAY",
                ui->segment_subpage ? UI_GOLD : ui_segment_colour(s), 0);
        break;
    }
    case MELLO_UI_SEQ_LOOP: {
        static const char *tabs[3] = {"STEP", "HARM", "LOOP"};
        MelloLoopCycleStep *s = &e->loop_cycle.steps[ui->selected_loop_step];
        uint8_t i, len = e->loop_cycle.length;
        if (len < 2u) len = 2u;
        if (len > MELLO_MAX_LOOP_CYCLE_STEPS) len = MELLO_MAX_LOOP_CYCLE_STEPS;
        o->text(o->ctx, 8, 33, 80, 0, e->loop_cycle.active ? "LOOP CYCLE" : "AUDITION",
                e->loop_cycle.active ? UI_GOLD : UI_BLUE, 0);
        value_u(a, "STEP", (uint32_t)ui->selected_loop_step + 1u);
        o->text(o->ctx, 96, 33, 48, 0, a, UI_BRIGHT, 0);
        value_u(a, "LEN", len);
        o->text(o->ctx, 152, 33, 40, 0, a, UI_MID, 0);
        o->text(o->ctx, 200, 33, 32, 0, e->loop_cycle.sync ? "SYNC" : "FREE",
                e->loop_cycle.sync ? UI_GOLD : UI_DIM, 2);
        for (i = 0u; i < 3u; ++i) {
            int16_t tx = (int16_t)(8 + i * 75u);
            uint16_t tc = i == ui->loop_subpage ? UI_GOLD : UI_PANEL2;
            ui_frame(o, tx, 51, 70, 18, tc);
            o->text(o->ctx, tx + 2, 52, 66, 0, tabs[i], i == ui->loop_subpage ? UI_GOLD : UI_DIM, 1);
        }
        /* The eight physical cells always mirror the first eight white keys. */
        for (i = 0u; i < MELLO_MAX_LOOP_CYCLE_STEPS; ++i) {
            MelloLoopCycleStep *ls = &e->loop_cycle.steps[i];
            int16_t x = (int16_t)(8 + i * 28u);
            int16_t hold_h = (int16_t)(5 + ((uint32_t)(ls->hold > 128u ? 128u : ls->hold) * 34u) / 128u);
            int16_t root_y = (int16_t)(91 - ((int32_t)ls->root_offset * 1));
            uint8_t in_len = i < len;
            uint8_t active = i == e->loop_cycle_pos;
            uint8_t selected = i == ui->selected_loop_step;
            o->fill(o->ctx, x, 72, 23, 55, in_len ? UI_PANEL : UI_BLACK);
            ui_frame(o, x, 72, 23, 55,
                     active ? (e->loop_cycle.active ? UI_GOLD : UI_BLUE)
                            : (selected ? UI_WHITE : (in_len ? UI_PANEL2 : UI_BLACK)));
            if (in_len) {
                o->fill(o->ctx, x + 3, (int16_t)(123 - hold_h), 17, hold_h,
                        active ? (e->loop_cycle.active ? UI_GOLD : UI_BLUE) : UI_MID);
                root_y = (int16_t)(root_y + 10);
                if (root_y < 76) root_y = 76;
                if (root_y > 121) root_y = 121;
                o->fill(o->ctx, x + 3, root_y, 17, 2, UI_GOLD);
                if (ls->octave) o->fill(o->ctx, x + 3, ls->octave > 0 ? 75 : 121, 17, 2, UI_BLUE);
            }
            a[0] = (char)('1' + i);
            a[1] = 0;
            o->text(o->ctx, x, 130, 23, 0, a,
                    active ? (e->loop_cycle.active ? UI_GOLD : UI_BLUE) : (selected ? UI_WHITE : UI_DIM), 1);
        }
        if (ui->loop_subpage == 0u) {
            value_u(a, "H", s->hold);
            value_i(b, "R", s->root_offset);
            o->text(o->ctx, 8, 148, 52, 0, a, UI_GOLD, 0);
            o->text(o->ctx, 64, 148, 54, 0, b, UI_MID, 0);
            value_i(a, "T", s->pitch);
            value_i(b, "O", s->octave);
            o->text(o->ctx, 122, 148, 50, 0, a, UI_ACC, 0);
            o->text(o->ctx, 176, 148, 56, 0, b, UI_BLUE, 2);
        } else if (ui->loop_subpage == 1u) {
            if (s->root_note < 0)
                copy_s(a, "FOLLOW");
            else {
                value_u(a, "KEY", (uint32_t)s->root_note);
            }
            if (s->scale_set < 0)
                copy_s(b, "FOLLOW");
            else
                copy_s(b, mello_scale_name((MelloScale)s->scale_set));
            o->text(o->ctx, 8, 148, 56, 0, a, UI_GOLD, 0);
            o->text(o->ctx, 68, 148, 76, 0, b, UI_MID, 1);
            value_u(a, "S", mello_loop_cycle_sub_tone(s));
            value_i(b, "B", mello_loop_cycle_sub_transpose(s));
            o->text(o->ctx, 148, 148, 36, 0, a, UI_MID, 0);
            o->text(o->ctx, 188, 148, 44, 0, b, UI_BLUE, 2);
        } else {
            value_u(a, "LEN", len);
            o->text(o->ctx, 8, 148, 48, 0, a, UI_GOLD, 0);
            o->text(o->ctx, 60, 148, 40, 0, e->loop_cycle.sync ? "SYNC" : "FREE",
                    e->loop_cycle.sync ? UI_GOLD : UI_MID, 1);
            o->text(o->ctx, 104, 148, 72, 0, e->loop_cycle.active ? "CYCLE ON" : "CYCLE OFF",
                    e->loop_cycle.active ? UI_GOLD : UI_DIM, 1);
            o->text(o->ctx, 180, 148, 52, 0, "K4 RST", UI_DIM, 2);
        }
        break;
    }
    case MELLO_UI_SEQ_SLOTSEQ: {
        MelloSlotSeqStep *s = &e->slot_seq.steps[ui->selected_slot_seq_step];
        uint8_t i, start = (uint8_t)(ui->slot_seq_bank * 16u), len = e->slot_seq.length;
        o->text(o->ctx, 8, 33, 96, 0, e->slot_seq.active ? "SLOT SEQ ON" : "SLOT SEQ OFF",
                e->slot_seq.active ? UI_BLUE : UI_DIM, 0);
        value_u(a, "LEN", len);
        o->text(o->ctx, 106, 33, 52, 0, a, UI_MID, 1);
        value_u(a, "BANK", ui->slot_seq_bank + 1u);
        o->text(o->ctx, 162, 33, 70, 0, a, UI_GOLD, 2);
        /* 16-position bank: cell fill carries slot identity, bottom bar carries HOLD. */
        for (i = 0u; i < 16u; ++i) {
            uint8_t idx = (uint8_t)(start + i);
            MelloSlotSeqStep *ss = &e->slot_seq.steps[idx];
            int16_t x = (int16_t)(8 + (i % 8u) * 28u), y = (int16_t)(54 + (i / 8u) * 43u);
            uint8_t in_len = idx < len, active = e->slot_seq.active && idx == e->slot_seq_pos,
                    selected = idx == ui->selected_slot_seq_step;
            uint16_t col = ss->slot < 0 ? UI_DIM : UI_BLUE;
            o->fill(o->ctx, x, y, 23, 33, in_len ? UI_PANEL : UI_BLACK);
            ui_frame(o, x, y, 23, 33,
                     active ? UI_WHITE : (selected ? UI_SOFT : (in_len ? UI_PANEL2 : UI_BLACK)));
            if (in_len && ss->slot >= 0) {
                uint8_t shade = (uint8_t)(((uint8_t)ss->slot % 4u));
                o->fill(o->ctx, x + 3, y + 4, 17, 16,
                        shade == 0u   ? UI_ACC
                        : shade == 1u ? UI_BLUE
                        : shade == 2u ? UI_PURPLE
                                      : UI_GOLD);
                value_u(a, "", (uint32_t)ss->slot + 1u);
                o->text(o->ctx, x + 2, y + 7, 19, 0, a, UI_WHITE, 1);
            }
            if (in_len) {
                int16_t hw = (int16_t)(3 + ((uint32_t)(ss->hold > 256u ? 256u : ss->hold) * 14u) / 256u);
                o->fill(o->ctx, x + 3, y + 25, hw, 3, col);
            }
        }
        if (s->slot < 0)
            copy_s(a, "SLOT OFF");
        else
            value_u(a, "SLOT", (uint32_t)s->slot + 1u);
        value_u(b, "HOLD", s->hold);
        o->text(o->ctx, 8, 146, 104, 0, a, s->slot < 0 ? UI_DIM : UI_BLUE, 0);
        o->text(o->ctx, 120, 146, 112, 0, b, UI_GOLD, 2);
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
        uint8_t i, enabled = 0u, rn_count = random_target_count();
        int16_t x, y, w;
        o->text(o->ctx, 8, 33, 138, 0, "INSPIRATION MASK", UI_PURPLE, 0);
        o->text(o->ctx, 150, 33, 82, 0, random_intensity_name(ui->random_intensity), UI_PURPLE, 2);
        for (i = 0u; i < rn_count; ++i) {
            uint8_t on = (e->randomize.mask & random_target_bit(i)) ? 1u : 0u;
            uint8_t row = i < 7u ? 0u : 1u;
            uint8_t col = row == 0u ? i : (uint8_t)(i - 7u);
            w = (i == (uint8_t)(rn_count - 1u)) ? 34 : 30;
            x = (int16_t)(8 + col * 32);
            y = (int16_t)(50 + row * 22);
            if (on) ++enabled;
            o->fill(o->ctx, x, y, w, 17, on ? UI_PANEL2 : UI_BLACK);
            ui_frame(o, x, y, w, 17, on ? rc[i] : UI_PANEL2);
            if (on) o->fill(o->ctx, x + 3, y + 13, w - 6, 1, rc[i]);
            o->text(o->ctx, x + 1, y + 2, w - 2, 0, rn[i], on ? rc[i] : UI_DIM, 1);
        }
        value_u(a, "ACTIVE", enabled);
        o->text(o->ctx, 8, 95, 72, 0, a, UI_MID, 0);
        o->text(o->ctx, 84, 95, 148, 0, "KEYS TOGGLE TARGET", UI_DIM, 2);
        ui_frame(o, 8, 112, 224, 18, UI_CORAL);
        o->fill(o->ctx, 11, 113, 218, 16, UI_PANEL);
        o->text(o->ctx, 12, 113, 216, 0, "RND: REROLL   UNDO: BACK", UI_CORAL, 0);
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
        MelloPatternSlot *s = &e->slots[ui->selected_slot];
        uint8_t i, start = (uint8_t)(ui->slot_bank * 16u);
        value_u(a, "BANK", ui->slot_bank + 1u);
        o->text(o->ctx, 8, 33, 58, 0, a, UI_GOLD, 0);
        value_u(a, "VISIBLE", e->visible_slot_count);
        o->text(o->ctx, 70, 33, 88, 0, a, UI_BLUE, 1);
        o->text(o->ctx, 162, 33, 70, 0, ui->slots_subpage ? "RENAME" : "LOAD", UI_MID, 2);
        for (i = 0u; i < 16u; ++i) {
            uint8_t idx = (uint8_t)(start + i);
            MelloPatternSlot *ps = &e->slots[idx];
            int16_t x = (int16_t)(8 + (i % 8u) * 28u), y = (int16_t)(54 + (i / 8u) * 36u);
            uint8_t selected = idx == ui->selected_slot, active = e->active_slot == (int8_t)idx,
                    queued = e->queued_slot == (int8_t)idx, visible = idx < e->visible_slot_count;
            uint16_t frame =
                active ? UI_ACC
                       : (queued ? UI_BLUE : (selected ? UI_WHITE : (visible ? UI_PANEL2 : UI_BLACK)));
            o->fill(o->ctx, x, y, 23, 27, ps->valid ? UI_PANEL2 : (visible ? UI_PANEL : UI_BLACK));
            ui_frame(o, x, y, 23, 27, frame);
            if (ps->valid)
                o->fill(o->ctx, x + 4, y + 5, 15, 10, active ? UI_ACC : (queued ? UI_BLUE : UI_MID));
            value_u(a, "", idx + 1u);
            o->text(o->ctx, x + 2, y + 17, 19, 0, a, selected ? UI_WHITE : (visible ? UI_DIM : UI_BLACK), 1);
        }
        o->text(o->ctx, 8, 132, 224, 0, s->valid ? s->name : "EMPTY", s->valid ? UI_ACC : UI_DIM, 0);
        if (ui->slots_subpage && s->valid) {
            int16_t cx = (int16_t)(8 + ((uint32_t)ui->slot_name_cursor * 224u) / MELLO_SLOT_NAME_CHARS);
            o->fill(o->ctx, cx, 151, 8, 2, UI_GOLD);
        }
        break;
    }
    case MELLO_UI_DOT: {
        MelloDotMatrixConfig *c = &e->dot[ui->selected_dot_target];
        static const char *tn[4] = {"ACC", "FLT", "RAT", "SUB"};
        static const uint16_t tc[4] = {UI_CORAL, UI_CYAN, UI_MAGENTA, UI_GOLD};
        uint8_t i, r, col;
        uint16_t colour = tc[ui->selected_dot_target];
        /* Target tabs keep the four original matrix identities visible at once. */
        for (i = 0u; i < 4u; ++i) {
            int16_t tx = (int16_t)(8 + i * 57u);
            ui_frame(o, tx, 33, 52, 17, i == ui->selected_dot_target ? tc[i] : UI_PANEL2);
            o->text(o->ctx, tx + 2, 38, 48, 0, tn[i], i == ui->selected_dot_target ? tc[i] : UI_DIM, 1);
        }
        /* Large 4x4 matrix: fill = enabled, bright inset frame = current read position. */
        for (r = 0u; r < 4u; ++r) {
            for (col = 0u; col < 4u; ++col) {
                uint8_t idx = (uint8_t)(r * 4u + col);
                int16_t x = (int16_t)(20 + col * 51u);
                int16_t y = (int16_t)(57 + r * 23u);
                uint8_t on = (c->cells & (uint16_t)(1u << idx)) ? 1u : 0u;
                o->fill(o->ctx, x, y, 45, 18, on ? colour : UI_PANEL);
                ui_frame(o, x, y, 45, 18, on ? colour : UI_PANEL2);
                if (e->dot_active_cell[ui->selected_dot_target] == (int8_t)idx)
                    ui_frame(o, x + 2, y + 2, 41, 14, UI_WHITE);
            }
        }
        o->text(o->ctx, 8, 153, 48, 0, dot_mode_name(c->mode), colour, 0);
        value_u(a, "LEN", c->length);
        o->text(o->ctx, 64, 153, 48, 0, a, UI_MID, 0);
        value_u(a, "DEP", c->amount / 10u);
        o->text(o->ctx, 120, 153, 56, 0, a, colour, 0);
        o->text(o->ctx, 184, 153, 48, 0, c->follow ? "FOLLOW" : (c->reset ? "RESET" : "FREE"),
                c->follow ? UI_BLUE : UI_DIM, 2);
        break;
    }
    case MELLO_UI_TURING: {
        uint8_t i;
        uint8_t len = e->tm.length < 3u ? 3u : e->tm.length;
        uint8_t window = len <= 8u ? 0u : (uint8_t)((ui->selected_tm_bit / 8u) * 8u);
        if ((uint16_t)window + 8u > len) window = (uint8_t)(len > 8u ? len - 8u : 0u);
        o->text(o->ctx, 8, 33, 92, 0, e->tm.on ? (e->tm.locked ? "EVOLVE LOCK" : "EVOLVE ON") : "EVOLVE OFF",
                e->tm.on ? UI_BLUE : UI_DIM, 0);
        value_u(a, "LEN", len);
        o->text(o->ctx, 102, 33, 42, 0, a, UI_MID, 1);
        o->text(o->ctx, 148, 33, 84, 0,
                e->tm.rate == MELLO_TM_RATE_HALF     ? "RATE 1/2X"
                : e->tm.rate == MELLO_TM_RATE_DOUBLE ? "RATE 2X"
                                                     : "RATE 1X",
                UI_MID, 2);
        /* Eight register lanes: continuous float value in green + gate pad in cyan. */
        for (i = 0u; i < 8u; ++i) {
            uint8_t idx = (uint8_t)(window + i);
            int16_t x = (int16_t)(8 + i * 28u);
            int16_t h = idx < len ? (int16_t)(((uint32_t)e->tm_float_reg[idx] * 69u) / 65535u) : 0;
            uint8_t gate = idx < len && e->tm_bool_reg[idx];
            uint8_t active = e->tm_active_index == (int8_t)idx;
            o->fill(o->ctx, x, 52, 23, 84, UI_PANEL);
            ui_frame(o, x, 52, 23, 84,
                     active ? UI_WHITE : (idx == ui->selected_tm_bit ? UI_SOFT : UI_PANEL2));
            if (h > 0) o->fill(o->ctx, x + 3, (int16_t)(128 - h), 17, h, UI_BLUE);
            o->fill(o->ctx, x + 3, 130, 17, 4, gate ? UI_ACC : UI_PANEL2);
            a[0] = (char)('1' + (idx % 10u));
            a[1] = 0;
            if (idx >= 9u) {
                a[0] = (char)('0' + ((idx + 1u) / 10u));
                a[1] = (char)('0' + ((idx + 1u) % 10u));
                a[2] = 0;
            }
            o->text(o->ctx, x, 137, 23, 0, a, active ? UI_WHITE : UI_DIM, 1);
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
        uint8_t i, xlen = e->sub.x_length;
        if (xlen != 2u && xlen != 3u && xlen != 5u) xlen = 5u;
        o->text(o->ctx, 8, 33, 70, 0, e->sub.on ? "SUB ON" : "SUB OFF", e->sub.on ? UI_GOLD : UI_DIM, 0);
        o->text(o->ctx, 82, 33, 72, 0, sub_mode_name(e->sub.mode), UI_GOLD, 1);
        value_i(a, "OCT", e->sub.octave);
        o->text(o->ctx, 158, 33, 74, 0, a, UI_MID, 2);
        /* Main generator: pulse density + gate length remain visible regardless of X override. */
        o->text(o->ctx, 8, 53, 42, 0, "PULSE", UI_DIM, 0);
        ui_hbar(o, 52, 56, 78, e->sub.euclid_pulses, 64u, UI_GOLD);
        value_u(a, "", e->sub.euclid_pulses);
        o->text(o->ctx, 134, 53, 24, 0, a, UI_GOLD, 0);
        o->text(o->ctx, 162, 53, 34, 0, "ROT", UI_DIM, 0);
        value_u(a, "", e->sub.euclid_rotate);
        o->text(o->ctx, 198, 53, 34, 0, a, UI_MID, 2);
        o->text(o->ctx, 8, 69, 42, 0, "GATE", UI_DIM, 0);
        ui_hbar(o, 52, 72, 106, e->sub.gate_amount > 1200u ? 1200u : e->sub.gate_amount, 1200u, UI_GOLD);
        value_u(a, "", e->sub.gate_amount / 10u);
        o->text(o->ctx, 162, 69, 70, 0, a, UI_MID, 2);
        /* SUB 5-SEQ is a compact 2/3/5-step yellow sequencer. Disabled cells remain physically present. */
        o->text(o->ctx, 8, 87, 72, 0, e->sub.x_on ? "SUB 5-SEQ" : "5-SEQ OFF", e->sub.x_on ? UI_GOLD : UI_DIM,
                0);
        value_u(a, "LEN", xlen);
        o->text(o->ctx, 84, 87, 40, 0, a, UI_MID, 0);
        o->text(o->ctx, 128, 87, 104, 0, e->sub.x_reset ? "RESET LOOP" : "FREE RUN",
                e->sub.x_reset ? UI_GOLD : UI_DIM, 2);
        for (i = 0u; i < MELLO_SUB_X_MAX_STEPS; ++i) {
            MelloSubXStep *xs = &e->sub.x_steps[i];
            int16_t x = (int16_t)(8 + i * 45u);
            int16_t tone_h = (int16_t)(5 + (xs->tone * 4u));
            uint8_t in_len = i < xlen;
            uint8_t active = e->sub.x_on && e->sub_x_active_index == (int8_t)i;
            uint8_t selected = i == ui->selected_sub_x_step;
            o->fill(o->ctx, x, 104, 40, 42, in_len ? UI_PANEL : UI_BLACK);
            ui_frame(o, x, 104, 40, 42,
                     active ? UI_GOLD : (selected ? UI_WHITE : (in_len ? UI_PANEL2 : UI_BLACK)));
            if (in_len && xs->on) {
                if (tone_h > 28) tone_h = 28;
                o->fill(o->ctx, x + 4, (int16_t)(139 - tone_h), 32, tone_h, UI_GOLD);
            } else if (in_len) {
                o->fill(o->ctx, x + 4, 137, 32, 2, UI_DIM);
            }
            if (in_len && xs->octave) o->fill(o->ctx, x + 4, xs->octave > 0 ? 108 : 142, 32, 2, UI_BLUE);
            a[0] = (char)('1' + i);
            a[1] = 0;
            o->text(o->ctx, x, 149, 40, 0, a, active ? UI_GOLD : (selected ? UI_WHITE : UI_DIM), 1);
        }
        break;
    }
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: {
        MelloChordConfig *c = &e->chord;
        uint8_t i;
        uint16_t col = UI_CHORD;
        o->text(o->ctx, 8, 33, 72, 0, c->on ? "CHORD ON" : "CHORD OFF", c->on ? col : UI_DIM, 0);
        o->text(o->ctx, 82, 33, 72, 0, chord_mode_name(c->mode), col, 1);
        o->text(o->ctx, 158, 33, 74, 0, c->trigger == MELLO_CHORD_TRIGGER_GRID ? "GRID" : "STEP", UI_MID, 2);
        if (ui->chord_subpage == 3u) {
            for (i = 0u; i < 16u; ++i) {
                int16_t x = (int16_t)(20 + (i % 4u) * 51u), y = (int16_t)(57 + (i / 4u) * 23u);
                uint8_t on = (c->grid_cells & (uint16_t)(1u << i)) != 0u;
                o->fill(o->ctx, x, y, 45, 18, on ? col : UI_PANEL);
                ui_frame(o, x, y, 45, 18, on ? col : UI_PANEL2);
                if (e->chord_grid_active_cell == (int8_t)i) ui_frame(o, x + 2, y + 2, 41, 14, UI_WHITE);
            }
            o->text(o->ctx, 8, 153, 224, 0, "CHORD TRIGGER GRID", col, 0);
        } else if (ui->chord_subpage == 4u) {
            for (i = 0u; i < c->pattern_length; ++i) {
                int16_t x = (int16_t)(10 + i * 28u);
                uint8_t j;
                uint8_t m = c->pattern_masks[i];
                ui_frame(o, x, 62, 24, 72, i == ui->selected_chord_pattern ? UI_WHITE : UI_PANEL2);
                for (j = 0u; j < 4u; ++j)
                    o->fill(o->ctx, x + 4, 68 + j * 14, 16, 8, (m & (1u << j)) ? col : UI_PANEL2);
            }
            o->text(o->ctx, 8, 145, 224, 0, "PATTERN: SOLO + POLY MASKS", UI_DIM, 0);
        } else {
            uint8_t notes[4],
                n = mello_engine_resolve_chord_notes(
                    e, ui->selected_chord_step < c->length ? ui->selected_chord_step : 0u, notes);
            for (i = 0u; i < c->length; ++i) {
                int16_t x = (int16_t)(8 + i * 28u);
                uint16_t fc =
                    i == e->chord_chain_pos ? UI_WHITE : (i == ui->selected_chord_step ? col : UI_PANEL2);
                ui_frame(o, x, 58, 23, 32, fc);
                value_u(a, "", (uint32_t)c->steps[i].degree + 1u);
                o->text(o->ctx, x, 68, 23, 0, a, fc, 1);
            }
            for (i = 0u; i < n; ++i) {
                int16_t x = (int16_t)(12 + i * 55u);
                midi_note_name(a, notes[i]);
                ui_frame(o, x, 105, 49, 33, col);
                o->text(o->ctx, x + 2, 116, 45, 0, a, col, 1);
            }
            if (ui->chord_subpage == 1u)
                o->text(o->ctx, 8, 148, 224, 0,
                        chord_shape_name(c->steps[ui->selected_chord_step % c->length].shape), col, 1);
            else if (ui->chord_subpage == 2u)
                o->text(o->ctx, 8, 148, 224, 0, "DEDICATED TONE + MAIN FX", UI_DIM, 0);
            else {
                const char *clock_hint;
                if (c->trigger == MELLO_CHORD_TRIGGER_GRID)
                    clock_hint = "GRID CLOCK / CHAIN HIT";
                else if (c->mode == MELLO_CHORD_HOLD)
                    clock_hint = "HOLD PAD / CHAIN STEP";
                else if (c->mode == MELLO_CHORD_STAB)
                    clock_hint = "STAB PULSE / CHAIN STEP";
                else
                    clock_hint = "ARP/PAT PULSE / CHAIN STEP";
                o->text(o->ctx, 8, 148, 224, 0, clock_hint, UI_DIM, 0);
            }
        }
        break;
    }
#endif
    case MELLO_UI_GLOBAL: draw_param_panel(ui, o); break;
    case MELLO_UI_SOUND_RESERVED: draw_param_panel(ui, o); break;
    case MELLO_UI_AUDIO_FX: draw_param_panel(ui, o); break;
    case MELLO_UI_SAMPLE_RESERVED: {
        uint8_t si = e->sample.edit_slot < MELLO_SAMPLE_SLOT_COUNT ? e->sample.edit_slot : 0u, i;
        MelloSampleSlotConfig *sl = &e->sample.slots[si];
        const MelloSampleSlotMeta *sm = ui->sample_bank ? &ui->sample_bank->meta[si] : 0;
        uint8_t loaded = (uint8_t)(sm && sm->loaded);
        uint32_t ms =
            loaded ? (sm->sample_count * 1000u + MELLO_SAMPLE_SOURCE_RATE / 2u) / MELLO_SAMPLE_SOURCE_RATE
                   : 0u;
        o->text(o->ctx, 8, 33, 96, 0,
                ui->sample_subpage == 0u   ? "SAMPLE MAIN"
                : ui->sample_subpage == 1u ? "SAMPLE PITCH"
                : ui->sample_subpage == 2u ? "TRIM / LOOP"
                                           : "ENVELOPE",
                UI_GOLD, 0);
        o->text(o->ctx, 104, 33, 128, 0,
                e->sample.on ? (e->sample.per_step ? "PER STEP" : "FIXED SLOT") : "LAYER OFF",
                e->sample.on ? UI_GOLD : UI_DIM, 2);
        /* A-D occupancy is metadata, not an invented waveform. */
        for (i = 0u; i < MELLO_SAMPLE_SLOT_COUNT; ++i) {
            const MelloSampleSlotMeta *mi = ui->sample_bank ? &ui->sample_bank->meta[i] : 0;
            int16_t x = (int16_t)(8 + i * 56u);
            uint8_t ld = (uint8_t)(mi && mi->loaded);
            uint8_t selected = i == si;
            o->fill(o->ctx, x, 51, 52, 22, ld ? UI_PANEL2 : UI_PANEL);
            ui_frame(o, x, 51, 52, 22, selected ? UI_WHITE : (ld ? UI_ACC : UI_PANEL2));
            a[0] = (char)('A' + i);
            a[1] = 0;
            o->text(o->ctx, x + 2, 55, 48, 0, a, selected ? UI_WHITE : (ld ? UI_ACC : UI_DIM), 1);
        }
        if (loaded) {
            char line[32], *p = copy_s(line, "SLOT ");
            *p++ = (char)('A' + si);
            *p++ = ' ';
            copy_s(p, sm->name);
            o->text(o->ctx, 8, 80, 150, 0, line, UI_ACC, 0);
            value_u(a, "MS", ms);
            o->text(o->ctx, 162, 80, 70, 0, a, UI_BLUE, 2);
        } else {
            o->text(o->ctx, 8, 80, 224, 0, "EMPTY - USB UPLOAD STOPPED", UI_DIM, 0);
        }
        if (ui->sample_subpage == 3u) {
            ui_env_graph(o, 8, 101, 224, 55, sl->attack_ms, sl->decay_ms, sl->sustain_permille,
                         sl->release_ms, UI_ACC);
        } else {
            int16_t tx0 = (int16_t)(8 + ((uint32_t)224u * sl->trim_start) / 1000u),
                    tx1 = (int16_t)(8 + ((uint32_t)224u * sl->trim_end) / 1000u);
            o->fill(o->ctx, 8, 104, 224, 30, UI_PANEL);
            ui_frame(o, 8, 104, 224, 30, UI_PANEL2);
            if (tx1 < tx0 + 2) tx1 = (int16_t)(tx0 + 2);
            if (tx1 > 231) tx1 = 231;
            o->fill(o->ctx, tx0, 109, (int16_t)(tx1 - tx0), 20, loaded ? UI_ACC : UI_MID);
            o->fill(o->ctx, tx0, 104, 2, 30, UI_GOLD);
            o->fill(o->ctx, tx1, 104, 2, 30, UI_GOLD);
            if (ui->sample_subpage == 1u) {
                int16_t center = 120 + (int16_t)(sl->coarse_semitones * 4);
                if (center < 10) center = 10;
                if (center > 230) center = 230;
                o->fill(o->ctx, center, 101, 2, 36, UI_BLUE);
            }
            if (sl->loop) o->text(o->ctx, 12, 139, 48, 0, "LOOP", UI_GOLD, 0);
            if (sl->track) o->text(o->ctx, 64, 139, 56, 0, "TRACK", UI_BLUE, 0);
            value_u(a, "LEVEL", sl->level / 10u);
            o->text(o->ctx, 124, 139, 108, 0, a, UI_MID, 2);
        }
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
            o->text(o->ctx, (int16_t)(px + 3), 172, (int16_t)(w - 6), 0, fl[f], on[f] ? UI_BRIGHT : UI_DIM, 1);
            px = (int16_t)(px + w + 2);
        }
        ui_spaced(o, (int16_t)(px + 6), 172, (int16_t)(236 - px - 6), "STEP", UI_DIM, 2);
        return;
    }
    if (ui->page == MELLO_UI_HOME && ui->keys_mode) { /* every key is a note now: say what a key does */
        o->text(o->ctx, 4, 172, 232, 0,
                ui->keys_rec                 ? "REC: KEYS WRITE STEPS"
                : ui->transport->running     ? "KEYS: KEY = NEW ROOT"
                                             : "KEYS: PLAY  REC: STEPS",
                ui->keys_rec ? UI_RED : UI_GOLD, 1);
        return;
    }
    for (i = 0u; i < 11u; ++i) {
        const MelloContextAction *a = context_action(ui->page, i);
        if (i == group_end[g]) {
            x = (int16_t)(x + 3);
            ++g;
        }
        if (a && a->verb != MELLO_CTX_NONE && a->label) {
            int16_t w = (int16_t)(ui_text_len(a->label) * 8u + 2u);
            uint16_t c = a->verb == MELLO_CTX_RANDOM_CLEAR || a->verb == MELLO_CTX_GLOBAL_RESET
                             ? (ui->reset_armed ? UI_RED : UI_DIM)
                             : accent;
            ui_round_frame(o, x, 172, w, 16, UI_LINE); /* a pill, as the HTML's buttons: a dim edge */
            o->text(o->ctx, (int16_t)(x + 1), 172, (int16_t)(w - 2), 0, a->label, c, 1);
            x = (int16_t)(x + w + 1); /* air between pills, as the HTML's buttons */
        } else {
            o->fill(o->ctx, x, 179, 2, 2, UI_LINE);
            x = (int16_t)(x + 3);
        }
    }
}

static const uint16_t CUTOFF_HZ[101] = { /* 80 * 200^(i / 100): logFilterHz, the HTML's cutoff in Hz */
    80, 84, 89, 94, 99, 104, 110, 116, 122, 129, 136, 143,
    151, 159, 168, 177, 187, 197, 208, 219, 231, 243, 257, 271,
    285, 301, 317, 334, 353, 372, 392, 413, 436, 460, 485, 511,
    539, 568, 599, 632, 666, 702, 740, 781, 823, 868, 915, 965,
    1018, 1073, 1131, 1193, 1258, 1326, 1398, 1475, 1555, 1639, 1729, 1823,
    1922, 2026, 2137, 2253, 2375, 2505, 2641, 2785, 2936, 3096, 3264, 3442,
    3629, 3827, 4035, 4255, 4486, 4730, 4988, 5259, 5545, 5847, 6165, 6500,
    6854, 7227, 7620, 8035, 8472, 8933, 9419, 9932, 10472, 11042, 11643, 12276,
    12944, 13649, 14391, 15174, 16000,
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
            v = k == 0u ? e->sound.filter_cut : k == 1u ? e->sound.decay_ms : k == 2u ? e->fx.reverb_wet : e->fx.dist_amount;
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
        case 6: v = f->delay_tone; break;
        case 7: v = f->delay_level, m = 1500u; break;
        case 10: v = f->delay_time_ms, m = 2000u; break;
        case 11: v = f->dist_level, m = 1400u; break;
        case 12: v = f->reverb_wet; break;
        case 13: v = f->reverb_size; break;
        case 14: v = f->reverb_decay_ms, m = 60000u; break;
        case 15: v = f->reverb_tone; break;
        case 16: v = f->reverb_pre_ms, m = 120u; break;
        case 17: v = f->reverb_level, m = 3000u; break;
        default: return false;
        }
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
        uint8_t sg = ui->selected_segment < e->project.seg_count ? ui->selected_segment : 0u;
        if (ui->segment_subpage == 0u) {
            l[0] = "REPEAT";
            value_u(v[0], "", e->project.seg_repeats[sg]);
            l[1] = "DIR";
            copy_s(v[1], e->project.seg_direction[sg] == MELLO_SEG_FWD ? "FWD" : "REV");
            l[2] = "MODE";
            copy_s(v[2], e->project.seg_mode[sg] == MELLO_SEG_LINEAR     ? "LIN"
                         : e->project.seg_mode[sg] == MELLO_SEG_PINGPONG ? "P-P"
                                                                         : "RND");
            l[3] = "SKIP";
            copy_s(v[3], e->project.seg_skip[sg] ? "ON" : "OFF");
        } else {
            l[0] = "COUNT";
            value_u(v[0], "", e->project.seg_count);
            l[1] = "SPLIT";
            if (sg > 0u)
                value_u(v[1], "", e->project.seg_splits[sg - 1u]);
            else
                copy_s(v[1], "START");
            l[2] = "REPEAT";
            value_u(v[2], "", e->project.seg_repeats[sg]);
            l[3] = "SKIP";
            copy_s(v[3], e->project.seg_skip[sg] ? "ON" : "OFF");
        }
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
        MelloDotMatrixConfig *c = &e->dot[ui->selected_dot_target];
        l[0] = "MODE";
        copy_s(v[0], dot_mode_name(c->mode));
        l[1] = "LEN";
        value_u(v[1], "", c->length);
        l[2] = "DEPTH";
        value_u(v[2], "", c->amount / 10u);
        l[3] = "FOLLOW";
        copy_s(v[3], c->follow ? "ON" : "OFF");
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
            value_u(v[2], "", e->sub.decay_ms);
            l[3] = "REL";
            value_u(v[3], "", e->sub.release_ms);
        } else {
            l[0] = "GLIDE";
            value_u(v[0], "", e->sub.glide_ms);
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
        MelloChordConfig *c = &e->chord;
        MelloChordStep *s = &c->steps[ui->selected_chord_step % c->length];
        if (ui->chord_subpage == 0u) {
            l[0] = "MODE";
            copy_s(v[0], chord_mode_name(c->mode));
            l[1] = "TRIG";
            copy_s(v[1], c->trigger == MELLO_CHORD_TRIGGER_GRID ? "GRID" : "STEP");
            l[2] = "CHAIN";
            value_u(v[2], "", c->length);
            l[3] = "LEVEL";
            value_u(v[3], "", c->level / 10u);
        } else if (ui->chord_subpage == 1u) {
            l[0] = "DEG";
            value_u(v[0], "", s->degree + 1u);
            l[1] = "SHAPE";
            copy_s(v[1], chord_shape_name(s->shape));
            l[2] = "HOLD";
            value_u(v[2], "", s->hold);
            l[3] = "OCT";
            value_i(v[3], "", s->octave);
        } else if (ui->chord_subpage == 2u) {
            l[0] = "INV";
            value_u(v[0], "", s->inversion);
            l[1] = "SPREAD";
            value_u(v[1], "", s->spread);
            l[2] = "WAVE";
            copy_s(v[2], wave_name(c->wave));
            l[3] = "GATE";
            value_u(v[3], "", c->gate_amount / 10u);
        } else if (ui->chord_subpage == 3u) {
            l[0] = "MODE";
            copy_s(v[0], dot_mode_name(c->grid_mode));
            l[1] = "LEN";
            value_u(v[1], "", c->grid_length);
            l[2] = "DEPTH";
            value_u(v[2], "", c->grid_amount / 10u);
            l[3] = "FOLLOW";
            copy_s(v[3], c->grid_follow ? "ON" : "OFF");
        } else {
            uint8_t p = ui->selected_chord_pattern % c->pattern_length;
            l[0] = "MASK";
            value_u(v[0], "", c->pattern_masks[p]);
            l[1] = "LEN";
            value_u(v[1], "", c->pattern_length);
            l[2] = "VOICES";
            value_u(v[2], "", c->voices);
            l[3] = "REL";
            value_u(v[3], "", c->release_ms);
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
            l[2] = "TONE";
            value_pct(v[2], e->fx.delay_tone);
            l[3] = "LEVEL";
            value_pct(v[3], e->fx.delay_level);
        } else if (ui->fx_subpage == 2u) {
            l[0] = "SYNC";
            copy_s(v[0], e->fx.delay_sync ? "ON" : "OFF");
            l[1] = "DIV";
            copy_s(v[1], delay_div_name(e->fx.delay_div));
            l[2] = "TIME";
            value_ms(v[2], e->fx.delay_time_ms);
            l[3] = "D.LEVEL";
            value_pct(v[3], e->fx.dist_level);
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
            copy_s(v[2], "-");
            copy_s(v[3], "-");
        }
        break;
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
            value_u(v[0], "", sl->attack_ms);
            l[1] = "DEC";
            value_u(v[1], "", sl->decay_ms);
            l[2] = "SUS";
            value_u(v[2], "", sl->sustain_permille / 10u);
            l[3] = "REL";
            value_u(v[3], "", sl->release_ms);
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

/* A layer as the HTML's synth panel: its title spaced in the domain's colour, then K1-K4 as rows, each a label and
 * its value, with the HTML's slider under a continuous value and a pill around a list's or a switch's. The texts are
 * the footer's (footer_texts), the colours knob_colour's (the envelope's A D S R as the HTML's letters). */
static void draw_param_panel(MelloUi *ui, const MelloUiDrawOps *o)
{
    char v[4][24];
    const char *l[4], *title = layer_name(ui);
    uint16_t accent = ui_page_accent(ui);
    uint8_t k;
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
    static const char *step[4] = {"TRIG", "NOTE", "FLAGS", "SEQ"}, *seg[2] = {"PLAY", "STRUCT"},
                      *loop[3] = {"STEP", "HARM", "LOOP"}, *slots[2] = {"LOAD", "RENAME"},
                      *dot[4] = {"ACCENT", "FILTER", "RATCHET", "SUB"}, *tm[3] = {"MAIN", "ROUTES", "DEPTH"},
                      *sub[5] = {"TRIG", "5-SEQ", "STEP", "VOICE", "TONE"}, *glo[2] = {"CLOCK", "MIDI"},
                      *snd[5] = {"OSC / FILTER", "AMP ENVELOPE", "AUDIO LFO", "FOLLOWER SUB", "PLAY / LEVEL"},
                      *fx[5] = {"DISTORTION", "DELAY", "DELAY CONFIG", "REVERB", "REVERB RETURN"},
                      *smp[4] = {"SAMPLE MAIN", "SAMPLE PITCH", "TRIM / LOOP", "ENVELOPE"};
    switch (ui->page) {
    case MELLO_UI_SEQ_STEP: return step[ui->step_subpage % 4u];
    case MELLO_UI_SEQ_SEGMENT: return seg[ui->segment_subpage % 2u];
    case MELLO_UI_SEQ_LOOP: return loop[ui->loop_subpage % 3u];
    case MELLO_UI_SLOTS: return slots[ui->slots_subpage % 2u];
    case MELLO_UI_DOT: return dot[ui->selected_dot_target % 4u];
    case MELLO_UI_TURING: return tm[ui->tm_subpage % 3u];
    case MELLO_UI_SUB: return sub[ui->sub_subpage % 5u];
    case MELLO_UI_GLOBAL: return glo[ui->global_subpage % 2u];
    case MELLO_UI_SOUND_RESERVED: return ui->sound_subpage == 2u ? 0 : snd[ui->sound_subpage % 5u];
    case MELLO_UI_AUDIO_FX: return fx[ui->fx_subpage % 5u];
    case MELLO_UI_SAMPLE_RESERVED: return smp[ui->sample_subpage % 4u];
    case MELLO_UI_HOME: return ui->held_step == MELLO_UI_NO_STEP ? 0 : "HELD STEP";
    default: return 0;
    }
}

/* The footer's short labels in full, for the readout's large line (13 characters fit). */
static const char *full_label(const char *l)
{
    static const char *map[][2] = {{"CUT", "CUTOFF"},      {"RES", "RESONANCE"},  {"ATK", "ATTACK"},
                                   {"DEC", "DECAY"},       {"SUS", "SUSTAIN"},    {"REL", "RELEASE"},
                                   {"PWM", "PULSE WIDTH"}, {"FDBK", "FEEDBACK"},  {"AMT", "AMOUNT"},
                                   {"CHAR", "CHARACTER"},  {"RATCH", "RATCHET"},  {"OCT", "OCTAVE"},
                                   {"PRE", "PRE-DELAY"},   {"DIV", "DIVISION"},   {"D.LEVEL", "DIST LEVEL"},
                                   {"SUBOSC", "SUB OSC"},  {"PROB", "PROBABILITY"}, {"LEN", "LENGTH"},
                                   {"DEP", "DEPTH"},       {"ACC", "ACCENT"}};
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
    o->fill(o->ctx, 0, 30, 240, 142, UI_BLACK); /* in place of the body, not over it: no overdraw, no flicker */
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
        if (ui->transport->running && at >= base && at < base + 16u) mask ^= bit_u32(white_key_notes[at - base]);
    } else if (ui->page == MELLO_UI_SEQ_STEP) {
        uint8_t p = (uint8_t)(ui->selected_step % 16u);
        if (ui->selected_step / 16u == ui->step_bank) mask |= bit_u32(white_key_notes[p]);
    } else if (ui->page == MELLO_UI_SEQ_SEGMENT) {
        if (ui->selected_segment < 6u) mask |= bit_u32(white_key_notes[ui->selected_segment]);
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
        if (at >= 0 && at < 16) mask ^= bit_u32(white_key_notes[at]); /* the read position: inverted, so it shows */
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
