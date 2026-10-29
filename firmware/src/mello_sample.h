#ifndef MELLO_SAMPLE_H
#define MELLO_SAMPLE_H

#include <stdbool.h>
#include <stdint.h>
#include "mello_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MELLO_SAMPLE_SOURCE_RATE 22050u
#define MELLO_SAMPLE_BLOCK_SAMPLES 256u
#define MELLO_SAMPLE_BLOCK_BYTES 132u
#define MELLO_SAMPLE_RECORD_HEADER 64u
#define MELLO_SAMPLE_RECORD_FOOTER 8u
#define MELLO_SAMPLE_FLASH_SLOT_SIZE 0x8000u
#define MELLO_SAMPLE_FLASH_STAGING_ADDR 0x000B8000u
#define MELLO_SAMPLE_FLASH_BASE 0x000C0000u
#define MELLO_SAMPLE_FLASH_END 0x000E0000u
#define MELLO_SAMPLE_DATA_MAX (MELLO_SAMPLE_FLASH_SLOT_SIZE - MELLO_SAMPLE_RECORD_HEADER - MELLO_SAMPLE_RECORD_FOOTER)
#define MELLO_SAMPLE_MAX_BLOCKS (MELLO_SAMPLE_DATA_MAX / MELLO_SAMPLE_BLOCK_BYTES)
#define MELLO_SAMPLE_MAX_SAMPLES (MELLO_SAMPLE_MAX_BLOCKS * MELLO_SAMPLE_BLOCK_SAMPLES)
#define MELLO_SAMPLE_NAME_CHARS 20u

#define MELLO_SAMPLE_MIDI_DATA_STATUS 0xAFu
#define MELLO_SAMPLE_MIDI_CTRL_STATUS 0xBFu
#define MELLO_SAMPLE_CTRL_START 110u
#define MELLO_SAMPLE_CTRL_END 111u
#define MELLO_SAMPLE_CTRL_ABORT 112u
#define MELLO_SAMPLE_CTRL_CLEAR 113u
#define MELLO_SAMPLE_CTRL_QUERY 114u
#define MELLO_SAMPLE_CTRL_ACK 120u
#define MELLO_SAMPLE_CTRL_ERROR 121u

typedef struct {
    bool loaded;
    char name[MELLO_SAMPLE_NAME_CHARS + 1u];
    uint16_t sample_rate;
    uint32_t sample_count;
    uint16_t block_count;
    uint32_t data_bytes;
    uint32_t data_crc;
} MelloSampleSlotMeta;

typedef struct {
    uint8_t *data[MELLO_SAMPLE_SLOT_COUNT];
    uint32_t capacity[MELLO_SAMPLE_SLOT_COUNT];
    MelloSampleSlotMeta meta[MELLO_SAMPLE_SLOT_COUNT];
} MelloSampleBank;

typedef bool (*MelloSampleReadFn)(void *ctx, uint32_t address, void *dst, uint32_t length);
typedef bool (*MelloSampleErase4kFn)(void *ctx, uint32_t address);
typedef bool (*MelloSampleProgramFn)(void *ctx, uint32_t address, const void *src, uint32_t length);
typedef void (*MelloSampleIdleFn)(void *ctx);
typedef void (*MelloSampleMidiSendFn)(void *ctx, uint8_t status, uint8_t d1, uint8_t d2);

typedef struct {
    MelloSampleReadFn read;
    MelloSampleErase4kFn erase4k;
    MelloSampleProgramFn program;
    MelloSampleIdleFn idle;
    void *ctx;
} MelloSampleIo;

typedef struct {
    MelloSampleIo io;
    MelloSampleBank *bank;
    uint8_t scratch[256];
} MelloSampleStore;

typedef struct {
    MelloSampleStore *store;
    MelloSampleMidiSendFn midi_send;
    void *midi_ctx;
    bool active;
    bool failed;
    uint8_t slot;
    uint8_t wire7[8];
    uint8_t wire7_used;
    uint32_t raw_received;
    uint32_t expected_total;
    uint32_t flash_written;
    uint16_t buf_used;
    uint8_t buf[256];
} MelloSampleTransfer;

void mello_sample_bank_init(MelloSampleBank *bank, uint8_t *slot0, uint8_t *slot1, uint8_t *slot2, uint8_t *slot3, uint32_t capacity_each);
void mello_sample_store_init(MelloSampleStore *store, const MelloSampleIo *io, MelloSampleBank *bank);
bool mello_sample_store_load_all(MelloSampleStore *store);
bool mello_sample_store_load_slot(MelloSampleStore *store, uint8_t slot);
bool mello_sample_store_clear_slot(MelloSampleStore *store, uint8_t slot);
bool mello_sample_store_recover_staging(MelloSampleStore *store);

bool mello_sample_decode_block(const MelloSampleBank *bank, uint8_t slot, uint16_t block, int16_t out[MELLO_SAMPLE_BLOCK_SAMPLES], uint16_t *valid_samples);

void mello_sample_transfer_init(MelloSampleTransfer *xfer, MelloSampleStore *store, MelloSampleMidiSendFn send, void *midi_ctx);
bool mello_sample_transfer_midi_in(MelloSampleTransfer *xfer, uint8_t status, uint8_t d1, uint8_t d2, bool safe_to_write);
void mello_sample_transfer_abort(MelloSampleTransfer *xfer);
bool mello_sample_transfer_active(const MelloSampleTransfer *xfer);

#ifdef __cplusplus
}
#endif
#endif
