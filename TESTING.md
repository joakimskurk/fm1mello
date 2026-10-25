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

These numbers remain regression evidence. They no longer mean the reverb is product-complete. Reverb V3 is approved and must add perceptual listening gates.

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

Performance baseline remains valid. Visual acceptance is reopened for density, clipping and artifact cleanup.

## Current diagnostic CI state

At product-code checkpoint `1c0748c2140e106e0c38fa0637b4c06139c2625f`:

- Workflow run: `39925230733`
- Workflow: `Reconstructed source gate`
- Result: `failure`
- Reason: `ci/randomizer_roadmap_probe.py` intentionally terminates with non-zero `SystemExit` after printing diagnostic source context.

Do not treat this as a functional regression. Do not declare WP-A complete while this intentional diagnostic failure remains in the real gate.

## WP-A required tests - active next implementation

Before WP-A can be marked complete:

1. `MED` compatibility with previous randomizer behavior. Prefer deterministic old-path vs MED golden vectors/reference output where RNG design permits it.
2. Same selected target mask under LOW, MED and HIGH.
3. `WAVE` isolated randomization does not mutate unrelated targets.
4. LOW mutation depth is smaller than MED for applicable targets.
5. HIGH mutation depth is larger than MED without illegal values.
6. Every mutated field stays in legal range.
7. Sequence lengths `4..32` remain valid.
8. Reproducibility for identical seed/state where seed control exists.
9. RANDOM retains complete 4..32-step overview.
10. At 16 steps skyline is comfortably legible; at 17..32 no active step disappears.
11. Pulse/ratchet/event markers distinguish structurally different randomized patterns under compression.
12. RANDOM incremental redraw does not regress Step 3 performance budget.
13. Host smoke, reconstructed target build, emulator/browser path, 80 MHz stress and Chromium E2E remain green as applicable.
14. Existing reverb/chord technical regression coverage remains green.
15. Intentional diagnostic probe failure is removed/disabled in the real gate after its findings are represented by actual tests.

## WP-B trust/reliability gates

### Live parameter editing / audio freeze

Create a deterministic or stress-style reproduction that turns every continuous rotary-controlled parameter while transport and audio are active.

Pass criteria:

- no silent/frozen audio state;
- no STOP/PLAY recovery required;
- transport phase remains valid;
- no unbounded audio rebuild/allocation path from normal rotary movement;
- smoothing/ramping does not create zipper noise or destabilize timing.

Record which parameters trigger any failure and whether cause is audio-state rebuild, scheduler/event corruption, parameter discontinuity or UI/audio contention.

### Rotary display/handler parity

For every page/layer, verify R1-R4 from one authoritative mapping table.

Pass criteria:

- physical rotary N changes the parameter rendered under Rn;
- rendered value changes with that parameter only;
- legal min/max and direction match the handler;
- no hidden off-by-one page/layer mappings.

This becomes automated coverage, not screenshot-only inspection.

### Text fit and overlap

Extend the existing font-metric/bounds gate to every page/layer and extreme formatted value.

Pass criteria:

- no text box overflow;
- no label/value collision;
- no accidental graphic overlap;
- no stale dirty-region artifact such as pink line over `SINGLE` on HOME.

## WP-C Control Grammar v2 tests

Approved semantics must be tested after implementation:

- REC is recording/capture only;
- EDIT is edit/focus/lock only;
- legacy REROLL/TAP/RESET/CLEAR functionality remains reachable through explicit visible contextual actions;
- black-key contextual actions match their on-screen hints;
- stable verbs prefer stable physical black-key positions across pages;
- destructive actions require the approved hold/confirmation behavior;
- manual note-entry modes preserve required musical black-key behavior;
- SELECT/OCT/white-key behavior remains page-consistent;
- LFO/DOT/Turing navigation reflects the approved MOD family without changing source semantics.

`tools/control_grammar_smoke.c` should consume or validate the same page-action mapping table used by UI/input code where practical.

## WP-D timing-model tests

The approved default is `1 BAR`.

Tests must distinguish:

1. main-sequence bar reset/free mode;
2. Loop Cycle SYNC/FREE;
3. Slot/Chain hold length;
4. external MIDI/transport sync.

Required cases:

- default main sequence is 1 BAR = 16 sixteenth-note ticks;
- FREE follows natural sequence/segment/pulse structure without forced bar reset;
- 1/2/4/8/16-bar modes force intended phase reset independently of sequence length;
- Loop Cycle SYNC/FREE remains independent from main-sequence bar mode;
- a 16-step Chain/Slot hold does not silently redefine main-sequence length;
- external sync does not overwrite the internal semantic distinction between those controls.

## Ratchet parity tests

Original HTML is the reference behavior: ratchet is micro-retriggers inside a pulse and retriggers the synth envelope for every sub-trigger.

Required evidence:

- ratchet 1 produces one trigger;
- ratchet 2..8 produces matching ordered sub-triggers;
- each synth sub-trigger retriggers amplitude envelope;
- timing is distributed across pulse, not collapsed at start;
- note/gate lengths remain audible at realistic BPM values;
- sub/MIDI behavior is intentionally documented if it differs from lead synth.

## FX quality gates

### Distortion

Approved target is an intentionally extreme upper range with controlled lower range.

Pass criteria:

- maximum is clearly more extreme than current baseline;
- output remains bounded by final limiter/headroom model;
- gain compensation prevents loudness-only preference;
- lower range remains useful for subtle saturation;
- live parameter changes do not freeze/drop audio.

### Reverb V3

Retain objective impulse/energy/pre-delay/stability tests and add perceptual references.

Reject ordinary musical settings that expose obvious periodic ringing, comb-like pitch, short-delay character or ring-mod-like metallicity.

Audition at minimum/medium/maximum size, short/long decay, multiple pre-delay values and bright/dark tone settings. Include dry A/B and original HTML reference where practical.

## Future synth-engine gates

Before `FM` mode ships:

- fixed CPU/RAM/pool budget on 80 MHz target path;
- no regression to ANALOG mode;
- patch/state persistence versioned safely;
- randomizer target isolation for `SOUND`;
- per-step timbre changes are realtime-safe;
- no parameter-lock path allocates/rebuilds audio objects per step;
- useful musical range is tested, not only mathematical range.

## Harmonic-layer gates

Prototype order is approved:

1. scale-aware HARMONY;
2. soft filtered PAD;
3. SUB HARMONY/CHORD V2 only if needed.

Compare musical usefulness, tone control, CPU cost and interface clarity. Current Chord regression and compile-out tests stay active until a successor is explicitly selected.

## Visual/onboarding gates

Before final polish acceptance:

- splash screen appears cleanly and does not delay audio/boot materially;
- HOME has no static artifacts and remains performant;
- Key/Scale communicates root/scale/span without manual;
- SEQ pages show what they change in relation to running sequence;
- black-key contextual hints fit and match actual key actions;
- a user unfamiliar with browser prototype can identify PLAY, recording/edit state, sequence timing and randomizer operation from device alone.

## Test reporting rule

When completing a work package, record:

- exact commit SHA;
- exact workflow run ID;
- pass/fail conclusion;
- relevant measured gates;
- hands-on audition result where required;
- intentionally skipped hardware-only validation.

The user does not currently have a physical FM-1, so emulator/web/host/target-build validation is the primary available path until hardware arrives.
