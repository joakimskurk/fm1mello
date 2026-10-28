# PROJECT STATE

Last verified subtask closeout: 2026-10-28

## Repository

- Repo: `joakimskurk/fm1mello`
- Active branch: `visual-port-home`
- Verify branch HEAD before editing.
- New chats start with `START_HERE.md`; repo documentation is source of truth.
- Firmware is reconstructed by `ci/unpack_source.sh` from source chunks plus ordered transforms/tests.

## Work-package position

- WP-A Randomizer intensity + skyline: **COMPLETE**.
- WP-B Trust and reliability: **COMPLETE**.
- WP-C Control Grammar v2: **ACTIVE**.
  - C1 audit: **COMPLETE** at `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.
  - C2 REC cleanup: **COMPLETE** at `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.
  - C3 EDIT cleanup: **COMPLETE** at `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`.
  - C4 contextual rail / hidden actions / MOD navigation: **COMPLETE** at `84d42c9fa21b2a91db5fa1aff2340c6ef3289a60`.
  - **ACTIVE SUBTASK: C5 authoritative page-action mapping + full WP-C closeout.**

Do not begin WP-D before C5 is green and WP-C is documented closed.

## Retained control contracts

C2 reserves REC for real recording/capture and introduced stable contextual verbs. C3 reserves EDIT for genuine edit/focus/lock behavior and moved selected STEP/SUB toggles to B4. STEP `SEL+black` remains chromatic manual note entry and has priority over contextual actions.

C4 completes the visible contextual grammar:

- RANDOM: B1 reroll, B3 confirmed clear, B6 NONE, B7 ALL; OCT does not edit the mask.
- GLOBAL: B1 tap, B3 confirmed project reset; hidden OCT-pair reset removed.
- LOOP: B2 reset; layer-3 K4 is inert.
- TURING: B0 on/off, B1 reroll; EDIT remains LOCK.
- B5 opens RANDOM on mapped non-RANDOM pages.
- LFO/MOD navigation cycles Audio LFO -> DOT -> EVOLVE and does not toggle Turing.
- FX opens AUDIO FX directly.
- active black-key actions are rendered; inactive action slots are inert.

Exact final C4 generated UI SHA-256: `a3829d2fdc354154021727838b4b17cf8420832f88565a2b2f290b17a06002c5`.

## C4 validation evidence

Implementation/test checkpoint `84d42c9fa21b2a91db5fa1aff2340c6ef3289a60`:

- reconstruction `40701004129`: PASS;
- contextual rail/MOD `40701004213`: PASS;
- REC `40701004034`: PASS;
- EDIT `40701004030`: PASS;
- action contract `40701004015`: PASS;
- audit `40701004022`: PASS;
- rotary `40701004133`: PASS;
- display `40701004059`: PASS;
- live-edit `40701004057`: PASS;
- full target `40701005084`: PASS.

Full target retained canonical randomizer semantics, 300 BPM timing, live-edit safety, emulator/browser/Chromium and target memory gates. RAM headroom `21472 B`; pool headroom `17696 B`; package SHA-256 `9747bed2ac5c974aed8b0cf9f340f395c5d566b879e5632162de65b75d9d2864`.

## Active C5 direction

C4 still contains duplicated knowledge: contextual handler semantics and rendered hint strings are maintained through separate code paths. C5 must centralize this into one authoritative/testable page-action descriptor where practical, then prove behavior/render parity on every current page with Chord enabled and compiled out.

C5 must preserve C1-C4 exactly and rerun the complete WP-A/WP-B/WP-C/audio/timing/emulator/browser/80 MHz/Chromium/target matrix. Require >=16 KiB RAM and pool headroom.

When C5 is fully green, close WP-C, set WP-D Timing model active, and stop/handoff.

## Future sequence

WP-D Timing model -> WP-E Audio character -> WP-F Undo Last Random -> WP-G ANALOG/FM + SOUND randomization/per-step timbre -> WP-H harmonic layer decision -> WP-I polish/onboarding -> WP-J final integration.
