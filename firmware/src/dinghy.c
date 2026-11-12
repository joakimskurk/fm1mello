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
#ifdef DINGHY_WEB
/* in a browser: the project in the page's flash (web/app/web_host.c), kept there */
static MelloStorage storage;
static bool storage_ready;
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
static volatile uint16_t audio_load,
    audio_load2; /* (audio_load2: the second core's jobs, as the same share) */
static void app_audio(int32_t *out, uint32_t n)
{
    static uint16_t worst, worst2, blocks;
    uint32_t start = fm1_micros(), t0 = start, took, share;
    if (!audio_ready) return;
#ifdef MELLO_FIXED_CLOCK
    { /* fm1-emu's A/B (DECISIONS "The second core"): the render's time counted in samples from its first block, not
       * read at each interrupt, so two builds whose interrupts take different time place every event alike */
        static uint32_t base, us, rem;
        static bool started;
        if (!started) base = t0, started = true;
        t0 = base + us;
        us += n * 1000000u / 44100u;
        rem += n * 1000000u % 44100u;
        if (rem >= 44100u) rem -= 44100u, us++;
    }
#endif
    mello_audio_render(&audio, out, n, t0, mello_transport_effective_bpm(&mt));
    took = fm1_micros() - start;     /* us; the block is n / 44.1 us long */
    share = took * 441u / (n * 10u); /* permille */
    if (share > worst) worst = (uint16_t)(share > 9999u ? 9999u : share);
    share = audio.parts_busy * 441u / (n * 10u); /* the second core's, this block (0 on one core) */
    audio.parts_busy = 0u;
    if (share > worst2) worst2 = (uint16_t)(share > 9999u ? 9999u : share);
    if (++blocks >= 172u) {
        audio_load = worst, audio_load2 = worst2;
        worst = worst2 = 0u, blocks = 0u;
    }
}

#if !defined(DINGHY_HOST) || defined(DINGHY_WEB)
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
    fm1_led_key_glow(KEY_LED(key_index), on);
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
    mello_audio_set_perf_fx(&audio, mt.engine.perf.fx);
    mello_audio_set_perf_fx2(&audio, mt.engine.perf.fx2, mt.engine.perf.fx2_amt); /* PERFORM's black keys */
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

#ifndef DINGHY_HOST
/* SEED (GLOBAL > SYSTEM; DECISIONS "SEED"): the engine's random numbers, RNDMZR's and the rolls', EVOLVE's, the drums'.
 * AUTO (0, the default): moved at power-on by the chip's random register and TIMER4, and again by the time of the
 * first press or key (a person's, to the 24 MHz tick), so no two power-ons roll the same; a number: that number's, the
 * same at every power-on. Turned, it is taken at once and saved once it rests (a flash erase a detent is too many).
 * On a computer none of this: the host build and its pictures stay as they were. */
static bool seed_first_press; /* AUTO: the first press still to come */
static uint32_t seed_save_ms; /* SEED turned: saved at this time (0: nothing to save) */
static void seed_take(uint16_t n)
{
    if (n) {
        mello_engine_reseed(&mt.engine, mello_seed_of(n));
        seed_first_press = false;
    } else {
        mello_engine_mix_seed(&mt.engine, fm1_rand32() ^ fm1_ticks());
        seed_first_press = true;
    }
}
#endif

/* In a browser (web/app/web_host.c) the page's sound asks for its frames, and app_main runs a pass a call: the start
 * in the first, then the splash's passes, then the loop's, its state kept between them. */
static void app_main(void)
{
#ifdef DINGHY_WEB
    static uint32_t last_render_ms = 0u, last_service_ms = 0xFFFFFFFFu, splash_us, play_us, shown_ms;
    static uint8_t phase; /* 0: to start, 1: the splash, 2: the loop */
    static MelloUiDrawOps draw;
    MelloProject project;
    if (phase == 1u) goto splash;
    if (phase == 2u) goto pass;
    phase = 1u;
    draw = ui_draw_ops();
#else
    uint32_t last_render_ms = 0u, last_service_ms = 0xFFFFFFFFu, splash_us;
    MelloUiDrawOps draw = ui_draw_ops();
    MelloProject project; /* the defaults to start the transport from; the engine keeps its own (B4: 596 B) */
#endif

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
#ifdef DINGHY_WEB
    { /* the project in the page's flash: SAVE and the autosave keep it in the browser */
        MelloStorageIo io;
        memset(&io, 0, sizeof io);
        io.read = mello_flash_read;
        io.erase4k = mello_flash_erase;
        io.program = mello_flash_program;
        mello_storage_init(&storage, &io, 0);
        (void)mello_storage_load(&storage, &mt.engine);
        mello_transport_set_bpm(&mt, mt.engine.project.bpm);
        storage_ready = true;
        mello_ui_init(&ui, &mt, &storage);
    }
#else
    mello_ui_init(&ui, &mt, 0);
#endif
    mello_sample_bank_init(&sample_bank, 0, 0, 0, 0, 0u);
    mello_ui_attach_sample_bank(&ui, &sample_bank);
#endif

    mello_ui_set_mod_view(&ui, audio.mod_view); /* F14: the second thumbs */
#ifndef DINGHY_HOST
    mello_ui_set_cores(&ui, cpu1_state, plat_settings.second_core != 0u); /* GLOBAL > SYSTEM (platform.c) */
    mello_ui_set_seed(&ui, plat_settings.seed);
    seed_take(ui.seed); /* (a saved number past the range is AUTO: mello_ui_set_seed) */
    plat_settings.seed = ui.seed;
#endif
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
#ifndef DINGHY_HOST
#ifndef MELLO_CPU1_RENDER
#define MELLO_CPU1_RENDER 1 /* 0: the second core started as switched, the chords kept here (fm1-emu's A/B) */
#endif
    if (MELLO_CPU1_RENDER && cpu1_on()) { /* CORE 2 ON and running: the chords render there (ROADMAP H2) */
        static const MelloAudioCores two_cores = {cpu1_run, cpu1_wait, cpu1_sync, cpu1_micros};
        mello_audio_set_cores(&audio, &two_cores);
    }
#endif
    audio_ready = true;
    last_running = mt.running;

#ifdef DINGHY_WEB
    play_us = fm1_micros(), shown_ms = 0u;
    return;
splash: /* the splash, as below: a pass a call for 1.5 s, a frame every 30 ms */
    if ((uint32_t)(fm1_micros() - splash_us) < 1500000u) {
        uint32_t now_ms;
        platform_poll();
        now_ms = (uint32_t)(fm1_micros() - play_us) / 1000u;
        if (now_ms - shown_ms >= 30u) {
            mello_ui_splash_frame(&draw, shown_ms, now_ms);
            shown_ms = now_ms;
        }
        return;
    }
    phase = 2u;
    mello_ui_render(&ui, &draw);
    mello_ui_sync_leds(&ui, &draw);
pass: /* the loop's body, once a call */
    do {
#else
    { /* the splash for 1.5 s at least (on the host too: tools/screens.py pictures it), its skyline playing: a frame
       * every 30 ms, the bars that moved */
        uint32_t play_us = fm1_micros(), shown_ms = 0u, now_ms;
        while ((uint32_t)(fm1_micros() - splash_us) < 1500000u) {
            platform_poll();
            now_ms = (uint32_t)(fm1_micros() - play_us) / 1000u;
            if (now_ms - shown_ms >= 30u) {
                mello_ui_splash_frame(&draw, shown_ms, now_ms);
                shown_ms = now_ms;
            }
        }
    }
    mello_ui_render(&ui, &draw);
    mello_ui_sync_leds(&ui, &draw);

    for (;;) {
#endif
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
        mello_ui_set_audio_load2(&ui, audio_load2);
        if (ui.cores != cpu1_state)
            mello_ui_set_cores(&ui, cpu1_state, ui.second_core);   /* (given up: GAVE UP) */
        if (ui.second_core != (plat_settings.second_core != 0u)) { /* CORE 2 turned: for the next power-on */
            plat_settings.second_core = ui.second_core;
            settings_save(&plat_settings, sizeof plat_settings); /* (~50 ms: the flash sector's erase) */
            seed_save_ms = 0u;                                   /* (SEED went with it) */
        }
        if (seed_first_press &&
            (pressed || notes)) { /* SEED AUTO: the first press's time in them, before it acts */
            mello_engine_mix_seed(&mt.engine, fm1_ticks());
            seed_first_press = false;
        }
#endif
        mello_ui_input(&ui, now_us, fm1_ms, pressed, released, fm1_in.buttons, notes, enc);
#ifndef DINGHY_HOST
        if (ui.seed != plat_settings.seed) { /* SEED turned: from now, and saved 2 s after the last detent */
            plat_settings.seed = ui.seed;
            seed_take(ui.seed);
            seed_save_ms = fm1_ms + 2000u;
            if (!seed_save_ms) seed_save_ms = 1u;
        }
        if (seed_save_ms && (int32_t)(fm1_ms - seed_save_ms) >= 0) {
            settings_save(&plat_settings, sizeof plat_settings);
            seed_save_ms = 0u;
        }
#endif
        mello_transport_poll_ahead(&mt, now_us, MELLO_TRANSPORT_LOOKAHEAD_US);
        if (last_running && !mt.running) mello_audio_all_notes_off(&audio);
        last_running = mt.running;
        /* every 8 ms: the compare is ~600 bytes a pass (byte by byte on this libc), the ramps take 50 ms anyway */
        if (!(fm1_ms & 7u)) sync_audio_config();
        mello_ui_tick(&ui);

#if !defined(DINGHY_HOST) || defined(DINGHY_WEB)
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
#ifdef DINGHY_WEB
    } while (0);
#else
    }
#endif
}
