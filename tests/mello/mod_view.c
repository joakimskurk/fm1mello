/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Modulation seen moving (ROADMAP E7 / F14): the audio keeps what modulation does to each destination now
 * (MelloAudio.mod_view, permille of its range), an EVOLVE route on NOISE moving it; a value row whose parameter is
 * modulated draws a second, white thumb where it is now, none when nothing moves it; the UI redraws the body when
 * that moves, at most 15 times a second. */
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"
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

static int white_thumbs;
static void fill(void *c, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t col)
{
    (void)c, (void)y;
    (void)col;
    if (w == 2 && (h == 6 || h == 7) && x >= 78 && x < 170) white_thumbs++; /* (6 rows in the body, stretched) */
}
static void text(void *c, int16_t x, int16_t y, int16_t w, uint8_t l, const char *s, uint16_t col, uint8_t a)
{
    (void)c, (void)x, (void)y, (void)w, (void)l, (void)s, (void)col, (void)a;
}

static int16_t db[65536], rb[24576];
static MelloAudio au;

int main(void)
{
    static MelloProject p;
    static MelloTransport t;
    static MelloStorage st;
    static MelloUi u;
    int16_t view[MELLO_DEST_COUNT];
    MelloUiDrawOps ops = {fill, text, 0, 0, 0, 0, 0};

    /* the audio: EVOLVE on NOISE moves mod_view[NOISE] */
    {
        MelloEngine e;
        MelloModRouting m;
        int32_t out[96];
        mello_project_defaults(&p);
        mello_engine_init(&e, &p, 3u);
        m = e.modr;
        m.tm_dest[0] = MELLO_DEST_NOISE;
        m.tm_depth[0] = 1000;
        mello_audio_init(&au, db, 65536, rb, 24576);
        mello_audio_set_config(&au, &e.sound, &e.fx, &e.sub, &e.sample);
        mello_audio_set_mod_routing(&au, &m);
        au.tm_value = 20000; /* EVOLVE's register, high */
        au.tail_guard_frames = 100000u; /* nothing sounds: keep the render from its silent early return */
        au.silent = false;
        memset(out, 0, sizeof out);
        mello_audio_render(&au, out, 44u, 0u, 120u);
        mello_audio_render(&au, out, 44u, 1000u, 120u);
        CHECK(au.mod_view[MELLO_DEST_NOISE] > 100, "EVOLVE on NOISE: mod_view %d", au.mod_view[MELLO_DEST_NOISE]);
        CHECK(au.mod_view[MELLO_DEST_FM] == 0, "FM moved without a route (%d)", au.mod_view[MELLO_DEST_FM]);
    }

    /* the screen: SOUND FILTER, CUT modulated: a white thumb; nothing modulated: none */
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 1u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    memset(view, 0, sizeof view);
    mello_ui_set_mod_view(&u, view);
    mello_ui_set_page(&u, MELLO_UI_SOUND_RESERVED);
    u.dirty = MELLO_UI_DIRTY_ALL;
    white_thumbs = 0;
    mello_ui_render(&u, &ops);
    CHECK(white_thumbs == 0, "a second thumb with nothing modulated (%d)", white_thumbs);
    view[MELLO_DEST_CUTOFF] = 300;
    u.dirty = MELLO_UI_DIRTY_ALL;
    white_thumbs = 0;
    mello_ui_render(&u, &ops);
    CHECK(white_thumbs == 1, "CUT modulated: %d white thumbs, not 1", white_thumbs);

    /* the redraw: the UI looks every 66 ticks (one a millisecond): a move then marks the body */
    {
        unsigned n;
        for (n = 0u; n < 70u; ++n) mello_ui_tick(&u);
        u.dirty = 0u;
        view[MELLO_DEST_CUTOFF] = 400;
        for (n = 0u; n < 30u; ++n) mello_ui_tick(&u);
        CHECK(!(u.dirty & MELLO_UI_DIRTY_BODY), "redrawn within 66 ms");
        for (n = 0u; n < 40u; ++n) mello_ui_tick(&u);
        CHECK(u.dirty & MELLO_UI_DIRTY_BODY, "the thumb moved and the body was not redrawn");
    }

    if (fails) return 1;
    puts("PASS: mod_view");
    return 0;
}
