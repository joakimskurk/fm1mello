# CHANGELOG

## 2026-10-25 - approved next-iteration handoff

Product owner approved every recommendation from the 2026-10-25 product workshop.

Documentation/handoff updates only; no firmware implementation changed in this approval checkpoint.

Added:

- `START_HERE.md` as the canonical entry point for new ChatGPT/work sessions.

Locked product decisions:

- main-sequence default = `1 BAR`, FREE remains explicit;
- REC reserved for actual recording/capture;
- EDIT reserved for edit/focus/lock;
- black keys approved as visible contextual action rail where useful;
- LFO + DOT grouped conceptually under `MOD`, with Turing in the broader generative/modulation family;
- Reverb V3 approved with perceptual listening as a hard gate;
- distortion approved for an intentionally extreme upper range with gain management;
- ratchet audibility handled as parity/timing/envelope retrigger first;
- current synth preserved as ANALOG with curated FM mode approved after stabilization;
- SOUND approved as future independent randomizer target;
- per-step timbre approved after stabilization;
- harmonic-layer spike starts with scale-aware HARMONY, then PAD;
- current Chord remains regression-protected until a successor wins;
- splash screen required before final polish acceptance.

Updated:

- `PROJECT_STATE.md`
- `ROADMAP.md`
- `DECISIONS.md`
- `CONTROL_GRAMMAR.md`
- `TESTING.md`
- `docs/NEXT_ITERATION_ROADMAP.md`
- `docs/PRODUCT_WORKSHOP_2026-10-25.md`
- `docs/UI_VISUAL_LANGUAGE.md`
- `README.md`

Active next implementation remains **WP-A / Step 4 - Randomizer intensity + RANDOM skyline refinement**.

After WP-A is verified and documented, stop and hand off to **WP-B Trust and reliability**, where live-edit audio freeze is the first blocker.

## 2026-10-25 - post-audition product review

Documentation/planning checkpoint after extended hands-on emulator testing. No firmware implementation was changed in this checkpoint.

Added:

- `docs/PRODUCT_WORKSHOP_2026-10-25.md`

Important status changes:

- Step 4 Randomizer intensity remained current implementation work package.
- Reverb V2 kept its technical regression baseline but product acceptance was reopened after metallic/ring-mod-like audition feedback.
- Chord Machine remained regression-protected while its final musical purpose was opened for design exploration.
- P0 reliability backlog added live-edit audio freeze, rotary-label/handler mismatch, text overlap/clipping and HOME pink-line artifact.
- Original HTML timing semantics were re-checked: main-sequence bar reset and Loop Cycle SYNC/FREE are separate concepts.
- Ratchet reference behavior was re-checked: HTML retriggers synth envelope for each ratchet sub-trigger.
- Future product direction documented ANALOG/FM exploration, SOUND randomization and per-step timbre after stabilization gates.

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

Current work remained **Step 4 - Randomizer intensity**. The source-reconstruction CI failure was intentionally caused by `ci/randomizer_roadmap_probe.py` exiting after diagnostic output and must not be mistaken for a functional regression.
