# PROJECT STATE

Last product review: 2026-10-25

## Repository

- Repo: `joakimskurk/fm1mello`
- Active branch: `visual-port-home`
- Product-code checkpoint before the 2026-10-25 documentation review: `1c0748c2140e106e0c38fa0637b4c06139c2625f`
- Documentation handoff HEAD before this review: `d2b7be19f36ab1595384c09bc2998f1857234ebd`
- Always resolve current branch HEAD before editing.

## Current roadmap position

The authoritative detailed roadmap is `docs/NEXT_ITERATION_ROADMAP.md`.

Current implementation item remains **Step 4 - Randomizer intensity**.

The 2026-10-25 hands-on audition broadened the planning backlog but did not authorize skipping Step 4 or implementing all later ideas at once. Finish Step 4 first unless a P0 reliability defect prevents meaningful testing.

## Product status after the 2026-10-25 audition

The instrument is already musically useful and capable of producing compelling loops. The main product problem is not lack of features. It is that several subsystems use different interaction grammars and some audio/UI paths are not yet trustworthy enough under live editing.

The detailed review and simulated workshop are in `docs/PRODUCT_WORKSHOP_2026-10-25.md`.

### P0 trust/reliability issues

- Audio can freeze while parameters are being turned; STOP/PLAY often recovers it.
- Some rendered R1-R4 labels do not match the rotary that actually changes the parameter.
- Text clipping/overlap still occurs on some screens.
- A static pink line can appear over `SINGLE` on HOME.

These are stabilization blockers before major synth expansion.

### Control grammar debt

- REC currently performs unrelated page-specific actions such as randomize, tap tempo and reset.
- EDIT is overloaded as both edit state and modifier for unrelated/destructive operations.
- LFO navigation is conceptually buried under the sound/envelope area.
- DOT matrices are modulation/generative modulators and should be evaluated as part of a coherent MOD family.
- Black keys are underused and should be prototyped as a visible contextual action rail, subject to product-owner approval.

### Timing model finding from original HTML

The browser prototype separates:

1. Main-sequence `RESET AFTER`: off / 1 / 2 / 4 / 8 / 16 bars, used to force the melody back to step 1 against fixed-length loops.
2. Loop Cycle `SYNC / FREE`: a separate alignment rule for the Loop Cycle counter.

The FM-1 port must keep these as separate concepts. New product requirement from audition: main sequence should use a bar-locked musical default while retaining FREE/off as an explicit option. Exact default bar length is pending product-owner sign-off.

### Reverb status changed

Reverb V2 remains the technically validated baseline, but it is **not product-accepted** after the latest audition. Metallic/ring-mod-like character is still audible and the result is not sufficiently hi-fi/diffuse. Preserve V2 regression measurements, but reopen the perceptual DSP task as Reverb V3 after the current randomizer/reliability gates.

### Ratchet status

Original HTML retriggers the synth envelope for every ratchet sub-trigger. Ratchets being difficult to hear should therefore first be treated as a parity/timing/envelope-retrigger defect, not as a reason to redesign the feature.

### Chord status changed

Chord Machine remains technically regression-protected but its product purpose is now explicitly unresolved. Do not remove it and do not invest in incremental polishing yet. Future design spike must compare at least:

- soft filtered PAD;
- scale-aware HARMONY derived from lead/sub context;
- SUB HARMONY;
- improved CHORD V2 with filter/envelope/voicing;
- compile-out/default-hidden option if none earns its space.

### Synth evolution direction

Desired future exploration after stabilization:

- preserve current synth as an `ANALOG` mode;
- add a curated, immediate `FM` mode appropriate to FM-1;
- add synth tone as an independent randomizer target;
- support per-step timbre locks/modulation;
- do not expose operator complexity unless it proves musically useful on the hardware.

## Randomizer remains core

The randomizer is a primary creative instrument. Step 4 still requires:

- `LOW`: conservative variation around current material;
- `MED`: preserve existing/default randomizer character as closely as practical;
- `HIGH`: larger structural/timbral movement while remaining musical;
- intensity changes target-specific mutation depth/span, not the selected target mask;
- `WAVE` remains independently selectable;
- compact full 4..32-step overview remains visible;
- RANDOM skyline must be visually denser and make at least 16 steps comfortably legible while still showing 17..32 without display paging.

## Work order from here

1. Finish Step 4 Randomizer intensity and RANDOM overview refinement.
2. Run immediate trust/reliability gate: audio-freeze reproduction, rotary-label parity, clipping/overlap and HOME artifact.
3. Resolve Control Grammar v2 and timing model.
4. Audio-character pass: distortion range, Reverb V3, ratchet parity and live parameter smoothing.
5. Add one-level Undo Last Random.
6. Only then expand synth architecture into ANALOG/FM, SOUND randomization and per-step timbre.
7. Run harmonic-layer design spike before deciding the fate of Chord Machine.
8. Final visual/onboarding polish including splash screen and HOME/SEQ/Key-Scale refinement.
9. Full integration/performance gate.

Do not treat this list as permission to implement later work packages before their dependencies are green.

## Current technical state

`ci/randomizer_roadmap_probe.py` is still a diagnostic probe. It deliberately ends with:

`raise SystemExit('RANDOMIZER ROADMAP PROBE V2 COMPLETE')`

Therefore the known source-reconstruction CI failure at product-code checkpoint `1c0748c2140e106e0c38fa0637b4c06139c2625f` is intentional diagnostic state, not evidence of a firmware regression.

Known run:

- Workflow run: `39925230733`
- Workflow: `Reconstructed source gate`
- Result: `failure`
- Reason: intentional probe exit after printing findings.

## Source reconstruction model

Firmware source is reconstructed by `ci/unpack_source.sh` from the source archive/chunks plus the ordered patch chain. New functional work must follow that patch/test architecture. Do not assume reconstructed source is committed as ordinary editable files.

## First action in a new chat

1. Fetch current `visual-port-home` HEAD.
2. Read `PROJECT_STATE.md`, `ROADMAP.md`, `DECISIONS.md`, `CONTROL_GRAMMAR.md`, `TESTING.md`, `docs/NEXT_ITERATION_ROADMAP.md` and `docs/PRODUCT_WORKSHOP_2026-10-25.md`.
3. Continue the currently active work package only.
4. For Step 4, read `ci/randomizer_roadmap_probe.py`, the probe output, `ci/unpack_source.sh`, `ci/build_target.sh` and analogous patch/test scripts before editing.
5. Remove/disable the intentional probe failure when its findings have been converted into real Step 4 tests.
6. Never claim a work package complete without exact commit and CI/run evidence.

## Definition of done for Step 4

Step 4 is not complete until:

- LOW/MED/HIGH exists in engine behavior and FM-1 UI;
- MED preserves the current/default creative behavior as closely as practical;
- intensity never changes the selected target mask;
- WAVE can be randomized independently;
- values remain legal at every sequence length 4..32;
- deterministic/reproducible tests exist where the RNG architecture permits it;
- RANDOM shows the whole active sequence with useful density at 4..32 steps;
- redraw performance remains within the established baseline;
- existing chord/reverb technical regressions and target build remain green;
- the intentional diagnostic probe failure is gone from the real gate;
- project/testing docs record exact commit and run information.
