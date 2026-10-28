# SkurkyMello FM-1 - stabilization and product-evolution roadmap

Approved direction: 2026-10-25. Updated after WP-C closeout and WP-D D1 audit: 2026-10-28.

This roadmap is stabilization-first. Approval of later product direction is not permission to implement later work packages early.

See `START_HERE.md` for exact continuation procedure, `PROJECT_STATE.md` for verified checkpoint, `CONTROL_GRAMMAR.md` for normative control rules, `docs/WP_D_TIMING_AUDIT.md` for the pre-D2 timing baseline, `docs/WP_C_CONTROL_AUDIT.md` for historical pre-remap baseline and `TESTING.md` for acceptance evidence.

## WP-A. Randomizer intensity + RANDOM skyline - COMPLETE

Completed 2026-10-26. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

## WP-B. Trust and reliability - COMPLETE

Completed 2026-10-28. Final checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; target `40604214660`: PASS.

## WP-C. Control Grammar v2 - COMPLETE

Completed 2026-10-28. Final implementation/test checkpoint `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

WP-C result:

- REC = actual recording/capture only;
- EDIT = edit/focus/lock only;
- SELECT = functional-layer navigation;
- white keys own positions/cells except explicit note-entry;
- black keys form a visible stable contextual B0-B7 action rail;
- hidden RANDOM OCT, GLOBAL OCT-reset and LOOP K4 actions removed;
- Audio LFO, DOT and EVOLVE/Turing grouped coherently under MOD navigation;
- FX opens AUDIO FX directly;
- one authoritative `MelloContextAction` descriptor now drives both contextual handler semantics and rendered action guidance.

C5 final parity:

- Chord enabled: `16 pages / 31 active / 97 inactive` slots;
- Chord compiled out: `15 pages / 29 active / 91 inactive`;
- C5 UI SHA `f6d26e0d1a57a51b08bf65e878144f5d48bfa248244d97b80c7a0842eb601818`.

Final green runs: authoritative `40703570244`, reconstruction `40703570315`, context `40703570196`, REC `40703570212`, EDIT `40703570277`, action `40703570204`, audit `40703570330`, rotary `40703570466`, display `40703570254`, live-edit `40703570316`, target `40703571071`.

Target closeout: RAM headroom `21472 B`, pool headroom `17696 B`, package SHA-256 `755a098ecbe66a6f8f7697a5787cb240e50a69df2520dc2f10b690f0056b8da1`.

## WP-D. Timing model - ACTIVE

Approved main-sequence product model:

- default **`2 BAR`**;
- options `1 / 2 / 4 / 8 / 16 BAR / FREE`;
- BAR values are musical durations and retain their bar count across sequence pulse-division changes;
- main-sequence bar reset is not external/MIDI sync;
- Loop Cycle `SYNC / FREE` remains a separate timing concept;
- Slot/Chain hold length remains separate;
- UI terminology must distinguish these concepts explicitly.

### D1. Audit and deterministic baseline - CLOSEOUT GATE

The current reconstructed implementation is now enumerated in `docs/WP_D_TIMING_AUDIT.md`. Legacy main reset is `master_len` in processed pulses; `0` means natural/free. External transport sync, Loop Cycle sync and Slot/Chain hold are separate state/event paths.

`tools/wp_d_timing_baseline.c` provides deterministic runtime coverage for defaults/storage, natural versus forced main reset, Loop Cycle sync separation, Slot Seq hold separation and external MIDI slave timing. The WP-D audit workflow locks both this behavior and the legacy timing labels before D2 intentionally changes them.

D1 becomes COMPLETE when that workflow is green on the final D1 documentation/CI checkpoint.

### D2. Main-sequence timing model

Implement `1 / 2 / 4 / 8 / 16 BAR / FREE` with **`2 BAR` as the default**. Use the smallest compatible engine/storage migration. A selected BAR value must map to musical bars rather than a fixed raw processed-pulse count, so sequence pulse-division changes cannot silently change the chosen bar duration.

### D3. UI/control language

Expose the timing choices with terminology that cannot be mistaken for MIDI clock or Loop Cycle sync. Preserve the existing control grammar and rotary/action parity.

### D4. Regression closeout

Rerun WP-A, all WP-B/WP-C gates, timing/audio event-order, emulator/browser, 80 MHz, Chromium and target build. Require >=16 KiB RAM and pool headroom. Record exact evidence, set WP-E active, then stop/handoff.

### WP-D exclusions

No distortion expansion, Reverb V3, Undo, FM/SOUND randomization, per-step timbre, harmonic replacement or general onboarding work inside WP-D.

## WP-E. Audio character

Extreme bounded distortion, Reverb V3 with perceptual A/B, ratchet parity and realtime-safe live smoothing.

## WP-F. Undo Last Random

One-level pre-random snapshot after control/timing/audio stabilization.

## WP-G. Synth evolution

Preserve ANALOG, add curated FM under budgets, independent SOUND randomization and realtime-safe per-step timbre.

## WP-H. Harmonic layer decision

Prototype scale-aware HARMONY first, PAD second; current Chord remains regression-protected until a successor wins.

## WP-I. Instrument polish and onboarding

Splash, HOME hierarchy, Key/Scale, SEQ explanatory hierarchy, contextual hints and newcomer usability.

## WP-J. Final integration gate

Full control, timing, randomizer, audio, visual, feature-flag, target, emulator/browser, memory/headroom and hands-on product audition gates.
