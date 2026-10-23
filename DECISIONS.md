# DECISIONS

## Product and scope decisions

### Randomizer is core

The randomizer is a central creative function, not a secondary utility. Architecture, UI and testing decisions must preserve its ability to generate inspiring musical starting points.

### Step 4 intensity semantics

Use three musical depth modes:

- `LOW`: conservative changes around current material.
- `MED`: current/default behavior as closely as practical.
- `HIGH`: broader structural and timbral movement while staying useful.

Intensity is not a global probability knob. It changes target-specific mutation ranges/depth.

### MED is the compatibility anchor

Where practical, preserve the original randomizer code path and RNG consumption order for `MED`. If exact parity is impossible, document the difference explicitly and prove that the creative range has not been weakened.

### Target mask is independent

Changing intensity must never silently enable, disable, add or remove random targets. `WAVE` remains a standalone independent target.

### Sequence overview

The RANDOM page keeps the shared compact 4..32-step visual overview established in Step 3. Do not reintroduce visual paging merely to add intensity.

### Chord Machine

Chord Machine is technically accepted but product-paused. Preserve regression tests and compile-out behavior. Do not spend further design/development effort unless explicitly resumed.

### Step boundaries

Finish and verify Step 4 before Step 5. Do not implement Undo Last Random as part of Step 4, but avoid architecture choices that make the one-level pre-random snapshot unnecessarily difficult.

## Engineering process decisions

- GitHub repo state is the durable source of truth between chats.
- Read current branch HEAD and project docs before editing.
- Work in small verifiable packages.
- Do not claim completion without exact commit and CI evidence.
- Diagnostic probes may intentionally fail CI, but that state must be clearly documented and removed/disabled before declaring the real feature green.
