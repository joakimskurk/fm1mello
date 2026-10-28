# ROADMAP

This file is the short handoff index. Detailed scope lives in `docs/NEXT_ITERATION_ROADMAP.md`; new chats start with `START_HERE.md`.

## Completed

### WP-A - Randomizer intensity + RANDOM skyline

Complete 2026-10-26. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; clean reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

### WP-B - Trust and reliability

Complete 2026-10-28. Final implementation/test checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; final target `40604214660`: PASS.

## Active implementation

### WP-C - Control Grammar v2

Goal: keep functionality/depth while reducing page-specific grammars.

#### C1 - Audit and deterministic pre-remap baseline: COMPLETE

Checkpoint `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.

#### C2 - REC cleanup: COMPLETE

Checkpoint `f6d1f81660dab8db1de6da4e4650d01482d1ced3`. REC is reserved for record/capture only; B0-B3 contextual verbs preserve former REC functions; SAMPLE navigation moved to ENV/SOUND.

#### C3 - EDIT cleanup: COMPLETE

Implementation/test checkpoint `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`.

Delivered:

- generic EDIT -> RANDOM removed;
- Turing EDIT=LOCK and Sample EDIT=per-step retained;
- STEP/SUB hidden EDIT+white-key actions moved to selected-position B4;
- DOT reset moved to B2;
- B5 provides non-EDIT RANDOM access;
- STEP SEL+black note-entry priority preserved;
- C2 REC contract preserved.

Exact generated C3 UI SHA-256: `c811c6ee8e5d6422a479452c65e0c4720d93d0b979cba3b4bcd1281f95a162da`.

Green evidence: EDIT `40685833340`, reconstruction `40685833285`, REC `40685833209`, action contract `40685833243`, rotary `40685833249`, display `40685833215`, live-edit `40685833246`, audit `40685833273`, target `40685833648`.

Target retained `21472 B` RAM headroom, `17696 B` pool headroom; package SHA-256 `6673f3688954809a6b5ce00ed73ba39666764ec2414d224e1f206689e1fe7a52`.

#### C4 - Contextual rail / hidden actions / MOD navigation: ACTIVE

- migrate RANDOM mask NONE/ALL off OCT;
- migrate GLOBAL RESET ALL off hidden OCT chord;
- remove LOOP K4 RESET in favor of contextual B2;
- make active black-key actions visibly identifiable;
- group actual Audio LFO, DOT and Turing/EVOLVE coherently under MOD/generative navigation;
- preserve SELECT, C2/C3 semantics and STEP manual-note exception;
- add deterministic feature-on/off coverage.

#### C5 - Authoritative mapping + full closeout

After C4 only: make one authoritative/testable mapping source drive behavior/rendered guidance where practical, then rerun the full WP-A/WP-B/audio/timing/emulator/browser/80 MHz/Chromium/target matrix. Set WP-D active and stop.

## Approved sequence after WP-C

- WP-D Timing model: main sequence `1 / 2 / 4 / 8 / 16 BAR / FREE`, default `1 BAR`.
- WP-E Audio character: extreme bounded distortion, Reverb V3, ratchet parity, live smoothing.
- WP-F Undo Last Random.
- WP-G ANALOG/FM + SOUND randomization + realtime-safe per-step timbre.
- WP-H harmonic layer decision: HARMONY first, PAD second.
- WP-I instrument polish/onboarding.
- WP-J final integration/performance gate.

## Product rule

**Keep the depth, reduce the number of grammars.** Do not remove useful functionality merely to simplify the interface.
