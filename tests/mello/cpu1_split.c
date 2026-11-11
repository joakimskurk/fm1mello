/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The chords and the drums on the second core (ROADMAP H2-H3, DECISIONS "The second core"): the output is the same
 * to the sample whether they render here a sample at a time (one core), on a real thread beside it, on a slow and
 * erratic one, on one that takes the job and loses it (then given up and the chunk rendered here), or on one that
 * refuses it. (Not on one that does the whole chunk before the render goes on, as a double could: the drums' STUTTER
 * reads the memory the render writes beside it, and would wait for it for ever; the FM-1's fm1_cpu1_run hands the
 * job over and returns.) A double of the platform's cpu1_run / cpu1_wait, as SLOOP's tests/cpu1_host.c:
 * the render cannot tell it from the FM-1's. Nine scenes: the default project with chords, the worst case (POLY and
 * its chorus), the worst case as SUB MELLO, C.CUT routed from a MOD lane and EVOLVE with LINK, one with all notes off,
 * OCT- held, the chords switched off and on again while it plays; the drums routed (lanes, EVOLVE and the LFO on
 * TUNE, DECAY, CRUSH, DRIVE), STUTTER pitched up (short loops: the drums wait for the memory the render writes) with
 * DUCK, DRIVE and FOLD, GLITCH's kind changed every quarter second, and the default project's drums. Block sizes
 * 256 (the FM-1's), 100 and 1000 (chunks that do not fill a block, and more than one a block).
 * The reverb (ROADMAP H6): V3 on two cores (mello_audio_set_oliverb(false)) is one core's to the sample, in every
 * way. Oliverb, the reverb with the second core: the same on a thread and on a slow one; refused, V3 from the first
 * sample (one core's); lost, Oliverb's until the lost job, then V3 from the chunk after. A tenth scene, the default
 * project with WET up only after a second: the job lost before it, so the whole of it is one core's. */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

/* ---- the double of the second core */
enum { ONE_CORE, THREAD, CHAOS, LOST, REFUSED, MODES };
static const char *const MODE_NAME[MODES] = {"one core", "a thread", "a slow thread", "a lost job",
                                             "refused"};
static int mode;
static pthread_t worker;
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static void (*volatile job_fn)(uint32_t);
static uint32_t job_arg;
static int job_pending, job_running, quit, held;
static unsigned jobs, lost_at, seed = 7u;
enum { V3, OLIVERB };
static int reverb_kind;
#define BLOCKS_MAX 1400u
#define NONE       0xFFFFFFFFu
static uint32_t *trail, held_block; /* the hash after each block; the block the core was held in */

static void pause_us(unsigned us)
{
    struct timespec ts = {0, (long)us * 1000L};
    nanosleep(&ts, 0);
}
static void *work(void *x)
{
    (void)x;
    pthread_mutex_lock(&mu);
    for (;;) {
        while (!job_pending && !quit) pthread_cond_wait(&cv, &mu);
        if (quit) break;
        job_pending = 0, job_running = 1;
        pthread_mutex_unlock(&mu);
        if (mode == CHAOS) {
            seed = seed * 1103515245u + 12345u;
            pause_us((seed >> 16) % 300u); /* up to 0.3 ms late: the render waits at its first samples */
        }
        job_fn(job_arg);
        pthread_mutex_lock(&mu);
        job_running = 0;
        pthread_cond_broadcast(&cv);
    }
    pthread_mutex_unlock(&mu);
    return 0;
}
static int run(void (*fn)(uint32_t), uint32_t arg)
{
    if (held || mode == REFUSED) return 0;
    ++jobs;
    if (mode == LOST && jobs == lost_at) return 1; /* taken, and never run */
    pthread_mutex_lock(&mu);
    job_fn = fn, job_arg = arg, job_pending = 1;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mu);
    return 1;
}
static int wait_job(void)
{
    if (mode == LOST && jobs == lost_at && !held) {
        held = 1; /* as fm1_cpu1_wait: late, so the core is held from now on */
        return -1;
    }
    pthread_mutex_lock(&mu);
    while (job_pending || job_running) pthread_cond_wait(&cv, &mu);
    pthread_mutex_unlock(&mu);
    return 0;
}
static void sync_cores(void)
{
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}
static const MelloAudioCores CORES = {run, wait_job, sync_cores, 0}; /* (not timed) */

/* ---- the scenes */
static int16_t delay_pool[MELLO_AUDIO_DELAY_SAMPLES];
static int16_t reverb_pool[MELLO_AUDIO_REVERB_SAMPLES];
static MelloTransport t;
static MelloAudio a;
static uint32_t fnv(uint32_t h, const void *p, size_t n)
{
    const unsigned char *b = p;
    while (n--) h = (h ^ *b++) * 16777619u;
    return h;
}
static void chords(bool on, uint16_t chorus, bool poly, bool link)
{
    MelloChordConfig c = t.engine.chord;
    c.on = on, c.poly = poly, c.link = link;
    if (chorus) c.chorus = chorus;
    mello_engine_chord_set_config(&t.engine, &c);
    mello_audio_set_chord_config(&a, &t.engine.chord);
}
static uint32_t scene(int which, uint32_t blk)
{
    static MelloProject p;
    static int32_t out[2 * 1000];
    uint32_t now = 0u, b, h = 2166136261u, blocks = 3u * MELLO_AUDIO_FS / blk;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 0x4D454C4Cu, 0, 0);
    if (which >= 1 && which <= 7) mello_engine_stress(&t.engine);
    if (which == 3 || which == 4) {
        t.engine.modr.dot_target[0] = MELLO_DEST_CHORD_CUT;
        t.engine.modr.tm_dest[1] = MELLO_DEST_CHORD_CUT;
        t.engine.modr.tm_depth[1] = 700;
    }
    if (which == 5) { /* the drums routed: lanes, EVOLVE and the LFO on TUNE, DECAY, CRUSH, DRIVE */
        t.engine.modr.dot_target[0] = MELLO_DEST_DRUM_TUNE;
        t.engine.modr.dot_target[1] = MELLO_DEST_DRUM_DECAY;
        t.engine.modr.tm_dest[0] = MELLO_DEST_DRUM_DRIVE, t.engine.modr.tm_depth[0] = 900;
        t.engine.modr.tm_dest[1] = MELLO_DEST_DRUM_CRUSH, t.engine.modr.tm_depth[1] = -800;
        t.engine.sound.lfo_on = true, t.engine.sound.lfo_target = MELLO_LFO_DRUM_DRIVE,
        t.engine.sound.lfo_depth = 900;
    }
    if (which == 6 || which == 7) { /* STUTTER pitched up (short loops), DUCK, DRIVE and FOLD */
        MelloDrumConfig d = t.engine.drums;
        d.voice[MELLO_DRUM_GLITCH].tone = 900u, d.voice[MELLO_DRUM_GLITCH].tune = 0u;
        d.voice[MELLO_DRUM_GLITCH].decay = 1000u, d.voice[MELLO_DRUM_GLITCH].chance = 1000u;
        d.duck = 800u, d.drive = 700u, d.fold = 500u;
        mello_engine_drums_set_config(&t.engine, &d);
        t.engine.modr.dot_target[0] = MELLO_DEST_DRUM_TUNE;
    }
    if (which == 8) {
        MelloDrumConfig d = t.engine.drums;
        d.on = true;
        mello_engine_drums_set_config(&t.engine, &d);
    }
    if (which == 9) t.engine.fx.reverb_wet = 0u; /* up after a second */
    mello_audio_init(&a, delay_pool, MELLO_AUDIO_DELAY_SAMPLES, reverb_pool, MELLO_AUDIO_REVERB_SAMPLES);
    if (reverb_kind == V3) mello_audio_set_oliverb(&a, false);
    mello_audio_set_config(&a, &t.engine.sound, &t.engine.fx, &t.engine.sub, &t.engine.sample);
    mello_audio_set_mod_routing(&a, &t.engine.modr);
    mello_audio_set_drum_config(&a, &t.engine.drums);
    mello_audio_set_bass_config(&a, &t.engine.bass);
    chords(true, 0u, which == 2 ? false : (which == 0 ? t.engine.chord.poly : true), which >= 3);
    if (mode != ONE_CORE) mello_audio_set_cores(&a, &CORES);
    mello_transport_set_event_sink(&t, mello_audio_event_sink, &a);
    mello_transport_set_bpm(&t, t.engine.project.bpm);
    mello_transport_start(&t, 0u);
    for (b = 0u; b < blocks; ++b) {
        uint32_t at = b * blk;
        if (which == 4 && at / 22050u != (at + blk) / 22050u) { /* every half second, something else */
            static const uint16_t oct[8] = {0, 0, 0, 0, 0, 800, 0, 0};
            switch ((at + blk) / 22050u) {
            case 1u: mello_audio_all_notes_off(&a); break;
            case 2u: mello_audio_set_perf_fx2(&a, MELLO_PERF_FX2_OCTDN, oct); break;
            case 3u: chords(false, 0u, true, true); break;
            case 4u: chords(true, 1000u, true, true); break;
            default: break;
            }
        }
        if (which == 7 && b % (11025u / blk + 1u) == 0u) { /* GLITCH's kind every quarter second */
            static const uint16_t kinds[5] = {100u, 350u, 600u, 900u, 1000u};
            MelloDrumConfig d = t.engine.drums;
            d.voice[MELLO_DRUM_GLITCH].tone = kinds[(b / (11025u / blk + 1u)) % 5u];
            mello_engine_drums_set_config(&t.engine, &d);
            mello_audio_set_drum_config(&a, &t.engine.drums);
        }
        if (which == 9 && at < MELLO_AUDIO_FS && at + blk >= MELLO_AUDIO_FS) {
            t.engine.fx.reverb_wet = 240u;
            mello_audio_set_config(&a, &t.engine.sound, &t.engine.fx, &t.engine.sub, &t.engine.sample);
        }
        mello_transport_poll_ahead(&t, now, MELLO_TRANSPORT_LOOKAHEAD_US);
        memset(out, 0, 2u * blk * sizeof out[0]);
        mello_audio_render(&a, out, blk, now, mello_transport_effective_bpm(&t));
        h = fnv(h, out, 2u * blk * sizeof out[0]);
        if (trail && b < BLOCKS_MAX) trail[b] = h;
        if (held && held_block == NONE) held_block = b;
        now += blk * 1000000u / MELLO_AUDIO_FS;
    }
    return h;
}

/* a scene in a way: the double reset (LOST: the scene's third job on) */
static uint32_t in_mode(int m, int s, uint32_t blk, uint32_t *tr)
{
    mode = m, jobs = 0u, held = 0, lost_at = 3u + (unsigned)s, trail = tr, held_block = NONE;
    return scene(s, blk);
}

int main(void)
{
    static const uint32_t blks[3] = {256u, 100u, 1000u};
    static uint32_t olv_trail[BLOCKS_MAX], lost_trail[BLOCKS_MAX];
    int s, m;
    unsigned k;
    uint32_t sum = 0u;
    pthread_create(&worker, 0, work, 0);
    for (k = 0u; k < 3u; ++k)
        for (s = 0; s < 10; ++s) {
            uint32_t ref, olv, h, b, blk = blks[k];
            reverb_kind = V3;
            ref = in_mode(ONE_CORE, s, blk, 0);
            sum ^= ref;
            for (m = THREAD; m < MODES; ++m) { /* V3 on two cores: one core's, every way */
                h = in_mode(m, s, blk, 0);
                CHECK(h == ref, "scene %d, blocks of %u: V3, %s gave %08x, one core %08x", s, (unsigned)blk,
                      MODE_NAME[m], h, ref);
                if (m == LOST)
                    CHECK(held, "scene %d, blocks of %u: the lost job left the core on", s, (unsigned)blk);
            }
            reverb_kind = OLIVERB;
            olv = in_mode(THREAD, s, blk, olv_trail);
            CHECK(olv != ref && a.oliverb_on, "scene %d, blocks of %u: Oliverb did not run", s,
                  (unsigned)blk);
            h = in_mode(CHAOS, s, blk, 0);
            CHECK(h == olv, "scene %d, blocks of %u: Oliverb, a slow thread gave %08x, a thread %08x", s,
                  (unsigned)blk, h, olv);
            h = in_mode(REFUSED, s, blk, 0);
            CHECK(h == ref && !a.oliverb_on,
                  "scene %d, blocks of %u: Oliverb refused gave %08x, one core %08x", s, (unsigned)blk, h,
                  ref);
            h = in_mode(LOST, s, blk, lost_trail);
            CHECK(held && !a.oliverb_on && !a.cores, "scene %d, blocks of %u: Oliverb lost, still on", s,
                  (unsigned)blk);
            for (b = 0u; b < held_block && b < BLOCKS_MAX; ++b)
                if (lost_trail[b] != olv_trail[b]) {
                    CHECK(0, "scene %d, blocks of %u: Oliverb lost, block %u before it not a thread's", s,
                          (unsigned)blk, (unsigned)b);
                    break;
                }
            if (s == 9)
                CHECK(h == ref, "scene 9, blocks of %u: lost before WET, %08x, not one core's %08x",
                      (unsigned)blk, h, ref);
        }
    pthread_mutex_lock(&mu);
    quit = 1;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mu);
    pthread_join(worker, 0);
    if (fails) return 1;
    printf("PASS: cpu1_split (10 scenes x 3 block sizes: V3 %d ways, Oliverb 4, %08x)\n", MODES, sum);
    return 0;
}
