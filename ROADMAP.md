# ROADMAP

This file is the short handoff index. Detailed scope lives in `docs/NEXT_ITERATION_ROADMAP.md`; new chats start with `START_HERE.md`.

## Completed

### WP-A - Randomizer intensity + RANDOM skyline

Complete 2026-10-26. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; clean reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

### WP-B - Trust and reliability

Complete 2026-10-28. Final implementation/test checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; final target `40604214660`: PASS. B1 live-edit, B2 rotary parity and B3 display integrity remain regression-protected.

## Active implementation

### WP-C - Control Grammar v2

Goal: keep functionality/depth while reducing page-specific grammars.

#### C1 - Audit and deterministic pre-remap baseline: COMPLETE

Checkpoint `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.

Delivered full REC/EDIT/SELECT/black-key/OCT/ALGORITHM/PRESETS/page-action mapping, Chord on/off fixtures and documented migration debt in `docs/WP_C_CONTROL_AUDIT.md`.

#### C2 - REC cleanup: COMPLETE

Checkpoint `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.

REC is now reserved for actual recording/capture only. Legacy REC functions were preserved on stable contextual slots: B0 state, B1 trigger, B2 reset, B3 confirmed destructive. SAMPLE navigation moved to ENV/SOUND. STEP manual black-key note entry remains protected.

Green evidence: audit `40671589758`, action contract `40671589741`, REC grammar `40671589777`, rotary `40671589763`, display `40671589734`, live-edit `40671589787`, reconstruction `40671589813`, target `40671590524`.

Target retained `21472 B` RAM headroom, `17696 B` pool headroom and package SHA-256 `cb09e296517f2f699fdf2b926e2baba7de033b52b498be765850cf5709615c12`.

#### C3 - EDIT cleanup: ACTIVE

- remove generic EDIT -> RANDOM navigation without making RANDOM inaccessible;
- retain only true edit/focus/lock meanings (Turing LOCK, Sample per-step state);
- migrate STEP SKIP and SUB step-enable hidden EDIT+key modifiers to visible contextual actions;
- classify/migrate DOT reset if it is not true edit/focus/lock;
- preserve C2 REC contract and manual note-entry exception;
- add deterministic current EDIT contract with Chord enabled/compiled out.

#### C4 - Contextual rail / remaining hidden actions / MOD navigation

After C3 only: migrate RANDOM OCT ALL/NONE, GLOBAL RESET ALL chord and LOOP K4 RESET; make black-key actions visibly guided; reorganize Audio LFO/DOT/Turing under coherent MOD navigation.

#### C5 - Authoritative mapping + full closeout

One authoritative/testable page-action contract should drive behavior/rendered guidance where practical. Then rerun WP-A, WP-B, audio/timing, emulator/browser, 80 MHz, Chromium and target gates. Set WP-D active and stop.

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
