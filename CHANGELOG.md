# CHANGELOG

## 2026-10-25 - post-audition product review

Documentation/planning checkpoint after extended hands-on emulator testing. No firmware implementation was changed in this checkpoint.

Added:

- `docs/PRODUCT_WORKSHOP_2026-10-25.md`

Updated:

- `PROJECT_STATE.md`
- `ROADMAP.md`
- `DECISIONS.md`
- `CONTROL_GRAMMAR.md`
- `TESTING.md`
- `docs/NEXT_ITERATION_ROADMAP.md`
- `docs/UI_VISUAL_LANGUAGE.md`

Important status changes:

- Step 4 Randomizer intensity remains the current implementation work package.
- Reverb V2 keeps its technical regression baseline but product acceptance is reopened after metallic/ring-mod-like audition feedback.
- Chord Machine remains regression-protected, but its final musical purpose is now explicitly open for a PAD/HARMONY/SUB HARMONY/CHORD V2 design spike.
- P0 reliability backlog now includes live-edit audio freeze, rotary-label/handler mismatch, text overlap/clipping and the HOME pink-line artifact.
- Original HTML timing semantics were re-checked: main-sequence bar reset and Loop Cycle SYNC/FREE are separate concepts and must remain separate in the port.
- Ratchet reference behavior was re-checked: the HTML retriggers the synth envelope for each ratchet sub-trigger.
- Future product direction documents ANALOG/FM engine exploration, independent SOUND randomization and per-step timbre only after stabilization gates.
- Splash screen is now a final-polish requirement.

The workshop document contains product-owner checkboxes for proposed control-grammar and harmonic-layer decisions. Those proposals are not treated as implemented or approved until explicitly selected.

## 2026-10-23 - handoff checkpoint for new chat

Documentation-only handoff prepared for continuation of the same FM-1 Mello project.

Added root-level continuity files:

- `PROJECT_STATE.md`
- `ROADMAP.md`
- `DECISIONS.md`
- `TESTING.md`
- `CHANGELOG.md`

No firmware/product implementation was changed by this handoff.

The product-code checkpoint immediately before the handoff is:

`1c0748c2140e106e0c38fa0637b4c06139c2625f`

Current work remains **Step 4 - Randomizer intensity**. The current source-reconstruction CI failure is intentionally caused by `ci/randomizer_roadmap_probe.py` exiting after diagnostic output and must not be mistaken for a functional regression.

Next chat should begin by reading the root continuity files plus `docs/NEXT_ITERATION_ROADMAP.md`, then inspect the existing probe output and reconstruction pipeline before implementing LOW/MED/HIGH.
