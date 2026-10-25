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

### Functionality preservation

Do not remove working musical functionality merely to simplify the interface. Prefer reorganizing related functions under a coherent interaction grammar. Removal/default-hidden/compile-out decisions require explicit product-owner approval.

### User audition can reopen technical acceptance

A technically green feature is not product-complete if hands-on audition exposes a meaningful musical or interaction defect. Keep measurements/regressions, but reopen product acceptance when necessary.

This rule currently applies to Reverb V2: its technical baseline is preserved, but perceptual product acceptance is reopened because metallic/ring-mod-like character remains audible.

### Main-sequence timing model

The port must preserve the conceptual distinction found in the original HTML:

- main-sequence bar reset/free-running length;
- Loop Cycle `SYNC / FREE`;
- external transport/MIDI sync;
- Slot/Chain hold length.

These must not be presented as one generic `SYNC` concept.

Product requirement after audition: main sequence should default to a normal bar-locked musical length, with FREE available deliberately. Exact default bar count remains pending sign-off.

### Control mapping must have one source of truth

R1-R4 labels and the actual rotary handlers must not be separately maintained mappings that can drift. The implementation should derive display labels/value formatting and encoder action from the same page/layer mapping definition wherever practical.

### Splash screen

A splash/boot identity screen is required before final product-polish acceptance.

### Chord Machine

Chord Machine remains technically regression-protected but product direction is unresolved. Do not remove it and do not incrementally polish the current concept until a design spike compares PAD, scale-aware HARMONY, SUB HARMONY and CHORD V2. Compile-out behavior remains required.

### Synth evolution

The desired future direction is to preserve the current synth as an `ANALOG` mode and investigate a curated `FM` mode, SOUND randomization and per-step timbre. This expansion begins only after the reliability/control/audio stabilization gates are green.

## Proposed decisions awaiting product-owner sign-off

The following are strong workshop recommendations, not locked decisions yet:

- reserve REC for actual recording/capture rather than generic page-specific actions;
- stop using EDIT as a hidden modifier for unrelated/destructive commands;
- prototype the black keys as a contextual action rail with visible labels;
- group LFO and DOT under a common `MOD` concept;
- treat Turing as part of the generative/modulation family while retaining its distinct behavior;
- use 1 BAR as the initial main-sequence reset default;
- make PAD and scale-aware HARMONY the first two Chord-successor prototypes.

See `docs/PRODUCT_WORKSHOP_2026-10-25.md` for checkboxes and rationale.

## Sequence overview

HOME, STEP, SEGMENT and RANDOM keep a compact 4..32-step visual overview. Do not reintroduce visual paging. The 2026-10-25 audition adds a stronger legibility requirement: 16 steps must be comfortably readable, while 17..32 must still remain visible and show meaningful pulse/event information.

## Step boundaries

Finish and verify Step 4 before implementing later product expansion. A P0 reliability defect may be fixed earlier only if it blocks meaningful Step 4 testing. Undo Last Random stays separate from Step 4.

## Engineering process decisions

- GitHub repo state is the durable source of truth between chats.
- Read current branch HEAD and project docs before editing.
- Work in small verifiable packages.
- Do not claim completion without exact commit and CI evidence.
- Diagnostic probes may intentionally fail CI, but that state must be clearly documented and removed/disabled before declaring the real feature green.
- Perceptual audio acceptance needs listening criteria in addition to numerical DSP tests.
