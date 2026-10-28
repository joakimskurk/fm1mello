# TESTING

## Validation philosophy

A green technical metric is necessary but not sufficient for product acceptance. Deterministic tests, target/performance measurements and hands-on audition gates are used where applicable.

The user does not currently have a physical FM-1, so host/emulator/web/target-build validation is primary until hardware arrives.

## Locked baselines

WP-A randomizer remains `LOW < MED < HIGH`, target isolation/WAVE and 4..32 skyline protected. WP-B live-edit, rotary mapping and display integrity remain hard gates. WP-C locks REC/EDIT/contextual-action/navigation parity. WP-D locks the musical bar timing model and timing-language separation.

## Final WP-D verified baseline

Implementation/test checkpoint: `0cd810b5deb90087d60b181682de7e9ff52682cd`.

Permanent D1 baseline/audit:

- `tools/wp_d_timing_baseline.c`;
- `docs/WP_D_TIMING_AUDIT.md`.

D2/D3 deterministic model:

- choices `FREE / 1 / 2 / 4 / 8 / 16 BAR`;
- default **2 BAR**;
- selected BAR count is invariant across sequence `DIV 1/2/4`;
- `FREE` preserves natural sequence wrap;
- malformed noncanonical stored timing values remain safe;
- external/MIDI sync, Loop Cycle `SYNC/FREE` and Slot/Chain HOLD stay independent;
- rendered SEQ timing language uses `RESET` + BAR/FREE, not generic SYNC.

Exact generated locks:

- `mello/mello_engine.c`: `daa00773838efe5d511c81afcdc84ba3e8bef42c6e08d368de5843c643f7670c`;
- `mello/mello_engine.h`: `769ea7e8b7a1be51d38bbd48e5eb09e39114dd79ca363dcb70b235e1483cdab8`;
- `mello/mello_ui.c`: `08bd19d7a2f112de318d50516c5a27b3ed2324d0c82bd50a087caadd0b7ecd55`.

## Final WP-D workflow matrix

Fast workflows on `0cd810b...`:

- display integrity `40763919218`: PASS;
- authoritative actions `40763917759`: PASS;
- WP-D timing model `40763917598`: PASS;
- live-edit `40763917508`: PASS;
- reconstructed source `40763917460`: PASS;
- contextual rail/MOD `40763917641`: PASS;
- rotary parity `40763917555`: PASS;
- control-action contract `40763917660`: PASS;
- REC grammar `40763917588`: PASS;
- EDIT grammar `40763917591`: PASS;
- control grammar audit `40763917470`: PASS;
- full target `40763918161`: PASS.

The reusable target workflow's separate `Tests` step is skipped because no separate `test:` input is supplied; `bash ci/build_target.sh` executes the required test matrix inside Build.

## Final WP-D target/performance evidence

Run `40763918161`:

- source reconstruction and all retained WP-C + WP-D D2/D3 output locks: PASS;
- WP-D canonical choices/default/formatters/timing-concept separation: PASS;
- Reverb V2 IR: dense `42497`, corr² `0.00075`, jumpE `2.279`, predelay delta `5292`: PASS;
- 300 BPM ratchet/chord/sub/sample timing stress: PASS;
- Chord ARP: ons `12`, offs `12`, distinct notes `3`, max one attack/pulse: PASS;
- Chord + Reverb V2 tail: dry `0/0`, wet `2025330/8170`: PASS;
- Chord compile-out: PASS;
- WP-A enabled: `LOW=194824 < MED=516917 < HIGH=1082367`, redraw fills/texts `4/0`: PASS;
- WP-A Chord-off: `LOW=180984 < MED=484357 < HIGH=1038671`, redraw fills/texts `4/0`: PASS;
- live-edit host: energy `433436686798`, ticks `159600`, applies `2470`, SUB builds `67`, reverb reconfigs `135`: PASS;
- emulator panel/audio: `189440` frames, peak `3530`, `148408` nonzero samples: PASS;
- browser/WASM 100 MHz: steady realtime `1.520x`, p95 `5.268 ms`, p99 `7.467 ms`, max `60.274 ms`: PASS;
- SEQ/HOME 80 MHz: steady realtime `1.699x`, p95 `5.143 ms`, p99 `7.339 ms`, max `13.764 ms`: PASS;
- Chord + Reverb V2 80 MHz: steady realtime `1.595x`, p95 `5.485 ms`, p99 `7.482 ms`, max `10.171 ms`: PASS;
- physical-encoder live-edit stress 80 MHz: steady realtime `1.622x`, p95 `5.417 ms`, p99 `7.793 ms`, max `25.064 ms`, tail nonzero `88174`, tail peak `3847`: PASS;
- Chromium keyboard/pointer including PLAY start-stop: PASS;
- image `190156 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `skurkymello-0.1-dev.fwsc`, `611545 B`;
- package SHA-256 `bee43ce894a5a26472270b0890f1d64e400bfb68246980f5fe285179a4225b7f`;
- provenance attestation: PASS.

Hosted emulator timing varies with runner load; acceptance uses established thresholds, not fastest historical values. Existing compiler warnings in DOT enum comparisons/Rust unused assignments/wasm casts predate WP-D and did not fail the build.

## Active WP-E testing direction

Start WP-E with an E1 audit/baseline before changing sound. Capture current distortion bounds/transfer behavior, Reverb V2 reference behavior, ratchet audio/timing parity, live parameter smoothing, relevant control mapping and CPU/memory cost.

WP-E acceptance must combine deterministic technical gates with perceptual A/B where sound quality is the product criterion, especially Reverb V3. All WP-A/WP-B/WP-C/WP-D gates remain mandatory, with emulator/browser/80 MHz/Chromium/target coverage and >=16 KiB RAM/pool headroom.

## Retained hardware/perceptual notes

Physical FM-1 timing/live-edit reproduction remains a later hands-on regression when hardware becomes available. Reverb V3 perceptual acceptance can be prepared with rendered A/B evidence before hardware but must not be replaced by technical metrics alone.

## Reporting rule

At each closeout record exact checkpoint SHA, workflow run IDs, measured gates and any hardware-only validation that remains deferred.
