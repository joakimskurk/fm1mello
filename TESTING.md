# TESTING

## Validation philosophy

A green technical metric is necessary but not sufficient for product acceptance. Deterministic tests, target/performance measurements and hands-on audition gates are all used where applicable.

The user does not currently have a physical FM-1, so host/emulator/web/target-build validation is the primary available path until hardware arrives.

## Locked baselines

### WP-A

- clean reconstruction `42cf506be5b127e7f03c763966d1e75cc4a48a85`;
- canonical randomizer distance `LOW=194824 < MED=516917 < HIGH=1082367`;
- RANDOM redraw `4 fills / 0 texts`;
- WAVE isolation and 4..32 skyline: PASS.

### WP-B

Final implementation/test checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`.

- reconstruction `40604213816`: PASS;
- live-edit `40604213682`: PASS;
- rotary parity `40604213768`: PASS;
- display integrity `40604213707`: PASS;
- full target `40604214660`: PASS.

B3 retained coverage: Chord enabled `150` fixtures / compiled out `135`, `0` clips, `0` overlaps; HOME dirty-vs-clean protected region PASS.

## WP-C C1 verified baseline

C1 checkpoint: `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.

The permanent pre-remap fixtures capture:

- REC/EDIT meaning on every page;
- SELECT layer behavior and content-selection invariance;
- empty black-key action baseline outside the manual-note exception;
- hidden STEP/SUB EDIT+key modifiers;
- RANDOM CLEAR/OCT debt;
- GLOBAL RESET ALL chord;
- LOOP K4 RESET action;
- ALGORITHM duplicate-SELECT behavior;
- PRESETS global slot-cursor behavior;
- current LFO/DOT/Turing placement.

Historical C1 counts: Chord enabled `16` pages / `16` REC overloads / `13` EDIT overloads; Chord disabled `15 / 15 / 12`.

## WP-C C2 verified baseline - REC cleanup

Implementation/test checkpoint: `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.

Current C2 contract:

- REC is reserved/inert until an actual capture path exists;
- B0 = state toggle;
- B1 = trigger/reroll/tap;
- B2 = reset;
- B3 = destructive action with confirmation;
- SAMPLE is reachable through ENV/SOUND, not REC;
- STEP `SEL+black key` remains chromatic manual-note input and overrides contextual actions.

Migrated actions:

- RANDOM B1 reroll / B3 confirmed CLEAR;
- GLOBAL B1 TAP;
- LOOP B2 reset;
- DOT/TURING B1 reroll;
- SUB/CHORD/SAMPLE B0 toggle.

Exact generated output locks:

- UI SHA-256 `3da137c8faa5800bcfddd8291176e77718bc320d890f20f9f9d0c9cba4aaffb0`;
- migrated WP-A smoke SHA-256 `71fc32a27fedfb0509ab0ff9eccfc85ba53d90c9b6d6bf48f0915f15c1d5fa13`.

C2 workflow matrix:

- control audit `40671589758`: PASS;
- control-action contract `40671589741`: PASS;
- REC grammar `40671589777`: PASS;
- rotary parity `40671589763`: PASS;
- display integrity `40671589734`: PASS;
- live-edit `40671589787`: PASS;
- reconstruction `40671589813`: PASS;
- target `40671590524`: PASS.

Full target `40671590524`:

- WP-A enabled `LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4/0`: PASS;
- WP-A Chord-off `LOW=180984 < MED=484357 < HIGH=1038671`: PASS;
- 300 BPM timing: PASS;
- live-edit host regression: PASS;
- emulator panel/audio: PASS;
- browser/WASM 100 MHz steady realtime `3.287x`;
- SEQ/HOME 80 MHz `3.355x`;
- Chord + Reverb V2 80 MHz `3.129x`;
- physical encoder live-edit 80 MHz `2.920x`, tail `88156` nonzero samples, peak `3274`;
- Chromium keyboard/pointer incl. PLAY: PASS;
- image `190820 B / 581564 B`;
- RAM `76832 B / 98304 B`, headroom `21472 B`;
- pool `326368 B / 344064 B`, headroom `17696 B`;
- package `611545 B`, SHA-256 `cb09e296517f2f699fdf2b926e2baba7de033b52b498be765850cf5709615c12`;
- provenance: PASS.

The reusable target workflow's separate `Tests` step is intentionally skipped because `test:` is empty; `bash ci/build_target.sh` runs the required regression/emulator/browser/performance tests inside Build.

## Active C3 testing direction

C3 must add deterministic Chord-on/off coverage proving:

- generic EDIT no longer navigates to RANDOM;
- Turing EDIT still toggles LOCK;
- Sample EDIT still toggles per-step edit state;
- STEP SKIP remains available through its migrated visible contextual action and no longer needs EDIT+white-key;
- SUB step enable remains available through its migrated visible contextual action and no longer needs EDIT+white-key;
- DOT reset is either retained only if justified as edit-state or migrated and regression-covered;
- RANDOM remains reachable through a coherent non-EDIT route;
- C2 REC semantics and SEL+black-key manual note entry remain unchanged.

C3 cannot close until WP-A/WP-B quick gates and a full target run remain green with >=16 KiB RAM and pool headroom.

## Retained technical baselines

Reverb V2 and Chord technical acceptance remain regression evidence. Reverb V3 is later WP-E. Held `ARP_UP` remains protected with 12 note-ons, 12 note-offs, 3 distinct pitches and clean Chord compile-out.

## Reporting rule

At each subtask closeout record exact checkpoint SHA, workflow run IDs, measured gates and any intentionally deferred hardware-only validation.
