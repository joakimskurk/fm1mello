# ROADMAP

This is the short handoff index. Detailed scope lives in `docs/NEXT_ITERATION_ROADMAP.md`; new chats start with `START_HERE.md`.

## Completed

### WP-A - Randomizer intensity + RANDOM skyline

Complete 2026-10-26. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; clean reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

### WP-B - Trust and reliability

Complete 2026-10-28. Final implementation/test checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; full target `40604214660`: PASS.

## Active implementation

### WP-C - Control Grammar v2

Goal: keep functionality/depth while reducing page-specific grammars.

#### C1 - Audit and deterministic baseline: COMPLETE

Checkpoint `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.

#### C2 - REC cleanup: COMPLETE

Checkpoint `f6d1f81660dab8db1de6da4e4650d01482d1ced3`. REC is record/capture only; former REC actions moved to contextual verbs.

#### C3 - EDIT cleanup: COMPLETE

Checkpoint `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`. EDIT is edit/focus/lock only; STEP/SUB selected-position toggles moved to B4, DOT reset to B2, RANDOM access to B5.

#### C4 - Contextual rail / hidden actions / MOD navigation: COMPLETE

Implementation/test checkpoint `84d42c9fa21b2a91db5fa1aff2340c6ef3289a60`.

Delivered:

- RANDOM NONE/ALL -> B6/B7; RANDOM OCT inert;
- GLOBAL hidden OCT reset -> confirmed B3 reset;
- LOOP K4 reset removed; B2 reset retained;
- active black-key actions rendered and inactive slots inert;
- MOD navigation cycles Audio LFO -> DOT -> EVOLVE without toggling Turing;
- FX opens AUDIO FX directly;
- C2/C3/SELECT/manual-note contracts preserved.

Exact generated C4 UI SHA: `a3829d2fdc354154021727838b4b17cf8420832f88565a2b2f290b17a06002c5`.

Green evidence: reconstruction `40701004129`, context `40701004213`, REC `40701004034`, EDIT `40701004030`, action `40701004015`, audit `40701004022`, rotary `40701004133`, display `40701004059`, live-edit `40701004057`, target `40701005084`.

Target retained `21472 B` RAM headroom and `17696 B` pool headroom; package SHA-256 `9747bed2ac5c974aed8b0cf9f340f395c5d566b879e5632162de65b75d9d2864`.

#### C5 - Authoritative mapping + full WP-C closeout: ACTIVE

- centralize contextual handler semantics + rendered action guidance in one authoritative/testable descriptor where practical;
- preserve B0-B7 physical positions, confirmation semantics, inactive slots, C1-C4 behavior and feature-flag behavior;
- add page/action parity coverage with Chord enabled and compiled out;
- rerun full WP-A/WP-B/WP-C/audio/timing/emulator/browser/80 MHz/Chromium/target matrix;
- require >=16 KiB RAM and pool headroom;
- close WP-C, set WP-D active, then stop/handoff.

## Approved sequence after WP-C

- WP-D Timing model: `1 / 2 / 4 / 8 / 16 BAR / FREE`, default `1 BAR`.
- WP-E Audio character: extreme bounded distortion, Reverb V3, ratchet parity, live smoothing.
- WP-F Undo Last Random.
- WP-G ANALOG/FM + SOUND randomization + realtime-safe per-step timbre.
- WP-H harmonic layer decision: HARMONY first, PAD second.
- WP-I instrument polish/onboarding.
- WP-J final integration/performance gate.

**Product rule: Keep the depth, reduce the number of grammars.**
