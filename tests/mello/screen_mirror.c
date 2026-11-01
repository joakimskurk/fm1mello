/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Big screen (DECISIONS.md): the encoder (screen_mirror.c, MIT) and the glue (mirror.c) on their own. A hello turns
 * the mirror on and asks for everything; a row becomes F0 7D 'F' 'S' 01 row col {count colour} F7 with the colour
 * in three 7-bit bytes and runs of at most 127; messages stay at 70 bytes or less; a fill of the screen's width is
 * two runs a row; a blit's byte-swapped pixels come out native; nothing is sent 3 s after the last hello; rows the
 * queue has no room for ask for their zone again once it has drained; with nobody asking, the LCD hooks are free. */
#include <stdio.h>
#include <string.h>

#define SM_BUFFER 2048u /* small, so the queue fills in the test */
#include "../../firmware/src/screen_mirror.c"
#include "../../firmware/src/mirror.c"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

static uint8_t got[65536];
static unsigned got_n, msgs, busy;
static int send(const uint8_t *m, unsigned n)
{
    if (busy) return -2;
    CHECK(n <= 70u, "a message of %u bytes", n);
    CHECK(m[0] == 0xF0 && m[n - 1u] == 0xF7, "not a SysEx");
    memcpy(got + got_n, m, n);
    got_n += n;
    msgs++;
    return 0;
}

int main(void)
{
    uint16_t px[240];
    unsigned i;

    CHECK(!sm_on(), "on before any hello");
    mirror_fill(0, 0, 240, 240, 0x1234); /* nobody asking: nothing queued */
    mirror_poll(100, send);
    CHECK(got_n == 0u, "sent %u bytes with nobody asking", got_n);

    mirror_hello(1);
    CHECK(sm_on(), "a hello turns it on");
    mirror_poll(200, send);
    CHECK(mirror_take_dirty() == MIRROR_DIRTY_HEADER, "a full hello asks for the screen, the header first");
    CHECK(mirror_take_dirty() == 0u, "and only once");
    mirror_poll(201, send);
    (void)mirror_take_dirty(); /* status */
    mirror_poll(202, send);
    (void)mirror_take_dirty(); /* footer */
    mirror_poll(203, send);
    (void)mirror_take_dirty(); /* body: the plan is done */

    /* one row of a fill: 240 wide is 127 + 113 */
    mirror_fill(0, 7, 240, 1, 0xF81F);
    mirror_poll(300, send);
    CHECK(msgs == 1u && got_n == 18u, "a 240-wide row is one message of 18 bytes (%u, %u)", msgs, got_n);
    CHECK(got[5] == 7 && got[6] == 0 && got[7] == 0 && got[8] == 0, "row 7, column 0");
    CHECK(got[9] == 127 && got[13] == 113, "runs of 127 and 113 (%u, %u)", got[9], got[13]);
    CHECK(got[10] == (0xF81F & 127) && got[11] == ((0xF81F >> 7) & 127) && got[12] == (0xF81F >> 14),
          "the colour in three 7-bit bytes");

    /* a blit arrives byte-swapped and comes out native */
    for (i = 0u; i < 4u; ++i) px[i] = (uint16_t)(0x07E0 >> 8 | 0x07E0 << 8);
    got_n = msgs = 0u;
    mirror_blit_swapped(100, 200, 4, 1, px);
    mirror_poll(400, send);
    CHECK(msgs == 1u && got[5] == (200 & 127) && got[6] == (200 >> 7) && got[7] == 100 && got[9] == 4 &&
              (got[10] | got[11] << 7 | got[12] << 14) == 0x07E0,
          "a blit of 4 green pixels at (100, 200)");

    /* the queue is small here (2 KB) and the USB is full: a screen of noise overflows it; the rows wait a little
     * (the clock stands still here, so they give up), the zone is planned again, and it is asked for once the
     * queue has drained, one zone at a time */
    for (i = 0u; i < 240u; ++i) px[i] = (uint16_t)(i * 2654435761u >> 3); /* no runs to speak of */
    got_n = msgs = 0u;
    busy = 1u; /* the USB is full: nothing leaves */
    mirror_fake_now = 500u;
    for (i = 0u; i < 40u; ++i) {
        mirror_blit_swapped(0, 40 + i, 240, 1, px);
        mirror_poll(500 + i, send);
    }
    CHECK(got_n == 0u, "busy: nothing sent");
    CHECK(mirror_take_dirty() == 0u, "the lost rows wait for the queue to drain");
    busy = 0u;
    mirror_poll(900, send);
    CHECK(got_n > 0u, "the queue drains once the USB has room");
    CHECK(sm_pending() == 0u, "and is empty");
    mirror_poll(901, send);
    CHECK(mirror_take_dirty() == MIRROR_DIRTY_BODY, "the body is asked for again once the queue is empty");
    CHECK(mirror_take_dirty() == 0u, "once");

    { /* a full request is paced: header, status, footer, body, each as the queue empties */
        uint16_t seen = 0u;
        mirror_hello(1);
        mirror_poll(1000, send);
        CHECK(mirror_take_dirty() == MIRROR_DIRTY_HEADER, "a full request starts with the header");
        mirror_fill(0, 0, 240, 30, 0x0000); /* the header painted: queued; the poll sends it and, the queue
                                              * empty again, hands out the next zone */
        mirror_poll(1001, send);
        seen = mirror_take_dirty();
        CHECK(seen == MIRROR_DIRTY_STATUS, "then the status line (%u)", seen);
        mirror_poll(1002, send);
        CHECK(mirror_take_dirty() == MIRROR_DIRTY_FOOTER, "then the footer");
        mirror_poll(1003, send);
        CHECK(mirror_take_dirty() == MIRROR_DIRTY_BODY, "then the body");
        mirror_poll(1004, send);
        CHECK(mirror_take_dirty() == 0u, "and that is all");
    }

    /* 3 s after the last hello: off, and the hooks are free again; a hello after a pause starts the paced repaint */
    mirror_poll(4100, send);
    CHECK(!sm_on(), "still on 3 s after the hello");
    got_n = msgs = 0u;
    mirror_fill(0, 0, 240, 1, 0xFFFF);
    mirror_poll(4200, send);
    CHECK(got_n == 0u, "sent after the app went quiet");
    mirror_hello(0);
    mirror_poll(4300, send);
    CHECK(sm_on() && mirror_take_dirty() == MIRROR_DIRTY_HEADER,
          "a plain hello after a pause: on, and the header first");

    if (fails) return 1;
    puts("PASS: screen_mirror");
    return 0;
}
