# Mello hardware control grammar

This contract keeps the FM-1 interface predictable and hardware-first.

## Current status

The 2026-10-25 product audition found real grammar drift: REC and EDIT have unrelated page-specific meanings, and some R1-R4 screen labels do not match the rotary that actually edits the parameter. The existing rules below remain valid, but the implementation now needs a Control Grammar v2 stabilization pass.

No new page is considered interaction-complete until the physical control, rendered label and actual handler agree.

## Global invariants

### PLAY belongs to transport

PLAY toggles transport. Do not overload PLAY with a page-specific edit action.

### Position belongs to the keys

Where a page represents steps, cells, bits, slots, chain positions or other discrete positions, the white keys select those positions directly. The keyboard surface mirrors the current step/cell domain unless a page explicitly requires manual note entry.

For domains longer than 16 positions, OCT DOWN/UP may change the edit bank. K1-K4 must not be consumed by bank/cursor navigation when the keys can represent that selection directly.

### SELECT belongs to function layers

SELECT changes the current page's functional layer/subpage. The selected step/cell must not move when SELECT changes layer.

Examples include STEP TRIG/NOTE/FLAGS/SEQ, SEGMENT PLAY/STRUCT, LOOP STEP/HARM/LOOP, TURING MAIN/ROUTE/DEPTH and SUB TRIG/5SEQ/STEP/VOICE/TONE.

### K1-K4 stay musical

K1-K4 edit sound, rhythm, harmony, timing or structural parameters. They must not be used to scroll a selected position when direct key selection exists.

### R1-R4 labels and handlers are one contract

A page/layer must have one authoritative mapping for each rotary containing at least:

- label;
- value formatter;
- legal range/step;
- handler/action;
- optional acceleration/coarse behavior.

The screen and input handler should consume the same mapping definition wherever practical. A state where the third displayed rotary label describes parameter 3 while physical rotary 3 changes parameter 4 is a hard failure.

### Always show the mapping

Dense pages must show compact K1-K4 labels and any contextual key actions. The user should not need to remember a hidden encoder mode.

All rendered UI text must fit its assigned box. No clipping/overlap is acceptable as normal operation.

## REC and EDIT: current debt

Current firmware uses REC for several unrelated actions including RANDOM reroll, GLOBAL tap tempo, Loop reset, DOT randomize and Turing reroll. RANDOM also uses `EDIT+REC` for clear. This is now documented interaction debt rather than an intentional long-term grammar.

### Proposed v2 direction, pending product-owner sign-off

- REC becomes a stable recording/capture verb.
- Page-specific commands such as REROLL, TAP, RESET and CLEAR move to visible contextual actions.
- EDIT changes edit state/focus/lock behavior only. It is not a hidden modifier for destructive unrelated commands.
- Destructive actions require explicit visible labeling and, when appropriate, confirmation/hold behavior.

Do not implement this proposal until sign-off and a complete page-action audit exist.

## Black-key contextual action rail: proposal

The black keys are underused and should be prototyped as a contextual action rail. Candidate uses include page-level actions such as REROLL, CLEAR, LOCK, RESET, SYNC/FREE or other verbs that are currently awkwardly attached to REC/EDIT.

Rules if approved:

- actions must be visible on screen;
- the same physical black key should prefer the same verb across pages when possible;
- destructive actions must be visually distinct;
- black-key actions must not break manual note entry modes;
- do not consume black keys merely because they are available.

Exact mappings are deliberately not locked yet.

## Modulation grouping: proposal

LFO and DOT matrices are both modulation sources. The current navigation places Audio LFO inside a sound/envelope-oriented path while DOT is separate. Control Grammar v2 should prototype a common `MOD` concept with separate source identities rather than pretending they are the same algorithm.

Turing may live in the same broader generative/modulation family but must retain its distinct evolving-register semantics.

## Loop Cycle reference

Loop Cycle uses eight direct key positions and three layers:

- STEP: HOLD, ROOT+, TRANS, OCT
- HARM: KEY, SCALE, SUB, BASS TR
- LOOP: LEN, SYNC/FREE, ON/OFF, RESET NOW

SELECT cycles STEP/HARM/LOOP. The first eight white keys select loop positions 1-8.

When Loop Cycle is OFF, pressing loop keys 1-8 also auditions that slot immediately by making its harmonic context current. When Loop Cycle is ON, keys 1-8 only select the slot being edited and must not jump the running playback position.

`BASS TR` is a per-cycle-step semitone transpose for the dedicated sub voice only, independent from the cycle lead/main `TRANS`. Range: -12..+12 semitones.

RESET NOW resets playback position without changing the selected loop step.

Important: Loop Cycle `SYNC/FREE` is not the same control as main-sequence bar reset/free-running, and neither is the same as external MIDI/transport sync.

## Required audit before Control Grammar v2 is complete

Every page/layer must be listed in a machine-readable or testable mapping table covering:

1. K1-K4 labels and handlers.
2. white-key behavior.
3. black-key behavior, if any.
4. SELECT behavior.
5. OCT DOWN/UP behavior.
6. REC behavior.
7. EDIT behavior.
8. destructive actions.
9. live-edit safety while transport is running.

## Regression gate

`tools/control_grammar_smoke.c` remains a hard CI gate. It must be expanded during the stabilization pass to catch rotary label/handler drift and the approved REC/EDIT/black-key grammar in addition to existing Loop Cycle behavior.
