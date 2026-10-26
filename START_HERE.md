# START HERE - FM-1 Mello continuation

Use this file at the start of every new ChatGPT work session on `joakimskurk/fm1mello`.

## Repository

- Repo: `joakimskurk/fm1mello`
- Branch: `visual-port-home`
- Always fetch and verify the current branch HEAD before editing.
- Do not reconstruct project direction from old chat memory when repo documentation is available.
- Follow the active work package below and do not jump ahead.

## Read these files in this order

1. `START_HERE.md`
2. `PROJECT_STATE.md`
3. `ROADMAP.md`
4. `DECISIONS.md`
5. `CONTROL_GRAMMAR.md`
6. `TESTING.md`
7. `docs/NEXT_ITERATION_ROADMAP.md`
8. `docs/PRODUCT_WORKSHOP_2026-10-25.md`
9. `docs/UI_VISUAL_LANGUAGE.md` when UI is touched.

## Last completed work package

**WP-A / Step 4 - Randomizer intensity + RANDOM skyline refinement: COMPLETE.**

Implementation commit: `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`.
Clean reconstruction closeout: `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

Verified outcomes:

- LOW/MED/HIGH implemented;
- MED preserves the old/default path directly;
- target masks are unchanged by intensity;
- WAVE remains independently selectable;
- RANDOM shows the full 4..32-step skyline without paging;
- no storage migration;
- host, timing, chord/reverb, emulator/browser, 80 MHz and target-build gates are green;
- the obsolete diagnostic probe invocation and temporary reconstruction tracing have been removed.

Do not reopen WP-A unless a real regression is demonstrated.

## Active next work package

**WP-B - Trust and reliability.**

Do this work package only.

### Work order

1. Reproduce and fix the live-edit audio freeze reported during parameter turning.
2. Guarantee R1-R4 rendered labels/values match the actual rotary handlers.
3. Remove text clipping and overlap at extreme values.
4. Remove the static pink-line artifact reported over `SINGLE` on HOME.
5. Extend automated mapping, text-fit and stale-region coverage so those defects cannot silently return.

### WP-B principles

- trust/reliability before new synthesis scope;
- reproduce before redesigning;
- preserve currently green audio, randomizer, timing, chord, feature-flag, UI-performance and target gates;
- normal live rotary movement must not allocate/rebuild DSP in an unbounded way;
- one authoritative mapping source should drive both displayed R1-R4 semantics and handlers where practical;
- display fixes must be covered by bounds/dirty-region tests, not screenshot judgement only.

### Do not implement inside WP-B

- Control Grammar v2 / REC-EDIT remapping;
- contextual black-key action rail;
- MOD navigation redesign;
- main-sequence 1 BAR timing model;
- distortion expansion;
- Reverb V3;
- Undo Last Random;
- ANALOG/FM engine work;
- SOUND randomization;
- per-step timbre;
- harmonic-layer redesign;
- splash/onboarding polish.

Those directions are approved but belong to later work packages.

## Required inspection before editing WP-B

At minimum inspect:

- `ci/unpack_source.sh`
- `ci/build_target.sh`
- `TESTING.md`
- relevant audio parameter/state code in the reconstructed source;
- UI rotary label/handler paths;
- existing `tools/` visual/control/performance tests;
- `docs/PRODUCT_WORKSHOP_2026-10-25.md` for the reported defects and product context.

The firmware source is reconstructed from chunks plus an ordered patch chain. Follow that architecture rather than treating generated `mello/` files as ordinary committed source.

## WP-B required acceptance

Before WP-B can close:

1. deterministic/stress reproduction exists for live parameter edits while transport/audio run;
2. no audio freeze/silent state and no STOP/PLAY recovery is required;
3. transport phase remains valid through live edits;
4. R1-R4 display/handler parity is validated across page/layer mappings;
5. text fit and overlap pass at extreme formatted values;
6. HOME stale/pink-line artifact is eliminated and regression-covered;
7. existing WP-A LOW/MED/HIGH + skyline gates remain green;
8. Chord/Reverb technical regressions remain green;
9. 300 BPM timing, emulator/browser, 80 MHz and Chromium paths remain green;
10. full target build remains green with >=16 KiB RAM and pool headroom;
11. project docs record exact commit/run evidence.

## Stop condition

When WP-B is fully green:

- record exact implementation/closeout commit SHA;
- record workflow run IDs and measured gates;
- update `PROJECT_STATE.md`, `ROADMAP.md`, `TESTING.md` and `CHANGELOG.md`;
- set **WP-C Control Grammar v2** as the next active package;
- stop and hand off instead of beginning WP-C automatically.

## Approved future sequence after WP-B

1. WP-C Control Grammar v2: REC/EDIT cleanup, contextual black keys, MOD family.
2. WP-D Timing: default 1 BAR, explicit FREE, keep Loop Cycle/Slot/external sync concepts separate.
3. WP-E Audio character: extreme distortion, Reverb V3, ratchet parity, live smoothing.
4. WP-F Undo Last Random.
5. WP-G ANALOG/FM + SOUND randomization + per-step timbre.
6. WP-H scale-aware HARMONY first, PAD second, then compare alternatives.
7. WP-I splash/HOME/Key-Scale/SEQ/onboarding polish.
8. WP-J final integration/performance gate.

## Non-negotiable product principle

**Keep the depth, reduce the number of grammars.**

Do not remove useful functionality merely to make the interface simpler. Make related functions behave consistently and visibly instead.
