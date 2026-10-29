#include "mello_ui.h"

#define UI_RGB(r, g, b)                                                                                      \
    ((uint16_t)((((uint16_t)(r) & 0xF8u) << 8) | (((uint16_t)(g) & 0xFCu) << 3) | ((uint16_t)(b) >> 3)))
#define UI_BLACK   UI_RGB(0, 0, 0)
#define UI_PANEL   UI_RGB(3, 15, 10)
#define UI_PANEL2  UI_RGB(6, 24, 16)
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
#define UI_CHORD   UI_RGB(148, 126, 255) /* harmonic layer */

/* Structural colours: segment identity is deliberately softer than function colours. */
#define UI_SEG_A     UI_RGB(72, 196, 126)
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
    case MELLO_UI_SAMPLE_RESERVED: return MELLO_UI_BTN_ENV;
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
    mark_full(ui);
}

static void seq_button(MelloUi *ui)
{
    switch (ui->page) {
    case MELLO_UI_SEQ_STEP: mello_ui_set_page(ui, MELLO_UI_SEQ_SEGMENT); break;
    case MELLO_UI_SEQ_SEGMENT: mello_ui_set_page(ui, MELLO_UI_SEQ_LOOP); break;
    case MELLO_UI_SEQ_LOOP: mello_ui_set_page(ui, MELLO_UI_SEQ_SLOTSEQ); break;
    case MELLO_UI_SEQ_SLOTSEQ: mello_ui_set_page(ui, MELLO_UI_SEQ_STEP); break;
    default: mello_ui_set_page(ui, MELLO_UI_SEQ_STEP); break;
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
    MELLO_CTX_SAMPLE_TOGGLE
} MelloContextVerb;

typedef struct {
    MelloContextVerb verb;
    const char *label;
} MelloContextAction;

/* WP-C5 authoritative contextual-action mapping. The physical B0-B7 slot,
 * rendered label and handler verb are defined together here. */
static const MelloContextAction context_actions[MELLO_UI_SAMPLE_RESERVED + 1u][8] = {
    [MELLO_UI_HOME] = {[5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_SEQ_STEP] = {[4] = {MELLO_CTX_STEP_SKIP, "SKIP"}, [5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_SEQ_SEGMENT] = {[5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_SEQ_LOOP] = {[2] = {MELLO_CTX_LOOP_RESET, "RST"}, [5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_SEQ_SLOTSEQ] = {[5] = {MELLO_CTX_OPEN_RANDOM, "RANDOM"}},
    [MELLO_UI_RANDOM] = {[1] = {MELLO_CTX_RANDOM_REROLL, "RND"},
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

static void handle_note_press(MelloUi *ui, uint32_t now_ms, uint32_t held_buttons, uint8_t note_index)
{
    MelloEngine *e = eng(ui);
    int8_t w;
    if (!e) return;

    if (ui->page == MELLO_UI_SEQ_STEP && (held_buttons & bit_u32(MELLO_UI_BTN_SEL))) {
        set_pitch_from_key(ui, note_index);
        return;
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
    case MELLO_UI_HOME: set_selected_step_from_note(ui, note_index); break;
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

static void adjust_home(MelloUi *ui, uint8_t knob, int16_t delta)
{
    MelloEngine *e = eng(ui);
    MelloStep *s;
    if (!e || !delta) return;
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
            p = clamp_i32(p + (delta > 0 ? 5 : -5), 0, 100);
            p -= p % 5;
            s->probability = pct_prob((uint8_t)p);
        } else if (knob == 2u)
            s->sample_slot = (uint8_t)clamp_i32((int32_t)s->sample_slot + (delta > 0 ? 1 : -1), 0, 4);
        else if (knob == 3u)
            s->accent = !s->accent;
        break;
    case 2u: /* FLAGS */
        if (knob == 0u)
            s->accent = !s->accent;
        else if (knob == 1u)
            s->sub = !s->sub;
        else if (knob == 2u)
            s->slide = !s->slide;
        else if (knob == 3u)
            s->skip = !s->skip;
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
            e->project.seg_skip[s] = !e->project.seg_skip[s];
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
            e->project.seg_skip[s] = !e->project.seg_skip[s];
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
            mello_engine_loop_cycle_set_sync(e, !e->loop_cycle.sync);
        else if (knob == 2u)
            mello_engine_loop_cycle_set_active(e, !e->loop_cycle.active);
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
        c.amount = (uint16_t)clamp_i32((int32_t)c.amount + (delta > 0 ? 10 : -10), 0, 1000);
    else if (knob == 3u)
        c.follow = !c.follow;
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
            c.mutation_prob = (uint16_t)clamp_i32((int32_t)c.mutation_prob + (delta > 0 ? 5 : -5), 0, 500);
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
        c.routes ^= mask;
    } else {
        uint16_t *d = knob == 0u   ? &c.depth_gate
                      : knob == 1u ? &c.depth_filter
                      : knob == 2u ? &c.depth_octave
                                   : &c.depth_decay;
        *d = (uint16_t)clamp_i32((int32_t)*d + (delta > 0 ? 10 : -10), 0, 1000);
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
            c.x_on = !c.x_on;
        else if (knob == 1u) {
            if (delta > 0)
                c.x_length = c.x_length == 2u ? 3u : 5u;
            else
                c.x_length = c.x_length == 5u ? 3u : 2u;
        } else if (knob == 2u)
            c.x_reset = !c.x_reset;
        else if (knob == 3u)
            c.gate_amount = (uint16_t)clamp_i32((int32_t)c.gate_amount + (delta > 0 ? 50 : -50), 150, 1200);
    } else if (ui->sub_subpage == 2u) {
        if (knob == 0u)
            xs->on = !xs->on;
        else if (knob == 1u)
            xs->tone = (uint8_t)wrap_u8((int32_t)xs->tone + (delta > 0 ? 1 : -1), 8u);
        else if (knob == 2u)
            xs->octave = (int8_t)clamp_i32((int32_t)xs->octave + delta, -2, 2);
        else if (knob == 3u)
            c.gate_amount = (uint16_t)clamp_i32((int32_t)c.gate_amount + (delta > 0 ? 50 : -50), 150, 1200);
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
            c.grid_follow = !c.grid_follow;
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
            c.pwm_permille = (uint16_t)clamp_i32((int32_t)c.pwm_permille + (delta > 0 ? 10 : -10), 50, 950);
        else if (knob == 2u)
            c.filter_cut = (uint16_t)clamp_i32((int32_t)c.filter_cut + (delta > 0 ? 10 : -10), 0, 1000);
        else
            c.filter_res = (uint16_t)clamp_i32((int32_t)c.filter_res + (delta > 0 ? 10 : -10), 0, 1000);
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
            c.lfo_on = !c.lfo_on;
        else if (knob == 1u)
            c.lfo_rate_millihz = (uint16_t)clamp_i32((int32_t)c.lfo_rate_millihz + delta * 100, 8, 20000);
        else if (knob == 2u)
            c.lfo_depth = (uint16_t)clamp_i32((int32_t)c.lfo_depth + delta * 10, 0, 1000);
        else
            c.lfo_target = (MelloLfoTarget)wrap_u8((int32_t)c.lfo_target + (delta > 0 ? 1 : -1), 4u);
    } else if (ui->sound_subpage == 3u) {
        if (knob == 0u)
            c.follower_sub_on = !c.follower_sub_on;
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
            c.delay_sync = !c.delay_sync;
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
            sl->track = !sl->track;
        else
            sl->loop = !sl->loop;
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
            sl->track = !sl->track;
        else
            sl->loop = !sl->loop;
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
            mello_transport_set_midi_note_out(t, !t->midi_note_out_enabled);
    } else {
        if (knob == 0u)
            mello_transport_set_midi_channel(t,
                                             (uint8_t)clamp_i32((int32_t)t->midi_channel + 1 + delta, 1, 16));
        else if (knob == 1u)
            mello_transport_set_midi_sub_channel(
                t, (uint8_t)clamp_i32((int32_t)t->midi_sub_channel + 1 + delta, 1, 16));
        else if (knob == 2u)
            mello_transport_set_midi_fixed_length(t, !t->midi_fixed_length, t->midi_fixed_len_ms);
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
        ui->sound_subpage = wrap_u8((int32_t)ui->sound_subpage + (delta > 0 ? 1 : -1), 5u);
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

static void adjust_knobs(MelloUi *ui, const int16_t enc[MELLO_UI_ENCODER_COUNT], uint32_t now_us,
                         uint32_t now_ms)
{
    uint8_t k;
    adjust_select(ui, enc[MELLO_UI_ENC_SELECT]);
    adjust_algorithm(ui, enc[MELLO_UI_ENC_ALGORITHM]);
    adjust_presets(ui, enc[MELLO_UI_ENC_PRESETS]);
    for (k = 0u; k < 4u; ++k) {
        int16_t d = enc[MELLO_UI_ENC_KNOB1 + k];
        if (!d) continue;
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
    case MELLO_CTX_RANDOM_CLEAR:
        if (ui->reset_armed && (int32_t)(now_ms - ui->reset_confirm_until_ms) <= 0) {
            ui->reset_armed = false;
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
        }
    }
    if (pressed & bit_u32(MELLO_UI_BTN_SEL)) mello_ui_set_page(ui, MELLO_UI_KEY_SCALE);
    if (pressed & bit_u32(MELLO_UI_BTN_FX)) mello_ui_set_page(ui, MELLO_UI_AUDIO_FX);
    if (pressed & bit_u32(MELLO_UI_BTN_LFO)) {
        if (ui->page == MELLO_UI_SOUND_RESERVED && ui->sound_subpage == 2u) {
            mello_ui_set_page(ui, MELLO_UI_DOT);
        } else if (ui->page == MELLO_UI_DOT) {
            mello_ui_set_page(ui, MELLO_UI_TURING);
        } else {
            ui->sound_subpage = 2u;
            if (ui->page == MELLO_UI_SOUND_RESERVED)
                mark_full(ui);
            else
                mello_ui_set_page(ui, MELLO_UI_SOUND_RESERVED);
        }
    }
    if (pressed & bit_u32(MELLO_UI_BTN_ARP)) {
#if MELLO_FEATURE_CHORD_MACHINE
        if (ui->page == MELLO_UI_SUB)
            mello_ui_set_page(ui, MELLO_UI_CHORD);
        else if (ui->page == MELLO_UI_CHORD)
            mello_ui_set_page(ui, MELLO_UI_SUB);
        else
            mello_ui_set_page(ui, MELLO_UI_SUB);
#else
        if (ui->page == MELLO_UI_SUB) {
            MelloSubConfig c = e->sub;
            c.on = !c.on;
            mello_engine_sub_set_config(e, &c);
            mark_edit(ui);
        } else
            mello_ui_set_page(ui, MELLO_UI_SUB);
#endif
    }
    if (pressed & bit_u32(MELLO_UI_BTN_GLO)) mello_ui_set_page(ui, MELLO_UI_GLOBAL);
    if (pressed & bit_u32(MELLO_UI_BTN_ENV)) {
        /* The SOUND page's AUDIO LFO subpage belongs to the LFO button: ENV never lands there (it kept the
         * subpage LFO left, so after one LFO press ENV showed the LFO and OSC/ENV were out of reach). */
        if (ui->page == MELLO_UI_SOUND_RESERVED && ui->sound_subpage != 2u)
            mello_ui_set_page(ui, MELLO_UI_SAMPLE_RESERVED);
        else {
            if (ui->sound_subpage == 2u) ui->sound_subpage = 0u;
            if (ui->page == MELLO_UI_SOUND_RESERVED)
                mark_full(ui);
            else
                mello_ui_set_page(ui, MELLO_UI_SOUND_RESERVED);
        }
    }
    if (pressed & bit_u32(MELLO_UI_BTN_SAVE)) {
        if (ui->page == MELLO_UI_SLOTS)
            save_selected_slot(ui, now_ms);
        else
            mello_ui_set_page(ui, MELLO_UI_SLOTS);
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

void mello_ui_input(MelloUi *ui, uint32_t now_us, uint32_t now_ms, uint32_t pressed_buttons,
                    uint32_t released_buttons, uint32_t held_buttons, uint32_t note_pressed,
                    const int16_t encoder_delta[MELLO_UI_ENCODER_COUNT])
{
    uint8_t i;
    (void)released_buttons;
    if (!ui || !ui->transport) return;
    handle_buttons(ui, now_us, now_ms, pressed_buttons, held_buttons);
    for (i = 0u; i < 27u; ++i)
        if (note_pressed & bit_u32(i)) handle_note_press(ui, now_ms, held_buttons, i);
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

static void ui_frame(const MelloUiDrawOps *o, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour)
{
    if (w < 2 || h < 2) return;
    o->fill(o->ctx, x, y, w, 1, colour);
    o->fill(o->ctx, x, (int16_t)(y + h - 1), w, 1, colour);
    o->fill(o->ctx, x, y, 1, h, colour);
    o->fill(o->ctx, (int16_t)(x + w - 1), y, 1, h, colour);
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

static void ui_wave_icon(const MelloUiDrawOps *o, int16_t x, int16_t y, int16_t w, int16_t h,
                         MelloWaveform wave, uint16_t colour)
{
    static const uint8_t sine16[16] = {8u, 5u, 3u, 1u, 0u, 1u, 3u, 5u, 8u, 11u, 13u, 15u, 16u, 15u, 13u, 11u};
    uint8_t i;
    if (w < 16 || h < 8) return;
    o->fill(o->ctx, x, y, w, h, UI_PANEL);
    ui_frame(o, x, y, w, h, wave == MELLO_WAVE_OFF ? UI_DIM : UI_PANEL2);
    if (wave == MELLO_WAVE_OFF) return;
    for (i = 0u; i < 24u; ++i) {
        int16_t px = (int16_t)(x + 4 + ((int32_t)i * (w - 8)) / 24);
        int16_t py;
        uint8_t ph = (uint8_t)((i * 16u) / 24u);
        if (wave == MELLO_WAVE_SAW)
            py = (int16_t)(y + 4 + ((int32_t)(i % 12u) * (h - 9)) / 11);
        else if (wave == MELLO_WAVE_SQUARE)
            py = (int16_t)(y + ((i % 12u) < 6u ? 5 : h - 6));
        else if (wave == MELLO_WAVE_TRIANGLE) {
            uint8_t q = (uint8_t)(i % 12u);
            uint8_t t = q < 6u ? q : (uint8_t)(11u - q);
            py = (int16_t)(y + h - 5 - ((int32_t)t * (h - 10)) / 5);
        } else if (wave == MELLO_WAVE_PULSE)
            py = (int16_t)(y + ((i % 12u) < 3u ? 5 : h - 6));
        else
            py = (int16_t)(y + 4 + ((int32_t)sine16[ph] * (h - 9)) / 16);
        o->fill(o->ctx, px, py, 3, 2, colour);
    }
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
    case MELLO_UI_TURING: return UI_BLUE;
    case MELLO_UI_AUDIO_FX:
        return ui->fx_subpage == 0u ? UI_CORAL : (ui->fx_subpage <= 2u ? UI_BLUE : UI_PURPLE);
    case MELLO_UI_SOUND_RESERVED:
        return ui->sound_subpage == 0u
                   ? UI_CYAN
                   : (ui->sound_subpage == 2u ? UI_BLUE : (ui->sound_subpage == 3u ? UI_GOLD : UI_ACC));
    case MELLO_UI_SAMPLE_RESERVED: return UI_BLUE;
    case MELLO_UI_HOME:
    case MELLO_UI_SEQ_STEP:
        if (e) return ui_segment_colour(ui_segment_for_step(&e->project, ui->selected_step));
        return UI_ACC;
    case MELLO_UI_GLOBAL: return UI_SOFT;
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

static void draw_home_step(MelloUi *ui, const MelloUiDrawOps *o, uint8_t i)
{
    MelloEngine *e = eng(ui);
    MelloStep *s;
    uint8_t lo, hi, midi, active, selected, segment;
    int16_t x, w, base_y = 143, top_y, h, bar_w;
    uint16_t bar;
    if (!e || i >= ui_step_overview_count(&e->project)) return;
    x = ui_step_overview_x(&e->project, i);
    w = ui_step_overview_w(&e->project, i);
    o->fill(o->ctx, x, 47, w, 109, UI_BLACK);
    if (i >= e->project.seq_len) {
        o->fill(o->ctx, x, 139, w, 4, UI_PANEL);
        return;
    }
    s = &e->project.steps[i];
    segment = ui_segment_for_step(&e->project, i);
    bar = s->skip ? ui_segment_colour_dim(segment) : ui_segment_colour(segment);
    home_pitch_range(&e->project, &lo, &hi);
    midi = mello_resolve_step_midi(&e->project, s);
    h = (int16_t)(14 + ((int32_t)(midi - lo) * 72) / (hi > lo ? (int32_t)(hi - lo) : 1));
    if (h > 86) h = 86;
    top_y = (int16_t)(base_y - h);
    active = ui->transport->display_step == i;
    selected = ui->selected_step == i;
    bar_w = w > 4 ? (int16_t)(w - 2) : w;
    o->fill(o->ctx, x, 51, w, 93, UI_PANEL);
    o->fill(o->ctx, x + (w - bar_w) / 2, top_y, bar_w, h, bar);
    if (active)
        ui_frame(o, x, (int16_t)(top_y - 2), w, (int16_t)(h + 4), UI_WHITE);
    else if (selected)
        ui_frame(o, x, (int16_t)(top_y - 2), w, (int16_t)(h + 4), UI_SOFT);
    {
        int16_t mw = (int16_t)(((uint32_t)(s->pulses > 8u ? 8u : s->pulses) * (uint32_t)w + 7u) / 8u);
        if (mw < 1) mw = 1;
        if (mw > w) mw = w;
        o->fill(o->ctx, x, 147, mw, 1, UI_SOFT);
    }
    if (s->ratchet > 1u) o->fill(o->ctx, x, 150, w, 1, UI_MAGENTA);
    if (s->accent) o->fill(o->ctx, x, 152, w, 1, UI_CORAL);
    if (s->sub) o->fill(o->ctx, x, 154, w, 1, UI_GOLD);
    if (s->slide) o->fill(o->ctx, x, 155, w, 1, UI_BLUE);
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
        o->fill(o->ctx, x, 38, (int16_t)(x2 - x + 1), i == focus_seg ? 2 : 1, c);
        o->fill(o->ctx, x, 38, 1, 5, c);
        o->fill(o->ctx, x2, 38, 1, 5, c);
        b[0] = (char)('A' + i);
        b[1] = 0;
        o->text(o->ctx, x, 31, (int16_t)(x2 - x + 1), 0, b, i == focus_seg ? UI_WHITE : c, 1);
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
        p = copy_s(p, " MIDI ");
        put_u32(p, mello_resolve_step_midi(&e->project, s));
        o->text(o->ctx, 8, 156, 96, 0, b, UI_WHITE, 0);
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
    o->text(o->ctx, 4, 7, 88, 0, "SKURKYMELLO", UI_WHITE, 0);
    p = copy_s(b, mello_ui_page_name(ui->page));
    if (e &&
        (ui->page == MELLO_UI_HOME || ui->page == MELLO_UI_SEQ_STEP || ui->page == MELLO_UI_SEQ_SEGMENT)) {
        uint8_t seg = ui->page == MELLO_UI_SEQ_SEGMENT ? ui->selected_segment
                                                       : ui_segment_for_step(&e->project, ui->selected_step);
        *p++ = ' ';
        *p++ = (char)('A' + (seg < MELLO_MAX_SEGMENTS ? seg : 0u));
        *p = 0;
    }
    o->text(o->ctx, 96, 7, 92, 0, b, accent, 1);
    p = copy_s(status, ui->transport->running ? "R" : "S");
    put_u32(p, mello_transport_effective_bpm(ui->transport));
    o->text(o->ctx, 192, 7, 44, 0, status, ui->transport->running ? UI_ACC : UI_SOFT, 2);
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
        o->text(o->ctx, 8, 33, 96, 0, e->loop_cycle.active ? "LOOP CYCLE" : "AUDITION",
                e->loop_cycle.active ? UI_GOLD : UI_BLUE, 0);
        value_u(a, "STEP", (uint32_t)ui->selected_loop_step + 1u);
        o->text(o->ctx, 107, 33, 48, 0, a, UI_WHITE, 1);
        value_u(a, "LEN", len);
        o->text(o->ctx, 157, 33, 40, 0, a, UI_MID, 1);
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
        o->text(o->ctx, 12, 113, 216, 0, "REC REROLL   EDIT+REC CLEAR", UI_CORAL, 0);
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
        o->text(o->ctx, 70, 132, 162, 0, "SEL+NOTE FAST PITCH", UI_DIM, 2);
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
        o->text(o->ctx, 57, 153, 48, 0, a, UI_MID, 0);
        value_u(a, "DEP", c->amount / 10u);
        o->text(o->ctx, 106, 153, 56, 0, a, colour, 0);
        o->text(o->ctx, 164, 153, 68, 0, c->follow ? "FOLLOW" : (c->reset ? "RESET" : "FREE"),
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
    case MELLO_UI_GLOBAL:
        o->text(o->ctx, 8, 33, 70, 0, ui->global_subpage ? "MIDI" : "CLOCK", UI_GOLD, 0);
        o->text(o->ctx, 82, 33, 150, 0, sync_name(ui->transport->sync_mode), UI_BLUE, 2);
        if (!ui->global_subpage) {
            uint16_t bpm = mello_transport_effective_bpm(ui->transport);
            value_u(a, "BPM", bpm);
            o->text(o->ctx, 8, 58, 72, 1, a, UI_ACC, 0);
            ui_hbar(o, 84, 69, 148, bpm > 250u ? 250u : bpm, 250u, UI_ACC);
            value_u(a, "DIV", e->project.clock_div);
            o->text(o->ctx, 8, 104, 60, 0, a, UI_BLUE, 0);
            o->text(o->ctx, 72, 104, 88, 0,
                    ui->transport->midi_note_out_enabled ? "NOTEOUT ON" : "NOTEOUT OFF",
                    ui->transport->midi_note_out_enabled ? UI_GOLD : UI_DIM, 1);
            o->text(o->ctx, 162, 104, 70, 0, ui->transport->running ? "RUN" : "STOP",
                    ui->transport->running ? UI_ACC : UI_DIM, 2);
        } else {
            value_u(a, "LEAD", (uint32_t)ui->transport->midi_channel + 1u);
            value_u(b, "SUB", (uint32_t)ui->transport->midi_sub_channel + 1u);
            o->text(o->ctx, 8, 58, 104, 1, a, UI_ACC, 0);
            o->text(o->ctx, 120, 58, 112, 1, b, UI_GOLD, 2);
            o->text(o->ctx, 8, 108, 96, 0, ui->transport->midi_fixed_length ? "FIXED LEN" : "GATE LEN",
                    ui->transport->midi_fixed_length ? UI_BLUE : UI_DIM, 0);
            value_u(a, "MS", ui->transport->midi_fixed_len_ms);
            o->text(o->ctx, 108, 108, 124, 0, a, UI_MID, 2);
            ui_hbar(o, 8, 137, 224,
                    ui->transport->midi_fixed_len_ms > 2000u ? 2000u : ui->transport->midi_fixed_len_ms,
                    2000u, UI_BLUE);
        }
        o->text(o->ctx, 8, 154, 224, 0,
                ui->reset_armed ? "RESET? OCT-+OCT+ AGAIN" : "REC TAP   OCT-+OCT+ RESET",
                ui->reset_armed ? UI_RED : UI_DIM, 0);
        break;
    case MELLO_UI_SOUND_RESERVED: {
        const char *title = ui->sound_subpage == 0u   ? "OSC / FILTER"
                            : ui->sound_subpage == 1u ? "AMP ENVELOPE"
                            : ui->sound_subpage == 2u ? "AUDIO LFO"
                            : ui->sound_subpage == 3u ? "FOLLOWER SUB"
                                                      : "PLAY / LEVEL";
        o->text(o->ctx, 8, 33, 122, 0, title, UI_GOLD, 0);
        o->text(o->ctx, 134, 33, 98, 0, wave_name(e->sound.osc_wave),
                e->sound.osc_wave == MELLO_WAVE_OFF ? UI_DIM : UI_ACC, 2);
        if (ui->sound_subpage == 1u) {
            ui_env_graph(o, 8, 54, 224, 72, e->sound.attack_ms, e->sound.decay_ms, e->sound.sustain_permille,
                         e->sound.release_ms, UI_ACC);
            value_u(a, "SUS", e->sound.sustain_permille / 10u);
            o->text(o->ctx, 8, 136, 68, 0, a, UI_ACC, 0);
            value_u(a, "ATK", e->sound.attack_ms);
            o->text(o->ctx, 80, 136, 70, 0, a, UI_MID, 0);
            value_u(a, "REL", e->sound.release_ms);
            o->text(o->ctx, 154, 136, 78, 0, a, UI_MID, 2);
        } else if (ui->sound_subpage == 2u) {
            ui_wave_icon(o, 8, 54, 224, 54, MELLO_WAVE_SINE, e->sound.lfo_on ? UI_BLUE : UI_DIM);
            o->text(o->ctx, 8, 118, 70, 0, e->sound.lfo_on ? "LFO ON" : "LFO OFF",
                    e->sound.lfo_on ? UI_BLUE : UI_DIM, 0);
            o->text(o->ctx, 82, 118, 76, 0, lfo_target_name(e->sound.lfo_target), UI_MID, 1);
            ui_hbar(o, 162, 122, 70, e->sound.lfo_depth, 1000u, UI_BLUE);
            value_u(a, "RATE", e->sound.lfo_rate_millihz);
            o->text(o->ctx, 8, 142, 104, 0, a, UI_MID, 0);
            value_u(a, "DEP", e->sound.lfo_depth / 10u);
            o->text(o->ctx, 116, 142, 116, 0, a, UI_BLUE, 2);
        } else if (ui->sound_subpage == 3u) {
            ui_wave_icon(o, 8, 54, 150, 64, e->sound.follower_sub_wave,
                         e->sound.follower_sub_on ? UI_GOLD : UI_DIM);
            o->text(o->ctx, 164, 58, 68, 0, e->sound.follower_sub_on ? "SUB ON" : "SUB OFF",
                    e->sound.follower_sub_on ? UI_GOLD : UI_DIM, 2);
            value_i(a, "OCT", e->sound.follower_sub_octave);
            o->text(o->ctx, 164, 82, 68, 0, a, UI_BLUE, 2);
            ui_hbar(o, 164, 108, 68, e->sound.follower_sub_level, 1000u, UI_GOLD);
            value_u(a, "LEVEL", e->sound.follower_sub_level / 10u);
            o->text(o->ctx, 8, 136, 224, 0, a, UI_GOLD, 0);
        } else {
            ui_wave_icon(o, 8, 54, 150, 64, e->sound.osc_wave, UI_ACC);
            o->text(o->ctx, 164, 58, 68, 0, wave_name(e->sound.osc_wave), UI_ACC, 2);
            if (ui->sound_subpage == 0u) {
                o->text(o->ctx, 8, 128, 40, 0, "CUT", UI_DIM, 0);
                ui_hbar(o, 50, 132, 108, e->sound.filter_cut, 1000u, UI_CYAN);
                o->text(o->ctx, 162, 128, 34, 0, "RES", UI_DIM, 0);
                ui_hbar(o, 198, 132, 34, e->sound.filter_res, 1000u, UI_CYAN);
                value_u(a, "PWM", e->sound.pwm_permille / 10u);
                o->text(o->ctx, 164, 88, 68, 0, a, UI_MID, 2);
            } else {
                o->text(o->ctx, 8, 128, 44, 0, "SLIDE", UI_DIM, 0);
                ui_hbar(o, 54, 132, 78, e->sound.slide_ms, 300u, UI_BLUE);
                o->text(o->ctx, 136, 128, 48, 0, "MASTER", UI_DIM, 0);
                ui_hbar(o, 186, 132, 46, e->sound.master_level, 1000u, UI_GOLD);
            }
        }
        break;
    }
    case MELLO_UI_AUDIO_FX: {
        uint8_t sel = ui->fx_subpage;
        uint16_t dist = e->fx.dist_amount, dly = e->fx.delay_wet, rev = e->fx.reverb_wet;
        o->text(o->ctx, 8, 33, 224, 0,
                sel == 0u   ? "DISTORTION"
                : sel == 1u ? "DELAY"
                : sel == 2u ? "DELAY CONFIG"
                : sel == 3u ? "REVERB"
                            : "REVERB RETURN",
                UI_GOLD, 0);
        /* Three always-visible processors make signal state readable at a glance. */
        {
            int16_t xs[3] = {8, 84, 160};
            uint16_t vals[3] = {dist, dly, rev};
            uint16_t cols[3] = {UI_CORAL, UI_BLUE, UI_PURPLE};
            const char *names[3] = {"DIST", "DELAY", "REVERB"};
            uint8_t i;
            for (i = 0u; i < 3u; ++i) {
                uint8_t active =
                    (i == 0u ? sel == 0u : (i == 1u ? (sel == 1u || sel == 2u) : (sel == 3u || sel == 4u)));
                o->fill(o->ctx, xs[i], 54, 72, 78, UI_PANEL);
                ui_frame(o, xs[i], 54, 72, 78, active ? cols[i] : UI_PANEL2);
                o->text(o->ctx, xs[i] + 3, 59, 66, 0, names[i], active ? cols[i] : UI_MID, 1);
                ui_hbar(o, xs[i] + 7, 86, 58, vals[i], 1000u, cols[i]);
                value_u(a, "", vals[i] / 10u);
                o->text(o->ctx, xs[i] + 3, 101, 66, 0, a, cols[i], 1);
            }
        }
        if (sel == 0u) {
            value_u(a, "MIX", e->fx.dist_mix / 10u);
            value_u(b, "CHAR", e->fx.dist_character / 10u);
            o->text(o->ctx, 8, 142, 104, 0, a, UI_CORAL, 0);
            o->text(o->ctx, 120, 142, 112, 0, b, UI_MID, 2);
        } else if (sel == 1u) {
            value_u(a, "FDBK", e->fx.delay_feedback / 10u);
            value_u(b, "TONE", e->fx.delay_tone / 10u);
            o->text(o->ctx, 8, 142, 104, 0, a, UI_BLUE, 0);
            o->text(o->ctx, 120, 142, 112, 0, b, UI_MID, 2);
        } else if (sel == 2u) {
            o->text(o->ctx, 8, 142, 82, 0, e->fx.delay_sync ? delay_div_name(e->fx.delay_div) : "FREE",
                    UI_BLUE, 0);
            value_u(a, "TIME", e->fx.delay_time_ms);
            o->text(o->ctx, 94, 142, 138, 0, a, UI_MID, 2);
        } else if (sel == 3u) {
            value_u(a, "SIZE", e->fx.reverb_size / 10u);
            value_u(b, "DEC", e->fx.reverb_decay_ms);
            o->text(o->ctx, 8, 142, 104, 0, a, UI_PURPLE, 0);
            o->text(o->ctx, 120, 142, 112, 0, b, UI_MID, 2);
        } else {
            value_u(a, "PRE", e->fx.reverb_pre_ms);
            value_u(b, "LEVEL", e->fx.reverb_level / 10u);
            o->text(o->ctx, 8, 142, 104, 0, a, UI_PURPLE, 0);
            o->text(o->ctx, 120, 142, 112, 0, b, UI_MID, 2);
        }
        break;
    }
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
    o->text(o->ctx, x + 2, 188, 56, 0, label, UI_DIM, 0);
    o->text(o->ctx, x + 2, 204, 56, 0, shown, colour, 1);
}

static void draw_control_hint(MelloUi *ui, const MelloUiDrawOps *o)
{
    char b[64];
    char *p = b;
    uint8_t i;
    b[0] = 0;
    for (i = 0u; i < 8u; ++i) {
        const MelloContextAction *a = context_action(ui->page, i);
        char key[4];
        if (!a || a->verb == MELLO_CTX_NONE || !a->label) continue;
        if (p != b) p = copy_s(p, " ");
        key[0] = 'B';
        key[1] = (char)('0' + i);
        key[2] = ':';
        key[3] = 0;
        p = copy_s(p, key);
        p = copy_s(p, a->label);
    }
    o->fill(o->ctx, 0, 172, 240, 16, UI_PANEL2);
    o->text(o->ctx, 4, 172, 232, 0, b, UI_SOFT, 0);
}

static void draw_footer(MelloUi *ui, const MelloUiDrawOps *o)
{
    MelloEngine *e = eng(ui);
    char v[4][24];
    const char *l[4] = {"K1", "K2", "K3", "K4"};
    uint8_t k;
    for (k = 0u; k < 4u; ++k) v[k][0] = 0;
    if (!e) return;
    switch (ui->page) {
    case MELLO_UI_HOME: {
        uint8_t si = ui->selected_step < MELLO_MAX_STEPS ? ui->selected_step : 0u;
        MelloStep *s = &e->project.steps[si];
        l[0] = "PITCH";
        value_i(v[0], "", s->pitch);
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
            value_u(v[1], "", e->sound.pwm_permille / 10u);
            l[2] = "CUT";
            value_u(v[2], "", e->sound.filter_cut / 10u);
            l[3] = "RES";
            value_u(v[3], "", e->sound.filter_res / 10u);
        } else if (ui->sound_subpage == 1u) {
            l[0] = "ATK";
            value_u(v[0], "", e->sound.attack_ms);
            l[1] = "DEC";
            value_u(v[1], "", e->sound.decay_ms);
            l[2] = "SUS";
            value_u(v[2], "", e->sound.sustain_permille / 10u);
            l[3] = "REL";
            value_u(v[3], "", e->sound.release_ms);
        } else if (ui->sound_subpage == 2u) {
            l[0] = "LFO";
            copy_s(v[0], e->sound.lfo_on ? "ON" : "OFF");
            l[1] = "RATE";
            value_u(v[1], "", e->sound.lfo_rate_millihz);
            l[2] = "DEPTH";
            value_u(v[2], "", e->sound.lfo_depth / 10u);
            l[3] = "TARGET";
            copy_s(v[3], lfo_target_name(e->sound.lfo_target));
        } else if (ui->sound_subpage == 3u) {
            l[0] = "SUBOSC";
            copy_s(v[0], e->sound.follower_sub_on ? "ON" : "OFF");
            l[1] = "LEVEL";
            value_u(v[1], "", e->sound.follower_sub_level / 10u);
            l[2] = "OCT";
            value_i(v[2], "", e->sound.follower_sub_octave);
            l[3] = "WAVE";
            copy_s(v[3], wave_name(e->sound.follower_sub_wave));
        } else {
            l[0] = "SLIDE";
            value_u(v[0], "", e->sound.slide_ms);
            l[1] = "MASTER";
            value_u(v[1], "", e->sound.master_level / 10u);
            copy_s(v[2], "-");
            copy_s(v[3], "-");
        }
        break;
    case MELLO_UI_AUDIO_FX:
        if (ui->fx_subpage == 0u) {
            l[0] = "AMT";
            value_u(v[0], "", e->fx.dist_amount / 10u);
            l[1] = "MIX";
            value_u(v[1], "", e->fx.dist_mix / 10u);
            l[2] = "CHAR";
            value_u(v[2], "", e->fx.dist_character / 10u);
            l[3] = "TONE";
            value_u(v[3], "", e->fx.dist_tone / 10u);
        } else if (ui->fx_subpage == 1u) {
            l[0] = "WET";
            value_u(v[0], "", e->fx.delay_wet / 10u);
            l[1] = "FDBK";
            value_u(v[1], "", e->fx.delay_feedback / 10u);
            l[2] = "TONE";
            value_u(v[2], "", e->fx.delay_tone / 10u);
            l[3] = "LEVEL";
            value_u(v[3], "", e->fx.delay_level / 10u);
        } else if (ui->fx_subpage == 2u) {
            l[0] = "SYNC";
            copy_s(v[0], e->fx.delay_sync ? "ON" : "OFF");
            l[1] = "DIV";
            copy_s(v[1], delay_div_name(e->fx.delay_div));
            l[2] = "TIME";
            value_u(v[2], "", e->fx.delay_time_ms);
            l[3] = "D.LEVEL";
            value_u(v[3], "", e->fx.dist_level / 10u);
        } else if (ui->fx_subpage == 3u) {
            l[0] = "WET";
            value_u(v[0], "", e->fx.reverb_wet / 10u);
            l[1] = "SIZE";
            value_u(v[1], "", e->fx.reverb_size / 10u);
            l[2] = "DECAY";
            value_u(v[2], "", e->fx.reverb_decay_ms);
            l[3] = "TONE";
            value_u(v[3], "", e->fx.reverb_tone / 10u);
        } else {
            l[0] = "PRE";
            value_u(v[0], "", e->fx.reverb_pre_ms);
            l[1] = "LEVEL";
            value_u(v[1], "", e->fx.reverb_level / 10u);
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
    draw_control_hint(ui, o);
    for (k = 0u; k < 4u; ++k) {
        uint16_t c = ui_page_accent(ui);
        if (ui->page == MELLO_UI_TURING && ui->tm_subpage >= 1u) {
            static const uint16_t route_col[4] = {UI_ACC, UI_CYAN, UI_BLUE, UI_CORAL};
            c = route_col[k];
        }
        footer_pair(o, k, l[k], v[k], c);
    }
}

static void draw_status(MelloUi *ui, const MelloUiDrawOps *o)
{
    char b[48], *p;
    MelloEngine *e = eng(ui);
    o->fill(o->ctx, 0, 220, 240, 20, UI_PANEL2);
    p = copy_s(b, "S");
    p = put_u32(p, ui->transport->display_step == 0xFFu ? 0u : (uint32_t)ui->transport->display_step + 1u);
    p = copy_s(p, " P");
    p = put_u32(p, (uint32_t)ui->transport->display_pulse + 1u);
    p = copy_s(p, " G");
    p = put_u32(p,
                ui->transport->display_segment == 0xFFu ? 0u : (uint32_t)ui->transport->display_segment + 1u);
    if (e && e->active_slot >= 0) {
        p = copy_s(p, " SLOT ");
        put_u32(p, (uint32_t)e->active_slot + 1u);
    }
    o->text(o->ctx, 4, 222, 232, 0, b, UI_MID, 0);
}

typedef struct {
    const MelloUiDrawOps *raw;
} MelloUiFitCtx;

static void fit_fill(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour)
{
    MelloUiFitCtx *f = (MelloUiFitCtx *)ctx;
    f->raw->fill(f->raw->ctx, x, y, w, h, colour);
}

static void fit_text(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large, const char *text,
                     uint16_t colour, uint8_t align)
{
    MelloUiFitCtx *f = (MelloUiFitCtx *)ctx;
    uint16_t n = ui_text_len(text);
    if (large && (uint32_t)n * 16u > (uint32_t)(w > 0 ? w : 0)) large = 0u;
    f->raw->text(f->raw->ctx, x, y, w, large, text, colour, align);
}

void mello_ui_render(MelloUi *ui, const MelloUiDrawOps *ops)
{
    uint16_t d;
    MelloUiFitCtx fit_ctx;
    MelloUiDrawOps fitted;
    if (!ui || !ops || !ops->fill || !ops->text) return;
    fit_ctx.raw = ops;
    fitted.fill = fit_fill;
    fitted.text = fit_text;
    fitted.key_led = 0;
    fitted.button_led = 0;
    fitted.ctx = &fit_ctx;
    d = ui->dirty;
    if (d & MELLO_UI_DIRTY_FULL) {
        fitted.fill(fitted.ctx, 0, 0, 240, 240, UI_BLACK);
        d |= MELLO_UI_DIRTY_HEADER | MELLO_UI_DIRTY_BODY | MELLO_UI_DIRTY_FOOTER | MELLO_UI_DIRTY_STATUS;
    }
    if (d & MELLO_UI_DIRTY_HEADER) draw_header(ui, &fitted);
    if (d & MELLO_UI_DIRTY_BODY)
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
    if (ui->page == MELLO_UI_HOME || ui->page == MELLO_UI_SEQ_STEP) {
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
        for (i = 0u; i < 16u; ++i)
            if (bits & (uint16_t)(1u << i)) mask |= bit_u32(white_key_notes[i]);
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
