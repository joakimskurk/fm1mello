# ROADMAP

This file is the short handoff index. Detailed scope and acceptance criteria live in `docs/NEXT_ITERATION_ROADMAP.md`. The 2026-10-25 product review is in `docs/PRODUCT_WORKSHOP_2026-10-25.md`.

## Current work

### Step 4 - Randomizer intensity - CURRENT

Goal: add `LOW / MED / HIGH` without weakening the existing creative behavior.

Also refine the RANDOM overview so 4..32 active steps remain visible with enough density to read pitch/pulse/event structure.

Hard constraints:

- intensity changes target-specific mutation depth/span;
- intensity never changes the selected target mask;
- `MED` is the compatibility/default behavior;
- `LOW` stays close to current material;
- `HIGH` moves further but remains musical;
- `WAVE` remains independently selectable;
- do not implement later feature expansion inside Step 4.

## Next stabilization gates

### Gate A - Trust and reliability

- reproduce/fix audio freeze during live parameter changes;
- guarantee R1-R4 screen labels match actual rotary handlers;
- remove text clipping/overlap;
- remove HOME pink-line artifact;
- extend automated mapping/text-fit coverage.

### Gate B - Control grammar + timing

- resolve REC and EDIT overloading;
- prototype black-key contextual actions;
- clarify SELECT/OCT behavior;
- evaluate LFO + DOT as a coherent MOD family;
- restore explicit main-sequence bar-reset/FREE model;
- keep Loop Cycle SYNC/FREE separate;
- clarify Slot/Chain hold-length semantics.

### Gate C - Audio character

- extend distortion into an intentionally extreme but gain-managed range;
- reopen reverb as Reverb V3 with perceptual quality as a hard gate;
- verify ratchet envelope retrigger/timing parity with original HTML;
- add parameter smoothing/no-dropout acceptance under live editing.

## Then

1. Undo Last Random: one-level pre-random snapshot.
2. Synth evolution: preserve current engine as `ANALOG`, design curated `FM`, add SOUND randomization and per-step timbre.
3. Harmonic-layer design spike: PAD vs scale-aware HARMONY vs SUB HARMONY vs CHORD V2 vs default-hidden/compile-out.
4. Instrument polish: splash screen, slicker HOME, Key/Scale refinement, SEQ explanation/onboarding.
5. Final full integration/performance gate.

## Product rule

Do not reduce functionality merely to make the interface simpler. Prefer one coherent control grammar over several page-specific grammars.

## Deferred until dependencies are green

- Factory samples until source audio is supplied.
- Major new synthesis/sequence scope before the trust and control gates.
- Chord implementation work before the harmonic-layer design decision.
