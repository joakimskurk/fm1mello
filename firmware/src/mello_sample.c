#include "mello_sample.h"

#define SAMPLE_MAGIC     0x30314D53u /* SM10 little-endian */
#define SAMPLE_COMMIT    0x21504D53u /* SMP! */
#define SAMPLE_VERSION   1u
#define SAMPLE_CODEC_IMA 1u
#define CRC_INIT         0xFFFFFFFFu
#define CRC_XOR          0xFFFFFFFFu

static const int16_t IMA_STEP[89] = {
    7,    8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,   28,
    31,   34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,  118,
    130,  143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,  494,
    544,  598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878, 2066,
    2272, 2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845, 8630,
    9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
static const int8_t IMA_INDEX[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

static uint32_t sample_crc_step(uint32_t c, uint8_t b)
{
    uint8_t i;
    c ^= b;
    for (i = 0u; i < 8u; ++i) c = (c & 1u) ? ((c >> 1) ^ 0xEDB88320u) : (c >> 1);
    return c;
}
static uint32_t crc32_bytes(const uint8_t *p, uint32_t n)
{
    uint32_t c = CRC_INIT, i;
    for (i = 0u; i < n; ++i) c = sample_crc_step(c, p[i]);
    return c ^ CRC_XOR;
}
static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void idle(MelloSampleStore *s)
{
    if (s && s->io.idle) s->io.idle(s->io.ctx);
}
static uint32_t slot_addr(uint8_t slot)
{
    return MELLO_SAMPLE_FLASH_BASE + (uint32_t)slot * MELLO_SAMPLE_FLASH_SLOT_SIZE;
}

static bool parse_header(const uint8_t h[MELLO_SAMPLE_RECORD_HEADER], uint8_t expected_slot,
                         MelloSampleSlotMeta *m, uint32_t *total)
{
    uint32_t hc, stored;
    uint8_t nl;
    uint16_t bc;
    if (rd32(h) != SAMPLE_MAGIC || rd16(h + 4) != SAMPLE_VERSION || h[6] != expected_slot ||
        h[7] != SAMPLE_CODEC_IMA)
        return false;
    if (rd16(h + 8) != MELLO_SAMPLE_SOURCE_RATE || rd16(h + 10) != MELLO_SAMPLE_BLOCK_SAMPLES) return false;
    stored = rd32(h + 56);
    hc = crc32_bytes(h, 56u);
    if (stored != hc) return false;
    if (m) {
        memset(m, 0, sizeof *m);
        m->sample_rate = rd16(h + 8);
        m->sample_count = rd32(h + 12);
        bc = rd16(h + 16);
        m->block_count = bc;
        m->data_bytes = rd32(h + 20);
        m->data_crc = rd32(h + 24);
        nl = h[28];
        if (nl > MELLO_SAMPLE_NAME_CHARS) nl = MELLO_SAMPLE_NAME_CHARS;
        memcpy(m->name, h + 32, nl);
        m->name[nl] = '\0';
    } else
        bc = rd16(h + 16);
    if (rd32(h + 20) > MELLO_SAMPLE_DATA_MAX || bc > MELLO_SAMPLE_MAX_BLOCKS ||
        rd32(h + 12) > (uint32_t)bc * MELLO_SAMPLE_BLOCK_SAMPLES)
        return false;
    if (rd32(h + 20) != (uint32_t)bc * MELLO_SAMPLE_BLOCK_BYTES) return false;
    if (total) *total = MELLO_SAMPLE_RECORD_HEADER + rd32(h + 20) + MELLO_SAMPLE_RECORD_FOOTER;
    return true;
}

void mello_sample_bank_init(MelloSampleBank *b, uint8_t *a, uint8_t *c, uint8_t *d, uint8_t *e, uint32_t cap)
{
    uint8_t i;
    uint8_t *p[4] = {a, c, d, e};
    if (!b) return;
    memset(b, 0, sizeof *b);
    for (i = 0u; i < 4u; ++i) {
        b->data[i] = p[i];
        b->capacity[i] = cap;
    }
}
void mello_sample_store_init(MelloSampleStore *s, const MelloSampleIo *io, MelloSampleBank *b)
{
    if (!s) return;
    memset(s, 0, sizeof *s);
    if (io) s->io = *io;
    s->bank = b;
}

static bool read_valid_record(MelloSampleStore *s, uint32_t base, uint8_t expected_slot,
                              MelloSampleSlotMeta *meta, bool load_data)
{
    uint8_t h[MELLO_SAMPLE_RECORD_HEADER], f[MELLO_SAMPLE_RECORD_FOOTER];
    uint32_t total, db, crc = CRC_INIT, pos = 0u, k;
    MelloSampleSlotMeta m;
    if (!s || !s->io.read || !s->io.read(s->io.ctx, base, h, sizeof h) ||
        !parse_header(h, expected_slot, &m, &total))
        return false;
    db = m.data_bytes;
    if (total > MELLO_SAMPLE_FLASH_SLOT_SIZE ||
        !s->io.read(s->io.ctx, base + MELLO_SAMPLE_RECORD_HEADER + db, f, sizeof f))
        return false;
    if (rd32(f) != SAMPLE_COMMIT || rd32(f + 4) != m.data_crc) return false;
    if (load_data && (!s->bank || !s->bank->data[expected_slot] || s->bank->capacity[expected_slot] < db))
        return false;
    while (pos < db) {
        k = db - pos;
        if (k > sizeof s->scratch) k = sizeof s->scratch;
        if (!s->io.read(s->io.ctx, base + MELLO_SAMPLE_RECORD_HEADER + pos, s->scratch, k)) return false;
        {
            uint32_t i;
            for (i = 0u; i < k; ++i) crc = sample_crc_step(crc, s->scratch[i]);
        }
        if (load_data) memcpy(s->bank->data[expected_slot] + pos, s->scratch, k);
        pos += k;
        idle(s);
    }
    crc ^= CRC_XOR;
    if (crc != m.data_crc) return false;
    m.loaded = true;
    if (meta) *meta = m;
    if (load_data) s->bank->meta[expected_slot] = m;
    return true;
}

static bool erase_region(MelloSampleStore *s, uint32_t base)
{
    uint32_t o;
    if (!s || !s->io.erase4k) return false;
    for (o = 0u; o < MELLO_SAMPLE_FLASH_SLOT_SIZE; o += 0x1000u) {
        if (!s->io.erase4k(s->io.ctx, base + o)) return false;
        idle(s);
    }
    return true;
}
static bool copy_record(MelloSampleStore *s, uint32_t src, uint32_t dst, uint8_t slot)
{
    uint8_t h[MELLO_SAMPLE_RECORD_HEADER];
    uint32_t total, pos = 0u, k;
    if (!s || !s->io.read || !s->io.program) return false;
    if (!s->io.read(s->io.ctx, src, h, sizeof h) || !parse_header(h, slot, 0, &total)) return false;
    if (!erase_region(s, dst)) return false;
    while (pos < total) {
        k = total - pos;
        if (k > sizeof s->scratch) k = sizeof s->scratch;
        if (!s->io.read(s->io.ctx, src + pos, s->scratch, k) ||
            !s->io.program(s->io.ctx, dst + pos, s->scratch, k))
            return false;
        pos += k;
        idle(s);
    }
    return read_valid_record(s, dst, slot, 0, false);
}

bool mello_sample_store_recover_staging(MelloSampleStore *s)
{
    uint8_t h[MELLO_SAMPLE_RECORD_HEADER], slot;
    uint32_t total;
    if (!s || !s->io.read) return false;
    if (!s->io.read(s->io.ctx, MELLO_SAMPLE_FLASH_STAGING_ADDR, h, sizeof h)) return true;
    slot = h[6];
    if (slot >= MELLO_SAMPLE_SLOT_COUNT || !parse_header(h, slot, 0, &total) ||
        !read_valid_record(s, MELLO_SAMPLE_FLASH_STAGING_ADDR, slot, 0, false))
        return true;
    if (!read_valid_record(s, slot_addr(slot), slot, 0, false)) {
        if (!copy_record(s, MELLO_SAMPLE_FLASH_STAGING_ADDR, slot_addr(slot), slot)) return false;
    }
    return true;
}

bool mello_sample_store_load_slot(MelloSampleStore *s, uint8_t slot)
{
    if (!s || !s->bank || slot >= MELLO_SAMPLE_SLOT_COUNT) return false;
    memset(&s->bank->meta[slot], 0, sizeof s->bank->meta[slot]);
    return read_valid_record(s, slot_addr(slot), slot, &s->bank->meta[slot], true);
}
bool mello_sample_store_load_all(MelloSampleStore *s)
{
    uint8_t i;
    bool ok = true;
    if (!s) return false;
    if (!mello_sample_store_recover_staging(s)) ok = false;
    for (i = 0u; i < MELLO_SAMPLE_SLOT_COUNT; ++i) (void)mello_sample_store_load_slot(s, i);
    return ok;
}
bool mello_sample_store_clear_slot(MelloSampleStore *s, uint8_t slot)
{
    if (!s || slot >= MELLO_SAMPLE_SLOT_COUNT) return false;
    if (!erase_region(s, slot_addr(slot))) return false;
    if (s->bank) memset(&s->bank->meta[slot], 0, sizeof s->bank->meta[slot]);
    return true;
}

bool mello_sample_decode_block(const MelloSampleBank *b, uint8_t slot, uint16_t block,
                               int16_t out[MELLO_SAMPLE_BLOCK_SAMPLES], uint16_t *valid)
{
    const uint8_t *p;
    int32_t pred, idx, step, diff;
    uint16_t i, count;
    uint8_t nib;
    if (!b || slot >= MELLO_SAMPLE_SLOT_COUNT || !out || !b->meta[slot].loaded ||
        block >= b->meta[slot].block_count)
        return false;
    p = b->data[slot] + (uint32_t)block * MELLO_SAMPLE_BLOCK_BYTES;
    pred = (int16_t)rd16(p);
    idx = p[2];
    if (idx > 88) idx = 88;
    out[0] = (int16_t)pred;
    for (i = 1u; i < MELLO_SAMPLE_BLOCK_SAMPLES; ++i) {
        uint16_t ni = (uint16_t)(i - 1u);
        uint8_t by = p[4u + (ni >> 1)];
        nib = (ni & 1u) ? (uint8_t)(by >> 4) : (uint8_t)(by & 15u);
        step = IMA_STEP[idx];
        diff = step >> 3;
        if (nib & 1u) diff += step >> 2;
        if (nib & 2u) diff += step >> 1;
        if (nib & 4u) diff += step;
        if (nib & 8u)
            pred -= diff;
        else
            pred += diff;
        if (pred > 32767) pred = 32767;
        if (pred < -32768) pred = -32768;
        idx += IMA_INDEX[nib];
        if (idx < 0) idx = 0;
        if (idx > 88) idx = 88;
        out[i] = (int16_t)pred;
    }
    count = MELLO_SAMPLE_BLOCK_SAMPLES;
    if ((uint32_t)(block + 1u) * MELLO_SAMPLE_BLOCK_SAMPLES > b->meta[slot].sample_count)
        count = (uint16_t)(b->meta[slot].sample_count - (uint32_t)block * MELLO_SAMPLE_BLOCK_SAMPLES);
    if (valid) *valid = count;
    return true;
}

static void ack(MelloSampleTransfer *x, uint8_t ok, uint8_t arg)
{
    if (x && x->midi_send)
        x->midi_send(x->midi_ctx, MELLO_SAMPLE_MIDI_CTRL_STATUS,
                     ok ? MELLO_SAMPLE_CTRL_ACK : MELLO_SAMPLE_CTRL_ERROR, arg);
}
static void xflush(MelloSampleTransfer *x)
{
    if (!x || !x->active || x->failed || x->buf_used == 0u) return;
    if (!x->store || !x->store->io.program ||
        !x->store->io.program(x->store->io.ctx, MELLO_SAMPLE_FLASH_STAGING_ADDR + x->flash_written, x->buf,
                              x->buf_used)) {
        x->failed = true;
        return;
    }
    x->flash_written += x->buf_used;
    x->buf_used = 0u;
    idle(x->store);
}
static void xraw(MelloSampleTransfer *x, uint8_t b)
{
    if (!x || !x->active || x->failed) return;
    if (x->expected_total && x->raw_received >= x->expected_total) return;
    x->buf[x->buf_used++] = b;
    x->raw_received++;
    if (x->buf_used == sizeof x->buf) xflush(x);
    if (!x->expected_total && x->raw_received >= MELLO_SAMPLE_RECORD_HEADER &&
        x->flash_written + x->buf_used >= MELLO_SAMPLE_RECORD_HEADER) {
        uint8_t h[MELLO_SAMPLE_RECORD_HEADER];
        uint32_t avail = 0u;
        if (x->flash_written == 0u && x->buf_used >= MELLO_SAMPLE_RECORD_HEADER) {
            memcpy(h, x->buf, MELLO_SAMPLE_RECORD_HEADER);
            avail = MELLO_SAMPLE_RECORD_HEADER;
        } else if (x->store && x->store->io.read &&
                   x->store->io.read(x->store->io.ctx, MELLO_SAMPLE_FLASH_STAGING_ADDR, h, sizeof h))
            avail = MELLO_SAMPLE_RECORD_HEADER;
        if (avail) {
            uint32_t total;
            if (!parse_header(h, x->slot, 0, &total) || total > MELLO_SAMPLE_FLASH_SLOT_SIZE) {
                x->failed = true;
            } else
                x->expected_total = total;
        }
    }
}
static void xdigit(MelloSampleTransfer *x, uint8_t v)
{
    uint8_t i, mask;
    if (!x || !x->active || x->failed) return;
    x->wire7[x->wire7_used++] = (uint8_t)(v & 0x7Fu);
    if (x->wire7_used == 8u) {
        mask = x->wire7[0];
        for (i = 0u; i < 7u; ++i) xraw(x, (uint8_t)(x->wire7[i + 1u] | (((mask >> i) & 1u) << 7)));
        x->wire7_used = 0u;
    }
}

void mello_sample_transfer_init(MelloSampleTransfer *x, MelloSampleStore *s, MelloSampleMidiSendFn send,
                                void *ctx)
{
    if (!x) return;
    memset(x, 0, sizeof *x);
    x->store = s;
    x->midi_send = send;
    x->midi_ctx = ctx;
}
void mello_sample_transfer_abort(MelloSampleTransfer *x)
{
    if (!x) return;
    x->active = false;
    x->failed = false;
    x->wire7_used = 0u;
    x->raw_received = 0u;
    x->expected_total = 0u;
    x->flash_written = 0u;
    x->buf_used = 0u;
}
bool mello_sample_transfer_active(const MelloSampleTransfer *x)
{
    return x && x->active;
}

static bool finish_transfer(MelloSampleTransfer *x)
{
    uint8_t h[MELLO_SAMPLE_RECORD_HEADER];
    uint32_t total;
    if (!x || !x->active) return false;
    xflush(x);
    if (x->failed || !x->expected_total || x->raw_received < x->expected_total) return false;
    if (!x->store->io.read(x->store->io.ctx, MELLO_SAMPLE_FLASH_STAGING_ADDR, h, sizeof h) ||
        !parse_header(h, x->slot, 0, &total) || total != x->expected_total)
        return false;
    if (!read_valid_record(x->store, MELLO_SAMPLE_FLASH_STAGING_ADDR, x->slot, 0, false)) return false;
    if (!copy_record(x->store, MELLO_SAMPLE_FLASH_STAGING_ADDR, slot_addr(x->slot), x->slot)) return false;
    if (!mello_sample_store_load_slot(x->store, x->slot)) return false;
    return true;
}

bool mello_sample_transfer_midi_in(MelloSampleTransfer *x, uint8_t st, uint8_t d1, uint8_t d2, bool safe)
{
    uint8_t slot;
    if (!x) return false;
    if (st == MELLO_SAMPLE_MIDI_DATA_STATUS) {
        if (x->active) {
            xdigit(x, d1);
            xdigit(x, d2);
        }
        return x->active;
    }
    if (st != MELLO_SAMPLE_MIDI_CTRL_STATUS) return false;
    if (d1 == MELLO_SAMPLE_CTRL_START) {
        slot = d2;
        if (slot >= MELLO_SAMPLE_SLOT_COUNT || !safe || !x->store ||
            !erase_region(x->store, MELLO_SAMPLE_FLASH_STAGING_ADDR)) {
            ack(x, 0u, slot);
            return true;
        }
        mello_sample_transfer_abort(x);
        x->active = true;
        x->slot = slot;
        ack(x, 1u, slot);
        return true;
    }
    if (d1 == MELLO_SAMPLE_CTRL_ABORT) {
        mello_sample_transfer_abort(x);
        ack(x, 1u, d2);
        return true;
    }
    if (d1 == MELLO_SAMPLE_CTRL_CLEAR) {
        slot = d2;
        if (slot >= MELLO_SAMPLE_SLOT_COUNT || !safe || !mello_sample_store_clear_slot(x->store, slot)) {
            ack(x, 0u, slot);
            return true;
        }
        ack(x, 1u, slot);
        return true;
    }
    if (d1 == MELLO_SAMPLE_CTRL_END) {
        slot = x->slot;
        if (!safe || !finish_transfer(x)) {
            mello_sample_transfer_abort(x);
            ack(x, 0u, slot);
            return true;
        }
        mello_sample_transfer_abort(x);
        ack(x, 1u, slot);
        return true;
    }
    if (d1 == MELLO_SAMPLE_CTRL_QUERY) {
        slot = d2;
        ack(x,
            slot < MELLO_SAMPLE_SLOT_COUNT && x->store && x->store->bank && x->store->bank->meta[slot].loaded,
            slot);
        return true;
    }
    return false;
}
