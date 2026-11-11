/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* DECAY is the time to -60 dB (DECISIONS "DECAY in seconds", 0.5-f119), in V3 and in Oliverb: a second of noise into
 * each, and the network's output after it falls (its energy left, Schroeder's backward sum, from -5 to -25 dB, x 3)
 * in DECAY's time within 10%: V3 from 0.4 s to 30 s, Oliverb from 2.5 s (under that its loop's allpasses ring on
 * their own: 1 s rings under 1.3 s, 0.4 s under 0.75 s). Before, as the HTML's Tone.Reverb, DECAY 13 s fell 60 dB in
 * 3.4 s. And a project saved with Tone.Reverb's DECAY (its FX section before version 7) loads as the time it gave, 6 s
 * as 2.5 s, so it sounds as it did; one saved since keeps its DECAY. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"
#include "mello_storage.h"

#if !defined(MELLO_REVERB_TEST_HOOK) || !MELLO_REVERB_TEST_HOOK
#error "reverb_decay needs MELLO_REVERB_TEST_HOOK=1"
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

/* ---- the time to -60 dB */
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

#define FS    44100
#define N_MAX (40 * FS)
static int16_t delay_buf[4096], reverb_buf[MELLO_AUDIO_REVERB_SAMPLES];
static double edc[N_MAX];
static MelloAudio a;

static double decay_time(int oliverb, uint16_t decay_ms, uint16_t size)
{
    MelloSoundConfig sound;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sample;
    int32_t out[2], l, r;
    uint32_t seed = 12345u, n, len = FS + (uint32_t)decay_ms * FS / 1000u * 8u / 10u + 2u * FS, t5 = 0u,
             t25 = 0u;
    double sum = 0.0;
    if (len > N_MAX) len = N_MAX;
    mello_sound_defaults(&sound);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sample);
    fx.delay_wet = 0u, fx.delay_level = 0u, fx.dist_amount = 0u;
    fx.reverb_wet = 1000u, fx.reverb_level = 1000u, fx.reverb_decay_ms = decay_ms, fx.reverb_size = size;
    mello_audio_init(&a, delay_buf, 4096u, reverb_buf, MELLO_AUDIO_REVERB_SAMPLES);
    mello_audio_set_config(&a, &sound, &fx, &sub, &sample);
    mello_audio_render(&a, out, 1u, 0u, 80u);
    if (oliverb) {
        mello_audio_set_cores(&a, &CORES);
        mello_audio_reverb_choose_probe(&a);
    }
    CHECK(a.oliverb_on == !!oliverb, "%s not chosen", oliverb ? "Oliverb" : "V3");
    for (n = 0u; n < len; ++n) {
        int32_t in = 0;
        if (n < (uint32_t)FS) seed = seed * 1664525u + 1013904223u, in = ((int32_t)(seed >> 20) - 2048) * 2;
        mello_audio_reverb_stereo_probe(&a, in, &l, &r);
        edc[n] = (double)l * l + (double)r * r;
    }
    for (n = len; n-- > (uint32_t)FS;) sum += edc[n], edc[n] = sum;
    for (n = FS; n < len && !t25; ++n) {
        double d = 10.0 * log10(edc[n] / edc[FS] + 1e-30);
        if (!t5 && d < -5.0) t5 = n;
        if (d < -25.0) t25 = n;
    }
    return t25 ? 3.0 * (double)(t25 - t5) / FS : -1.0;
}

/* ---- a project saved before version 7 of the FX section */
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
static uint16_t loaded_decay(void)
{
    static MelloProject p;
    MelloStorage s;
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 2u);
    mello_storage_init(&s, &io, NULL);
    (void)mello_storage_load(&s, &e);
    return e.fx.reverb_decay_ms;
}
static void old_project(void)
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
    CHECK(e.fx.reverb_decay_ms == 2500u, "a new project's DECAY is %u", e.fx.reverb_decay_ms);
    f = e.fx;
    f.reverb_decay_ms = 6000u;
    mello_engine_fx_set_config(&e, &f);
    mello_storage_init(&s, &io, NULL);
    mello_storage_request_save(&s, t);
    for (; t <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u) w = mello_storage_poll(&s, &e, t, true);
    CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "not saved");
    CHECK(loaded_decay() == 6000u, "saved now, DECAY 6 s loads as %u", e.fx.reverb_decay_ms);
    /* the same record, its FX section made version 6 (the bytes are the same but for the version) */
    if (get32(flash + MELLO_STORAGE_BANK0_ADDR) == MELLO_STORAGE_MAGIC) base = MELLO_STORAGE_BANK0_ADDR;
    if (get32(flash + MELLO_STORAGE_BANK1_ADDR) == MELLO_STORAGE_MAGIC) base = MELLO_STORAGE_BANK1_ADDR;
    CHECK(base != 0u, "no record");
    if (!base) return;
    size = get32(flash + base + 8u);
    pl = flash + base + MELLO_STORAGE_HEADER_SIZE;
    for (pos = 0u; pos + 8u <= size;) {
        uint32_t id = pl[pos] | (uint32_t)pl[pos + 1u] << 8, ver = pl[pos + 2u] | (uint32_t)pl[pos + 3u] << 8,
                 len = get32(pl + pos + 4u);
        if (id == 5u) { /* SEC_FX: DECAY at byte 26 (after DIST's five, the delay's seven, WET and SIZE) */
            CHECK(ver == 7u && (pl[pos + 8u + 26u] | pl[pos + 8u + 27u] << 8) == 6000,
                  "the FX section is version %u, DECAY at byte 26 %u", ver,
                  pl[pos + 8u + 26u] | pl[pos + 8u + 27u] << 8);
            pl[pos + 2u] = 6u, patched = 1;
        }
        pos += 8u + len;
    }
    CHECK(patched, "no FX section");
    put32(flash + base + 16u, mello_storage_crc32(pl, size));
    put32(flash + base + 20u, mello_storage_crc32(flash + base, 20u));
    CHECK(loaded_decay() == 2500u, "saved with Tone.Reverb's DECAY 6 s, it loads as %u (2500: as it sounded)",
          e.fx.reverb_decay_ms);
}

int main(void)
{
    static const uint16_t D[6] = {400u, 1000u, 2500u, 6000u, 13000u, 30000u};
    unsigned k;
    int olv;
    for (k = 0u; k < 6u; ++k)
        for (olv = 0; olv < 2; ++olv) {
            uint16_t size = D[k] >= 30000u ? 1000u : 700u;
            double t = decay_time(olv, D[k], size), want = D[k] / 1000.0,
                   hi = !olv || D[k] >= 2500u ? 1.1 * want
                        : D[k] >= 1000u       ? 1.3
                                              : 0.75;
            CHECK(t > 0.9 * want && t < hi, "%s, DECAY %.1f s, SIZE %u: 60 dB in %.2f s",
                  olv ? "Oliverb" : "V3", want, size, t);
            printf("%-7s DECAY %4.1f s: 60 dB in %5.2f s\n", olv ? "Oliverb" : "V3", want, t);
        }
    CHECK(mello_reverb_decay_from_tone(6000u) == 2500u && mello_reverb_decay_from_tone(13000u) == 3400u &&
              mello_reverb_decay_from_tone(60000u) == 5400u && mello_reverb_decay_from_tone(400u) == 400u,
          "Tone.Reverb's DECAY as its time: 6 s %u, 13 s %u, 60 s %u, 0.4 s %u",
          mello_reverb_decay_from_tone(6000u), mello_reverb_decay_from_tone(13000u),
          mello_reverb_decay_from_tone(60000u), mello_reverb_decay_from_tone(400u));
    old_project();
    if (fails) return 1;
    puts("PASS: reverb_decay");
    return 0;
}
