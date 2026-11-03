/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The sample transfer over MIDI (mello_sample.c, tools/fm1_samples.py): a record built the way the host tool
 * builds it goes in as START, DATA digits and END; the device answers ACK, the slot plays the sound back, and
 * it is still there after a power cycle. A START while the engine runs is refused, a corrupted digit is caught
 * by the CRC and leaves the old sample alone, CLEAR and QUERY answer. When tests/run_mello.sh could run the
 * Python tool, its record (py_record.bin, a 440 Hz sine named PYTONE in slot 1) is fed in too: the tool's
 * encoder against the firmware's decoder. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mello_sample.h"

#define FLASH_SIZE 0x100000u
static uint8_t flash[FLASH_SIZE];
static uint32_t erases, programs;

static bool ram_read(void *ctx, uint32_t a, void *dst, uint32_t n)
{
    (void)ctx;
    if (a + n > FLASH_SIZE) return false;
    memcpy(dst, flash + a, n);
    return true;
}
static bool ram_erase4k(void *ctx, uint32_t a)
{
    (void)ctx;
    if (a + 0x1000u > FLASH_SIZE || (a & 0xFFFu)) return false;
    memset(flash + a, 0xFF, 0x1000u);
    erases++;
    return true;
}
static bool ram_program(void *ctx, uint32_t a, const void *src, uint32_t n)
{
    uint32_t i;
    (void)ctx;
    if (a + n > FLASH_SIZE) return false;
    for (i = 0u; i < n; ++i) flash[a + i] &= ((const uint8_t *)src)[i]; /* NOR: only 1 -> 0 */
    programs++;
    return true;
}

/* the device's answers */
static uint8_t acks[64][3];
static uint32_t n_acks;
static void capture(void *ctx, uint8_t st, uint8_t d1, uint8_t d2)
{
    (void)ctx;
    if (n_acks < 64u) {
        acks[n_acks][0] = st;
        acks[n_acks][1] = d1;
        acks[n_acks][2] = d2;
    }
    n_acks++;
}
static int last_ack_ok(void)
{
    if (!n_acks) return -1;
    if (acks[n_acks - 1u][0] != MELLO_SAMPLE_MIDI_CTRL_STATUS) return -1;
    return acks[n_acks - 1u][1] == MELLO_SAMPLE_CTRL_ACK ? 1 : 0;
}

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                      \
            printf(__VA_ARGS__);                                                                             \
            printf("\n");                                                                                    \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

/* ---- the host tool's encoder, in C (tools/fm1_samples.py ima_encode / make_record) */
static const int16_t STEP[89] = {
    7,    8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,   28,
    31,   34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,  118,
    130,  143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,  494,
    544,  598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878, 2066,
    2272, 2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845, 8630,
    9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
static const int8_t INDEX[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

static uint32_t crc32(const uint8_t *p, uint32_t n)
{
    uint32_t c = 0xFFFFFFFFu, i;
    uint8_t k;
    for (i = 0u; i < n; ++i) {
        c ^= p[i];
        for (k = 0u; k < 8u; ++k) c = (c & 1u) ? ((c >> 1) ^ 0xEDB88320u) : (c >> 1);
    }
    return c ^ 0xFFFFFFFFu;
}
static void put16(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static void put32(uint8_t *p, uint32_t v)
{
    put16(p, v & 0xFFFFu);
    put16(p + 2, v >> 16);
}

/* -> record length */
static uint32_t make_record(uint8_t *rec, const int16_t *pcm, uint32_t n, uint8_t slot, const char *name)
{
    uint32_t blocks = (n + MELLO_SAMPLE_BLOCK_SAMPLES - 1u) / MELLO_SAMPLE_BLOCK_SAMPLES, b, i, dbytes;
    uint8_t *data = rec + MELLO_SAMPLE_RECORD_HEADER;
    int32_t idx = 0;
    for (b = 0u; b < blocks; ++b) {
        uint8_t *blk = data + b * MELLO_SAMPLE_BLOCK_BYTES;
        int32_t pred = pcm[b * MELLO_SAMPLE_BLOCK_SAMPLES];
        put16(blk, (uint16_t)pred);
        blk[2] = (uint8_t)idx;
        blk[3] = 0u;
        memset(blk + 4, 0, 128u);
        for (i = 1u; i < MELLO_SAMPLE_BLOCK_SAMPLES; ++i) {
            uint32_t si = b * MELLO_SAMPLE_BLOCK_SAMPLES + i;
            int32_t s = si < n ? pcm[si] : pcm[n - 1u], step = STEP[idx], diff = s - pred, vp = step >> 3;
            uint8_t nib = 0u;
            if (diff < 0) {
                nib = 8u;
                diff = -diff;
            }
            if (diff >= step) {
                nib |= 4u;
                diff -= step;
                vp += step;
            }
            if (diff >= step >> 1) {
                nib |= 2u;
                diff -= step >> 1;
                vp += step >> 1;
            }
            if (diff >= step >> 2) {
                nib |= 1u;
                vp += step >> 2;
            }
            pred = (nib & 8u) ? pred - vp : pred + vp;
            if (pred > 32767) pred = 32767;
            if (pred < -32768) pred = -32768;
            idx += INDEX[nib];
            if (idx < 0) idx = 0;
            if (idx > 88) idx = 88;
            blk[4u + ((i - 1u) >> 1)] |= (uint8_t)(((i - 1u) & 1u) ? nib << 4 : nib);
        }
    }
    dbytes = blocks * MELLO_SAMPLE_BLOCK_BYTES;
    memset(rec, 0, MELLO_SAMPLE_RECORD_HEADER);
    memcpy(rec, "SM10", 4);
    put16(rec + 4, 1u);
    rec[6] = slot;
    rec[7] = 1u;
    put16(rec + 8, MELLO_SAMPLE_SOURCE_RATE);
    put16(rec + 10, MELLO_SAMPLE_BLOCK_SAMPLES);
    put32(rec + 12, n);
    put16(rec + 16, blocks);
    put32(rec + 20, dbytes);
    put32(rec + 24, crc32(data, dbytes));
    rec[28] = (uint8_t)strlen(name);
    memcpy(rec + 32, name, strlen(name));
    put32(rec + 56, crc32(rec, 56u));
    memcpy(data + dbytes, "SMP!", 4);
    put32(data + dbytes + 4, crc32(data, dbytes));
    return MELLO_SAMPLE_RECORD_HEADER + dbytes + MELLO_SAMPLE_RECORD_FOOTER;
}

/* the wire: seven bytes -> a mask digit and seven low-bit digits, two digits a message */
static void send_record(MelloSampleTransfer *x, const uint8_t *rec, uint32_t len, bool safe,
                        uint32_t corrupt_at)
{
    uint8_t digits[8];
    uint32_t i, k, d = 0u;
    uint8_t pend[2];
    uint32_t np = 0u;
    for (i = 0u; i < len; i += 7u) {
        digits[0] = 0u;
        for (k = 0u; k < 7u; ++k) {
            uint8_t b = i + k < len ? rec[i + k] : 0u;
            digits[0] |= (uint8_t)((b >> 7) << k);
            digits[1u + k] = b & 0x7Fu;
        }
        for (k = 0u; k < 8u; ++k) {
            uint8_t v = digits[k];
            if (d == corrupt_at) v ^= 1u;
            d++;
            pend[np++] = v;
            if (np == 2u) {
                (void)mello_sample_transfer_midi_in(x, MELLO_SAMPLE_MIDI_DATA_STATUS, pend[0], pend[1], safe);
                np = 0u;
            }
        }
    }
    if (np) (void)mello_sample_transfer_midi_in(x, MELLO_SAMPLE_MIDI_DATA_STATUS, pend[0], 0u, safe);
}

static double snr_db(const MelloSampleBank *bank, uint8_t slot, const int16_t *ref, uint32_t n)
{
    int16_t out[MELLO_SAMPLE_BLOCK_SAMPLES];
    uint16_t valid, blk;
    double sig = 0.0, err = 0.0;
    uint32_t i, pos = 0u;
    for (blk = 0u; blk < bank->meta[slot].block_count; ++blk) {
        if (!mello_sample_decode_block(bank, slot, blk, out, &valid)) return -1.0;
        for (i = 0u; i < valid && pos < n; ++i, ++pos) {
            double e = (double)out[i] - (double)ref[pos];
            sig += (double)ref[pos] * (double)ref[pos];
            err += e * e;
        }
    }
    if (pos != n || err == 0.0) return pos != n ? -2.0 : 99.0;
    return 10.0 * log10(sig / err);
}

static int16_t tone[MELLO_SAMPLE_MAX_SAMPLES];
static int16_t sine[MELLO_SAMPLE_SOURCE_RATE];
static uint8_t rec[MELLO_SAMPLE_FLASH_SLOT_SIZE], rec2[MELLO_SAMPLE_FLASH_SLOT_SIZE];
static uint8_t pool[MELLO_SAMPLE_SLOT_COUNT][MELLO_SAMPLE_DATA_MAX];

int main(void)
{
    MelloSampleIo io;
    MelloSampleBank bank;
    MelloSampleStore store;
    MelloSampleTransfer x;
    uint32_t len, len2, i, n = 50000u, e0;
    double snr;

    memset(flash, 0xFF, sizeof flash);
    memset(&io, 0, sizeof io);
    io.read = ram_read;
    io.erase4k = ram_erase4k;
    io.program = ram_program;
    mello_sample_bank_init(&bank, pool[0], pool[1], pool[2], pool[3], MELLO_SAMPLE_DATA_MAX);
    mello_sample_store_init(&store, &io, &bank);
    CHECK(mello_sample_store_load_all(&store), "load_all on blank flash");
    mello_sample_transfer_init(&x, &store, capture, 0);

    /* a 2.27 s pluck: a decaying tone with a bright attack, the longest kind of sound a slot takes */
    for (i = 0u; i < n; ++i) {
        double t = (double)i / MELLO_SAMPLE_SOURCE_RATE, env = exp(-1.5 * t);
        tone[i] =
            (int16_t)(26000.0 * env *
                      (sin(6.2831853 * 196.0 * t) + 0.4 * sin(6.2831853 * 588.0 * t) * exp(-6.0 * t)) / 1.4);
    }
    len = make_record(rec, tone, n, 3u, "PLUCK");
    CHECK(len <= MELLO_SAMPLE_FLASH_SLOT_SIZE, "record %u B over a slot", len);

    /* 1. not while playing */
    n_acks = 0u;
    CHECK(
        mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_START, 3u, false),
        "START not taken as ours");
    CHECK(last_ack_ok() == 0, "START while running was not refused");
    CHECK(!mello_sample_transfer_active(&x), "a refused START left the transfer active");

    /* 2. the whole way: START, the digits, END; the slot plays the tone */
    n_acks = 0u;
    e0 = erases;
    CHECK(mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_START, 3u, true),
          "START");
    CHECK(last_ack_ok() == 1, "START not acknowledged");
    CHECK(erases - e0 == 8u, "START erased %u sectors, not the 32 KiB staging area", erases - e0);
    CHECK(mello_sample_transfer_active(&x), "not active after START");
    send_record(&x, rec, len, true, 0xFFFFFFFFu);
    CHECK(x.raw_received == len, "received %u of %u bytes", x.raw_received, len);
    CHECK(mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_END, 0u, true),
          "END");
    CHECK(last_ack_ok() == 1, "END not acknowledged");
    CHECK(bank.meta[3].loaded, "slot 3 not loaded after END");
    CHECK(bank.meta[3].sample_count == n && strcmp(bank.meta[3].name, "PLUCK") == 0 &&
              bank.meta[3].sample_rate == MELLO_SAMPLE_SOURCE_RATE,
          "slot 3 meta: %u samples %s", bank.meta[3].sample_count, bank.meta[3].name);
    snr = snr_db(&bank, 3u, tone, n);
    CHECK(snr > 25.0, "slot 3 plays back at %.1f dB SNR", snr);
    printf("pluck: %u samples, %u B, %u MIDI messages, SNR %.1f dB\n", n, len, (len * 8u / 7u + 1u) / 2u,
           snr);

    /* 3. power cycle: a new store finds it in flash */
    {
        MelloSampleBank bank2;
        MelloSampleStore store2;
        mello_sample_bank_init(&bank2, pool[0], pool[1], pool[2], pool[3], MELLO_SAMPLE_DATA_MAX);
        mello_sample_store_init(&store2, &io, &bank2);
        CHECK(mello_sample_store_load_all(&store2), "load_all after the transfer");
        CHECK(bank2.meta[3].loaded && !bank2.meta[0].loaded && !bank2.meta[1].loaded && !bank2.meta[2].loaded,
              "after a power cycle: slot 3 %d, others %d%d%d", bank2.meta[3].loaded, bank2.meta[0].loaded,
              bank2.meta[1].loaded, bank2.meta[2].loaded);
        snr = snr_db(&bank2, 3u, tone, n);
        CHECK(snr > 25.0, "after a power cycle slot 3 plays at %.1f dB", snr);
    }

    /* 4. a corrupted digit: END says ERROR and the old sample stays */
    for (i = 0u; i < 8000u; ++i)
        sine[i] = (int16_t)(20000.0 * sin(6.2831853 * 440.0 * i / MELLO_SAMPLE_SOURCE_RATE));
    len2 = make_record(rec2, sine, 8000u, 3u, "BAD");
    n_acks = 0u;
    CHECK(
        mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_START, 3u, true) &&
            last_ack_ok() == 1,
        "second START");
    send_record(&x, rec2, len2, true, 900u);
    CHECK(mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_END, 0u, true),
          "END after corruption");
    CHECK(last_ack_ok() == 0, "a corrupted record was accepted");
    CHECK(bank.meta[3].loaded && bank.meta[3].sample_count == n && strcmp(bank.meta[3].name, "PLUCK") == 0,
          "the old sample did not survive a failed transfer");
    snr = snr_db(&bank, 3u, tone, n);
    CHECK(snr > 25.0, "the old sample plays at %.1f dB after a failed transfer", snr);

    /* 5. QUERY and CLEAR */
    n_acks = 0u;
    CHECK(
        mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_QUERY, 3u, true) &&
            last_ack_ok() == 1 && acks[n_acks - 1u][2] == 3u,
        "QUERY 3 should say loaded");
    CHECK(
        mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_QUERY, 0u, true) &&
            last_ack_ok() == 0,
        "QUERY 0 should say empty");
    /* CLEAR only straight after a QUERY of the same slot (F22): a stray CC 113 clears nothing */
    CHECK(mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_CLEAR, 3u, true) &&
              last_ack_ok() == 0 && bank.meta[3].loaded,
          "a CLEAR without a QUERY before it cleared the slot");
    (void)mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_QUERY, 3u, true);
    (void)mello_sample_transfer_midi_in(&x, 0x90u, 60u, 100u, true); /* a note between */
    CHECK(mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_CLEAR, 3u, true) &&
              last_ack_ok() == 0 && bank.meta[3].loaded,
          "a CLEAR after a note (the QUERY not the last message) cleared the slot");
    (void)mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_QUERY, 2u, true);
    CHECK(mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_CLEAR, 3u, true) &&
              last_ack_ok() == 0 && bank.meta[3].loaded,
          "a CLEAR of slot 3 after a QUERY of slot 2 cleared it");
    (void)mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_QUERY, 3u, true);
    CHECK(mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_CLEAR, 3u,
                                        false) &&
              last_ack_ok() == 0 && bank.meta[3].loaded,
          "CLEAR while running was not refused");
    (void)mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_QUERY, 3u, true);
    (void)mello_sample_transfer_midi_in(&x, 0xF8u, 0u, 0u, true); /* a clock byte between does not count */
    CHECK(
        mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_CLEAR, 3u, true) &&
            last_ack_ok() == 1 && !bank.meta[3].loaded,
        "CLEAR 3 straight after QUERY 3");
    CHECK(
        mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_QUERY, 3u, true) &&
            last_ack_ok() == 0,
        "QUERY 3 after CLEAR should say empty");
    /* other channel messages are not ours */
    CHECK(!mello_sample_transfer_midi_in(&x, 0x90u, 60u, 100u, true),
          "a note was taken as a transfer message");
    CHECK(!mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_DATA_STATUS, 1u, 2u, true),
          "data outside a transfer was taken");

    /* 6. the Python tool's record, when the runner could make it: a 440 Hz sine, 1 s, PYTONE, slot 1 */
    {
        FILE *f = fopen("py_record.bin", "rb");
        if (!f) {
            puts(
                "py_record.bin: not made (no python3?); the tool's encoder is not cross-checked in this run");
        } else {
            size_t got = fread(rec2, 1u, sizeof rec2, f);
            fclose(f);
            CHECK(got > MELLO_SAMPLE_RECORD_HEADER + MELLO_SAMPLE_RECORD_FOOTER, "py_record.bin is %zu B",
                  got);
            for (i = 0u; i < MELLO_SAMPLE_SOURCE_RATE; ++i)
                sine[i] = (int16_t)(16383.0 * sin(6.2831853 * 440.0 * i / MELLO_SAMPLE_SOURCE_RATE));
            n_acks = 0u;
            CHECK(mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_START,
                                                1u, true) &&
                      last_ack_ok() == 1,
                  "START for the Python record");
            send_record(&x, rec2, (uint32_t)got, true, 0xFFFFFFFFu);
            CHECK(mello_sample_transfer_midi_in(&x, MELLO_SAMPLE_MIDI_CTRL_STATUS, MELLO_SAMPLE_CTRL_END, 0u,
                                                true) &&
                      last_ack_ok() == 1,
                  "the Python tool's record was not accepted (raw %u, expected %u)", x.raw_received,
                  x.expected_total);
            CHECK(bank.meta[1].loaded && bank.meta[1].sample_count == MELLO_SAMPLE_SOURCE_RATE &&
                      strcmp(bank.meta[1].name, "PYTONE") == 0,
                  "Python record meta: %u samples %s", bank.meta[1].sample_count, bank.meta[1].name);
            snr = snr_db(&bank, 1u, sine, MELLO_SAMPLE_SOURCE_RATE);
            CHECK(snr > 25.0, "the Python tool's sine plays back at %.1f dB SNR", snr);
            printf("py_record.bin: %zu B, PYTONE in slot 1, SNR %.1f dB\n", got, snr);
        }
    }

    puts("PASS: a sample goes in over MIDI, plays, survives a power cycle; a bad digit, a running engine, "
         "CLEAR and "
         "QUERY are answered");
    return 0;
}
