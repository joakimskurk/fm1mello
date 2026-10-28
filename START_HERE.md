# START HERE - FM-1 Mello continuation

Use this file at the start of every new ChatGPT work session on `joakimskurk/fm1mello`.

## Repository

- Repo: `joakimskurk/fm1mello`
- Branch: `visual-port-home`
- Always fetch and verify the current branch HEAD before editing.
- Repo documentation is source of truth; do not reconstruct direction from old chat memory.
- Follow the active subtask only. Do not jump ahead.
- Firmware is reconstructed by `ci/unpack_source.sh`; do not treat generated `mello/` as ordinary committed source.

## Read these files in this order

1. `START_HERE.md`
2. `PROJECT_STATE.md`
3. `ROADMAP.md`
4. `DECISIONS.md`
5. `CONTROL_GRAMMAR.md`
6. `TESTING.md`
7. `docs/NEXT_ITERATION_ROADMAP.md`
8. `docs/WP_C_CONTROL_AUDIT.md`
9. `docs/PRODUCT_WORKSHOP_2026-10-25.md`
10. `docs/UI_VISUAL_LANGUAGE.md` when UI is touched.

## Last completed work package

**WP-B - Trust and reliability: COMPLETE.**

Final implementation/test checkpoint: `7c20efbdc391c59118fef3f000689eeafaa176b9`.
Final full target run: `40604214660`: PASS.

Do not reopen WP-B without a demonstrated regression. Physical FM-1 live-edit reproduction remains a later hands-on check because hardware is not yet available.

## Active work package

**WP-C - Control Grammar v2.**

Product rule: **Keep the depth, reduce the number of grammars.**

Normative target:

- PLAY = transport only;
- REC = actual recording/capture only;
- EDIT = edit/focus/lock only;
- SELECT = function-layer navigation only;
- white keys own direct positions/cells except explicit manual note entry;
- black keys are a visible contextual action rail where useful;
- destructive actions require confirmation/hold treatment;
- K1-K4 remain musical/value controls rather than hidden page actions;
- LFO, DOT and Turing/EVOLVE must end in a coherent MOD/generative family;
- rendered guidance must match actual handlers.

### C1 - Complete action audit: COMPLETE

Verified baseline checkpoint: `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.

C1 established deterministic pre-remap coverage for REC, EDIT, SELECT, black keys, OCT, ALGORITHM/PRESETS and hidden page actions. See `docs/WP_C_CONTROL_AUDIT.md`.

Important C1 findings retained for later migration:

- STEP: `EDIT+white key` toggles SKIP;
- SUB: `EDIT+white key` toggles SUB step enable;
- RANDOM: legacy `EDIT+REC` CLEAR and OCT ALL/NONE;
- GLOBAL: hidden OCT chord RESET ALL;
- LOOP layer 3 K4 = RESET page action;
- physical LFO currently routes to Turing/EVOLVE while actual Audio LFO is SOUND layer 2;
- physical FX currently routes through DOT to AUDIO FX;
- ALGORITHM duplicates SELECT layer movement;
- PRESETS is a global slot cursor;
- black keys were otherwise inert except STEP manual note entry with SEL held.

### C2 - REC cleanup: COMPLETE

Implementation/test checkpoint: `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.

Delivered:

- REC has no non-record page-action semantics; until an actual capture path exists it is reserved/inert;
- stable contextual verb slots introduced: B0=`state toggle`, B1=`trigger/reroll/tap`, B2=`reset`, B3=`destructive`;
- former REC actions migrated without removing functionality:
  - RANDOM B1 reroll, B3 confirmed CLEAR;
  - GLOBAL B1 TAP;
  - LOOP B2 reset;
  - DOT/TURING B1 reroll;
  - SUB/CHORD/SAMPLE B0 toggle;
- SAMPLE remains reachable through the ENV/SOUND family: ENV -> SOUND, SOUND ENV -> SAMPLE, SAMPLE ENV -> SOUND;
- STEP `SEL+black key` manual note entry still takes precedence and remains musical;
- WP-A randomizer UI smoke now uses RANDOM B1 instead of REC without weakening LOW/MED/HIGH, WAVE, skyline or redraw assertions;
- exact generated C2 UI and migrated WP-A smoke hashes are locked.

C2 green evidence:

- control audit `40671589758`;
- control-action contract `40671589741`;
- REC grammar `40671589777`;
- rotary parity `40671589763`;
- display integrity `40671589734`;
- live-edit `40671589787`;
- reconstruction `40671589813`;
- full target `40671590524`.

Full target retained:

- WP-A `LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4/0`;
- 300 BPM event-order stress: PASS;
- browser/WASM 100 MHz steady realtime `3.287x`;
- SEQ/HOME 80 MHz `3.355x`;
- Chord + Reverb V2 80 MHz `3.129x`;
- live-edit 80 MHz `2.920x`, tail `88156` nonzero samples;
- RAM headroom `21472 B`;
- pool headroom `17696 B`;
- package SHA-256 `cb09e296517f2f699fdf2b926e2baba7de033b52b498be765850cf5709615c12`;
- provenance: PASS.

## Active WP-C subtask

**C3 - EDIT cleanup.**

Start here. Do not begin C4 early.

### C3 required work

1. Remove generic `EDIT -> RANDOM` navigation without making RANDOM inaccessible.
2. Preserve EDIT meanings that are genuinely edit/focus/lock semantics:
   - Turing `EDIT = LOCK`;
   - Sample `EDIT = per-step edit mode`.
3. Move STEP `EDIT+white key = SKIP` to a visible contextual action on the selected step.
4. Move SUB `EDIT+white key = step enable` to a visible contextual action on the selected SUB step.
5. Evaluate DOT `EDIT = reset toggle`; if it is not a true edit/focus/lock state, migrate it rather than retaining an exception.
6. Keep the C2 REC contract exact and preserve the STEP manual-note exception.
7. Add deterministic Chord-on/off EDIT-contract coverage before C3 closeout.

### Leave for C4

- RANDOM OCT ALL/NONE migration;
- GLOBAL RESET ALL OCT chord migration;
- LOOP K4 RESET removal;
- final visible black-key guidance/action rail rendering;
- MOD navigation reorganization (Audio LFO / DOT / Turing);
- authoritative handler + rendered-guidance action table.

### WP-C exclusions

No WP-D timing model, distortion/Reverb V3, Undo, FM/SOUND randomization, per-step timbre, harmonic replacement or general onboarding scope.

## WP-C stop condition

When C1-C5 and full regressions are green:

- record exact implementation checkpoint/run evidence;
- set **WP-D Timing model** active;
- stop and hand off rather than beginning WP-D automatically.
