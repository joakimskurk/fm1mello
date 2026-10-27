# ROADMAP

This file is the short handoff index. Detailed scope and acceptance criteria live in `docs/NEXT_ITERATION_ROADMAP.md`. New chats start with `START_HERE.md`.

## Completed

### WP-A / Step 4 - Randomizer intensity + RANDOM skyline

**Complete 2026-10-26.**

Implementation commit: `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`.
Clean reconstruction closeout: `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

### WP-B1 - Live-edit audio freeze

**Complete 2026-10-27 on available host/emulator/target evidence.**

Verified checkpoint: `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`.

Delivered:

- stable pending audio-config publication;
- heavy SUB distortion-curve generation moved out of audio render;
- selective invalidation of derived LFO/delay/reverb/distortion/SUB state;
- 32-bit publication generation with 32768-update regression coverage;
- deterministic live-edit host stress;
- real 80 MHz front-panel encoder stress integrated into target validation;
- corrected emulator encoder-event syntax in existing 100 MHz and 80 MHz scripts.

Green evidence:

- live-edit run `40596812842`;
- reconstruction run `40596812864`;
- target run `40596813400`;
- 80 MHz live-edit steady realtime `1.673x`, with audio still present in the final second;
- target RAM headroom `21472 B`, pool headroom `17696 B`.

Physical FM-1 hands-on regression remains deferred until hardware is available.

## Active implementation

### WP-B2 - Rotary display/handler parity

Goal: make every K1-K4 label/value describe the parameter or action that the same physical rotary actually controls.

Confirmed defects to start from:

- GLOBAL subpage 0 K4 toggles MIDI note output but renders `REC / TAP`;
- Loop Cycle STEP has K3/K4 `TRANS` and `OCT` displayed opposite to their handlers.

Hard constraints:

- audit every page/layer, not only the two confirmed mismatches;
- create deterministic mapping coverage so drift becomes a CI failure;
- preserve all B1, WP-A, audio, timing and target-build gates;
- do not fold B3, Control Grammar v2, FM, Reverb V3 or later feature work into B2.

## Remaining WP-B work after B2

### WP-B3 - Display integrity

- remove text clipping/overlap at extreme values;
- remove HOME pink-line artifact;
- test stale dirty regions;
- extend text-fit and visual integrity coverage.

WP-B closes only after B1, B2 and B3 plus full regression validation are green.

## Approved work order after WP-B

### WP-C - Control Grammar v2

- REC = real recording/capture only;
- EDIT = edit/focus/lock only;
- page actions migrate to visible contextual actions;
- black keys become the contextual action rail where useful;
- SELECT remains functional-layer navigation;
- LFO + DOT group under MOD;
- Turing belongs to the broader modulation/generative family;
- page-action mapping becomes one authoritative/testable table.

### WP-D - Timing model

- main sequence exposes `1 / 2 / 4 / 8 / 16 BAR / FREE`;
- default is `1 BAR`;
- Loop Cycle `SYNC / FREE` remains separate;
- Slot/Chain hold length remains separate;
- external clock/MIDI sync remains separate.

### WP-E - Audio character

- intentionally extreme distortion range with gain management;
- Reverb V3 with perceptual listening as a hard gate;
- ratchet envelope/timing parity with original HTML;
- live parameter smoothing/no-dropout acceptance.

### WP-F - Undo Last Random

One-level pre-random snapshot after the core randomizer and trust/control/audio gates are stable.

### WP-G - Synth evolution

- preserve current engine as `ANALOG`;
- add curated `FM` mode under CPU/RAM budgets;
- add independent `SOUND` randomizer target;
- add realtime-safe per-step timbre locks/modulation.

### WP-H - Harmonic layer decision

Prototype and audition in this order:

1. scale-aware `HARMONY`;
2. soft filtered `PAD`;
3. SUB HARMONY if useful;
4. CHORD V2 if useful.

Current Chord remains regression-protected until a successor wins.

### WP-I - Instrument polish

- splash screen;
- slicker HOME hierarchy;
- Key/Scale refinement;
- SEQ explanatory hierarchy/onboarding;
- consistent margins/truncation/value formatting;
- newcomer usability pass.

### WP-J - Final integration/performance gate

Run all control, randomizer, audio, visual, feature-flag, target, emulator/browser and performance gates before release acceptance.

## Product rule

Do not reduce functionality merely to make the interface simpler. Prefer one coherent control grammar over several page-specific grammars.

## Deferred until dependencies are green

- Factory samples until source audio is supplied.
- Major new synth/sequence scope before WP-B through WP-E.
- Chord implementation changes before WP-H.
