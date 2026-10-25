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

## Locked decisions from the 2026-10-25 product workshop

The product owner approved all workshop recommendations on 2026-10-25. The following are now requirements, not proposals.

### Main-sequence timing

- Main sequence uses a bar-locked musical default.
- Default is `1 BAR`, meaning reset after 16 sixteenth-note ticks.
- `FREE` remains an explicit option for natural/polymetric sequence length.
- Main-sequence bar reset is distinct from Loop Cycle `SYNC / FREE`, Slot/Chain hold length and external MIDI/transport sync.

### REC and EDIT

- `REC` is reserved for actual recording/capture semantics.
- REC must not be the generic page-specific action button for REROLL, TAP, RESET or CLEAR.
- `EDIT` is reserved for edit focus/state/lock semantics.
- EDIT must not be a hidden modifier for unrelated or destructive actions.
- Page-specific actions move to explicit visible contextual actions.

### Black-key contextual action rail

The black keyboard keys will be prototyped and then standardized as a contextual action rail where useful.

Rules:

- every active black-key action is visible on screen;
- prefer stable verb placement across pages;
- destructive actions are visually distinct and require suitable hold/confirmation behavior;
- manual note-entry modes keep musical black-key behavior when required;
- do not consume a black key unless the page has a useful contextual action for it.

### Modulation family

- LFO and DOT are grouped conceptually under `MOD`.
- LFO remains a continuous cyclic modulation source.
- DOT remains a discrete matrix modulation source.
- Turing belongs to the broader generative/modulation family while retaining its evolving-register identity.
- Grouping must not flatten these into one algorithm or hide their different time models.

### Rotary mapping single source of truth

R1-R4 labels and actual rotary handlers must not be separately maintained mappings that can drift. Wherever practical, display label, formatter, legal range, acceleration and handler come from the same page/layer mapping definition.

### Reverb V3

Reverb is reopened as a product-quality task. Reverb V2 measurements remain regression evidence, but Reverb V3 must pass perceptual listening criteria in addition to objective DSP tests.

### Distortion range

Distortion gains an intentionally extreme upper range. The lower range must remain controllable, and gain compensation/headroom must prevent the feature from becoming only a loudness increase.

### Ratchet parity

Ratchet audibility is treated first as a parity/timing/envelope-retrigger task. The original HTML behavior, with envelope retrigger for every sub-trigger, is the reference before any musical redesign.

### Synth evolution

- Preserve the current engine as first-class `ANALOG` mode.
- Add a curated, immediate `FM` mode after stabilization gates are green.
- Do not expose generic operator complexity unless it proves musically useful on FM-1.
- Add `SOUND` as an independent randomizer target.
- Add per-step timbre locks/modulation after reliability/control/audio stabilization.
- Per-step timbre must be realtime-safe and must not allocate or rebuild audio objects on each step.

### Harmonic layer

Current Chord Machine remains regression-protected as a prototype until a successor wins by audition.

First prototypes to build and compare:

1. scale-aware `HARMONY` derived from lead/sub/key context;
2. soft filtered `PAD`.

Then compare as needed:

- `SUB HARMONY`;
- `CHORD V2` with filter/envelope/voicing;
- default-hidden/compile-out if no concept earns its space.

### Splash screen

A splash/boot identity screen is required before final product-polish acceptance.

## Sequence overview

HOME, STEP, SEGMENT and RANDOM keep a compact 4..32-step visual overview. Do not reintroduce visual paging. At 16 steps the overview must be comfortably readable, while 17..32 must still remain visible and retain meaningful pulse/event information.

## Step boundaries

Finish and verify Step 4 before implementing later product expansion. A P0 reliability defect may be fixed earlier only if it blocks meaningful Step 4 testing. Undo Last Random stays separate from Step 4.

## Engineering process decisions

- GitHub repo state is the durable source of truth between chats.
- Read current branch HEAD and project docs before editing.
- Work in small verifiable packages.
- Do not claim completion without exact commit and CI evidence.
- Diagnostic probes may intentionally fail CI, but that state must be clearly documented and removed/disabled before declaring the real feature green.
- Perceptual audio acceptance needs listening criteria in addition to numerical DSP tests.
- Do not implement several approved future work packages at once merely because their product direction is now locked.
