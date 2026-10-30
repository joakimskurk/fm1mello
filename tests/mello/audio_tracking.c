/* Phase 5: velocity and key tracking (DECISIONS.md "Velocity and key tracking"). By default a note is as the HTML
 * plays it; VEL > CUTOFF opens the filter for an accent and leaves a plain step's alone; VEL > LEVEL 0 plays an
 * accent as loud as a plain step; KEY TRACK moves the cutoff with the note from C4 (10.9 permille a semitone at
 * 100%); VEL > MOD ENV doubles an accent's envelope. */
#include <stdio.h>
#include <string.h>

#include "mello_audio.h"

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fputc('\n', stderr);                                                                             \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

static MelloAudio a;

/* A note at velocity vel, rendered ms milliseconds; returns the cutoff the filter used last (mf_cut). */
static uint16_t play(const MelloSoundConfig *s, uint8_t note, uint8_t vel, uint32_t ms)
{
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    MelloEvent e;
    int32_t out[2 * 44];
    uint32_t t;
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sm);
    sub.on = false;
    mello_audio_init(&a, 0, 0u, 0, 0u);
    mello_audio_set_config(&a, s, &fx, &sub, &sm);
    memset(&e, 0, sizeof e);
    e.type = MELLO_EVENT_NOTE_ON;
    e.note = note;
    e.velocity = vel;
    (void)mello_audio_push_engine_event(&a, &e, 0u);
    for (t = 0u; t < ms; ++t) mello_audio_render(&a, out, 44u, t * 1000u, 120u);
    return a.mf_cut;
}

int main(void)
{
    MelloSoundConfig s, base;
    uint16_t plain, accent, lo, hi;
    mello_sound_defaults(&base);
    base.sustain_permille = 1000u;
    CHECK(!base.vel_cutoff && base.vel_level == 1000u && !base.key_track && !base.vel_menv,
          "tracking is on by default");

    plain = play(&base, 57u, 91u, 5u);
    CHECK(a.lead.vel_level == 91u, "a plain step's velocity changed (%u)", a.lead.vel_level);
    accent = play(&base, 57u, 121u, 5u);
    CHECK(a.lead.vel_level == 121u && accent == plain, "by default an accent is not the HTML's");

    s = base;
    s.vel_cutoff = 1000;
    CHECK(play(&s, 57u, 91u, 5u) == plain, "VEL > CUTOFF moved a plain step");
    accent = play(&s, 57u, 121u, 5u);
    CHECK(accent >= plain + 290u && accent <= plain + 300u,
          "VEL > CUTOFF 100%% did not open an accent by 30%% (%u -> %u)", plain, accent);
    s.vel_cutoff = -500;
    CHECK(play(&s, 57u, 121u, 5u) + 140u <= plain, "VEL > CUTOFF -50%% did not close an accent");

    s = base;
    s.vel_level = 0u;
    (void)play(&s, 57u, 121u, 1u);
    CHECK(a.lead.vel_level == 91u, "VEL > LEVEL 0 did not play an accent as a plain step");
    s.vel_level = 2000u;
    (void)play(&s, 57u, 110u, 1u);
    CHECK(a.lead.vel_level == 127u, "VEL > LEVEL 200%% did not push the accent's level (%u)",
          a.lead.vel_level);

    s = base;
    s.key_track = 1000u;
    lo = play(&s, 48u, 91u, 5u);
    hi = play(&s, 72u, 91u, 5u);
    CHECK(hi - lo >= 258u && hi - lo <= 264u, "KEY TRACK 100%% is not 10.9 permille a semitone (%u)",
          hi - lo);
    CHECK(play(&s, 60u, 91u, 5u) == plain, "KEY TRACK moved C4");

    s = base;
    s.menv_amount = 300, s.menv_decay_ms = 2000u, s.filter_cut = 300u;
    plain = play(&s, 57u, 121u, 10u);
    s.vel_menv = 1000;
    accent = play(&s, 57u, 121u, 10u);
    CHECK(accent - 300u >= (plain - 300u) * 2u - 4u,
          "VEL > MOD ENV did not double an accent's envelope (%u, %u)", plain, accent);
    puts("PASS: velocity and key tracking: off by default, VEL > CUTOFF / LEVEL / MOD ENV and KEY TRACK as "
         "stated");
    return 0;
}
