# Mello hardware control grammar

This contract keeps the FM-1 interface predictable and hardware-first.

## Current status

The 2026-10-25 product audition found real grammar drift: REC and EDIT have unrelated page-specific meanings, and some R1-R4 screen labels do not match the rotary that actually edits the parameter.

The product owner approved the Control Grammar v2 direction on 2026-10-25. The rules below are now implementation requirements.

No page is interaction-complete until the physical control, rendered label and actual handler agree.

## The rules (G1-G8, 2026-11-03)

One set of rules every page is checked against, from the rules decided 2026-10-31 (ROADMAP C2) and what playing
0.5-f46 showed. ROADMAP "Next: everything open, in one order" says which wave applies each; the status of each is
here.

| | Rule | Status |
|---|---|---|
| G1 | **B0 switches what the page plays.** On every page with a part or a lane B0 is its ON: LOOP CYCLE, SLOT SEQ, DOT (the picked matrix: off, its route keeps moving and nothing is sent), EVOLVE, BASS, CHORD, SAMPLE, DRUMS. Pages without one (SOUND, FX, KEY, GLOBAL, MIX, RANDOM, SLOTS, HOME, STEP, SEGMENT) keep their own first key. | Applied 0.5-f47 |
| G2 | **A part's first page is enough to hear it.** What turns a part on and makes it play is on its first layer: BASS's B1 is SEQ, the bass sequencer (a white key turns a BASS SEQ step on or off). | Applied 0.5-f47 |
| G3 | **REC records the part on screen**, on every page with a sequence (HOME and STEP the lead, BASS the bass line, CHORD or SUB MELLO the chain, DRUMS the drums: white keys 1-4 KICK, SNARE, HAT, GLITCH, the next keys the hit's locks). Stopped: step by step; playing: on the clock; one UNDO; the white keys a keyboard in the key now, OCT- / OCT+ moving it. HOME's EDIT > REC stays. | Decided; wave 3 |
| G4 | **Performance in one place.** B5 is PERF: tapped, the PERFORM page and 16 actions on the white keys until PERF or a page button; held, while held. Every action acts on every part. B6 is MUTATE on every page; B7 and B8 go to the pages. | Applied 0.5-f52 |
| G5 | **The lights.** Lit = now, active or held; glow = set or there; dark = nothing; on the white and black keys, on every page. | Wave 4 |
| G6 | **K1-K4.** A voice's first layer is K1 CUT, K2 RES, K3 DECAY or ENV, K4 LEVEL (the sampler: LEVEL first). | Wave 4 |
| G7 | **The screen.** One line on top (path, key now, count); the picture; K1-K4; the black keys' strip at the bottom; the readout over K1-K4 and the strip. | Chosen; wave 4 |
| G8 | **A rule is a test.** control_authoritative_mapping (verify_g1_g2 and the page-by-page strips), control_context_contract, rotary_mapping_parity and ui_glossary hold the applied rules. | Applied with each rule |

## Global invariants

### PLAY belongs to transport

PLAY toggles transport. Do not overload PLAY with a page-specific edit action.

### REC belongs to recording/capture

REC is reserved for actual recording/capture semantics.

REC must not mean REROLL, TAP, RESET, CLEAR, randomize-now or any other generic page action.

Existing page-specific REC behavior is legacy debt and must be migrated during Control Grammar v2 without silently removing functionality.

### EDIT belongs to edit state/focus/lock

EDIT may change edit focus, edit state or lock behavior.

EDIT must not be a hidden modifier for unrelated or destructive commands. In particular, legacy combinations such as `EDIT+REC CLEAR` are migration targets, not accepted final grammar.

### Position belongs to the white keys

Where a page represents steps, cells, bits, slots, chain positions or other discrete positions, the white keys select those positions directly. The keyboard surface mirrors the current step/cell domain unless a page explicitly requires manual note entry.

For domains longer than 16 positions, OCT DOWN/UP may change the edit bank. K1-K4 must not be consumed by bank/cursor navigation when the keys can represent that selection directly.

### Black keys are the contextual action rail

The black keys are approved as the contextual action surface where a page needs explicit verbs such as REROLL, CLEAR, LOCK, RESET or other non-position actions.

Rules:

- every active action must be visible on screen;
- prefer the same physical key for the same verb across pages;
- destructive actions must be visually distinct and use suitable hold/confirmation behavior;
- manual note-entry modes retain musical black-key behavior when required;
- do not assign black-key actions merely to fill every key;
- migrating a legacy REC/EDIT action must preserve its musical function unless a later explicit product decision removes it.

Exact page-by-page mapping is determined from the complete audit before implementation is considered complete.

### SELECT belongs to function layers

SELECT changes the current page's functional layer/subpage. The selected step/cell must not move when SELECT changes layer.

Examples include STEP TRIG/NOTE/FLAGS/SEQ, LOOP STEP/HARM/LOOP, TURING MAIN/ROUTE/DEPTH and SUB TRIG/SEQ/STEP/VOICE/TONE/FILTER.

### K1-K4 stay musical

K1-K4 edit sound, rhythm, harmony, timing or structural parameters. They must not be used to scroll a selected position when direct key selection exists.

### R1-R4 labels and handlers are one contract

A page/layer has one authoritative mapping for each rotary containing at least:

- label;
- value formatter;
- legal range/step;
- handler/action;
- optional acceleration/coarse behavior.

The screen and input handler should consume the same mapping definition wherever practical. A state where physical rotary 3 changes a parameter described under R4 is a hard failure.

### Always show the mapping

Dense pages show compact rotary labels and contextual key actions. The user should not need to remember a hidden mode or modifier.

All rendered UI text must fit its assigned box. No clipping/overlap is acceptable as normal operation.

## MOD family

LFO, DOT and Turing are now one conceptual modulation/generative family, while remaining distinct sources.

- `LFO`: continuous cyclic modulation with waveform/rate/depth/target.
- `DOT`: discrete matrix modulation with run mode/length/reset/target.
- `TURING`: evolving register/generative modulation.

Navigation and visual grouping should make their relationship discoverable without pretending that their timing or algorithms are identical.

The current placement of Audio LFO under a sound/envelope-oriented route is migration debt.

## Timing language

Never use one generic `SYNC` label for all timing concepts.

The interface must distinguish:

1. external clock/MIDI sync;
2. main-sequence `BAR RESET / FREE`;
3. Loop Cycle `SYNC / FREE`;
4. Slot/Chain hold length.

Main-sequence default is `1 BAR`. FREE is an explicit deliberate mode.

## Loop Cycle reference

Loop Cycle uses eight direct key positions and three layers:

- STEP: HOLD, ROOT+, TRANS, OCT
- HARM: KEY, SCALE, SUB, BASS TR
- LOOP: LEN, SYNC/FREE, ON/OFF, RESET NOW

SELECT cycles STEP/HARM/LOOP. The first eight white keys select loop positions 1-8.

When Loop Cycle is OFF, pressing loop keys 1-8 also auditions that slot immediately by making its harmonic context current. When Loop Cycle is ON, keys 1-8 only select the slot being edited and must not jump the running playback position.

`BASS TR` is a per-cycle-step semitone transpose for the dedicated sub voice only, independent from cycle lead/main `TRANS`. Range: -12..+12 semitones. Both land on the key's nearest tone in their direction (CHROMATIC excepted); `ROOT+` moves the key itself (DECISIONS "One key that always holds").

RESET NOW resets playback position without changing the selected loop step.

Loop Cycle `SYNC/FREE` is not the same control as main-sequence bar reset/free-running, and neither is the same as external MIDI/transport sync.

## Required page-action audit

Before Control Grammar v2 implementation begins, enumerate every page/layer in a machine-readable or directly testable mapping table covering:

1. R1-R4 labels, formatters, ranges and handlers.
2. white-key behavior.
3. black-key contextual actions.
4. SELECT behavior.
5. OCT DOWN/UP behavior.
6. REC behavior.
7. EDIT behavior.
8. destructive actions and confirmation rules.
9. live-edit safety while transport is running.
10. manual note-entry exceptions.

This table becomes the implementation source of truth and regression fixture, not merely documentation.

## Regression gate

`tools/control_grammar_smoke.c` remains a hard CI gate. It must be expanded during Control Grammar v2 to catch:

- rotary label/handler drift;
- REC semantics;
- EDIT semantics;
- contextual black-key actions;
- timing-language separation;
- existing Loop Cycle behavior;
- feature-flag/compile-out regressions where applicable.
