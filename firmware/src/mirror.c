/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Big screen: the glue between the LCD, the USB and screen_mirror.c (MIT, Patrick F. Kellogg). Included by
 * platform.c on the FM-1 and by tests/host.c on a computer, with MIRROR_NOW() the millisecond clock. Nothing here
 * runs in the audio interrupt.
 *
 * The queue is 16 KB and a page is more than that, so a repaint is paced: a full request paints one zone at a time
 * (the header, the status line, the footer, then the body), each once the queue has drained; a row the queue cannot
 * take waits a little for the USB to take some (the interrupt drains the ring meanwhile), and a row still lost asks
 * for its zone again, once the queue has drained. An app that stopped reading gets no waiting: the next hello (or
 * 3 s) sorts it out. */
#include "mirror.h"
#include "screen_mirror.h"

#ifndef MIRROR_NOW
static uint32_t mirror_fake_now;
#define MIRROR_NOW() mirror_fake_now
#endif
#define MIRROR_WAIT_MS  40u  /* a row waits this long at most for room in the queue */
#define MIRROR_STALL_MS 1000u /* after a wait without progress: no waiting for this long */

static uint16_t mirror_row[240]; /* one row in native RGB565, as sm_rows wants it */
static uint16_t mirror_plan;     /* zones to paint again, one at a time as the queue drains */
static uint16_t mirror_dirty;    /* what the UI is told to paint, taken by mirror_take_dirty */
static uint32_t mirror_stalled_until;
static mirror_send_fn mirror_send;

static uint16_t mirror_zone(uint32_t y)
{
    return y < 30u ? MIRROR_DIRTY_HEADER : y < 189u ? MIRROR_DIRTY_BODY : y < 221u ? MIRROR_DIRTY_FOOTER
                                                                                 : MIRROR_DIRTY_STATUS;
}

static void mirror_row_out(uint32_t x, uint32_t y, uint32_t w)
{
    uint32_t t0, spins;
    unsigned before;
    if (sm_rows(mirror_row, x, y, w, 1u)) return;
    t0 = MIRROR_NOW();
    if (mirror_send && (int32_t)(t0 - mirror_stalled_until) >= 0) {
        before = sm_pending();
        for (spins = 0u; spins < 100000u && MIRROR_NOW() - t0 < MIRROR_WAIT_MS; ++spins) {
            sm_poll(MIRROR_NOW(), mirror_send);
            if (sm_rows(mirror_row, x, y, w, 1u)) return;
        }
        if (sm_pending() >= before) mirror_stalled_until = MIRROR_NOW() + MIRROR_STALL_MS; /* nobody is reading */
    }
    mirror_plan |= mirror_zone(y); /* lost: this zone again, once the queue has drained */
}

void mirror_hello(int full)
{
    uint8_t m[6] = {0xF0u, 0x7Du, 'F', 'S', (uint8_t)(full ? 1u : 0u), 0xF7u};
    (void)sm_sysex(m, 6u);
}

void mirror_poll(uint32_t now_ms, mirror_send_fn send)
{
    static const uint16_t order[4] = {MIRROR_DIRTY_HEADER, MIRROR_DIRTY_STATUS, MIRROR_DIRTY_FOOTER,
                                      MIRROR_DIRTY_BODY};
    unsigned i;
    mirror_send = send;
    sm_poll(now_ms, send);
    if (sm_take_full_request())
        mirror_plan |= MIRROR_DIRTY_HEADER | MIRROR_DIRTY_STATUS | MIRROR_DIRTY_FOOTER | MIRROR_DIRTY_BODY;
    if (!sm_on()) {
        mirror_plan = mirror_dirty = 0u;
        return;
    }
    if (mirror_plan && !mirror_dirty && !sm_pending()) /* the queue is empty: the next zone */
        for (i = 0u; i < 4u; ++i)
            if (mirror_plan & order[i]) {
                mirror_dirty = order[i];
                mirror_plan = (uint16_t)(mirror_plan & ~order[i]);
                break;
            }
}

uint16_t mirror_take_dirty(void)
{
    uint16_t d = mirror_dirty;
    mirror_dirty = 0u;
    return d;
}

void mirror_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t colour)
{
    uint32_t i, j;
    if (!sm_on() || !w || !h || x >= 240u || y >= 240u) return;
    if (x + w > 240u) w = 240u - x;
    if (y + h > 240u) h = 240u - y;
    for (i = 0u; i < w; ++i) mirror_row[i] = colour;
    for (j = 0u; j < h; ++j) mirror_row_out(x, y + j, w);
}

void mirror_blit_swapped(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px)
{
    uint32_t i, j, cw;
    if (!sm_on() || !w || !h || x >= 240u || y >= 240u) return;
    cw = x + w > 240u ? 240u - x : w;
    for (j = 0u; j < h && y + j < 240u; ++j, px += w) {
        for (i = 0u; i < cw; ++i) mirror_row[i] = (uint16_t)(px[i] >> 8 | px[i] << 8);
        mirror_row_out(x, y + j, cw);
    }
}
