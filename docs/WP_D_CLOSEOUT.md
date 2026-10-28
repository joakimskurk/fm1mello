# WP-D Timing model closeout

Completed: 2026-10-28

Final implementation/test checkpoint: `0cd810b5deb90087d60b181682de7e9ff52682cd`.

WP-D is complete. This document records the implementation and validation evidence used to hand off to WP-E Audio character.

## Delivered

### D1 - audit and deterministic baseline

The pre-change timing model was mapped before modification:

- main reset used 16-bit `master_len`;
- canonical legacy values were `0 / 16 / 32 / 64 / 128 / 256`;
- `0` meant natural/free sequence wrap;
- nonzero legacy values were processed-pulse reset lengths;
- external/MIDI transport sync, Loop Cycle sync and Slot/Chain HOLD already used separate state/event paths.

Permanent baseline: `tools/wp_d_timing_baseline.c` and `docs/WP_D_TIMING_AUDIT.md`.

### D2 - musical main-sequence timing

The main sequence now exposes `FREE / 1 / 2 / 4 / 8 / 16 BAR`, default **2 BAR**.

The existing 16-bit `master_len` storage field/layout is retained. Runtime interpretation is musical, so changing sequence `DIV` does not silently change the selected number of bars. Noncanonical stored values are converted safely and cannot create a zero-length forced-reset interval.

Exact generated D2 locks:

- `mello/mello_engine.c`: `daa00773838efe5d511c81afcdc84ba3e8bef42c6e08d368de5843c643f7670c`;
- `mello/mello_engine.h`: `769ea7e8b7a1be51d38bbd48e5eb09e39114dd79ca363dcb70b235e1483cdab8`.

### D3 - timing language

SEQ exposes the main timing control as `RESET` with `FREE`, `1 BAR`, `2 BAR`, `4 BAR`, `8 BAR`, `16 BAR`.

The UI and runtime continue to distinguish:

- main-sequence bar reset;
- external/MIDI sync;
- Loop Cycle `SYNC/FREE`;
- Slot/Chain HOLD length.

Exact generated D3 UI lock:

- `mello/mello_ui.c`: `08bd19d7a2f112de318d50516c5a27b3ed2324d0c82bd50a087caadd0b7ecd55`.

## D4 final workflow matrix

Fast workflows on `0cd810b...`:

- display integrity `40763919218`: PASS;
- authoritative action mapping `40763917759`: PASS;
- WP-D timing model `40763917598`: PASS;
- live-edit `40763917508`: PASS;
- reconstructed source `40763917460`: PASS;
- contextual rail/MOD `40763917641`: PASS;
- rotary parity `40763917555`: PASS;
- control-action contract `40763917660`: PASS;
- REC grammar `40763917588`: PASS;
- EDIT grammar `40763917591`: PASS;
- control grammar audit `40763917470`: PASS.

Full target run `40763918161`: **PASS**.

## Full target evidence

Retained feature/audio gates:

- WP-A enabled randomizer `LOW=194824 < MED=516917 < HIGH=1082367`, redraw fills/texts `4/0`: PASS;
- WP-A Chord-off `LOW=180984 < MED=484357 < HIGH=1038671`, redraw `4/0`: PASS;
- Reverb V2 IR dense `42497`, corr² `0.00075`, jumpE `2.279`, predelay delta `5292`: PASS;
- 300 BPM ratchet/chord/sub/sample timing stress: PASS;
- Chord ARP ons `12`, offs `12`, distinct notes `3`, max one attack/pulse: PASS;
- Chord + Reverb V2 tail dry `0/0`, wet `2025330/8170`: PASS;
- Chord compile-out: PASS;
- live-edit host energy `433436686798`, ticks `159600`, applies `2470`, SUB builds `67`, reverb reconfigs `135`: PASS.

Emulator/browser/performance:

- emulator panel/audio `189440` frames, peak `3530`, `148408` nonzero samples: PASS;
- browser/WASM 100 MHz steady realtime `1.520x`, p95 `5.268 ms`, p99 `7.467 ms`, max `60.274 ms`: PASS;
- SEQ/HOME 80 MHz steady realtime `1.699x`, p95 `5.143 ms`, p99 `7.339 ms`, max `13.764 ms`: PASS;
- Chord + Reverb V2 80 MHz steady realtime `1.595x`, p95 `5.485 ms`, p99 `7.482 ms`, max `10.171 ms`: PASS;
- physical-encoder live-edit stress 80 MHz steady realtime `1.622x`, p95 `5.417 ms`, p99 `7.793 ms`, max `25.064 ms`, tail nonzero `88174`, tail peak `3847`: PASS;
- real Chromium keyboard/pointer routes including PLAY start-stop: PASS.

Target resources/package:

- image `190156 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `skurkymello-0.1-dev.fwsc`, `611545 B`;
- package SHA-256 `bee43ce894a5a26472270b0890f1d64e400bfb68246980f5fe285179a4225b7f`;
- provenance attestation: PASS.

The >=16 KiB RAM and pool headroom requirement remains satisfied.

Existing compiler warnings around DOT enum comparisons, Rust unused assignments/dead code and wasm casts predate WP-D and did not fail the build.

## Deferred hardware validation

No physical FM-1 is available yet. Hands-on timing/live-edit reproduction remains deferred until hardware exists. This does not block WP-D because host, emulator, browser, 80 MHz and target evidence is green.

## Handoff

**WP-E Audio character is ACTIVE NEXT.**

Begin WP-E with E1 audit/baseline only. Do not begin distortion/Reverb V3 implementation until current audio paths, controls, realtime update behavior, CPU/memory budgets and deterministic/perceptual acceptance criteria are documented.

Do not start WP-F or later packages early.
