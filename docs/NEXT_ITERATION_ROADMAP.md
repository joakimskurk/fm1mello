# SkurkyMello FM-1 - stabilization and product-evolution roadmap

Approved direction: 2026-10-25. Updated after WP-B2 closeout: 2026-10-28.

This roadmap is stabilization-first. Approval of later product direction is not permission to implement later work packages early.

See `START_HERE.md` for exact continuation procedure, `PROJECT_STATE.md` for the verified checkpoint, `docs/PRODUCT_WORKSHOP_2026-10-25.md` for audition findings, `DECISIONS.md` for locked product decisions, `CONTROL_GRAMMAR.md` for interaction rules and `TESTING.md` for acceptance gates.

## WP-A. Randomizer intensity + RANDOM skyline - COMPLETE

Completed 2026-10-26.

Implementation commit: `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`.
Clean reconstruction closeout: `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

## WP-B. Trust and reliability - ACTIVE

This gate exists because advanced functionality is useless if live editing and the interface cannot be trusted.

### B1. Live-edit audio freeze - COMPLETE

Accepted on current host/emulator/browser/target evidence on 2026-10-27.

Verified checkpoint: `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`.

Evidence: live-edit run `40596812842`, reconstruction `40596812864`, target `40596813400`. Physical FM-1 hands-on regression remains required when hardware becomes available.

### B2. Rotary mapping parity - COMPLETE

Accepted 2026-10-28.

Verified checkpoint: `85a3c376b47829af6d6abdd3264ec75cb37d42bf`.

Delivered:

- truthful GLOBAL K4 MIDI OUT footer;
- corrected Loop Cycle STEP K3/K4 OCT/TRANS positions;
- truthful SLOTS inactive/edit footer state;
- real-encoder K1-K4 fixture for every current page/layer;
- explicit action semantics in the fixture;
- feature-on and Chord compile-out CI coverage.

Evidence:

- rotary parity run `40599056435`: PASS, `180/45` enabled and `160/40` compiled out;
- reconstruction `40599056475`: PASS;
- B1 regression `40599056403`: PASS;
- full target `40599056945`: PASS;
- RAM headroom `21472 B`, pool headroom `17696 B`.

### B3. Display integrity - ACTIVE SUBTASK

B3 acceptance:

- eliminate clipping/overlap at legal extreme values;
- remove the static pink line reported over `SINGLE` on HOME;
- test stale/dirty regions and relevant incremental redraw equivalence;
- cover every current page/layer with deterministic visual-integrity checks;
- preserve B1, B2, WP-A and all audio/timing/performance/target gates.

Reproduce before changing rendering. Screenshot judgement alone is not sufficient.

Major synth expansion remains blocked until WP-B is green.

### WP-B completion rule

After B3 is green, rerun the full regression and target matrix. Record exact implementation/run evidence in project/testing docs, set WP-C active, then stop and hand off rather than silently beginning WP-C.

## WP-C. Control Grammar v2

REC means actual recording/capture only; EDIT means edit/focus/state only; unrelated page actions migrate to visible contextual actions. Black keys become a contextual action rail where useful, SELECT remains layer navigation, and LFO/DOT/Turing are organized coherently under the modulation/generative family. The page-action mapping becomes authoritative and testable.

## WP-D. Timing model

Approved main-sequence default: `1 BAR`. Expose `1 / 2 / 4 / 8 / 16 BAR / FREE` while keeping external sync, main-sequence reset/free, Loop Cycle sync/free and Slot/Chain hold length distinct.

## WP-E. Audio character

Extend distortion to an intentionally extreme but bounded range, build Reverb V3 with perceptual A/B as a hard gate, preserve HTML ratchet envelope-retrigger parity and keep continuous sound/FX controls realtime-safe.

## WP-F. Undo Last Random

One-level pre-random snapshot, cheap in RAM/storage and isolated from normal slot/project persistence.

## WP-G. Synth evolution

Only after WP-B through WP-E are green: preserve current synth as `ANALOG`, add curated `FM` under hard CPU/RAM/pool budgets, add independent `SOUND` randomization and realtime-safe per-step timbre locks/modulation.

## WP-H. Harmonic layer decision

Current Chord remains regression-protected. Prototype/audition HARMONY first, PAD second, then SUB HARMONY/CHORD V2 only if useful.

## WP-I. Instrument polish and onboarding

Splash, HOME hierarchy, Key/Scale, SEQ explanatory hierarchy, contextual hints, consistent typography/margins and newcomer usability.

## WP-J. Final integration gate

Run the full text-fit, control mapping, randomizer, ratchet/audio event-order, reference parity, chord feature-on/compile-out, target, emulator, browser, 80 MHz, Chromium, memory/headroom and hands-on product audition gates.

## Deferred / dependency-gated

Factory samples wait for source audio. Large synthesis/sequencing expansion waits for WP-B through WP-E. Chord implementation changes wait for WP-H.
