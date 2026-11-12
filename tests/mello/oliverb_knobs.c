/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Oliverb's knobs (FX > REVERB OLIVERB, ROADMAP H6, DECISIONS "Oliverb's knobs"): DIFFUSE, SWING, RATE and SHIMMER
 * turn it as Parasites' Oliverb mode turns its STEREO SPREAD, REVERB, FEEDBACK and PITCH. A new project's, and one
 * saved before them (its FX section before version 8), give Oliverb as it was (Parasites' Init: diffusion 0.625, the
 * swing 30 samples at 32 kHz, its rate 0.5, no shimmer), the same numbers to the bit; a saved project keeps them; the
 * layer comes after RETURNS on SELECT, both ways; and a loop shifting its pitch at every pass still dies away. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"
#include "mello_storage.h"
#include "mello_ui.h"

#if !defined(MELLO_REVERB_TEST_HOOK) || !MELLO_REVERB_TEST_HOOK
#error "oliverb_knobs needs MELLO_REVERB_TEST_HOOK=1"
#endif

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

/* ---- the audio: what each knob sets */
static int no_run(void (*fn)(uint32_t), uint32_t arg)
{
    (void)fn, (void)arg;
    return 0;
}
static int no_wait(void)
{
    return 0;
}
static void no_sync(void) {}
static const MelloAudioCores CORES = {no_run, no_wait, no_sync, 0}; /* (only so that Oliverb is chosen) */

#define FS 44100
static int16_t delay_buf[4096], reverb_buf[MELLO_AUDIO_REVERB_SAMPLES];
static MelloAudio a;
static MelloSoundConfig sound;
static MelloSubConfig sub;
static MelloSampleConfig sample;

static void start(const MelloFxConfig *fx)
{
    int32_t out[2];
    mello_sound_defaults(&sound);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sample);
    mello_audio_init(&a, delay_buf, 4096u, reverb_buf, MELLO_AUDIO_REVERB_SAMPLES);
    mello_audio_set_config(&a, &sound, fx, &sub, &sample);
    mello_audio_render(&a, out, 1u, 0u, 80u);
    mello_audio_set_cores(&a, &CORES);
    mello_audio_reverb_choose_probe(&a);
    CHECK(a.oliverb_on, "Oliverb not chosen");
}
/* the knobs turned on a running Oliverb, as the UI's edits arrive: applied at the next block */
static void turn(MelloFxConfig *fx)
{
    int32_t out[2];
    mello_audio_set_config(&a, &sound, fx, &sub, &sample);
    mello_audio_render(&a, out, 1u, 0u, 80u);
}

static void settings(void)
{
    MelloFxConfig fx;
    mello_fx_defaults(&fx);
    CHECK(fx.olv_diffuse == 650u && fx.olv_swing == 100u && fx.olv_rate == 500u && fx.olv_shimmer == 0,
          "a new project's DIFFUSE %u, SWING %u, RATE %u, SHIMMER %d", fx.olv_diffuse, fx.olv_swing,
          fx.olv_rate, fx.olv_shimmer);
    start(&fx);
    /* Parasites' Init, as 0.5-f115 to f119 had it in constants */
    CHECK(a.oliverb.kap == 20480 && a.oliverb.mod_q16 == 1354752 && a.oliverb.slope_q24 == 1902u &&
              a.oliverb.ratio_q16 == 65536u && a.oliverb.psa == 0,
          "at a new project's knobs: kap %d, swing %d, slope %u, ratio %u, shimmer %d", a.oliverb.kap,
          a.oliverb.mod_q16, a.oliverb.slope_q24, a.oliverb.ratio_q16, a.oliverb.psa);
    fx.olv_diffuse = 0u, fx.olv_swing = 1000u, fx.olv_rate = 1000u, fx.olv_shimmer = 12;
    turn(&fx);
    CHECK(a.oliverb.kap == 9830 && a.oliverb.mod_q16 == 13547520 && a.oliverb.slope_q24 >= 121730u &&
              a.oliverb.slope_q24 <= 121739u && a.oliverb.ratio_q16 == 131072u && a.oliverb.psa == 32768,
          "DIFFUSE 0, SWING, RATE 100%%, SHIMMER +12: kap %d (0.3), swing %d (300 at 32 kHz), slope %u, "
          "ratio %u, "
          "shimmer %d",
          a.oliverb.kap, a.oliverb.mod_q16, a.oliverb.slope_q24, a.oliverb.ratio_q16, a.oliverb.psa);
    fx.olv_diffuse = 1000u, fx.olv_swing = 0u, fx.olv_rate = 0u, fx.olv_shimmer = -12;
    turn(&fx);
    CHECK(a.oliverb.kap == 26214 && a.oliverb.mod_q16 == 0 && a.oliverb.slope_q24 == 1u &&
              a.oliverb.ratio_q16 == 32768u && a.oliverb.psa == 32768,
          "DIFFUSE 100%%, SWING, RATE 0, SHIMMER -12: kap %d (0.8), swing %d, slope %u, ratio %u, shimmer %d",
          a.oliverb.kap, a.oliverb.mod_q16, a.oliverb.slope_q24, a.oliverb.ratio_q16, a.oliverb.psa);
    fx.olv_shimmer = 0;
    turn(&fx);
    CHECK(a.oliverb.ratio_q16 == 65536u && a.oliverb.psa == 0, "SHIMMER OFF: ratio %u, shimmer %d",
          a.oliverb.ratio_q16, a.oliverb.psa);
}

/* a second of noise into the network, then its energy each second; the loop, its pitch shifted at each pass, falls */
static void shimmer_dies_away(void)
{
    MelloFxConfig fx;
    uint32_t seed = 4242u, n;
    int32_t l, r;
    double sec[12] = {0}, peak = 0.0;
    mello_fx_defaults(&fx);
    fx.delay_wet = 0u, fx.delay_level = 0u;
    fx.reverb_wet = 1000u, fx.reverb_level = 1000u, fx.reverb_size = 1000u;
    fx.olv_diffuse = 1000u, fx.olv_swing = 1000u, fx.olv_rate = 1000u, fx.olv_shimmer = 24;
    start(&fx);
    for (n = 0u; n < 12u * FS; ++n) {
        int32_t in = 0;
        if (n < (uint32_t)FS) seed = seed * 1664525u + 1013904223u, in = ((int32_t)(seed >> 20) - 2048) * 2;
        mello_audio_reverb_stereo_probe(&a, in, &l, &r);
        sec[n / FS] += (double)l * l + (double)r * r;
        if (fabs((double)l) > peak) peak = fabs((double)l);
        if (fabs((double)r) > peak) peak = fabs((double)r);
    }
    printf("SHIMMER +24 at DECAY %.1f s, every knob up: each second %.1f, %.1f, %.1f, %.1f dB; the last %.1f "
           "dB\n",
           fx.reverb_decay_ms / 1000.0, 10.0 * log10(sec[1] / sec[0] + 1e-30),
           10.0 * log10(sec[2] / sec[0] + 1e-30), 10.0 * log10(sec[3] / sec[0] + 1e-30),
           10.0 * log10(sec[4] / sec[0] + 1e-30), 10.0 * log10(sec[11] / sec[0] + 1e-30));
    CHECK(peak < 32767.0, "SHIMMER +24 reached %.0f", peak);
    CHECK(sec[11] < sec[0] * 1e-6, "SHIMMER +24: the twelfth second %.1f dB under the first (want under -60)",
          10.0 * log10(sec[11] / sec[0] + 1e-30));
}

/* ---- storage: saved and loaded; a project from before the knobs (FX section version 7) loads Oliverb as it was */
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
    f = e.fx;
    f.olv_diffuse = 230u, f.olv_swing = 870u, f.olv_rate = 40u, f.olv_shimmer = -7;
    f.trim_db = 2;
    mello_engine_fx_set_config(&e, &f);
    f.olv_shimmer = 30, f.olv_diffuse = 2000u;
    mello_engine_fx_set_config(&e, &f);
    CHECK(e.fx.olv_shimmer == 24 && e.fx.olv_diffuse == 1000u, "SHIMMER 30 kept as %d, DIFFUSE 2000 as %u",
          e.fx.olv_shimmer, e.fx.olv_diffuse);
    f.olv_shimmer = -7, f.olv_diffuse = 230u;
    mello_engine_fx_set_config(&e, &f);
    mello_storage_init(&s, &io, NULL);
    mello_storage_request_save(&s, t);
    for (; t <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u) w = mello_storage_poll(&s, &e, t, true);
    CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "not saved");
    load();
    CHECK(e.fx.olv_diffuse == 230u && e.fx.olv_swing == 870u && e.fx.olv_rate == 40u &&
              e.fx.olv_shimmer == -7 && e.fx.trim_db == 2,
          "saved, loaded as DIFFUSE %u, SWING %u, RATE %u, SHIMMER %d, TRIM %d", e.fx.olv_diffuse,
          e.fx.olv_swing, e.fx.olv_rate, e.fx.olv_shimmer, e.fx.trim_db);
    /* the same record, its FX section made version 7: its last 14 bytes go (version 8's 7, the knobs, 9's 5, 10's 2) */
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
            const uint8_t *k = pl + pos + 8u + len - 14u;
            CHECK(ver == 10u && (k[0] | k[1] << 8) == 230 && (k[2] | k[3] << 8) == 870 &&
                      (k[4] | k[5] << 8) == 40 && (int8_t)k[6] == -7,
                  "the FX section is version %u, the knobs not 14 bytes from its end", ver);
            memmove(pl + pos + 8u + len - 14u, pl + pos + 8u + len, /* (the footer after the payload too) */
                    size - (pos + 8u + len) + MELLO_STORAGE_FOOTER_SIZE);
            len -= 14u, size -= 14u;
            put32(pl + pos + 4u, len);
            put32(flash + base + 8u, size);
            pl[pos + 2u] = 7u, patched = 1;
        }
        pos += 8u + len;
    }
    CHECK(patched, "no FX section");
    put32(flash + base + 16u, mello_storage_crc32(pl, size));
    put32(flash + base + 20u, mello_storage_crc32(flash + base, 20u));
    load();
    CHECK(e.fx.olv_diffuse == 650u && e.fx.olv_swing == 100u && e.fx.olv_rate == 500u &&
              e.fx.olv_shimmer == 0 && e.fx.trim_db == 2,
          "saved before the knobs, loaded as DIFFUSE %u, SWING %u, RATE %u, SHIMMER %d, TRIM %d (want 650, "
          "100, "
          "500, 0, 2)",
          e.fx.olv_diffuse, e.fx.olv_swing, e.fx.olv_rate, e.fx.olv_shimmer, e.fx.trim_db);
}

/* ---- the UI: REVERB's layers on SELECT, ROOM, RETURNS, OLIVERB, both ways; the knobs, SHIMMER a semitone a detent */
static void ui(void)
{
    static MelloProject p;
    static MelloTransport t;
    static MelloStorage st;
    static MelloUi u;
    static const uint8_t fwd[4] = {5u, 6u, 7u, 5u}, back[4] = {5u, 7u, 6u, 5u};
    int16_t enc[MELLO_UI_ENCODER_COUNT];
    uint32_t ms = 100u;
    uint8_t i;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 123u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_AUDIO_FX);
    u.fx_subpage = 5u;
    for (i = 1u; i < 4u; ++i) {
        memset(enc, 0, sizeof enc);
        enc[MELLO_UI_ENC_SELECT] = 1;
        mello_ui_input(&u, ms * 1000u, ms, 0u, 0u, 0u, 0u, enc), ms += 50u;
        CHECK(u.fx_subpage == fwd[i], "SELECT on: layer %u, want %u", u.fx_subpage, fwd[i]);
    }
    for (i = 1u; i < 4u; ++i) {
        memset(enc, 0, sizeof enc);
        enc[MELLO_UI_ENC_SELECT] = -1;
        mello_ui_input(&u, ms * 1000u, ms, 0u, 0u, 0u, 0u, enc), ms += 50u;
        CHECK(u.fx_subpage == back[i], "SELECT back: layer %u, want %u", u.fx_subpage, back[i]);
    }
    u.fx_subpage = 7u;
    memset(enc, 0, sizeof enc);
    enc[MELLO_UI_ENC_KNOB1] = 3, enc[MELLO_UI_ENC_KNOB2] = -2, enc[MELLO_UI_ENC_KNOB3] = 5,
    enc[MELLO_UI_ENC_KNOB4] = 3;
    mello_ui_input(&u, ms * 1000u, ms, 0u, 0u, 0u, 0u, enc), ms += 50u;
    {
        const MelloFxConfig *f = &t.engine.fx;
        CHECK(f->olv_diffuse == 680u && f->olv_swing == 80u && f->olv_rate == 550u && f->olv_shimmer == 1,
              "the knobs turned: DIFFUSE %u, SWING %u, RATE %u, SHIMMER %d (want 680, 80, 550, +1: a step a "
              "turn)",
              f->olv_diffuse, f->olv_swing, f->olv_rate, f->olv_shimmer);
    }
}

int main(void)
{
    settings();
    shimmer_dies_away();
    storage();
    ui();
    if (fails) return 1;
    puts("PASS: oliverb_knobs");
    return 0;
}
