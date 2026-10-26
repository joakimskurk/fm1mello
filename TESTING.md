# TESTING

## Validation philosophy

A green technical metric is necessary but not sufficient for product acceptance. Deterministic tests, target/performance measurements and hands-on audition gates are all used where applicable.

The user does not currently have a physical FM-1, so host/emulator/web/target-build validation is the primary available path until hardware arrives.

## Current verified baseline - WP-A closeout

WP-A / Randomizer intensity + RANDOM skyline is complete.

### CI evidence

Clean reconstruction checkpoint:

- commit: `42cf506be5b127e7f03c763966d1e75cc4a48a85`
- `Reconstructed source gate`: run `40294234032`
- result: PASS

Full target closeout:

- `Visual port target build`: run `40294234527`
- result: PASS
- repo build/test command: `bash ci/build_target.sh`
- note: the reusable workflow's separate `Tests` step is empty/skipped; `ci/build_target.sh` itself compiles and executes the required host, emulator, browser, performance and target tests inside the Build step.

### WP-A randomizer gates

Final run `40294234527`:

- Chord-enabled intensity distance: `LOW=194824 < MED=516917 < HIGH=1082367`
- Chord-compiled-out intensity distance: `LOW=180984 < MED=484357 < HIGH=1038671`
- MED uses the previous/default randomizer path directly: PASS
- LOW/MED/HIGH preserve the selected target mask: PASS
- WAVE-only randomization is isolated: PASS
- mutated fields remain in legal range: PASS
- sequence lengths `4..32` remain valid: PASS
- RANDOM full-sequence skyline remains present at `4..32` without visual paging: PASS
- RANDOM incremental redraw: `4 fills / 0 texts`
- deterministic/reproducible host coverage: PASS.

### Broader regression gates

Final run `40294234527`:

- 300 BPM timing stress: PASS
- CLEAR/TAP/RESET source parity: PASS
- universal 32-step overview and page-aware incremental redraw: PASS
- control grammar regression: PASS
- Chord Machine smoke and final acceptance: PASS
- Chord feature compile-out: PASS
- Reverb V2 technical regression: PASS
- emulator panel input/audio: PASS
- browser/WASM 100 MHz path: PASS, `1.872x` steady realtime
- Chromium keyboard/pointer E2E: PASS
- 80 MHz SEQ/HOME path: PASS, `2.231x` steady realtime
  - steady render p95: `4.011 ms`
  - steady render p99: `5.438 ms`
  - steady render max: `12.996 ms`
- 80 MHz Chord + Reverb V2 worst-case: PASS, `2.249x` steady realtime
  - steady render p95: `3.837 ms`
  - steady render p99: `5.574 ms`
  - steady render max: `8.237 ms`.

### Target budget baseline

Final run `40294234527`:

- image: `190560 B / 581564 B`
- RAM: `76320 B / 98304 B` -> headroom `21984 B`
- pool: `326368 B / 344064 B` -> headroom `17696 B`
- required RAM headroom: >= `16384 B` -> PASS
- required pool headroom: >= `16384 B` -> PASS
- package: `skurkymello-0.1-dev.fwsc`, `611545 B`
- package SHA-256: `fcd1fbf7624b50995f90267f4e168fe2a72328a8f7eeeb46c4982632cd999df0`
- build provenance attestation: PASS.

### Reconstruction integrity

WP-A patch transport is locked by:

- concatenated base64 SHA-256: `6893352e8da1fdceb0c3ca40e174c229f4c027d4f21326300f649b5c590172d8`
- decoded gzip SHA-256: `2902990dab7e56a9e8136ba131d49076b56fce63414df56c77c41c46d2102d8f`
- exact pre-WP-A source hashes
- exact post-WP-A source/test hashes.

The old `randomizer_roadmap_probe.py` is no longer part of the production reconstruction path. Its deliberate non-zero exit was the cause of the temporary red gate, not a product regression.

## Reverb V2 technical baseline

Retained as regression evidence, not final perceptual acceptance:

- corr²: `0.00075`
- block-energy jump: `2.279`
- exact 120 ms pre-delay shift: `5292` samples
- target pool headroom remains above release floor.

Reverb V3 remains a later approved work package and requires perceptual listening gates.

## Chord Machine regression baseline

Held `ARP_UP` path remains protected:

- `12` note-ons
- `12` note-offs
- `3` distinct pitches
- feature must compile out with `MELLO_FEATURE_CHORD_MACHINE=0`.

## Active WP-B trust/reliability gates

### Live parameter editing / audio freeze

Create a deterministic or stress-style reproduction that turns every continuous rotary-controlled parameter while transport and audio are active.

Pass criteria:

- no silent/frozen audio state
- no STOP/PLAY recovery required
- transport phase remains valid
- no unbounded audio rebuild/allocation path from normal rotary movement
- smoothing/ramping does not destabilize timing.

Record which parameters trigger any failure and whether the cause is audio-state rebuild, scheduler/event corruption, parameter discontinuity or UI/audio contention.

### Rotary display/handler parity

For every page/layer verify R1-R4 from one authoritative mapping source.

Pass criteria:

- physical rotary N changes the parameter rendered under Rn
- rendered value changes with that parameter only
- legal min/max and direction match the handler
- no hidden off-by-one page/layer mappings.

### Text fit, overlap and stale regions

Extend font-metric/bounds and dirty-region coverage across pages/layers and extreme values.

Pass criteria:

- no text overflow
- no label/value collision
- no accidental graphic overlap
- no stale artifact such as the reported pink line over `SINGLE` on HOME.

## Later-package gates

### WP-C Control Grammar v2

REC is recording/capture only, EDIT is edit/focus/lock only, contextual black-key actions match visible hints, destructive actions use approved confirmation behavior, and all page-action mapping is validated from one authoritative table.

### WP-D timing model

Tests must keep separate:

1. external MIDI/transport sync
2. main-sequence BAR RESET/FREE
3. Loop Cycle SYNC/FREE
4. Slot/Chain hold length.

Default main sequence is `1 BAR` = 16 sixteenth-note ticks.

### WP-E audio character

Distortion must gain an intentionally extreme but bounded upper range. Reverb V3 keeps objective stability tests and adds listening rejection for periodic ringing/comb/ring-mod-like metallicity. Ratchet behavior must preserve reference envelope retrigger semantics.

### WP-G synth evolution

Before FM ships: hard 80 MHz CPU/RAM/pool budget, no ANALOG regression, safe persistence, SOUND target isolation and realtime-safe per-step timbre.

### WP-H harmonic layer

Prototype HARMONY first, then PAD. Current Chord regression and compile-out tests remain active until a successor is explicitly selected.

### Final visual/onboarding gate

Splash, HOME, Key/Scale, SEQ hierarchy, contextual hints, margins/truncation and newcomer usability must all pass before final release acceptance.

## Test reporting rule

When completing a work package, record:

- exact implementation/closeout commit SHA
- exact workflow run ID
- pass/fail conclusion
- relevant measured gates
- hands-on audition result where required
- intentionally skipped hardware-only validation.
