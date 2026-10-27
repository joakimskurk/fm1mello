# SkurkyMello FM-1 - stabilization and product-evolution roadmap

Approved direction: 2026-10-25. Updated after WP-B1 closeout: 2026-10-27.

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

Delivered LOW/MED/HIGH intensity, direct MED compatibility, target-mask isolation, independent WAVE, no storage migration, full 4..32-step RANDOM skyline and deterministic/performance coverage.

Implementation commit: `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`.
Clean reconstruction closeout: `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

## WP-B. Trust and reliability - ACTIVE

This gate exists because advanced functionality is useless if live editing and the interface cannot be trusted.

### B1. Live-edit audio freeze - COMPLETE

Accepted on current host/emulator/browser/target evidence on 2026-10-27.

Verified checkpoint: `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`.

Implemented:

- stable odd/even pending-config publication;
- 32-bit generation counter with 32768-update regression;
- SUB distortion-curve generation moved out of audio render;
- selective derived-state invalidation rather than full rebuild on every parameter change;
- permanent host live-edit stress;
- real 80 MHz front-panel encoder live-edit stress.

Evidence:

- live-edit run `40596812842`: PASS;
- reconstruction run `40596812864`: PASS;
- target run `40596813400`: PASS;
- 80 MHz live-edit stress `1.673x` steady realtime with audio still active in the final second;
- RAM headroom `21472 B`, pool headroom `17696 B`.

Physical FM-1 hands-on regression remains required when hardware becomes available.

### B2. Rotary mapping parity - ACTIVE SUBTASK

Every page/layer must use or validate one authoritative K1-K4 mapping containing label, formatter, range/step, handler and intentional action semantics.

Confirmed defects before implementation:

- GLOBAL subpage 0 K4 toggles MIDI note output but footer says `REC / TAP`;
- Loop Cycle STEP K3/K4 handlers for octave/transpose are displayed in the opposite `TRANS`/`OCT` positions.

Hard failure: any rendered Kn label/value describes a different parameter than physical rotary n changes.

B2 acceptance:

- audit every page/layer, not only confirmed defects;
- exercise actual encoder input paths;
- verify label/value, direction and legal range where meaningful;
- explicitly classify momentary/action controls;
- deterministic CI fails on future mapping drift;
- preserve all B1 and prior regression gates.

### B3. Display integrity - PENDING AFTER B2

- eliminate clipping/overlap at extreme values;
- remove the static pink line over `SINGLE` on HOME;
- test stale dirty regions;
- extend visual smoke coverage across every page/layer.

Major synth expansion remains blocked until WP-B is green.

### WP-B completion rule

Preserve all WP-A, B1 and existing regression gates. Record exact commit/run evidence in project/testing docs, then stop and hand off to WP-C rather than silently beginning it.

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

Create one machine-readable/testable page-layer table covering K1-K4, white keys, black keys, SELECT, OCT, REC, EDIT, destructive actions and manual note-entry exceptions.

## WP-D. Timing model

Approved main-sequence default: `1 BAR`.

Expose `1 / 2 / 4 / 8 / 16 BAR / FREE` while keeping external sync, main-sequence reset/free, Loop Cycle sync/free and Slot/Chain hold length distinct.

## WP-E. Audio character

- extend distortion to an intentionally extreme but bounded range;
- build Reverb V3 with perceptual A/B as a hard gate;
- preserve HTML ratchet envelope-retrigger parity;
- all continuous sound/FX controls remain realtime-safe.

## WP-F. Undo Last Random

One-level pre-random snapshot, cheap in RAM/storage and isolated from normal slot/project persistence.

## WP-G. Synth evolution

Only after WP-B through WP-E are green:

- preserve current synth as `ANALOG`;
- add curated `FM` under hard CPU/RAM/pool budgets;
- add independent `SOUND` randomization;
- add realtime-safe per-step timbre locks/modulation.

## WP-H. Harmonic layer decision

Current Chord remains regression-protected. Prototype/audition HARMONY first, PAD second, then SUB HARMONY/CHORD V2 only if useful.

## WP-I. Instrument polish and onboarding

Splash, HOME hierarchy, Key/Scale, SEQ explanatory hierarchy, contextual hints, consistent typography/margins and newcomer usability.

## WP-J. Final integration gate

Run the full text-fit, control mapping, randomizer, ratchet/audio event-order, reference parity, chord feature-on/compile-out, target, emulator, browser, 80 MHz, Chromium, memory/headroom and hands-on product audition gates.

## Deferred / dependency-gated

- Factory samples baked into firmware: wait for source audio.
- Any feature requiring unresolved implementation details from the current work package.
- Large synthesis/sequencing expansion before WP-B through WP-E.
- Chord implementation changes before WP-H design decision.
