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

    puts("PASS: HOME macros, hold-a-step edit, tap toggles on release, keys past the length inert, step LEDs");
    return 0;
}
