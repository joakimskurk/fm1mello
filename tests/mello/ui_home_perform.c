/* Phase 3, PERFORM HOME (docs/mello/UI_ICEBERG.md): K1-K4 are CUTOFF / DECAY / SPACE / DRIVE; a white key held down
 * holds its step for K1-K4 and, let go without a turn, switches the step on/off; keys past the length do nothing;
 * the step LEDs show the steps that are on, with the playhead flipping its key. */
#include <stdio.h>
#include <string.h>

#include "mello_ui.h"

static uint32_t leds;
static void fill(void *c, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t col)
{
    (void)c, (void)x, (void)y, (void)w, (void)h, (void)col;
}
static void text(void *c, int16_t x, int16_t y, int16_t w, uint8_t l, const char *s, uint16_t col, uint8_t a)
{
    (void)c, (void)x, (void)y, (void)w, (void)l, (void)s, (void)col, (void)a;
}
static void key_led(void *c, uint8_t key, bool on)
{
    (void)c;
    if (on) leds |= 1u << key;
}
static void btn_led(void *c, uint8_t b, bool on)
{
    (void)c, (void)b, (void)on;
}
static uint8_t white(uint8_t pos) /* the note key of white key W(pos+1) */
{
    uint8_t i;
    for (i = 0u; i < 27u; ++i)
        if (mello_ui_white_key_position(i) == (int8_t)pos) return i;
    return 0u;
}
static void input(MelloUi *u, uint32_t held, uint32_t pressed, uint8_t knob, int16_t d)
{
    int16_t e[MELLO_UI_ENCODER_COUNT] = {0};
    e[MELLO_UI_ENC_KNOB1 + knob] = d;
    mello_ui_set_held_notes(u, held);
    mello_ui_input(u, 0u, 0u, 0u, 0u, 0u, pressed, e);
}
static MelloEvent sunk[8];
static unsigned sunk_n;
static void sink(void *ctx, const MelloEvent *ev, uint32_t due)
{
    (void)ctx, (void)due;
    if (sunk_n < 8u) sunk[sunk_n++] = *ev;
}
static int fail(const char *w)
{
    fprintf(stderr, "FAIL: %s\n", w);
    return 1;
}

int main(void)
{
    static MelloProject p;
    static MelloTransport t;
    static MelloStorage st;
    static MelloUi u;
    MelloUiDrawOps o = {fill, text, key_led, btn_led, 0};
    uint8_t k3 = white(2u), k16 = white(15u);
    uint16_t cut, decay, rel;

    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 1u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_HOME);

    /* Macros: no key held. */
    cut = t.engine.sound.filter_cut;
    decay = t.engine.sound.decay_ms;
    rel = t.engine.sound.release_ms;
    input(&u, 0u, 0u, 0u, 3);
    if (t.engine.sound.filter_cut != cut + 30u) return fail("K1 is not CUTOFF");
    input(&u, 0u, 0u, 1u, 4);
    if (t.engine.sound.decay_ms <= decay || t.engine.sound.release_ms <= rel) return fail("K2 is not DECAY");
    input(&u, 0u, 0u, 2u, 6);
    if (t.engine.fx.reverb_wet != 300u || t.engine.fx.delay_wet != 150u) return fail("K3 is not SPACE");
    input(&u, 0u, 0u, 3u, 5);
    if (t.engine.fx.dist_amount != 50u) return fail("K4 is not DRIVE");

    /* A tap on W3: the step goes off on release, on again on the next tap. */
    input(&u, 1u << k3, 1u << k3, 0u, 0);
    if (t.engine.project.steps[2].skip) return fail("the press itself toggled");
    input(&u, 0u, 0u, 0u, 0);
    if (!t.engine.project.steps[2].skip) return fail("a tap did not switch the step off");
    input(&u, 1u << k3, 1u << k3, 0u, 0);
    input(&u, 0u, 0u, 0u, 0);
    if (t.engine.project.steps[2].skip) return fail("a second tap did not switch it on");

    /* Hold W3 and turn K1: the step's pitch, not the cutoff; no toggle on release. */
    cut = t.engine.sound.filter_cut;
    {
        int8_t pitch = t.engine.project.steps[2].pitch;
        input(&u, 1u << k3, 1u << k3, 0u, 0);
        input(&u, 1u << k3, 0u, 0u, 1);
        if (t.engine.project.steps[2].pitch == pitch || t.engine.sound.filter_cut != cut)
            return fail("hold + K1 did not edit the step's pitch");
        input(&u, 0u, 0u, 0u, 0);
        if (t.engine.project.steps[2].skip) return fail("an edit toggled the step on release");
        input(&u, 0u, 0u, 0u, 1);
        if (t.engine.sound.filter_cut == cut) return fail("K1 is not CUTOFF again after the release");
    }

    /* W16 past an 8-step sequence: nothing, and the length stays. */
    t.engine.project.seq_len = 8u;
    input(&u, 1u << k16, 1u << k16, 0u, 0);
    input(&u, 0u, 0u, 0u, 0);
    if (t.engine.project.seq_len != 8u || u.held_step != MELLO_UI_NO_STEP) return fail("a key past the length acted");

    /* LEDs: steps 1-8 on except W3 off; running at step 5 flips W5. */
    t.engine.project.steps[2].skip = true;
    t.running = true;
    t.display_step = 4u;
    leds = 0u;
    u.last_key_led_mask = 0u; /* sync reports changes only: from all-off, every lit key is reported */
    u.dirty |= MELLO_UI_DIRTY_LEDS;
    mello_ui_sync_leds(&u, &o);
    if (!(leds & (1u << white(0u))) || (leds & (1u << k3)) || (leds & (1u << white(4u))) ||
        (leds & (1u << white(8u))))
        return fail("step LEDs");

    /* The rail: RND then UNDO gives back the pattern, UNDO again redoes; MUT moves pitches only. */
    t.running = false;
    {
        static MelloProject before, rolled;
        uint8_t b0 = 0u, b1 = 0u, b2 = 0u, i, n = 0u;
        for (i = 0u; i < 27u; ++i)
            if (mello_ui_white_key_position(i) < 0) {
                if (n == 0u) b0 = i;
                if (n == 1u) b1 = i;
                if (n == 2u) b2 = i;
                ++n;
            }
        before = t.engine.project;
        input(&u, 0u, 1u << b0, 0u, 0);
        rolled = t.engine.project;
        if (!memcmp(&before.steps, &rolled.steps, sizeof before.steps)) return fail("RND changed nothing");
        input(&u, 0u, 1u << b1, 0u, 0);
        if (memcmp(&before.steps, &t.engine.project.steps, sizeof before.steps)) return fail("UNDO did not restore");
        input(&u, 0u, 1u << b1, 0u, 0);
        if (memcmp(&rolled.steps, &t.engine.project.steps, sizeof rolled.steps)) return fail("UNDO again did not redo");
        before = t.engine.project;
        input(&u, 0u, 1u << b2, 0u, 0);
        for (i = 0u; i < MELLO_MAX_STEPS; ++i)
            if (before.steps[i].pulses != t.engine.project.steps[i].pulses ||
                before.steps[i].skip != t.engine.project.steps[i].skip)
                return fail("MUT touched more than pitch");
    }

    /* The encoders: SELECT = tempo, ALGORITHM = intensity, PRESETS = the next saved slot. */
    {
        int16_t e[MELLO_UI_ENCODER_COUNT] = {0};
        uint16_t bpm = t.engine.project.bpm;
        e[MELLO_UI_ENC_SELECT] = 5;
        mello_ui_input(&u, 0u, 0u, 0u, 0u, 0u, 0u, e);
        u.dirty = 0u;
        mello_ui_input(&u, 0u, 0u, 0u, 0u, 0u, 0u, e);
        if (t.engine.project.bpm != bpm + 10u || u.readout != MELLO_UI_READOUT_TEMPO) return fail("SELECT is not TEMPO");
        if (!(u.dirty & MELLO_UI_DIRTY_BODY) || !(u.dirty & MELLO_UI_DIRTY_HEADER))
            return fail("TEMPO is not redrawn (readout, header)");
        memset(e, 0, sizeof e);
        e[MELLO_UI_ENC_ALGORITHM] = 1;
        u.random_intensity = MELLO_RANDOMIZE_MED;
        mello_ui_input(&u, 0u, 0u, 0u, 0u, 0u, 0u, e);
        if (u.random_intensity != MELLO_RANDOMIZE_HIGH) return fail("ALGORITHM is not RANDOM intensity");
        (void)mello_engine_save_slot(&t.engine, 4u, "FIVE");
        u.selected_slot = 0u;
        memset(e, 0, sizeof e);
        e[MELLO_UI_ENC_PRESETS] = 1;
        mello_ui_input(&u, 0u, 0u, 0u, 0u, 0u, 0u, e);
        if (u.selected_slot != 4u || u.readout != MELLO_UI_READOUT_PATTERN) return fail("PRESETS did not find slot 5");
    }

    /* SAVE: short = save the project, held 0.7 s = SLOTS. */
    {
        int16_t e[MELLO_UI_ENCODER_COUNT] = {0};
        uint32_t save = 1u << MELLO_UI_BTN_SAVE;
        mello_ui_set_page(&u, MELLO_UI_HOME);
        st.manual_pending = false;
        mello_ui_input(&u, 0u, 5000u, save, 0u, save, 0u, e);
        mello_ui_input(&u, 0u, 5200u, 0u, save, 0u, 0u, e);
        if (!st.manual_pending || u.page != MELLO_UI_HOME) return fail("a short SAVE did not save");
        st.manual_pending = false;
        mello_ui_input(&u, 0u, 6000u, save, 0u, save, 0u, e);
        mello_ui_input(&u, 0u, 6400u, 0u, 0u, save, 0u, e);
        if (u.page != MELLO_UI_HOME) return fail("SAVE opened SLOTS too soon");
        mello_ui_input(&u, 0u, 6750u, 0u, 0u, save, 0u, e);
        if (u.page != MELLO_UI_SLOTS || st.manual_pending) return fail("a held SAVE did not open SLOTS");
        mello_ui_input(&u, 0u, 6900u, 0u, save, 0u, 0u, e);
        if (st.manual_pending) return fail("letting go after the hold saved");
    }

    /* Acceleration: a detent 10 ms after the last counts x4, one 500 ms later x1. Switches follow direction. */
    {
        int16_t e[MELLO_UI_ENCODER_COUNT] = {0};
        uint16_t c0;
        mello_ui_set_page(&u, MELLO_UI_HOME);
        e[MELLO_UI_ENC_KNOB1] = -1;
        mello_ui_input(&u, 0u, 20000u, 0u, 0u, 0u, 0u, e);
        c0 = t.engine.sound.filter_cut;
        mello_ui_input(&u, 0u, 20010u, 0u, 0u, 0u, 0u, e);
        if (t.engine.sound.filter_cut != c0 - 40u) return fail("a fast detent did not count x4");
        mello_ui_input(&u, 0u, 20510u, 0u, 0u, 0u, 0u, e);
        if (t.engine.sound.filter_cut != c0 - 50u) return fail("a slow detent did not count x1");
        mello_ui_set_page(&u, MELLO_UI_SOUND_RESERVED);
        u.sound_subpage = 3u; /* FOLLOWER: K1 = on/off */
        memset(e, 0, sizeof e);
        e[MELLO_UI_ENC_KNOB1] = 1;
        mello_ui_input(&u, 0u, 21000u, 0u, 0u, 0u, 0u, e);
        mello_ui_input(&u, 0u, 21500u, 0u, 0u, 0u, 0u, e);
        if (!t.engine.sound.follower_sub_on) return fail("right twice did not leave the switch on");
        e[MELLO_UI_ENC_KNOB1] = -1;
        mello_ui_input(&u, 0u, 22000u, 0u, 0u, 0u, 0u, e);
        if (t.engine.sound.follower_sub_on) return fail("left did not switch it off");
    }

    /* KEYS (EDIT): stopped, a key plays the lead while held; REC writes steps; playing, a key is the new root. */
    {
        int16_t e[MELLO_UI_ENCODER_COUNT] = {0};
        uint32_t edit = 1u << MELLO_UI_BTN_EDIT, rec = 1u << MELLO_UI_BTN_REC, key = 1u << 7; /* C4 = 53 + 7 */
        mello_ui_set_page(&u, MELLO_UI_HOME);
        t.running = false;
        mello_transport_set_event_sink(&t, sink, 0);
        mello_ui_input(&u, 0u, 30000u, edit, 0u, edit, 0u, e);
        if (!u.keys_mode) return fail("EDIT on HOME did not enter KEYS");
        sunk_n = 0u;
        mello_ui_set_held_notes(&u, key);
        mello_ui_input(&u, 0u, 30100u, 0u, 0u, 0u, key, e);
        mello_ui_set_held_notes(&u, 0u);
        mello_ui_input(&u, 0u, 30150u, 0u, 0u, 0u, 0u, e);
        if (sunk_n != 2u || sunk[0].type != MELLO_EVENT_NOTE_ON || sunk[0].note != 60u ||
            sunk[1].type != MELLO_EVENT_NOTE_OFF || sunk[1].note != 60u)
            return fail("a key in KEYS did not play C4 while held");
        mello_ui_input(&u, 0u, 30200u, rec, 0u, rec, 0u, e);
        if (!u.keys_rec) return fail("REC in KEYS did not start step-record");
        mello_ui_set_held_notes(&u, 1u << 10); /* D#4 = 63: a note of C minor */
        mello_ui_input(&u, 0u, 30300u, 0u, 0u, 0u, 1u << 10, e);
        mello_ui_set_held_notes(&u, 0u);
        if (mello_resolve_step_midi(&t.engine.project, &t.engine.project.steps[0]) != 63u || u.keys_rec_pos != 1u)
            return fail("step-record did not write D#4 to step 1");
        mello_ui_input(&u, 0u, 30400u, rec, 0u, rec, 0u, e);
        t.running = true;
        mello_ui_set_held_notes(&u, 1u << 2); /* G3 = 55 */
        mello_ui_input(&u, 0u, 30500u, 0u, 0u, 0u, 1u << 2, e);
        mello_ui_set_held_notes(&u, 0u);
        if (t.engine.project.root_note != 55u) return fail("a key while playing did not set the root");
        mello_ui_set_page(&u, MELLO_UI_SOUND_RESERVED);
        if (u.keys_mode) return fail("KEYS outlived HOME");
        t.running = false;
    }

    puts("PASS: HOME macros, hold-a-step edit, tap toggles on release, keys past the length inert, step LEDs, "
         "RND/UNDO/redo/MUT, TEMPO/intensity/PATTERN, SAVE short/held, acceleration, direction switches, KEYS play/step-record/root");
    return 0;
}
