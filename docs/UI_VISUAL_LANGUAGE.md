# SkurkyMello FM-1 visual language

This document locks the visual semantics for the 240x240 FM-1 port. Colour is functional navigation, not decoration: the same colour should communicate the same kind of musical information across pages.

The 2026-10-25 hands-on review adds a stronger rule: visual density is allowed, ambiguity is not. Advanced functionality may stay deep, but current state and physical control mapping must remain readable without prior knowledge of the browser prototype.

## Functional colour language

- Green: main sequence, pitch and melodic material.
- Cyan: filter-specific information.
- Blue: modulation, evolving movement, traversal, slide and timing motion.
- Magenta: ratchets, repeats and rhythmic multiplication.
- Yellow/gold: sub-bass and sub-oriented sequencing.
- Coral/orange: accent and musical emphasis.
- Purple: randomizer and generative behaviour.
- Red: destructive/reset/warning states only.
- White/light neutral: playhead and edit-selection state, so selection never competes with function colours.

## Display integrity is a hard gate

No normal operating state may contain:

- clipped labels or values;
- overlapping text and graphics;
- stale dirty-region pixels;
- decorative lines that can be mistaken for state;
- a control label that does not match the physical control action.

Extreme formatted values and every SELECT layer must be included in visual smoke coverage.

The reported static pink line over `SINGLE` on HOME is a bug, not an accepted visual element.

## Segment identity

Segments A-F have stable structural colours independent of the functional palette. Segment colours recur on HOME, STEP and SEGMENT views. Functional markers layered on top retain their own semantic colour.

## Full-sequence overview

Whenever a page visualizes the main sequencer, the complete active sequence remains visible at once. Sequence length is 4-32 steps, so overview columns become progressively thinner rather than hiding steps in off-screen visual banks.

The physical keys may still edit a 16-step bank, but display must make the whole sequence, playhead and current edit position legible.

HOME, STEP, SEGMENT and RANDOM share proportional geometry so the user does not relearn where a step lives from page to page.

### Density targets

- 4-8 steps: generous columns and full event detail.
- 9-16 steps: compact but comfortably readable columns.
- 17-32 steps: every step remains visible; event markers simplify rather than disappear.
- no visual paging for main overview.

## HOME skyline

HOME is the primary performance view and should be recognizable as SkurkyMello without relying on text.

- Pitch height is the main silhouette.
- Segment identity remains visible.
- Playhead uses a white outline; selected edit step uses softer neutral outline.
- Ratchet is magenta, accent coral, sub yellow/gold and slide blue.
- Pitch scaling follows selected pitch span and expands only when necessary.
- Segment brackets reinforce structure but are not the only segment cue.
- Compact header carries identity, page/context and transport/BPM without stealing sequence height.
- All active main-sequence steps remain visible up to 32.

HOME is functionally acceptable but not visually final. Later polish improves hierarchy and instrument identity without decorative clutter or increased redraw cost. Skyline remains the hero element.

## Randomizer

Randomization is a core creative function, not a utility page. Purple identifies generative/random behaviour throughout the interface.

The selective target model remains intact: PITCH, PUL, RAT, GATE, OCT, ACC, SUB, SLD, SKIP, PROB, SAMP and WAVE, plus CHD only while Chord Machine is compiled in. WAVE randomizes main oscillator waveform/PWM only and never selects OFF. ALL/NONE shortcuts remain available.

RANDOM carries a full-width miniature sequence/skyline so randomization never hides the musical result.

### RANDOM skyline refinement

Required for WP-A:

- at least 16 steps read immediately as a coherent skyline;
- 17-32 remain visible in the same view;
- pitch height remains legible after compression;
- pulse count and ratchet/activity cues still distinguish rhythmic structures;
- playhead remains unambiguous;
- LOW/MED/HIGH intensity state must not consume enough vertical space to weaken skyline;
- no visual banks/pages to solve density.

## MOD visual family - APPROVED

LFO, DOT and Turing belong to one conceptual modulation/generative family. Use blue as common family language while preserving source-specific accents where useful.

- LFO: continuous cyclic movement, waveform/rate/depth/target.
- DOT: discrete matrix modulation with run mode/length/reset/target.
- Turing: evolving register/generative modulation.

The grouping should make these sources feel related without hiding their different time models or algorithms.

## Contextual black-key actions - APPROVED

When a page uses the black-key action rail:

- every active action must be labeled on screen;
- action labels must sit close enough to the keyboard/action context to be understood immediately;
- destructive actions use red/warning language and suitable hold/confirmation feedback;
- unused black keys should look unused rather than implying hidden actions;
- manual note-entry modes must clearly indicate when black keys are musical notes instead of actions.

Do not create dense action legends that compete with the main musical visualization.

## REC / EDIT state language

REC is reserved for actual recording/capture. EDIT is reserved for edit/focus/lock.

The UI must not display REC/EDIT state in ways that imply page-specific REROLL, RESET, TAP or CLEAR semantics.

Contextual actions such as REROLL, RESET, LOCK and CLEAR should be named directly.

## Timing language

Avoid generic `SYNC` wording when actual concept is different.

Visually distinguish:

- external clock/MIDI sync;
- main-sequence `1/2/4/8/16 BAR / FREE`;
- Loop Cycle `SYNC / FREE`;
- Slot/Chain hold length.

Main-sequence default is `1 BAR`.

## Key / Scale

Key/Scale should communicate three questions immediately:

1. What is the root?
2. What scale is active?
3. What pitch/octave span will sequence editing/randomization use?

Keyboard geometry is useful only when it clarifies these answers. Do not spend display area on decorative piano rendering that does not improve selection or understanding.

## SEQ pages

SEQ pages need to explain their effect on running music, not only show parameter values.

Whenever practical, show:

- current position relative to full sequence;
- active length/hold in musical units;
- whether relevant subsystem is bar-locked/aligned/free-running;
- what advances/resets and when.

## Chord / harmonic layer

Current Chord Machine visual identity is not locked for final product. Scale-aware HARMONY is the first successor prototype, PAD second. Show musical relationship first, synthesis parameters second.

## Splash screen

A splash screen is required before final polish acceptance.

Constraints:

- strong FM-1 Mello / SkurkyMello identity;
- short and clean, not a blocking animation;
- no visual glitch during transition to HOME;
- must not delay audio initialization materially;
- avoid tiny version/legal text unless useful on-device.

## SUB 5-SEQ naming

Former user-facing `X-SUB` terminology remains deprecated. Display `SUB 5-SEQ` / `5-SEQ`; internal identifiers may remain unchanged to avoid engine risk.

## Rendering constraints

Visual refinement must not trade away FM-1 performance. Static structure is drawn on page/structural changes; moving playheads and selected items use narrow dirty-region redraws.

HOME/RANDOM playhead regression tests enforce existing partial-redraw budget. Any richer skyline, contextual hint system or splash transition must be measured on the 80 MHz target path rather than assumed cheap.
