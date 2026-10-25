# TESTING

## Validation philosophy

A green technical metric is necessary but not sufficient for product acceptance. The 2026-10-25 audition exposed issues that previous hard gates did not catch, especially perceptual reverb quality, live-edit audio freezes, control-label drift and visual clipping.

Future acceptance combines deterministic tests, target/performance measurements and explicit hands-on audition criteria.

## Current known validation baseline

### Reverb V2 - technical baseline retained, product acceptance reopened

Previous measured full target gate:

- corr²: `0.00075`
- block-energy jump: `2.279`
- exact 120 ms pre-delay shift: `5292` samples
- 80 MHz chord/reverb stress: `1.833x` steady realtime
- target pool headroom: `17696 B`

These numbers remain regression evidence. They no longer mean the reverb is product-complete. Latest audition still reports metallic/ring-mod-like character and insufficient hi-fi diffusion. Reverb V3 therefore needs a perceptual listening gate in addition to numerical topology/energy tests.

### Chord Machine regression baseline

Technical acceptance remains protected while product direction is unresolved.

Held `ARP_UP` path:

- `12` note-ons
- `12` note-offs
- `3` distinct pitches

Feature must continue to compile out with `MELLO_FEATURE_CHORD_MACHINE=0`.

### UI performance / sequence overview baseline

Step 3 completed in `00c6c72ff5a2e7fda1f6f95d0add7b00fa37f171`.

Measured host redraw gates:

- HOME incremental playhead: `9 fills / 1 text`
- STEP incremental playhead: `15 fills / 1 text`
- RANDOM incremental playhead: `9 fills / 1 text`
- Static FX page during transport: `1 fill / 1 text`

80 MHz SEQ/HOME benchmark:

- `3.023x` steady realtime
- p95 render: `2.859 ms`
- p99 render: `4.451 ms`
- max render: `7.061 ms`
- target RAM headroom: `21984 B`
- pool headroom: `17696 B`

The performance gate remains valid, but visual acceptance is reopened for density, clipping and artifact cleanup.

## Current diagnostic CI state

At product-code checkpoint `1c0748c2140e106e0c38fa0637b4c06139c2625f`:

- Workflow run: `39925230733`
- Workflow: `Reconstructed source gate`
- Result: `failure`
- Reason: `ci/randomizer_roadmap_probe.py` intentionally terminates with non-zero `SystemExit` after printing diagnostic source context.

Do not treat this as a functional regression. Do not declare Step 4 complete while this intentional diagnostic failure remains in the real gate.

## Required Step 4 tests

Before Step 4 can be marked complete, add/verify:

1. `MED` compatibility with previous randomizer behavior. Prefer deterministic old-path vs MED golden vectors/reference output where RNG design permits it.
2. Same selected target mask under LOW, MED and HIGH.
3. `WAVE` isolated randomization does not mutate unrelated targets.
4. LOW mutation depth is smaller than MED for applicable targets.
5. HIGH mutation depth is larger than MED without illegal values.
6. Every mutated field stays in legal range.
7. Sequence lengths `4..32` remain valid.
8. Reproducibility for identical seed/state where seed control exists.
9. RANDOM retains the complete 4..32-step overview.
10. At 16 steps the skyline remains comfortably legible; at 17..32 no active step disappears from the overview.
11. Pulse/ratchet/event markers remain visible enough to distinguish structurally different randomized patterns.
12. RANDOM incremental redraw does not regress the Step 3 performance budget.
13. Host smoke, reconstructed target build, emulator/browser path, 80 MHz stress and Chromium E2E remain green as applicable.
14. Existing reverb/chord technical regression coverage remains green.

## Immediate post-Step-4 reliability gates

These are P0/P1 product gates before major synth expansion.

### Live parameter editing / audio freeze

Create a deterministic or stress-style reproduction that turns every continuous rotary-controlled parameter while transport and audio are active.

Pass criteria:

- no silent/frozen audio state;
- no requirement to STOP/PLAY to recover;
- transport phase remains valid;
- no unbounded audio rebuild/allocation path from normal rotary movement;
- control-rate smoothing/ramping does not create zipper noise or destabilize timing.

Record which parameters trigger any failure and whether the cause is audio state rebuild, scheduler/event corruption, parameter discontinuity or UI/audio contention.

### Rotary display/handler parity

For every page and SELECT layer, verify R1-R4 using one authoritative mapping table.

Pass criteria:

- physical rotary N changes the parameter rendered under Rn;
- rendered value changes with that parameter only;
- legal min/max and direction match the handler;
- no hidden off-by-one page/layer mappings.

This should become automated rather than relying only on screenshot inspection.

### Text fit and overlap

Extend the existing font-metric/bounds gate to cover every page/layer and extreme formatted value.

Pass criteria:

- no text box overflow;
- no label/value collision;
- no accidental overlap with graphics;
- no stale dirty-region artifact such as the pink line over `SINGLE` on HOME.

## Timing-model tests

When the timing work package begins, add tests that distinguish all four concepts:

1. Main-sequence bar reset/free mode.
2. Loop Cycle SYNC/FREE.
3. Slot/Chain hold length.
4. External MIDI/transport sync.

Required main-sequence cases:

- FREE follows the natural sequence/segment/pulse structure without forced bar reset.
- 1/2/4/8/16-bar modes force the intended phase reset independently of sequence length.
- default value matches the final product decision.
- a 16-step Chain/Slot hold does not silently redefine the full main-sequence length.

## Ratchet parity tests

Original HTML is the reference behavior: ratchet is a set of micro-retriggers inside a pulse and retriggers the synth envelope for every sub-trigger.

Required evidence:

- ratchet 1 produces one trigger;
- ratchet 2..8 produces the matching number of ordered sub-triggers;
- each synth sub-trigger actually retriggers the amplitude envelope;
- timing is distributed across the pulse, not collapsed at its start;
- note/gate lengths remain audible at realistic BPM values;
- sub/MIDI behavior is intentionally documented if it differs from the lead synth.

## FX quality gates

### Distortion

When range is extended, audition and measure at low, medium and maximum settings.

Pass criteria:

- maximum is clearly more extreme than current baseline;
- output remains bounded by the final limiter/headroom model;
- gain compensation prevents the effect from being judged only by loudness;
- lower range remains controllable for subtle use.

### Reverb V3

Retain objective impulse/energy/pre-delay/stability tests, but add perceptual references.

Reject if a normal synth impulse/note exposes obvious periodic ringing, comb-like pitch, short-delay character or ring-mod-like metallicity at ordinary musical settings.

Audition at minimum/medium/maximum size, short/long decay, multiple pre-delay values and bright/dark tone settings. Include dry A/B and original HTML reference where practical.

## Future synth-engine gates

Before `FM` mode ships:

- fixed CPU/RAM/pool budget on 80 MHz target path;
- no regression to ANALOG mode;
- patch/state persistence versioned safely;
- randomizer target isolation for SOUND;
- per-step timbre changes are realtime-safe;
- no parameter-lock path may allocate/rebuild audio objects on each step;
- useful musical range is tested, not only mathematical parameter range.

## Visual/onboarding gates

Before final polish acceptance:

- splash screen appears cleanly and does not delay audio/boot unnecessarily;
- HOME has no static artifacts and remains performant;
- Key/Scale communicates root/scale/span without requiring the manual;
- SEQ pages show what they change in relation to the running sequence;
- black-key contextual hints, if approved, fit and match the actual key actions;
- a user unfamiliar with the browser prototype can identify PLAY, record/edit state, sequence length/timing and randomizer operation from the device alone.

## Test reporting rule

When completing a work package, record:

- exact commit SHA;
- exact workflow run ID;
- pass/fail conclusion;
- relevant measured gates;
- hands-on audition result where required;
- any intentionally skipped hardware-only validation.

The user does not currently have a physical FM-1, so emulator/web/host/target-build validation is the primary available path until hardware arrives.
