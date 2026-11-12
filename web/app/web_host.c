/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* SkurkyMello in a browser: an FM-1 for the page (web/app/), as tests/host.c is one for a computer. After Star
 * Distroyer's web/app/web_host.c (github.com/joakimskurk/fm1stardistroyer). platform.c includes this file instead of
 * tests/host.c when DINGHY_WEB is set (with DINGHY_HOST: the app's host branches), and dinghy.c's app_main runs a pass
 * a call there. The firmware's own code runs on it, compiled to WebAssembly (web/app/build.sh), no imports, no C
 * library:
 *
 *   web_init()                the app's start; the splash then plays a pass a millisecond
 *   web_run(n)                n stereo frames of its sound into web_out (L R L R, +-32767): the app's main loop runs
 *                             a pass for each millisecond those frames are worth, the sound in its blocks of 256
 *                             frames, as the audio interrupt renders it
 *   web_lcd(), web_lcd_rev()  the screen, 240 x 240 RGB565 (CPU order), and a count that moves when it changes
 *   web_leds(i), web_glow(i)  the LEDs, 32 ids each (i 0: ids 0..31, 1: 32..63): 0..13 the buttons, 14..40 the keys;
 *                             lit, and the dim layer
 *   web_key(k, down)          key k (0 = F3 .. 26 = G5) down or up
 *   web_btn(id, down)         a button (BTN_*) down or up
 *   web_enc(id, n)            an encoder turned n detents (ENC_*, + = clockwise)
 *   web_store(), web_store_size(), web_store_rev()   the flash the project lives in (mello_storage.h's two banks,
 *                             128 KiB): the page keeps a copy in the browser when the count moves, and puts it back
 *                             before web_init
 *
 * A press and its release within one pass both count: the release waits a pass, so the app sees it held. */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define WEB_EXPORT(name) __attribute__((export_name(#name)))

/* the C library the compiler and the app call: byte by byte (volatile: so the loops are not made into calls of
 * themselves) */
void *memcpy(void *d, const void *s, size_t n)
{
    volatile uint8_t *a = d;
    const uint8_t *b = s;
    while (n--) *a++ = *b++;
    return d;
}
void *memmove(void *d, const void *s, size_t n)
{
    volatile uint8_t *a = d;
    const uint8_t *b = s;
    if (a < b)
        while (n--) *a++ = *b++;
    else
        while (n--) a[n] = b[n];
    return d;
}
void *memset(void *d, int c, size_t n)
{
    volatile uint8_t *a = d;
    while (n--) *a++ = (uint8_t)c;
    return d;
}
int memcmp(const void *x, const void *y, size_t n)
{
    const uint8_t *a = x, *b = y;
    for (; n; n--, a++, b++)
        if (*a != *b) return *a - *b;
    return 0;
}

#ifndef APP_NAME
#define APP_NAME "DINGHY"
#endif
#ifndef APP_VER
#define APP_VER "0.0"
#endif
#define FS 44100u
#define HALF_FRAMES 256u

static volatile uint32_t fm1_ms;
static struct { uint32_t notes, buttons; } fm1_in;
static uint32_t host_edges, host_rel;                 /* buttons pressed, released, since the app last asked */
static uint32_t host_note_edges;                      /* keys gone down since the app last asked */
static uint32_t btn_fresh, btn_up_next, btn_up_due;   /* pressed this pass; released this pass (due the next) */
static uint32_t key_fresh, key_up_next, key_up_due;
static uint32_t fm1_input_edges(uint32_t *released)
{
    uint32_t b = host_edges;
    host_edges = 0;
    *released = host_rel;
    host_rel = 0;
    return b;
}
static uint32_t fm1_input_note_edges(void)
{
    uint32_t n = host_note_edges;
    host_note_edges = 0;
    return n;
}
static int32_t host_enc[8];
static int32_t fm1_enc_take(uint32_t e)
{
    int32_t d = host_enc[e & 7u];
    host_enc[e & 7u] = 0;
    return d;
}
static uint64_t host_leds, host_glow;
static void fm1_led_key(uint32_t id, int on)
{
    if (id >= 64u) return;
    if (on) host_leds |= (uint64_t)1 << id;
    else host_leds &= ~((uint64_t)1 << id);
}
static void fm1_led_key_glow(uint32_t id, int on)
{
    if (id >= 64u) return;
    if (on) host_glow |= (uint64_t)1 << id;
    else host_glow &= ~((uint64_t)1 << id);
}

/* Big screen: no USB here, so no viewer ever asks for a repaint */
uint16_t mirror_take_dirty(void) { return 0; }

/* the screen: 240 x 240 RGB565, in the CPU's order (gfx.c sends the bytes swapped, as the ST7789 wants them) */
static uint16_t fb[240 * 240];
static uint32_t fb_rev;
static void lcd_sync(void) {}
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    for (j = y; j < y + h && j < 240u; j++)
        for (i = x; i < x + w && i < 240u; i++)
            fb[j * 240u + i] = c;
    fb_rev++;
}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px)
{
    uint32_t i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if (x + i < 240u && y + j < 240u) {
                uint16_t c = px[j * w + i];
                fb[(y + j) * 240u + x + i] = (uint16_t)(c >> 8 | c << 8);
            }
    fb_rev++;
}
#define CV_MAX (240u * 64u)
#include "gfx.c"

/* the flash the project lives in, in RAM: mello_storage.h's two banks; erased is 0xFF, a write only clears bits, as
 * the FM-1's NOR flash does. Outside it reads erased and takes no writes. */
#include "mello_storage.h"
#define WEB_FLASH_BASE MELLO_STORAGE_BANK0_ADDR
#define WEB_FLASH_SIZE (2u * MELLO_STORAGE_BANK_SIZE)
static uint8_t web_flash[WEB_FLASH_SIZE];
static uint32_t flash_rev, flash_kept; /* flash_kept: the page asked where to put its copy back */
static int web_flash_in(uint32_t off, uint32_t n) { return off >= WEB_FLASH_BASE && off - WEB_FLASH_BASE + n <= WEB_FLASH_SIZE; }
static int st_read(uint32_t off, void *dst, uint32_t n)
{
    uint8_t *d = dst;
    uint32_t i;
    for (i = 0; i < n; i++) d[i] = web_flash_in(off + i, 1) ? web_flash[off + i - WEB_FLASH_BASE] : 0xFFu;
    return 0;
}
static int fl_erase4k(uint32_t off, uint32_t *took_us)
{
    *took_us = 0;
    if (!web_flash_in(off & ~0xFFFu, 0x1000u)) return -1;
    memset(web_flash + ((off & ~0xFFFu) - WEB_FLASH_BASE), 0xFF, 0x1000u);
    flash_rev++;
    return 0;
}
static int fl_write(uint32_t off, const uint8_t *src, uint32_t n)
{
    uint32_t i;
    if (!web_flash_in(off, n)) return -1;
    for (i = 0; i < n; i++) web_flash[off - WEB_FLASH_BASE + i] &= src[i];
    flash_rev++;
    return 0;
}

/* MIDI: none in here (the page has no MIDI port to give it); nothing goes out */
static int midi_get(uint8_t *st, uint8_t *d1, uint8_t *d2)
{
    (void)st, (void)d1, (void)d2;
    return 0;
}
static void midi_send(uint32_t st, uint32_t d1, uint32_t d2) { (void)st, (void)d1, (void)d2; }

static void app_audio(int32_t *out, uint32_t n);
static void app_main(void);

/* the sound: a ring the passes fill, a block at a time; web_run hands it out */
#define RING 8192u /* frames */
static int16_t ring[2u * RING];
static uint32_t ring_w, ring_r; /* frames written, read */
static int32_t web_out[2u * 1024u];

static void platform_poll(void) /* one call = one millisecond, as on the host */
{
    static uint32_t owed;
    static int32_t blk[2u * HALF_FRAMES];
    fm1_ms++;
    host_rel |= btn_up_due; /* the releases a pass late */
    fm1_in.buttons &= ~btn_up_due;
    fm1_in.notes &= ~key_up_due;
    btn_up_due = btn_up_next, btn_up_next = 0, btn_fresh = 0;
    key_up_due = key_up_next, key_up_next = 0, key_fresh = 0;
    owed += FS;
    while (owed >= 1000u * HALF_FRAMES) {
        uint32_t i;
        owed -= 1000u * HALF_FRAMES;
        for (i = 0; i < 2u * HALF_FRAMES; i++) blk[i] = 0;
        app_audio(blk, HALF_FRAMES);
        for (i = 0; i < HALF_FRAMES; i++) { /* (as the USB audio takes it: the MASTER knob all the way up) */
            uint32_t k = (ring_w + i) % RING;
            int32_t l = blk[2u * i], r = blk[2u * i + 1u];
            ring[2u * k] = (int16_t)(l > 32767 ? 32767 : l < -32767 ? -32767 : l);
            ring[2u * k + 1u] = (int16_t)(r > 32767 ? 32767 : r < -32767 ? -32767 : r);
        }
        ring_w += HALF_FRAMES;
    }
}

/* SEED AUTO in a browser (dinghy.c seed_take): the page's random numbers (crypto.getRandomValues), given before
 * web_init; none given (0), only the clock's, the same at every start */
static uint32_t web_entropy;
WEB_EXPORT(web_seed) void web_seed(uint32_t r) { web_entropy = r; }
WEB_EXPORT(web_init) void web_init(void)
{
    static int started;
    if (started) return;
    started = 1;
    if (!flash_kept) memset(web_flash, 0xFF, sizeof web_flash); /* (the page put nothing back: out of the box) */
    app_main(); /* its start (app_main: a pass a call) */
}
WEB_EXPORT(web_run) uint32_t web_run(uint32_t n)
{
    uint32_t i, guard = 0;
    if (n > 1024u) n = 1024u;
    while (ring_w - ring_r < n && guard++ < 1000u) app_main();
    for (i = 0; i < n; i++) {
        uint32_t k = (ring_r + i) % RING;
        web_out[2u * i] = ring[2u * k];
        web_out[2u * i + 1u] = ring[2u * k + 1u];
    }
    ring_r += n;
    return n;
}
WEB_EXPORT(web_out_ptr) int32_t *web_out_ptr(void) { return web_out; }
WEB_EXPORT(web_lcd) uint16_t *web_lcd(void) { return fb; }
WEB_EXPORT(web_lcd_rev) uint32_t web_lcd_rev(void) { return fb_rev; }
WEB_EXPORT(web_ms) uint32_t web_ms(void) { return fm1_ms; }
WEB_EXPORT(web_leds) uint32_t web_leds(uint32_t i) { return (uint32_t)(host_leds >> (i ? 32 : 0)); }
WEB_EXPORT(web_glow) uint32_t web_glow(uint32_t i) { return (uint32_t)(host_glow >> (i ? 32 : 0)); }
WEB_EXPORT(web_key) void web_key(uint32_t k, uint32_t down)
{
    uint32_t b;
    if (k >= 27u) return;
    b = 1u << k;
    if (down) {
        if (!(fm1_in.notes & b)) host_note_edges |= b;
        fm1_in.notes |= b, key_fresh |= b, key_up_next &= ~b, key_up_due &= ~b;
    } else if (key_fresh & b)
        key_up_next |= b;
    else
        fm1_in.notes &= ~b;
}
WEB_EXPORT(web_btn) void web_btn(uint32_t id, uint32_t down)
{
    uint32_t b;
    if (id >= 14u) return;
    b = 1u << id;
    if (down) {
        if (!(fm1_in.buttons & b)) host_edges |= b;
        fm1_in.buttons |= b, btn_fresh |= b, btn_up_next &= ~b, btn_up_due &= ~b;
    } else if (btn_fresh & b)
        btn_up_next |= b;
    else if (fm1_in.buttons & b) {
        host_rel |= b;
        fm1_in.buttons &= ~b;
    }
}
WEB_EXPORT(web_enc) void web_enc(uint32_t id, int32_t n) { host_enc[id & 7u] += n; }
WEB_EXPORT(web_store) uint8_t *web_store(void)
{
    flash_kept = 1;
    return web_flash;
}
WEB_EXPORT(web_store_size) uint32_t web_store_size(void) { return (uint32_t)sizeof web_flash; }
WEB_EXPORT(web_store_rev) uint32_t web_store_rev(void) { return flash_rev; }
