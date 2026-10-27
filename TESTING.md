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

Verified implementation/test checkpoint: `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`.

- `WP-B live-edit gate` run `40596812842`: PASS;
- `Reconstructed source gate` run `40596812864`: PASS;
- `Visual port target build` run `40596813400`: PASS;
- 2400 audio blocks complete without silence, queue overflow or transport stall;
- 32768-publication coalescing regression: PASS;
- 80 MHz physical encoder route remains audible through the final second;
- RAM/pool headroom remain above the 16 KiB floor.

The exact originally reported physical FM-1 freeze remains a required hands-on regression when hardware becomes available.

## WP-B2 verified baseline - rotary display/handler parity

B2 is complete.

Verified implementation/test checkpoint: `85a3c376b47829af6d6abdd3264ec75cb37d42bf`.

### Dedicated parity gate

- workflow: `WP-B rotary parity gate`;
- run: `40599056435`;
- result: PASS.

Coverage:

- actual `mello_ui_input` K1-K4 encoder path is exercised;
- rendered footer label/value for the same physical rotary is captured;
- all current page/layer fixtures are included;
- intentional action controls are represented explicitly;
- coarse/fine display updates are tolerated without hiding a non-responsive mapping;
- Chord feature-on and compile-out builds both pass.

Measured fixture coverage:

- Chord enabled: `180` rotary/footer mappings across `45` page/layer fixtures: PASS;
- Chord disabled: `160` rotary/footer mappings across `40` page/layer fixtures: PASS.

Confirmed mismatches fixed by B2:

- GLOBAL CLOCK K4: `MIDI OUT` / actual ON-OFF state;
- Loop Cycle STEP K3: `OCT`; K4: `TRANS`;
- SLOTS inactive/edit footer states made truthful.

### Reconstruction and regression gates

- `Reconstructed source gate` run `40599056475`: PASS;
- B1 live-edit gate run `40599056403`: PASS;
- full `Visual port target build` run `40599056945`: PASS.

Full target-run evidence at B2 checkpoint:

- 100 MHz browser/WASM: `1.663x` steady realtime;
- SEQ/HOME 80 MHz: `1.776x` steady realtime;
- Chord + Reverb V2 80 MHz: `1.760x` steady realtime;
- physical encoder live-edit 80 MHz: `1.707x` steady realtime;
- final one-second live-edit tail: `88168` nonzero samples, peak `3902`;
- 300 BPM timing: PASS;
- WP-A randomizer: PASS (`LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4/0`);
- emulator panel/audio: PASS;
- Chromium keyboard/pointer incl PLAY: PASS;
- image: `191280 B / 581564 B`;
- RAM: `76832 B / 98304 B`, headroom `21472 B`;
- pool: `326368 B / 344064 B`, headroom `17696 B`;
- package: `skurkymello-0.1-dev.fwsc`, `611545 B`;
- package SHA-256: `26ccf3716c4b15b3671e9b4d6d91cf5f162ec1e1e2f692ba486a70c0ea8cd8f8`;
- provenance attestation: PASS.

## Active WP-B3 gate - display integrity

B3 must be deterministic, not screenshot-only.

Required coverage:

- exercise every current page/layer at legal extreme formatted values;
- record text draw rectangles and assert they stay inside intended regions;
- assert adjacent footer label/value columns do not collide;
- detect accidental graphic primitives crossing protected text regions;
- reproduce the HOME `SINGLE` pink-line artifact with draw operations / stale-region state;
- compare incremental redraw against a clean redraw for affected HOME regions;
- keep B1/B2/WP-A/audio/timing/performance/target gates green.

B3 acceptance:

- no text overflow at legal extremes;
- no label/value collision;
- no accidental graphic overlap;
- no stale dirty-region artifact over `SINGLE` on HOME;
- every page/layer included in deterministic visual-integrity coverage.

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

When completing a work package or major WP-B subtask, record exact checkpoint SHA, workflow run IDs, pass/fail conclusion, relevant measured gates, hands-on audition result where required, and intentionally skipped hardware-only validation.
