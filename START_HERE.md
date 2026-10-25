# START HERE - FM-1 Mello continuation

Use this file at the start of every new ChatGPT work session on `joakimskurk/fm1mello`.

## Repository

- Repo: `joakimskurk/fm1mello`
- Branch: `visual-port-home`
- Always fetch the current branch HEAD before editing.
- Do not reconstruct project direction from old chat memory when repo documentation is available.

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

## Active next work package

**WP-A / Step 4 - Randomizer intensity + RANDOM skyline refinement.**

Do this work package only.

### Goals

- add `LOW / MED / HIGH` randomizer intensity;
- preserve MED as closely as practical to the current/default creative behavior;
- make LOW a conservative variation around current material;
- make HIGH a broader but still musical jump;
- intensity changes mutation depth/range per target, not target selection;
- preserve independent `WAVE` target;
- keep all 4..32 active sequence steps visible;
- make 16-step RANDOM skyline comfortably readable and 17..32 still meaningful without display paging.

### Do not implement inside WP-A

- Undo Last Random;
- REC/EDIT remapping;
- black-key action rail;
- MOD menu/navigation redesign;
- main-sequence 1 BAR timing implementation;
- Reverb V3;
- distortion range expansion;
- ANALOG/FM engine work;
- SOUND randomization;
- per-step timbre;
- harmonic-layer redesign;
- splash screen.

Those directions are approved, but they belong to later work packages.

## Required source/workflow inspection before editing WP-A

Read:

- `ci/randomizer_roadmap_probe.py`
- `ci/unpack_source.sh`
- `ci/build_target.sh`
- analogous existing patch scripts in `ci/`
- relevant existing randomizer/UI host tests in `tools/`

The firmware source is reconstructed from chunks plus an ordered patch chain. Follow that architecture rather than assuming ordinary committed source files are the editable source of truth.

## Known CI condition

At product-code checkpoint `1c0748c2140e106e0c38fa0637b4c06139c2625f`, workflow run `39925230733` is intentionally red because `ci/randomizer_roadmap_probe.py` exits non-zero after printing diagnostics.

This is not a functional regression, but WP-A cannot be declared complete until that intentional failure is removed/disabled and replaced by real deterministic coverage.

## WP-A required tests

At minimum verify:

1. MED compatibility with previous behavior.
2. LOW/MED/HIGH do not alter selected target mask.
3. WAVE-only randomization is isolated.
4. LOW mutation depth is smaller than MED where applicable.
5. HIGH mutation depth is larger than MED where applicable without illegal values.
6. all mutated values remain legal.
7. sequence lengths 4..32 remain valid.
8. reproducibility for identical seed/state where possible.
9. full RANDOM overview remains visible at 4..32.
10. 16 steps are comfortably legible; 17..32 remain visible.
11. pulse/ratchet/event cues remain meaningful under compression.
12. incremental redraw/performance does not regress Step 3 baseline.
13. existing Chord/Reverb technical regression tests remain green.
14. target build and applicable emulator/browser/80 MHz/Chromium gates are green.

## Stop condition

When WP-A is fully green:

- record exact implementation commit SHA;
- record exact workflow run ID and relevant measured gates;
- update `PROJECT_STATE.md`, `ROADMAP.md`, `TESTING.md` and `CHANGELOG.md`;
- set **WP-B Trust and reliability** as the next active package;
- stop and prepare a new-chat handoff instead of beginning WP-B automatically.

## Approved future sequence after WP-A

1. WP-B Trust/reliability: audio freeze first, rotary parity, clipping/overlap, HOME artifact.
2. WP-C Control Grammar v2: REC/EDIT cleanup, contextual black keys, MOD family.
3. WP-D Timing: default 1 BAR, explicit FREE, keep Loop Cycle/Slot/external sync concepts separate.
4. WP-E Audio character: extreme distortion, Reverb V3, ratchet parity, live smoothing.
5. WP-F Undo Last Random.
6. WP-G ANALOG/FM + SOUND randomization + per-step timbre.
7. WP-H scale-aware HARMONY first, PAD second, then compare alternatives.
8. WP-I splash/HOME/Key-Scale/SEQ/onboarding polish.
9. WP-J final integration/performance gate.

## Non-negotiable product principle

**Keep the depth, reduce the number of grammars.**

Do not remove useful functionality just to make the interface simpler. Make related functions behave consistently and visibly instead.
