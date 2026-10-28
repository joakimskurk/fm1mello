#include "mello_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    REC_NAV_SAMPLE = 0,
    REC_RANDOMIZE,
    REC_TAP,
    REC_LOOP_RESET,
    REC_DOT_REROLL,
    REC_TURING_REROLL,
    REC_TOGGLE_SUB,
    REC_TOGGLE_CHORD,
    REC_TOGGLE_SAMPLE
} RecAction;

typedef enum {
    EDIT_NAV_RANDOM = 0,
    EDIT_NONE,
    EDIT_DOT_RESET,
    EDIT_TURING_LOCK,
    EDIT_SAMPLE_PER_STEP
} EditAction;

typedef struct {
    const char *name;
    MelloUiPage page;
    RecAction rec;
    EditAction edit;
    uint8_t layers;
    uint8_t edit_violation;
} PageAudit;

typedef struct {
    uint8_t step;
    uint8_t segment;
    uint8_t loop_step;
    uint8_t slot_seq_step;
    uint8_t slot;
    uint8_t tm_bit;
    uint8_t sub_step;
#if MELLO_FEATURE_CHORD_MACHINE
    uint8_t chord_step;
    uint8_t chord_pattern;
#endif
    uint8_t sample_slot;
} SelectionSnapshot;

#define ROW(n,p,r,e,l,v) {n,p,r,e,l,v}
static const PageAudit pages[] = {
    ROW("HOME",       MELLO_UI_HOME,            REC_NAV_SAMPLE,    EDIT_NAV_RANDOM,       1u, 1u),
    ROW("STEP",       MELLO_UI_SEQ_STEP,        REC_NAV_SAMPLE,    EDIT_NAV_RANDOM,       4u, 1u),
    ROW("SEGMENT",    MELLO_UI_SEQ_SEGMENT,     REC_NAV_SAMPLE,    EDIT_NAV_RANDOM,       2u, 1u),
    ROW("LOOP",       MELLO_UI_SEQ_LOOP,        REC_LOOP_RESET,    EDIT_NAV_RANDOM,       3u, 1u),
    ROW("SLOTSEQ",    MELLO_UI_SEQ_SLOTSEQ,     REC_NAV_SAMPLE,    EDIT_NAV_RANDOM,       1u, 1u),
    ROW("RANDOM",     MELLO_UI_RANDOM,          REC_RANDOMIZE,     EDIT_NONE,             1u, 0u),
    ROW("KEY/SCALE",  MELLO_UI_KEY_SCALE,       REC_NAV_SAMPLE,    EDIT_NAV_RANDOM,       1u, 1u),
    ROW("SLOTS",      MELLO_UI_SLOTS,           REC_NAV_SAMPLE,    EDIT_NAV_RANDOM,       2u, 1u),
    ROW("DOT",        MELLO_UI_DOT,             REC_DOT_REROLL,    EDIT_DOT_RESET,        4u, 1u),
    ROW("TURING",     MELLO_UI_TURING,          REC_TURING_REROLL, EDIT_TURING_LOCK,      3u, 0u),
    ROW("SUB",        MELLO_UI_SUB,             REC_TOGGLE_SUB,    EDIT_NAV_RANDOM,       5u, 1u),
#if MELLO_FEATURE_CHORD_MACHINE
    ROW("CHORD",      MELLO_UI_CHORD,           REC_TOGGLE_CHORD,  EDIT_NAV_RANDOM,       5u, 1u),
#endif
    ROW("GLOBAL",     MELLO_UI_GLOBAL,          REC_TAP,           EDIT_NAV_RANDOM,       2u, 1u),
    ROW("SOUND",      MELLO_UI_SOUND_RESERVED,  REC_NAV_SAMPLE,    EDIT_NAV_RANDOM,       5u, 1u),
    ROW("AUDIO FX",   MELLO_UI_AUDIO_FX,        REC_NAV_SAMPLE,    EDIT_NAV_RANDOM,       5u, 1u),
    ROW("SAMPLE",     MELLO_UI_SAMPLE_RESERVED, REC_TOGGLE_SAMPLE, EDIT_SAMPLE_PER_STEP,  4u, 0u),
};

static uint32_t bit_u32(unsigned n) { return 1u << n; }

static void fail(const char *page, const char *what)
{
    fprintf(stderr, "FAIL: %s: %s\n", page, what);
    exit(1);
}

static void setup(MelloProject *p, MelloTransport *t, MelloStorage *st, MelloUi *ui, MelloUiPage page)
{
    mello_project_defaults(p);
    mello_transport_init(t, p, 123u, 0, 0);
    mello_storage_init(st, 0, 0);
    mello_ui_init(ui, t, st);
    mello_ui_set_page(ui, page);
}

static void input(MelloUi *ui, uint32_t pressed, uint32_t held, uint32_t note_mask, int enc_id, int delta)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    if (enc_id >= 0 && enc_id < (int)MELLO_UI_ENCODER_COUNT) enc[enc_id] = (int16_t)delta;
    mello_ui_input(ui, 1000000u, 1000u, pressed, 0u, held, note_mask, enc);
}

static void press(MelloUi *ui, uint8_t button, uint32_t held)
{
    input(ui, bit_u32(button), held, 0u, -1, 0);
}

static void turn_select(MelloUi *ui, int delta)
{
    input(ui, 0u, 0u, 0u, MELLO_UI_ENC_SELECT, delta);
}

static uint8_t layer(const MelloUi *ui, MelloUiPage page)
{
    switch (page) {
    case MELLO_UI_SEQ_STEP: return ui->step_subpage;
    case MELLO_UI_SEQ_SEGMENT: return ui->segment_subpage;
    case MELLO_UI_SEQ_LOOP: return ui->loop_subpage;
    case MELLO_UI_SLOTS: return ui->slots_subpage;
    case MELLO_UI_DOT: return ui->selected_dot_target;
    case MELLO_UI_TURING: return ui->tm_subpage;
    case MELLO_UI_SUB: return ui->sub_subpage;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: return ui->chord_subpage;
#endif
    case MELLO_UI_GLOBAL: return ui->global_subpage;
    case MELLO_UI_SOUND_RESERVED: return ui->sound_subpage;
    case MELLO_UI_AUDIO_FX: return ui->fx_subpage;
    case MELLO_UI_SAMPLE_RESERVED: return ui->sample_subpage;
    default: return 0u;
    }
}

static SelectionSnapshot selections(const MelloUi *ui, const MelloEngine *e)
{
    SelectionSnapshot s;
    memset(&s, 0, sizeof s);
    s.step = ui->selected_step;
    s.segment = ui->selected_segment;
    s.loop_step = ui->selected_loop_step;
    s.slot_seq_step = ui->selected_slot_seq_step;
    s.slot = ui->selected_slot;
    s.tm_bit = ui->selected_tm_bit;
    s.sub_step = ui->selected_sub_x_step;
#if MELLO_FEATURE_CHORD_MACHINE
    s.chord_step = ui->selected_chord_step;
    s.chord_pattern = ui->selected_chord_pattern;
#endif
    s.sample_slot = e->sample.edit_slot;
    return s;
}

static void seed_selections(MelloUi *ui, MelloEngine *e)
{
    ui->selected_step = 5u;
    ui->selected_segment = 1u;
    ui->selected_loop_step = 3u;
    ui->selected_slot_seq_step = 4u;
    ui->selected_slot = 2u;
    ui->selected_tm_bit = 6u;
    ui->selected_sub_x_step = 4u;
#if MELLO_FEATURE_CHORD_MACHINE
    ui->selected_chord_step = 2u;
    ui->selected_chord_pattern = 1u;
#endif
    e->sample.edit_slot = 1u;
}

static void verify_rec(const PageAudit *c)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi ui;
    MelloEngine before;
    bool b;

    setup(&p, &t, &st, &ui, c->page);
    before = t.engine;
    switch (c->rec) {
    case REC_NAV_SAMPLE:
        press(&ui, MELLO_UI_BTN_REC, 0u);
        if (ui.page != MELLO_UI_SAMPLE_RESERVED) fail(c->name, "REC no longer navigates SAMPLE in C1 baseline");
        break;
    case REC_RANDOMIZE:
        st.dirty = false;
        press(&ui, MELLO_UI_BTN_REC, 0u);
        if (ui.page != c->page || !st.dirty || !memcmp(&before, &t.engine, sizeof before)) fail(c->name, "REC randomize baseline mismatch");
        break;
    case REC_TAP:
        press(&ui, MELLO_UI_BTN_REC, 0u);
        if (ui.page != c->page || ui.tap_count != 1u) fail(c->name, "REC tap-tempo baseline mismatch");
        break;
    case REC_LOOP_RESET:
        t.engine.loop_cycle_pos = 5u;
        t.engine.loop_cycle_tick = 9u;
        press(&ui, MELLO_UI_BTN_REC, 0u);
        if (ui.page != c->page || t.engine.loop_cycle_pos != 0u || t.engine.loop_cycle_tick != 0u) fail(c->name, "REC loop reset baseline mismatch");
        break;
    case REC_DOT_REROLL:
        before = t.engine;
        press(&ui, MELLO_UI_BTN_REC, 0u);
        if (ui.page != c->page || !memcmp(&before, &t.engine, sizeof before)) fail(c->name, "REC DOT reroll baseline mismatch");
        break;
    case REC_TURING_REROLL:
        before = t.engine;
        press(&ui, MELLO_UI_BTN_REC, 0u);
        if (ui.page != c->page || !memcmp(&before, &t.engine, sizeof before)) fail(c->name, "REC Turing reroll baseline mismatch");
        break;
    case REC_TOGGLE_SUB:
        b = t.engine.sub.on;
        press(&ui, MELLO_UI_BTN_REC, 0u);
        if (ui.page != c->page || t.engine.sub.on == b) fail(c->name, "REC SUB toggle baseline mismatch");
        break;
    case REC_TOGGLE_CHORD:
#if MELLO_FEATURE_CHORD_MACHINE
        b = t.engine.chord.on;
        press(&ui, MELLO_UI_BTN_REC, 0u);
        if (ui.page != c->page || t.engine.chord.on == b) fail(c->name, "REC CHORD toggle baseline mismatch");
#else
        fail(c->name, "unexpected CHORD row in compile-out build");
#endif
        break;
    case REC_TOGGLE_SAMPLE:
        b = t.engine.sample.on;
        press(&ui, MELLO_UI_BTN_REC, 0u);
        if (ui.page != c->page || t.engine.sample.on == b) fail(c->name, "REC SAMPLE toggle baseline mismatch");
        break;
    }
}

static void verify_edit(const PageAudit *c)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi ui;
    bool b;

    setup(&p, &t, &st, &ui, c->page);
    switch (c->edit) {
    case EDIT_NAV_RANDOM:
        press(&ui, MELLO_UI_BTN_EDIT, 0u);
        if (ui.page != MELLO_UI_RANDOM) fail(c->name, "EDIT navigation baseline mismatch");
        break;
    case EDIT_NONE:
        press(&ui, MELLO_UI_BTN_EDIT, 0u);
        if (ui.page != c->page) fail(c->name, "EDIT should be a no-op on RANDOM baseline");
        break;
    case EDIT_DOT_RESET:
        b = t.engine.dot[ui.selected_dot_target].reset;
        press(&ui, MELLO_UI_BTN_EDIT, 0u);
        if (t.engine.dot[ui.selected_dot_target].reset == b) fail(c->name, "EDIT DOT reset baseline mismatch");
        break;
    case EDIT_TURING_LOCK:
        b = t.engine.tm.locked;
        press(&ui, MELLO_UI_BTN_EDIT, 0u);
        if (t.engine.tm.locked == b) fail(c->name, "EDIT Turing lock baseline mismatch");
        break;
    case EDIT_SAMPLE_PER_STEP:
        b = t.engine.sample.per_step;
        press(&ui, MELLO_UI_BTN_EDIT, 0u);
        if (t.engine.sample.per_step == b) fail(c->name, "EDIT SAMPLE per-step baseline mismatch");
        break;
    }
}

static void verify_select(const PageAudit *c)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi ui;
    SelectionSnapshot before, after;
    uint8_t before_layer;

    setup(&p, &t, &st, &ui, c->page);
    seed_selections(&ui, &t.engine);
    before = selections(&ui, &t.engine);
    before_layer = layer(&ui, c->page);
    turn_select(&ui, +1);
    after = selections(&ui, &t.engine);
    if (memcmp(&before, &after, sizeof before)) fail(c->name, "SELECT moved content selection");
    if (c->layers > 1u) {
        if (layer(&ui, c->page) != (uint8_t)((before_layer + 1u) % c->layers)) fail(c->name, "SELECT did not advance function layer");
    } else if (layer(&ui, c->page) != before_layer) {
        fail(c->name, "SELECT changed a page with no function layers");
    }
}

static void verify_black_keys(const PageAudit *c)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi ui;
    MelloEngine before_engine;
    SelectionSnapshot before_sel, after_sel;
    uint8_t before_layer;

    setup(&p, &t, &st, &ui, c->page);
    seed_selections(&ui, &t.engine);
    before_engine = t.engine;
    before_sel = selections(&ui, &t.engine);
    before_layer = layer(&ui, c->page);
    input(&ui, 0u, 0u, bit_u32(10u), -1, 0); /* A# is a physical black key. */
    after_sel = selections(&ui, &t.engine);
    if (ui.page != c->page || layer(&ui, c->page) != before_layer || memcmp(&before_sel, &after_sel, sizeof before_sel) || memcmp(&before_engine, &t.engine, sizeof before_engine)) {
        fail(c->name, "black-key baseline is not empty/context-free");
    }
}

static void verify_known_hidden_debt(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi ui;
    uint8_t note;
    bool b;
    int8_t pitch;

    /* EDIT+white-key currently hides STEP SKIP behind a modifier. */
    setup(&p, &t, &st, &ui, MELLO_UI_SEQ_STEP);
    note = mello_ui_white_key_note_index(3u);
    b = t.engine.project.steps[3].skip;
    input(&ui, 0u, bit_u32(MELLO_UI_BTN_EDIT), bit_u32(note), -1, 0);
    if (t.engine.project.steps[3].skip == b) fail("STEP", "EDIT+key SKIP debt missing from baseline");

    /* EDIT+white-key currently hides SUB step enable behind a modifier. */
    setup(&p, &t, &st, &ui, MELLO_UI_SUB);
    note = mello_ui_white_key_note_index(2u);
    b = t.engine.sub.x_steps[2].on;
    input(&ui, 0u, bit_u32(MELLO_UI_BTN_EDIT), bit_u32(note), -1, 0);
    if (t.engine.sub.x_steps[2].on == b) fail("SUB", "EDIT+key ON debt missing from baseline");

    /* EDIT+REC currently hides destructive CLEAR on RANDOM. */
    setup(&p, &t, &st, &ui, MELLO_UI_RANDOM);
    t.engine.project.steps[0].pitch = 9;
    press(&ui, MELLO_UI_BTN_REC, bit_u32(MELLO_UI_BTN_EDIT));
    if (t.engine.project.steps[0].pitch == 9) fail("RANDOM", "EDIT+REC CLEAR debt missing from baseline");

    /* RANDOM also overloads OCT buttons as ALL/NONE rather than position banks. */
    setup(&p, &t, &st, &ui, MELLO_UI_RANDOM);
    mello_engine_set_randomize_mask(&t.engine, MELLO_RAND_PITCH);
    press(&ui, MELLO_UI_BTN_OCT_DOWN, 0u);
    if (t.engine.randomize.mask != 0u) fail("RANDOM", "OCT DOWN mask-NONE debt missing");
    press(&ui, MELLO_UI_BTN_OCT_UP, 0u);
    if (t.engine.randomize.mask != MELLO_RAND_ALL) fail("RANDOM", "OCT UP mask-ALL debt missing");

    /* GLOBAL factory reset is a hidden destructive OCT chord (arm only here). */
    setup(&p, &t, &st, &ui, MELLO_UI_GLOBAL);
    press(&ui, MELLO_UI_BTN_OCT_DOWN, bit_u32(MELLO_UI_BTN_OCT_DOWN) | bit_u32(MELLO_UI_BTN_OCT_UP));
    if (!ui.reset_armed) fail("GLOBAL", "hidden OCT reset chord debt missing");

    /* Physical LFO currently routes to Turing/EVOLVE, while real Audio LFO is SOUND layer 2. */
    setup(&p, &t, &st, &ui, MELLO_UI_HOME);
    press(&ui, MELLO_UI_BTN_LFO, 0u);
    if (ui.page != MELLO_UI_TURING) fail("MOD", "LFO->TURING routing debt missing");
    setup(&p, &t, &st, &ui, MELLO_UI_SOUND_RESERVED);
    turn_select(&ui, +1);
    turn_select(&ui, +1);
    if (ui.sound_subpage != 2u) fail("MOD", "Audio LFO SOUND layer missing");
    press(&ui, MELLO_UI_BTN_LFO, 0u);
    if (ui.page != MELLO_UI_TURING) fail("MOD", "physical LFO does not expose current placement debt");

    /* DOT is currently reached through FX and then toggles to AUDIO FX. */
    setup(&p, &t, &st, &ui, MELLO_UI_HOME);
    press(&ui, MELLO_UI_BTN_FX, 0u);
    if (ui.page != MELLO_UI_DOT) fail("MOD", "FX->DOT routing debt missing");
    press(&ui, MELLO_UI_BTN_FX, 0u);
    if (ui.page != MELLO_UI_AUDIO_FX) fail("MOD", "DOT->AUDIO FX toggle debt missing");

    /* Black keys remain musical in the explicit manual-note exception. */
    setup(&p, &t, &st, &ui, MELLO_UI_SEQ_STEP);
    ui.selected_step = 0u;
    pitch = t.engine.project.steps[0].pitch;
    input(&ui, 0u, bit_u32(MELLO_UI_BTN_SEL), bit_u32(10u), -1, 0);
    if (t.engine.project.steps[0].pitch == pitch) fail("STEP", "SEL+black-key manual note entry exception missing");
}

int main(void)
{
    unsigned i;
    unsigned edit_violations = 0u;

    for (i = 0u; i < sizeof(pages) / sizeof(pages[0]); ++i) {
        verify_rec(&pages[i]);
        verify_edit(&pages[i]);
        verify_select(&pages[i]);
        verify_black_keys(&pages[i]);
        edit_violations += pages[i].edit_violation;
    }
    verify_known_hidden_debt();

    printf("C1 CURRENT MAP: pages=%u REC-overloads=%u EDIT-overloads=%u black-actions=0 SELECT-layer-contract=PASS\n",
           (unsigned)(sizeof(pages) / sizeof(pages[0])),
           (unsigned)(sizeof(pages) / sizeof(pages[0])), edit_violations);
    puts("C1 DEBT: EDIT+key STEP/SUB modifiers; EDIT+REC CLEAR; RANDOM OCT ALL/NONE; GLOBAL OCT reset chord; LFO->EVOLVE; FX->DOT; Audio LFO under SOUND");
    puts("PASS: deterministic WP-C C1 control-action baseline matches reconstructed pre-remap behavior");
    return 0;
}
