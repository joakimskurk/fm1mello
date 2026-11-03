/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The status line's counter (DECISIONS.md "The status line counts beats, not steps"): playing with RESET set it
 * reads the beat of the beats before the reset, in clock pulses ("21/32" for 2 BAR), wraps with the reset, and past
 * 99 beats reads bar.beat/bars ("7.05/8"); with RESET on FREE the step of the length ("5/16"); stopped, the length
 * ("16 ST"). The transport is polled the way the app polls it, a millisecond at a time with its look-ahead. */
#include "mello_engine.h"
#include "mello_storage.h"
#include "mello_transport.h"
#include "mello_ui.h"
#include <stdio.h>
#include <string.h>

static char status[32];
static void fill(void *c, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t col)
{
    (void)c;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)col;
}
static void text(void *c, int16_t x, int16_t y, int16_t w, uint8_t large, const char *s, uint16_t col,
                 uint8_t al)
{
    (void)c;
    (void)w;
    (void)large;
    (void)col;
    (void)al;
    if (y == 7 && x == 184 && s) snprintf(status, sizeof status, "%s", s); /* the header's right (G7) */
}
static void led(void *c, uint8_t i, bool on)
{
    (void)c;
    (void)i;
    (void)on;
}
static void midi(void *c, uint8_t a, uint8_t b, uint8_t d)
{
    (void)c;
    (void)a;
    (void)b;
    (void)d;
}

static MelloTransport t;
static MelloStorage st;
static MelloUi ui;
static uint32_t polled_us;

static void fresh(uint16_t master_len)
{
    static MelloProject p;
    mello_project_defaults(&p);
    p.master_len = master_len;
    mello_transport_init(&t, &p, 1u, midi, NULL);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&ui, &t, &st);
    polled_us = 0u;
}
static const char *
status_at(uint32_t us) /* poll up to us, then render: what the status line's counter reads */
{
    MelloUiDrawOps ops = {fill, text, led, led, 0, 0};
    for (; polled_us <= us; polled_us += 1000u)
        mello_transport_poll_ahead(&t, polled_us, MELLO_TRANSPORT_LOOKAHEAD_US);
    status[0] = 0;
    ui.dirty = MELLO_UI_DIRTY_ALL;
    mello_ui_render(&ui, &ops);
    return status;
}
static int expect(const char *want, const char *got, const char *when)
{
    if (strcmp(want, got) == 0) return 0;
    printf("FAIL: %s: the counter reads \"%s\", wanted \"%s\"\n", when, got, want);
    return 1;
}

int main(void)
{
    uint32_t pulse_us;
    int bad = 0;

    fresh(32u); /* 2 BAR */
    pulse_us = 60000000u / t.engine.project.bpm / 4u;
    bad |= expect("16 ST", status_at(0u), "stopped");
    mello_transport_start(&t, 0u);
    bad |= expect("1/32", status_at(pulse_us / 2u), "2 BAR, the first pulse");
    bad |= expect("21/32", status_at(20u * pulse_us + pulse_us / 2u), "2 BAR, pulse 21");
    bad |= expect("32/32", status_at(31u * pulse_us + pulse_us / 2u), "2 BAR, the last pulse");
    bad |= expect("1/32", status_at(32u * pulse_us + pulse_us / 2u), "2 BAR, after the reset");
    bad |= expect("5/32", status_at(36u * pulse_us + pulse_us / 2u), "2 BAR, second time round");
    mello_transport_stop(&t);
    bad |= expect("16 ST", status_at(37u * pulse_us), "stopped again");

    fresh(128u); /* 8 BAR: bar.beat/bars */
    mello_transport_start(&t, 0u);
    bad |= expect("1.01/8", status_at(pulse_us / 2u), "8 BAR, the first pulse");
    bad |= expect("2.05/8", status_at(20u * pulse_us + pulse_us / 2u), "8 BAR, pulse 21");
    bad |= expect("7.05/8", status_at(100u * pulse_us + pulse_us / 2u), "8 BAR, pulse 101");
    bad |= expect("8.16/8", status_at(127u * pulse_us + pulse_us / 2u), "8 BAR, the last pulse");
    bad |= expect("1.01/8", status_at(128u * pulse_us + pulse_us / 2u), "8 BAR, after the reset");

    fresh(0u); /* FREE: the step of the length */
    mello_transport_start(&t, 0u);
    {
        const char *s = status_at(20u * pulse_us + pulse_us / 2u);
        unsigned step = 0u, len = 0u;
        if (sscanf(s, "%u/%u", &step, &len) != 2 || len != 16u || step < 1u || step > 16u) {
            printf("FAIL: FREE: the counter reads \"%s\", wanted a step of 16\n", s);
            bad = 1;
        }
    }

    if (bad) return 1;
    printf(
        "PASS: the status line counts the beats before RESET (21/32, 7.05/8), the step when FREE, the length "
        "stopped\n");
    return 0;
}
