/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The website's take: one performance on the firmware's own code (the UI, the engine, the transport and the audio,
 * firmware/src/mello_*.c), played the way one person plays one FM-1 and rendered on a computer.
 *
 * A fresh project on GROOVE at 120 BPM. RNDMZR is pressed a bar before each change, so the whole band rolls into a
 * new style every four bars, and a PERFORM key is held the last 4 or 8 beats into it. EVOLVE moves the gate length,
 * the filter and the decay, with its DEST A on the chords' cutoff, set anew each section; the lead's knobs are
 * turned in each. (The demo of 2026-11-07, 0.5-f98, made after the performance of 0.5-f90.)
 *
 *   cc -std=c11 -O2 -Ifirmware/src web/take.c firmware/src/mello_*.c -lm -o take && ./take take.wav
 *
 * Out: a 44.1 kHz stereo WAV, and on stdout what happens when, one line each ("  12.34  kind  what"): style (the
 * style that plays from then), hold (a PERFORM key held, with its length), mute, key, evolve and knob.
 * web/make_site.py makes the page's chapters from the style, hold and mute lines. SEED= picks another start (24 is the take's). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mello_audio.h"
#include "mello_storage.h"
#include "mello_transport.h"
#include "mello_ui.h"

#define SECS     84u
#define FS       44100u
#define BPM      120u
#define SEED     24u
#define FADE_MS  2500u

static int16_t delay_buf[65536], reverb_buf[24576];
static int16_t pcm[2u * FS * SECS + 2u * 64u];
static MelloAudio a;
static MelloTransport t;
static MelloStorage st;
static MelloUi u;

static void sink(void *c, const MelloEvent *e, uint32_t due) { (void)mello_audio_push_engine_event((MelloAudio *)c, e, due); }

/* the engine's configs into the audio when they change, as the firmware does (dinghy.c sync_audio_config) */
static void sync_audio_config(void)
{
    MelloEngine *e = &t.engine;
    MelloAudioDrumKit kit;
    MelloAudioBassVoice bass;
    mello_audio_set_sub_mute(&a, e->perf.sub_mute);
    mello_audio_set_mix_mute(&a, e->perf.mix_mute);
    mello_audio_set_perf_fx(&a, e->perf.fx);
    mello_audio_set_perf_fx2(&a, e->perf.fx2, e->perf.fx2_amt);
    mello_audio_drum_kit(&e->drums, &kit);
    mello_audio_bass_voice(&e->bass, &bass);
    if (memcmp(&a.pending_config, &e->sound, sizeof a.pending_config) || memcmp(&a.pending_fx, &e->fx, sizeof a.pending_fx) ||
        memcmp(&a.pending_sub_config, &e->sub, sizeof a.pending_sub_config) ||
        memcmp(&a.pending_chord_config, &e->chord, sizeof a.pending_chord_config) ||
        memcmp(&a.pending_modr, &e->modr, sizeof a.pending_modr) ||
        memcmp(&a.pending_sample_config, &e->sample, sizeof a.pending_sample_config) ||
        memcmp(&a.pending_drum_config, &kit, sizeof kit) || memcmp(&a.pending_bass_config, &bass, sizeof bass)) {
        mello_audio_set_drum_config(&a, &e->drums);
        mello_audio_set_bass_config(&a, &e->bass);
        mello_audio_set_config(&a, &e->sound, &e->fx, &e->sub, &e->sample);
        mello_audio_set_mod_routing(&a, &e->modr);
        mello_audio_set_chord_config(&a, &e->chord);
    }
}

/* the keys: white 0..15 (PERFORM's 4 x 4, its rows in order), black B(0)..B(10) as the UI counts them */
static const uint8_t BLACK[11] = {1u, 3u, 5u, 8u, 10u, 13u, 15u, 17u, 20u, 22u, 25u};
enum { FILL, ARP, REVERSE, ACCENT, REP8, REP4, REP2, REP1, LP_SWP, HP_SWP, THROW, WASH, LD_OFF, BS_OFF, CH_OFF, DR_OFF };
#define B(n)    (100 + (n))
#define STUTTER B(0)
#define TAPE    B(1)
#define PLUCK   B(2)
#define CRUSH   B(3)
#define CHOP    B(4)
#define OCT_DN  B(5)
#define OCT_UP  B(6)
#define WARP    B(7)
#define RNDMZR  B(9)
#define PERF    B(10)
#define PLAY    999

/* PLAY at 1 s: bar k's one at 1 + 2k s (120 BPM), a section four bars (8 s). RNDMZR goes a bar before the change, so
 * the roll lands on the next section's one; the effects are held the last 4 or 8 beats into it. kind: "hold" for a
 * PERFORM key held and "mute" for a part muted (the page lists both), "key" for the rest. */
typedef struct {
    uint32_t t0, t1;
    int key;
    const char *kind, *name;
} Press;
static const Press PR[] = {
    {200, 260, RNDMZR, "key", "RNDMZR, stopped: a band at once (GROOVE, STYLE ANY)"},
    {1000, 1060, PLAY, "key", "PLAY"},
    {1300, 1360, PERF, "key", "PERF: PERFORM open"},
    {5000, 9000, FILL, "hold", "FILL"},
    {7000, 9000, ACCENT, "hold", "ACCENT"},
    {7100, 7160, RNDMZR, "key", "RNDMZR"},
    {15000, 17000, STUTTER, "hold", "STUTTER"},
    {15000, 17000, CRUSH, "hold", "CRUSH"},
    {15100, 15160, RNDMZR, "key", "RNDMZR"},
    {21000, 25000, HP_SWP, "hold", "HP SWP"},
    {23000, 24000, REP8, "hold", "REP 8"},
    {24000, 24500, REP4, "hold", "REP 4"},
    {24500, 24750, REP2, "hold", "REP 2"},
    {24750, 25000, REP1, "hold", "REP 1"},
    {23100, 23160, RNDMZR, "key", "RNDMZR"},
    {29000, 33000, WARP, "hold", "WARP"},
    {31000, 33000, OCT_DN, "hold", "OCT DOWN"},
    {31100, 31160, RNDMZR, "key", "RNDMZR"},
    {39000, 41000, LP_SWP, "hold", "LP SWP"},
    {40300, 41000, TAPE, "hold", "TAPE STOP"},
    {39100, 39160, RNDMZR, "key", "RNDMZR"},
    {45000, 45060, DR_OFF, "mute", "DR OFF"},
    {47000, 49000, REVERSE, "hold", "REVERSE"},
    {47000, 49000, THROW, "hold", "THROW"},
    {48900, 48960, DR_OFF, "key", "DR back on the one"},
    {47100, 47160, RNDMZR, "key", "RNDMZR"},
    {53000, 57000, PLUCK, "hold", "PLUCK"},
    {55000, 57000, CHOP, "hold", "CHOP"},
    {55100, 55160, RNDMZR, "key", "RNDMZR"},
    {59000, 59060, BS_OFF, "mute", "BS OFF"},
    {59100, 59160, CH_OFF, "mute", "CH OFF"},
    {63000, 65000, WASH, "hold", "WASH"},
    {63000, 65000, OCT_UP, "hold", "OCT UP"},
    {64900, 64960, BS_OFF, "key", "BS back"},
    {64910, 64970, CH_OFF, "key", "CH back"},
    {63100, 63160, RNDMZR, "key", "RNDMZR"},
    {67000, 67060, LD_OFF, "mute", "LD OFF"},
    {68900, 68960, LD_OFF, "key", "LD back"},
    {69000, 73000, ARP, "hold", "ARP"},
    {69000, 73000, FILL, "hold", "FILL"},
    {72500, 73000, STUTTER, "hold", "STUTTER"},
    {71100, 71160, RNDMZR, "key", "RNDMZR"},
    {80000, 81000, TAPE, "hold", "TAPE STOP"},
};
#define NPR (sizeof PR / sizeof PR[0])

/* EVOLVE as on its pages (MAIN: MUTATE, LENGTH, RATE, LOOP; ROUTES: GATE LENGTH, FILTER, DECAY; DEPTH; SOUND: DEST A
 * and B), set anew just after each section's one, so each section moves its own way */
#define ROUTES (MELLO_TM_ROUTE_LENGTH | MELLO_TM_ROUTE_FILTER | MELLO_TM_ROUTE_DECAY)
typedef struct {
    uint32_t t;
    uint16_t mutate;            /* permille */
    uint8_t length, rate, loop, reroll;
    uint16_t gate, filter, decay; /* the depths */
    uint8_t dest_a;
    int16_t depth_a;
    uint8_t dest_b;
    int16_t depth_b;
    const char *what;
} Evo;
static const Evo EV[] = {
    {1000, 100, 8, MELLO_TM_RATE_ONE, 0, 0, 350, 450, 400, MELLO_DEST_CHORD_CUT, -500, MELLO_DEST_OFF, 0,
     "on: MUTATE 10%, LENGTH 8; GATE 35 FILTER 45 DECAY 40; A chords' CUT -50"},
    {9050, 200, 16, MELLO_TM_RATE_ONE, 0, 1, 450, 600, 500, MELLO_DEST_CHORD_CUT, -600, MELLO_DEST_OFF, 0,
     "RND, MUTATE 20%, LENGTH 16; FILTER 60"},
    {17050, 0, 16, MELLO_TM_RATE_ONE, 1, 1, 550, 600, 600, MELLO_DEST_CHORD_CUT, -700, MELLO_DEST_CHORD_LEVEL, 350,
     "RND, LOOP (the figure repeats); B chords' LEVEL +35"},
    {25050, 350, 16, MELLO_TM_RATE_DOUBLE, 0, 0, 400, 700, 400, MELLO_DEST_CHORD_CUT, -500, MELLO_DEST_SAT, 400,
     "MUTATE 35%, RATE 2X; FILTER 70; B SAT +40"},
    {33050, 150, 5, MELLO_TM_RATE_ONE, 0, 1, 500, 550, 550, MELLO_DEST_CHORD_CUT, -500, MELLO_DEST_OFF, 0,
     "RND, LENGTH 5 against the 16 (MUTATE 15%)"},
    {41050, 250, 16, MELLO_TM_RATE_ONE, 0, 0, 300, 800, 600, MELLO_DEST_CHORD_CUT, -600, MELLO_DEST_OFF, 0,
     "MUTATE 25%; FILTER 80 DECAY 60"},
    {49050, 0, 12, MELLO_TM_RATE_ONE, 1, 1, 600, 500, 600, MELLO_DEST_CHORD_CUT, -700, MELLO_DEST_CHORD_LEVEL, 400,
     "RND, LOOP, LENGTH 12; B chords' LEVEL +40"},
    {57050, 400, 16, MELLO_TM_RATE_HALF, 0, 0, 450, 650, 450, MELLO_DEST_CHORD_CUT, -500, MELLO_DEST_OFF, 0,
     "MUTATE 40%, RATE 1/2X"},
    {65050, 200, 7, MELLO_TM_RATE_ONE, 0, 1, 500, 500, 500, MELLO_DEST_CHORD_CUT, -800, MELLO_DEST_OFF, 0,
     "RND, LENGTH 7, A chords' CUT -80"},
    {73050, 100, 8, MELLO_TM_RATE_ONE, 0, 0, 300, 400, 300, MELLO_DEST_CHORD_CUT, -400, MELLO_DEST_OFF, 0,
     "MUTATE 10%, gentler to the end"},
};
#define NEV (sizeof EV / sizeof EV[0])

/* The lead's knobs turned (SOUND: FILTER, CHARACTER, MOD ENV, GRIT) from where the style left them: TRI out to the
 * peak and back, FROM the peak back to it, TO the peak and left there (the next roll sets it again) */
enum { K_CUT, K_RES, K_SAT, K_MENV, K_DECAY, K_NOISE };
enum { TRI, FROM, TO };
typedef struct {
    uint32_t t0, t1;
    uint8_t knob, shape;
    int16_t peak;
    const char *what;
} Turn;
static const Turn TU[] = {
    {1500, 8800, K_CUT, FROM, -380, "CUTOFF opened from low"},
    {9500, 14500, K_RES, TRI, 380, "RESONANCE up and back"},
    {10000, 14000, K_CUT, TRI, -220, "CUTOFF down and back"},
    {17500, 22500, K_MENV, TRI, 420, "MOD ENV up and back"},
    {25500, 30500, K_SAT, TRI, 620, "SAT up and back"},
    {33500, 38500, K_CUT, TRI, -420, "CUTOFF closed and opened"},
    {41500, 46500, K_RES, TRI, 440, "RESONANCE up and back"},
    {41500, 46500, K_DECAY, TRI, 280, "DECAY longer and back"},
    {49500, 54500, K_CUT, TRI, 260, "CUTOFF brighter and back"},
    {49500, 54500, K_NOISE, TRI, 160, "NOISE in and out"},
    {57500, 62500, K_SAT, TRI, 380, "SAT up and back"},
    {58000, 62000, K_CUT, TRI, -260, "CUTOFF down and back"},
    {65500, 70500, K_RES, TRI, 420, "RESONANCE up and back"},
    {74000, 81500, K_CUT, TO, -520, "CUTOFF closing to the end"},
};
#define NTU (sizeof TU / sizeof TU[0])

static uint16_t *knob(MelloSoundConfig *s, uint8_t k)
{
    switch (k) {
    case K_CUT: return &s->filter_cut;
    case K_RES: return &s->filter_res;
    case K_SAT: return &s->saturation;
    case K_DECAY: return &s->decay_ms;
    case K_NOISE: return &s->noise_level;
    default: return 0;
    }
}

static void say(uint32_t ms, const char *kind, const char *what) { printf("%7.2f  %-6s %s\n", ms / 1000.0, kind, what); }

static void turn(uint32_t ms)
{
    static int32_t base[NTU];
    MelloSoundConfig s = t.engine.sound;
    unsigned k;
    int changed = 0;
    for (k = 0; k < NTU; ++k) {
        const Turn *w = &TU[k];
        int32_t v, lo = 0, hi = 1000, amt;
        uint32_t f;
        if (ms < w->t0 || ms > w->t1) continue;
        if (ms == w->t0) {
            base[k] = w->knob == K_MENV ? s.menv_amount : *knob(&s, w->knob);
            say(ms, "knob", w->what);
        }
        f = (uint32_t)((uint64_t)(ms - w->t0) * 1000u / (w->t1 - w->t0)); /* 0..1000 */
        amt = w->shape == TRI ? (f < 500u ? (int32_t)f * 2 : (int32_t)(1000u - f) * 2)
              : w->shape == FROM ? (int32_t)(1000u - f)
                                 : (int32_t)f;
        v = base[k] + w->peak * amt / 1000;
        if (w->knob == K_MENV) {
            s.menv_amount = (int16_t)(v < -1000 ? -1000 : v > 1000 ? 1000 : v);
        } else {
            if (w->knob == K_DECAY) lo = 5;
            *knob(&s, w->knob) = (uint16_t)(v < lo ? lo : v > hi ? hi : v);
        }
        changed = 1;
    }
    if (changed) mello_engine_sound_set_config(&t.engine, &s);
}

static void evolve(uint32_t ms)
{
    unsigned k;
    for (k = 0; k < NEV; ++k) {
        const Evo *v = &EV[k];
        MelloTmConfig tm;
        MelloModRouting modr;
        if (ms != v->t) continue;
        tm = t.engine.tm;
        tm.on = true;
        tm.locked = v->loop != 0u;
        tm.mutation_prob = v->mutate;
        tm.length = v->length;
        tm.rate = (MelloTmRate)v->rate;
        tm.offset = 0;
        tm.routes = ROUTES;
        tm.depth_gate = v->gate;
        tm.depth_filter = v->filter;
        tm.depth_decay = v->decay;
        mello_engine_tm_set_config(&t.engine, &tm);
        if (v->reroll) mello_engine_tm_reroll(&t.engine);
        modr = t.engine.modr;
        modr.tm_dest[0] = v->dest_a;
        modr.tm_depth[0] = v->depth_a;
        modr.tm_dest[1] = v->dest_b;
        modr.tm_depth[1] = v->depth_b;
        mello_engine_mod_routing_set(&t.engine, &modr);
        say(ms, "evolve", v->what);
    }
}

static uint32_t lead_hash(void) /* the sequence's steps: a roll changes them */
{
    uint32_t h = 2166136261u, i;
    const uint8_t *b = (const uint8_t *)t.engine.project.steps;
    for (i = 0; i < sizeof t.engine.project.steps; ++i) h = (h ^ b[i]) * 16777619u;
    return h;
}

static void put32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }

int main(int argc, char **argv)
{
    MelloProject p;
    uint32_t nf = 0, ms, prev_keys = 0, btn_was = 0, lh, seed = getenv("SEED") ? (uint32_t)atoi(getenv("SEED")) : SEED;
    int32_t out[96];
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    FILE *f;
    char line[96];
    unsigned k;
    if (argc != 2) {
        fprintf(stderr, "usage: take OUT.wav\n");
        return 2;
    }
    mello_project_defaults(&p);
    p.bpm = BPM;
    mello_transport_init(&t, &p, seed, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_transport_set_bpm(&t, BPM);
    mello_ui_init(&u, &t, &st);
    u.random_intensity = MELLO_RANDOMIZE_GROOVE; /* ALGORITHM on HOME: GROOVE (STYLE ANY, as a new project has it) */
    mello_audio_init(&a, delay_buf, 65536, reverb_buf, 24576);
    mello_transport_set_event_sink(&t, sink, &a);
    sync_audio_config();
    lh = lead_hash();
    for (ms = 0; ms < SECS * 1000u; ++ms) {
        uint32_t fr = 44u + (ms % 10u == 0u), i, keys = 0, btn = 0, btn_press = 0; /* 44.1 frames a ms */
        for (k = 0; k < NPR; ++k) {
            const Press *q = &PR[k];
            if (ms < q->t0 || ms >= q->t1) continue;
            if (ms == q->t0) {
                if (!strcmp(q->kind, "hold"))
                    snprintf(line, sizeof line, "%s (%.1f beats)", q->name, (q->t1 - q->t0) * BPM / 60000.0);
                else
                    snprintf(line, sizeof line, "%s", q->name);
                say(ms, q->kind, line);
            }
            if (q->key == PLAY) {
                btn = 1u << MELLO_UI_BTN_PLAY;
                if (ms == q->t0) btn_press = btn;
                continue;
            }
            keys |= 1u << (q->key >= 100 ? BLACK[q->key - 100] : mello_ui_white_key_note_index((uint8_t)q->key));
        }
        mello_ui_set_held_notes(&u, keys);
        mello_ui_input(&u, ms * 1000u, ms, btn_press, btn_was & ~btn, btn, keys & ~prev_keys, enc);
        prev_keys = keys;
        btn_was = btn;
        evolve(ms);
        turn(ms);
        mello_transport_poll_ahead(&t, ms * 1000u, MELLO_TRANSPORT_LOOKAHEAD_US);
        if (!(ms & 7u)) sync_audio_config();
        mello_ui_tick(&u);
        if (lead_hash() != lh) {
            lh = lead_hash();
            say(ms, "style", mello_style_name(t.engine.style_now));
        }
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, fr, ms * 1000u, t.engine.project.bpm);
        for (i = 0; i < fr * 2u; ++i) {
            int32_t x = out[i];
            if (ms > SECS * 1000u - FADE_MS) x = x * (int32_t)(SECS * 1000u - ms) / (int32_t)FADE_MS;
            pcm[2u * nf + i] = (int16_t)(x > 32767 ? 32767 : x < -32767 ? -32767 : x);
        }
        nf += fr;
    }
    f = fopen(argv[1], "wb");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    fwrite("RIFF", 1, 4, f);
    put32(f, nf * 4u + 36u);
    fwrite("WAVEfmt ", 1, 8, f);
    put32(f, 16u), put32(f, 0x00020001u), put32(f, FS), put32(f, FS * 4u), put32(f, 0x00100004u);
    fwrite("data", 1, 4, f);
    put32(f, nf * 4u);
    fwrite(pcm, 4, nf, f);
    fclose(f);
    return 0;
}
