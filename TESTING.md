# TESTING

## Validation philosophy

A green technical metric is necessary but not sufficient for product acceptance. Deterministic tests, target/performance measurements and hands-on audition gates are all used where applicable.

The user does not currently have a physical FM-1, so host/emulator/web/target-build validation is the primary available path until hardware arrives.

## WP-A verified baseline

WP-A / Randomizer intensity + RANDOM skyline is complete.

- clean reconstruction commit `42cf506be5b127e7f03c763966d1e75cc4a48a85`, run `40294234032`: PASS;
- full target run `40294234527`: PASS;
- randomizer distance `LOW=194824 < MED=516917 < HIGH=1082367`;
- RANDOM incremental redraw `4 fills / 0 texts`;
- WAVE isolation, 4..32 skyline, deterministic/legal-range coverage: PASS;
- Chord/Reverb, 300 BPM timing, emulator/browser/80 MHz/Chromium: PASS.

## WP-B1 verified baseline - live parameter editing / audio freeze

B1 is complete on the currently available host/emulator/target evidence.

Verified implementation/test checkpoint:

- commit: `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`.

### Dedicated host stress

- workflow: `WP-B live-edit gate`;
- run: `40596812842`;
- result: PASS.

Coverage:

- all major continuous SOUND, FX, SUB and SAMPLE config families are repeatedly mutated while transport and audio run;
- 2400 audio blocks complete without a silent stress window;
- no audio event-queue overflow;
- transport remains running and advances to tick `159600`;
- no STOP/PLAY recovery is required;
- test output: `2470` config applies, `67` SUB curve builds, `135` reverb reconfigurations;
- a separate 32768-publication coalescing case proves the 32-bit generation protocol does not alias at the old 16-bit distance and the newest config is applied once.

### Reconstruction

- `Reconstructed source gate` run `40596812864`: PASS.

### Full target/emulator/browser validation

- workflow: `Visual port target build`;
- run: `40596813400`;
- result: PASS;
- repo build/test command: `bash ci/build_target.sh`.

Live-edit 80 MHz physical encoder route:

- emulated duration: `9002 ms`;
- steady realtime: `1.673x`;
- raw nonzero audio samples: `743872`;
- raw peak: `3902`;
- final one-second tail after encoder stress: `88168` nonzero samples, peak `3902`;
- result: PASS.

The script navigates and edits through real front-panel encoder routes across SOUND, SUB, FX and SAMPLE while PLAY remains active.

Additional final-run gates:

- browser/WASM 100 MHz: PASS, `1.660x` steady realtime;
- SEQ/HOME 80 MHz with real encoder syntax: PASS, `1.756x` steady realtime;
- Chord + Reverb V2 80 MHz worst-case: PASS, `1.663x` steady realtime;
- emulator panel input/audio: PASS;
- Chromium keyboard/pointer E2E: PASS;
- WP-A randomizer regressions: PASS;
- 300 BPM timing: PASS;
- Chord/Reverb and compile-out regressions: PASS.

Target budget after B1:

- image: `191232 B / 581564 B`;
- RAM: `76832 B / 98304 B`, headroom `21472 B`;
- pool: `326368 B / 344064 B`, headroom `17696 B`;
- RAM headroom >= `16384 B`: PASS;
- pool headroom >= `16384 B`: PASS;
- package: `skurkymello-0.1-dev.fwsc`, `611545 B`;
- package SHA-256: `9d118b8559bd6fbd2bf89c3c763edb8a626da9ed75ec37e2324696d2ed5d72da`;
- build provenance attestation: PASS.

### B1 architecture assertions now regression-covered

- pending config publication has explicit update-in-progress/stable generations;
- normal live rotary changes do not allocate memory;
- SUB drive curve is prepared outside audio render and copied only when drive changes;
- unrelated SOUND edits do not rebuild reverb or SUB drive curve;
- reverb is reconfigured only when its structural inputs change;
- rapid pre-render edits coalesce to the latest complete snapshot.

### B1 hardware limitation

The exact originally reported physical FM-1 freeze has not been hands-on reproduced or re-auditioned because physical hardware is not yet available. When hardware arrives, repeated live rotary editing across SOUND/FX/SUB/SAMPLE remains a required regression audition. This limitation does not invalidate the deterministic realtime risks fixed and the currently green host/emulator/target evidence.

## Active WP-B2 gate - rotary display/handler parity

B2 is now the active subtask.

Confirmed source-level mismatches before implementation:

1. GLOBAL subpage 0 K4/R4 handler toggles MIDI note output, but the footer renders `REC / TAP`.
2. Loop Cycle STEP K3 changes octave and K4 changes transpose, but the footer renders K3=`TRANS`, K4=`OCT`.

Required test direction:

- exercise every page/layer K1-K4 through the actual `mello_ui_input` encoder path;
- capture the rendered footer label/value for the same physical rotary;
- verify label/value describes the state that rotary changes;
- verify direction and legal range where meaningful;
- use middle-of-range fixtures so boundary clamping does not create false negatives;
- explicitly classify momentary/action controls rather than pretending they are continuous parameters;
- account for displays whose formatter changes only after more than one fine encoder step;
- make future label/handler drift a CI failure.

Do not treat two manual label fixes as B2 completion.

## WP-B3 gates - display integrity

After B2:

- no text overflow at extreme values;
- no label/value collision;
- no accidental graphic overlap;
- no stale dirty-region artifact such as the reported pink line over `SINGLE` on HOME;
- every page/layer included in deterministic text-fit/stale-region coverage.

## Reverb V2 technical baseline

Retained as regression evidence, not final perceptual acceptance:

- corr²: `0.00075`;
- block-energy jump: `2.279`;
- exact 120 ms pre-delay shift: `5292` samples.

Reverb V3 remains a later approved work package and requires perceptual listening gates.

## Chord Machine regression baseline

Held `ARP_UP` path remains protected:

- `12` note-ons;
- `12` note-offs;
- `3` distinct pitches;
- feature must compile out with `MELLO_FEATURE_CHORD_MACHINE=0`.

## Later-package gates

WP-C through WP-J remain dependency-gated exactly as documented in `ROADMAP.md` and `docs/NEXT_ITERATION_ROADMAP.md`. Do not implement their scope inside WP-B.

## Test reporting rule

When completing a work package or major WP-B subtask, record:

- exact implementation/test checkpoint SHA;
- exact workflow run ID;
- pass/fail conclusion;
- relevant measured gates;
- hands-on audition result where required;
- intentionally skipped hardware-only validation.
