# PROJECT STATE

Last handoff prepared: 2026-10-23

## Repository

- Repo: `joakimskurk/fm1mello`
- Active branch: `visual-port-home`
- Product-code checkpoint before this documentation handoff: `1c0748c2140e106e0c38fa0637b4c06139c2625f`
- That commit message is `Narrow randomizer probe to ranges and encoder layout`.
- Any commits after that checkpoint may be documentation-only handoff commits. Always resolve the current branch HEAD before editing.

## Current roadmap position

The authoritative stabilization roadmap is `docs/NEXT_ITERATION_ROADMAP.md`.

Current item: **Step 4 - Randomizer intensity**.

Completed/paused before Step 4:

1. Reverb overhaul - COMPLETE.
2. Chord Machine final go/no-go - PAUSED after technical acceptance. Keep regression coverage, do not develop further unless explicitly resumed.
3. UI performance + universal sequence overview - COMPLETE.
4. Randomizer intensity - CURRENT WORK.
5. Undo last random - NOT STARTED.
6. Final integration/polish gate - NOT STARTED.

## Critical product intent

The randomizer is a core creative feature of SkurkyMello. It must remain capable of producing inspiring musical starting points. Do not simplify it into generic probability-based mutation.

Step 4 requirement:

- `LOW`: conservative variation around the current pattern/current values.
- `MED`: preserve the existing/default randomizer range and creative character as closely as practical.
- `HIGH`: larger structural/timbral movement while remaining musically useful, not chaos.
- Intensity changes target-specific mutation depth/span, not the selected target mask.
- `WAVE` remains an independent selectable target.
- Keep the compact universal 4..32-step overview on the RANDOM page.
- Prefer exact existing RNG call order for `MED` where practical so the old creative behavior is not accidentally changed.

## Current technical state

`ci/randomizer_roadmap_probe.py` is still a diagnostic probe. It reads reconstructed files including:

- `mello/mello_engine.h`
- `mello/mello_engine.c`
- `mello/mello_ui.h`
- `mello/mello_ui.c`
- `mello/mello_storage.c`

The probe deliberately ends with:

`raise SystemExit('RANDOMIZER ROADMAP PROBE V2 COMPLETE')`

Therefore the current source-reconstruction CI failure is intentional diagnostic state, not evidence of a firmware regression.

Known diagnostic workflow:

- Run: `39925230733`
- Head SHA: `1c0748c2140e106e0c38fa0637b4c06139c2625f`
- Workflow: `Reconstructed source gate`
- Conclusion: `failure` because the probe intentionally exits non-zero after printing its findings.

## Source reconstruction model

Do not assume reconstructed firmware source is directly committed as ordinary source files. `ci/unpack_source.sh` rebuilds the working source and applies patch stages. Before adding Step 4, inspect that reconstruction pipeline and follow its existing patch/test architecture.

## First action in a new chat

Do not start by redesigning Step 4 from memory. Do this in order:

1. Fetch current `visual-port-home` HEAD.
2. Read `PROJECT_STATE.md`, `ROADMAP.md`, `DECISIONS.md`, `TESTING.md`, and `docs/NEXT_ITERATION_ROADMAP.md`.
3. Read `ci/randomizer_roadmap_probe.py`.
4. Read the existing workflow/job log produced by the probe and extract the actual randomizer targets, ranges, encoder/input handling, UI state fields, and storage layout.
5. Read `ci/unpack_source.sh`, `ci/build_target.sh`, and analogous existing patch scripts before writing a new patch.
6. Implement Step 4 only.
7. Remove or disable the intentional probe failure once its information has been incorporated into real tests.
8. Add deterministic host tests for `LOW/MED/HIGH`, including MED parity and target isolation.
9. Run the full relevant CI gate and repair failures before declaring Step 4 complete.
10. Stop after Step 4. Do not begin Step 5 unless explicitly requested.

## Definition of done for Step 4

Step 4 is not complete until all of these are true:

- LOW/MED/HIGH exists in firmware behavior and FM-1 UI.
- MED preserves current/default randomizer character as closely as practical.
- Intensity never changes the selected target mask.
- WAVE can be randomized independently.
- Values remain inside legal ranges for every supported sequence length 4..32.
- Deterministic/reproducible tests are present where the RNG architecture permits it.
- Existing chord, reverb, UI/performance and build regressions remain green.
- CI is green for the real implementation, not merely for a diagnostic probe.
- This project state and testing documentation are updated with exact commit/run information.
