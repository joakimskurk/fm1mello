/* Where the audio's time goes, on this computer (ROADMAP B2). The transport drives the audio as dinghy.c does, in
 * 256-frame blocks, for the default pattern and for the worst case (mello_engine_stress); then the worst case again
 * with one part taken out at a time. Prints the CPU time per second of audio, so 1000 ms = this computer's whole
 * core. The chip's own numbers come from tools/emu_cpu.sh; this table says which parts are worth the work.
 *   make bench            (build/bench/cpu_bench [seconds [worst]]; `worst` alone, for a sampling profiler)
 * Not a test: it has no pass or fail, and its times vary with the machine. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mello_audio.h"
#include "mello_transport.h"

static int16_t delay_pool[MELLO_AUDIO_DELAY_SAMPLES];
static int16_t reverb_pool[MELLO_AUDIO_REVERB_SAMPLES];
static MelloTransport t;
static MelloAudio a;

typedef void (*Tweak)(MelloEngine *e);

static void no_reverb(MelloEngine *e)
{
    MelloFxConfig f = e->fx;
    f.reverb_wet = 0u;
    mello_engine_fx_set_config(e, &f);
}
static void no_delay(MelloEngine *e)
{
    MelloFxConfig f = e->fx;
    f.delay_wet = 0u;
    mello_engine_fx_set_config(e, &f);
}
static void no_dist(MelloEngine *e)
{
    MelloFxConfig f = e->fx;
    f.dist_amount = 0u;
    mello_engine_fx_set_config(e, &f);
}
static void no_drums(MelloEngine *e)
{
    MelloDrumConfig d = e->drums;
    d.on = false;
    mello_engine_drums_set_config(e, &d);
}
static void drums_no_drive(MelloEngine *e)
{
    MelloDrumConfig d = e->drums;
    d.drive = d.fold = 0u;
    mello_engine_drums_set_config(e, &d);
}
static void drums_no_stutter(MelloEngine *e)
{
    MelloDrumConfig d = e->drums;
    d.voice[MELLO_DRUM_GLITCH].tone = 0u; /* BLEEP */
    mello_engine_drums_set_config(e, &d);
}
static void no_chords(MelloEngine *e)
{
#if MELLO_FEATURE_CHORD_MACHINE
    MelloChordConfig c = e->chord;
    c.on = false;
    mello_engine_chord_set_config(e, &c);
#else
    (void)e;
#endif
}
static void chords_no_chorus(MelloEngine *e)
{
#if MELLO_FEATURE_CHORD_MACHINE
    MelloChordConfig c = e->chord;
    c.chorus = 0u;
    mello_engine_chord_set_config(e, &c);
#else
    (void)e;
#endif
}
static void no_sub(MelloEngine *e)
{
    MelloSubConfig s = e->sub;
    MelloBassConfig b = e->bass;
    s.on = false;
    b.on = false;
    mello_engine_sub_set_config(e, &s);
    mello_engine_bass_set_config(e, &b);
}
static void bass_plain_filter(MelloEngine *e)
{
    MelloBassConfig b = e->bass;
    b.res = b.env = 0u;
    mello_engine_bass_set_config(e, &b);
}
static void no_follower_sub(MelloEngine *e)
{
    MelloSoundConfig s = e->sound;
    s.follower_sub_on = false;
    mello_engine_sound_set_config(e, &s);
}
static void lead_plain(MelloEngine *e)
{
    MelloSoundConfig s = e->sound;
    s.unison = 1u;
    s.osc2_mix = 0u;
    s.noise_level = s.noise_fm = s.saturation = 0u;
    mello_engine_sound_set_config(e, &s);
}
static void lead_no_unison(MelloEngine *e)
{
    MelloSoundConfig s = e->sound;
    s.unison = 1u;
    mello_engine_sound_set_config(e, &s);
}
static void lead_no_osc2(MelloEngine *e)
{
    MelloSoundConfig s = e->sound;
    s.osc2_mix = 0u;
    mello_engine_sound_set_config(e, &s);
}
static void no_lfo(MelloEngine *e)
{
    MelloSoundConfig s = e->sound;
    s.lfo_on = false;
    mello_engine_sound_set_config(e, &s);
}
static void no_mods(MelloEngine *e)
{
    uint8_t i;
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) {
        MelloDotMatrixConfig d = e->dot[i];
        d.cells = 0u;
        mello_engine_dot_set_config(e, (MelloDotTarget)i, &d);
    }
}
static void lead_off(MelloEngine *e)
{
    MelloSoundConfig s = e->sound;
    s.osc_wave = MELLO_WAVE_OFF;
    s.follower_sub_on = false;
    mello_engine_sound_set_config(e, &s);
}

/* CPU seconds spent rendering `seconds` of audio. */
static double run(bool stress, Tweak tweak, double seconds)
{
    static MelloProject p;
    static int32_t out[512];
    uint32_t now = 0u, blocks = (uint32_t)(seconds * MELLO_AUDIO_FS / 256.0), b;
    clock_t t0, used = 0;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 0x4D454C4Cu, 0, 0);
    if (stress) mello_engine_stress(&t.engine);
    if (tweak) tweak(&t.engine);
    mello_audio_init(&a, delay_pool, MELLO_AUDIO_DELAY_SAMPLES, reverb_pool, MELLO_AUDIO_REVERB_SAMPLES);
    mello_audio_set_config(&a, &t.engine.sound, &t.engine.fx, &t.engine.sub, &t.engine.sample);
    mello_audio_set_mod_routing(&a, &t.engine.modr);
#if MELLO_FEATURE_CHORD_MACHINE
    mello_audio_set_chord_config(&a, &t.engine.chord);
#endif
    mello_audio_set_drum_config(&a, &t.engine.drums);
    mello_audio_set_bass_config(&a, &t.engine.bass);
    mello_transport_set_event_sink(&t, mello_audio_event_sink, &a);
    mello_transport_set_bpm(&t, t.engine.project.bpm);
    mello_transport_start(&t, 0u);
    for (b = 0u; b < blocks; ++b) {
        mello_transport_poll_ahead(&t, now, MELLO_TRANSPORT_LOOKAHEAD_US);
        memset(out, 0, sizeof out);
        t0 = clock();
        mello_audio_render(&a, out, 256u, now, mello_transport_effective_bpm(&t));
        used += clock() - t0;
        now += 256u * 1000000u / MELLO_AUDIO_FS;
    }
    return (double)used / CLOCKS_PER_SEC;
}

int main(int argc, char **argv)
{
    static const struct {
        const char *name;
        Tweak tweak;
    } parts[] = {{"no reverb", no_reverb},
                 {"no delay", no_delay},
                 {"no distortion", no_dist},
                 {"no drums", no_drums},
                 {"drums: no DRIVE", drums_no_drive},
                 {"drums: GLITCH not STUTTER", drums_no_stutter},
                 {"no chords", no_chords},
                 {"chords: no chorus", chords_no_chorus},
                 {"no sub, no BASS SEQ", no_sub},
                 {"bass: the plain filter", bass_plain_filter},
                 {"no follower sub", no_follower_sub},
                 {"lead: plain saw", lead_plain},
                 {"lead: no unison", lead_no_unison},
                 {"lead: no OSC 2", lead_no_osc2},
                 {"no LFO", no_lfo},
                 {"no MOD lanes", no_mods},
                 {"lead off", lead_off}};
    double seconds = argc > 1 ? atof(argv[1]) : 20.0, base, worst;
    unsigned i;
    if (seconds < 1.0) seconds = 1.0;
    if (argc > 2) { /* `worst`: the worst case alone, for a sampling profiler to watch */
        worst = run(true, 0, seconds);
        printf("%-28s %7.1f ms per second of audio\n", "worst case", 1000.0 * worst / seconds);
        return 0;
    }
    base = run(false, 0, seconds);
    worst = run(true, 0, seconds);
    printf("%-28s %7.1f ms per second of audio\n", "default pattern", 1000.0 * base / seconds);
    printf("%-28s %7.1f ms per second of audio\n", "worst case", 1000.0 * worst / seconds);
    puts("worst case, one part out:        ms/s   saved");
    for (i = 0u; i < sizeof parts / sizeof parts[0]; ++i) {
        double tm = run(true, parts[i].tweak, seconds);
        printf("  %-28s %7.1f %6.1f%%\n", parts[i].name, 1000.0 * tm / seconds, 100.0 * (worst - tm) / worst);
    }
    return 0;
}
