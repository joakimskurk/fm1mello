# TESTING

## Validation philosophy

A green technical metric is necessary but not sufficient for product acceptance. Deterministic tests, target/performance measurements and hands-on audition gates are used where applicable.

The user does not currently have a physical FM-1, so host/emulator/web/target-build validation is the primary available path until hardware arrives.

## Locked baselines

### WP-A

- canonical randomizer distance `LOW=194824 < MED=516917 < HIGH=1082367`;
- RANDOM redraw `4 fills / 0 texts`;
- WAVE isolation and 4..32 skyline: PASS.

### WP-B

Final checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`. Reconstruction `40604213816`, live-edit `40604213682`, rotary `40604213768`, display `40604213707`, target `40604214660`: PASS.

## WP-C C1 verified baseline

C1 checkpoint `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`. Permanent pre-remap fixtures capture REC/EDIT/SELECT/black-key/OCT/ALGORITHM/PRESETS/page-action debt with Chord enabled and compiled out.

## WP-C C2 verified baseline - REC cleanup

Checkpoint `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.

- REC reserved/inert until a real capture path exists;
- B0 state, B1 trigger, B2 reset, B3 confirmed destructive;
- RANDOM B1 reroll / B3 CLEAR, GLOBAL B1 TAP, LOOP B2 reset, DOT/TURING B1 reroll, SUB/CHORD/SAMPLE B0 toggle;
- SAMPLE via ENV/SOUND;
- STEP SEL+black manual note input overrides contextual actions.

Exact C2 locks: UI `3da137c8faa5800bcfddd8291176e77718bc320d890f20f9f9d0c9cba4aaffb0`; WP-A smoke `71fc32a27fedfb0509ab0ff9eccfc85ba53d90c9b6d6bf48f0915f15c1d5fa13`.

## WP-C C3 verified baseline - EDIT cleanup

Implementation/test checkpoint: `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`.

Current C3 contract:

- generic EDIT no longer opens RANDOM;
- Turing EDIT toggles LOCK;
- Sample EDIT toggles per-step edit state;
- all other EDIT presses are behaviorally inert;
- STEP white key selects only; B4 toggles SKIP on selected step;
- SUB white key selects only; B4 toggles selected SUB-step enable;
- DOT EDIT is inert; B2 toggles DOT reset;
- B5 opens RANDOM from non-RANDOM pages;
- STEP SEL+black remains musical and takes priority over contextual B5;
- REC/C2 actions remain unchanged.

Exact generated C3 UI SHA-256: `c811c6ee8e5d6422a479452c65e0c4720d93d0b979cba3b4bcd1281f95a162da`.

C3 workflow matrix:

- EDIT grammar `40685833340`: PASS;
- reconstruction `40685833285`: PASS;
- REC grammar `40685833209`: PASS;
- control-action contract `40685833243`: PASS;
- rotary parity `40685833249`: PASS;
- display integrity `40685833215`: PASS;
- live-edit `40685833246`: PASS;
- control audit `40685833273`: PASS;
- full target `40685833648`: PASS.

Full target `40685833648`:

- WP-A enabled `LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4/0`: PASS;
- WP-A Chord-off `LOW=180984 < MED=484357 < HIGH=1038671`: PASS;
- 300 BPM timing: PASS;
- live-edit host energy `433436686798`, ticks `159600`, applies `2470`, SUB builds `67`, reverb reconfigs `135`: PASS;
- emulator panel/audio: `189440` frames, peak `3530`, `148408` nonzero samples: PASS;
- browser/WASM 100 MHz steady realtime `1.680x`: PASS;
- SEQ/HOME 80 MHz steady realtime `1.701x`: PASS;
- Chord + Reverb V2 80 MHz steady realtime `1.556x`: PASS;
- physical encoder live-edit 80 MHz steady realtime `1.595x`, tail `88156`, peak `3274`: PASS;
- Chromium keyboard/pointer including PLAY start-stop: PASS;
- image `190788 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `611545 B`, SHA-256 `6673f3688954809a6b5ce00ed73ba39666764ec2414d224e1f206689e1fe7a52`;
- provenance: PASS.

The 100/80 MHz host emulator timing numbers vary materially with GitHub runner load; acceptance is the existing gate threshold, not comparison against the fastest historical hosted-runner result. All C3 performance gates remained above realtime.

The reusable target workflow's separate `Tests` step is intentionally skipped because `test:` is empty; `bash ci/build_target.sh` executes the required test matrix inside Build.

## Active C4 testing direction

Before C4 closes, deterministic Chord-on/off coverage must prove:

- RANDOM mask NONE/ALL no longer use OCT and remain available contextually;
- GLOBAL factory RESET ALL no longer uses the OCT chord and still requires explicit confirmation;
- LOOP K4 no longer performs RESET; contextual B2 still does;
- OCT has no unrelated page-action meanings;
- every active black-key contextual action has matching rendered guidance and inactive slots do nothing;
- actual Audio LFO, DOT and Turing/EVOLVE are reachable through coherent MOD-family navigation;
- physical LFO/MOD does not secretly toggle Turing state;
- C2/C3 REC/EDIT semantics and STEP SEL+black manual-note priority remain green;
- WP-A and WP-B quick gates remain green.

Full target/performance closeout for all of WP-C is C5, but C4 must still run sufficient reconstruction/control/display regression gates before handoff to C5.

## Retained technical baselines

Reverb V2 and Chord technical acceptance remain regression evidence. Reverb V3 is later WP-E. Held `ARP_UP` remains protected with 12 note-ons, 12 note-offs, 3 distinct pitches and clean Chord compile-out.

## Reporting rule

At each subtask closeout record exact checkpoint SHA, workflow run IDs, measured gates and any intentionally deferred hardware-only validation.
