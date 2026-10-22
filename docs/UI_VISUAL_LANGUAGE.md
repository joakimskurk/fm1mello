# SkurkyMello FM-1 visual language

This document locks the visual semantics for the 240×240 FM-1 port. Colour is functional navigation, not decoration: the same colour should communicate the same kind of musical information across pages.

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

## Segment identity

Segments A–F have stable structural colours independent of the functional palette. Segment colours recur on HOME, STEP and SEGMENT views. They show where a note belongs in the sequence; functional markers layered on top still retain their own semantic colour.

## HOME skyline

HOME is the primary performance view and should be recognizable as SkurkyMello without relying on text.

- Each pitch column is filled with the colour of its segment.
- Pitch height is the main silhouette; event markers remain secondary.
- Playhead uses a white outline, while the selected edit step uses a softer neutral outline.
- Ratchet is magenta, accent coral, sub yellow/gold and slide blue.
- Pitch display scaling follows the selected pitch span and only expands when notes exceed it, avoiding unstable visual exaggeration after randomization.
- Segment brackets remain as structural reinforcement rather than the sole segment indicator.
- The compact header carries `SKURKYMELLO`, current page, segment context and RUN/STOP/BPM without stealing sequence height.

## Randomizer

Randomization is a core creative function, not a utility page. Purple identifies generative/random behaviour throughout the interface. The original selective target model must remain intact: PITCH, PUL, RAT, GATE, OCT, ACC, SUB, SLD, SKIP, PROB and SAMP, including ALL/NONE shortcuts.

## SUB 5-SEQ naming

The former user-facing `X-SUB` terminology is deprecated. The compact follower sequence is displayed as `SUB 5-SEQ` / `5-SEQ`, while internal `x_*` identifiers may remain unchanged to avoid needless engine risk.

## Rendering constraints

Visual refinement must not trade away FM-1 performance. Static structure is drawn on page/structural changes; moving playheads and selected items use narrow dirty-region redraws. HOME playhead regression tests enforce the existing partial-redraw budget.
