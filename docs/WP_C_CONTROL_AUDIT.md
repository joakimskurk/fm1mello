# WP-C C1 control-action audit

Checkpoint scope: **pre-remap baseline only**. This document records the reconstructed WP-B-final behavior that WP-C must migrate. It does not approve the legacy overloads.

The executable sources of truth are `tools/control_action_baseline.c` and `tools/control_encoder_baseline.c`; the dedicated workflow compiles both with Chord enabled and compiled out. `CONTROL_GRAMMAR.md` remains the normative target.

## Locked grammar target

- PLAY is transport only.
- REC is recording/capture only.
- EDIT is edit/focus/lock only.
- SELECT changes functional layers/subpages and does not move content selection.
- White keys own direct positions/cells except explicit manual note entry.
- Black keys form a contextual action rail only where useful; every active action must be visibly identified.
- OCT buttons remain position-bank navigation where >16 positions require it; unrelated page actions migrate away.
- K1-K4 remain musical/value controls rather than hidden page actions.
- Audio LFO, DOT and Turing/EVOLVE are organized coherently under the MOD/generative family.

## Current pre-remap page map

| Page | REC today | EDIT today | SELECT layers | Black keys today | C1 classification |
|---|---|---|---:|---|---|
| HOME | opens SAMPLE | opens RANDOM | 1 | none | REC/EDIT overload |
| STEP | opens SAMPLE | opens RANDOM; held with white key toggles SKIP | 4 | none; musical with SEL note-entry | REC/EDIT + hidden modifier debt |
| SEGMENT | opens SAMPLE | opens RANDOM | 2 | none | REC/EDIT overload |
| LOOP CYCLE | RESET NOW | opens RANDOM | 3 | none | REC/EDIT overload; layer 3 K4 is also RESET |
| SLOT SEQ | opens SAMPLE | opens RANDOM | 1 | none | REC/EDIT overload |
| RANDOM | reroll sequence; EDIT+REC = CLEAR | no-op on same page | 1 | none | REC overload + hidden destructive combo |
| KEY / SCALE | opens SAMPLE | opens RANDOM | 1 | none | REC/EDIT overload |
| SLOTS | opens SAMPLE | opens RANDOM | 2 | none | REC/EDIT overload |
| DOT | reroll cells | toggle DOT reset | 4 targets | none | REC/EDIT overload; MOD placement debt |
| TURING / EVOLVE | reroll register | toggle LOCK | 3 | none | REC overload; EDIT LOCK is valid |
| SUB | toggle SUB on/off | opens RANDOM; held with white key toggles SUB-step on/off | 5 | none | REC/EDIT + hidden modifier debt |
| CHORD | toggle CHORD on/off | opens RANDOM | 5 | none | REC/EDIT overload (feature-on build) |
| GLOBAL | TAP tempo | opens RANDOM | 2 | none | REC/EDIT overload |
| SOUND | opens SAMPLE | opens RANDOM | 5 | none | REC/EDIT overload; Audio LFO is layer 2 |
| AUDIO FX | opens SAMPLE | opens RANDOM | 5 | none | REC/EDIT overload |
| SAMPLE | toggle sample engine on/off | toggle per-step edit mode | 4 | none | REC overload; EDIT state is valid |

With Chord enabled the baseline therefore contains 16 non-record REC meanings and 13 EDIT meanings outside the final edit/focus/lock contract. With Chord compiled out those counts are 15 and 12.

## Other action debt confirmed by C1

### Hidden modifiers / destructive actions

- STEP: hold EDIT + white key toggles `SKIP`.
- SUB: hold EDIT + white key toggles that SUB 5-SEQ step on/off.
- RANDOM: hold EDIT + press REC performs destructive `CLEAR`.
- GLOBAL: OCT DOWN + OCT UP arms `RESET ALL`; repeating the chord within the confirmation window executes it.
- LOOP CYCLE layer 3: K4 is a momentary `RESET` page action rather than a musical/value edit.

These functions are preserved during migration but must become visible contextual actions. Destructive actions retain explicit confirmation/hold treatment.

### OCT

Allowed position-navigation uses remain:

- HOME / STEP bank 1–16 vs 17–32;
- SLOT SEQ bank navigation;
- SLOTS bank navigation;
- TURING 1–16 vs 17–32.

Migration debt:

- RANDOM OCT DOWN = randomize mask `NONE`;
- RANDOM OCT UP = randomize mask `ALL`.

### SELECT / ALGORITHM / PRESETS encoders

The reconstructed encoder path confirms three separate facts:

- `SELECT` is the correct function-layer control and does not move selected content.
- `ALGORITHM` is currently a byte-for-byte semantic duplicate of `SELECT`: it calls the same layer dispatcher on every layered page. This is redundant grammar, not hidden RANDOM functionality.
- `PRESETS` always moves the global `selected_slot` cursor and derives `slot_bank` from it, even when the current page does not render that cursor. It does not change engine/audio state and is not a hidden RANDOM mask/action path.

C1 keeps these facts explicit so later cleanup cannot accidentally confuse a redundant layer control with a page action.

### MOD family

The current physical/navigation grammar is internally inconsistent:

- physical `LFO` opens `TURING / EVOLVE`; pressing it again there toggles Turing on/off;
- the actual Audio LFO lives under `SOUND` layer 2 (`AUDIO LFO` / footer `LFO RATE DEPTH TARGET`);
- physical `FX` opens `DOT`; from DOT the same button opens `AUDIO FX`.

WP-C must reorganize these without changing DSP behavior.

## SELECT contract confirmed

`MELLO_UI_ENC_SELECT` delegates to the existing function-layer dispatcher. C1 verifies that it advances the current function layer/target where a page has multiple layers and does not move the page's selected step/cell/slot/pattern.

Current layer counts used as the executable baseline:

- STEP 4
- SEGMENT 2
- LOOP 3
- SLOTS 2
- DOT 4 targets
- TURING 3
- SUB 5
- CHORD 5 when enabled
- GLOBAL 2
- SOUND 5
- AUDIO FX 5
- SAMPLE 4
- HOME, SLOT SEQ, RANDOM and KEY/SCALE: no SELECT layer cycle

The companion encoder fixture verifies that ALGORITHM currently mirrors the same layer movement on all of these pages.

## Black-key baseline

Outside manual STEP note entry, black keys are currently inert. That is intentional as a C1 starting point: WP-C can add only the actions that are useful and can keep stable verb positions rather than inheriting legacy behavior.

The explicit exception is STEP manual note entry (`SEL` held), where chromatic black keys remain musical input and must never be interpreted as contextual commands.

## Migration order from this baseline

1. **C2:** remove every unrelated REC meaning while preserving those functions through the contextual-action mechanism.
2. **C3:** remove EDIT navigation/unrelated toggles; retain true edit/focus/lock semantics and migrate hidden STEP/SUB actions.
3. **C4:** establish visible contextual black-key actions, destructive confirmation, migrate LOOP K4 RESET, and make MOD navigation coherent.
4. **C5:** make the resulting action map authoritative for handlers and rendered guidance, then run the full regression/target matrix.

No timing-model, audio-character, FM, random-undo, harmonic-layer or onboarding work belongs in this audit or migration.
