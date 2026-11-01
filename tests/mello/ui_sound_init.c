/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* Feedback 2026-10-30: "the randomizer ruins the synth sound and there is no way back". RNDMZR rolls the pattern by
 * default (the first nine targets), not the sound; OSC 2 is OFF on a new project; SOUND's INIT, pressed twice, puts
 * the sound back to the defaults, and UNDO undoes that. */
#include <stdio.h>
#include <string.h>

#include "mello_ui.h"

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fputc('\n', stderr);                                                                             \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

static uint8_t black_note(uint8_t pos)
{
    uint8_t i, n = 0u;
    for (i = 0u; i < 27u; ++i) {
        if (mello_ui_white_key_position(i) >= 0) continue;
        if (n == pos) return i;
        ++n;
    }
    return 0xFFu;
}

int main(void)
{
    static MelloProject p;
    static MelloTransport t;
    static MelloStorage st;
    static MelloUi u;
    MelloSoundConfig s, d;
    unsigned r;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 3u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_sound_defaults(&d);

    CHECK(t.engine.randomize.mask == MELLO_RAND_DEFAULT && !(t.engine.randomize.mask & MELLO_RAND_WAVE) &&
              !(t.engine.randomize.mask & MELLO_RAND_PROB),
          "the randomizer's default mask is not the first nine");
    for (r = 0u; r < 20u; ++r)
        mello_engine_randomize_steps_intensity(&t.engine,
                                               r & 1u ? MELLO_RANDOMIZE_HIGH : MELLO_RANDOMIZE_MED);
    CHECK(t.engine.sound.osc_wave == MELLO_WAVE_SAW, "a default roll changed the waveform");
    CHECK(d.osc2_wave == MELLO_WAVE_OFF, "OSC 2 is not OFF by default");

    s = t.engine.sound; /* a sound far from the default */
    s.osc_wave = MELLO_WAVE_SQUARE, s.noise_level = 600u, s.unison = 4u, s.filter_cut = 120u;
    mello_engine_sound_set_config(&t.engine, &s);
    mello_ui_set_page(&u, MELLO_UI_SOUND_RESERVED);
    mello_ui_input(&u, 1000000u, 1000u, 0u, 0u, 0u, 1u << black_note(1u), 0); /* INIT is B1 (R3: B0 is ON) */
    CHECK(t.engine.sound.osc_wave == MELLO_WAVE_SQUARE && u.reset_armed,
          "one press of INIT changed the sound");
    mello_ui_input(&u, 2000000u, 2000u, 0u, 0u, 0u, 1u << black_note(1u), 0); /* INIT is B1 (R3: B0 is ON) */
    CHECK(t.engine.sound.osc_wave == MELLO_WAVE_SAW && !t.engine.sound.noise_level &&
              t.engine.sound.unison == 1u && t.engine.sound.filter_cut == d.filter_cut,
          "INIT twice did not restore the default sound");
    mello_ui_input(&u, 3000000u, 3000u, 0u, 0u, 0u, 1u << black_note(9u), 0); /* UNDO */
    CHECK(t.engine.sound.osc_wave == MELLO_WAVE_SQUARE && t.engine.sound.noise_level == 600u,
          "UNDO did not undo INIT");
    puts("PASS: RNDMZR rolls the pattern only by default; OSC 2 OFF; INIT twice restores the sound, UNDO "
         "undoes it");
    return 0;
}
