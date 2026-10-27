# ROADMAP

This file is the short handoff index. Detailed scope and acceptance criteria live in `docs/NEXT_ITERATION_ROADMAP.md`. New chats start with `START_HERE.md`.

## Completed

### WP-A / Step 4 - Randomizer intensity + RANDOM skyline

**Complete 2026-10-26.**

Implementation: `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`.
Clean reconstruction closeout: `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

### WP-B - Trust and reliability

**Complete 2026-10-28.**

Final implementation/test checkpoint: `7c20efbdc391c59118fef3f000689eeafaa176b9`.

B1 delivered safe live audio-config publication, selective DSP invalidation and host/80 MHz live-edit stress. B2 delivered deterministic K1-K4 display/handler parity across every current page/layer. B3 reflowed UI geometry around the real 8x16/16x32 font metrics, removed the HOME pink-line collision, added deterministic clip/overlap/dirty-redraw coverage and preserved full RANDOM `WAVE`.

Final green evidence:

- reconstruction `40604213816`;
- B1 live-edit `40604213682`;
- B2 rotary parity `40604213768`;
- B3 display integrity `40604213707` (`150` fixtures on / `135` off, `0` clips, `0` overlaps);
- full target `40604214660`;
- RAM headroom `21472 B`, pool headroom `17696 B`;
- package SHA-256 `b0bf86215ab4c547c668c54c3f023ff05fb7f4e87b728bcb7e84365a02cebf1e`.

Physical FM-1 live-edit regression remains a later hands-on check when hardware exists; current host/emulator/browser/target evidence is green.

## Active implementation

### WP-C - Control Grammar v2

Goal: keep functionality and depth while eliminating inconsistent page-specific button grammars.

Approved work:

- audit and test the current complete action map first;
- REC = actual recording/capture only;
- EDIT = edit/focus/lock only;
- unrelated page actions migrate to visible contextual actions;
- black keys become the contextual action rail where useful;
- SELECT remains functional-layer navigation;
- LFO + DOT group under MOD;
- Turing belongs to the broader modulation/generative family;
- page-action mapping becomes one authoritative/testable source where practical;
- rendered guidance must match actual handlers.

Hard constraints:

- preserve WP-A and all WP-B gates;
- no timing/audio-engine/FM/harmonic feature scope creep;
- full target must retain >=16 KiB RAM and pool headroom.

## Approved work order after WP-C

### WP-D - Timing model

Main sequence exposes `1 / 2 / 4 / 8 / 16 BAR / FREE`, default `1 BAR`; Loop Cycle sync/free, Slot/Chain hold length and external sync remain separate concepts.

### WP-E - Audio character

Extreme but bounded distortion range, Reverb V3 with perceptual A/B, ratchet envelope/timing parity and live parameter smoothing/no-dropout acceptance.

### WP-F - Undo Last Random

One-level pre-random snapshot after the core control/audio gates are stable.

### WP-G - Synth evolution

Preserve current engine as `ANALOG`, add curated `FM` under CPU/RAM budgets, independent `SOUND` randomization and realtime-safe per-step timbre locks/modulation.

### WP-H - Harmonic layer decision

Prototype and audition scale-aware `HARMONY` first, soft filtered `PAD` second, then SUB HARMONY / CHORD V2 only if useful. Current Chord remains regression-protected until a successor wins.

### WP-I - Instrument polish

Splash, HOME hierarchy, Key/Scale refinement, SEQ explanatory hierarchy/onboarding, consistent margins/truncation/value formatting and newcomer usability.

### WP-J - Final integration/performance gate

Run all control, randomizer, audio, visual, feature-flag, target, emulator/browser and performance gates before release acceptance.

## Product rule

Do not reduce functionality merely to make the interface simpler. Prefer one coherent control grammar over several page-specific grammars.
