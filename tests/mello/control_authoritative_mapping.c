#include "mello_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char hint[96];
} Capture;
typedef struct {
    const char *name;
    MelloUiPage page;
    const char *hint;
} MapCase;

static const MapCase cases[] = {
    {"HOME", MELLO_UI_HOME, "B0:- B1:+ B2:MUT B3:RST B4:CLR B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"STEP", MELLO_UI_SEQ_STEP, "B1:SKIP B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"SEGMENT", MELLO_UI_SEQ_SEGMENT, "B0:EVEN B1:A B2:B B3:C B4:D B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"LOOP", MELLO_UI_SEQ_LOOP, "B0:ON B1:RST B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"SLOTSEQ", MELLO_UI_SEQ_SLOTSEQ, "B0:ON B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"RANDOM", MELLO_UI_RANDOM, "B0:NONE B1:ALL B2:CLR B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"KEY/SCALE", MELLO_UI_KEY_SCALE, "B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"SLOTS", MELLO_UI_SLOTS, "B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"DOT", MELLO_UI_DOT, "B1:RND B2:RST B3:FLW B4:REV B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"TURING", MELLO_UI_TURING, "B0:ON B1:RND B2:LOOP B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"SUB", MELLO_UI_SUB, "B0:ON B1:STEP B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
#if MELLO_FEATURE_CHORD_MACHINE
    {"CHORD", MELLO_UI_CHORD, "B0:ON B1:BASS B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
#endif
    {"GLOBAL", MELLO_UI_GLOBAL, "B1:TAP B2:RST2 B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"SOUND", MELLO_UI_SOUND_RESERVED, "B1:INIT B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"AUDIO FX", MELLO_UI_AUDIO_FX, "B1:PRE-DIST B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"SAMPLE", MELLO_UI_SAMPLE_RESERVED, "B0:ON B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
    {"DRUMS", MELLO_UI_DRUMS, "B0:ON B1:VOX B2:ACC B3:ROLL B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
#if MELLO_FEATURE_CHORD_MACHINE
    {"MIX", MELLO_UI_MIX, "B0:LD B1:BS B2:CH B3:DR B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
#else
    {"MIX", MELLO_UI_MIX, "B0:LD B1:BS B3:DR B5:F B6:B B7:< B8:R B9:UNDO B10:RNDMZR"},
#endif
};

/* The strip shows labels only, in key order: "B0:RND B1:UNDO" is drawn as "RND UNDO". */
static void labels_of(const char *hint, char *out)
{
    *out = 0;
    while (*hint) {
        const char *colon = strchr(hint, ':'), *end = strchr(hint, ' ');
        size_t n;
        if (!colon) break;
        if (!end) end = hint + strlen(hint);
        if (*out) strcat(out, " ");
        n = strlen(out);
        memcpy(out + n, colon + 1, (size_t)(end - colon - 1));
        out[n + (size_t)(end - colon - 1)] = 0;
        hint = *end ? end + 1 : end;
    }
}

static uint32_t bit_u32(unsigned n)
{
    return 1u << n;
}
static void die(const char *where, const char *what)
{
    fprintf(stderr, "FAIL: %s: %s\n", where, what);
    exit(1);
}
static void fill(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c)
{
    (void)ctx;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)c;
}
static void text(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large, const char *s, uint16_t c,
                 uint8_t align)
{
    Capture *cap = (Capture *)ctx;
    (void)x;
    (void)w;
    (void)large;
    (void)c;
    (void)align;
    if (y == 172 && s && *s &&
        strlen(cap->hint) + strlen(s) + 2u < sizeof cap->hint) { /* the strip's labels */
        if (cap->hint[0]) strcat(cap->hint, " ");
        strcat(cap->hint, s);
    }
}
static void led_key(void *ctx, uint8_t i, bool on)
{
    (void)ctx;
    (void)i;
    (void)on;
}
static void led_btn(void *ctx, uint8_t i, bool on)
{
    (void)ctx;
    (void)i;
    (void)on;
}
static void midi(void *ctx, uint8_t a, uint8_t b, uint8_t c)
{
    (void)ctx;
    (void)a;
    (void)b;
    (void)c;
}
static void setup(MelloProject *p, MelloTransport *t, MelloStorage *st, MelloUi *u, MelloUiPage page)
{
    mello_project_defaults(p);
    mello_transport_init(t, p, 123u, midi, 0);
    mello_storage_init(st, 0, 0);
    mello_ui_init(u, t, st);
    mello_ui_set_page(u, page);
}
static uint8_t black_note(uint8_t pos)
{
    uint8_t i, n = 0u;
    for (i = 0u; i < 27u; ++i) {
        if (mello_ui_white_key_position(i) >= 0) continue;
        if (n == pos) return i;
        ++n;
    }
    return 0xFFu;
}
static void black(MelloUi *u, uint32_t ms, uint8_t pos)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    uint8_t note = black_note(pos);
    if (note == 0xFFu) die("BLACK", "missing key");
    mello_ui_input(u, ms * 1000u, ms, 0u, 0u, 0u, bit_u32(note), enc);
}
static void render(MelloUi *u, Capture *cap)
{
    MelloUiDrawOps ops = {fill, text, led_key, led_btn, cap, 0};
    memset(cap, 0, sizeof *cap);
    u->dirty = MELLO_UI_DIRTY_ALL;
    mello_ui_render(u, &ops);
}
static int has_slot(const char *hint, uint8_t slot)
{
    char k[5];
    if (slot < 10u)
        k[0] = 'B', k[1] = (char)('0' + slot), k[2] = ':', k[3] = 0;
    else
        k[0] = 'B', k[1] = '1', k[2] = (char)('0' + slot - 10u), k[3] = ':', k[4] = 0;
    return strstr(hint, k) != 0;
}

static void verify_every_page_and_inactive_slot(void)
{
    unsigned i;
    unsigned active = 0u, inactive = 0u;
    for (i = 0u; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        MelloProject p;
        MelloTransport t;
        MelloStorage st;
        MelloUi u;
        Capture cap;
        uint8_t slot;
        setup(&p, &t, &st, &u, cases[i].page);
        render(&u, &cap);
        {
            char want[96];
            labels_of(cases[i].hint, want);
            if (strcmp(cap.hint, want)) die(cases[i].name, "rendered descriptor hint drift");
        }
        for (slot = 0u; slot < 11u; ++slot) {
            if (has_slot(cases[i].hint, slot)) {
                ++active;
                continue;
            }
            MelloEngine before_engine;
            MelloUi before_ui;
            setup(&p, &t, &st, &u, cases[i].page);
            before_engine = t.engine;
            before_ui = u;
            black(&u, (uint32_t)(100u + slot), slot);
            if (memcmp(&before_engine, &t.engine, sizeof before_engine) ||
                memcmp(&before_ui, &u, sizeof before_ui))
                die(cases[i].name, "inactive contextual slot changed state");
            ++inactive;
        }
    }
#if MELLO_FEATURE_CHORD_MACHINE
    if (active != 149u) die("MAP", "active slot count != 149 with Chord");
#else
    if (active != 140u) die("MAP", "active slot count != 140 without Chord");
#endif
    printf("C5 MAP COVERAGE: pages=%u active=%u inactive=%u\n", (unsigned)(sizeof(cases) / sizeof(cases[0])),
           active, inactive);
}

static void verify_b5_from_every_advertised_page(void)
{
    unsigned i, count = 0u;
    for (i = 0u; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        MelloProject p;
        MelloTransport t;
        MelloStorage st;
        MelloUi u;
        if (!has_slot(cases[i].hint, 10u)) continue; /* RNDMZR: the last black key */
        setup(&p, &t, &st, &u, cases[i].page);
        { /* RNDMZR: a tap rolls the pattern, a hold of 0.5 s opens RANDOM */
            int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
            uint32_t note = bit_u32(black_note(10u));
            MelloProject before = t.engine.project;
            mello_ui_set_held_notes(&u, note);
            mello_ui_input(&u, 100000u, 100u, 0u, 0u, 0u, note, enc);
            mello_ui_input(&u, 700000u, 700u, 0u, 0u, 0u, 0u, enc);
            if (u.page != MELLO_UI_RANDOM) die(cases[i].name, "held RNDMZR did not open RANDOM");
            setup(&p, &t, &st, &u, cases[i].page);
            before = t.engine.project;
            black(&u, 100u, 10u);
            if (!memcmp(&before.steps, &t.engine.project.steps, sizeof before.steps) &&
                cases[i].page != MELLO_UI_RANDOM)
                die(cases[i].name, "a tap on RNDMZR did not roll");
        }
        ++count;
    }
#if MELLO_FEATURE_CHORD_MACHINE
    if (count != 18u) die("B5", "advertised page count != 18 with Chord");
#else
    if (count != 17u) die("B5", "advertised page count != 17 without Chord");
#endif
}

static void verify_manual_note_priority(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi u;
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    uint8_t note = black_note(4u);
    int8_t before;
    setup(&p, &t, &st, &u, MELLO_UI_SEQ_STEP);
    u.selected_step = 0u;
    before = t.engine.project.steps[0].pitch;
    mello_ui_input(&u, 100000u, 100u, 0u, 0u, bit_u32(MELLO_UI_BTN_SEL), bit_u32(note), enc);
    if (t.engine.project.steps[0].pitch == before) die("STEP SEL+black", "manual note priority lost");
    if (u.page != MELLO_UI_SEQ_STEP) die("STEP SEL+black", "context navigation stole musical key");
}

int main(void)
{
    verify_every_page_and_inactive_slot();
    verify_b5_from_every_advertised_page();
    verify_manual_note_priority();
    puts("PASS: WP-C5 all page/slot guidance is covered; inactive slots inert and B5/manual-note contracts "
         "hold");
    return 0;
}
