# START HERE - FM-1 Mello continuation

Use this file at the start of every new ChatGPT work session on `joakimskurk/fm1mello`.

## Repository

- Repo: `joakimskurk/fm1mello`
- Branch: `visual-port-home`
- Always verify current branch HEAD before editing.
- Repo documentation is source of truth; do not reconstruct direction from chat memory.
- Firmware is reconstructed by `ci/unpack_source.sh`; generated `mello/` is not ordinary committed source.

## Read in order

1. `START_HERE.md`
2. `PROJECT_STATE.md`
3. `ROADMAP.md`
4. `DECISIONS.md`
5. `CONTROL_GRAMMAR.md`
6. `TESTING.md`
7. `docs/NEXT_ITERATION_ROADMAP.md`
8. `docs/WP_D_TIMING_AUDIT.md`
9. `docs/WP_C_CONTROL_AUDIT.md`
10. `docs/PRODUCT_WORKSHOP_2026-10-25.md`
11. `docs/UI_VISUAL_LANGUAGE.md` when UI is touched.

## Completed work packages

- WP-A Randomizer intensity + RANDOM skyline: COMPLETE.
- WP-B Trust and reliability: COMPLETE.
- **WP-C Control Grammar v2: COMPLETE.**

Final WP-C implementation/test checkpoint: `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

WP-C subtask checkpoints:

- C1 audit: `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`;
- C2 REC cleanup: `f6d1f81660dab8db1de6da4e4650d01482d1ced3`;
- C3 EDIT cleanup: `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`;
- C4 contextual rail/MOD navigation: `84d42c9fa21b2a91db5fa1aff2340c6ef3289a60`;
- C5 authoritative action mapping + final regression lock: `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

## WP-C final result

- PLAY remains transport only.
- REC is reserved for actual recording/capture; no unrelated REC page actions remain.
- EDIT is limited to genuine edit/focus/lock semantics.
- SELECT remains functional-layer navigation.
- white keys remain direct positions/cells except explicit manual note entry.
- STEP `SEL+black key` manual-note entry retains musical priority.
- black keys provide explicit contextual actions in stable B0-B7 positions; inactive slots are inert.
- RANDOM uses B1 reroll, B3 confirmed clear, B6 NONE and B7 ALL; RANDOM OCT is inert.
- GLOBAL uses B1 TAP and B3 confirmed RESET ALL; hidden OCT reset is gone.
- LOOP uses contextual B2 reset; layer-3 K4 reset is gone.
- Audio LFO, DOT and EVOLVE/Turing form the MOD/generative navigation family; FX opens AUDIO FX directly.
- handler semantics and rendered contextual guidance now consume one authoritative `MelloContextAction` page/slot descriptor.
- C5 parity covers every current page and B0-B7 slot: Chord enabled `16 pages / 31 active / 97 inactive`; Chord compiled out `15 / 29 / 91`.

Exact generated C5 locks:

- `mello/mello_ui.c`: `f6d26e0d1a57a51b08bf65e878144f5d48bfa248244d97b80c7a0842eb601818`;
- `tools/control_authoritative_mapping.c`: `bf688e15845324e1982336830f20c067118d90923f8ba6da0b692c94dac4914d`;
- retained `tools/control_context_contract.c`: `b5479562eb9a4aca5817663e835c04151e0c3337e4c9b131cb4c675daeabddfd`.

Final WP-C green workflow matrix on `2c280184...`:

- authoritative actions `40703570244`;
- reconstruction `40703570315`;
- context grammar `40703570196`;
- REC grammar `40703570212`;
- EDIT grammar `40703570277`;
- control-action contract `40703570204`;
- control audit `40703570330`;
- rotary parity `40703570466`;
- display integrity `40703570254`;
- live-edit `40703570316`;
- full target `40703571071`.

Full target retained WP-A/WP-B/audio/timing/emulator/browser/80 MHz/Chromium gates. RAM headroom is `21472 B`, pool headroom `17696 B`; package SHA-256 is `755a098ecbe66a6f8f7697a5787cb240e50a69df2520dc2f10b690f0056b8da1`. See `TESTING.md` for exact measurements.

## Active work package

**WP-D - Timing model.**

Do not begin WP-E early.

### WP-D approved product direction

- main sequence default becomes **`2 BAR`**;
- expose explicit `1 / 2 / 4 / 8 / 16 BAR / FREE` main-sequence reset choices;
- BAR is a musical duration and must remain the selected number of bars across sequence pulse-division changes;
- main-sequence bar reset is distinct from external/MIDI sync;
- Loop Cycle `SYNC / FREE` remains a separate concept;
- Slot/Chain hold length remains separate;
- names and UI must not collapse these timing concepts into one generic `SYNC` control.

### WP-D current position

D1 audit has identified the legacy main reset as `master_len` measured in processed pulses, with `0` meaning natural/free. External transport sync, Loop Cycle sync and Slot/Chain hold are already separate runtime concepts. `tools/wp_d_timing_baseline.c` and `docs/WP_D_TIMING_AUDIT.md` are the permanent D1 baseline; D1 CI closeout is the immediate gate before D2.

### WP-D procedure

1. Close D1 by running the permanent timing baseline and legacy-language separation gate in CI.
2. D2: implement the smallest engine/storage migration to `1 / 2 / 4 / 8 / 16 BAR / FREE`, default **2 BAR**, without changing Loop Cycle/external sync/hold semantics.
3. D3: expose timing choices with unambiguous language while preserving control grammar/parity.
4. D4: rerun all WP-A/WP-B/WP-C/audio/performance/target gates and document closeout.

### Do not implement inside WP-D

- distortion expansion or Reverb V3;
- Undo Last Random;
- ANALOG/FM engine expansion;
- SOUND randomization or per-step timbre;
- harmonic-layer replacement;
- general onboarding/polish beyond timing-language guidance required by WP-D.

## Stop condition

When WP-D is fully green, record exact implementation checkpoint/run evidence, set WP-E Audio character active, then stop/handoff instead of beginning WP-E automatically.

**Product rule: Keep the depth, reduce the number of grammars.**
