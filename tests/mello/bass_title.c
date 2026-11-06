/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* BASS says what plays it (2026-11-06, the product owner: "hard to see which mode the bass is in; when it follows
 * the chords it is very hard to understand what happens"). The page's title names BASS SEQ and its length, the
 * 5-SEQ, or the MODE (EUCLID with its pulses), never SEQ's LEN while SEQ is off; with the chords on and CHD lit, the
 * chord it stands on now ("ON Fm7", stopped "ON CHORDS"), 27 characters at most in every combination. CHD, B3 on
 * BASS, is CHORD's BASS: the same switch. */
#include <stdio.h>
#include <string.h>

#include "mello_ui.h"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

static MelloProject p;
static MelloTransport t;
static MelloStorage st;
static MelloUi u;
static char title[64];

static void fill(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c)
{
    (void)ctx, (void)x, (void)y, (void)w, (void)h, (void)c;
}
static void text(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large, const char *s, uint16_t c,
                 uint8_t align)
{
    (void)ctx, (void)x, (void)w, (void)c, (void)align, (void)large;
    if (y > 20 && y < 60 && s) { /* the section's title, under the header (its row as the screen fits it) */
        strncpy(title, s, sizeof title - 1u);
        title[sizeof title - 1u] = 0;
    }
}
static const char *shown(void)
{
    MelloUiDrawOps o;
    memset(&o, 0, sizeof o);
    o.fill = fill;
    o.text = text;
    title[0] = 0;
    u.dirty = MELLO_UI_DIRTY_ALL;
    mello_ui_render(&u, &o);
    return title;
}
static uint32_t black(uint8_t pos)
{
    uint8_t i, n = 0u;
    for (i = 0u; i < 27u; ++i) {
        if (mello_ui_white_key_position(i) >= 0) continue;
        if (n++ == pos) return 1u << i;
    }
    return 0u;
}

int main(void)
{
    MelloEngine *e;
    MelloSubConfig sub;
    MelloBassConfig b;
    uint8_t mode, x, seq, follow;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 5u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_SUB);
    e = &t.engine;
    sub = e->sub;
    sub.on = true, sub.mode = MELLO_SUB_MODE_STEP, sub.x_on = false;
    mello_engine_sub_set_config(e, &sub);
    b = e->bass;
    b.on = false;
    mello_engine_bass_set_config(e, &b);

    CHECK(!strcmp(shown(), "BASS  STEP"), "STEP, SEQ off: '%s'", title);
    b.on = true, b.length = 12u;
    mello_engine_bass_set_config(e, &b);
    CHECK(!strcmp(shown(), "BASS  SEQ 12"), "SEQ on: '%s'", title);
    b.on = false;
    mello_engine_bass_set_config(e, &b);
    sub.mode = MELLO_SUB_MODE_EUCLID, sub.euclid_pulses = 5u;
    mello_engine_sub_set_config(e, &sub);
    CHECK(!strcmp(shown(), "BASS  EUCLID 5"), "EUCLID: '%s'", title);
    sub.x_on = true;
    mello_engine_sub_set_config(e, &sub);
    CHECK(!strcmp(shown(), "BASS  5-SEQ"), "the 5-SEQ over EUCLID: '%s'", title);

#if MELLO_FEATURE_CHORD_MACHINE
    {
        MelloChordConfig c = e->chord;
        uint32_t chd = black(3u), ms;
        char name[8], want[32];
        c.on = true, c.sub_root = false;
        mello_engine_chord_set_config(e, &c);
        sub.x_on = false, sub.mode = MELLO_SUB_MODE_STEP;
        mello_engine_sub_set_config(e, &sub);
        CHECK(!strcmp(shown(), "BASS  STEP"), "chords on, CHD off: '%s'", title);
        /* CHD on the BASS page is CHORD's BASS */
        mello_ui_set_held_notes(&u, chd);
        mello_ui_input(&u, 1000000u, 1000u, 0u, 0u, 0u, chd, 0);
        mello_ui_set_held_notes(&u, 0u);
        mello_ui_input(&u, 1100000u, 1100u, 0u, 0u, 0u, 0u, 0);
        CHECK(e->chord.sub_root, "CHD on BASS did not set the bass on the chords' root");
        CHECK(!strcmp(shown(), "BASS  STEP  ON CHORDS"), "CHD, stopped: '%s'", title);
        mello_transport_start(&t, 2000000u);
        for (ms = 2000u; ms < 4500u; ++ms)
            mello_transport_poll_ahead(&t, ms * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
        CHECK(e->chord_voiced_slot < e->chord.length, "no chord voiced after 2.5 s");
        mello_engine_chord_name(e, e->chord_voiced_slot, name);
        (void)snprintf(want, sizeof want, "BASS  STEP  ON %s", name);
        CHECK(!strcmp(shown(), want), "CHD, playing: '%s', not '%s'", title, want);

        /* every combination fits: 27 characters (8 px each in 222) */
        for (mode = 0u; mode <= MELLO_SUB_MODE_DRONE; ++mode)
            for (x = 0u; x < 2u; ++x)
                for (seq = 0u; seq < 2u; ++seq)
                    for (follow = 0u; follow < 2u; ++follow) {
                        sub.mode = (MelloSubMode)mode, sub.x_on = x != 0u, sub.euclid_pulses = 64u;
                        mello_engine_sub_set_config(e, &sub);
                        b.on = seq != 0u, b.length = 16u;
                        mello_engine_bass_set_config(e, &b);
                        c = e->chord;
                        c.sub_root = follow != 0u;
                        mello_engine_chord_set_config(e, &c);
                        CHECK(strlen(shown()) <= 27u, "'%s' is longer than 27", title);
                    }
        mello_transport_stop(&t);
        for (mode = 0u; mode <= MELLO_SUB_MODE_DRONE; ++mode) {
            sub.mode = (MelloSubMode)mode, sub.x_on = true;
            mello_engine_sub_set_config(e, &sub);
            CHECK(strlen(shown()) <= 27u, "'%s' (stopped) is longer than 27", title);
        }
    }
#endif
    if (fails) return 1;
    puts("PASS: BASS's title says what plays it and the chord it follows, inside the screen; CHD is CHORD's "
         "BASS");
    return 0;
}
