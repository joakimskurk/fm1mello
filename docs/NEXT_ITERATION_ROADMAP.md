# SkurkyMello FM-1 - stabilization and product-evolution roadmap

Approved direction: 2026-10-25. Updated after WP-A closeout: 2026-10-26.

This roadmap is stabilization-first. Approval of later product direction is not permission to implement later work packages early.

See:

- `START_HERE.md` for exact new-chat continuation procedure;
- `PROJECT_STATE.md` for current verified checkpoint;
- `docs/PRODUCT_WORKSHOP_2026-10-25.md` for audition findings and rationale;
- `DECISIONS.md` for locked product decisions;
- `CONTROL_GRAMMAR.md` for approved hardware interaction rules;
- `TESTING.md` for acceptance gates.

## WP-A. Randomizer intensity + RANDOM skyline - COMPLETE

Completed 2026-10-26.

Delivered:

- `LOW`: conservative variation around current material;
- `MED`: direct compatibility path through the previous/default randomizer;
- `HIGH`: larger structural/timbral movement while remaining useful and legal;
- intensity changes target-specific mutation ranges/depth, not target selection;
- `WAVE` remains independent;
- no storage migration;
- RANDOM retains a single full-sequence 4..32-step skyline with no visual paging;
- pitch contour, playhead and event information remain meaningful under compression;
- incremental redraw improved to `4 fills / 0 texts` in the measured WP-A smoke;
- deterministic host coverage, timing stress, chord/reverb regressions, target build, emulator/browser, 80 MHz and Chromium gates pass.

Implementation commit: `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`.
Clean reconstruction closeout: `42cf506be5b127e7f03c763966d1e75cc4a48a85`.
Reconstruction run: `40294234032`.
Target closeout run: `40294234527`.

The temporary red reconstruction was caused by the old diagnostic probe still being invoked and deliberately exiting non-zero. The probe invocation and temporary tracing were removed; WP-A patch bytes plus pre/post source hashes are now integrity-locked.

## WP-B. Trust and reliability - ACTIVE NEXT IMPLEMENTATION

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

Every page/layer must use or validate one authoritative R1-R4 mapping containing label, formatter, range/step, handler and acceleration behavior.

Hard failure: any rendered Rn label/value describes a different parameter than physical rotary n changes.

### B3. Display integrity

- eliminate clipping/overlap at extreme values;
- remove the static pink line over `SINGLE` on HOME;
- test stale dirty regions;
- extend visual smoke coverage across every page/layer.

Major synth expansion remains blocked until WP-B is green.

### WP-B completion rule

Preserve all WP-A and existing regression gates. Record exact commit/run evidence in project/testing docs, then stop and hand off to WP-C rather than silently beginning it.

## WP-C. Control Grammar v2

### C1. REC

REC means actual recording/capture only. Legacy REC meanings such as REROLL, TAP, RESET or generic page action must migrate without losing functionality.

### C2. EDIT

EDIT means edit focus/state/lock only. It must not be a hidden modifier for unrelated or destructive commands.

### C3. Black-key contextual action rail

Use black keys for explicit contextual verbs where useful. Actions must be visible on screen, stable where practical, and manual note-entry modes must preserve musical behavior.

### C4. MOD family

Group conceptually:

- LFO = continuous cyclic modulation;
- DOT = discrete matrix modulation;
- Turing = evolving/generative modulation.

### C5. Complete action audit

Create one machine-readable/testable page-layer table covering R1-R4, white keys, black keys, SELECT, OCT, REC, EDIT, destructive actions and manual note-entry exceptions.

## WP-D. Timing model

Approved main-sequence default: `1 BAR`.

Expose:

- `1 BAR`
- `2 BAR`
- `4 BAR`
- `8 BAR`
- `16 BAR`
- `FREE`

Keep separate:

1. external MIDI/transport sync;
2. main-sequence BAR RESET/FREE;
3. Loop Cycle SYNC/FREE;
4. Slot/Chain hold length.

## WP-E. Audio character

### E1. Distortion

Extend the upper range until maximum is intentionally extreme while preserving subtle lower range, gain-managed headroom and realtime-safe live changes.

### E2. Reverb V3

Reverb V2 remains a technical regression baseline only. Reverb V3 must reject ordinary musical settings that expose periodic ringing, comb-like pitch, short-delay character or ring-mod-like metallicity, with perceptual A/B as a hard gate.

### E3. Ratchet parity

Original HTML is the reference: pulse is split into sub-triggers and synth amplitude envelope retriggers for every ratchet.

### E4. Live parameter smoothing

All continuous sound/FX controls remain safe while transport runs and do not require STOP/PLAY recovery.

## WP-F. Undo Last Random

One-level pre-random snapshot, cheap in RAM/storage and isolated from normal slot/project persistence.

## WP-G. Synth evolution

Only after WP-B through WP-E are green.

- preserve current synth as `ANALOG`;
- add curated `FM` under hard CPU/RAM/pool budgets;
- add independent `SOUND` randomization;
- add realtime-safe per-step timbre locks/modulation.

## WP-H. Harmonic layer decision

Current Chord remains regression-protected. Prototype/audition in order:

1. scale-aware `HARMONY`;
2. soft filtered `PAD`;
3. `SUB HARMONY` if useful;
4. `CHORD V2` if useful;
5. default-hidden/compile-out if none earns its space.

## WP-I. Instrument polish and onboarding

- splash screen;
- slicker HOME without sacrificing skyline/performance;
- Key/Scale refinement;
- SEQ explanatory hierarchy;
- visible contextual action hints;
- consistent margins/truncation/value formatting;
- newcomer test without prior HTML-prototype knowledge.

## WP-J. Final integration gate

Before release acceptance run the full text-fit, control mapping, randomizer, ratchet/audio event-order, reference parity, chord feature-on/compile-out, target, emulator, browser, 80 MHz, Chromium, memory/headroom and hands-on product audition gates.

## Deferred / dependency-gated

- Factory samples baked into firmware: wait for source audio.
- Any feature requiring unresolved implementation details from the current work package.
- Large synthesis/sequencing expansion before WP-B through WP-E.
- Chord implementation changes before WP-H design decision.
