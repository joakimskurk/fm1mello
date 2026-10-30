#ifndef MELLO_UI_H
#define MELLO_UI_H

#include "mello_transport.h"
#include "mello_storage.h"
#include "mello_sample.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MELLO_UI_SCREEN_W      240
#define MELLO_UI_SCREEN_H      240
#define MELLO_UI_WHITE_KEYS    16u
#define MELLO_UI_BUTTON_COUNT  14u
#define MELLO_UI_ENCODER_COUNT 7u

/* Logical panel ids deliberately match Dinghy BTN_* / ENC_* numeric ids. */
typedef enum {
    MELLO_UI_BTN_OCT_DOWN = 0,
    MELLO_UI_BTN_OCT_UP = 1,
    MELLO_UI_BTN_FX = 2,
    MELLO_UI_BTN_SEL = 3,
    MELLO_UI_BTN_ENV = 4,
    MELLO_UI_BTN_LFO = 5,
    MELLO_UI_BTN_EDIT = 6,
    MELLO_UI_BTN_GLO = 7,
    MELLO_UI_BTN_HOME = 8,
    MELLO_UI_BTN_SAVE = 9,
    MELLO_UI_BTN_ARP = 10,
    MELLO_UI_BTN_SEQ = 11,
    MELLO_UI_BTN_PLAY = 12,
    MELLO_UI_BTN_REC = 13
} MelloUiButton;

typedef enum {
    MELLO_UI_ENC_SELECT = 0,
    MELLO_UI_ENC_ALGORITHM = 1,
    MELLO_UI_ENC_KNOB1 = 2,
    MELLO_UI_ENC_KNOB2 = 3,
    MELLO_UI_ENC_KNOB3 = 4,
    MELLO_UI_ENC_KNOB4 = 5,
    MELLO_UI_ENC_PRESETS = 6
} MelloUiEncoder;

typedef enum {
    MELLO_UI_HOME = 0,
    MELLO_UI_SEQ_STEP,
    MELLO_UI_SEQ_SEGMENT,
    MELLO_UI_SEQ_LOOP,
    MELLO_UI_SEQ_SLOTSEQ,
    MELLO_UI_RANDOM,
    MELLO_UI_KEY_SCALE,
    MELLO_UI_SLOTS,
    MELLO_UI_DOT,
    MELLO_UI_TURING,
    MELLO_UI_SUB,
#if MELLO_FEATURE_CHORD_MACHINE
    MELLO_UI_CHORD,
#endif
    MELLO_UI_GLOBAL,
    MELLO_UI_SOUND_RESERVED,
    MELLO_UI_AUDIO_FX,
    MELLO_UI_SAMPLE_RESERVED
} MelloUiPage;

#define MELLO_UI_NO_STEP           0xFFu
#define MELLO_UI_TEXT_SPACED       2u /* text style: small, letter-spaced (text_on) */
#define MELLO_UI_READOUT_LAYER     5u /* MelloUi.readout: SELECT changed the layer */
#define MELLO_UI_READOUT_TEMPO     6u /* HOME's SELECT, ALGORITHM, PRESETS */
#define MELLO_UI_READOUT_INTENSITY 7u
#define MELLO_UI_READOUT_PATTERN   8u
#define MELLO_UI_READOUT_MS        1200u /* how long the readout stays after the last detent */

enum {
    MELLO_UI_DIRTY_HEADER = 1u << 0,
    MELLO_UI_DIRTY_BODY = 1u << 1,
    MELLO_UI_DIRTY_FOOTER = 1u << 2,
    MELLO_UI_DIRTY_STATUS = 1u << 3,
    MELLO_UI_DIRTY_LEDS = 1u << 4,
    MELLO_UI_DIRTY_FULL = 1u << 5,
    MELLO_UI_DIRTY_HOME_PLAYHEAD = 1u << 6,
    MELLO_UI_DIRTY_SEQ_PLAYHEAD = 1u << 7,
    MELLO_UI_DIRTY_ALL = 0xFFu
};

typedef struct {
    void (*fill)(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour);
    void (*text)(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large, const char *text, uint16_t colour,
                 uint8_t align);
    void (*key_led)(void *ctx, uint8_t key_index, bool on);
    void (*button_led)(void *ctx, uint8_t button_id, bool on);
    void *ctx;
    /* Optional: text on a background colour (the box is cleared to bg, the glyphs' edges blend into it), style 0
     * small, 1 large, 2 small letter-spaced (a pixel between characters). Without it, text() draws on black. */
    void (*text_on)(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t style, const char *text,
                    uint16_t colour, uint16_t bg, uint8_t align);
} MelloUiDrawOps;

typedef struct {
    MelloTransport *transport;
    MelloStorage *storage;
    MelloUiPage page;

    uint8_t step_subpage;    /* TRIG, NOTE, FLAGS, SEQ */
    uint8_t segment_subpage; /* PLAY, STRUCT */
    uint8_t loop_subpage;    /* STEP, HARM, LOOP */
    uint8_t slots_subpage;   /* LOAD, NAME */
    uint8_t tm_subpage;      /* MAIN, ROUTES, DEPTH */
    uint8_t sub_subpage;     /* MAIN, 5-SEQ CFG, 5-SEQ STEP */
#if MELLO_FEATURE_CHORD_MACHINE
    uint8_t chord_subpage; /* MAIN, CHAIN, VOICE, GRID, PATTERN */
#endif
    uint8_t global_subpage; /* CLOCK, MIDI */
    uint8_t sound_subpage;  /* OSC, ENV, LFO, FOLLOW, PLAY, GRIT, OSC 2, CHARACTER, MOD ENV */
    uint8_t fx_subpage;     /* DIST, DELAY A/B, REVERB A/B */
    uint8_t sample_subpage; /* MAIN, PITCH, TRIM, ENV */
    /* The readout: what the last turned control moves, large over the body, until readout_until_ms. */
    uint8_t readout; /* 0 = none, 1..4 = K1..K4, MELLO_UI_READOUT_LAYER = SELECT changed the layer */
    uint32_t readout_until_ms;
    /* HOME: a white key held down holds its step (K1-K4 edit it); released without a turn, it toggles the step. */
    uint32_t held_notes; /* mello_ui_set_held_notes: the note keys held now */
    uint8_t held_step;   /* MELLO_UI_NO_STEP when none */
    uint8_t held_key;    /* its note key */
    bool held_step_edited;
    /* SAVE: a short press saves the project, holding it opens SLOTS (on SLOTS a press saves the slot). */
    uint32_t save_down_ms;
    bool save_down;
    uint8_t seen_save_state;                      /* draw_status's save word, to redraw it when it changes */
    uint32_t enc_last_ms[MELLO_UI_ENCODER_COUNT]; /* each encoder's last detent, for acceleration */
    /* KEYS (EDIT on HOME): the 27 keys are a keyboard. Playing: a key is the new root. Stopped: it plays the lead
     * while held; with REC on, it writes the next step (keys_rec_pos) and moves on. */
    bool keys_mode;
    bool keys_rec;
    uint8_t keys_rec_pos;
    uint8_t audition_key; /* the key playing the lead, 0xFF none */
    uint8_t audition_note;
    bool audition_release;
    int8_t keys_octave; /* KEYS: OCT-/OCT+ move the keyboard, -2..+2 */
    bool rndmzr_down;   /* RNDMZR pressed: a tap rolls, a hold opens RANDOM */
    uint8_t rndmzr_key;
    uint32_t rndmzr_down_ms;
    MelloRandomizeIntensity random_intensity; /* runtime creative depth; default MED */

    uint8_t selected_step;
    uint8_t step_bank;
    uint8_t selected_segment;
    uint8_t selected_loop_step;
    uint8_t selected_slot_seq_step;
    uint8_t slot_seq_bank;
    uint8_t selected_slot;
    uint8_t slot_bank;
    uint8_t selected_dot_target; /* the matrix (MelloDotTarget) */
    bool dot_shape;              /* a MOD lane's SHAPE layer */
    uint8_t tm_bank;
    uint8_t selected_tm_bit;
    uint8_t selected_sub_x_step;
#if MELLO_FEATURE_CHORD_MACHINE
    uint8_t selected_chord_step;
    uint8_t selected_chord_pattern;
#endif
    uint8_t slot_name_cursor;
    MelloSampleBank *sample_bank;

    uint32_t tap_times_ms[8];
    uint8_t tap_count;
    bool reset_armed;
    uint32_t reset_confirm_until_ms;

    uint16_t dirty;
    uint32_t last_key_led_mask;
    uint32_t last_button_led_mask;

    uint8_t seen_step;
    uint8_t seen_pulse;
    uint8_t seen_segment;
    uint8_t seen_loop_pos;
    uint8_t seen_slot_seq_pos;
    int8_t seen_tm_pos;
    int8_t seen_dot_cell; /* the shown DOT matrix's read position, for its playhead */
    int8_t dot_trail[2];  /* its last two positions, drawn fading */
    int8_t seen_sub_x_pos;
#if MELLO_FEATURE_CHORD_MACHINE
    uint8_t seen_chord_pos;
    int8_t seen_chord_grid;
#endif
    uint8_t seen_running;
    uint8_t home_old_step;
    uint8_t home_new_step;
} MelloUi;

void mello_ui_init(MelloUi *ui, MelloTransport *transport, MelloStorage *storage);
void mello_ui_attach_sample_bank(MelloUi *ui, MelloSampleBank *bank);
void mello_ui_set_page(MelloUi *ui, MelloUiPage page);
void mello_ui_tick(MelloUi *ui);

/* One non-blocking main-loop input pass. note_pressed is bit 0=F3 .. bit 26=G5.
 * held_buttons is the current level bitmask, while pressed/released are edge masks. */
/* The note keys held down now (fm1_in.notes), before mello_ui_input each round: HOME's hold-a-step needs the
 * releases, which the press edges do not carry. */
void mello_ui_set_held_notes(MelloUi *ui, uint32_t notes);
void mello_ui_input(MelloUi *ui, uint32_t now_us, uint32_t now_ms, uint32_t pressed_buttons,
                    uint32_t released_buttons, uint32_t held_buttons, uint32_t note_pressed,
                    const int16_t encoder_delta[MELLO_UI_ENCODER_COUNT]);

void mello_ui_render(MelloUi *ui, const MelloUiDrawOps *ops);
void mello_ui_sync_leds(MelloUi *ui, const MelloUiDrawOps *ops);
uint16_t mello_ui_dirty(const MelloUi *ui);
const char *mello_ui_page_name(MelloUiPage page);

/* Mapping helpers are public for deterministic host tests. */
int8_t mello_ui_white_key_position(uint8_t note_index);
uint8_t mello_ui_white_key_note_index(uint8_t white_position);

#ifdef __cplusplus
}
#endif
#endif
