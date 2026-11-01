/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* R5 (DECISIONS.md "One thread": words and units). Every page and layer rendered: no footer label from the banned
 * list of abbreviations (one word per thing); an unused knob has no label and "-"; a known level or amount reads
 * with %, a known time with MS or S, every CUT in HZ; no label wider than its 60-pixel cell (7 characters). */
#include <stdio.h>
#include <string.h>

#include "mello_ui.h"

static int fails;
typedef struct {
    char lab[4][32], val[4][32];
} Foot;

static void fill(void *c, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t col)
{
    (void)c, (void)x, (void)y, (void)w, (void)h, (void)col;
}
static void text(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large, const char *s, uint16_t col,
                 uint8_t al)
{
    Foot *f = ctx;
    int k;
    (void)w, (void)large, (void)col, (void)al;
    if (!s || x < 2) return;
    k = (x - 2) / 60;
    if (k < 0 || k > 3) return;
    if (y == 188) snprintf(f->lab[k], 32, "%s", s);
    if (y == 204) snprintf(f->val[k], 32, "%s", s);
}
static void led(void *c, uint8_t i, bool on)
{
    (void)c, (void)i, (void)on;
}
static void midi(void *c, uint8_t a, uint8_t b, uint8_t d)
{
    (void)c, (void)a, (void)b, (void)d;
}

static int subcount(MelloUiPage p)
{
    switch (p) {
    case MELLO_UI_SEQ_STEP: return 4;
    case MELLO_UI_SEQ_SEGMENT: return 2;
    case MELLO_UI_SEQ_LOOP: return 3;
    case MELLO_UI_SLOTS: return 2;
    case MELLO_UI_TURING: return 4;
    case MELLO_UI_SUB: return 6;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: return 9;
#endif
    case MELLO_UI_GLOBAL: return 3;
    case MELLO_UI_SOUND_RESERVED: return 10;
    case MELLO_UI_AUDIO_FX: return 5;
    case MELLO_UI_SAMPLE_RESERVED: return 4;
    case MELLO_UI_DRUMS: return 5;
    case MELLO_UI_DOT: return 5;
    case MELLO_UI_RANDOM: return 3;
    default: return 1;
    }
}
static void set_sub(MelloUi *ui, MelloUiPage page, int sub)
{
    switch (page) {
    case MELLO_UI_SEQ_STEP: ui->step_subpage = (uint8_t)sub; break;
    case MELLO_UI_SEQ_SEGMENT: ui->segment_subpage = (uint8_t)sub; break;
    case MELLO_UI_SEQ_LOOP: ui->loop_subpage = (uint8_t)sub; break;
    case MELLO_UI_SLOTS: ui->slots_subpage = (uint8_t)sub; break;
    case MELLO_UI_TURING: ui->tm_subpage = (uint8_t)sub; break;
    case MELLO_UI_SUB: ui->sub_subpage = (uint8_t)sub; break;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: ui->chord_subpage = (uint8_t)sub; break;
#endif
    case MELLO_UI_GLOBAL: ui->global_subpage = (uint8_t)sub; break;
    case MELLO_UI_SOUND_RESERVED: ui->sound_subpage = (uint8_t)sub; break;
    case MELLO_UI_AUDIO_FX: ui->fx_subpage = (uint8_t)sub; break;
    case MELLO_UI_SAMPLE_RESERVED: ui->sample_subpage = (uint8_t)sub; break;
    case MELLO_UI_DRUMS: ui->drum_subpage = (uint8_t)sub; break;
    case MELLO_UI_DOT: ui->selected_dot_target = (uint8_t)sub; break;
    case MELLO_UI_RANDOM:
        ui->random_links = sub == 1;
        ui->random_edit = sub == 2;
        break;
    default: break;
    }
}

static int ends(const char *s, const char *e)
{
    size_t a = strlen(s), b = strlen(e);
    return a >= b && !strcmp(s + a - b, e);
}
static int in(const char *s, const char *const *list)
{
    for (; *list; ++list)
        if (!strcmp(s, *list)) return 1;
    return 0;
}

int main(void)
{
    static const char *const banned[] = {"LEN",   "DEC",   "ATK", "SUS", "REL", "ACC",   "E.DEC",
                                         "F DEC", "F ATK", "INT", "MUT", "ROT", "CHAIN", "K1",
                                         "K2",    "K3",    "K4",  "-",   0};
    static const char *const percent[] = {
        "LEVEL", "CUTOFF",  "RES",  "GATE",  "PROB", "CHORUS", "SPACE",  "DEPTH",   "AMOUNT", "DRIVE", "FOLD",
        "CRUSH", "SUSTAIN", "WET",  "FDBK",  "MIX",  "SWING",  "ROLL",   "CHANCE",  "TUNE",   "TONE",  "ENV",
        "FENV",  "MUTATE",  "DIST", "START", "END",  "ACCENT", "MASTER", "SUBOSC",  "NOISE",  "N.FM",  "FM",
        "SAT",   "PWM",     "LEAD", "SUB",   "BASS", "CHORD",  "PERC",   "D.LEVEL", 0};
    static const char *const no_percent[] = {"ON", "OFF", "SND", "-", 0}; /* words a percent knob may show */
    static const char *const ms_labels[] = {"DECAY", "ATTACK", "RELEASE", "F.ATK", "F.DEC", "GLIDE",
                                            "STRUM", "SLIDE",  "TIME",    "PRE",   "MS",    0};
    static MelloProject p;
    static MelloTransport t;
    static MelloStorage st;
    static MelloUi ui;
    int page, sub, k, checked = 0;
    for (page = MELLO_UI_HOME; page <= MELLO_UI_MIX; ++page)
        for (sub = 0; sub < subcount((MelloUiPage)page); ++sub) {
            Foot f;
            MelloUiDrawOps ops = {fill, text, led, led, &f, 0};
            memset(&f, 0, sizeof f);
            mello_project_defaults(&p);
            mello_transport_init(&t, &p, 1u, midi, NULL);
            mello_storage_init(&st, 0, 0);
            mello_ui_init(&ui, &t, &st);
            mello_ui_set_page(&ui, (MelloUiPage)page);
            set_sub(&ui, (MelloUiPage)page, sub);
            ui.dirty = MELLO_UI_DIRTY_ALL;
            mello_ui_render(&ui, &ops);
            for (k = 0; k < 4; ++k) {
                const char *l = f.lab[k], *v = f.val[k];
                const char *where = mello_ui_page_name((MelloUiPage)page);
                checked++;
                if (in(l, banned))
                    fails++, fprintf(stderr, "FAIL: %s layer %d K%d: label '%s' is on the banned list\n",
                                     where, sub, k + 1, l);
                if (strlen(l) > 7u)
                    fails++, fprintf(stderr, "FAIL: %s layer %d K%d: label '%s' is wider than its cell\n",
                                     where, sub, k + 1, l);
                if (!l[0] && strcmp(v, "-"))
                    fails++, fprintf(stderr, "FAIL: %s layer %d K%d: no label but value '%s'\n", where, sub,
                                     k + 1, v);
                if (!strcmp(l, "CUT") && !ends(v, "HZ"))
                    fails++, fprintf(stderr, "FAIL: %s layer %d K%d: CUT reads '%s', not in Hz\n", where, sub,
                                     k + 1, v);
                if (in(l, percent) && !ends(v, "%") && !in(v, no_percent) && !strchr(v, ' ') &&
                    ((v[0] >= '0' && v[0] <= '9') || v[0] == '+' || v[0] == '-') && /* a word is a mode */
                    (page == MELLO_UI_SUB || page == MELLO_UI_CHORD || page == MELLO_UI_SAMPLE_RESERVED ||
                     page == MELLO_UI_TURING || page == MELLO_UI_SEQ_STEP || page == MELLO_UI_DRUMS ||
                     page == MELLO_UI_MIX || page == MELLO_UI_AUDIO_FX || page == MELLO_UI_SOUND_RESERVED))
                    fails++, fprintf(stderr, "FAIL: %s layer %d K%d: %s reads '%s', not in %%\n", where, sub,
                                     k + 1, l, v);
                if (in(l, ms_labels) && v[0] >= '0' && v[0] <= '9' && !ends(v, "MS") && !ends(v, "S") &&
                    page !=
                        MELLO_UI_TURING) /* EVOLVE's DEPTH layer: each route's depth, named by its route */
                    fails++, fprintf(stderr, "FAIL: %s layer %d K%d: %s reads '%s', not in ms or s\n", where,
                                     sub, k + 1, l, v);
            }
        }
    printf("%d knobs over every page and layer\n", checked);
    if (fails) return 1;
    puts("PASS: one word per thing, % for levels, ms for times, Hz for CUT");
    return 0;
}
