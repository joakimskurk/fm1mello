/* P1 regression (review 2026-10, P1-3): Chord Machine settings were not in the saved project at all.
 * They now have their own section (SEC_CHORD); readers without it skip it. Flash is a RAM model. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mello_storage.h"

#define FLASH_SIZE (MELLO_STORAGE_BANK0_ADDR + 2u * MELLO_STORAGE_BANK_SIZE)
static uint8_t flash[FLASH_SIZE];
static unsigned erases;

static bool ram_read(void *ctx, uint32_t address, void *dst, uint32_t length)
{
    (void)ctx;
    if (address + length > FLASH_SIZE) return false;
    memcpy(dst, flash + address, length);
    return true;
}
static bool ram_erase4k(void *ctx, uint32_t address)
{
    (void)ctx;
    if (address % 4096u || address + 4096u > FLASH_SIZE) return false;
    memset(flash + address, 0xFF, 4096u);
    erases++;
    return true;
}
static bool ram_program(void *ctx, uint32_t address, const void *src, uint32_t length)
{
    const uint8_t *p = src;
    uint32_t i;
    (void)ctx;
    if (address + length > FLASH_SIZE) return false;
    for (i = 0u; i < length; ++i) flash[address + i] &= p[i]; /* NOR: program only clears bits */
    return true;
}
static const MelloStorageIo io = {ram_read, ram_erase4k, ram_program, NULL};

static void boot(MelloStorage *s, MelloEngine *e, MelloStorageLoadResult *r)
{
    MelloProject p;
    mello_project_defaults(&p);
    mello_engine_init(e, &p, 1u);
    mello_storage_init(s, &io, NULL);
    *r = mello_storage_load(s, e);
}

#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            printf("FAIL: " __VA_ARGS__);                                                                    \
            printf("\n");                                                                                    \
            return 1;                                                                                        \
        }                                                                                                    \
    } while (0)

int main(void)
{
#if MELLO_FEATURE_CHORD_MACHINE
    static MelloStorage s;
    static MelloEngine e;
    MelloStorageLoadResult r;
    MelloChordConfig c, want;
    uint8_t i;
    uint32_t t;
    MelloStorageSaveResult w = MELLO_STORAGE_SAVE_IDLE;

    memset(flash, 0xFF, sizeof flash);
    boot(&s, &e, &r);
    (void)mello_storage_poll(&s, &e, 0u, true); /* baseline: the defaults */

    /* Every field away from its default and inside the sanitizer's range. */
    mello_chord_defaults(&c);
    c.on = !c.on;
    c.trigger = MELLO_CHORD_TRIGGER_GRID;
    c.mode = (MelloChordMode)(MELLO_CHORD_MODE_COUNT - 1);
    c.length = 5u;
    c.voices = 3u;
    c.level = 901u;
    c.wave = MELLO_WAVE_SQUARE;
    c.gate_amount = 1111u;
    c.attack_ms = 1234u;
    c.release_ms = 3210u;
    for (i = 0u; i < MELLO_CHORD_MAX_STEPS; ++i) {
        c.steps[i].degree = (uint8_t)(i + 2u);
        c.steps[i].shape = (MelloChordShape)(i % MELLO_CHORD_SHAPE_COUNT);
        c.steps[i].inversion = (uint8_t)(i % 4u);
        c.steps[i].octave = (int8_t)((int)(i % 5u) - 2);
        c.steps[i].spread = (uint8_t)(3u - i % 4u);
        c.steps[i].hold = (uint8_t)(i + 1u);
    }
    c.grid_cells = 0xA5A5u;
    c.grid_mode = MELLO_DOT_SNAKE;
    c.grid_length = 13u;
    c.grid_reset = !c.grid_reset;
    c.grid_follow = !c.grid_follow;
    c.grid_amount = 777u;
    c.pattern_length = 6u;
    for (i = 0u; i < MELLO_CHORD_PATTERN_STEPS; ++i) c.pattern_masks[i] = (uint8_t)(15u - i);
    c.cut = 123u; /* the voice (section version 2) */
    c.res = 456u;
    c.fenv = 789u;
    c.fenv_attack_ms = 33u;
    c.fenv_decay_ms = 2222u;
    c.decay_ms = 1500u;
    c.sustain = 321u;
    c.chorus = 999u;
    c.space = 444u;
    c.strum_ms = 37u;
    c.link = !c.link;
    c.voice_lead = !c.voice_lead;
    mello_engine_chord_set_config(&e, &c);
    want = e.chord;

    for (t = 100u; t <= 30000u && w != MELLO_STORAGE_SAVE_WRITTEN; t += 100u)
        w = mello_storage_poll(&s, &e, t, true);
    CHECK(w == MELLO_STORAGE_SAVE_WRITTEN, "chord edit never saved (last result %d)", (int)w);

    boot(&s, &e, &r);
    CHECK(r == MELLO_STORAGE_LOAD_OK, "reload %d", (int)r);
    c = e.chord;
    CHECK(c.on == want.on && c.trigger == want.trigger && c.mode == want.mode && c.length == want.length &&
              c.voices == want.voices && c.level == want.level && c.wave == want.wave &&
              c.gate_amount == want.gate_amount && c.attack_ms == want.attack_ms &&
              c.release_ms == want.release_ms,
          "chord header fields differ after reload");
    for (i = 0u; i < MELLO_CHORD_MAX_STEPS; ++i)
        CHECK(c.steps[i].degree == want.steps[i].degree && c.steps[i].shape == want.steps[i].shape &&
                  c.steps[i].inversion == want.steps[i].inversion &&
                  c.steps[i].octave == want.steps[i].octave && c.steps[i].spread == want.steps[i].spread &&
                  c.steps[i].hold == want.steps[i].hold,
              "chord step %u differs after reload", (unsigned)i);
    CHECK(c.grid_cells == want.grid_cells && c.grid_mode == want.grid_mode &&
              c.grid_length == want.grid_length && c.grid_reset == want.grid_reset &&
              c.grid_follow == want.grid_follow && c.grid_amount == want.grid_amount &&
              c.pattern_length == want.pattern_length &&
              memcmp(c.pattern_masks, want.pattern_masks, sizeof c.pattern_masks) == 0,
          "chord grid/pattern fields differ after reload");
    CHECK(c.cut == want.cut && c.res == want.res && c.fenv == want.fenv &&
              c.fenv_attack_ms == want.fenv_attack_ms && c.fenv_decay_ms == want.fenv_decay_ms &&
              c.decay_ms == want.decay_ms && c.sustain == want.sustain && c.chorus == want.chorus &&
              c.space == want.space && c.strum_ms == want.strum_ms && c.link == want.link &&
              c.voice_lead == want.voice_lead,
          "chord voice fields differ after reload");
    printf("PASS: Chord Machine settings survive a power cycle\n");
#else
    (void)boot;
    printf("PASS: Chord Machine compiled out; nothing to save\n");
#endif
    return 0;
}
