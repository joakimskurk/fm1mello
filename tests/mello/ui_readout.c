/* Phase 3: the readout. Turning a knob shows, large over the body, where you are (page / layer), what the knob
 * moves and its value, with the footer's own texts; it goes 1.2 s after the last detent. An inert knob and a page
 * change leave no readout. */
#include <stdio.h>
#include <string.h>

#include "mello_ui.h"

static char seen[32][48];
static unsigned seen_n;

static void fill(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c)
{
    (void)ctx, (void)x, (void)y, (void)w, (void)h, (void)c;
}
static void text(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large, const char *s, uint16_t c, uint8_t a)
{
    (void)ctx, (void)x, (void)w, (void)large, (void)c, (void)a;
    if (y >= 30 && y < 172 && seen_n < 32u) { /* the body only */
        strncpy(seen[seen_n], s, 47);
        seen[seen_n++][47] = 0;
    }
}
static int body_has(const char *s)
{
    unsigned i;
    for (i = 0u; i < seen_n; ++i)
        if (!strcmp(seen[i], s)) return 1;
    return 0;
}
static void turn(MelloUi *u, uint32_t ms, uint8_t enc, int16_t d)
{
    int16_t e[MELLO_UI_ENCODER_COUNT] = {0};
    e[enc] = d;
    mello_ui_input(u, ms * 1000u, ms, 0u, 0u, 0u, 0u, e);
}
static void render(MelloUi *u)
{
    MelloUiDrawOps o = {fill, text, 0, 0, 0, 0};
    seen_n = 0u;
    mello_ui_render(u, &o);
}
static int fail(const char *what)
{
    unsigned i;
    fprintf(stderr, "FAIL: %s; body texts:", what);
    for (i = 0u; i < seen_n; ++i) fprintf(stderr, " '%s'", seen[i]);
    fputc('\n', stderr);
    return 1;
}

int main(void)
{
    static MelloProject p;
    static MelloTransport t;
    static MelloStorage st;
    static MelloUi u;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 1u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_SOUND_RESERVED);
    render(&u);

    turn(&u, 1000u, MELLO_UI_ENC_KNOB3, 1); /* SOUND / OSC: K3 = cutoff */
    if (u.readout != 3u) return fail("K3 on SOUND gave no readout");
    render(&u);
    if (!body_has("SOUND / OSC / FILTER")) return fail("readout does not say where");
    {
        unsigned i, big = 0u;
        for (i = 0u; i < seen_n; ++i)
            if (strstr(seen[i], "CUT")) big = 1u;
        if (!big) return fail("readout does not name the cutoff");
    }

    turn(&u, 2000u, MELLO_UI_ENC_KNOB3, 0); /* 1.0 s later: still there */
    if (u.readout != 3u) return fail("readout went too early");
    turn(&u, 2300u, MELLO_UI_ENC_KNOB1, 0); /* 1.3 s after the detent: gone, and the body comes back */
    if (u.readout || !(u.dirty & MELLO_UI_DIRTY_BODY)) return fail("readout did not go");

    turn(&u, 3000u, MELLO_UI_ENC_SELECT, 1); /* SELECT: the layer, by name */
    render(&u);
    if (u.readout != MELLO_UI_READOUT_LAYER || !body_has("AMP ENVELOPE")) return fail("SELECT shows no layer");

    mello_ui_set_page(&u, MELLO_UI_RANDOM); /* a page change drops it */
    if (u.readout) return fail("readout survived a page change");
    turn(&u, 4000u, MELLO_UI_ENC_KNOB4, 1); /* RANDOM's K4 is inert */
    if (u.readout) return fail("an inert knob showed a readout");

    puts("PASS: readout shows page / layer, control and value; goes after 1.2 s; quiet when nothing changed");
    return 0;
}
