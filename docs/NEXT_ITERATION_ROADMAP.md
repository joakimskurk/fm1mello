# SkurkyMello FM-1 - stabilization and product-evolution roadmap

Approved direction: 2026-10-25. Updated after WP-C3 closeout: 2026-10-28.

This roadmap is stabilization-first. Approval of later product direction is not permission to implement later work packages early.

See `START_HERE.md` for exact continuation procedure, `PROJECT_STATE.md` for the verified checkpoint, `docs/WP_C_CONTROL_AUDIT.md` for the historical control-migration baseline, `CONTROL_GRAMMAR.md` for normative interaction rules and `TESTING.md` for acceptance gates.

## WP-A. Randomizer intensity + RANDOM skyline - COMPLETE

Completed 2026-10-26. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; clean reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

## WP-B. Trust and reliability - COMPLETE

Completed 2026-10-28. Final implementation/test checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; full target `40604214660`: PASS.

## WP-C. Control Grammar v2 - ACTIVE

Purpose: preserve depth while making one coherent interaction language.

### C1. Audit and baseline - COMPLETE

Checkpoint `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`. Enumerated and tested pre-remap REC, EDIT, SELECT, black-key, OCT, ALGORITHM/PRESETS and page-action behavior.

### C2. REC cleanup - COMPLETE

Checkpoint `f6d1f81660dab8db1de6da4e4650d01482d1ced3`. REC is reserved for record/capture; legacy functions moved to contextual B0-B3; SAMPLE moved to ENV/SOUND; STEP manual note exception preserved.

### C3. EDIT cleanup - COMPLETE

Implementation/test checkpoint `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`.

- generic EDIT -> RANDOM removed;
- Turing LOCK and Sample per-step EDIT states retained;
- STEP/SUB hidden EDIT+white actions moved to selected-position B4;
- DOT reset moved to B2;
- B5 opens RANDOM;
- C2 REC contract and SEL+black note entry preserved;
- exact generated UI SHA `c811c6ee8e5d6422a479452c65e0c4720d93d0b979cba3b4bcd1281f95a162da`.

Evidence: EDIT `40685833340`, reconstruction `40685833285`, REC `40685833209`, action contract `40685833243`, rotary `40685833249`, display `40685833215`, live-edit `40685833246`, audit `40685833273`, target `40685833648`: PASS.

Target retained RAM headroom `21472 B`, pool `17696 B`; package SHA-256 `6673f3688954809a6b5ce00ed73ba39666764ec2414d224e1f206689e1fe7a52`.

### C4. Contextual rail and MOD/navigation cleanup - ACTIVE

- migrate RANDOM mask NONE/ALL away from OCT;
- migrate GLOBAL RESET ALL away from hidden OCT chord, retaining explicit confirmation;
- remove LOOP layer-3 K4 RESET and rely on contextual B2;
- render visible guidance for every active black-key action while inactive slots remain inert;
- group actual Audio LFO, DOT and Turing/EVOLVE coherently under MOD/generative navigation;
- physical LFO/MOD navigation must not secretly toggle Turing state;
- preserve SELECT layer behavior, C2/C3 actions and STEP SEL+black manual-note exception;
- add deterministic Chord-on/off action/navigation coverage.

### C5. Authoritative mapping contract + regression closeout

After C4 only: make one authoritative/testable page-action mapping source drive handler semantics and rendered guidance where practical. Rerun WP-A/WP-B/audio/timing/emulator/browser/80 MHz/Chromium/target; require >=16 KiB RAM and pool headroom. Record exact evidence, set WP-D active, stop/handoff.

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
