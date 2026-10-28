# SkurkyMello FM-1 - stabilization and product-evolution roadmap

Approved direction: 2026-10-25. Updated after WP-C4 closeout: 2026-10-28.

This roadmap is stabilization-first. Approval of later product direction is not permission to implement later work packages early.

See `START_HERE.md` for exact continuation procedure, `PROJECT_STATE.md` for verified checkpoint, `docs/WP_C_CONTROL_AUDIT.md` for the historical control baseline, `CONTROL_GRAMMAR.md` for normative rules and `TESTING.md` for acceptance evidence.

## WP-A. Randomizer intensity + RANDOM skyline - COMPLETE

Completed 2026-10-26. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; clean reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

## WP-B. Trust and reliability - COMPLETE

Completed 2026-10-28. Final implementation/test checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; full target `40604214660`: PASS.

## WP-C. Control Grammar v2 - ACTIVE

Purpose: preserve depth while making one coherent interaction language.

### C1. Audit and baseline - COMPLETE

Checkpoint `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.

### C2. REC cleanup - COMPLETE

Checkpoint `f6d1f81660dab8db1de6da4e4650d01482d1ced3`. REC reserved for record/capture; legacy actions moved to contextual controls.

### C3. EDIT cleanup - COMPLETE

Checkpoint `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`. EDIT restricted to genuine edit/focus/lock; selected STEP/SUB actions moved to B4; DOT reset to B2; RANDOM access to B5.

### C4. Contextual rail and MOD/navigation cleanup - COMPLETE

Implementation/test checkpoint `84d42c9fa21b2a91db5fa1aff2340c6ef3289a60`.

Delivered:

- RANDOM B6/B7 = NONE/ALL; OCT has no RANDOM action semantics;
- GLOBAL B3 = confirmed reset; hidden OCT chord removed;
- LOOP K4 reset removed; contextual B2 retained;
- active black-key actions have rendered guidance and inactive slots are inert;
- MOD cycles Audio LFO -> DOT -> EVOLVE without hidden state toggles;
- FX opens AUDIO FX directly;
- C2/C3, SELECT and manual-note contracts preserved.

Exact generated C4 UI SHA `a3829d2fdc354154021727838b4b17cf8420832f88565a2b2f290b17a06002c5`.

Evidence: reconstruction `40701004129`, context `40701004213`, REC `40701004034`, EDIT `40701004030`, action `40701004015`, audit `40701004022`, rotary `40701004133`, display `40701004059`, live-edit `40701004057`, target `40701005084`: PASS.

### C5. Authoritative mapping contract + WP-C regression closeout - ACTIVE

C4 intentionally leaves one structural cleanup for C5: contextual handler semantics and rendered action guidance still encode overlapping page/action knowledge separately.

C5 requirements:

- introduce one authoritative/testable page-action descriptor for B0-B7 actions where practical;
- handler dispatch and rendered guidance consume that descriptor instead of independent page switches;
- preserve active/inactive slots, stable verb positions and destructive confirmation semantics;
- cover every current page/action mapping with Chord enabled and compiled out;
- preserve all C1-C4 rules and behavior;
- rerun WP-A, WP-B, WP-C, audio/timing, emulator/browser, 80 MHz, Chromium and full target;
- require >=16 KiB RAM and pool headroom.

When C5 is green, close WP-C, set WP-D active and stop/handoff.

### WP-C exclusions

No timing redesign, distortion/Reverb V3, Undo, FM/SOUND randomization, per-step timbre, harmonic replacement or general onboarding polish inside WP-C.

## WP-D. Timing model

Main sequence `1 / 2 / 4 / 8 / 16 BAR / FREE`, default `1 BAR`; external sync, Loop Cycle sync/free and Slot/Chain hold length remain distinct.

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
