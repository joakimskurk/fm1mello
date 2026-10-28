# SkurkyMello FM-1 - stabilization and product-evolution roadmap

Approved direction: 2026-10-25. Updated after WP-C2 closeout: 2026-10-28.

This roadmap is stabilization-first. Approval of later product direction is not permission to implement later work packages early.

See `START_HERE.md` for exact continuation procedure, `PROJECT_STATE.md` for the verified checkpoint, `docs/WP_C_CONTROL_AUDIT.md` for the current control migration baseline, `CONTROL_GRAMMAR.md` for normative interaction rules and `TESTING.md` for acceptance gates.

## WP-A. Randomizer intensity + RANDOM skyline - COMPLETE

Completed 2026-10-26. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; clean reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

## WP-B. Trust and reliability - COMPLETE

Completed 2026-10-28. Final implementation/test checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; full target `40604214660`: PASS.

B1 safe live editing, B2 rotary display/handler parity and B3 deterministic display integrity remain protected regressions. Physical FM-1 live-edit reproduction stays deferred until hardware is available.

## WP-C. Control Grammar v2 - ACTIVE

Purpose: preserve depth while making one coherent interaction language.

### C1. Audit and baseline - COMPLETE

Checkpoint `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.

Enumerated and deterministically tested the pre-remap REC, EDIT, SELECT, black-key, OCT, ALGORITHM/PRESETS and page-specific action map. Detailed baseline and migration debt live in `docs/WP_C_CONTROL_AUDIT.md`.

### C2. REC cleanup - COMPLETE

Checkpoint `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.

REC is now reserved for actual recording/capture. Legacy non-record functions moved to stable contextual slots B0 state / B1 trigger / B2 reset / B3 confirmed destructive. SAMPLE navigation moved into ENV/SOUND. Manual STEP SEL+black-key note entry remains musical.

Evidence: audit `40671589758`, current action contract `40671589741`, REC grammar `40671589777`, rotary `40671589763`, display `40671589734`, live-edit `40671589787`, reconstruction `40671589813`, target `40671590524`: PASS.

### C3. EDIT cleanup - ACTIVE

- remove generic EDIT -> RANDOM navigation while keeping RANDOM accessible through a coherent non-EDIT route;
- retain Turing EDIT=LOCK and Sample EDIT=per-step edit state;
- migrate STEP EDIT+white-key SKIP to a visible contextual action on the selected step;
- migrate SUB EDIT+white-key step enable to a visible contextual action on the selected SUB step;
- classify/migrate DOT reset if it is not genuine edit/focus/lock state;
- preserve C2 REC semantics and SEL+black-key manual-note exception;
- add deterministic Chord-on/off EDIT-contract coverage.

### C4. Contextual rail and MOD/navigation cleanup

After C3 only:

- migrate RANDOM OCT ALL/NONE;
- migrate GLOBAL RESET ALL from hidden OCT chord;
- remove LOOP K4 RESET page action in favor of contextual B2;
- make contextual actions visibly guided;
- group actual Audio LFO, DOT and Turing/EVOLVE coherently under MOD/generative navigation.

### C5. Authoritative mapping contract + regression closeout

One authoritative/testable action source should drive handlers and rendered guidance where practical. Rerun WP-A/WP-B/audio/timing/emulator/browser/80 MHz/Chromium/target; require >=16 KiB RAM and pool headroom. Record exact evidence, set WP-D active, stop/handoff.

### WP-C exclusions

No timing redesign, distortion/Reverb V3, Undo, FM/SOUND randomization, per-step timbre, harmonic replacement or general onboarding polish inside WP-C.

## WP-D. Timing model

Main sequence `1 / 2 / 4 / 8 / 16 BAR / FREE`, default `1 BAR`; external sync, Loop Cycle sync/free and Slot/Chain hold length remain separate concepts.

## WP-E. Audio character

Extreme bounded distortion, Reverb V3 with perceptual A/B, ratchet parity and realtime-safe live smoothing.

## WP-F. Undo Last Random

One-level pre-random snapshot after core control/audio gates are stable.

## WP-G. Synth evolution

Preserve ANALOG, add curated FM under budgets, independent SOUND randomization and realtime-safe per-step timbre.

## WP-H. Harmonic layer decision

Prototype scale-aware HARMONY first, PAD second; current Chord remains regression-protected until a successor wins.

## WP-I. Instrument polish and onboarding

Splash, HOME hierarchy, Key/Scale, SEQ explanatory hierarchy, contextual hints and newcomer usability.

## WP-J. Final integration gate

Full control, randomizer, audio, visual, feature-flag, target, emulator/browser, memory/headroom and hands-on product audition gates.
