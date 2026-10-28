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
8. `docs/WP_C_CONTROL_AUDIT.md`
9. `docs/PRODUCT_WORKSHOP_2026-10-25.md`
10. `docs/UI_VISUAL_LANGUAGE.md` when UI is touched.

## Completed

- WP-A Randomizer intensity + RANDOM skyline: COMPLETE.
- WP-B Trust and reliability: COMPLETE.
- WP-C1 audit: COMPLETE at `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.
- WP-C2 REC cleanup: COMPLETE at `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.
- WP-C3 EDIT cleanup: COMPLETE at `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`.
- WP-C4 contextual rail / hidden actions / MOD navigation: COMPLETE at implementation/test checkpoint `84d42c9fa21b2a91db5fa1aff2340c6ef3289a60`.

## C4 delivered

- RANDOM `NONE/ALL` moved from OCT to contextual B6/B7; RANDOM OCT is inert.
- GLOBAL hidden OCT-pair reset is gone; contextual B3 performs confirmed reset.
- LOOP layer-3 K4 reset is gone/inert; contextual B2 remains reset.
- Active black-key actions are visible in rendered guidance; inactive slots are inert.
- LFO/MOD cycles Audio LFO -> DOT -> EVOLVE -> Audio LFO without toggling Turing state.
- FX opens AUDIO FX directly; DOT belongs to MOD navigation.
- C2/C3 REC/EDIT rules, SELECT navigation and STEP `SEL+black` manual-note priority are preserved.

Exact generated C4 UI SHA-256: `a3829d2fdc354154021727838b4b17cf8420832f88565a2b2f290b17a06002c5`.
Generated `ui_visual_smoke.c` SHA-256: `2659928170069c1e52ed4e41ff95e21cfd03d80d540a958b3f43e7b1402bffb1`.

C4 green runs on the implementation checkpoint:

- reconstruction `40701004129`;
- contextual rail/MOD `40701004213`;
- REC `40701004034`;
- EDIT `40701004030`;
- action contract `40701004015`;
- audit `40701004022`;
- rotary `40701004133`;
- display `40701004059`;
- live-edit `40701004057`;
- full target `40701005084`.

Full target retained >16 KiB RAM/pool headroom and all WP-A/WP-B/audio/timing/emulator/browser/80 MHz/Chromium gates. See `TESTING.md` for exact measurements.

## Active subtask

**WP-C5 - Authoritative page-action mapping + full WP-C regression closeout.**

Do not begin WP-D early.

### C5 required work

1. Replace duplicated contextual-action definitions between handler semantics and rendered hints with one authoritative/testable mapping source where practical.
2. The source must represent active/inactive B0-B7 slots and visible action labels without changing C4 musical behavior.
3. Handler dispatch and rendered guidance must consume the same mapping/descriptor rather than independently maintained page switches.
4. Preserve all C1-C4 rules: REC/EDIT, SELECT, OCT navigation, MOD family, destructive confirmation and STEP `SEL+black` manual-note priority.
5. Add deterministic page/action parity coverage with Chord enabled and compiled out.
6. Rerun WP-A, WP-B, WP-C, audio/timing, emulator/browser, 80 MHz, Chromium and full target; require >=16 KiB RAM and pool headroom.

## C5 stop condition

When C5 and the full matrix are green, record exact evidence, set **WP-D Timing model** active, then stop/handoff instead of implementing WP-D.

## Exclusions

No timing redesign, distortion/Reverb V3, Undo, FM/SOUND randomization, per-step timbre, harmonic replacement or general onboarding inside C5.

**Product rule: Keep the depth, reduce the number of grammars.**
