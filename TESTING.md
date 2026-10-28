# TESTING

## Validation philosophy

A green technical metric is necessary but not sufficient for product acceptance. Deterministic tests, target/performance measurements and hands-on audition gates are used where applicable.

The user does not currently have a physical FM-1, so host/emulator/web/target-build validation is primary until hardware arrives.

## Locked baselines

WP-A randomizer remains `LOW < MED < HIGH`, target isolation/WAVE and 4..32 skyline protected. WP-B live-edit, rotary mapping and display integrity remain hard gates. WP-C now adds complete REC/EDIT/contextual-action/navigation parity.

## Final WP-C verified baseline

Implementation/test checkpoint: `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

C5 exact locks:

- UI `f6d26e0d1a57a51b08bf65e878144f5d48bfa248244d97b80c7a0842eb601818`;
- authoritative mapping test `bf688e15845324e1982336830f20c067118d90923f8ba6da0b692c94dac4914d`;
- retained C4 context contract `b5479562eb9a4aca5817663e835c04151e0c3337e4c9b131cb4c675daeabddfd`.

Authoritative mapping coverage:

- Chord enabled: `16 pages`, `31 active` B0-B7 slots, `97 inactive` slots;
- Chord compiled out: `15 pages`, `29 active`, `91 inactive`;
- rendered hint strings match descriptor labels exactly;
- inactive slots are inert;
- every advertised B5 opens RANDOM;
- STEP `SEL+black` retains manual-note priority;
- handler dispatch contains no duplicate page-mapping switch;
- handler and renderer both consume the same `MelloContextAction` descriptor.

Final workflow matrix:

- authoritative actions `40703570244`: PASS;
- reconstruction `40703570315`: PASS;
- context grammar `40703570196`: PASS;
- REC grammar `40703570212`: PASS;
- EDIT grammar `40703570277`: PASS;
- control-action contract `40703570204`: PASS;
- control audit `40703570330`: PASS;
- rotary parity `40703570466`: PASS;
- display integrity `40703570254`: PASS;
- live-edit `40703570316`: PASS;
- full target `40703571071`: PASS.

The reusable target workflow's separate `Tests` step is skipped because no separate `test:` input is supplied; `bash ci/build_target.sh` executes the required test matrix inside Build.

## Final WP-C target/performance evidence

Run `40703571071`:

- source reconstruction and all C2/C3/C4/C5 output locks: PASS;
- universal visual/control regressions: PASS;
- source-parity CLEAR/TAP/RESET semantics: PASS;
- Reverb V2 IR: dense `42497`, corr² `0.00075`, jumpE `2.279`, predelay delta `5292`;
- 300 BPM ratchet/chord/sub/sample timing stress: PASS;
- Chord technical/compile-out acceptance: PASS;
- WP-A enabled: `LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4/0`: PASS;
- WP-A Chord-off: `LOW=180984 < MED=484357 < HIGH=1038671`, redraw `4/0`: PASS;
- live-edit host: energy `433436686798`, ticks `159600`, applies `2470`, SUB builds `67`, reverb reconfigs `135`: PASS;
- emulator panel/audio: `189440` frames, peak `3530`, `148408` nonzero samples: PASS;
- browser/WASM 100 MHz: steady realtime `1.700x`, steady render p95 `5.040 ms`, p99 `7.162 ms`, max `17.503 ms`: PASS;
- SEQ/HOME 80 MHz: steady realtime `1.718x`, p95 `4.978 ms`, p99 `7.043 ms`, max `13.041 ms`: PASS;
- Chord + Reverb V2 80 MHz: steady realtime `1.629x`, p95 `5.175 ms`, p99 `7.625 ms`, max `11.090 ms`: PASS;
- live-edit encoder stress 80 MHz: steady realtime `1.661x`, p95 `5.327 ms`, p99 `7.600 ms`, max `21.838 ms`, tail `88174`, peak `3847`: PASS;
- Chromium keyboard/pointer including PLAY start-stop: PASS;
- image `190060 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `skurkymello-0.1-dev.fwsc`, `611545 B`;
- package SHA-256 `755a098ecbe66a6f8f7697a5787cb240e50a69df2520dc2f10b690f0056b8da1`;
- provenance attestation: PASS.

Hosted emulator timing varies with runner load; acceptance uses established thresholds, not fastest historical values.

## Active WP-D testing direction

Before changing timing behavior, create deterministic coverage for current main-sequence reset/free-running behavior, stored/default value, transport reset boundary and rendered timing language. The approved target is main sequence `1 / 2 / 4 / 8 / 16 BAR / FREE` with `1 BAR` default while external/MIDI sync, Loop Cycle `SYNC/FREE` and Slot/Chain hold remain separate.

WP-D must preserve the full WP-A/WP-B/WP-C/audio/emulator/browser/80 MHz/Chromium/target matrix and retain >=16 KiB RAM and pool headroom.

## Retained hardware/perceptual notes

Physical FM-1 live-edit reproduction remains a later hands-on regression when hardware becomes available. Reverb V2 remains technical regression evidence; product-quality Reverb V3 belongs to WP-E.

## Reporting rule

At each closeout record exact checkpoint SHA, workflow run IDs, measured gates and any hardware-only validation that remains deferred.
