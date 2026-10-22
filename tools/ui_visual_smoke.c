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

    mello_ui_render(&ui, &ops);
    if (bounds_error || fills < 30 || texts < 8) return 2;

    transport.display_step = 1u;
    mello_ui_tick(&ui);
    fills = 0;
    texts = 0;
    mello_ui_render(&ui, &ops);

    if (bounds_error) return 3;
    if (fills >= 20 || texts >= 4) {
        fprintf(stderr, "HOME playhead redraw too broad: fills=%d texts=%d\n", fills, texts);
        return 4;
    }
    printf("PASS: HOME visual bounds + incremental redraw (fills=%d texts=%d)\n", fills, texts);
    return 0;
}
