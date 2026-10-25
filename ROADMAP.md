# ROADMAP

This file is the short handoff index. Detailed scope and acceptance criteria live in `docs/NEXT_ITERATION_ROADMAP.md`. New chats start with `START_HERE.md`.

## Active next implementation

### WP-A / Step 4 - Randomizer intensity + RANDOM skyline

Goal: add `LOW / MED / HIGH` without weakening the existing creative behavior, and make the RANDOM skyline denser while keeping all 4..32 steps visible.

Hard constraints:

- intensity changes target-specific mutation depth/span;
- intensity never changes the selected target mask;
- `MED` is the compatibility/default behavior;
- `LOW` stays close to current material;
- `HIGH` moves further but remains musical;
- `WAVE` remains independently selectable;
- 16 steps are comfortably legible;
- 17..32 remain visible without visual paging;
- do not implement later feature expansion inside WP-A.

## Approved work order after WP-A

### WP-B - Trust and reliability

1. Reproduce/fix live-edit audio freeze.
2. Guarantee R1-R4 screen labels match actual handlers.
3. Remove text clipping/overlap.
4. Remove HOME pink-line artifact.
5. Extend mapping/text-fit/stale-region automated coverage.

Major synth expansion is blocked until WP-B is green.

### WP-C - Control Grammar v2

Approved direction:

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
- Loop Cycle `SYNC / FREE` remains a separate concept;
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
