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

Delivered stable pending audio-config publication, selective DSP invalidation, 32-bit generation coverage, host stress and real 80 MHz front-panel encoder stress. Physical FM-1 hands-on regression remains deferred until hardware is available.

Green evidence: live-edit run `40596812842`, reconstruction `40596812864`, target `40596813400`.

### WP-B2 - Rotary display/handler parity

**Complete 2026-10-28.**

Verified checkpoint: `85a3c376b47829af6d6abdd3264ec75cb37d42bf`.

Delivered:

- fixed GLOBAL K4 `MIDI OUT` footer parity;
- fixed Loop Cycle STEP K3=`OCT`, K4=`TRANS` parity;
- truthful inactive/edit state on SLOTS footer;
- deterministic real-encoder mapping fixture across every current page/layer;
- feature-on and Chord compile-out coverage.

Green evidence:

- rotary parity run `40599056435`: `180` mappings / `45` fixtures enabled, `160` / `40` compiled out;
- reconstruction run `40599056475`;
- B1 regression run `40599056403`;
- target run `40599056945`;
- RAM headroom `21472 B`, pool headroom `17696 B`.

## Active implementation

### WP-B3 - Display integrity

Goal: make legal extreme values and incremental redraws visually trustworthy on every page/layer.

Known defects / required reproductions:

- text clipping or overlap at extreme values;
- static pink-line artifact reported over `SINGLE` on HOME.

Hard constraints:

- reproduce before changing rendering;
- deterministic text-fit/overlap coverage across every current page/layer;
- deterministic stale/dirty-region coverage for HOME and relevant incremental redraws;
- preserve B1, B2, WP-A, audio, timing and target-build gates;
- do not fold Control Grammar v2, FM, Reverb V3 or later feature work into B3.

WP-B closes only after B3 plus full regression validation are green.

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

Prototype and audition scale-aware `HARMONY` first, soft filtered `PAD` second, then SUB HARMONY / CHORD V2 only if useful. Current Chord remains regression-protected until a successor wins.

### WP-I - Instrument polish

Splash, HOME hierarchy, Key/Scale refinement, SEQ explanatory hierarchy/onboarding, consistent margins/truncation/value formatting and newcomer usability.

### WP-J - Final integration/performance gate

Run all control, randomizer, audio, visual, feature-flag, target, emulator/browser and performance gates before release acceptance.

## Product rule

Do not reduce functionality merely to make the interface simpler. Prefer one coherent control grammar over several page-specific grammars.

## Deferred until dependencies are green

- Factory samples until source audio is supplied.
- Major new synth/sequence scope before WP-B through WP-E.
- Chord implementation changes before WP-H.
