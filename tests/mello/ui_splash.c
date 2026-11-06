/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The splash (2026-10-31): everything inside 240x240, the wordmark there, its text and the version's inside their
 * boxes (8 pixels a character, 16 large), and nothing drawn for a missing ops table. Its skyline playing
 * (2026-11-06): a frame draws inside the sky alone (over the line, between the bars' edges), nothing when nothing
 * moved, the step hit jumps, and frames 33 ms apart leave the screen as one frame straight to the end does. */
#include <stdio.h>
#include <string.h>

#include "mello_ui.h"

static int fills, texts, bad, wordmark, version, outside_sky, sky_mode;
static uint16_t screen[240][240];

static void fill(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c)
{
    int16_t i, j;
    (void)ctx;
    fills++;
    if (x < 0 || y < 0 || w < 0 || h < 0 || x + w > 240 || y + h > 240) {
        bad++;
        return;
    }
    if (sky_mode && (x < 25 || x + w > 25 + 15 * 12 + 9 || y < 38 || y + h > 116)) outside_sky++;
    for (j = y; j < y + h; ++j)
        for (i = x; i < x + w; ++i) screen[j][i] = c;
}
static void text(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large, const char *s, uint16_t c,
                 uint8_t align)
{
    (void)ctx;
    (void)c;
    (void)align;
    texts++;
    if (!s || x < 0 || y < 0 || x + w > 240 || y + (large == 1u ? 32 : 16) > 240 ||
        (int)strlen(s) * (large == 1u ? 16 : 8) > w)
        bad++;
    if (s && !strcmp(s, "SKURKYMELLO") && large == 1u) wordmark = 1;
    if (s && strstr(s, "0.5-f18")) version = 1;
}

int main(void)
{
    MelloUiDrawOps o;
    memset(&o, 0, sizeof o);
    o.fill = fill;
    o.text = text;
    mello_ui_draw_splash(NULL, "x");
    if (fills || texts) {
        fprintf(stderr, "FAIL: drew without ops\n");
        return 1;
    }
    mello_ui_draw_splash(&o, "0.5-f18");
    printf("splash: %d fills, %d texts\n", fills, texts);
    if (bad || !wordmark || !version || fills < 17) {
        fprintf(stderr, "FAIL: splash: %d out of bounds or clipped, wordmark %d, version %d\n", bad, wordmark,
                version);
        return 1;
    }
    {
        static uint16_t still[240][240], stepped[240][240];
        uint32_t t;
        int col0 = 25 + 4; /* step 0's middle; its rest top is 116 - 18 = 98 */
        sky_mode = 1;
        fills = 0;
        mello_ui_splash_frame(&o, 0u, 0u);
        mello_ui_splash_frame(&o, 500u, 500u);
        mello_ui_splash_frame(NULL, 0u, 500u);
        if (fills) {
            fprintf(stderr, "FAIL: a splash frame drew with nothing moved, or without ops (%d fills)\n",
                    fills);
            return 1;
        }
        mello_ui_splash_frame(&o, 0u, 95u); /* the playhead on step 0 (at 90 ms) */
        if (!fills || screen[98 - 12][col0] == 0u || screen[98 - 13][col0] != 0u) {
            fprintf(stderr, "FAIL: step 0 hit does not jump 12 px (%d fills)\n", fills);
            return 1;
        }
        mello_ui_splash_frame(&o, 95u, 0u); /* back to the still splash */
        memcpy(still, screen, sizeof screen);
        for (t = 0u; t < 4000u; t += 33u) mello_ui_splash_frame(&o, t, t + 33u); /* ends at t */
        memcpy(stepped, screen, sizeof screen);
        memcpy(screen, still, sizeof screen);
        mello_ui_splash_frame(&o, 0u, t);
        if (memcmp(stepped, screen, sizeof screen)) {
            fprintf(stderr, "FAIL: the splash's frames, 33 ms apart, end other than one frame does\n");
            return 1;
        }
        if (outside_sky || bad) {
            fprintf(stderr, "FAIL: splash frames drew outside the sky (%d) or the screen (%d)\n", outside_sky,
                    bad);
            return 1;
        }
    }
    puts("PASS: the splash fits, with the wordmark and the version; its skyline plays inside the sky");
    return 0;
}
