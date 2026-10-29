# WP-D Timing Audit

D1 baseline captured: 2026-10-28.

This document records the timing behavior reconstructed at branch checkpoint `dc29d8fafbc4521ee9f11c1434737a107d24959f` before WP-D changes timing semantics. It is a behavior baseline, not the target model.

## D1 conclusion

The current firmware has four different timing concepts that must remain distinct:

1. **Main-sequence reset** — `MelloProject.master_len` controls the main arranger reset boundary in processed pulse entries.
2. **External/MIDI sync** — `MelloTransport.sync_mode` chooses OFF / SLAVE / MASTER clock transport behavior. It does not own the main reset length.
3. **Loop Cycle sync** — `MelloLoopCycleConfig.sync` controls Loop Cycle phase behavior at a main-sequence wrap. It does not choose the main reset boundary.
4. **Slot/Chain hold** — Slot Seq and harmonic-chain hold counters advance on their own counters and are not external sync or main-sequence reset controls.

The approved D2 target therefore replaces only the main-sequence reset representation with `1 / 2 / 4 / 8 / 16 BAR / FREE`, default **2 BAR**. External sync, Loop Cycle `SYNC / FREE`, and hold semantics remain separate.

## Current main-sequence behavior

`MelloProject` currently stores:

- `seq_len` — number of source steps;
- `clock_div` — processed-pulse duration choice `1 / 2 / 4`;
- `master_len` — `uint16_t`, where `0` means no forced reset and a nonzero value is a processed-pulse reset length.

`mello_engine_rebuild_sequence()` calculates the natural runtime pulse count from segments/repeats and sets `loop_pulse_length = master_len` when `master_len > 0`; otherwise it uses the natural runtime length.

Every successful `mello_engine_advance_pulse()` increments `absolute_pulse`. A main wrap occurs when `absolute_pulse % loop_pulse_length == 0`. The wrap increments `loop_count` and emits `MELLO_EVENT_PATTERN_WRAP`.

When a forced `master_len` wrap occurs, main segment traversal is reset to segment 0 / pulse 0. With `master_len == 0`, normal segment traversal naturally returns to the start instead.

### Current UI representation

The SEQ parameter page currently exposes:

- `LEN` = `seq_len`;
- `RESET` = numeric `master_len`;
- `DIV` = `clock_div`;
- `SPAN` = pitch span.

The selectable reset values are currently `0 / 16 / 32 / 64 / 128 / 256` processed pulses. `0` is displayed numerically rather than as `FREE`.

This is the legacy representation D2 will replace.

## Defaults and storage

`mello_project_defaults()` currently sets `seq_len = 16`, `bpm = 80`, `clock_div = 1` and `master_len = 0`.

`MelloStoredProject` persists `master_len` as two bytes, `master_len_lo` and `master_len_hi`. Encode/decode round-trips the value directly. This gives D2 an existing persistent field that can be migrated without conflating transport sync state with project timing.

Loop Cycle configuration is stored separately from the main project timing representation. Slot Seq configuration/runtime is likewise a separate engine concept rather than part of `master_len`.

## Transport event path

Internal transport:

`mello_transport_poll()` -> `poll_internal_transport()` -> `process_one_pulse()` -> `mello_engine_advance_pulse()`.

External MIDI slave transport:

- `MELLO_SYNC_SLAVE` disables internal polling;
- MIDI Start/Continue starts the engine and emits the first sequence pulse;
- MIDI Clock accumulates to the current `clock_div` requirement before `process_one_pulse()` is called;
- changing sync mode does not rewrite `master_len`.

MIDI MASTER emits clock externally while the engine still advances from the internal pulse scheduler. Transport clock source and main reset boundary are therefore independent layers.

## Loop Cycle separation

Loop Cycle has its own `sync` boolean and per-step `hold` counter. When active, `loop_cycle_tick` advances once per processed main pulse. At a main wrap, Loop Cycle `SYNC` may reset its phase counter when the current hold fits within the main loop boundary. Changing Loop Cycle `SYNC/FREE` does not change `loop_pulse_length`, `master_len`, or external transport mode.

## Slot/Chain hold separation

Slot Seq steps use their own `hold` values and `slot_seq_hold` runtime counter. These advance independently of the main reset setting. The harmonic chain similarly uses per-chain-step hold state. Neither is represented by transport sync or `master_len`.

## D1 deterministic contract

`tools/wp_d_timing_baseline.c` locks the reconstructed pre-D2 behavior:

- default `master_len == 0`;
- storage encode/decode round-trip of legacy `master_len`;
- natural loop versus forced main reset;
- Loop Cycle `SYNC/FREE` affects Loop Cycle phase without changing the main reset;
- Slot Seq HOLD advances independently;
- MIDI SLAVE clock advances the same engine and preserves the main reset setting.

The WP-D audit workflow also locks the current rendered legacy timing labels before D2 changes them intentionally.

## Reconstructed-source fingerprints

From audit run `40737976686` / artifact `wp-d-reconstructed-timing-source`:

- `mello_engine.h` — `5a431221418d3f2c90d6094303a6eab03f13757d211284381251f004d29fa2f8`
- `mello_engine.c` — `7c9b7428920f787225c383ae4e6ba08a8e1ac5a2a95064b3d7d14ea0f8d3c64d`
- `mello_transport.h` — `1faae77ef2ddc524ea2dc6e3184d41368b0b4d637eda3e19ddf82a836b44eb4d`
- `mello_transport.c` — `d49ccace38d402dc8ffd678934c20c61f8c9fd6988a53780c03ffd0c2d80e08b`
- `mello_ui.h` — `0b712e7c5641c989f98a85af97f5dad5ee290897f33340f0e40d9cad2005159d`
- `mello_ui.c` — `f6d26e0d1a57a51b08bf65e878144f5d48bfa248244d97b80c7a0842eb601818`

## D2 target locked by product owner

- options: `1 / 2 / 4 / 8 / 16 BAR / FREE`;
- default: **2 BAR**;
- bar reset must remain musically bar-based when pulse division changes;
- `FREE` retains natural/polymetric sequence behavior;
- do not reuse generic `SYNC` wording for this control.
