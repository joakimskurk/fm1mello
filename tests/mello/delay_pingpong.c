/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* PING (ROADMAP F25, DECISIONS "PING"): one short note into the delay alone (300 ms, FDBK 50%). With PING the first
 * echo is on the left, the second on the right, the third on the left again, each the other side's at least 12 dB
 * over, and they fall as without PING (each about FDBK under the one before: -6 dB); without it left and right are
 * the same. The setting survives a save. Built with the output's gain at 0 dB (run_mello.sh). */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"
#include "mello_storage.h"
#include "mello_transport.h"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

static int16_t db[65536], rb[24576];

#define FLASH_SIZE (MELLO_STORAGE_BANK0_ADDR + 2u * MELLO_STORAGE_BANK_SIZE)
static uint8_t flash[FLASH_SIZE];
static bool ram_read(void *c, uint32_t ad, void *dst, uint32_t n)
{
    (void)c;
    if (ad + n > FLASH_SIZE) return false;
    memcpy(dst, flash + ad, n);
    return true;
}
static bool ram_erase4k(void *c, uint32_t ad)
{
    (void)c;
    if (ad % 4096u || ad + 4096u > FLASH_SIZE) return false;
    memset(flash + ad, 0xFF, 4096u);
    return true;
}
static bool ram_program(void *c, uint32_t ad, const void *src, uint32_t n)
{
    const uint8_t *q = src;
    uint32_t i;
    (void)c;
    if (ad + n > FLASH_SIZE) return false;
    for (i = 0u; i < n; ++i) flash[ad + i] &= q[i];
    return true;
}
static const MelloStorageIo io = {ram_read, ram_erase4k, ram_program, NULL};
static MelloAudio a;
static MelloTransport t;
static void sink(void *ctx, const MelloEvent *e, uint32_t due)
{
    (void)mello_audio_push_engine_event((MelloAudio *)ctx, e, due);
}

/* Energy (dB) left and right in the 60 ms around echo n (n * 300 ms after the note). */
static void echoes(bool ping, double l[4], double r[4])
{
    MelloProject p;
    MelloFxConfig fx;
    MelloSoundConfig s;
    uint32_t ms, i;
    int32_t out[96];
    double el[4] = {0}, er[4] = {0};
    mello_project_defaults(&p);
    p.seq_len = 32u;
    for (i = 0; i < MELLO_MAX_STEPS; ++i) p.steps[i].gate = i ? MELLO_GATE_REST : MELLO_GATE_SINGLE;
    mello_transport_init(&t, &p, 1u, 0, 0);
    s = t.engine.sound;
    s.decay_ms = 40u; /* a short note: its echoes stand apart */
    mello_engine_sound_set_config(&t.engine, &s);
    fx = t.engine.fx;
    fx.reverb_wet = 0u;
    fx.delay_wet = 1000u;
    fx.delay_feedback = 500u;
    fx.delay_sync = false;
    fx.delay_time_ms = 300u;
    fx.delay_lowcut = 0u;
    fx.delay_pingpong = ping;
    mello_engine_fx_set_config(&t.engine, &fx);
    mello_audio_init(&a, db, 65536, rb, 24576);
    mello_audio_set_config(&a, &t.engine.sound, &t.engine.fx, &t.engine.sub, &t.engine.sample);
    mello_transport_set_event_sink(&t, sink, &a);
    mello_transport_start(&t, 0u);
    for (ms = 0; ms < 1400u; ++ms) {
        uint32_t fr = 44u + (ms % 10u == 0u ? 1u : 0u);
        int n;
        mello_transport_poll_ahead(&t, ms * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, fr, ms * 1000u, t.engine.project.bpm);
        for (n = 1; n <= 4; ++n)
            if (ms + 10u >= (uint32_t)n * 300u && ms < (uint32_t)n * 300u + 50u)
                for (i = 0; i < fr; ++i)
                    el[n - 1] += (double)out[2 * i] * out[2 * i], er[n - 1] += (double)out[2 * i + 1] * out[2 * i + 1];
    }
    for (i = 0; i < 4u; ++i) l[i] = 10 * log10(el[i] + 1), r[i] = 10 * log10(er[i] + 1);
}

int main(void)
{
    double l[4], r[4], ml[4], mr[4];
    int n;
    echoes(true, l, r);
    echoes(false, ml, mr);
    for (n = 0; n < 4; ++n)
        printf("echo %d: PING left %5.1f right %5.1f dB; without left %5.1f right %5.1f\n", n + 1, l[n], r[n], ml[n],
               mr[n]);
    CHECK(l[0] > r[0] + 12.0, "PING: the first echo is not on the left (%.1f / %.1f)", l[0], r[0]);
    CHECK(r[1] > l[1] + 12.0, "PING: the second echo is not on the right (%.1f / %.1f)", l[1], r[1]);
    CHECK(l[2] > r[2] + 12.0, "PING: the third echo is not on the left (%.1f / %.1f)", l[2], r[2]);
    CHECK(fabs((r[1] - l[0]) - (mr[1] - ml[0])) < 1.5 && fabs((l[2] - r[1]) - (ml[2] - mr[1])) < 1.5,
          "PING's echoes do not fall as without it");
    for (n = 0; n < 4; ++n) CHECK(fabs(ml[n] - mr[n]) < 0.1, "without PING, echo %d differs left and right", n + 1);

    /* saved and read back */
    {
        static MelloEngine e2;
        static MelloStorage st;
        MelloProject p;
        MelloFxConfig fx = t.engine.fx;
        fx.delay_pingpong = true;
        mello_engine_fx_set_config(&t.engine, &fx);
        memset(flash, 0xFF, sizeof flash);
        mello_storage_init(&st, &io, NULL);
        CHECK(mello_storage_save_now(&st, &t.engine, 0u) == MELLO_STORAGE_SAVE_WRITTEN, "the save failed");
        mello_project_defaults(&p);
        mello_engine_init(&e2, &p, 2u);
        mello_storage_init(&st, &io, NULL);
        CHECK(mello_storage_load(&st, &e2) == MELLO_STORAGE_LOAD_OK && e2.fx.delay_pingpong,
              "PING did not survive a save");
    }
    if (fails) return 1;
    puts("PASS: delay_pingpong");
    return 0;
}
