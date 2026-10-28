# PROJECT STATE

Last verified subtask closeout: 2026-10-28

## Repository

- Repo: `joakimskurk/fm1mello`
- Active branch: `visual-port-home`
- Always resolve current branch HEAD before editing.
- New chats start with root `START_HERE.md`; repo documentation is source of truth.
- Firmware is reconstructed by `ci/unpack_source.sh` from source chunks plus an ordered patch/test chain.

## Work-package position

- WP-A Randomizer intensity + RANDOM skyline: **COMPLETE**.
- WP-B Trust and reliability: **COMPLETE**.
- WP-C Control Grammar v2: **ACTIVE**.
  - C1 complete action audit: **COMPLETE** at `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.
  - C2 REC cleanup: **COMPLETE** at `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.
  - C3 EDIT cleanup: **COMPLETE** at implementation/test checkpoint `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`.
  - **ACTIVE SUBTASK: C4 contextual rail / hidden actions / MOD navigation.**

Do not begin C5 or WP-D before C4 is green and documented.

## Retained WP-B baseline

Final WP-B implementation/test checkpoint: `7c20efbdc391c59118fef3f000689eeafaa176b9`.

Final WP-B gates: reconstruction `40604213816`, live-edit `40604213682`, rotary parity `40604213768`, display integrity `40604213707`, full target `40604214660`: PASS.

Physical FM-1 live-edit reproduction remains a later hands-on regression because hardware is not currently available.

## C1/C2 retained control baseline

C1 mapped the complete pre-remap control behavior in `docs/WP_C_CONTROL_AUDIT.md`. C2 reserved REC for actual record/capture and introduced contextual B0 state / B1 trigger / B2 reset / B3 destructive-confirmed actions. C2 also moved SAMPLE navigation to ENV/SOUND and preserved STEP `SEL+black key` manual note entry.

Exact C2 outputs remain locked before later WP-C transforms:

- UI SHA-256 `3da137c8faa5800bcfddd8291176e77718bc320d890f20f9f9d0c9cba4aaffb0`;
- migrated WP-A smoke SHA-256 `71fc32a27fedfb0509ab0ff9eccfc85ba53d90c9b6d6bf48f0915f15c1d5fa13`.

## C3 delivered EDIT grammar

C3 makes EDIT truthful without deleting functionality:

- generic EDIT-to-RANDOM navigation removed;
- Turing `EDIT=LOCK` retained;
- Sample `EDIT=per-step edit state` retained;
- STEP hidden `EDIT+white=SKIP` removed; contextual B4 toggles SKIP on selected step;
- SUB hidden `EDIT+white=enable` removed; contextual B4 toggles selected SUB step;
- DOT reset moved from EDIT to contextual B2;
- contextual B5 opens RANDOM without restoring an EDIT overload;
- STEP `SEL+black` manual note entry remains higher priority than contextual actions;
- REC remains reserved/inert exactly as in C2.

Exact generated C3 UI SHA-256: `c811c6ee8e5d6422a479452c65e0c4720d93d0b979cba3b4bcd1281f95a162da`.

C3 deterministic contract passes with Chord enabled (`16` pages) and compiled out (`15` pages).

## C3 validation evidence

Implementation/test checkpoint `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`:

- EDIT grammar `40685833340`: PASS;
- reconstruction `40685833285`: PASS;
- REC grammar `40685833209`: PASS;
- control-action contract `40685833243`: PASS;
- rotary parity `40685833249`: PASS;
- display integrity `40685833215`: PASS;
- live-edit `40685833246`: PASS;
- control audit `40685833273`: PASS;
- full target `40685833648`: PASS.

Full target `40685833648` retained:

- WP-A `LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4/0`;
- Chord-off WP-A `LOW=180984 < MED=484357 < HIGH=1038671`;
- 300 BPM event-order stress;
- live-edit host energy `433436686798`, ticks `159600`, applies `2470`;
- emulator panel/audio peak `3530`, `148408` nonzero samples;
- browser/WASM 100 MHz steady realtime `1.680x`;
- SEQ/HOME 80 MHz `1.701x`;
- Chord + Reverb V2 80 MHz `1.556x`;
- live-edit encoder 80 MHz `1.595x`, tail `88156`, peak `3274`;
- Chromium keyboard/pointer/PLAY;
- image `190788 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `611545 B`, SHA-256 `6673f3688954809a6b5ce00ed73ba39666764ec2414d224e1f206689e1fe7a52`;
- provenance attestation.

## Active C4 direction

C4 completes the user-facing contextual grammar before C5 centralizes the mapping source.

Required:

1. RANDOM mask NONE/ALL must leave OCT and become contextual rail actions.
2. GLOBAL RESET ALL must leave the hidden OCT chord and use a confirmed destructive contextual action.
3. LOOP layer-3 K4 RESET must disappear; contextual B2 already carries reset semantics.
4. Active black-key actions must become visibly identifiable in rendered guidance.
5. Modulation/generative navigation must group actual Audio LFO, DOT and Turing/EVOLVE coherently; the physical LFO/MOD control must not secretly toggle Turing.
6. Preserve C2/C3 semantics, SELECT layer navigation and STEP SEL+black manual note exception.
7. Add deterministic Chord-on/off action/navigation coverage before C4 closeout.

Leave one-authoritative-mapping-source refactoring and final full WP-C closeout for C5.

## Locked future sequence

After WP-C: WP-D timing model, WP-E audio character, WP-F Undo Last Random, WP-G ANALOG/FM + SOUND randomization/per-step timbre, WP-H harmonic layer decision, WP-I polish/onboarding, WP-J final integration.
