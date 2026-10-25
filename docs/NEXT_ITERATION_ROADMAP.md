# SkurkyMello FM-1 - stabilization and product-evolution roadmap

Approved direction: 2026-10-25.

Baseline product-code checkpoint before this planning pass: `1c0748c2140e106e0c38fa0637b4c06139c2625f`.

This roadmap is stabilization-first. All product-owner workshop recommendations are approved, but approval is not permission to implement later work packages early.

See:

- `START_HERE.md` for exact new-chat continuation procedure;
- `docs/PRODUCT_WORKSHOP_2026-10-25.md` for audition findings and rationale;
- `DECISIONS.md` for locked product decisions;
- `CONTROL_GRAMMAR.md` for approved hardware interaction rules;
- `TESTING.md` for acceptance gates.

## WP-A. Randomizer intensity + RANDOM skyline - ACTIVE NEXT IMPLEMENTATION

Randomizer is a core creative instrument.

Add an intensity control independent of selected random targets:

- `LOW`: conservative variation around current material;
- `MED`: current/default creative range and compatibility anchor;
- `HIGH`: larger structural/timbral movement while remaining useful.

Requirements:

- intensity constrains target-specific mutation ranges/depth rather than acting as one global probability knob;
- target selection/mask is unchanged by intensity;
- preserve existing MED code path and RNG consumption order where practical;
- `WAVE` remains independent;
- legal values for all sequence lengths 4..32;
- deterministic host coverage where RNG architecture permits;
- no Undo, FM, Reverb V3 or broad control redesign inside WP-A.

### RANDOM skyline refinement

RANDOM remains a single full-sequence view:

- 4..16 steps: immediately readable columns;
- 16 steps must be comfortably legible;
- 17..32 steps: compressed, but every active step remains visible;
- retain pitch contour and unambiguous playhead;
- preserve meaningful pulse/ratchet/event cues under compression;
- no visual paging;
- incremental redraw cost remains inside the established Step 3 budget.

### WP-A completion rule

Remove/disable the intentional failure in `ci/randomizer_roadmap_probe.py` only when its findings have been converted into real tests. Do not declare WP-A complete until real CI is green and exact commit/run evidence is recorded.

Stop after WP-A. Prepare a handoff for WP-B instead of silently starting it.

## WP-B. Trust and reliability

This gate exists because advanced functionality is useless if live editing feels unreliable.

### B1. Live-edit audio freeze - highest priority

Reproduce and fix the reported case where audio freezes while turning parameters and recovers after STOP/PLAY.

Audit:

- audio-engine rebuilds/allocations;
- scheduler/event corruption;
- unsafe cross-state mutation;
- missing smoothing/ramping;
- UI/audio contention;
- any parameter path that reinitializes DSP unnecessarily.

Pass criteria:

- no silent/frozen state during repeated live parameter movement;
- no STOP/PLAY recovery required;
- transport phase remains valid;
- no unbounded rebuild/allocation path for normal rotary movement.

### B2. Rotary mapping parity

Every page/layer must use one authoritative R1-R4 mapping containing label, formatter, range/step, handler and acceleration behavior.

Hard failure: any rendered Rn label/value describes a different parameter than physical rotary n changes.

### B3. Display integrity

- eliminate clipping/overlap at extreme values;
- remove the static pink line over `SINGLE` on HOME;
- test stale dirty regions;
- extend visual smoke coverage across every page/layer.

Major synth expansion remains blocked until WP-B is green.

## WP-C. Control Grammar v2

Direction is approved.

### C1. REC

REC means actual recording/capture only.

Legacy REC meanings such as REROLL, TAP, RESET or generic page action must migrate without losing functionality.

### C2. EDIT

EDIT means edit focus/state/lock only.

It must not be a hidden modifier for unrelated or destructive commands.

### C3. Black-key contextual action rail

Use black keys for explicit contextual verbs where useful.

Rules:

- actions visible on screen;
- stable verb placement where practical;
- destructive actions distinct and confirmed/held where appropriate;
- manual note-entry modes preserve musical behavior;
- no filler assignments.

### C4. MOD family

Group conceptually:

- LFO = continuous cyclic modulation;
- DOT = discrete matrix modulation;
- Turing = evolving/generative modulation.

The current buried LFO navigation is migration debt.

### C5. Complete action audit

Before final mapping implementation, create one machine-readable/testable page-layer table covering R1-R4, white keys, black keys, SELECT, OCT, REC, EDIT, destructive actions and manual note-entry exceptions.

That table becomes the implementation source of truth and regression fixture.

## WP-D. Timing model

Approved main-sequence default: `1 BAR`.

Expose the main sequence as:

- `1 BAR`
- `2 BAR`
- `4 BAR`
- `8 BAR`
- `16 BAR`
- `FREE`

Keep four distinct timing concepts in state, naming and UI:

1. external MIDI/transport sync;
2. main-sequence `BAR RESET / FREE`;
3. Loop Cycle `SYNC / FREE`;
4. Slot/Chain hold length.

Tests must prove that a 16-step Chain/Slot hold does not silently redefine full main-sequence length.

## WP-E. Audio character

### E1. Distortion

Extend the upper range until maximum is intentionally extreme.

Requirements:

- preserve subtle lower range;
- gain compensation/headroom so extremity is not only louder;
- final limiter remains bounded;
- live changes remain realtime-safe.

### E2. Reverb V3

Reverb V2 remains technical regression baseline only.

Reverb V3 must be redesigned/tuned for perceptual diffusion quality and reject ordinary musical settings that expose:

- periodic ringing;
- comb-like pitch;
- short-delay character;
- ring-mod-like metallicity.

Retain objective impulse/energy/pre-delay/stability tests and add listening A/B against original HTML where practical.

### E3. Ratchet parity

Original HTML is reference behavior: pulse is split into sub-triggers and synth amplitude envelope retriggers for every ratchet.

Verify ratchet 1..8, timing distribution, audible note length and intended sub/MIDI behavior before redesign.

### E4. Live parameter smoothing

All continuous sound/FX controls remain safe while transport runs and do not require STOP/PLAY recovery.

## WP-F. Undo Last Random

One-level pre-random snapshot.

Requirements:

- cheap RAM/storage;
- restores every target touched by last random operation;
- no interference with normal slot/project persistence;
- architecture must later include WAVE/SOUND state when those targets exist.

## WP-G. Synth evolution

Only after WP-B through WP-E are green.

### ANALOG

Preserve the current synth engine and its musical behavior as a first-class mode.

### FM

Add a curated FM engine suited to FM-1 hardware identity.

Requirements:

- small set of musically useful macro parameters first;
- hard CPU/RAM/pool budget on 80 MHz path before expansion;
- no generic operator editor unless it proves necessary and usable;
- no regression to ANALOG mode.

### SOUND randomization

Add `SOUND` as an independent target so tone can change without forcing pitch/rhythm targets.

### Per-step timbre

Add realtime-safe timbre locks/modulation. Never allocate/rebuild audio objects per step.

## WP-H. Harmonic layer decision

Current Chord Machine remains regression-protected during the design spike.

Prototype/audition in order:

1. scale-aware `HARMONY` derived from lead/sub/key context;
2. soft filtered `PAD` with proper envelope/tone shaping;
3. `SUB HARMONY` if useful;
4. `CHORD V2` if useful;
5. default-hidden/compile-out if none earns its space.

A concept wins by fast musical usefulness and interface legibility, not parameter count.

## WP-I. Instrument polish and onboarding

- splash screen;
- slicker HOME without sacrificing skyline or performance;
- Key/Scale refinement;
- SEQ pages that explain their effect on the running pattern;
- visible contextual action hints;
- consistent margins/truncation/value formatting;
- newcomer test without prior HTML-prototype knowledge.

## WP-J. Final integration gate

Before release acceptance:

- full text-fit/clipping/stale-region suite;
- full Control Grammar v2 mapping suite;
- randomizer deterministic/parity suite;
- ratchet/audio event-order suite;
- HTML audio/sub reference checks where applicable;
- chord feature-on and compile-out regressions;
- target build;
- emulator audio;
- 100 MHz browser path;
- 80 MHz worst-case path;
- Chromium E2E;
- RAM/pool/image headroom review;
- hands-on product audition for reverb, distortion, live editing and learnability.

## Deferred / dependency-gated

- Factory samples baked into firmware: wait for source audio.
- Any feature requiring unresolved implementation details from the current work package.
- Large synthesis/sequencing expansion before WP-B through WP-E.
- Chord implementation changes before WP-H design decision.
