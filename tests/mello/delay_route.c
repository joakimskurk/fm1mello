/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* FX > DELAY ROUTE (DECISIONS "The looping delay"): REVERB sends the delay's return, as it is heard, into the reverb
 * with the parts' sends. 0 on a new project and on one saved before it (the FX section's version 9), so nothing
 * sounds different until it is turned; saved and loaded (version 10), clamped to 100%; K1 a percent a detent, K2..K4
 * nothing. In the audio: with every part's reverb send at 0, a note's echo leaves the room silent at 0 and rings in it
 * turned up, with PING as without and with PRE DIST too; before the echo it changes nothing. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"
#include "mello_storage.h"
#include "mello_ui.h"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

#define FLASH_SIZE (MELLO_STORAGE_BANK0_ADDR + 2u * MELLO_STORAGE_BANK_SIZE)
static uint8_t flash[FLASH_SIZE];
static bool ram_read(void *ctx, uint32_t address, void *dst, uint32_t length)
{
    (void)ctx;
    if (address + length > FLASH_SIZE) return false;
    memcpy(dst, flash + address, length);
    return true;
}
static bool ram_erase4k(void *ctx, uint32_t address)
{
    (void)ctx;
    if (address % 4096u || address + 4096u > FLASH_SIZE) return false;
    memset(flash + address, 0xFF, 4096u);
    return true;
}
static bool ram_program(void *ctx, uint32_t address, const void *src, uint32_t length)
{
    const uint8_t *p = src;
    uint32_t i;
    (void)ctx;
    if (address + length > FLASH_SIZE) return false;
    for (i = 0u; i < length; ++i) flash[address + i] &= p[i];
    return true;
}
static const MelloStorageIo io = {ram_read, ram_erase4k, ram_program, NULL};
static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v, p[1] = (uint8_t)(v >> 8), p[2] = (uint8_t)(v >> 16), p[3] = (uint8_t)(v >> 24);
}
static MelloEngine e;
static void load(void)
{
    static MelloProject p;
    MelloStorage s;
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 2u);
    mello_storage_init(&s, &io, NULL);
    (void)mello_storage_load(&s, &e);
}

static void storage(void)
{
    static MelloProject p;
    MelloStorage s;
    MelloFxConfig f;
    MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
    uint32_t t = 0u, base = 0u, size, pos;
    uint8_t *pl;
    int patched = 0;
    memset(flash, 0xFF, sizeof flash);
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 1u);
    CHECK(e.fx.delay_to_reverb == 0u, "a new project's REVERB %u", e.fx.delay_to_reverb);
    f = e.fx;
    f.delay_to_reverb = 1200u;
    mello_engine_fx_set_config(&e, &f);
    CHECK(e.fx.delay_to_reverb == 1000u, "REVERB 1200 kept as %u", e.fx.delay_to_reverb);
    f.delay_to_reverb = 370u, f.delay_pitch = 3;
    mello_engine_fx_set_config(&e, &f);
    mello_storage_init(&s, &io, NULL);
    mello_storage_request_save(&s, t);
    for (; t <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u) w = mello_storage_poll(&s, &e, t, true);
    CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "not saved");
    load();
    CHECK(e.fx.delay_to_reverb == 370u && e.fx.delay_pitch == 3,
          "saved, loaded as REVERB %u (want 370), PITCH %d", e.fx.delay_to_reverb, e.fx.delay_pitch);
    /* the same record, its FX section made version 9: its last 2 bytes go */
    if (get32(flash + MELLO_STORAGE_BANK0_ADDR) == MELLO_STORAGE_MAGIC) base = MELLO_STORAGE_BANK0_ADDR;
    if (get32(flash + MELLO_STORAGE_BANK1_ADDR) == MELLO_STORAGE_MAGIC) base = MELLO_STORAGE_BANK1_ADDR;
    CHECK(base != 0u, "no record");
    if (!base) return;
    size = get32(flash + base + 8u);
    pl = flash + base + MELLO_STORAGE_HEADER_SIZE;
    for (pos = 0u; pos + 8u <= size;) {
        uint32_t id = pl[pos] | (uint32_t)pl[pos + 1u] << 8, ver = pl[pos + 2u] | (uint32_t)pl[pos + 3u] << 8,
                 len = get32(pl + pos + 4u);
        if (id == 5u) { /* SEC_FX */
            const uint8_t *k = pl + pos + 8u + len - 2u;
            CHECK(ver == 10u && (k[0] | k[1] << 8) == 370,
                  "the FX section is version %u, its last 2 bytes not REVERB", ver);
            memmove(pl + pos + 8u + len - 2u, pl + pos + 8u + len, /* (the footer after the payload too) */
                    size - (pos + 8u + len) + MELLO_STORAGE_FOOTER_SIZE);
            len -= 2u, size -= 2u;
            put32(pl + pos + 4u, len);
            put32(flash + base + 8u, size);
            pl[pos + 2u] = 9u, patched = 1;
        }
        pos += 8u + len;
    }
    CHECK(patched, "no FX section");
    put32(flash + base + 16u, mello_storage_crc32(pl, size));
    put32(flash + base + 20u, mello_storage_crc32(flash + base, 20u));
    load();
    CHECK(e.fx.delay_to_reverb == 0u && e.fx.delay_pitch == 3,
          "saved before ROUTE, loaded as REVERB %u (want 0), PITCH %d (want 3)", e.fx.delay_to_reverb,
          e.fx.delay_pitch);
}

static void ui(void)
{
    static MelloProject p;
    static MelloTransport t;
    static MelloStorage st;
    static MelloUi u;
    int16_t enc[MELLO_UI_ENCODER_COUNT];
    uint32_t ms = 100u;
    MelloFxConfig before;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 123u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_AUDIO_FX);
    u.fx_subpage = 4u;
    memset(enc, 0, sizeof enc);
    enc[MELLO_UI_ENC_KNOB1] = 3;
    mello_ui_input(&u, ms * 1000u, ms, 0u, 0u, 0u, 0u, enc), ms += 500u;
    CHECK(t.engine.fx.delay_to_reverb == 30u, "K1 3 detents: REVERB %u (want 30)",
          t.engine.fx.delay_to_reverb);
    before = t.engine.fx;
    memset(enc, 0, sizeof enc);
    enc[MELLO_UI_ENC_KNOB2] = 4, enc[MELLO_UI_ENC_KNOB3] = -2, enc[MELLO_UI_ENC_KNOB4] = 5;
    mello_ui_input(&u, ms * 1000u, ms, 0u, 0u, 0u, 0u, enc), ms += 500u;
    CHECK(!memcmp(&before, &t.engine.fx, sizeof before), "K2..K4 on ROUTE changed the FX");
}

/* the audio: a note's echo, the parts' reverb sends all 0 */
#define FS 44100
static int16_t delay_buf[MELLO_AUDIO_DELAY_SAMPLES], reverb_buf[MELLO_AUDIO_REVERB_SAMPLES];
static MelloAudio a;
static int32_t out[2 * FS * 3];
static double render(uint16_t to_rev, bool ping, bool pre_dist, int32_t *keep)
{
    MelloSoundConfig sound;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sample;
    MelloEvent ev;
    double tail = 0.0;
    uint32_t n, k;
    mello_sound_defaults(&sound);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sample);
    for (k = 0u; k < MELLO_SEND_PARTS; ++k) fx.send_rev[k] = 0u;
    fx.delay_sync = false, fx.delay_time_ms = 300u, fx.delay_feedback = 0u, fx.delay_wet = 800u;
    fx.delay_pingpong = ping, fx.delay_pre_dist = pre_dist, fx.delay_to_reverb = to_rev;
    fx.reverb_wet = 600u, fx.reverb_decay_ms = 3000u;
    sound.decay_ms = 60u, sound.sustain_permille = 0u, sound.release_ms = 20u;
    mello_audio_init(&a, delay_buf, MELLO_AUDIO_DELAY_SAMPLES, reverb_buf, MELLO_AUDIO_REVERB_SAMPLES);
    mello_audio_set_config(&a, &sound, &fx, &sub, &sample);
    memset(&ev, 0, sizeof ev);
    ev.type = MELLO_EVENT_NOTE_ON, ev.note = 60u, ev.velocity = 110u;
    mello_audio_push_engine_event(&a, &ev, 0u);
    ev.type = MELLO_EVENT_NOTE_OFF;
    mello_audio_push_engine_event(&a, &ev, 50000u);
    memset(out, 0, sizeof out);
    for (n = 0u; n < 3u * FS; n += 64u) mello_audio_render(&a, out + 2u * n, 64u, n * 1000000ull / FS, 120u);
    /* 0.9 .. 3 s: the note and its one echo (at 0.3 s, as long) long gone; what is left is the room's */
    for (n = 9u * FS / 10u; n < 3u * FS; ++n)
        tail += (double)out[2u * n] * out[2u * n] + (double)out[2u * n + 1u] * out[2u * n + 1u];
    if (keep)
        for (n = 0u; n < 2u * 3u * FS; ++n) keep[n] = out[n];
    return tail;
}

int main(void)
{
    static int32_t plain[2 * FS * 3];
    storage();
    ui();
    {
        double off = render(0u, false, false, plain), on = render(1000u, false, false, 0),
               ping = render(1000u, true, false, 0), pre = render(1000u, false, true, 0);
        uint32_t n, diff = 0u;
        CHECK(off < 1e3, "REVERB 0: the room not silent after the echo (%g)", off);
        CHECK(on > 1e7, "REVERB 100%%: the echo not in the room (%g)", on);
        CHECK(ping > 1e7, "REVERB 100%%, PING: the echoes not in the room (%g)", ping);
        CHECK(pre > 1e7, "REVERB 100%%, PRE DIST: the echo not in the room (%g)", pre);
        printf("the room after the echo: REVERB 0 %.3g, 100%% %.3g, with PING %.3g, with PRE DIST %.3g\n",
               off, on, ping, pre);
        /* before the echo (0 .. 0.3 s) REVERB changes nothing: what it sends is the delay's */
        (void)render(1000u, false, false, out);
        for (n = 0u; n < 2u * 3u * FS / 10u; ++n)
            if (out[n] != plain[n]) diff++;
        CHECK(diff == 0u, "REVERB 100%%: %u samples differ before the echo", diff);
    }
    if (fails) return 1;
    puts("PASS: delay_route");
    return 0;
}
