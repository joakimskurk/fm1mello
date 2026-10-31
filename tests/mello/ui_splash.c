/* The splash (2026-10-31): everything inside 240x240, the wordmark there, its text and the version's inside their
 * boxes (8 pixels a character, 16 large), and nothing drawn for a missing ops table. */
#include <stdio.h>
#include <string.h>

#include "mello_ui.h"

static int fills, texts, bad, wordmark, version;

static void fill(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c)
{
    (void)ctx;
    (void)c;
    fills++;
    if (x < 0 || y < 0 || w < 0 || h < 0 || x + w > 240 || y + h > 240) bad++;
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
    puts("PASS: the splash fits, with the wordmark and the version");
    return 0;
}
