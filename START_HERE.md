# START HERE - FM-1 Mello continuation

Use this file at the start of every new ChatGPT work session on `joakimskurk/fm1mello`.

## Repository

- Repo: `joakimskurk/fm1mello`
- Branch: `visual-port-home`
- Always fetch and verify the current branch HEAD before editing.
- Repo documentation is source of truth; do not reconstruct direction from chat memory when these files are available.
- Firmware source is reconstructed by `ci/unpack_source.sh` from source chunks plus an ordered patch/test chain.

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

## Completed work

- WP-A Randomizer intensity + RANDOM skyline: COMPLETE.
- WP-B Trust and reliability: COMPLETE.
- WP-C1 control-action audit: COMPLETE at `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.
- WP-C2 REC cleanup: COMPLETE at `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.
- WP-C3 EDIT cleanup: COMPLETE at implementation/test checkpoint `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`.

### C3 delivered

EDIT is now truthful:

- generic `EDIT -> RANDOM` navigation is gone;
- Turing `EDIT = LOCK` remains;
- Sample `EDIT = per-step edit state` remains;
- STEP `EDIT+white key = SKIP` is removed; selected STEP SKIP now uses contextual B4;
- SUB `EDIT+white key = step enable` is removed; selected SUB-step enable now uses contextual B4;
- DOT reset moved from EDIT to contextual B2;
- RANDOM remains reachable through contextual B5;
- STEP `SEL+black key` remains chromatic manual-note input and takes priority over contextual actions;
- C2 REC semantics remain unchanged.

Exact generated C3 UI SHA-256: `c811c6ee8e5d6422a479452c65e0c4720d93d0b979cba3b4bcd1281f95a162da`.

C3 green evidence on implementation checkpoint:

- EDIT grammar `40685833340`;
- reconstruction `40685833285`;
- REC grammar `40685833209`;
- control-action contract `40685833243`;
- rotary parity `40685833249`;
- display integrity `40685833215`;
- live-edit `40685833246`;
- control audit `40685833273`;
- full target `40685833648`.

Full target `40685833648`:

- WP-A enabled `LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4/0`: PASS;
- WP-A Chord-off `LOW=180984 < MED=484357 < HIGH=1038671`: PASS;
- 300 BPM timing: PASS;
- live-edit host stress: energy `433436686798`, ticks `159600`, applies `2470`: PASS;
- emulator panel/audio: peak `3530`, `148408` nonzero samples: PASS;
- browser/WASM 100 MHz steady realtime `1.680x`: PASS;
- SEQ/HOME 80 MHz steady realtime `1.701x`: PASS;
- Chord + Reverb V2 80 MHz steady realtime `1.556x`: PASS;
- live-edit 80 MHz steady realtime `1.595x`, tail `88156` nonzero samples, peak `3274`: PASS;
- Chromium keyboard/pointer/PLAY routes: PASS;
- image `190788 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `skurkymello-0.1-dev.fwsc`, `611545 B`;
- package SHA-256 `6673f3688954809a6b5ce00ed73ba39666764ec2414d224e1f206689e1fe7a52`;
- provenance attestation: PASS.

The reusable workflow's separate `Tests` step is intentionally skipped because `test:` is empty; `bash ci/build_target.sh` runs the required regressions inside Build.

## Active subtask

**WP-C4 - Contextual rail / remaining hidden actions / MOD navigation.**

Do not begin C5 or WP-D early.

### C4 required work

1. Move RANDOM mask `NONE/ALL` off OCT buttons to visible contextual actions; OCT returns to position/bank navigation only.
2. Move GLOBAL `RESET ALL` off the hidden OCT-pair chord to a destructive contextual action with confirmation.
3. Remove LOOP layer-3 K4 `RESET`; preserve the already-existing contextual B2 reset so K1-K4 remain value/musical controls.
4. Make contextual black-key actions visibly identifiable in rendered guidance; inactive black keys remain inert.
5. Reorganize modulation/generative navigation coherently:
   - actual Audio LFO belongs under MOD;
   - DOT belongs under MOD;
   - Turing/EVOLVE belongs in the same broader modulation/generative family;
   - physical LFO/MOD navigation must no longer be a hidden Turing on/off toggle.
6. Preserve every C2/C3 action semantically and keep `SEL+black key` manual STEP note entry as the explicit musical exception.
7. Add deterministic Chord-on/off tests for the resulting rail and navigation before closing C4.

### C4 exclusions

Do not implement authoritative mapping/render-source refactoring beyond what C4 needs; that is C5. Do not begin WP-D timing, audio-character work, Undo, FM/SOUND randomization, per-step timbre, harmonic replacement or general onboarding polish.

## WP-C stop condition

After C4, complete C5 authoritative mapping + full regression closeout. Only when WP-C is fully green: record exact evidence, set **WP-D Timing model** active, then stop/handoff instead of beginning WP-D.

## Non-negotiable product rule

**Keep the depth, reduce the number of grammars.**
