/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 zvenson (the Dinghy app skeleton), joakimskurk (SkurkyMello, the app) */
/* SkurkyMello FM-1 Phase 10 shell: parity engine + transport + crash-safe storage,
 * native UI, fixed-point DSP and four-slot compressed sample layer. Audio/sample
 * events are timestamped in main-loop context and consumed inside app_audio().
 */
#ifndef APP_VER
#define APP_VER "dev"
#endif
#ifndef APP_NAME
#define APP_NAME "SKURKYMELLO"
#endif

#include "platform.c"
#include "mello_engine.c"
#include "mello_transport.c"
#include "mello_storage.c"
#include "mello_ui.c"
#include "mello_audio.c"
#include "mello_sample.c"

#ifdef DINGHY_HOST
#ifndef MELLO_HOST_HAS_MICROS
static uint32_t fm1_micros(void)
{
    return fm1_ms * 1000u;
}
#endif
#endif

static MelloTransport mt;
static MelloUi ui;
static MelloAudio audio;
static bool audio_ready;
static bool last_running;
static MelloSampleBank sample_bank;
#ifdef DINGHY_HOST
#define POOL_SECTION /* plain statics on the host: the delay and the reverb render there too (2026-11-01) */
#else
#define POOL_SECTION __attribute__((section(".pool")))
#endif
static int16_t audio_delay_pool[MELLO_AUDIO_DELAY_SAMPLES] POOL_SECTION;
static int16_t audio_reverb_pool[MELLO_AUDIO_REVERB_SAMPLES] POOL_SECTION;
#ifndef DINGHY_HOST
static uint8_t sample_pool0[MELLO_SAMPLE_DATA_MAX] __attribute__((section(".pool")));
static uint8_t sample_pool1[MELLO_SAMPLE_DATA_MAX] __attribute__((section(".pool")));
static uint8_t sample_pool2[MELLO_SAMPLE_DATA_MAX] __attribute__((section(".pool")));
static uint8_t sample_pool3[MELLO_SAMPLE_DATA_MAX] __attribute__((section(".pool")));
static MelloStorage storage;
static MelloSampleStore sample_store;
static MelloSampleTransfer sample_xfer;
static bool storage_ready;
static bool sample_ready;
#endif

static void midi_out_cb(void *ctx, uint8_t st, uint8_t d1, uint8_t d2)
{
    (void)ctx;
    midi_send(st, d1, d2);
}

static void sample_midi_out_cb(void *ctx, uint8_t st, uint8_t d1, uint8_t d2)
{
    (void)ctx;
    midi_send(st, d1, d2);
}

#ifndef DINGHY_HOST
static void sample_idle_cb(void *ctx)
{
    (void)ctx;
    platform_poll();
}
#endif

/* The chip's audio load (ROADMAP B1): the render's share of each block's time, the worst block of every ~1 s
 * (172 blocks), in permille. The one number the emulator cannot give: it counts instructions, the chip cycles. */
static volatile uint16_t audio_load;
static void app_audio(int32_t *out, uint32_t n)
{
    static uint16_t worst, blocks;
    uint32_t t0 = fm1_micros(), took, share;
    if (!audio_ready) return;
    mello_audio_render(&audio, out, n, t0, mello_transport_effective_bpm(&mt));
    took = fm1_micros() - t0;        /* us; the block is n / 44.1 us long */
    share = took * 441u / (n * 10u); /* permille */
    if (share > worst) worst = (uint16_t)(share > 9999u ? 9999u : share);
    if (++blocks >= 172u) {
        audio_load = worst;
        worst = 0u, blocks = 0u;
    }
}

#ifndef DINGHY_HOST
static bool mello_flash_read(void *ctx, uint32_t address, void *dst, uint32_t length)
{
    (void)ctx;
    return st_read(address, dst, length) == 0;
}
static bool mello_flash_erase(void *ctx, uint32_t address)
{
    uint32_t took = 0u;
    (void)ctx;
    return fl_erase4k(address, &took) == 0;
}
static bool mello_flash_program(void *ctx, uint32_t address, const void *src, uint32_t length)
{
    (void)ctx;
    return fl_write(address, src, length) == 0;
}
#endif

static void ui_fill(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour)
{
    (void)ctx;
    lcd_fill(x, y, w, h, colour);
}
static void ui_text(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large, const char *text,
                    uint16_t colour, uint8_t align)
{
    (void)ctx;
    draw_text_box(x, y, w, large ? &FONT_L : &FONT_S, text, colour, align);
}
static void ui_text_on(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t style, const char *text,
                       uint16_t colour, uint16_t bg, uint8_t align)
{
    (void)ctx;
    draw_text_box_on(x, y, w, style == 1u ? &FONT_L : &FONT_S, text, colour, bg, align, style == 2u ? 1 : 0);
}
static void ui_key_led(void *ctx, uint8_t key_index, bool on)
{
    (void)ctx;
    fm1_led_key(KEY_LED(key_index), on);
}
static void ui_key_glow(void *ctx, uint8_t key_index, bool on)
{
    (void)ctx;
    fm1_led_glow_key(KEY_LED(key_index), on);
}
static void ui_button_led(void *ctx, uint8_t button_id, bool on)
{
    (void)ctx;
    fm1_led_key(button_id, on);
}

static MelloUiDrawOps ui_draw_ops(void)
{
    MelloUiDrawOps o;
    o.fill = ui_fill;
    o.text = ui_text;
    o.key_led = ui_key_led;
    o.button_led = ui_button_led;
    o.ctx = 0;
    o.text_on = ui_text_on;
    o.key_glow = ui_key_glow;
    return o;
}

/* The engine's configs into the audio when they change. What the audio was last given is its own pending_*
 * copies (the main loop alone writes them), so there is no second set here to compare against (B4: ~560 B). */
static void sync_audio_config(void)
{
    MelloAudioDrumKit kit_now;
    MelloAudioBassVoice bass_now;
    if (!audio_ready) return;
    mello_audio_set_sub_mute(&audio, mt.engine.perf.sub_mute);
    mello_audio_set_mix_mute(&audio, mt.engine.perf.mix_mute);
    mello_audio_drum_kit(&mt.engine.drums, &kit_now);
    mello_audio_bass_voice(&mt.engine.bass, &bass_now);
    if (memcmp(&audio.pending_config, &mt.engine.sound, sizeof audio.pending_config) ||
        memcmp(&audio.pending_fx, &mt.engine.fx, sizeof audio.pending_fx) ||
        memcmp(&audio.pending_sub_config, &mt.engine.sub, sizeof audio.pending_sub_config) ||
#if MELLO_FEATURE_CHORD_MACHINE
        memcmp(&audio.pending_chord_config, &mt.engine.chord, sizeof audio.pending_chord_config) ||
#endif
        memcmp(&audio.pending_modr, &mt.engine.modr, sizeof audio.pending_modr) ||
        memcmp(&audio.pending_sample_config, &mt.engine.sample, sizeof audio.pending_sample_config) ||
        memcmp(&audio.pending_drum_config, &kit_now, sizeof kit_now) ||
        memcmp(&audio.pending_bass_config, &bass_now, sizeof bass_now)) {
        mello_audio_set_drum_config(&audio, &mt.engine.drums);
        mello_audio_set_bass_config(&audio, &mt.engine.bass);
        mello_audio_set_config(&audio, &mt.engine.sound, &mt.engine.fx, &mt.engine.sub, &mt.engine.sample);
        mello_audio_set_mod_routing(&audio, &mt.engine.modr);
#if MELLO_FEATURE_CHORD_MACHINE
        mello_audio_set_chord_config(&audio, &mt.engine.chord);
#endif
    }
}

static void app_main(void)
{
    uint32_t last_render_ms = 0u, last_service_ms = 0xFFFFFFFFu, splash_us;
    MelloUiDrawOps draw = ui_draw_ops();
    MelloProject project; /* the defaults to start the transport from; the engine keeps its own (B4: 596 B) */

    mello_ui_draw_splash(&draw, APP_VER); /* while storage and the samples load */
    splash_us = fm1_micros();

    mello_project_defaults(&project);
    mello_transport_init(&mt, &project, 0x4D454C4Cu, midi_out_cb, 0);
    mello_transport_set_midi_channel(&mt, 1u);
    mello_transport_set_midi_sub_channel(&mt, 2u);

#ifndef DINGHY_HOST
    {
        MelloStorageIo io;
        memset(&io, 0, sizeof io);
        io.read = mello_flash_read;
        io.erase4k = mello_flash_erase;
        io.program = mello_flash_program;
        mello_storage_init(&storage, &io, 0);
        (void)mello_storage_load(&storage, &mt.engine);
#ifdef MELLO_STRESS
        mello_engine_stress(
            &mt.engine); /* a measuring build: the worst case, whatever was saved (tools/emu_cpu.sh) */
#endif
        mello_transport_set_bpm(&mt, mt.engine.project.bpm);
        storage_ready = true;
        mello_ui_init(&ui, &mt, &storage);
        {
            MelloSampleIo sio;
            memset(&sio, 0, sizeof sio);
            sio.read = mello_flash_read;
            sio.erase4k = mello_flash_erase;
            sio.program = mello_flash_program;
            sio.idle = sample_idle_cb;
            mello_sample_bank_init(&sample_bank, sample_pool0, sample_pool1, sample_pool2, sample_pool3,
                                   MELLO_SAMPLE_DATA_MAX);
            mello_sample_store_init(&sample_store, &sio, &sample_bank);
            (void)mello_sample_store_load_all(&sample_store);
            mello_sample_transfer_init(&sample_xfer, &sample_store, sample_midi_out_cb, 0);
            sample_ready = true;
            mello_ui_attach_sample_bank(&ui, &sample_bank);
        }
    }
#else
    mello_ui_init(&ui, &mt, 0);
    mello_sample_bank_init(&sample_bank, 0, 0, 0, 0, 0u);
    mello_ui_attach_sample_bank(&ui, &sample_bank);
#endif

    mello_ui_set_mod_view(&ui, audio.mod_view); /* F14: the second thumbs */
    mello_audio_init(&audio, audio_delay_pool, MELLO_AUDIO_DELAY_SAMPLES, audio_reverb_pool,
                     MELLO_AUDIO_REVERB_SAMPLES);
    mello_audio_attach_sample_bank(&audio, &sample_bank);
    mello_audio_set_config(&audio, &mt.engine.sound, &mt.engine.fx, &mt.engine.sub, &mt.engine.sample);
    mello_audio_set_mod_routing(&audio, &mt.engine.modr);
#if MELLO_FEATURE_CHORD_MACHINE
    mello_audio_set_chord_config(&audio, &mt.engine.chord);
#endif
    mello_audio_set_drum_config(&audio, &mt.engine.drums);
    mello_audio_set_bass_config(&audio, &mt.engine.bass);
    mello_transport_set_event_sink(&mt, mello_audio_event_sink, &audio);
    audio_ready = true;
    last_running = mt.running;

    while ((uint32_t)(fm1_micros() - splash_us) < 1500000u)
        platform_poll(); /* the splash for 1.5 s at least (on the host too: tools/screens.py pictures it) */
    mello_ui_render(&ui, &draw);
    mello_ui_sync_leds(&ui, &draw);

    for (;;) {
        uint32_t now_us, pressed, released, notes;
        uint8_t st, d1, d2;
        int16_t enc[MELLO_UI_ENCODER_COUNT];
        uint8_t i;

        platform_poll();
        now_us = fm1_micros();

        while (midi_get(&st, &d1, &d2)) {
#ifndef DINGHY_HOST
            if (sample_ready && mello_sample_transfer_midi_in(&sample_xfer, st, d1, d2,
                                                              !mt.running && mello_audio_is_silent(&audio))) {
                ui.dirty |= MELLO_UI_DIRTY_BODY | MELLO_UI_DIRTY_FOOTER | MELLO_UI_DIRTY_LEDS;
                continue;
            }
#endif
            mello_transport_midi_in(&mt, st, d1, d2, now_us);
        }

        /* The rest once a millisecond, not on every wake (the 10 kHz key-scan timer wakes the loop): keys are
         * debounced in that interrupt and their edges wait, the transport looks 15 ms ahead, the screen is served at
         * 50 Hz. It used to run 10 000 times a second, ~35 M instructions an emulated second, half the emulator's
         * load besides the sound. */
        if (fm1_ms == last_service_ms) {
#ifndef DINGHY_HOST
            fm1_idle();
#endif
            continue;
        }
        last_service_ms = fm1_ms;

        pressed = fm1_input_edges(&released);
        notes = fm1_input_note_edges();
        for (i = 0u; i < MELLO_UI_ENCODER_COUNT; ++i) enc[i] = (int16_t)fm1_enc_take(i);

        mello_ui_set_held_notes(&ui, fm1_in.notes);
#ifndef DINGHY_HOST
        mello_ui_set_audio_load(&ui, audio_load); /* (the host's clock has milliseconds: nothing to show) */
#endif
        mello_ui_input(&ui, now_us, fm1_ms, pressed, released, fm1_in.buttons, notes, enc);
        mello_transport_poll_ahead(&mt, now_us, MELLO_TRANSPORT_LOOKAHEAD_US);
        if (last_running && !mt.running) mello_audio_all_notes_off(&audio);
        last_running = mt.running;
        /* every 8 ms: the compare is ~600 bytes a pass (byte by byte on this libc), the ramps take 50 ms anyway */
        if (!(fm1_ms & 7u)) sync_audio_config();
        mello_ui_tick(&ui);

#ifndef DINGHY_HOST
        if (storage_ready)
            (void)mello_storage_poll(&storage, &mt.engine, fm1_ms,
                                     !mt.running && mello_audio_is_silent(&audio));
#endif

        { /* Big screen: a new viewer wants everything; rows the queue had no room for want their zone again */
            uint16_t md = mirror_take_dirty();
            if (md) ui.dirty |= md == MIRROR_DIRTY_ALL ? MELLO_UI_DIRTY_ALL : md;
        }
        if (mello_ui_dirty(&ui) & MELLO_UI_DIRTY_LEDS) mello_ui_sync_leds(&ui, &draw);

        /* At most 50 Hz UI service. Dirty regions mean this is not a 240x240 redraw loop. */
        if ((uint32_t)(fm1_ms - last_render_ms) >= 20u && mello_ui_dirty(&ui)) {
            last_render_ms = fm1_ms;
            mello_ui_render(&ui, &draw);
        }

#ifndef DINGHY_HOST
        /* Sleep until the next interrupt (the 10 kHz timer at the latest), as JieLi's SDK idles: everything the
         * loop serves arrives by interrupt, and an emulator skips the sleep instead of spinning through it. */
        fm1_idle();
#endif
    }
}
