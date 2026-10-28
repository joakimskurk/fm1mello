# PROJECT STATE

Last verified subtask closeout: 2026-10-28

## Repository

- Repo: `joakimskurk/fm1mello`
- Active branch: `visual-port-home`
- Always resolve current branch HEAD before editing.
- New chats start with `START_HERE.md`; repo documentation is source of truth.
- Firmware is reconstructed by `ci/unpack_source.sh` from source chunks plus an ordered patch/test chain.

## Work-package position

- WP-A Randomizer intensity + RANDOM skyline: **COMPLETE**.
- WP-B Trust and reliability: **COMPLETE**.
- WP-C Control Grammar v2: **ACTIVE**.
  - C1 complete action audit: **COMPLETE** at `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.
  - C2 REC cleanup: **COMPLETE** at `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.
  - **ACTIVE SUBTASK: C3 EDIT cleanup.**

Do not begin C4 or WP-D before the active subtask is green and documented.

## Retained WP-B baseline

Final WP-B implementation/test checkpoint: `7c20efbdc391c59118fef3f000689eeafaa176b9`.

Final WP-B gates: reconstruction `40604213816`, live-edit `40604213682`, rotary parity `40604213768`, display integrity `40604213707`, full target `40604214660`: PASS.

Physical FM-1 live-edit reproduction remains a later hands-on regression because hardware is not currently available.

## C1 delivered audit baseline

C1 mapped the complete pre-remap control behavior and locked it in deterministic fixtures/documentation.

Confirmed debt:

- every page had a non-record REC meaning before C2;
- generic EDIT opened RANDOM on most pages;
- STEP hid SKIP behind `EDIT+white key`;
- SUB hid step enable behind `EDIT+white key`;
- RANDOM hid CLEAR behind `EDIT+REC` and mask ALL/NONE behind OCT;
- GLOBAL hides RESET ALL behind the OCT pair;
- LOOP layer 3 K4 is RESET rather than a value control;
- Turing `EDIT=LOCK` and Sample `EDIT=per-step` are valid edit-state semantics;
- DOT `EDIT=reset toggle` still requires classification/migration in C3;
- SELECT is correct layer navigation and does not move selected content;
- ALGORITHM duplicates SELECT; PRESETS moves only the global slot cursor;
- black keys were inert except STEP manual note entry with SEL held;
- physical LFO/DOT navigation does not match the intended MOD family.

Detailed source-of-truth table: `docs/WP_C_CONTROL_AUDIT.md`.

## C2 delivered REC grammar

C2 implementation/test checkpoint: `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.

REC is now reserved exclusively for actual record/capture. Because no capture path exists yet, normal REC is intentionally inert rather than overloaded.

Former REC actions were preserved on a stable contextual rail contract:

- B0 = state toggle;
- B1 = trigger / reroll / tap;
- B2 = reset;
- B3 = destructive action with confirmation.

Current C2 mappings:

- RANDOM: B1 reroll; B3 two-press confirmed CLEAR;
- GLOBAL: B1 TAP;
- LOOP: B2 reset;
- DOT: B1 reroll;
- TURING: B1 reroll;
- SUB / CHORD / SAMPLE: B0 on/off;
- SAMPLE navigation: ENV family instead of REC;
- STEP SEL+black-key chromatic note entry remains a protected manual-note exception.

WP-A randomizer test trigger was migrated from REC to B1 while retaining the same deterministic HIGH expectation and all intensity/WAVE/skyline/redraw assertions.

Exact C2 generated outputs:

- `mello/mello_ui.c` SHA-256 `3da137c8faa5800bcfddd8291176e77718bc320d890f20f9f9d0c9cba4aaffb0`;
- `tools/random_wave_overview_smoke.c` SHA-256 `71fc32a27fedfb0509ab0ff9eccfc85ba53d90c9b6d6bf48f0915f15c1d5fa13`.

## C2 validation evidence

All eight checks are green on the implementation checkpoint:

- control audit `40671589758`;
- control-action contract `40671589741`;
- REC grammar `40671589777`;
- rotary parity `40671589763`;
- display integrity `40671589734`;
- live-edit `40671589787`;
- reconstruction `40671589813`;
- full target `40671590524`.

Full target run `40671590524`:

- WP-A `LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4/0`: PASS;
- Chord-off WP-A `LOW=180984 < MED=484357 < HIGH=1038671`: PASS;
- 300 BPM timing: PASS;
- live-edit host regression: PASS;
- emulator panel/audio: PASS;
- browser/WASM 100 MHz steady realtime `3.287x`;
- SEQ/HOME 80 MHz `3.355x`;
- Chord + Reverb V2 80 MHz `3.129x`;
- physical encoder live-edit 80 MHz `2.920x`, final tail `88156` nonzero samples, peak `3274`;
- Chromium keyboard/pointer incl. PLAY: PASS;
- image `190820 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `611545 B`, SHA-256 `cb09e296517f2f699fdf2b926e2baba7de033b52b498be765850cf5709615c12`;
- provenance: PASS.

## Active C3 direction

C3 makes EDIT truthful without deleting functionality.

Required:

1. remove generic EDIT-to-RANDOM navigation and provide RANDOM a coherent non-EDIT route;
2. retain Turing `EDIT=LOCK` and Sample `EDIT=per-step`;
3. migrate STEP SKIP and SUB step-enable hidden EDIT+key modifiers to visible contextual actions on the selected content;
4. classify/migrate DOT reset if it is not genuinely edit/focus/lock;
5. preserve C2 REC semantics and manual note-entry exception exactly;
6. add a deterministic current EDIT contract with Chord on/off before closing C3.

Leave OCT ALL/NONE, RESET ALL, LOOP K4 RESET, MOD navigation and final rendered action guidance for C4/C5 as documented.

## Locked future sequence

After WP-C: WP-D timing model, WP-E audio character, WP-F Undo Last Random, WP-G ANALOG/FM + SOUND randomization/per-step timbre, WP-H harmonic layer decision, WP-I polish/onboarding, WP-J final integration.
