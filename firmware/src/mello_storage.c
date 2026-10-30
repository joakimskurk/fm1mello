#include "mello_storage.h"

enum {
    SEC_META = 1,
    SEC_WORKING = 2,
    SEC_SLOTS = 3,
    SEC_SLOT_SEQ = 4,
    SEC_FX = 5,
    SEC_CHORD = 6,
    SEC_GRIT = 7,
    SEC_MOD = 8
};

enum {
    SECVER_META = 1,
    SECVER_WORKING = 1,
    SECVER_SLOTS = 1,
    SECVER_SLOT_SEQ = 1,
    SECVER_FX = 1,
    SECVER_CHORD = 1,
    SECVER_GRIT = 2,
    SECVER_MOD = 1
};

#define SAMPLE_WIRE_SIZE   83u
#define SNAPSHOT_WIRE_SIZE 528u
#define FX_WIRE_SIZE       34u
#define CHORD_WIRE_SIZE    (14u + MELLO_CHORD_MAX_STEPS * 6u + 8u + 1u + MELLO_CHORD_PATTERN_STEPS)
#define GRIT_ENTRY_SIZE_V1 8u
#define GRIT_ENTRY_SIZE    26u
#define MOD_ENTRY_SIZE     40u
#define SLOT_SEQ_WIRE_SIZE (2u + MELLO_MAX_SLOT_SEQ_STEPS * 3u)
#define SLOT_WIRE_SIZE     (1u + (MELLO_SLOT_NAME_CHARS + 1u) + SNAPSHOT_WIRE_SIZE)

#define CRC_INIT 0xFFFFFFFFu
#define CRC_XOR  0xFFFFFFFFu

typedef struct {
    MelloStorage *storage;
    uint32_t address;
    uint32_t count;
    uint32_t crc;
    bool count_only;
    bool ok;
    uint8_t buf[256];
    uint16_t used;
} Writer;

typedef struct {
    MelloStorage *storage;
    uint32_t address;
    uint32_t pos;
    uint32_t length;
    bool ok;
} Reader;

typedef struct {
    bool valid;
    bool present;
    bool unsupported;
    bool io_error;
    uint8_t bank;
    uint16_t version;
    uint32_t revision;
    uint32_t payload_size;
    uint32_t payload_crc;
    uint32_t payload_addr;
} RecordInfo;

static uint32_t crc_step(uint32_t c, uint8_t b)
{
    uint8_t i;
    c ^= (uint32_t)b;
    for (i = 0u; i < 8u; ++i) c = (c & 1u) ? ((c >> 1) ^ 0xEDB88320u) : (c >> 1);
    return c;
}

uint32_t mello_storage_crc32(const void *data, uint32_t length)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t c = CRC_INIT, i;
    if (!p && length) return 0u;
    for (i = 0u; i < length; ++i) c = crc_step(c, p[i]);
    return c ^ CRC_XOR;
}

static void wr_flush(Writer *w)
{
    if (!w || !w->ok || w->count_only || w->used == 0u) return;
    if (!w->storage || !w->storage->io.program ||
        !w->storage->io.program(w->storage->io.ctx, w->address, w->buf, w->used)) {
        w->ok = false;
        return;
    }
    w->address += (uint32_t)w->used;
    w->used = 0u;
}

static void wr_bytes(Writer *w, const void *src, uint32_t n)
{
    const uint8_t *p = (const uint8_t *)src;
    uint32_t i;
    if (!w || !w->ok || (!p && n)) return;
    for (i = 0u; i < n; ++i) {
        const uint8_t b = p[i];
        w->crc = crc_step(w->crc, b);
        w->count++;
        if (!w->count_only) {
            w->buf[w->used++] = b;
            if (w->used == (uint16_t)sizeof(w->buf)) wr_flush(w);
            if (!w->ok) return;
        }
    }
}
static void wr_u8(Writer *w, uint8_t v)
{
    wr_bytes(w, &v, 1u);
}
static void wr_i8(Writer *w, int8_t v)
{
    uint8_t b = (uint8_t)v;
    wr_u8(w, b);
}
static void wr_u16(Writer *w, uint16_t v)
{
    uint8_t b[2] = {(uint8_t)v, (uint8_t)(v >> 8)};
    wr_bytes(w, b, 2u);
}
static void wr_u32(Writer *w, uint32_t v)
{
    uint8_t b[4] = {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24)};
    wr_bytes(w, b, 4u);
}

static bool rd_bytes(Reader *r, void *dst, uint32_t n)
{
    if (!r || !r->ok || !dst || n > r->length - r->pos) return false;
    if (!r->storage || !r->storage->io.read ||
        !r->storage->io.read(r->storage->io.ctx, r->address + r->pos, dst, n)) {
        r->ok = false;
        return false;
    }
    r->pos += n;
    return true;
}
static bool rd_skip(Reader *r, uint32_t n)
{
    if (!r || n > r->length - r->pos) return false;
    r->pos += n;
    return true;
}
static bool rd_u8(Reader *r, uint8_t *v)
{
    return rd_bytes(r, v, 1u);
}
static bool rd_i8(Reader *r, int8_t *v)
{
    uint8_t b;
    if (!rd_u8(r, &b)) return false;
    *v = (int8_t)b;
    return true;
}
static bool rd_u16(Reader *r, uint16_t *v)
{
    uint8_t b[2];
    if (!rd_bytes(r, b, 2u)) return false;
    *v = (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
    return true;
}
static bool rd_u32(Reader *r, uint32_t *v)
{
    uint8_t b[4];
    if (!rd_bytes(r, b, 4u)) return false;
    *v = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    return true;
}

static void encode_stored_project(Writer *w, const MelloStoredProject *p)
{
    uint8_t i;
    wr_u8(w, p->seq_len);
    wr_u8(w, p->bpm_lo);
    wr_u8(w, p->bpm_hi);
    wr_u8(w, p->clock_div);
    wr_u8(w, p->root_note);
    wr_u8(w, p->scale);
    wr_i8(w, p->global_octave);
    wr_u8(w, p->pitch_span_oct);
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        const MelloStoredStep *s = &p->steps[i];
        wr_i8(w, s->pitch);
        wr_u8(w, s->pulse_ratchet);
        wr_u8(w, s->flags);
        wr_u8(w, s->extras);
        wr_u8(w, s->probability_lo);
        wr_u8(w, s->probability_hi);
    }
    wr_u8(w, p->seg_count);
    for (i = 0u; i < MELLO_MAX_SEGMENTS - 1u; ++i) wr_u8(w, p->seg_splits[i]);
    for (i = 0u; i < MELLO_MAX_SEGMENTS; ++i) wr_u8(w, p->seg_repeats[i]);
    for (i = 0u; i < MELLO_MAX_SEGMENTS; ++i) wr_u8(w, p->seg_direction[i]);
    for (i = 0u; i < MELLO_MAX_SEGMENTS; ++i) wr_u8(w, p->seg_mode[i]);
    wr_u8(w, p->seg_skip_mask);
    wr_u8(w, p->master_len_lo);
    wr_u8(w, p->master_len_hi);
}

static bool decode_stored_project(Reader *r, MelloStoredProject *p)
{
    uint8_t i;
    if (!rd_u8(r, &p->seq_len) || !rd_u8(r, &p->bpm_lo) || !rd_u8(r, &p->bpm_hi) ||
        !rd_u8(r, &p->clock_div) || !rd_u8(r, &p->root_note) || !rd_u8(r, &p->scale) ||
        !rd_i8(r, &p->global_octave) || !rd_u8(r, &p->pitch_span_oct))
        return false;
    for (i = 0u; i < MELLO_MAX_STEPS; ++i) {
        MelloStoredStep *s = &p->steps[i];
        if (!rd_i8(r, &s->pitch) || !rd_u8(r, &s->pulse_ratchet) || !rd_u8(r, &s->flags) ||
            !rd_u8(r, &s->extras) || !rd_u8(r, &s->probability_lo) || !rd_u8(r, &s->probability_hi))
            return false;
    }
    if (!rd_u8(r, &p->seg_count)) return false;
    for (i = 0u; i < MELLO_MAX_SEGMENTS - 1u; ++i)
        if (!rd_u8(r, &p->seg_splits[i])) return false;
    for (i = 0u; i < MELLO_MAX_SEGMENTS; ++i)
        if (!rd_u8(r, &p->seg_repeats[i])) return false;
    for (i = 0u; i < MELLO_MAX_SEGMENTS; ++i)
        if (!rd_u8(r, &p->seg_direction[i])) return false;
    for (i = 0u; i < MELLO_MAX_SEGMENTS; ++i)
        if (!rd_u8(r, &p->seg_mode[i])) return false;
    return rd_u8(r, &p->seg_skip_mask) && rd_u8(r, &p->master_len_lo) && rd_u8(r, &p->master_len_hi);
}

static void encode_loop(Writer *w, const MelloLoopCycleConfig *c)
{
    uint8_t i;
    wr_u8(w, c->active ? 1u : 0u);
    wr_u8(w, c->sync ? 1u : 0u);
    wr_u8(w, c->length);
    for (i = 0u; i < MELLO_MAX_LOOP_CYCLE_STEPS; ++i) {
        const MelloLoopCycleStep *s = &c->steps[i];
        wr_u16(w, s->hold);
        wr_i8(w, s->root_offset);
        wr_i8(w, s->root_note);
        wr_i8(w, s->scale_set);
        wr_i8(w, s->octave);
        wr_i8(w, s->pitch);
        wr_u8(w, s->sub_tone);
    }
}
static bool decode_loop(Reader *r, MelloLoopCycleConfig *c)
{
    uint8_t i, b;
    if (!rd_u8(r, &b)) return false;
    c->active = b != 0u;
    if (!rd_u8(r, &b)) return false;
    c->sync = b != 0u;
    if (!rd_u8(r, &c->length)) return false;
    for (i = 0u; i < MELLO_MAX_LOOP_CYCLE_STEPS; ++i) {
        MelloLoopCycleStep *s = &c->steps[i];
        if (!rd_u16(r, &s->hold) || !rd_i8(r, &s->root_offset) || !rd_i8(r, &s->root_note) ||
            !rd_i8(r, &s->scale_set) || !rd_i8(r, &s->octave) || !rd_i8(r, &s->pitch) ||
            !rd_u8(r, &s->sub_tone))
            return false;
    }
    return true;
}

static void encode_dot(Writer *w, const MelloDotMatrixConfig *d)
{
    wr_u16(w, d->cells);
    wr_u8(w, (uint8_t)d->mode);
    wr_u8(w, d->length);
    wr_u8(w, d->reset ? 1u : 0u);
    wr_u8(w, d->follow ? 1u : 0u);
    wr_u16(w, d->amount);
}
static bool decode_dot(Reader *r, MelloDotMatrixConfig *d)
{
    uint8_t b;
    if (!rd_u16(r, &d->cells) || !rd_u8(r, &b)) return false;
    d->mode = (MelloDotMode)b;
    if (!rd_u8(r, &d->length) || !rd_u8(r, &b)) return false;
    d->reset = b != 0u;
    if (!rd_u8(r, &b)) return false;
    d->follow = b != 0u;
    return rd_u16(r, &d->amount);
}

static void encode_tm(Writer *w, const MelloTmConfig *t)
{
    wr_u8(w, t->on ? 1u : 0u);
    wr_u8(w, t->locked ? 1u : 0u);
    wr_u16(w, t->mutation_prob);
    wr_u8(w, t->length);
    wr_u8(w, (uint8_t)t->rate);
    wr_i8(w, t->offset);
    wr_u8(w, t->routes);
    wr_u16(w, t->depth_gate);
    wr_u16(w, t->depth_filter);
    wr_u16(w, t->depth_octave);
    wr_u16(w, t->depth_decay);
}
static bool decode_tm(Reader *r, MelloTmConfig *t)
{
    uint8_t b;
    if (!rd_u8(r, &b)) return false;
    t->on = b != 0u;
    if (!rd_u8(r, &b)) return false;
    t->locked = b != 0u;
    if (!rd_u16(r, &t->mutation_prob) || !rd_u8(r, &t->length) || !rd_u8(r, &b)) return false;
    t->rate = (MelloTmRate)b;
    return rd_i8(r, &t->offset) && rd_u8(r, &t->routes) && rd_u16(r, &t->depth_gate) &&
           rd_u16(r, &t->depth_filter) && rd_u16(r, &t->depth_octave) && rd_u16(r, &t->depth_decay);
}

static void encode_sub(Writer *w, const MelloSubConfig *s)
{
    uint8_t i;
    wr_u8(w, s->on ? 1u : 0u);
    wr_u8(w, (uint8_t)s->mode);
    wr_u8(w, s->euclid_pulses);
    wr_u8(w, s->euclid_rotate);
    wr_i8(w, s->octave);
    wr_u16(w, s->gate_amount);
    wr_u16(w, s->level);
    wr_u8(w, (uint8_t)s->wave);
    wr_u16(w, s->decay_ms);
    wr_u16(w, s->release_ms);
    wr_u16(w, s->glide_ms);
    wr_u16(w, s->cut);
    wr_u16(w, s->drive);
    wr_u8(w, s->x_on ? 1u : 0u);
    wr_u8(w, s->x_reset ? 1u : 0u);
    wr_u8(w, s->x_length);
    for (i = 0u; i < MELLO_SUB_X_MAX_STEPS; ++i) {
        wr_u8(w, s->x_steps[i].on ? 1u : 0u);
        wr_u8(w, s->x_steps[i].tone);
        wr_i8(w, s->x_steps[i].octave);
    }
}
static bool decode_sub(Reader *r, MelloSubConfig *s)
{
    uint8_t i, b;
    if (!rd_u8(r, &b)) return false;
    s->on = b != 0u;
    if (!rd_u8(r, &b)) return false;
    s->mode = (MelloSubMode)b;
    if (!rd_u8(r, &s->euclid_pulses) || !rd_u8(r, &s->euclid_rotate) || !rd_i8(r, &s->octave) ||
        !rd_u16(r, &s->gate_amount) || !rd_u16(r, &s->level) || !rd_u8(r, &b))
        return false;
    s->wave = (MelloWaveform)b;
    if (!rd_u16(r, &s->decay_ms) || !rd_u16(r, &s->release_ms) || !rd_u16(r, &s->glide_ms) ||
        !rd_u16(r, &s->cut) || !rd_u16(r, &s->drive) || !rd_u8(r, &b))
        return false;
    s->x_on = b != 0u;
    if (!rd_u8(r, &b)) return false;
    s->x_reset = b != 0u;
    if (!rd_u8(r, &s->x_length)) return false;
    for (i = 0u; i < MELLO_SUB_X_MAX_STEPS; ++i) {
        if (!rd_u8(r, &b)) return false;
        s->x_steps[i].on = b != 0u;
        if (!rd_u8(r, &s->x_steps[i].tone) || !rd_i8(r, &s->x_steps[i].octave)) return false;
    }
    return true;
}

static void encode_sound(Writer *w, const MelloSoundConfig *c)
{
    wr_u8(w, (uint8_t)c->osc_wave);
    wr_u16(w, c->pwm_permille);
    wr_u16(w, c->attack_ms);
    wr_u16(w, c->decay_ms);
    wr_u16(w, c->sustain_permille);
    wr_u16(w, c->release_ms);
    wr_u16(w, c->slide_ms);
    wr_u16(w, c->filter_cut);
    wr_u16(w, c->filter_res);
    wr_u8(w, c->lfo_on ? 1u : 0u);
    wr_u16(w, c->lfo_rate_millihz);
    wr_u16(w, c->lfo_depth);
    wr_u8(w, (uint8_t)c->lfo_target);
    wr_u8(w, c->follower_sub_on ? 1u : 0u);
    wr_u16(w, c->follower_sub_level);
    wr_i8(w, c->follower_sub_octave);
    wr_u8(w, (uint8_t)c->follower_sub_wave);
    wr_u16(w, c->master_level);
}
static bool decode_sound(Reader *r, MelloSoundConfig *c)
{
    uint8_t b;
    mello_sound_defaults(
        c); /* Phase 5's fields are not in this wire format: SEC_GRIT, if any, sets them later */
    if (!rd_u8(r, &b)) return false;
    c->osc_wave = (MelloWaveform)b;
    if (!rd_u16(r, &c->pwm_permille) || !rd_u16(r, &c->attack_ms) || !rd_u16(r, &c->decay_ms) ||
        !rd_u16(r, &c->sustain_permille) || !rd_u16(r, &c->release_ms) || !rd_u16(r, &c->slide_ms) ||
        !rd_u16(r, &c->filter_cut) || !rd_u16(r, &c->filter_res) || !rd_u8(r, &b))
        return false;
    c->lfo_on = b != 0u;
    if (!rd_u16(r, &c->lfo_rate_millihz) || !rd_u16(r, &c->lfo_depth) || !rd_u8(r, &b)) return false;
    c->lfo_target = (MelloLfoTarget)b;
    if (!rd_u8(r, &b)) return false;
    c->follower_sub_on = b != 0u;
    if (!rd_u16(r, &c->follower_sub_level) || !rd_i8(r, &c->follower_sub_octave) || !rd_u8(r, &b) ||
        !rd_u16(r, &c->master_level))
        return false;
    c->follower_sub_wave = (MelloWaveform)b;
    return true;
}

static void encode_sample(Writer *w, const MelloSampleConfig *c)
{
    uint8_t i;
    wr_u8(w, c->on ? 1u : 0u);
    wr_u8(w, c->per_step ? 1u : 0u);
    wr_u8(w, c->edit_slot);
    for (i = 0u; i < MELLO_SAMPLE_SLOT_COUNT; ++i) {
        const MelloSampleSlotConfig *x = &c->slots[i];
        wr_i8(w, x->coarse_semitones);
        wr_i8(w, x->fine_cents);
        wr_u8(w, x->track ? 1u : 0u);
        wr_u8(w, x->loop ? 1u : 0u);
        wr_u16(w, x->level);
        wr_u16(w, x->dist);
        wr_u16(w, x->trim_start);
        wr_u16(w, x->trim_end);
        wr_u16(w, x->attack_ms);
        wr_u16(w, x->decay_ms);
        wr_u16(w, x->sustain_permille);
        wr_u16(w, x->release_ms);
    }
}
static bool decode_sample(Reader *r, MelloSampleConfig *c)
{
    uint8_t i, b;
    if (!rd_u8(r, &b)) return false;
    c->on = b != 0u;
    if (!rd_u8(r, &b)) return false;
    c->per_step = b != 0u;
    if (!rd_u8(r, &c->edit_slot)) return false;
    for (i = 0u; i < MELLO_SAMPLE_SLOT_COUNT; ++i) {
        MelloSampleSlotConfig *x = &c->slots[i];
        if (!rd_i8(r, &x->coarse_semitones) || !rd_i8(r, &x->fine_cents) || !rd_u8(r, &b)) return false;
        x->track = b != 0u;
        if (!rd_u8(r, &b)) return false;
        x->loop = b != 0u;
        if (!rd_u16(r, &x->level) || !rd_u16(r, &x->dist) || !rd_u16(r, &x->trim_start) ||
            !rd_u16(r, &x->trim_end) || !rd_u16(r, &x->attack_ms) || !rd_u16(r, &x->decay_ms) ||
            !rd_u16(r, &x->sustain_permille) || !rd_u16(r, &x->release_ms))
            return false;
    }
    return true;
}

static void encode_fx(Writer *w, const MelloFxConfig *c)
{
    wr_u16(w, c->dist_amount);
    wr_u16(w, c->dist_mix);
    wr_u16(w, c->dist_character);
    wr_u16(w, c->dist_tone);
    wr_u16(w, c->dist_level);
    wr_u16(w, c->delay_wet);
    wr_u8(w, c->delay_sync ? 1u : 0u);
    wr_u8(w, c->delay_div);
    wr_u16(w, c->delay_time_ms);
    wr_u16(w, c->delay_feedback);
    wr_u16(w, c->delay_tone);
    wr_u16(w, c->delay_level);
    wr_u16(w, c->reverb_wet);
    wr_u16(w, c->reverb_size);
    wr_u16(w, c->reverb_decay_ms);
    wr_u16(w, c->reverb_pre_ms);
    wr_u16(w, c->reverb_tone);
    wr_u16(w, c->reverb_level);
}
static bool decode_fx(Reader *r, MelloFxConfig *c)
{
    uint8_t b;
    if (!rd_u16(r, &c->dist_amount) || !rd_u16(r, &c->dist_mix) || !rd_u16(r, &c->dist_character) ||
        !rd_u16(r, &c->dist_tone) || !rd_u16(r, &c->dist_level) || !rd_u16(r, &c->delay_wet) || !rd_u8(r, &b))
        return false;
    c->delay_sync = b != 0u;
    return rd_u8(r, &c->delay_div) && rd_u16(r, &c->delay_time_ms) && rd_u16(r, &c->delay_feedback) &&
           rd_u16(r, &c->delay_tone) && rd_u16(r, &c->delay_level) && rd_u16(r, &c->reverb_wet) &&
           rd_u16(r, &c->reverb_size) && rd_u16(r, &c->reverb_decay_ms) && rd_u16(r, &c->reverb_pre_ms) &&
           rd_u16(r, &c->reverb_tone) && rd_u16(r, &c->reverb_level);
}

#if MELLO_FEATURE_CHORD_MACHINE
/* The working Chord Machine config. A reader without this section (an older or chord-off build) skips it. */
static void encode_chord(Writer *w, const MelloChordConfig *c)
{
    uint8_t i;
    wr_u8(w, c->on ? 1u : 0u);
    wr_u8(w, (uint8_t)c->trigger);
    wr_u8(w, (uint8_t)c->mode);
    wr_u8(w, c->length);
    wr_u8(w, c->voices);
    wr_u16(w, c->level);
    wr_u8(w, (uint8_t)c->wave);
    wr_u16(w, c->gate_amount);
    wr_u16(w, c->attack_ms);
    wr_u16(w, c->release_ms);
    for (i = 0u; i < MELLO_CHORD_MAX_STEPS; ++i) {
        wr_u8(w, c->steps[i].degree);
        wr_u8(w, (uint8_t)c->steps[i].shape);
        wr_u8(w, c->steps[i].inversion);
        wr_i8(w, c->steps[i].octave);
        wr_u8(w, c->steps[i].spread);
        wr_u8(w, c->steps[i].hold);
    }
    wr_u16(w, c->grid_cells);
    wr_u8(w, (uint8_t)c->grid_mode);
    wr_u8(w, c->grid_length);
    wr_u8(w, c->grid_reset ? 1u : 0u);
    wr_u8(w, c->grid_follow ? 1u : 0u);
    wr_u16(w, c->grid_amount);
    wr_u8(w, c->pattern_length);
    for (i = 0u; i < MELLO_CHORD_PATTERN_STEPS; ++i) wr_u8(w, c->pattern_masks[i]);
}
/* Out-of-range values are left to mello_engine_chord_set_config's sanitizer. */
static bool decode_chord(Reader *r, MelloChordConfig *c)
{
    uint8_t i, on, trigger, mode, wave, shape, grid_mode, reset, follow;
    if (!rd_u8(r, &on) || !rd_u8(r, &trigger) || !rd_u8(r, &mode) || !rd_u8(r, &c->length) ||
        !rd_u8(r, &c->voices) || !rd_u16(r, &c->level) || !rd_u8(r, &wave) || !rd_u16(r, &c->gate_amount) ||
        !rd_u16(r, &c->attack_ms) || !rd_u16(r, &c->release_ms))
        return false;
    c->on = on != 0u;
    c->trigger = (MelloChordTrigger)trigger;
    c->mode = (MelloChordMode)mode;
    c->wave = (MelloWaveform)wave;
    for (i = 0u; i < MELLO_CHORD_MAX_STEPS; ++i) {
        MelloChordStep *st = &c->steps[i];
        if (!rd_u8(r, &st->degree) || !rd_u8(r, &shape) || !rd_u8(r, &st->inversion) ||
            !rd_i8(r, &st->octave) || !rd_u8(r, &st->spread) || !rd_u8(r, &st->hold))
            return false;
        st->shape = (MelloChordShape)shape;
    }
    if (!rd_u16(r, &c->grid_cells) || !rd_u8(r, &grid_mode) || !rd_u8(r, &c->grid_length) ||
        !rd_u8(r, &reset) || !rd_u8(r, &follow) || !rd_u16(r, &c->grid_amount) ||
        !rd_u8(r, &c->pattern_length))
        return false;
    c->grid_mode = (MelloDotMode)grid_mode;
    c->grid_reset = reset != 0u;
    c->grid_follow = follow != 0u;
    for (i = 0u; i < MELLO_CHORD_PATTERN_STEPS; ++i)
        if (!rd_u8(r, &c->pattern_masks[i])) return false;
    return true;
}
#endif

/* MOD 1 in SEC_WORKING / SEC_SLOTS keeps the HTML's meaning (amount 1000 = +24% cutoff); in the engine it is the
 * depth itself (240 = +24%). SEC_MOD carries the exact depth. */
static uint16_t dot_mod1_wire(uint16_t depth)
{
    uint32_t v = ((uint32_t)depth * 1000u + 120u) / 240u;
    return (uint16_t)(v > 1000u ? 1000u : v);
}
static uint16_t dot_mod1_depth(uint16_t wire)
{
    return (uint16_t)(((uint32_t)wire * 240u + 500u) / 1000u);
}

static void encode_snapshot(Writer *w, const MelloPatternSlot *s)
{
    uint8_t i;
    encode_stored_project(w, &s->project);
    encode_loop(w, &s->loop_cycle);
    for (i = 0u; i < MELLO_DOT_WIRE_COUNT; ++i) {
        MelloDotMatrixConfig d = s->dot[i];
        if (i == MELLO_DOT_MOD1) d.amount = dot_mod1_wire(d.amount);
        encode_dot(w, &d);
    }
    encode_tm(w, &s->tm);
    wr_u8(w, s->tm_bool_len);
    for (i = 0u; i < MELLO_TM_MAX_STEPS; ++i) wr_u8(w, s->tm_bool_reg[i] ? 1u : 0u);
    encode_sub(w, &s->sub);
    wr_u16(w, s->randomize.mask);
    encode_sound(w, &s->sound);
    encode_sample(w, &s->sample);
}
static bool decode_snapshot(Reader *r, MelloPatternSlot *s)
{
    uint8_t i;
    if (!decode_stored_project(r, &s->project) || !decode_loop(r, &s->loop_cycle)) return false;
    for (i = 0u; i < MELLO_DOT_WIRE_COUNT; ++i)
        if (!decode_dot(r, &s->dot[i])) return false;
    /* Not in this wire format: SEC_MOD, if any, sets them later. MOD 1's amount is stored as the HTML's (100% = +24%
     * cutoff) and is now the lane's depth. */
    s->dot[MELLO_DOT_MOD1].amount = dot_mod1_depth(s->dot[MELLO_DOT_MOD1].amount);
    mello_dot_defaults_for(&s->dot[MELLO_DOT_MOD2], MELLO_DOT_MOD2);
    mello_mod_routing_defaults(&s->modr);
    if (!decode_tm(r, &s->tm) || !rd_u8(r, &s->tm_bool_len)) return false;
    for (i = 0u; i < MELLO_TM_MAX_STEPS; ++i)
        if (!rd_u8(r, &s->tm_bool_reg[i])) return false;
    if (!decode_sub(r, &s->sub) || !rd_u16(r, &s->randomize.mask) || !decode_sound(r, &s->sound) ||
        !decode_sample(r, &s->sample))
        return false;
    s->valid = true;
    return true;
}

static void section_header(Writer *w, uint16_t id, uint16_t ver, uint32_t len)
{
    wr_u16(w, id);
    wr_u16(w, ver);
    wr_u32(w, len);
}

/* Phase 5's sound (grit, DECISIONS.md, and the Virus layer) is in its own section so that SEC_WORKING and SEC_SLOTS
 * keep their wire format: an older build skips it and plays a plain saw. Only sounds that differ from the defaults
 * are written; index 0xFF is the working sound. Version 1 (grit only, 8 bytes an entry) is still read. */
#define GRIT_WORKING 0xFFu
static bool has_grit(const MelloSoundConfig *c)
{
    MelloSoundConfig d;
    mello_sound_defaults(&d);
    return c->noise_level != d.noise_level || c->noise_fm != d.noise_fm || c->unison != d.unison ||
           c->detune != d.detune || c->osc2_wave != d.osc2_wave || c->osc2_semi != d.osc2_semi ||
           c->osc2_fine != d.osc2_fine || c->osc2_mix != d.osc2_mix || c->osc2_sync != d.osc2_sync ||
           c->fm_amount != d.fm_amount || c->saturation != d.saturation || c->filter_mode != d.filter_mode ||
           c->menv_amount != d.menv_amount || c->menv_attack_ms != d.menv_attack_ms ||
           c->menv_decay_ms != d.menv_decay_ms || c->menv_target != d.menv_target;
}
static void encode_grit(Writer *w, uint8_t index, const MelloSoundConfig *c)
{
    wr_u8(w, index);
    wr_u16(w, c->noise_level);
    wr_u16(w, c->noise_fm);
    wr_u8(w, c->unison);
    wr_u16(w, c->detune);
    wr_u8(w, (uint8_t)c->osc2_wave);
    wr_i8(w, c->osc2_semi);
    wr_i8(w, c->osc2_fine);
    wr_u16(w, c->osc2_mix);
    wr_u8(w, c->osc2_sync ? 1u : 0u);
    wr_u16(w, c->fm_amount);
    wr_u16(w, c->saturation);
    wr_u8(w, (uint8_t)c->filter_mode);
    wr_u16(w, (uint16_t)c->menv_amount);
    wr_u16(w, c->menv_attack_ms);
    wr_u16(w, c->menv_decay_ms);
    wr_u8(w, (uint8_t)c->menv_target);
}
/* Reads one entry's fields over c (not its index); a version 1 entry has the grit only. */
static bool decode_grit(Reader *r, uint16_t ver, MelloSoundConfig *c)
{
    uint8_t b, sync, mode, target;
    uint16_t amount;
    if (!rd_u16(r, &c->noise_level) || !rd_u16(r, &c->noise_fm) || !rd_u8(r, &c->unison) ||
        !rd_u16(r, &c->detune))
        return false;
    if (ver < 2u) return true;
    if (!rd_u8(r, &b) || !rd_i8(r, &c->osc2_semi) || !rd_i8(r, &c->osc2_fine) || !rd_u16(r, &c->osc2_mix) ||
        !rd_u8(r, &sync) || !rd_u16(r, &c->fm_amount) || !rd_u16(r, &c->saturation) || !rd_u8(r, &mode) ||
        !rd_u16(r, &amount) || !rd_u16(r, &c->menv_attack_ms) || !rd_u16(r, &c->menv_decay_ms) ||
        !rd_u8(r, &target))
        return false;
    c->osc2_wave = (MelloWaveform)b;
    c->osc2_sync = sync != 0u;
    c->filter_mode = (MelloFilterMode)mode;
    c->menv_amount = (int16_t)amount;
    c->menv_target = (MelloModEnvTarget)target;
    return true;
}

/* DOT's MOD 2, its routing and EVOLVE's sound routes (SEC_MOD): written only for a working sound or slot that differs
 * from what a reader without the section would get. */
static bool has_mod(const MelloModRouting *m, const MelloDotMatrixConfig dot[MELLO_DOT_MATRIX_COUNT])
{
    MelloModRouting dm;
    MelloDotMatrixConfig d5;
    mello_mod_routing_defaults(&dm);
    mello_dot_defaults_for(&d5, MELLO_DOT_MOD2);
    const MelloDotMatrixConfig *d = &dot[MELLO_DOT_MOD2];
    uint8_t i;
    for (i = 0u; i < MELLO_DOT_MOD_LANES; ++i) /* field by field: the structs have padding */
        if (m->dot_target[i] != dm.dot_target[i] || m->dot_attack_ms[i] != dm.dot_attack_ms[i] ||
            m->dot_release_ms[i] != dm.dot_release_ms[i] || m->dot_gate[i] != dm.dot_gate[i] ||
            m->tm_dest[i] != dm.tm_dest[i] || m->tm_depth[i] != dm.tm_depth[i])
            return true;
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i)
        if (m->dot_evolve[i]) return true;
    return m->tm_on_loop != dm.tm_on_loop || d->cells != d5.cells || d->mode != d5.mode ||
           d->length != d5.length || d->reset != d5.reset || d->follow != d5.follow ||
           d->amount != d5.amount ||
           dot_mod1_depth(dot_mod1_wire(dot[MELLO_DOT_MOD1].amount)) != dot[MELLO_DOT_MOD1].amount;
}
static void encode_mod(Writer *w, uint8_t index, const MelloModRouting *m,
                       const MelloDotMatrixConfig dot[MELLO_DOT_MATRIX_COUNT])
{
    uint8_t i;
    wr_u8(w, index);
    for (i = 0u; i < MELLO_DOT_MOD_LANES; ++i) {
        wr_u8(w, m->dot_target[i]);
        wr_u16(w, m->dot_attack_ms[i]);
        wr_u16(w, m->dot_release_ms[i]);
        wr_u8(w, m->dot_gate[i] ? 1u : 0u);
        wr_u8(w, m->tm_dest[i]);
        wr_u16(w, (uint16_t)m->tm_depth[i]);
    }
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i) wr_u16(w, m->dot_evolve[i]);
    wr_u8(w, m->tm_on_loop ? 1u : 0u);
    wr_u16(w, dot[MELLO_DOT_MOD1].amount);
    encode_dot(w, &dot[MELLO_DOT_MOD2]);
}
static bool decode_mod(Reader *r, MelloModRouting *m, MelloDotMatrixConfig dot[MELLO_DOT_MATRIX_COUNT])
{
    uint8_t i, b;
    uint16_t v;
    for (i = 0u; i < MELLO_DOT_MOD_LANES; ++i) {
        if (!rd_u8(r, &m->dot_target[i]) || !rd_u16(r, &m->dot_attack_ms[i]) ||
            !rd_u16(r, &m->dot_release_ms[i]) || !rd_u8(r, &b) || !rd_u8(r, &m->tm_dest[i]) || !rd_u16(r, &v))
            return false;
        m->dot_gate[i] = b != 0u;
        m->tm_depth[i] = (int16_t)v;
    }
    for (i = 0u; i < MELLO_DOT_MATRIX_COUNT; ++i)
        if (!rd_u16(r, &m->dot_evolve[i])) return false;
    if (!rd_u8(r, &b) || !rd_u16(r, &dot[MELLO_DOT_MOD1].amount)) return false;
    m->tm_on_loop = b != 0u;
    return decode_dot(r, &dot[MELLO_DOT_MOD2]);
}

static bool encode_payload(Writer *w, const MelloEngine *e)
{
    MelloPatternSlot current;
    uint8_t i, valid_count = 0u;
    if (!w || !e) return false;
    section_header(w, SEC_META, SECVER_META, 1u);
    wr_u8(w, e->visible_slot_count);
    mello_engine_capture_working(e, &current);
    section_header(w, SEC_WORKING, SECVER_WORKING, SNAPSHOT_WIRE_SIZE);
    encode_snapshot(w, &current);
    section_header(w, SEC_FX, SECVER_FX, FX_WIRE_SIZE);
    encode_fx(w, &e->fx);
#if MELLO_FEATURE_CHORD_MACHINE
    section_header(w, SEC_CHORD, SECVER_CHORD, CHORD_WIRE_SIZE);
    encode_chord(w, &e->chord);
#endif
    for (i = 0u; i < MELLO_MAX_SLOTS; ++i)
        if (e->slots[i].valid) valid_count++;
    section_header(w, SEC_SLOTS, SECVER_SLOTS, 1u + (uint32_t)valid_count * SLOT_WIRE_SIZE);
    wr_u8(w, valid_count);
    for (i = 0u; i < MELLO_MAX_SLOTS; ++i) {
        if (e->slots[i].valid) {
            wr_u8(w, i);
            wr_bytes(w, e->slots[i].name, MELLO_SLOT_NAME_CHARS + 1u);
            encode_snapshot(w, &e->slots[i]);
        }
    }
    section_header(w, SEC_SLOT_SEQ, SECVER_SLOT_SEQ, SLOT_SEQ_WIRE_SIZE);
    wr_u8(w, e->slot_seq.active ? 1u : 0u);
    wr_u8(w, e->slot_seq.length);
    for (i = 0u; i < MELLO_MAX_SLOT_SEQ_STEPS; ++i) {
        wr_i8(w, e->slot_seq.steps[i].slot);
        wr_u16(w, e->slot_seq.steps[i].hold);
    }
    {
        uint8_t n = has_grit(&e->sound) ? 1u : 0u;
        for (i = 0u; i < MELLO_MAX_SLOTS; ++i)
            if (e->slots[i].valid && has_grit(&e->slots[i].sound)) n++;
        if (n) {
            section_header(w, SEC_GRIT, SECVER_GRIT, 1u + (uint32_t)n * GRIT_ENTRY_SIZE);
            wr_u8(w, n);
            if (has_grit(&e->sound)) encode_grit(w, GRIT_WORKING, &e->sound);
            for (i = 0u; i < MELLO_MAX_SLOTS; ++i)
                if (e->slots[i].valid && has_grit(&e->slots[i].sound)) encode_grit(w, i, &e->slots[i].sound);
        }
    }
    {
        uint8_t n = has_mod(&e->modr, e->dot) ? 1u : 0u;
        for (i = 0u; i < MELLO_MAX_SLOTS; ++i)
            if (e->slots[i].valid && has_mod(&e->slots[i].modr, e->slots[i].dot)) n++;
        if (n) {
            section_header(w, SEC_MOD, SECVER_MOD, 1u + (uint32_t)n * MOD_ENTRY_SIZE);
            wr_u8(w, n);
            if (has_mod(&e->modr, e->dot)) encode_mod(w, GRIT_WORKING, &e->modr, e->dot);
            for (i = 0u; i < MELLO_MAX_SLOTS; ++i)
                if (e->slots[i].valid && has_mod(&e->slots[i].modr, e->slots[i].dot))
                    encode_mod(w, i, &e->slots[i].modr, e->slots[i].dot);
        }
    }
    if (!w->count_only) wr_flush(w);
    return w->ok;
}

bool mello_storage_measure_payload(const MelloEngine *engine, uint32_t *size_out, uint32_t *crc_out)
{
    Writer w;
    if (!engine || !size_out || !crc_out) return false;
    memset(&w, 0, sizeof(w));
    w.count_only = true;
    w.ok = true;
    w.crc = CRC_INIT;
    if (!encode_payload(&w, engine)) return false;
    *size_out = w.count;
    *crc_out = w.crc ^ CRC_XOR;
    return true;
}

static bool rd_section_header(Reader *r, uint16_t *id, uint16_t *ver, uint32_t *len)
{
    return rd_u16(r, id) && rd_u16(r, ver) && rd_u32(r, len);
}

static bool parse_payload(MelloStorage *s, const RecordInfo *rec, MelloEngine *e, bool apply)
{
    Reader r;
    uint8_t visible = 8u;
    bool have_visible = false;
    if (!s || !rec) return false;
    r.storage = s;
    r.address = rec->payload_addr;
    r.pos = 0u;
    r.length = rec->payload_size;
    r.ok = true;
    if (apply && e) {
        mello_engine_clear_slots(e);
        mello_slot_seq_defaults(&e->slot_seq);
        e->slot_seq_pos = 0u;
        e->slot_seq_hold = 0u;
    }
    while (r.pos < r.length) {
        uint16_t id, ver;
        uint32_t len, start;
        if (r.length - r.pos < 8u || !rd_section_header(&r, &id, &ver, &len) || len > r.length - r.pos)
            return false;
        start = r.pos;
        if (id == SEC_META && ver == SECVER_META) {
            uint8_t v;
            if (len != 1u || !rd_u8(&r, &v)) return false;
            if (apply) {
                visible = v;
                have_visible = true;
            }
        } else if (id == SEC_WORKING && ver == SECVER_WORKING) {
            MelloPatternSlot snap;
            memset(&snap, 0, sizeof(snap));
            if (len != SNAPSHOT_WIRE_SIZE || !decode_snapshot(&r, &snap)) return false;
            if (apply && e && !mello_engine_restore_working(e, &snap)) return false;
        } else if (id == SEC_FX && ver == SECVER_FX) {
            MelloFxConfig fx;
            memset(&fx, 0, sizeof(fx));
            if (len != FX_WIRE_SIZE || !decode_fx(&r, &fx)) return false;
            if (apply && e) mello_engine_fx_set_config(e, &fx);
#if MELLO_FEATURE_CHORD_MACHINE
        } else if (id == SEC_CHORD && ver == SECVER_CHORD) {
            MelloChordConfig chord;
            mello_chord_defaults(&chord);
            if (len != CHORD_WIRE_SIZE || !decode_chord(&r, &chord)) return false;
            if (apply && e) mello_engine_chord_set_config(e, &chord);
#endif
        } else if (id == SEC_SLOTS && ver == SECVER_SLOTS) {
            uint8_t count, i;
            if (len < 1u || !rd_u8(&r, &count) || len != 1u + (uint32_t)count * SLOT_WIRE_SIZE) return false;
            if (count > MELLO_MAX_SLOTS) return false;
            for (i = 0u; i < count; ++i) {
                uint8_t idx;
                MelloPatternSlot slot;
                memset(&slot, 0, sizeof(slot));
                if (!rd_u8(&r, &idx) || idx >= MELLO_MAX_SLOTS ||
                    !rd_bytes(&r, slot.name, MELLO_SLOT_NAME_CHARS + 1u) || !decode_snapshot(&r, &slot))
                    return false;
                slot.name[MELLO_SLOT_NAME_CHARS] = '\0';
                if (apply && e && !mello_engine_import_slot(e, idx, &slot)) return false;
            }
        } else if (id == SEC_SLOT_SEQ && ver == SECVER_SLOT_SEQ) {
            uint8_t active, length, i;
            if (len != SLOT_SEQ_WIRE_SIZE || !rd_u8(&r, &active) || !rd_u8(&r, &length)) return false;
            if (apply && e) {
                mello_slot_seq_defaults(&e->slot_seq);
                mello_engine_slot_seq_set_length(e, length);
                mello_engine_slot_seq_set_active(e, active != 0u);
            }
            for (i = 0u; i < MELLO_MAX_SLOT_SEQ_STEPS; ++i) {
                int8_t slot;
                uint16_t hold;
                if (!rd_i8(&r, &slot) || !rd_u16(&r, &hold)) return false;
                if (apply && e) mello_engine_slot_seq_set_step(e, i, slot, hold);
            }
        } else if (id == SEC_GRIT && (ver == 1u || ver == SECVER_GRIT)) {
            uint8_t count, i;
            uint32_t size = ver == 1u ? GRIT_ENTRY_SIZE_V1 : GRIT_ENTRY_SIZE;
            if (len < 1u || !rd_u8(&r, &count) || len != 1u + (uint32_t)count * size) return false;
            for (i = 0u; i < count; ++i) {
                uint8_t idx;
                MelloSoundConfig c;
                if (!rd_u8(&r, &idx) || (idx != GRIT_WORKING && idx >= MELLO_MAX_SLOTS)) return false;
                c = !e ? (MelloSoundConfig){0} : (idx == GRIT_WORKING ? e->sound : e->slots[idx].sound);
                if (!decode_grit(&r, ver, &c)) return false;
                if (apply && e) {
                    if (idx == GRIT_WORKING)
                        mello_engine_sound_set_config(e, &c); /* sanitizes */
                    else
                        e->slots[idx].sound = c; /* sanitized when the slot loads */
                }
            }
        } else if (id == SEC_MOD && ver == SECVER_MOD) {
            uint8_t count, i;
            if (len < 1u || !rd_u8(&r, &count) || len != 1u + (uint32_t)count * MOD_ENTRY_SIZE) return false;
            for (i = 0u; i < count; ++i) {
                uint8_t idx;
                MelloModRouting m;
                MelloDotMatrixConfig dot[MELLO_DOT_MATRIX_COUNT];
                if (!rd_u8(&r, &idx) || (idx != GRIT_WORKING && idx >= MELLO_MAX_SLOTS)) return false;
                memset(&m, 0, sizeof m);
                memset(dot, 0, sizeof dot);
                if (!decode_mod(&r, &m, dot)) return false;
                if (apply && e) {
                    if (idx == GRIT_WORKING) { /* the setters sanitize */
                        MelloDotMatrixConfig d1 = e->dot[MELLO_DOT_MOD1];
                        d1.amount = dot[MELLO_DOT_MOD1].amount;
                        mello_engine_mod_routing_set(e, &m);
                        mello_engine_dot_set_config(e, MELLO_DOT_MOD1, &d1);
                        mello_engine_dot_set_config(e, MELLO_DOT_MOD2, &dot[MELLO_DOT_MOD2]);
                    } else { /* sanitized when the slot loads */
                        e->slots[idx].modr = m;
                        e->slots[idx].dot[MELLO_DOT_MOD1].amount = dot[MELLO_DOT_MOD1].amount;
                        e->slots[idx].dot[MELLO_DOT_MOD2] = dot[MELLO_DOT_MOD2];
                    }
                }
            }
        } else {
            if (!rd_skip(&r, len)) return false;
        }
        if (r.pos != start + len) return false;
    }
    if (!r.ok || r.pos != r.length) return false;
    if (apply && e) {
        mello_engine_set_visible_slot_count(e, have_visible ? visible : 8u);
        mello_engine_slot_seq_reset_position(e);
    }
    return true;
}

static void storage_put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static void storage_put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}
static uint16_t storage_get_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t storage_get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void make_header(uint8_t h[MELLO_STORAGE_HEADER_SIZE], uint32_t payload_size, uint32_t revision,
                        uint32_t payload_crc)
{
    uint32_t hc;
    memset(h, 0, MELLO_STORAGE_HEADER_SIZE);
    storage_put_u32(h + 0, MELLO_STORAGE_MAGIC);
    storage_put_u16(h + 4, MELLO_STORAGE_FORMAT_VERSION);
    storage_put_u16(h + 6, MELLO_STORAGE_HEADER_SIZE);
    storage_put_u32(h + 8, payload_size);
    storage_put_u32(h + 12, revision);
    storage_put_u32(h + 16, payload_crc);
    hc = mello_storage_crc32(h, 20u);
    storage_put_u32(h + 20, hc);
}

static bool crc_flash(MelloStorage *s, uint32_t addr, uint32_t n, uint32_t *out)
{
    uint8_t buf[256];
    uint32_t c = CRC_INIT, k, i;
    if (!s || !s->io.read || !out) return false;
    while (n) {
        k = n > sizeof(buf) ? (uint32_t)sizeof(buf) : n;
        if (!s->io.read(s->io.ctx, addr, buf, k)) return false;
        for (i = 0u; i < k; ++i) c = crc_step(c, buf[i]);
        addr += k;
        n -= k;
    }
    *out = c ^ CRC_XOR;
    return true;
}

static RecordInfo scan_bank(MelloStorage *s, uint8_t bank)
{
    RecordInfo rec;
    uint8_t h[MELLO_STORAGE_HEADER_SIZE], f[MELLO_STORAGE_FOOTER_SIZE];
    uint32_t base, ps, rev, pcrc, hcrc, actual;
    uint16_t ver, hs;
    memset(&rec, 0, sizeof(rec));
    rec.bank = bank;
    if (!s || bank > 1u || !s->io.read) return rec;
    base = s->config.bank_addr[bank];
    if (!s->io.read(s->io.ctx, base, h, sizeof(h))) {
        rec.io_error = true;
        return rec;
    }
    if (storage_get_u32(h) != MELLO_STORAGE_MAGIC) return rec;
    rec.present = true;
    ver = storage_get_u16(h + 4);
    rec.version = ver;
    hs = storage_get_u16(h + 6);
    rec.revision = storage_get_u32(h + 12);
    if (ver != MELLO_STORAGE_FORMAT_VERSION) {
        rec.unsupported = true;
        return rec;
    }
    if (hs != MELLO_STORAGE_HEADER_SIZE) return rec;
    ps = storage_get_u32(h + 8);
    rev = rec.revision;
    pcrc = storage_get_u32(h + 16);
    hcrc = storage_get_u32(h + 20);
    if (mello_storage_crc32(h, 20u) != hcrc) return rec;
    if (ps > s->config.bank_size - MELLO_STORAGE_HEADER_SIZE - MELLO_STORAGE_FOOTER_SIZE) return rec;
    if (!s->io.read(s->io.ctx, base + MELLO_STORAGE_HEADER_SIZE + ps, f, sizeof(f))) {
        rec.io_error = true;
        return rec;
    }
    if (storage_get_u32(f) != MELLO_STORAGE_COMMIT_MAGIC || storage_get_u32(f + 4) != (rev ^ 0xFFFFFFFFu))
        return rec;
    if (!crc_flash(s, base + MELLO_STORAGE_HEADER_SIZE, ps, &actual)) {
        rec.io_error = true;
        return rec;
    }
    if (actual != pcrc) return rec;
    rec.valid = true;
    rec.payload_size = ps;
    rec.payload_crc = pcrc;
    rec.payload_addr = base + MELLO_STORAGE_HEADER_SIZE;
    return rec;
}

static bool revision_newer(uint32_t a, uint32_t b)
{
    uint32_t d = a - b;
    return a != b && d < 0x80000000u;
}

void mello_storage_config_defaults(MelloStorageConfig *c)
{
    if (!c) return;
    c->bank_addr[0] = MELLO_STORAGE_BANK0_ADDR;
    c->bank_addr[1] = MELLO_STORAGE_BANK1_ADDR;
    c->bank_size = MELLO_STORAGE_BANK_SIZE;
    c->sector_size = MELLO_STORAGE_SECTOR_SIZE;
    c->autosave_debounce_ms = MELLO_STORAGE_AUTOSAVE_DEBOUNCE_MS;
    c->autosave_min_interval_ms = MELLO_STORAGE_AUTOSAVE_MIN_INTERVAL_MS;
}

void mello_storage_init(MelloStorage *s, const MelloStorageIo *io, const MelloStorageConfig *cfg)
{
    MelloStorageConfig d;
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->active_bank = -1;
    if (io) s->io = *io;
    if (cfg)
        s->config = *cfg;
    else {
        mello_storage_config_defaults(&d);
        s->config = d;
    }
    s->last_load_result = MELLO_STORAGE_LOAD_NONE;
    s->last_save_result = MELLO_STORAGE_SAVE_IDLE;
}

static void set_baseline(MelloStorage *s, uint32_t size, uint32_t crc)
{
    s->baseline_valid = true;
    s->baseline_size = size;
    s->baseline_crc = crc;
}

MelloStorageLoadResult mello_storage_load(MelloStorage *s, MelloEngine *e)
{
    RecordInfo a, b, *first, *second;
    bool ua, ub, fallback_hint = false;
    if (!s || !e || !s->io.read) return MELLO_STORAGE_LOAD_IO_ERROR;
    s->baseline_valid = false; /* nothing usable: poll takes the state after load as the baseline */
    a = scan_bank(s, 0u);
    b = scan_bank(s, 1u);
    ua = a.unsupported;
    ub = b.unsupported;
    if (!a.valid && !b.valid) {
        s->active_bank = -1;
        s->revision = 0u;
        s->active_payload_size = 0u;
        s->active_payload_crc = 0u;
        if (a.io_error || b.io_error)
            s->last_load_result = MELLO_STORAGE_LOAD_IO_ERROR;
        else if (ua || ub)
            s->last_load_result = MELLO_STORAGE_LOAD_UNSUPPORTED;
        else if (a.present || b.present)
            s->last_load_result = MELLO_STORAGE_LOAD_CORRUPT;
        else
            s->last_load_result = MELLO_STORAGE_LOAD_NONE;
        return s->last_load_result;
    }
    /* Never silently roll back over data from a newer format. */
    if (ua && (!b.valid || revision_newer(a.revision, b.revision))) {
        s->last_load_result = MELLO_STORAGE_LOAD_UNSUPPORTED;
        return s->last_load_result;
    }
    if (ub && (!a.valid || revision_newer(b.revision, a.revision))) {
        s->last_load_result = MELLO_STORAGE_LOAD_UNSUPPORTED;
        return s->last_load_result;
    }
    if (a.valid && b.valid) {
        if (revision_newer(b.revision, a.revision)) {
            first = &b;
            second = &a;
        } else {
            first = &a;
            second = &b;
        }
    } else if (a.valid) {
        first = &a;
        second = NULL;
        if (b.present && !b.unsupported && revision_newer(b.revision, a.revision)) fallback_hint = true;
    } else {
        first = &b;
        second = NULL;
        if (a.present && !a.unsupported && revision_newer(a.revision, b.revision)) fallback_hint = true;
    }
    if (parse_payload(s, first, e, false) && parse_payload(s, first, e, true)) {
        s->active_bank = (int8_t)first->bank;
        s->revision = first->revision;
        s->active_payload_size = first->payload_size;
        s->active_payload_crc = first->payload_crc;
        set_baseline(s, first->payload_size, first->payload_crc);
        s->dirty = false;
        s->manual_pending = false;
        s->last_load_result = fallback_hint ? MELLO_STORAGE_LOAD_FALLBACK : MELLO_STORAGE_LOAD_OK;
        return s->last_load_result;
    }
    if (second && parse_payload(s, second, e, false) && parse_payload(s, second, e, true)) {
        s->active_bank = (int8_t)second->bank;
        s->revision = second->revision;
        s->active_payload_size = second->payload_size;
        s->active_payload_crc = second->payload_crc;
        set_baseline(s, second->payload_size, second->payload_crc);
        s->dirty = false;
        s->manual_pending = false;
        s->last_load_result = MELLO_STORAGE_LOAD_FALLBACK;
        return s->last_load_result;
    }
    s->last_load_result = MELLO_STORAGE_LOAD_CORRUPT;
    return s->last_load_result;
}

static bool erase_bank(MelloStorage *s, uint8_t bank)
{
    uint32_t off, base;
    if (!s || !s->io.erase4k || s->config.sector_size == 0u ||
        s->config.bank_size % s->config.sector_size != 0u)
        return false;
    base = s->config.bank_addr[bank];
    for (off = 0u; off < s->config.bank_size; off += s->config.sector_size)
        if (!s->io.erase4k(s->io.ctx, base + off)) return false;
    return true;
}

MelloStorageSaveResult mello_storage_save_now(MelloStorage *s, const MelloEngine *e, uint32_t now_ms)
{
    uint32_t size, crc, rev, base, verify_crc;
    uint8_t bank, h[MELLO_STORAGE_HEADER_SIZE], f[MELLO_STORAGE_FOOTER_SIZE];
    Writer w;
    RecordInfo check;
    if (!s || !e || !s->io.read || !s->io.erase4k || !s->io.program) {
        if (s) s->last_save_result = MELLO_STORAGE_SAVE_IO_ERROR;
        return MELLO_STORAGE_SAVE_IO_ERROR;
    }
    if (!mello_storage_measure_payload(e, &size, &crc)) {
        s->last_save_result = MELLO_STORAGE_SAVE_IO_ERROR;
        return s->last_save_result;
    }
    if (size > s->config.bank_size - MELLO_STORAGE_HEADER_SIZE - MELLO_STORAGE_FOOTER_SIZE) {
        s->last_save_result = MELLO_STORAGE_SAVE_TOO_LARGE;
        return s->last_save_result;
    }
    if (s->active_bank >= 0 && size == s->active_payload_size && crc == s->active_payload_crc) {
        set_baseline(s, size, crc);
        s->dirty = false;
        s->manual_pending = false;
        s->last_save_ms = now_ms;
        s->last_save_result = MELLO_STORAGE_SAVE_UNCHANGED;
        return s->last_save_result;
    }
    bank = s->active_bank == 0 ? 1u : 0u;
    rev = s->active_bank < 0 ? 1u : s->revision + 1u;
    base = s->config.bank_addr[bank];
    /* After a rollback, a newer firmware's project is the user's newest work and this firmware cannot read
     * it. While either bank holds one, write nothing: neither that bank nor the other, which may hold the
     * user's previous project (load then runs on defaults). Installing the newer firmware again brings it
     * back. An older format is still replaced, so an upgrade can save. */
    {
        RecordInfo b0 = scan_bank(s, 0u), b1 = scan_bank(s, 1u);
        if (b0.io_error || b1.io_error) {
            s->last_save_result = MELLO_STORAGE_SAVE_IO_ERROR;
            return s->last_save_result;
        }
        if ((b0.present && b0.version > MELLO_STORAGE_FORMAT_VERSION) ||
            (b1.present && b1.version > MELLO_STORAGE_FORMAT_VERSION)) {
            set_baseline(s, size, crc); /* do not retry until the project changes again */
            s->dirty = false;
            s->manual_pending = false;
            s->last_save_result = MELLO_STORAGE_SAVE_PROTECTED;
            return s->last_save_result;
        }
    }
    if (!erase_bank(s, bank)) {
        s->last_save_result = MELLO_STORAGE_SAVE_IO_ERROR;
        return s->last_save_result;
    }
    make_header(h, size, rev, crc);
    if (!s->io.program(s->io.ctx, base, h, sizeof(h))) {
        s->last_save_result = MELLO_STORAGE_SAVE_IO_ERROR;
        return s->last_save_result;
    }
    memset(&w, 0, sizeof(w));
    w.storage = s;
    w.address = base + MELLO_STORAGE_HEADER_SIZE;
    w.crc = CRC_INIT;
    w.ok = true;
    if (!encode_payload(&w, e) || w.count != size || (w.crc ^ CRC_XOR) != crc) {
        s->last_save_result = MELLO_STORAGE_SAVE_IO_ERROR;
        return s->last_save_result;
    }
    if (!crc_flash(s, base + MELLO_STORAGE_HEADER_SIZE, size, &verify_crc) || verify_crc != crc) {
        s->last_save_result = MELLO_STORAGE_SAVE_IO_ERROR;
        return s->last_save_result;
    }
    storage_put_u32(f, MELLO_STORAGE_COMMIT_MAGIC);
    storage_put_u32(f + 4, rev ^ 0xFFFFFFFFu);
    if (!s->io.program(s->io.ctx, base + MELLO_STORAGE_HEADER_SIZE + size, f, sizeof(f))) {
        s->last_save_result = MELLO_STORAGE_SAVE_IO_ERROR;
        return s->last_save_result;
    }
    check = scan_bank(s, bank);
    if (!check.valid || check.revision != rev || check.payload_size != size || check.payload_crc != crc) {
        s->last_save_result = MELLO_STORAGE_SAVE_IO_ERROR;
        return s->last_save_result;
    }
    s->active_bank = (int8_t)bank;
    s->revision = rev;
    s->active_payload_size = size;
    s->active_payload_crc = crc;
    set_baseline(s, size, crc);
    s->dirty = false;
    s->manual_pending = false;
    s->last_save_ms = now_ms;
    s->last_save_result = MELLO_STORAGE_SAVE_WRITTEN;
    return s->last_save_result;
}

void mello_storage_mark_dirty(MelloStorage *s, uint32_t now_ms)
{
    if (!s) return;
    s->dirty = true;
    s->dirty_since_ms = now_ms;
}
void mello_storage_request_save(MelloStorage *s, uint32_t now_ms)
{
    if (!s) return;
    s->dirty = true;
    s->manual_pending = true;
    s->dirty_since_ms = now_ms;
}

MelloStorageSaveResult mello_storage_poll(MelloStorage *s, const MelloEngine *e, uint32_t now_ms,
                                          bool safe_to_write)
{
    if (!s || !e) return MELLO_STORAGE_SAVE_IO_ERROR;
    if (!s->dirty &&
        (!s->baseline_valid || (uint32_t)(now_ms - s->last_check_ms) >= MELLO_STORAGE_CHANGE_CHECK_MS)) {
        /* Catch every change to what a save would write, whatever made it (UI, MIDI, RANDOM, ...). */
        uint32_t size, crc;
        s->last_check_ms = now_ms;
        if (mello_storage_measure_payload(e, &size, &crc)) {
            if (!s->baseline_valid)
                set_baseline(s, size, crc);
            else if (size != s->baseline_size || crc != s->baseline_crc)
                mello_storage_mark_dirty(s, now_ms);
        }
    }
    if (!s->dirty) {
        s->last_save_result = MELLO_STORAGE_SAVE_IDLE;
        return s->last_save_result;
    }
    if (!safe_to_write) {
        s->last_save_result = MELLO_STORAGE_SAVE_DEFERRED;
        return s->last_save_result;
    }
    if (!s->manual_pending) {
        if ((uint32_t)(now_ms - s->dirty_since_ms) < s->config.autosave_debounce_ms) {
            s->last_save_result = MELLO_STORAGE_SAVE_DEFERRED;
            return s->last_save_result;
        }
        if (s->active_bank >= 0 &&
            (uint32_t)(now_ms - s->last_save_ms) < s->config.autosave_min_interval_ms) {
            s->last_save_result = MELLO_STORAGE_SAVE_DEFERRED;
            return s->last_save_result;
        }
    }
    return mello_storage_save_now(s, e, now_ms);
}
