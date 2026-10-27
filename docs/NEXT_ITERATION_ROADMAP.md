# SkurkyMello FM-1 - stabilization and product-evolution roadmap

Approved direction: 2026-10-25. Updated after WP-B closeout: 2026-10-28.

This roadmap is stabilization-first. Approval of later product direction is not permission to implement later work packages early.

See `START_HERE.md` for exact continuation procedure, `PROJECT_STATE.md` for the verified checkpoint, `docs/PRODUCT_WORKSHOP_2026-10-25.md` for audition findings, `DECISIONS.md` for locked product decisions, `CONTROL_GRAMMAR.md` for interaction rules and `TESTING.md` for acceptance gates.

## WP-A. Randomizer intensity + RANDOM skyline - COMPLETE

Completed 2026-10-26. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; clean reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

## WP-B. Trust and reliability - COMPLETE

Completed 2026-10-28.

Final implementation/test checkpoint: `7c20efbdc391c59118fef3f000689eeafaa176b9`.

### B1. Live-edit audio reliability - COMPLETE

Safe pending-config publication, selective DSP invalidation, 32-bit generation coverage, host stress and real 80 MHz encoder stress. Original checkpoint `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`.

### B2. Rotary mapping parity - COMPLETE

Truthful K1-K4 rendered footer vs actual handlers across every current page/layer, including feature-on and Chord compile-out. Original checkpoint `85a3c376b47829af6d6abdd3264ec75cb37d42bf`.

### B3. Display integrity - COMPLETE

Reproduced the HOME pink-line as a real font-box/graphic collision, reflowed the UI to actual pinned font metrics, added deterministic text-fit/overlap/overdraw/dirty-region coverage, preserved full RANDOM `WAVE`, and kept target text fitting freestanding-safe.

Evidence:

- display integrity `40604213707`: PASS, `150` fixtures enabled / `135` compiled out, `0` clips / `0` overlaps;
- reconstruction `40604213816`: PASS;
- B1 `40604213682`: PASS;
- B2 `40604213768`: PASS;
- full target `40604214660`: PASS;
- RAM headroom `21472 B`, pool headroom `17696 B`;
- package SHA-256 `b0bf86215ab4c547c668c54c3f023ff05fb7f4e87b728bcb7e84365a02cebf1e`.

The physical FM-1 live-edit symptom remains a later hands-on regression when hardware becomes available; it does not block the current host/emulator/browser/target WP-B closeout.

## WP-C. Control Grammar v2 - ACTIVE

The purpose of WP-C is not to delete depth; it is to make one coherent interaction language.

### C1. Audit and baseline

Before remapping anything, enumerate and deterministically test every current REC, EDIT, SELECT, black-key and page-specific action across all pages/layers. Identify every semantic overload against `CONTROL_GRAMMAR.md`.

### C2. Normalize REC / EDIT

- REC = actual recording/capture only.
- EDIT = edit/focus/lock only.
- Unrelated actions must migrate rather than survive as exceptions.

### C3. Contextual action rail and navigation

- move appropriate page actions to visible contextual actions;
- use black keys as the contextual action rail where useful and approved;
- keep SELECT as functional-layer navigation;
- group LFO + DOT coherently under MOD;
- place Turing in the broader modulation/generative family.

### C4. One authoritative mapping contract

Where practical, one authoritative/testable page-action mapping source should drive handler semantics and rendered guidance. Add parity coverage so UI hints cannot drift from behavior.

### C5. Regression closeout

Rerun WP-A, all WP-B gates, audio/timing, emulator/browser, 80 MHz, Chromium and target build. Require >=16 KiB RAM and pool headroom. Record exact evidence, set WP-D active, then stop/handoff.

### WP-C exclusions

No WP-D timing redesign, distortion/Reverb V3 work, Undo, FM/SOUND randomization, per-step timbre, harmonic replacement or general onboarding polish inside WP-C.

## WP-D. Timing model

Approved main-sequence default: `1 BAR`. Expose `1 / 2 / 4 / 8 / 16 BAR / FREE` while keeping external sync, main-sequence reset/free, Loop Cycle sync/free and Slot/Chain hold length distinct.

## WP-E. Audio character

Extend distortion to an intentionally extreme but bounded range, build Reverb V3 with perceptual A/B as a hard gate, preserve HTML ratchet envelope-retrigger parity and keep continuous sound/FX controls realtime-safe.

## WP-F. Undo Last Random

One-level pre-random snapshot, cheap in RAM/storage and isolated from normal slot/project persistence.

## WP-G. Synth evolution

Only after earlier gates are green: preserve current synth as `ANALOG`, add curated `FM` under hard CPU/RAM/pool budgets, independent `SOUND` randomization and realtime-safe per-step timbre locks/modulation.

## WP-H. Harmonic layer decision

Current Chord remains regression-protected. Prototype/audition HARMONY first, PAD second, then SUB HARMONY/CHORD V2 only if useful.

## WP-I. Instrument polish and onboarding

Splash, HOME hierarchy, Key/Scale, SEQ explanatory hierarchy, contextual hints, consistent typography/margins and newcomer usability.

## WP-J. Final integration gate

Run the full text-fit, control mapping, randomizer, ratchet/audio event-order, reference parity, chord feature-on/compile-out, target, emulator, browser, 80 MHz, Chromium, memory/headroom and hands-on product audition gates.

## Deferred / dependency-gated

Factory samples wait for source audio. Chord implementation changes wait for WP-H. Later feature packages do not leak into WP-C.
