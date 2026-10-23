# ROADMAP

This file is a short handoff index. The detailed stabilization roadmap remains authoritative in `docs/NEXT_ITERATION_ROADMAP.md`.

## Current sequence

1. Reverb overhaul - COMPLETE
2. Chord Machine final go/no-go - PAUSED
3. UI performance + universal sequence overview - COMPLETE
4. Randomizer intensity - CURRENT
5. Undo last random - NEXT AFTER STEP 4 ONLY
6. Final integration/polish gate

## Current work package: Step 4

Goal: add `LOW / MED / HIGH` randomizer intensity without weakening the existing creative behavior.

Constraints:

- Intensity controls mutation depth/span per target.
- It does not change which targets are selected.
- `MED` is the compatibility/default mode.
- `LOW` stays near the current material.
- `HIGH` moves further while remaining musical.
- `WAVE` is independent.
- Keep the universal 4..32-step overview.
- Do not expand scope into Step 5 or new sequencing features.

## Deferred

- Factory samples until audio files are supplied.
- Further oscillator anti-aliasing / Tone.js envelope refinement until current stabilization work is complete.
- Chord Machine product/design iteration until explicitly resumed.
- New large sequencing features until this roadmap is complete.
