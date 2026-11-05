/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* A send per part (ROADMAP E11, DECISIONS "A send per part"). The defaults send what a project sent before (the lead
 * all of its, the chords and the drums their old SPACE), but a new project's bass: no delay, 20% reverb. A part whose sends are at 0 sounds as with
 * the effects off; at 100% it does not. The bass has left the lead's filter: the lead's CUTOFF no longer changes it.
 * MIX's DELAY and REVERB layers set the sends, and they survive a power cycle. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"
#include "mello_storage.h"
#include "mello_transport.h"
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

static int16_t db[65536], rb[24576];
static MelloAudio a;
static MelloTransport t;
static void sink(void *ctx, const MelloEvent *e, uint32_t due)
{
    (void)mello_audio_push_engine_event((MelloAudio *)ctx, e, due);
}

enum { BASS_ONLY, LEAD_ONLY };
typedef struct {
    int part;
    int16_t send; /* both of the part's sends, or -1 to leave them */
    bool wet_off; /* the effects' WET at 0 */
    int16_t cut;  /* the lead's CUTOFF, or -1 */
} Case;

/* 6 s of the part alone, 100 BPM, the energy (dB) of the last 5; the samples kept (keep, if given) */
#define KEEP (6000u * 45u * 2u)
static int16_t keep_zero[KEEP], keep_full[KEEP];
static double render_keep(const Case *c, int16_t *keep)
{
    MelloProject p;
    MelloFxConfig fx;
    uint32_t ms, i;
    int32_t out[96];
    double acc = 0;
    unsigned n = 0u;
    uint32_t kn = 0u;
    mello_project_defaults(&p);
    p.bpm = 100;
    if (c->part == BASS_ONLY)
        for (i = 0; i < MELLO_MAX_STEPS; ++i) p.steps[i].gate = MELLO_GATE_REST;
    mello_transport_init(&t, &p, 1u, 0, 0);
    if (c->part == BASS_ONLY) {
        MelloSubConfig s = t.engine.sub;
        MelloBassConfig b = t.engine.bass;
        s.on = true;
        b.on = true;
        mello_engine_sub_set_config(&t.engine, &s);
        mello_engine_bass_set_config(&t.engine, &b);
    }
    fx = t.engine.fx;
    if (c->wet_off) fx.delay_wet = fx.reverb_wet = 0u;
    if (c->send >= 0) {
        uint8_t k = c->part == BASS_ONLY ? MELLO_SEND_BASS : MELLO_SEND_LEAD;
        fx.send_dly[k] = fx.send_rev[k] = (uint16_t)c->send;
    }
    mello_engine_fx_set_config(&t.engine, &fx);
    if (c->cut >= 0) {
        MelloSoundConfig s = t.engine.sound;
        s.filter_cut = (uint16_t)c->cut;
        mello_engine_sound_set_config(&t.engine, &s);
    }
    mello_audio_init(&a, db, 65536, rb, 24576);
    mello_audio_set_config(&a, &t.engine.sound, &t.engine.fx, &t.engine.sub, &t.engine.sample);
    mello_transport_set_event_sink(&t, sink, &a);
    mello_transport_start(&t, 0u);
    for (ms = 0; ms < 6000u; ++ms) {
        uint32_t fr = 44u + (ms % 10u == 0u ? 1u : 0u);
        mello_transport_poll_ahead(&t, ms * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, fr, ms * 1000u, t.engine.project.bpm);
        for (i = 0; i < fr * 2u; ++i) {
            if (keep && kn < KEEP)
                keep[kn++] = (int16_t)(out[i] > 32767 ? 32767 : out[i] < -32767 ? -32767 : out[i]);
            if (ms >= 1000u) acc += (double)out[i] * out[i], ++n;
        }
    }
    return 10.0 * log10(acc / n / (32768.0 * 32768.0) + 1e-20);
}
static double render(const Case *c)
{
    return render_keep(c, 0);
}
/* what the sends added: the energy (dB) of the difference of two takes alike but for them */
static double added(void)
{
    double acc = 0;
    uint32_t i;
    for (i = 44100u * 2u; i < KEEP; ++i)
        acc += ((double)keep_full[i] - keep_zero[i]) * ((double)keep_full[i] - keep_zero[i]);
    return 10.0 * log10(acc / (KEEP - 44100u * 2u) / (32768.0 * 32768.0) + 1e-20);
}

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
    (void)ctx;
    if (address + length > FLASH_SIZE) return false;
    memcpy(flash + address, src, length);
    return true;
}

int main(void)
{
    MelloFxConfig d;
    double dry, zero, full, cut_hi, cut_lo, lead_dry, lead_zero, lead_full;

    mello_fx_defaults(&d);
    CHECK(d.send_dly[MELLO_SEND_LEAD] == 1000u && d.send_rev[MELLO_SEND_LEAD] == 1000u &&
              d.send_dly[MELLO_SEND_BASS] == 0u && d.send_rev[MELLO_SEND_BASS] == 200u &&
              d.send_dly[MELLO_SEND_CHORD] == 600u && d.send_rev[MELLO_SEND_CHORD] == 600u &&
              d.send_dly[MELLO_SEND_DRUMS] == 250u && d.send_rev[MELLO_SEND_DRUMS] == 250u,
          "the default sends are not what the parts sent before");

    { /* the bass: its sends at 0 sound as the effects off; at 100% more; the lead's CUTOFF leaves it alone */
        Case c0 = {BASS_ONLY, -1, true, -1}, c1 = {BASS_ONLY, 0, false, -1},
             c2 = {BASS_ONLY, 1000, false, -1}, c3 = {BASS_ONLY, -1, true, 1000},
             c4 = {BASS_ONLY, -1, true, 50};
        dry = render(&c0), zero = render_keep(&c1, keep_zero), full = render_keep(&c2, keep_full);
        cut_hi = render(&c3), cut_lo = render(&c4);
        CHECK(fabs(zero - dry) < 0.01, "the bass with its sends at 0: %.2f dB, with the effects off %.2f",
              zero, dry);
        /* the sends add the effects' returns: what they add, not the sum's level (a held low note's reverb may come
         * back against it: the total can even dip, as it does since the reverb's lines stopped jumping, 0.5-f81) */
        CHECK(added() > dry - 30.0, "the bass with its sends at 100%%: the returns %.2f dB, dry %.2f",
              added(), dry);
        printf("fx_sends: the bass's sends at 100%% add %.2f dB (the sum %.2f, dry %.2f)\n", added(), full,
               dry);
        CHECK(fabs(cut_hi - cut_lo) < 0.01, "the lead's CUTOFF moved the bass: %.2f dB at 100%%, %.2f at 5%%",
              cut_hi, cut_lo);
    }
    { /* the lead the same way */
        Case c0 = {LEAD_ONLY, -1, true, -1}, c1 = {LEAD_ONLY, 0, false, -1},
             c2 = {LEAD_ONLY, 1000, false, -1};
        lead_dry = render(&c0), lead_zero = render(&c1), lead_full = render(&c2);
        CHECK(fabs(lead_zero - lead_dry) < 0.01, "the lead with its sends at 0: %.2f dB, dry %.2f", lead_zero,
              lead_dry);
        CHECK(lead_full - lead_dry > 0.05, "the lead with its sends at 100%%: %.2f dB, dry %.2f", lead_full,
              lead_dry);
    }

    { /* MIX: SELECT to DELAY, K2 the bass's send; SELECT to REVERB, K4 the drums'; saved */
        MelloProject p;
        MelloStorage st;
        MelloStorageIo io;
        MelloUi u;
        MelloStorageLoadResult r;
        MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;
        int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
        uint32_t tt;
        uint16_t bass_dly, drum_rev;
        memset(flash, 0xFF, sizeof flash);
        memset(&io, 0, sizeof io);
        io.read = ram_read, io.erase4k = ram_erase4k, io.program = ram_program;
        mello_project_defaults(&p);
        mello_transport_init(&t, &p, 1u, 0, 0);
        mello_storage_init(&st, &io, NULL);
        (void)mello_storage_load(&st, &t.engine);
        (void)mello_storage_poll(&st, &t.engine, 0u, true);
        mello_ui_init(&u, &t, &st);
        mello_ui_set_page(&u, MELLO_UI_MIX);
        enc[MELLO_UI_ENC_SELECT] = 1;
        mello_ui_input(&u, 100000u, 100u, 0u, 0u, 0u, 0u, enc);
        enc[MELLO_UI_ENC_SELECT] = 0, enc[MELLO_UI_ENC_KNOB2] = 3;
        mello_ui_input(&u, 200000u, 200u, 0u, 0u, 0u, 0u, enc);
        bass_dly = t.engine.fx.send_dly[MELLO_SEND_BASS];
        CHECK(u.mix_subpage == 1u && bass_dly > 0u && t.engine.fx.send_rev[MELLO_SEND_BASS] == 200u,
              "MIX DELAY K2: layer %u, the bass's sends %u / %u", u.mix_subpage,
              t.engine.fx.send_dly[MELLO_SEND_BASS], t.engine.fx.send_rev[MELLO_SEND_BASS]);
        enc[MELLO_UI_ENC_KNOB2] = 0, enc[MELLO_UI_ENC_SELECT] = 1;
        mello_ui_input(&u, 300000u, 300u, 0u, 0u, 0u, 0u, enc);
        enc[MELLO_UI_ENC_SELECT] = 0, enc[MELLO_UI_ENC_KNOB4] = 3;
        mello_ui_input(&u, 400000u, 400u, 0u, 0u, 0u, 0u, enc);
        drum_rev = t.engine.fx.send_rev[MELLO_SEND_DRUMS];
        CHECK(u.mix_subpage == 2u && drum_rev > 250u && t.engine.fx.send_dly[MELLO_SEND_DRUMS] == 250u,
              "MIX REVERB K4: layer %u, the drums' reverb send %u", u.mix_subpage,
              t.engine.fx.send_rev[MELLO_SEND_DRUMS]);
        for (tt = 1000u; tt <= 40000u && w != MELLO_STORAGE_SAVE_WRITTEN; tt += 100u)
            w = mello_storage_poll(&st, &t.engine, tt, true);
        CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "the sends were never saved");
        mello_project_defaults(&p);
        mello_transport_init(&t, &p, 1u, 0, 0);
        mello_storage_init(&st, &io, NULL);
        r = mello_storage_load(&st, &t.engine);
        CHECK(r == MELLO_STORAGE_LOAD_OK && t.engine.fx.send_dly[MELLO_SEND_BASS] == bass_dly &&
                  t.engine.fx.send_rev[MELLO_SEND_DRUMS] == drum_rev,
              "the sends lost in a power cycle (%d: %u, %u)", (int)r, t.engine.fx.send_dly[MELLO_SEND_BASS],
              t.engine.fx.send_rev[MELLO_SEND_DRUMS]);
    }

    printf("fx_sends: bass dry %.2f, sends 0 %.2f, 100%% %.2f; lead dry %.2f, 0 %.2f, 100%% %.2f dB\n", dry,
           zero, full, lead_dry, lead_zero, lead_full);
    if (fails) return 1;
    puts("PASS: fx_sends");
    return 0;
}
