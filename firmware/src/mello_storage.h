#ifndef MELLO_STORAGE_H
#define MELLO_STORAGE_H

#include <stdbool.h>
#include <stdint.h>
#include "mello_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MELLO_STORAGE_FORMAT_VERSION 3u
#define MELLO_STORAGE_MAGIC          0x374C454Du /* little-endian bytes: M E L 7 */
#define MELLO_STORAGE_COMMIT_MAGIC   0x21374B4Fu /* little-endian bytes: O K 7 ! */
#define MELLO_STORAGE_HEADER_SIZE    24u
#define MELLO_STORAGE_FOOTER_SIZE    8u
#define MELLO_STORAGE_SECTOR_SIZE    0x1000u
#define MELLO_STORAGE_BANK_SIZE      0x10000u /* 64 KiB: 64 full slots with their grit and MOD are 42.4 KiB */
#define MELLO_STORAGE_BANK0_ADDR     0x00098000u
#define MELLO_STORAGE_BANK1_ADDR     (MELLO_STORAGE_BANK0_ADDR + MELLO_STORAGE_BANK_SIZE)
/* Up to 0.5-f21 the banks were 40 KiB and the second began here; a record there is still read at boot (and the
 * first save then goes to bank 1, which does not overlap it), so an upgrade keeps the last project. */
#define MELLO_STORAGE_LEGACY_BANK1_ADDR        0x000A2000u
#define MELLO_STORAGE_AUTOSAVE_DEBOUNCE_MS     2000u
#define MELLO_STORAGE_AUTOSAVE_MIN_INTERVAL_MS 20000u
#define MELLO_STORAGE_CHANGE_CHECK_MS          250u /* how often poll compares the project with flash */

/* Target note: these two 64 KiB banks occupy 0x98000..0xB7FFF, up to the sample staging area. Dinghy 0.3
 * allows reads anywhere but its current FL_RANGE_OK must be deliberately widened
 * before erase/program is legal on hardware. Do not apply that target patch until
 * the exact upstream checkout/flash map has passed its unchanged baseline gate. */

typedef bool (*MelloStorageReadFn)(void *ctx, uint32_t address, void *dst, uint32_t length);
typedef bool (*MelloStorageErase4kFn)(void *ctx, uint32_t address);
typedef bool (*MelloStorageProgramFn)(void *ctx, uint32_t address, const void *src, uint32_t length);

typedef struct {
    MelloStorageReadFn read;
    MelloStorageErase4kFn erase4k;
    MelloStorageProgramFn program;
    void *ctx;
} MelloStorageIo;

typedef struct {
    uint32_t bank_addr[2];
    uint32_t bank_size;
    uint32_t sector_size;
    uint32_t autosave_debounce_ms;
    uint32_t autosave_min_interval_ms;
} MelloStorageConfig;

typedef enum {
    MELLO_STORAGE_LOAD_NONE = 0,
    MELLO_STORAGE_LOAD_OK,
    MELLO_STORAGE_LOAD_FALLBACK,
    MELLO_STORAGE_LOAD_IO_ERROR,
    MELLO_STORAGE_LOAD_UNSUPPORTED,
    MELLO_STORAGE_LOAD_CORRUPT
} MelloStorageLoadResult;

typedef enum {
    MELLO_STORAGE_SAVE_IDLE = 0,
    MELLO_STORAGE_SAVE_DEFERRED,
    MELLO_STORAGE_SAVE_WRITTEN,
    MELLO_STORAGE_SAVE_UNCHANGED,
    MELLO_STORAGE_SAVE_IO_ERROR,
    MELLO_STORAGE_SAVE_TOO_LARGE,
    MELLO_STORAGE_SAVE_PROTECTED /* the bank holds a newer firmware's project: not overwritten */
} MelloStorageSaveResult;

typedef struct {
    MelloStorageIo io;
    MelloStorageConfig config;
    int8_t active_bank;
    bool active_legacy; /* the record read came from the 40 KiB layout's second bank (an upgrade) */
    uint32_t revision;
    uint32_t active_payload_size;
    uint32_t active_payload_crc;
    bool dirty;
    bool manual_pending;
    uint32_t dirty_since_ms;
    uint32_t last_save_ms;
    MelloStorageLoadResult last_load_result;
    MelloStorageSaveResult last_save_result;
    /* What flash holds (or, with nothing usable there, the state right after load): poll marks the
     * storage dirty by itself when the project's payload stops matching it, so no edit can be missed. */
    bool baseline_valid;
    uint32_t baseline_size;
    uint32_t baseline_crc;
    uint32_t last_check_ms;
} MelloStorage;

void mello_storage_config_defaults(MelloStorageConfig *config);
void mello_storage_init(MelloStorage *storage, const MelloStorageIo *io, const MelloStorageConfig *config);

/* Boot load: scans both A/B banks, validates header+commit+payload CRC, and loads
 * the newest structurally decodable record. If the newest record is structurally
 * invalid despite a valid CRC, the older valid bank is tried as a fallback. */
MelloStorageLoadResult mello_storage_load(MelloStorage *storage, MelloEngine *engine);

/* Direct save is intended for tests and explicit safe/silent app points. It never
 * erases the currently-active valid bank; a new record is committed in the other bank. */
MelloStorageSaveResult mello_storage_save_now(MelloStorage *storage, const MelloEngine *engine,
                                              uint32_t now_ms);

/* Deferred save policy: mark_dirty is for edits/autosave, request_save is an explicit
 * user SAVE. poll writes only when safe_to_write is true. Explicit SAVE bypasses the
 * debounce/min-interval but never the safe/silent requirement. poll also compares the
 * project with the baseline every MELLO_STORAGE_CHANGE_CHECK_MS and marks it dirty on
 * any difference, so mark_dirty only makes an autosave start sooner. */
void mello_storage_mark_dirty(MelloStorage *storage, uint32_t now_ms);
void mello_storage_request_save(MelloStorage *storage, uint32_t now_ms);
MelloStorageSaveResult mello_storage_poll(MelloStorage *storage, const MelloEngine *engine, uint32_t now_ms,
                                          bool safe_to_write);

/* Deterministic helpers used by host tests and diagnostics. */
bool mello_storage_measure_payload(const MelloEngine *engine, uint32_t *size_out, uint32_t *crc_out);
uint32_t mello_storage_crc32(const void *data, uint32_t length);

#ifdef __cplusplus
}
#endif
#endif
