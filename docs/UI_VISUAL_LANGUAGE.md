# SkurkyMello FM-1 visual language

This document locks the visual semantics for the 240x240 FM-1 port. Colour is functional navigation, not decoration: the same colour should communicate the same kind of musical information across pages.

The 2026-10-25 hands-on review adds a stronger rule: visual density is allowed, ambiguity is not. Advanced functionality may stay deep, but the current state and physical control mapping must remain readable without prior knowledge of the browser prototype.

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

Extreme formatted values and every SELECT layer must be included in the visual smoke suite.

The reported static pink line over `SINGLE` on HOME is a bug, not an accepted visual element.

## Segment identity

Segments A-F have stable structural colours independent of the functional palette. Segment colours recur on HOME, STEP and SEGMENT views. They show where a note belongs in the sequence; functional markers layered on top retain their own semantic colour.

## Full-sequence overview

When a page visualizes the main sequencer, the complete active sequence remains visible at once. Sequence length is 4-32 steps, so overview columns become progressively thinner instead of hiding steps in an off-screen visual bank.

The physical keys may still edit a 16-step bank, but the display must make the whole sequence, playhead and current edit position legible.

HOME, STEP, SEGMENT and RANDOM share proportional geometry so the user does not relearn where a step lives from page to page.

### Density targets

- 4-8 steps: generous columns and full event detail.
- 9-16 steps: compact but comfortably readable columns.
- 17-32 steps: every step remains visible; event markers simplify rather than disappear.
- no visual paging for the main overview.

## HOME skyline

HOME is the primary performance view and should be recognizable as SkurkyMello without relying on text.

- Each pitch column is filled with the colour of its segment.
- Pitch height is the main silhouette; event markers remain secondary.
- Playhead uses a white outline; selected edit step uses a softer neutral outline.
- Ratchet is magenta, accent coral, sub yellow/gold and slide blue.
- Pitch scaling follows selected pitch span and only expands when necessary.
- Segment brackets reinforce structure but are not the only segment cue.
- Compact header carries identity, page/context and transport/BPM without stealing sequence height.
- All active main-sequence steps remain visible up to 32.

### HOME polish target

The current HOME is functionally acceptable but not final. Future polish should improve hierarchy and instrument identity without adding decorative clutter or increasing redraw cost. The skyline remains the hero element.

## Randomizer

Randomization is a core creative function, not a utility page. Purple identifies generative/random behaviour throughout the interface.

The selective target model remains intact: PITCH, PUL, RAT, GATE, OCT, ACC, SUB, SLD, SKIP, PROB, SAMP and WAVE, plus CHD only while Chord Machine is compiled in. WAVE randomizes main oscillator waveform/PWM only and never selects OFF. ALL/NONE shortcuts remain available.

RANDOM carries a full-width miniature sequence/skyline so randomization never hides the musical result.

### RANDOM skyline refinement

The 2026-10-25 audition found that the current overview is technically complete but not dense/expressive enough.

Required refinement:

- at least 16 steps should read immediately as a coherent skyline;
- 17-32 remain visible in the same view;
- pitch height must remain legible after compression;
- pulse count and ratchet/activity cues must still distinguish different rhythmic structures;
- playhead must remain unambiguous;
- LOW/MED/HIGH intensity state must not consume enough vertical space to weaken the skyline;
- do not bring back visual banks/pages to solve density.

## Modulation visual family - proposed

If Control Grammar v2 approves a common `MOD` area, use blue as the common family colour while keeping source identities distinct:

- LFO: continuous cyclic movement, waveform/rate/depth/target.
- DOT: discrete matrix modulation with run mode/length/reset and target.
- Turing: evolving register/generative modulation.

The visual grouping should make these feel related without hiding their different time models.

## Key / Scale

Key/Scale is currently usable but should communicate three questions immediately:

1. What is the root?
2. What scale is active?
3. What pitch/octave span will sequence editing/randomization use?

Keyboard geometry is useful only when it clarifies these answers. Do not spend display area on decorative piano rendering that does not improve selection or understanding.

## SEQ pages

SEQ pages need to explain their effect on the running music, not only display parameter values.

Whenever practical, show:

- current position in relation to the full sequence;
- active length/hold in musical units;
- whether the page is aligned, bar-locked or free-running;
- what will advance/reset and when.

Avoid generic `SYNC` wording when the actual concept is main-sequence bar reset, Loop Cycle alignment, Slot/Chain hold or external MIDI clock.

## Chord / harmonic layer

Current Chord Machine visual identity is not locked for final product. Future PAD/HARMONY/SUB HARMONY/CHORD V2 prototypes should follow the same rule as the rest of Mello: show the musical relationship first, synthesis parameters second.

## Splash screen

A splash screen is required before final polish acceptance.

Constraints:

- strong FM-1 Mello / SkurkyMello identity;
- short and clean, not a blocking animation;
- no visual glitch during transition to HOME;
- must not delay audio initialization materially;
- avoid tiny version/legal text unless it is actually useful on-device.

## SUB 5-SEQ naming

The former user-facing `X-SUB` terminology remains deprecated. Display `SUB 5-SEQ` / `5-SEQ`; internal identifiers may remain unchanged to avoid engine risk.

## Rendering constraints

Visual refinement must not trade away FM-1 performance. Static structure is drawn on page/structural changes; moving playheads and selected items use narrow dirty-region redraws.

HOME/RANDOM playhead regression tests enforce the existing partial-redraw budget. Any new animation, splash transition or richer skyline must be measured on the 80 MHz target path rather than assumed cheap.
