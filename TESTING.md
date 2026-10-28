# TESTING

## Validation philosophy

A green technical metric is necessary but not sufficient for product acceptance. Deterministic tests, target/performance measurements and hands-on audition gates are used where applicable.

The user does not currently have a physical FM-1, so host/emulator/web/target-build validation is the primary path until hardware arrives.

## Locked baselines

### WP-A

- canonical randomizer distance `LOW=194824 < MED=516917 < HIGH=1082367`;
- RANDOM dedicated redraw `4 fills / 0 texts`;
- WAVE isolation and 4..32 skyline: PASS.

### WP-B

Final checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`. Reconstruction `40604213816`, live-edit `40604213682`, rotary `40604213768`, display `40604213707`, target `40604214660`: PASS.

### WP-C1/C2/C3

- C1 audit `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.
- C2 REC cleanup `f6d1f81660dab8db1de6da4e4650d01482d1ced3`; UI lock `3da137c8faa5800bcfddd8291176e77718bc320d890f20f9f9d0c9cba4aaffb0`.
- C3 EDIT cleanup `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`; UI lock `c811c6ee8e5d6422a479452c65e0c4720d93d0b979cba3b4bcd1281f95a162da`.

## WP-C4 verified baseline

Implementation/test checkpoint: `84d42c9fa21b2a91db5fa1aff2340c6ef3289a60`.

Exact C4 locks include:

- UI `a3829d2fdc354154021727838b4b17cf8420832f88565a2b2f290b17a06002c5`;
- rotary parity `63cb37c6ed51df12b3ebef8510ee0decf3fab41772a37bfcbd1294adde2c1749`;
- UI control parity `7d21290a5a29ba94d7f751e361762e3c219df48b0783d6b69d414b4c327d0a1c`;
- encoder baseline `f83eedd2a2130a978fc30f02896f0316ab274b18978fe97b55e409f4bfb4fdb8`;
- control grammar smoke `6fd837ae7ff7eabaf8e4d7e3e04d7f6bac8d3dfa18e3988d16f3537d942f85c9`;
- visual smoke `2659928170069c1e52ed4e41ff95e21cfd03d80d540a958b3f43e7b1402bffb1`.

C4 deterministic contract proves with Chord enabled/compiled out:

- RANDOM OCT no longer edits mask; B6=NONE and B7=ALL;
- GLOBAL hidden OCT reset is absent; B3 requires confirmation and resets correctly;
- LOOP layer-3 K4 is inert; B2 performs reset;
- every active contextual action has matching rendered guidance; inactive rail slots are inert;
- Audio LFO, DOT and EVOLVE cycle through MOD navigation without Turing state side effects;
- FX opens AUDIO FX directly;
- C2/C3 REC/EDIT rules and STEP `SEL+black` manual-note priority remain green.

C4 workflow matrix:

- reconstruction `40701004129`: PASS;
- contextual rail/MOD `40701004213`: PASS;
- REC grammar `40701004034`: PASS;
- EDIT grammar `40701004030`: PASS;
- control-action contract `40701004015`: PASS;
- control audit `40701004022`: PASS;
- rotary parity `40701004133`: PASS;
- display integrity `40701004059`: PASS;
- live-edit `40701004057`: PASS;
- full target `40701005084`: PASS.

## C4 full-target evidence

Run `40701005084`:

- source/control locks: PASS;
- visual pages within 240x240: PASS;
- source-parity CLEAR/TAP/RESET semantics after contextual migration: PASS;
- Reverb V2 IR: dense `42497`, corr² `0.00075`, jumpE `2.279`, predelay delta `5292`;
- Chord technical/compile-out tests: PASS;
- 300 BPM ratchet/chord/sub/sample event-order stress: PASS;
- WP-A enabled `LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4/0`: PASS;
- WP-A Chord-off `LOW=180984 < MED=484357 < HIGH=1038671`, redraw `4/0`: PASS;
- live-edit host energy `433436686798`, ticks `159600`, applies `2470`, SUB builds `67`, reverb reconfigs `135`: PASS;
- emulator panel/audio: `189440` frames, peak `3530`, `148408` nonzero samples: PASS;
- browser/WASM 100 MHz steady realtime `2.206x`, steady render p95 `3.840 ms`, p99 `5.511 ms`, max `13.621 ms`: PASS;
- SEQ/HOME 80 MHz steady realtime `2.257x`, p95 `3.886 ms`, p99 `5.773 ms`, max `11.715 ms`: PASS;
- Chord + Reverb V2 80 MHz steady realtime `1.987x`, p95 `4.241 ms`, p99 `5.740 ms`, max `10.911 ms`: PASS;
- physical encoder live-edit 80 MHz steady realtime `2.087x`, p95 `4.379 ms`, p99 `6.186 ms`, max `14.915 ms`, tail `88174`, peak `3847`: PASS;
- Chromium keyboard/pointer including PLAY start-stop: PASS;
- image `189328 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `611545 B`, SHA-256 `9747bed2ac5c974aed8b0cf9f340f395c5d566b879e5632162de65b75d9d2864`;
- provenance attestation: PASS.

Hosted emulator timing varies with runner load; acceptance is the established threshold, not the fastest historical run.

The reusable target workflow's separate `Tests` step is intentionally skipped because `test:` is empty; `bash ci/build_target.sh` executes the regression matrix inside Build.

## Active C5 testing direction

C5 must make contextual action behavior and rendered guidance consume one authoritative/testable mapping source where practical. Required new parity coverage:

- all current pages and B0-B7 slots with Chord enabled and compiled out;
- active action label/slot matches handler semantics;
- inactive slots stay inert and unadvertised;
- destructive confirmation metadata/behavior cannot drift;
- C1-C4 REC/EDIT/SELECT/OCT/MOD/manual-note contracts remain green.

Then rerun the complete WP-A/WP-B/WP-C/audio/timing/emulator/browser/80 MHz/Chromium/target matrix. Require >=16 KiB RAM and pool headroom. On success close WP-C, activate WP-D, and stop/handoff.

## Retained technical baselines

Reverb V2 and current Chord acceptance remain regression evidence. Reverb V3 is later WP-E. The physical FM-1 live-edit symptom remains a later hands-on regression when hardware is available.

## Reporting rule

At each closeout record exact checkpoint SHA, workflow run IDs, measured gates and any hardware-only validation that remains deferred.
