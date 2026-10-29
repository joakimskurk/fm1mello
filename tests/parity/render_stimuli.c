/* Renders tools/tone-ref/stimuli.json (via stimuli.h) through the C audio engine, as 32-bit float WAVs that
 * tools/tone-ref/compare.py holds against the HTML's (tests/golden/html). Not a pass/fail test by itself.
 *   render_stimuli OUTDIR [name ...] */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mello_audio.h"
#include "stimuli.h"

#define FRAMES 256u
static int16_t delay_buf[4096 * 8];
static int16_t reverb_buf[24576];

static uint16_t permille(double v)
{
    if (v < 0.0) v = 0.0;
    return (uint16_t)(v * 1000.0 + 0.5);
}
static uint16_t ms(double sec)
{
    return (uint16_t)(sec * 1000.0 + 0.5);
}
static MelloWaveform wave(const char *t)
{
    if (!strcmp(t, "square")) return MELLO_WAVE_SQUARE;
    if (!strcmp(t, "triangle")) return MELLO_WAVE_TRIANGLE;
    if (!strcmp(t, "sine")) return MELLO_WAVE_SINE;
    if (!strcmp(t, "pulse")) return MELLO_WAVE_PULSE;
    if (!strcmp(t, "off")) return MELLO_WAVE_OFF;
    return MELLO_WAVE_SAW;
}

/* The HTML's parameter names (useState) onto the C configs. Unknown names stop the run. */
static void apply_param(const StimParam *p, MelloSoundConfig *s, MelloFxConfig *fx, MelloSubConfig *sub)
{
    const char *k = p->key;
    if (!strcmp(k, "filterCut")) s->filter_cut = permille(p->num);
    else if (!strcmp(k, "filterRes")) s->filter_res = permille(p->num);
    else if (!strcmp(k, "attack")) s->attack_ms = ms(p->num);
    else if (!strcmp(k, "decay")) s->decay_ms = ms(p->num);
    else if (!strcmp(k, "sustain")) s->sustain_permille = permille(p->num);
    else if (!strcmp(k, "release")) s->release_ms = ms(p->num);
    else if (!strcmp(k, "oscType")) s->osc_wave = wave(p->text);
    else if (!strcmp(k, "subOscOn")) s->follower_sub_on = p->num != 0.0;
    else if (!strcmp(k, "subOscLevel")) s->follower_sub_level = permille(p->num);
    else if (!strcmp(k, "distAmt")) fx->dist_amount = permille(p->num);
    else if (!strcmp(k, "distCharacter")) fx->dist_character = permille(p->num);
    else if (!strcmp(k, "subLevel")) sub->level = permille(p->num);
    else {
        fprintf(stderr, "render_stimuli: no C mapping for HTML parameter '%s'\n", k);
        exit(2);
    }
}

typedef struct {
    double t;
    MelloEvent e;
} Timed;

static int by_time(const void *a, const void *b)
{
    double d = ((const Timed *)a)->t - ((const Timed *)b)->t;
    return d < 0 ? -1 : d > 0;
}

static void put32(FILE *f, uint32_t v)
{
    uint8_t b[4] = {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24)};
    fwrite(b, 1, 4, f);
}
static void put16(FILE *f, uint16_t v)
{
    uint8_t b[2] = {(uint8_t)v, (uint8_t)(v >> 8)};
    fwrite(b, 1, 2, f);
}

static int render(const Stimulus *st, const char *dir)
{
    static MelloAudio a;
    static int32_t out[FRAMES * 2u];
    static Timed ev[256];
    MelloSoundConfig s;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sm;
    uint32_t total = (uint32_t)(st->seconds * STIM_SAMPLE_RATE + 0.5), done = 0u;
    int n = 0, next = 0, i;
    char path[1024];
    FILE *f;

    mello_sound_defaults(&s);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sm);
    for (i = 0; i < st->param_count; ++i) apply_param(&st->params[i], &s, &fx, &sub);
    if (st->fx == STIM_FX_DRY) fx.delay_wet = fx.reverb_wet = 0u;
    if (st->fx == STIM_FX_REVERB) fx.delay_wet = 0u;
    for (i = 0; i < st->lead_count; ++i) {
        const StimNote *x = &st->lead[i];
        uint8_t vel = (uint8_t)(x->vel * 127.0 + 0.5);
        memset(&ev[n], 0, sizeof ev[n]);
        ev[n].t = x->t;
        ev[n].e.type = MELLO_EVENT_NOTE_ON;
        ev[n].e.note = (uint8_t)x->midi;
        ev[n].e.velocity = vel;
        n++;
        memset(&ev[n], 0, sizeof ev[n]);
        ev[n].t = x->t + x->dur;
        ev[n].e.type = MELLO_EVENT_NOTE_OFF;
        ev[n].e.note = (uint8_t)x->midi;
        n++;
    }
    for (i = 0; i < st->sub_count; ++i) {
        const StimNote *x = &st->sub[i];
        memset(&ev[n], 0, sizeof ev[n]);
        ev[n].t = x->t;
        ev[n].e.type = MELLO_EVENT_SUB_NOTE_ON;
        ev[n].e.note = (uint8_t)x->midi;
        ev[n].e.velocity = 114u; /* triggerSub's fixed 0.9 */
        n++;
        memset(&ev[n], 0, sizeof ev[n]);
        ev[n].t = x->t + x->dur;
        ev[n].e.type = MELLO_EVENT_SUB_NOTE_OFF;
        ev[n].e.note = (uint8_t)x->midi;
        n++;
    }
    qsort(ev, (size_t)n, sizeof ev[0], by_time);

    mello_audio_init(&a, delay_buf, sizeof delay_buf / sizeof delay_buf[0], reverb_buf,
                     sizeof reverb_buf / sizeof reverb_buf[0]);
    mello_audio_set_config(&a, &s, &fx, &sub, &sm);
    snprintf(path, sizeof path, "%s/%s.wav", dir, st->name);
    f = fopen(path, "wb");
    if (!f) {
        perror(path);
        return 1;
    }
    fwrite("RIFF", 1, 4, f);
    put32(f, 36u + total * 8u);
    fwrite("WAVEfmt ", 1, 8, f);
    put32(f, 16u);
    put16(f, 3u); /* IEEE float */
    put16(f, 2u);
    put32(f, STIM_SAMPLE_RATE);
    put32(f, STIM_SAMPLE_RATE * 8u);
    put16(f, 8u);
    put16(f, 32u);
    fwrite("data", 1, 4, f);
    put32(f, total * 8u);
    while (done < total) {
        uint32_t frames = total - done < FRAMES ? total - done : FRAMES, us = (uint32_t)((double)done * 1e6 / STIM_SAMPLE_RATE);
        double block_end = (double)(done + FRAMES) / STIM_SAMPLE_RATE;
        /* Hand each event to the engine one block ahead, with its exact time, as a lookahead transport would. */
        while (next < n && ev[next].t < block_end + (double)FRAMES / STIM_SAMPLE_RATE) {
            mello_audio_push_engine_event(&a, &ev[next].e, (uint32_t)(ev[next].t * 1e6 + 0.5));
            next++;
        }
        memset(out, 0, sizeof out);
        mello_audio_render(&a, out, frames, us, STIM_BPM);
        for (i = 0; i < (int)frames * 2; ++i) {
            float v = (float)out[i] / 32768.0f;
            fwrite(&v, 4, 1, f);
        }
        done += frames;
    }
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    int i, j, rc = 0;
    if (argc < 2) {
        fprintf(stderr, "usage: render_stimuli OUTDIR [name ...]\n");
        return 2;
    }
    for (i = 0; i < STIM_COUNT; ++i) {
        int want = argc == 2;
        for (j = 2; j < argc; ++j)
            if (!strcmp(argv[j], STIMULI[i].name)) want = 1;
        if (want) {
            rc |= render(&STIMULI[i], argv[1]);
            printf("%s\n", STIMULI[i].name);
        }
    }
    return rc;
}
