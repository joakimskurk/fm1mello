# TESTING

## Current known validation baseline

### Reverb V2

Accepted and positively auditioned in emulator.

Measured full target gate at the stabilized baseline:

- corr²: `0.00075`
- block-energy jump: `2.279`
- exact 120 ms pre-delay shift: `5292` samples
- 80 MHz chord/reverb stress: `1.833x` steady realtime
- target pool headroom: `17696 B`

### Chord Machine regression baseline

Technical acceptance completed before product pause.

Held `ARP_UP` path:

- `12` note-ons
- `12` note-offs
- `3` distinct pitches

Feature must continue to compile out with `MELLO_FEATURE_CHORD_MACHINE=0`.

### UI performance / sequence overview baseline

Step 3 completed in `00c6c72ff5a2e7fda1f6f95d0add7b00fa37f171`.

Measured host redraw gates:

- HOME incremental playhead: `9 fills / 1 text`
- STEP incremental playhead: `15 fills / 1 text`
- RANDOM incremental playhead: `9 fills / 1 text`
- Static FX page during transport: `1 fill / 1 text`

80 MHz SEQ/HOME benchmark:

- `3.023x` steady realtime
- p95 render: `2.859 ms`
- p99 render: `4.451 ms`
- max render: `7.061 ms`
- target RAM headroom: `21984 B`
- pool headroom: `17696 B`

## Current diagnostic CI state

At product-code checkpoint `1c0748c2140e106e0c38fa0637b4c06139c2625f`:

- Workflow run: `39925230733`
- Workflow: `Reconstructed source gate`
- Result: `failure`
- Reason: `ci/randomizer_roadmap_probe.py` intentionally terminates with non-zero `SystemExit` after printing diagnostic source context.

Do not treat this as a functional regression. Do not declare Step 4 complete while this intentional diagnostic failure remains in the real gate.

## Required Step 4 tests

Before Step 4 can be marked complete, add/verify tests for:

1. `MED` compatibility with the previous randomizer behavior. Prefer deterministic old-path vs MED golden vectors/reference output where the RNG design permits it.
2. Same selected target mask under LOW, MED and HIGH.
3. `WAVE` isolated randomization does not mutate unrelated targets.
4. LOW mutation depth is smaller than MED for applicable continuous/discrete targets.
5. HIGH mutation depth is larger than MED for applicable targets without illegal values.
6. Every mutated field stays in its legal range.
7. Sequence lengths `4..32` remain valid.
8. Reproducibility for identical seed/state where seed control exists.
9. RANDOM UI retains full compact sequence overview and does not regress Step 3 redraw behavior.
10. Host smoke, reconstructed target build, emulator/browser path, 80 MHz stress and Chromium E2E remain green as applicable to the existing CI pipeline.
11. Existing reverb and Chord Machine regression coverage remains green.

## Test reporting rule

When completing a work package, record:

- exact commit SHA
- exact workflow run ID
- pass/fail conclusion
- relevant measured gates
- any intentionally skipped hardware-only validation

The user does not currently have a physical FM-1, so emulator/web/host/target-build validation is the primary available path until hardware arrives.
