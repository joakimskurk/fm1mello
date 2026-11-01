/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The TRS MIDI IN parser (firmware/src/midi_uart.c, from Dinghy 0.4) on the host: bytes land in its DMA ring the
 * way UART1 would write them, um_drain turns them into USB-MIDI packets on the shared ring with cable 1, and
 * midi_get's rule (cable 1 -> 2) tells the jack from USB. Running status, one-byte Cx / Dx messages, realtime
 * inside a message, SysEx (dropped), the 0xFD gap byte and a full ring are covered. The register driver
 * (fm1_uart1_midi_init, fm1_uart1_rx_take) is not run here: it writes the chip's registers. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* what usb.c / platform.c give midi_uart.c */
#define MQ 64u
static uint32_t midi_in_q[MQ];
static volatile uint32_t mi_w, mi_r;
#define RING_PUBLISH() ((void)0)

#include "../../firmware/src/midi_uart.c"

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                      \
            printf(__VA_ARGS__);                                                                             \
            printf("\n");                                                                                    \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

static uint32_t dma_w; /* where the "DMA" writes next */
static void dma(const uint8_t *b, uint32_t n)
{
    uint32_t i;
    for (i = 0u; i < n; ++i) {
        um_ring[dma_w] = b[i];
        dma_w = (dma_w + 1u) & (UM_RING - 1u);
    }
}
static int get(uint8_t *st, uint8_t *d1, uint8_t *d2) /* platform.c's midi_get */
{
    uint32_t pk;
    if (mi_r == mi_w) return 0;
    pk = midi_in_q[mi_r % MQ];
    mi_r++;
    *st = (uint8_t)(pk >> 8);
    *d1 = (uint8_t)(pk >> 16);
    *d2 = (uint8_t)(pk >> 24);
    return pk & 0xF0u ? 2 : 1;
}
static int expect(uint8_t st, uint8_t d1, uint8_t d2, int line)
{
    uint8_t s, a, b;
    int src = get(&s, &a, &b);
    if (src != 2 || s != st || a != d1 || b != d2) {
        printf("FAIL %s:%d: wanted %02X %02X %02X from the jack, got src %d: %02X %02X %02X\n", __FILE__,
               line, st, d1, d2, src, s, a, b);
        return 1;
    }
    return 0;
}
#define EXPECT(st, d1, d2)                                                                                   \
    if (expect(st, d1, d2, __LINE__)) return 1

int main(void)
{
    uint8_t s, a, b;
    uint32_t i;
    void (*const not_run[2])(void) = {uart_midi_init,
                                      uart_midi_poll}; /* the register driver: linked, not called */
    static const uint8_t note_on[] = {0x90, 60, 100};
    static const uint8_t running[] = {64, 90, 67, 80, 60, 0};       /* running status: three more on 0x90 */
    static const uint8_t with_clock[] = {0x91, 0xF8, 62, 0xF8, 70}; /* realtime inside a message */
    static const uint8_t one_byte[] = {0xC0, 5, 0xD1, 99,
                                       7}; /* program change, channel pressure (+ running) */
    static const uint8_t sysex[] = {0xF0, 0x7E, 0x7F, 0x06, 0x01,
                                    0xF7, 0x80, 60,   0}; /* dropped, then a note off */
    static const uint8_t transport[] = {0xFA, 0xFB, 0xFC, 0xFE,
                                        0xFF}; /* clock/transport in, sensing/reset not */
    static const uint8_t cc_pb[] = {0xB2, 1, 64, 0xE3, 0, 64};
    static const uint8_t stray_data[] = {12, 34}; /* data before any status: ignored */

    for (i = 0; i < UM_RING; i++) um_ring[i] = UM_EMPTY; /* uart_midi_init without the registers */
    (void)not_run;

    dma(note_on, sizeof note_on);
    um_drain();
    EXPECT(0x90, 60, 100);
    CHECK(get(&s, &a, &b) == 0, "more than one message from one note");

    dma(running, sizeof running);
    um_drain();
    EXPECT(0x90, 64, 90);
    EXPECT(0x90, 67, 80);
    EXPECT(0x90, 60, 0);

    dma(with_clock, sizeof with_clock);
    um_drain();
    EXPECT(0xF8, 0, 0);
    EXPECT(0xF8, 0, 0);
    EXPECT(0x91, 62, 70);

    dma(one_byte, sizeof one_byte);
    um_drain();
    EXPECT(0xC0, 5, 0);
    EXPECT(0xD1, 99, 0);
    EXPECT(0xD1, 7, 0);

    dma(sysex, sizeof sysex);
    um_drain();
    EXPECT(0x80, 60, 0);
    CHECK(get(&s, &a, &b) == 0, "SysEx bytes leaked out as messages");

    dma(transport, sizeof transport);
    um_drain();
    EXPECT(0xFA, 0, 0);
    EXPECT(0xFB, 0, 0);
    EXPECT(0xFC, 0, 0);
    CHECK(get(&s, &a, &b) == 0, "active sensing / reset were passed on");

    dma(cc_pb, sizeof cc_pb);
    um_drain();
    EXPECT(0xB2, 1, 64);
    EXPECT(0xE3, 0, 64);

    /* a message split across two polls: the first half waits for the second */
    dma(note_on, 2u);
    um_drain();
    CHECK(get(&s, &a, &b) == 0, "half a message was delivered");
    dma(note_on + 2, 1u);
    um_drain();
    EXPECT(0x90, 60, 100);

    /* the ring marker byte (0xFD) received as line noise with a byte after it: both pass, nothing is lost */
    {
        static const uint8_t noise[] = {0x90, 0xFD, 61, 101};
        dma(noise, sizeof noise);
        um_drain();
        EXPECT(0x90, 61, 101);
    }

    /* a status byte starts a fresh message: the stray data byte is swallowed */
    mi_r = mi_w; /* drop whatever */
    um.st = 0;
    dma(stray_data, sizeof stray_data);
    um_drain();
    CHECK(get(&s, &a, &b) == 0, "data without a status was delivered");

    /* a full shared ring drops, and counts the drop */
    {
        uint32_t drops0 = um.drops, k;
        uint8_t msg[3] = {0x90, 50, 50};
        mi_r = mi_w = 0u;
        for (k = 0; k < MQ + 4u; ++k) {
            dma(msg, 3u);
            um_drain(); /* the ring is 128 B: drain each one */
        }
        CHECK(mi_w - mi_r == MQ, "the shared ring holds %u, not %u", mi_w - mi_r, MQ);
        CHECK(um.drops - drops0 == 4u, "%u drops counted, 4 expected", um.drops - drops0);
        for (k = 0; k < MQ; ++k) EXPECT(0x90, 50, 50);
    }

    printf("PASS: the TRS parser gives %u messages so far; running status, realtime, SysEx, Cx/Dx, gaps and "
           "a full "
           "ring behave\n",
           um.msgs);
    return 0;
}
