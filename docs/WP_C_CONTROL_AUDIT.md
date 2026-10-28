# WP-C control-action migration record

This document records the deterministic pre-remap baseline and the control migrations performed inside WP-C. `CONTROL_GRAMMAR.md` remains normative.

## C1 - pre-remap baseline: COMPLETE

Checkpoint: `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.

Historical executable baseline is retained in `tools/control_action_pre_remap.c`; encoder debt remains covered by `tools/control_encoder_baseline.c`.

Pre-remap counts:

- Chord enabled: 16 pages, 16 non-record REC meanings, 13 EDIT meanings outside the final edit/focus/lock contract, no contextual black-key actions;
- Chord compiled out: 15 / 15 / 12 / 0.

### C1 debt map

| Page | REC before C2 | EDIT before C3 | Other debt |
|---|---|---|---|
| HOME | opens SAMPLE | opens RANDOM | - |
| STEP | opens SAMPLE | opens RANDOM; held+white toggles SKIP | SEL+black key is protected manual note input |
| SEGMENT | opens SAMPLE | opens RANDOM | - |
| LOOP | RESET | opens RANDOM | layer 3 K4 also RESET |
| SLOT SEQ | opens SAMPLE | opens RANDOM | - |
| RANDOM | reroll; EDIT+REC CLEAR | no-op | OCT DOWN/UP = mask NONE/ALL |
| KEY/SCALE | opens SAMPLE | opens RANDOM | - |
| SLOTS | opens SAMPLE | opens RANDOM | - |
| DOT | reroll | reset toggle | physical FX routes here |
| TURING/EVOLVE | reroll | LOCK | physical LFO routes here/toggles Turing |
| SUB | on/off | opens RANDOM; held+white toggles step on/off | - |
| CHORD | on/off | opens RANDOM | feature-on only |
| GLOBAL | TAP | opens RANDOM | hidden OCT pair RESET ALL |
| SOUND | opens SAMPLE | opens RANDOM | actual Audio LFO is layer 2 |
| AUDIO FX | opens SAMPLE | opens RANDOM | - |
| SAMPLE | on/off | per-step edit state | - |

### SELECT / ALGORITHM / PRESETS

- SELECT is correct function-layer navigation and does not move selected content.
- ALGORITHM duplicates SELECT layer movement on layered pages.
- PRESETS moves the global selected-slot cursor / slot bank without changing engine/audio state.

### Black-key baseline

Before C2, black keys were inert outside the explicit STEP manual-note exception with SEL held.

## C2 - REC cleanup: COMPLETE

Implementation/test checkpoint: `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.

REC is now reserved for actual recording/capture. Since no capture path exists yet, it is intentionally inert rather than overloaded.

Stable contextual verb positions introduced by C2:

- **B0** = state toggle;
- **B1** = trigger / reroll / tap;
- **B2** = reset;
- **B3** = destructive action with confirmation.

C2 mappings:

| Page | Contextual action after C2 |
|---|---|
| RANDOM | B1 reroll; B3 two-press confirmed CLEAR |
| GLOBAL | B1 TAP |
| LOOP | B2 reset |
| DOT | B1 reroll |
| TURING | B1 reroll |
| SUB | B0 on/off |
| CHORD | B0 on/off |
| SAMPLE | B0 on/off |

SAMPLE navigation moved from REC to the ENV/SOUND family: ENV opens SOUND; ENV again from SOUND opens SAMPLE; ENV from SAMPLE returns SOUND.

STEP SEL+black-key remains chromatic manual note entry and takes precedence over contextual actions.

WP-A's randomizer UI smoke was migrated from REC to RANDOM B1 while preserving the same deterministic HIGH comparison plus LOW/MED/HIGH, WAVE, 4..32 skyline and redraw assertions.

Exact generated outputs after C2:

- UI SHA-256 `3da137c8faa5800bcfddd8291176e77718bc320d890f20f9f9d0c9cba4aaffb0`;
- randomizer smoke SHA-256 `71fc32a27fedfb0509ab0ff9eccfc85ba53d90c9b6d6bf48f0915f15c1d5fa13`.

C2 evidence:

- control audit `40671589758`: PASS;
- action contract `40671589741`: PASS;
- REC grammar `40671589777`: PASS;
- rotary parity `40671589763`: PASS;
- display integrity `40671589734`: PASS;
- live-edit `40671589787`: PASS;
- reconstruction `40671589813`: PASS;
- target `40671590524`: PASS.

## C3 - EDIT cleanup: ACTIVE

C3 must preserve C2 exactly while changing only EDIT semantics and the actions directly dependent on EDIT.

Required migration:

- generic EDIT -> RANDOM navigation must disappear;
- RANDOM must remain accessible through a coherent non-EDIT route;
- Turing EDIT=LOCK remains valid;
- Sample EDIT=per-step edit state remains valid;
- STEP held EDIT+white-key SKIP moves to a contextual action on the selected step;
- SUB held EDIT+white-key step-enable moves to a contextual action on the selected SUB step;
- DOT EDIT reset toggle must be classified: retain only if it is genuinely an edit/focus/lock state, otherwise migrate to contextual action;
- SEL+black-key STEP manual note entry remains protected and musical.

C4 retains responsibility for RANDOM OCT ALL/NONE, GLOBAL RESET ALL, LOOP K4 RESET, final visible rail guidance and MOD navigation cleanup.
