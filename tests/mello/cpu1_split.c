/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The chords on the second core (ROADMAP H2, DECISIONS "The second core"): the output is the same to the sample
 * whether the chords render here before the rest (one core), on a core that has done them before the rest starts,
 * on a real thread beside it, on a slow and erratic one, or on one that takes the job and loses it (then given up
 * and the chunk rendered here). A double of the platform's cpu1_run / cpu1_wait, as SLOOP's tests/cpu1_host.c:
 * the render cannot tell it from the FM-1's. Five scenes: the default project with chords, the worst case (POLY and
 * its chorus), the worst case as SUB MELLO, C.CUT routed from a MOD lane and EVOLVE with LINK, and one with all
 * notes off, OCT- held, the chords switched off and on again while it plays. Block sizes 256 (the FM-1's), 100
 * and 1000 (chunks that do not fill a block, and more than one a block). */
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
enum { ONE_CORE, AT_ONCE, THREAD, CHAOS, LOST, REFUSED, MODES };
static const char *const MODE_NAME[MODES] = {"one core",      "done at once", "a thread",
                                             "a slow thread", "a lost job",   "refused"};
static int mode;
static pthread_t worker;
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static void (*volatile job_fn)(uint32_t);
static uint32_t job_arg;
static int job_pending, job_running, quit, held;
static unsigned jobs, lost_at, seed = 7u;

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
    if (mode == AT_ONCE) {
        fn(arg);
        return 1;
    }
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
    if (mode == AT_ONCE) return 0;
    pthread_mutex_lock(&mu);
    while (job_pending || job_running) pthread_cond_wait(&cv, &mu);
    pthread_mutex_unlock(&mu);
    return 0;
}
static void sync_cores(void)
{
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}
static const MelloAudioCores CORES = {run, wait_job, sync_cores};

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
    if (which >= 1) mello_engine_stress(&t.engine);
    if (which == 3 || which == 4) {
        t.engine.modr.dot_target[0] = MELLO_DEST_CHORD_CUT;
        t.engine.modr.tm_dest[1] = MELLO_DEST_CHORD_CUT;
        t.engine.modr.tm_depth[1] = 700;
    }
    mello_audio_init(&a, delay_pool, MELLO_AUDIO_DELAY_SAMPLES, reverb_pool, MELLO_AUDIO_REVERB_SAMPLES);
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
        mello_transport_poll_ahead(&t, now, MELLO_TRANSPORT_LOOKAHEAD_US);
        memset(out, 0, 2u * blk * sizeof out[0]);
        mello_audio_render(&a, out, blk, now, mello_transport_effective_bpm(&t));
        h = fnv(h, out, 2u * blk * sizeof out[0]);
        now += blk * 1000000u / MELLO_AUDIO_FS;
    }
    return h;
}

int main(void)
{
    static const uint32_t blks[3] = {256u, 100u, 1000u};
    int s, m;
    unsigned k;
    uint32_t sum = 0u;
    pthread_create(&worker, 0, work, 0);
    for (k = 0u; k < 3u; ++k)
        for (s = 0; s < 5; ++s) {
            uint32_t ref;
            mode = ONE_CORE;
            ref = scene(s, blks[k]);
            sum ^= ref;
            for (m = AT_ONCE; m < MODES; ++m) {
                uint32_t h;
                mode = m, jobs = 0u, held = 0,
                lost_at = 3u + (unsigned)s; /* LOST: the scene's third job on */
                h = scene(s, blks[k]);
                CHECK(h == ref, "scene %d, blocks of %u: %s gave %08x, one core %08x", s, (unsigned)blks[k],
                      MODE_NAME[m], h, ref);
                if (m == LOST)
                    CHECK(held, "scene %d, blocks of %u: the lost job left the core on", s,
                          (unsigned)blks[k]);
            }
        }
    pthread_mutex_lock(&mu);
    quit = 1;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mu);
    pthread_join(worker, 0);
    if (fails) return 1;
    printf("PASS: cpu1_split (5 scenes x 3 block sizes x %d ways, %08x)\n", MODES, sum);
    return 0;
}
