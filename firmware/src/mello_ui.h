/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
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
    MELLO_UI_SAMPLE_RESERVED,
    MELLO_UI_DRUMS,
    MELLO_UI_MIX /* the last page */
} MelloUiPage;

#define MELLO_UI_NO_STEP           0xFFu
#define MELLO_UI_TEXT_SPACED       2u /* text style: small, letter-spaced (text_on) */
#define MELLO_UI_READOUT_LAYER     5u /* MelloUi.readout: SELECT changed the layer */
#define MELLO_UI_READOUT_TEMPO     6u /* HOME's SELECT, ALGORITHM, PRESETS */
#define MELLO_UI_READOUT_INTENSITY 7u
#define MELLO_UI_READOUT_PATTERN   8u
#define MELLO_UI_READOUT_STYLE     9u    /* the style RNDMZR just rolled */
#define MELLO_UI_READOUT_KEPT      10u   /* RNDMZR + SAVE: kept in a slot */
#define MELLO_UI_READOUT_TAKE      11u   /* a take ended: locked (and the chords fitted) */
#define MELLO_UI_READOUT_INIT      12u   /* CLR held: a clean slate */
#define MELLO_UI_READOUT_UNLOCK    13u   /* UNLOCK (B7 while a part is locked): the parts unlocked */
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
    /* G5: a key's glow, the middle of the lights' three levels (lit, glow, dark); optional */
    void (*key_glow)(void *ctx, uint8_t key_index, bool on);
} MelloUiDrawOps;

typedef struct {
    MelloTransport *transport;
    MelloStorage *storage;
    MelloUiPage page;
    uint8_t drum_subpage; /* DRUMS: VOICE, STEP, VARY, KIT */
    uint8_t selected_drum_voice;
    uint8_t selected_drum_step;
    int8_t seen_drum_step;
    uint8_t seen_roll_wait; /* a roll waited for the one: when it lands, everything drawn again */
    int8_t seen_bass_step;

    uint8_t step_subpage;    /* TRIG, NOTE, FLAGS, SEQ */
    uint8_t segment_subpage; /* always 0: SEGMENT has one layer */
    uint8_t loop_subpage;    /* STEP, HARM, LOOP */
    uint8_t slots_subpage;   /* LOAD, NAME */
    uint8_t tm_subpage;      /* MAIN, ROUTES, DEPTH */
    uint8_t sub_subpage;     /* MAIN, 5-SEQ CFG, 5-SEQ STEP */
#if MELLO_FEATURE_CHORD_MACHINE
    uint8_t chord_subpage; /* MAIN, CHAIN, VOICE, GRID, PATTERN */
#endif
    uint8_t global_subpage; /* CLOCK, MIDI */
    uint8_t mix_subpage;    /* LEVEL, DELAY, REVERB: each part's level, and its sends */
    uint8_t sound_subpage;  /* OSC, ENV, LFO, FOLLOW, PLAY, GRIT, OSC 2, CHARACTER, MOD ENV */
    uint8_t fx_subpage;     /* DIST; DELAY SEND, CONFIG, LOOP, ROUTE; REVERB ROOM, RETURNS, OLIVERB */
    uint8_t sample_subpage; /* MAIN, PITCH, TRIM, ENV */
    /* The readout: what the last turned control moves, large over the body, until readout_until_ms. */
    uint8_t readout; /* 0 = none, 1..4 = K1..K4, MELLO_UI_READOUT_LAYER = SELECT changed the layer */
    uint32_t readout_until_ms;
    uint8_t unlocked; /* the parts UNLOCK unlocked last (MELLO_PART_*), for its readout */
    /* HOME: a white key held down holds its step (K1-K4 edit it); released without a turn, it toggles the step. */
    uint32_t held_notes;  /* mello_ui_set_held_notes: the note keys held now */
    uint16_t audio_load;  /* mello_ui_set_audio_load: the chip's audio load, permille (GLOBAL shows it) */
    uint16_t audio_load2; /* mello_ui_set_audio_load2: the second core's, its jobs' share (SYSTEM's CPU 2) */
    uint8_t cores; /* mello_ui_set_cores: the second core now (MELLO_UI_CORE_*; GLOBAL > SYSTEM shows it) */
    bool
        second_core; /* GLOBAL > SYSTEM's CORE 2: the switch; the platform saves it, the next power-on uses it */
    uint16_t held_black; /* HOME: the black keys held now (bit = B0..B10), for the strip's names */
    /* PERFORM (G4, ROADMAP F4): B5 PERF tapped opens it until PERF or a page button (1), held while held (2); the
     * white keys are then 16 performance actions on every part */
    /* F14: the audio's modulation now, and what of it the value rows last drew */
    const int16_t *mod_view;
    int16_t mod_drawn[4];
    uint16_t mod_ticks; /* mello_ui_tick's calls since the last look (the main loop's: one a millisecond) */
    uint8_t perform;
    uint32_t perform_down_ms;
    uint16_t perform_was; /* its white keys held at the last input, to redraw them when that changes */
    uint8_t
        perform_mutes; /* the mutes and the mutes asked for, as drawn: the engine turns them on the beat */
    uint8_t perf_repeat_len; /* REPEAT's length in pulses (a white key while REPEAT is held); 0 = 4 */
    uint8_t held_step;       /* MELLO_UI_NO_STEP when none */
    uint8_t held_key;        /* its note key */
    bool held_step_edited;
    /* SAVE: a short press saves the project, holding it opens SLOTS (on SLOTS a press saves the slot). */
    uint32_t save_down_ms;
    bool save_down;
    uint8_t seen_save_state;                      /* draw_status's save word, to redraw it when it changes */
    uint32_t enc_last_ms[MELLO_UI_ENCODER_COUNT]; /* each encoder's last detent, for acceleration */
    /* KEYS (EDIT on HOME): the 27 keys are a keyboard. Playing: a key is the new root. Stopped: it plays the lead
     * while held; with REC on, it writes the next step (keys_rec_pos) and moves on. */
    bool keys_mode;
    /* MACROS (EDIT held on HOME): K1-K4 pick what they turn (MelloMacroConfig); a tap of EDIT leaves. EDIT's tap
     * (KEYS) waits for the release, like SAVE's. */
    bool macro_mode;
    bool edit_down;
    uint32_t edit_down_ms;
    bool step_pitch_mode; /* STEP's EDIT: the white keys set the picked step's pitch (R6) */
    bool keys_rec;
    bool
        rec_keys; /* the take on HOME was started by REC from outside KEYS: REC round to off leaves KEYS again */
    /* REC on a page with a sequence (G3, ROADMAP F5): STEP, BASS, CHORD or DRUMS, the page it records (0: none);
     * keys_rec and keys_rec_target then say what, as HOME's KEYS > REC does */
    uint8_t page_rec;
    uint8_t drum_rec_voice, drum_rec_step; /* REC on DRUMS: the last hit, for the lock keys */
    uint8_t audition_part; /* what a key sounds while recording: 0 the lead, 1 the bass, 2 chords */
    uint8_t
        keys_rec_target; /* REC in KEYS cycles what the keys write: 1 SEQ, 2 BASS, 3 CHORD (MelloUiRecTarget) */
    uint8_t keys_rec_pos; /* the next step (or chord slot) written, stopped; playing, the keys write where the
                                clock is */
    uint8_t keys_rec_last;      /* the slot the last chord went to, for a second key held: its shape */
    uint8_t keys_rec_last_midi; /* and the key that wrote it */
    /* The key that wrote the last step, while it stays down: stopped, held long it makes that step a held note (SEQ:
     * gate HOLD) or a glide (BASS); playing, the clock reaching the next step carries the note on to it. */
    uint8_t rec_key; /* 0xFF none */
    uint8_t rec_key_pos;
    uint32_t rec_key_ms;
    bool rec_key_long;
    uint8_t audition_key; /* the key playing the lead, 0xFF none */
    uint8_t audition_note;
    bool audition_release;
    int8_t keys_octave; /* KEYS: OCT-/OCT+ move the keyboard, -2..+2 */
    bool rndmzr_down;   /* RNDMZR pressed: a tap rolls, a hold opens RANDOM */
    bool rndmzr_part;   /* a white key rolled one part while it was held: nothing more on letting go */
    bool rndmzr_menu;   /* held a moment: the parts the white keys roll are shown */
    bool rndmzr_ready;  /* held half a second, no key: letting go opens RANDOM (the line says so, lit) */
    uint8_t sect_btn; /* a section button held (0xFF none): a moment, and its pages shown on the white keys */
    bool sect_menu;
    uint32_t sect_down_ms;
    bool undo_down; /* UNDO held: white keys 1-3 go back to before the last three rolls */
    bool
        clr_down; /* CLR held: a second, INIT (a clean slate); let go sooner, CLR as before (twice to clear) */
    bool clr_done;
    uint32_t clr_down_ms;
    bool
        rec_down; /* REC pressed: a tap starts or stops a take (or goes on to the next target), a hold of half a
                         * second let go starts it from empty, white keys 1-4 meanwhile pick SEQ, BASS, CHORD, DRUMS */
    bool rec_menu, rec_used, rec_click_off, take_fit, rec_ready;
    uint8_t rec_wrote; /* the take's targets that were written, bit = MELLO_UI_REC_* */
    /* Step entry (REC stopped on HOME, DECISIONS "Step entry"): what the next step gets, kept from step to step:
     * SEQ's PULSE, RATCH, GATE, ACC (bit 0) and SLD (bit 1); BASS's GLIDE and ACCENT; CHORD's HOLD and SHAPE */
    uint8_t se_pulses, se_ratchet, se_gate, se_flags, se_bass, se_hold, se_shape;
    /* PERFORM's black keys (DECISIONS "PERFORM's black keys"): each effect's amount (K1 while it is held), and the
     * one held last, which K1 turns */
    uint16_t perf_amt[8];
    uint8_t perf_black_last;
    uint32_t rec_down_ms, rec_now_ms;
    bool undo_menu; /* held a moment: those shown */
    uint32_t undo_down_ms;
    int8_t kept_slot; /* RNDMZR + SAVE: the slot the band was kept in (-1: none free) */
    uint8_t rndmzr_key;
    uint32_t rndmzr_down_ms;
    MelloRandomizeIntensity random_intensity; /* runtime creative depth; default MED */
    uint8_t
        random_style; /* MelloStyle: RNDMZR rolls the band in it (ANY: a style each time; OFF: the lead, by INT) */
    bool roll_playing;  /* RANDOM's ROLLS: PLAYING, a roll of the band leaves the parts that are off off */
    bool random_links;  /* RANDOM's LINKS layer (SELECT): keys set + / - per target */
    bool random_edit;   /* RANDOM's EDIT: a key picks a target, K1 its amount */
    uint8_t random_sel; /* the target EDIT sets (index into the RANDOM page's pills) */

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
    uint8_t drum_bank; /* DRUMS: steps 1-16 or 17-32 (OCT- / OCT+) */
    uint8_t selected_tm_bit;
    uint8_t selected_sub_x_step; /* BASS SEQ's picked step (it was 5-SEQ's) */
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
    uint32_t last_key_glow_mask; /* G5: the keys that glow (fm1_led_dim) */
    uint32_t last_button_led_mask;
    bool
        leds_sent; /* the LEDs were synced since init: until then the masks above say nothing, and the first sync
                     * sends every LED, as the panel may show anything (all off at boot, the old page after RESET ALL) */

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
    uint8_t seen_chord_lit;
#endif
    uint8_t seen_running;
    uint8_t home_old_step;
    uint8_t home_new_step;
} MelloUi;

typedef enum {
    MELLO_UI_REC_SEQ = 1,
    MELLO_UI_REC_BASS = 2,
    MELLO_UI_REC_CHORD = 3,
    MELLO_UI_REC_DRUMS = 4 /* REC on DRUMS (G3): white keys 1-4 the voices, 5-8 the last hit's locks */
} MelloUiRecTarget;

void mello_ui_init(MelloUi *ui, MelloTransport *transport, MelloStorage *storage);
void mello_ui_attach_sample_bank(MelloUi *ui, MelloSampleBank *bank);
void mello_ui_set_page(MelloUi *ui, MelloUiPage page);
void mello_ui_tick(MelloUi *ui);

/* One non-blocking main-loop input pass. note_pressed is bit 0=F3 .. bit 26=G5.
 * held_buttons is the current level bitmask, while pressed/released are edge masks. */
/* The note keys held down now (fm1_in.notes), before mello_ui_input each round: HOME's hold-a-step needs the
 * releases, which the press edges do not carry. */
void mello_ui_set_held_notes(MelloUi *ui, uint32_t notes);
/* The chip's audio load, permille of real time the audio interrupt takes (dinghy.c measures it; ROADMAP B1): the
 * GLOBAL page shows it as CPU. 0 = unknown (the host). */
void mello_ui_set_audio_load(MelloUi *ui, uint16_t permille);
/* The second core's share, as the render's: the worst block of each second its jobs took, permille (0: none). */
void mello_ui_set_audio_load2(MelloUi *ui, uint16_t permille);
/* The second core (dinghy.c from the platform's cpu1_state, DECISIONS "The second core"): what runs now, and the
 * CORE 2 switch as saved. GLOBAL > SYSTEM shows both; K1 turns the switch (ui->second_core), which dinghy.c saves
 * for the next power-on. The host keeps OFF, one core. */
enum { MELLO_UI_CORE_OFF, MELLO_UI_CORE_NO_REPLY, MELLO_UI_CORE_GAVE_UP, MELLO_UI_CORE_ON };
void mello_ui_set_cores(MelloUi *ui, uint8_t cores, bool second_core);
/* What modulation does to each MelloModDest now (MelloAudio.mod_view): the value rows' second thumbs (ROADMAP F14). */
void mello_ui_set_mod_view(MelloUi *ui, const int16_t *view);
void mello_ui_input(MelloUi *ui, uint32_t now_us, uint32_t now_ms, uint32_t pressed_buttons,
                    uint32_t released_buttons, uint32_t held_buttons, uint32_t note_pressed,
                    const int16_t encoder_delta[MELLO_UI_ENCODER_COUNT]);

void mello_ui_render(MelloUi *ui, const MelloUiDrawOps *ops);
/* The splash, while the FM-1 starts (storage, samples): the wordmark over a sixteen-step motif, the version under it. */
void mello_ui_draw_splash(const MelloUiDrawOps *ops, const char *version);
/* Its skyline playing, from_ms to to_ms after the still splash: the bars that moved between the two are redrawn. */
void mello_ui_splash_frame(const MelloUiDrawOps *ops, uint32_t from_ms, uint32_t to_ms);
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
