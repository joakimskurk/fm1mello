#include "mello_ui.h"
#include <stdio.h>

static int fills = 0;
static int texts = 0;
static int bounds_error = 0;

static void smoke_fill(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour)
{
    (void)ctx; (void)colour;
    if (x < 0 || y < 0 || w < 0 || h < 0 || x + w > 240 || y + h > 240) {
        fprintf(stderr, "bad fill %d %d %d %d\n", x, y, w, h);
        bounds_error = 1;
    }
    ++fills;
}

static void smoke_text(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large,
                       const char *text_value, uint16_t colour, uint8_t align)
{
    (void)ctx; (void)large; (void)text_value; (void)colour; (void)align;
    if (x < 0 || y < 0 || w < 0 || x + w > 240 || y > 239) {
        fprintf(stderr, "bad text %d %d %d\n", x, y, w);
        bounds_error = 1;
    }
    ++texts;
}

static int render_page(MelloUi *ui, const MelloUiDrawOps *ops, MelloUiPage page)
{
    fills = 0;
    texts = 0;
    bounds_error = 0;
    mello_ui_set_page(ui, page);
    mello_ui_render(ui, ops);
    if (bounds_error) {
        fprintf(stderr, "page %d exceeded 240x240 bounds\n", (int)page);
        return 1;
    }
    return 0;
}

int main(void)
{
    MelloProject project;
    MelloTransport transport;
    MelloStorage storage;
    MelloUi ui;
    MelloUiDrawOps ops = { smoke_fill, smoke_text, 0, 0, 0 };

    mello_project_defaults(&project);
    mello_transport_init(&transport, &project, 123u, 0, 0);
    mello_storage_init(&storage, 0, 0);
    mello_ui_init(&ui, &transport, &storage);

    if (render_page(&ui, &ops, MELLO_UI_HOME) || fills < 30 || texts < 8) return 2;
    if (render_page(&ui, &ops, MELLO_UI_SEQ_STEP) || fills < 50 || texts < 12) return 3;
    if (render_page(&ui, &ops, MELLO_UI_SEQ_SEGMENT) || fills < 50 || texts < 12) return 4;
    if (render_page(&ui, &ops, MELLO_UI_SEQ_LOOP) || fills < 45 || texts < 15) return 5;
    if (render_page(&ui, &ops, MELLO_UI_RANDOM) || fills < 30 || texts < 10) return 6;
    if (render_page(&ui, &ops, MELLO_UI_DOT) || fills < 50 || texts < 8) return 7;
    if (render_page(&ui, &ops, MELLO_UI_TURING) || fills < 30 || texts < 12) return 8;
    if (render_page(&ui, &ops, MELLO_UI_SUB) || fills < 35 || texts < 18) return 9;

    mello_ui_set_page(&ui, MELLO_UI_HOME);
    mello_ui_render(&ui, &ops);
    transport.display_step = 1u;
    mello_ui_tick(&ui);
    fills = 0;
    texts = 0;
    bounds_error = 0;
    mello_ui_render(&ui, &ops);

    if (bounds_error) return 10;
    if (fills >= 20 || texts >= 4) {
        fprintf(stderr, "HOME playhead redraw too broad: fills=%d texts=%d\n", fills, texts);
        return 11;
    }
    printf("PASS: HOME/STEP/SEGMENT/LOOP/RANDOM/DOT/TURING/SUB visual bounds + HOME incremental redraw (fills=%d texts=%d)\n", fills, texts);
    return 0;
}
