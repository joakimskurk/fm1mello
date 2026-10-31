/* The drums on MIDI out and under the modulation (ROADMAP: "MIDI out for the drums, LFO / DOT / EVOLVE routes to
 * them"). Checked: with NOTE OUT on, each hit is a GM note on channel 10 (kick 36, snare 38, hat 42, glitch 39) with
 * its OFF, and none with NOTE OUT off; the LFO on D.TUNE moves the drums' pitch over time and on D.DRIVE their drive,
 * and with nothing routed a hit is the same whenever it comes. (DOT and EVOLVE reach the same four through the same
 * sum as the LFO; their routing is DOT and EVOLVE's own tests'.) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mello_audio.h"
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

static MelloTransport t;
static MelloAudio a;
static int16_t db[4096], rb[24576];
static unsigned ons[128], offs[128], other_channel;

static void midi(void *ctx, uint8_t st, uint8_t d1, uint8_t d2)
{
    (void)ctx;
    if ((st & 0x0Fu) != 9u) {
        other_channel++;
        return;
    }
    if ((st & 0xF0u) == 0x90u && d2) ons[d1 & 0x7Fu]++;
    if ((st & 0xF0u) == 0x80u) offs[d1 & 0x7Fu]++;
}

/* 4 s of the default beat at 80 BPM, a bar and a third (drums on, the lead's notes off), NOTE OUT on or off. */
static void play(bool note_out)
{
    MelloProject p;
    uint32_t ms;
    unsigned i;
    memset(ons, 0, sizeof ons);
    memset(offs, 0, sizeof offs);
    other_channel = 0u;
    mello_project_defaults(&p);
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) p.steps[i].gate = MELLO_GATE_REST, p.steps[i].sub = false;
    mello_transport_init(&t, &p, 1u, midi, NULL);
    t.engine.drums.on = true;
    t.engine.sub.on = false;
    mello_transport_set_sync(&t, MELLO_SYNC_MASTER, 0u); /* NOTE OUT plays in MIDI MASTER only */
    mello_transport_set_midi_note_out(&t, note_out);
    mello_transport_start(&t, 0u);
    for (ms = 0u; ms < 4000u; ++ms) mello_transport_poll_ahead(&t, ms * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
}

/* One hat hit's pitch, as zero crossings over 30 ms, `at` ms into a render with the LFO on target (or off). */
static int32_t hit_peak;
static unsigned crossings_at(unsigned at_ms, bool lfo_on, MelloLfoTarget target, uint8_t dot_dest)
{
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    MelloDrumConfig d;
    MelloModRouting m;
    MelloEvent ev;
    int32_t out[2 * 441], last = 0;
    unsigned blk, k, n = 0u;
    mello_sound_defaults(&s);
    s.osc_wave = MELLO_WAVE_OFF;
    s.follower_sub_on = false;
    s.lfo_on = lfo_on;
    s.lfo_target = target;
    s.lfo_depth = 1000u;
    s.lfo_rate_millihz = 1000u; /* a second a cycle */
    mello_fx_defaults(&fx);
    fx.delay_wet = fx.reverb_wet = 0u;
    mello_sub_defaults(&sub);
    sub.on = false;
    mello_sample_defaults(&sm);
    mello_drum_defaults(&d);
    d.on = true;
    d.voice[MELLO_DRUM_KICK].decay = 1000u;
    mello_audio_init(&a, db, 4096u, rb, 24576u);
    mello_audio_set_config(&a, &s, &fx, &sub, &sm);
    mello_audio_set_drum_config(&a, &d);
    mello_mod_routing_defaults(&m);
    if (dot_dest) {
        m.dot_target[0] = dot_dest;
        mello_audio_set_mod_routing(&a, &m);
    }
    memset(&ev, 0, sizeof ev);
    ev.type = MELLO_EVENT_DRUM_HIT;
    ev.sample_slot = MELLO_DRUM_KICK;
    ev.note = 64u;
    ev.velocity = 100u;
    (void)mello_audio_push_engine_event(&a, &ev, at_ms * 1000u);
    hit_peak = 0;
    for (blk = 0u; blk < at_ms / 10u + 3u; ++blk) {
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, 441u, blk * 10000u, 120u);
        if (blk * 10u >= at_ms + 10u) /* past the kick's pitch fall */
            for (k = 0u; k < 441u; ++k) {
                int32_t x = out[2u * k];
                n += (x > 0) != (last > 0);
                last = x;
                if (abs(x) > hit_peak) hit_peak = abs(x);
            }
    }
    return n;
}

int main(void)
{
    unsigned plain0, plain1, lfo0, lfo1;
    play(true);
    printf(
        "NOTE OUT on: kick %u/%u, snare %u/%u, hat %u/%u, glitch %u/%u (ON/OFF); %u other messages (clock)\n",
        ons[36], offs[36], ons[38], offs[38], ons[42], offs[42], ons[39], offs[39], other_channel);
    CHECK(ons[36] >= 6u && ons[38] >= 4u && ons[42] >= 12u && ons[39] >= 4u, "too few drum notes");
    CHECK(offs[36] + 1u >= ons[36] && offs[42] + 1u >= ons[42], "drum notes without their OFF");
    play(false);
    CHECK(!ons[36] && !ons[38] && !ons[42] && !ons[39], "drum notes with NOTE OUT off");

    plain0 = crossings_at(0u, false, MELLO_LFO_FILTER, 0u);
    plain1 = crossings_at(250u, false, MELLO_LFO_FILTER, 0u);
    lfo0 = crossings_at(0u, true, MELLO_LFO_DRUM_TUNE, 0u);
    lfo1 = crossings_at(250u, true, MELLO_LFO_DRUM_TUNE, 0u);
    printf("kick's crossings: plain %u, %u; LFO on D.TUNE %u at 0 ms, %u at 250 ms\n", plain0, plain1, lfo0,
           lfo1);
    CHECK(plain0 == plain1, "the plain kick differs by when it is hit (%u, %u)", plain0, plain1);
    CHECK(lfo1 * 2u >= plain1 * 3u, "the LFO on D.TUNE did not raise the kick at its peak (%u vs %u)", lfo1,
          plain1);
    {
        int32_t clean, driven;
        (void)crossings_at(250u, false, MELLO_LFO_FILTER, 0u);
        clean = hit_peak;
        (void)crossings_at(250u, true, MELLO_LFO_DRUM_DRIVE, 0u);
        driven = hit_peak;
        printf("kick's peak: plain %d, LFO on D.DRIVE %d\n", (int)clean, (int)driven);
        CHECK(driven != clean, "the LFO on D.DRIVE did nothing");
    }
    if (fails) return 1;
    puts("PASS: the drums on MIDI out, and moved by the LFO");
    return 0;
}
