/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 joakimskurk */
/* FM-1 random numbers: the WL82's random number register, JL_RAND (the AC79 SDK's WL82.h: R64L and R64H, read-only,
 * at the low-speed peripherals' 0x10000 + group 0x3B; its rand32() is R64L, which JieLi's own firmware takes its
 * Bluetooth keys from). Read as it stands: the SDK starts nothing for it. What it holds on an FM-1 is not proven here
 * (no FM-1 at hand; fm1-emu may give 0), so SkurkyMello mixes it with TIMER4 and the time of the first press. */
#pragma once
#include <stdint.h>

#define FM1_RAND_R64L (*(volatile uint32_t *)0x13B00u)
#define FM1_RAND_R64H (*(volatile uint32_t *)0x13B04u)

/* both halves in one word (the high one multiplied out, so neither cancels the other) */
static inline uint32_t fm1_rand32(void) { return FM1_RAND_R64L ^ (FM1_RAND_R64H * 2654435761u); }
