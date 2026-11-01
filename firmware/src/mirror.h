/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Big screen (DECISIONS.md): the FM-1's screen over USB MIDI to FM-1 Mirror (github.com/pfkellogg/fm1-mirror) or any
 * app that speaks its messages. The encoder is Patrick F. Kellogg's MIT screen_mirror.c; this is the glue: the LCD's
 * writes become rows, the USB's hello becomes a request, lost rows become a repaint of their zone. */
#ifndef MIRROR_H
#define MIRROR_H
#include <stdint.h>

/* The zones a lost row asks the UI to paint again; the values are mello_ui.h's MELLO_UI_DIRTY_* bits. */
#define MIRROR_DIRTY_HEADER 1u
#define MIRROR_DIRTY_BODY   2u
#define MIRROR_DIRTY_FOOTER 4u
#define MIRROR_DIRTY_STATUS 8u
#define MIRROR_DIRTY_ALL    0xFFu

typedef int (*mirror_send_fn)(const uint8_t *message, unsigned length);

/* The app's hello (full: and it wants the whole screen). From the USB's SysEx parser, or the host's script. */
void mirror_hello(int full);
/* Every main-loop pass: sends what is queued, times the app out after 3 s, turns a full request and the lost rows
 * into a repaint. */
void mirror_poll(uint32_t now_ms, mirror_send_fn send);
/* The zones the UI should paint again (MIRROR_DIRTY_*), once. */
uint16_t mirror_take_dirty(void);
/* The LCD's writes, as the screen gets them: a filled rectangle in the native RGB565 colour, and a blit whose
 * pixels are byte-swapped as the ST7789 wants them (gfx.c's canvas). */
void mirror_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t colour);
void mirror_blit_swapped(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px);

#endif
