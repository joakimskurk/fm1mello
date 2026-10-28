# SkurkyMello FM-1 - stabilization and product-evolution roadmap

Approved direction: 2026-10-25. Updated after WP-D closeout: 2026-10-28.

This roadmap is stabilization-first. Approval of later product direction is not permission to implement later work packages early.

See `START_HERE.md` for exact continuation procedure, `PROJECT_STATE.md` for verified checkpoint, `CONTROL_GRAMMAR.md` for normative control rules, `docs/WP_D_CLOSEOUT.md` for final timing evidence, `docs/WP_D_TIMING_AUDIT.md` for the pre-D2 timing baseline, `docs/WP_C_CONTROL_AUDIT.md` for historical pre-remap baseline and `TESTING.md` for acceptance evidence.

## WP-A. Randomizer intensity + RANDOM skyline - COMPLETE

Completed 2026-10-26. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

## WP-B. Trust and reliability - COMPLETE

Completed 2026-10-28. Final checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; target `40604214660`: PASS.

## WP-C. Control Grammar v2 - COMPLETE

Completed 2026-10-28. Final implementation/test checkpoint `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

WP-C result: REC/EDIT semantics cleaned up; SELECT and white-key roles preserved; visible stable B0-B7 contextual actions; coherent MOD navigation; one authoritative `MelloContextAction` descriptor for handler + rendered guidance.

Final target `40703571071`: PASS. RAM headroom `21472 B`, pool headroom `17696 B`.

## WP-D. Timing model - COMPLETE

Completed 2026-10-28. Final implementation/test checkpoint `0cd810b5deb90087d60b181682de7e9ff52682cd`.

### D1. Audit and deterministic baseline - COMPLETE

Mapped legacy `master_len` processed-pulse behavior, storage/defaults, transport reset path and the already-separate external sync, Loop Cycle sync and Slot/Chain HOLD concepts. Permanent baseline: `tools/wp_d_timing_baseline.c` + `docs/WP_D_TIMING_AUDIT.md`.

### D2. Main-sequence timing model - COMPLETE

Implemented `FREE / 1 / 2 / 4 / 8 / 16 BAR` with **2 BAR default** while retaining the existing 16-bit storage field/layout. BAR duration is musical across `DIV` changes. Noncanonical stored values are hardened so conversion cannot create a zero runtime reset length.

### D3. UI/control language - COMPLETE

SEQ renders `RESET` with `FREE`/BAR values. External/MIDI sync, Loop Cycle `SYNC/FREE` and Slot/Chain HOLD retain distinct language and behavior. Existing control grammar/parity stays regression-protected.

### D4. Regression closeout - COMPLETE

All 11 fast workflows PASS on `0cd810b...`; full target `40763918161` PASS. RAM headroom `21472 B`, pool headroom `17696 B`, image `190156 B`, package `611545 B`, package SHA-256 `bee43ce894a5a26472270b0890f1d64e400bfb68246980f5fe285179a4225b7f`, provenance PASS.

See `docs/WP_D_CLOSEOUT.md` for full evidence.

## WP-E. Audio character - ACTIVE

Goal: make the instrument more characterful without sacrificing realtime trust or the stabilized control/timing grammars.

### E1. Audit and deterministic/perceptual baseline - ACTIVE NEXT

Before changing audio behavior, enumerate and baseline:

- current distortion transfer/range and control path;
- Reverb V2 implementation, parameters, reference renders and technical gates;
- ratchet audio/timing behavior and any parity gaps;
- live parameter smoothing/update paths and allocation/reconfiguration behavior;
- CPU cost in normal, worst-case and live-edit 80 MHz scenarios;
- RAM/pool impact and current target headroom;
- all UI/control mappings touched by these audio parameters.

Add or extend deterministic tests where current behavior is not already protected. Define perceptual A/B criteria for sound-quality changes before implementation.

### E2. Extreme bounded distortion

Expand usable character toward more extreme settings without runaway gain, DC, unsafe clipping, zippering or realtime regressions. Preserve clear parameter semantics.

### E3. Reverb V3

Improve subjective reverb quality. Technical IR/tail/predelay gates remain necessary, but perceptual A/B is the acceptance criterion. Do not accept a numerically green but audibly worse reverb.

### E4. Ratchet parity + realtime-safe live smoothing

Close any audited ratchet parity gaps and ensure audio-character controls can be changed live without render-thread allocation, unsafe rebuilds or audible state corruption.

### WP-E closeout

Rerun all WP-A/WP-B/WP-C/WP-D gates plus audio-specific deterministic/perceptual A/B, emulator/browser, 80 MHz, Chromium and target build. Require >=16 KiB RAM and pool headroom. Record exact evidence, activate WP-F, then stop/handoff.

### WP-E exclusions

No Undo Last Random, ANALOG/FM expansion, SOUND randomization, per-step timbre, harmonic-layer replacement or general onboarding work inside WP-E.

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
