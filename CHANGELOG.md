# CHANGELOG

## 2026-10-26 - WP-A randomizer intensity + RANDOM skyline complete

WP-A / Step 4 is complete and handed off. No WP-B implementation is included in this checkpoint.

Implemented:

- `LOW / MED / HIGH` randomizer intensity;
- MED preserves the previous/default randomizer implementation directly;
- LOW stays closer to current material and HIGH produces broader legal variation;
- intensity changes mutation depth/span without changing selected target masks;
- WAVE remains independently selectable and isolated;
- intensity is runtime UI state, so no project/storage migration was introduced;
- RANDOM page exposes intensity through K1;
- RANDOM skyline retains the full active 4..32-step sequence without paging, with pitch/event/playhead information under compression.

Validation:

- implementation commit: `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`;
- clean reconstruction closeout: `42cf506be5b127e7f03c763966d1e75cc4a48a85`;
- reconstruction run `40294234032`: PASS;
- target closeout run `40294234527`;
- measured randomizer distance baseline: `LOW=194824 < MED=516917 < HIGH=1082367`;
- RANDOM incremental redraw: `4 fills / 0 texts`;
- 4..32 skyline, WAVE isolation, legal-range and deterministic coverage: PASS;
- 300 BPM timing, Chord/Reverb regressions, emulator/browser/Chromium and 80 MHz gates: PASS;
- RAM headroom `21984 B`; pool headroom `17696 B`, both above the 16 KiB floor;
- product-identical package SHA-256: `fcd1fbf7624b50995f90267f4e168fe2a72328a8f7eeeb46c4982632cd999df0`.

Closeout repair:

- the temporary red reconstruction was traced to the old `randomizer_roadmap_probe.py` still being invoked and deliberately exiting non-zero;
- the WP-A patch itself was valid;
- the obsolete probe invocation was removed;
- temporary reconstruction tracing/artifact export was removed;
- correct WP-A base64/gzip integrity hashes plus exact source pre/post hashes now lock reconstruction.

Next active package: **WP-B Trust and reliability**, beginning with live-edit audio freeze. Stop here; WP-B is not started by this closeout.

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

Active next implementation remained **WP-A / Step 4 - Randomizer intensity + RANDOM skyline refinement**.

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
