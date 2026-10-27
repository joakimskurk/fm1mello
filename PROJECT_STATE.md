# PROJECT STATE

Last verified work-package closeout: 2026-10-28

## Repository

- Repo: `joakimskurk/fm1mello`
- Active branch: `visual-port-home`
- Always resolve current branch HEAD before editing.
- New chats start with root `START_HERE.md` and follow the repo rather than old chat context.

## Work-package position

**WP-A / Step 4 - Randomizer intensity + RANDOM skyline: COMPLETE.**

Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; clean reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

**WP-B - Trust and reliability: COMPLETE.**

- B1 live-edit audio reliability: COMPLETE.
- B2 rotary display/handler parity: COMPLETE.
- B3 display integrity: COMPLETE.

Final WP-B implementation/test checkpoint: `7c20efbdc391c59118fef3f000689eeafaa176b9`.

**ACTIVE WORK PACKAGE: WP-C - Control Grammar v2.**

Do not begin WP-D or later work before WP-C is green and documented.

## WP-B final evidence

All five gates are green on the final implementation checkpoint:

- reconstruction `40604213816`;
- live-edit `40604213682`;
- rotary parity `40604213768`;
- display integrity `40604213707`;
- full target `40604214660`.

### B1 retained reliability contract

B1 introduced odd/even pending-config publication, 32-bit generation coverage, SUB drive-curve preparation outside audio render, selective derived-state invalidation, host stress and 80 MHz physical-encoder stress.

Original B1 checkpoint: `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`.

Final WP-B run reconfirms live editing:

- energy `433436686798`;
- transport tick `159600`;
- `2470` config applies;
- `67` SUB curve builds;
- `135` reverb reconfigs;
- physical encoder 80 MHz steady realtime `3.040x`;
- tail `88154` nonzero samples, peak `3207`.

Physical FM-1 hands-on reproduction remains deferred until hardware is available.

### B2 retained rotary contract

Original B2 checkpoint: `85a3c376b47829af6d6abdd3264ec75cb37d42bf`.

- GLOBAL K4 MIDI OUT parity fixed;
- Loop Cycle STEP K3=`OCT`, K4=`TRANS` parity fixed;
- SLOTS inactive/edit footer made truthful;
- real K1-K4 encoder fixture covers every current page/layer with Chord enabled and compiled out.

Final rotary-parity gate `40604213768`: PASS.

### B3 delivered display integrity

Final B3/WP-B checkpoint: `7c20efbdc391c59118fef3f000689eeafaa176b9`.

Root cause was structural: the pinned Dinghy/Felucca renderer uses an 8x16 small font and 16x32 large font and clears each full text box before blitting. Several legacy UI rows were spaced as if those boxes were shorter. On HOME, the `SINGLE` summary occupied y=160..175 while the purple probability bar was drawn through y=168..170 and the footer began immediately below it, producing the reported static pink-line corruption.

B3 now:

- reflows header/body/footer rows around the actual 16/32 px font metrics;
- removes the HOME graphic/text collision and protects dirty redraw equivalence;
- fits or compacts footer values deterministically instead of allowing clipping;
- preserves full `WAVE` on RANDOM with a 34 px final target cell;
- keeps the new text-length path freestanding-safe without libc `strlen`;
- moves the SEGMENT 32-cell strip to y=52 with the original 32-cell/bounds requirement intact;
- adds `tools/ui_display_integrity.c` and a dedicated CI gate.

B3 fixture evidence:

- Chord enabled: `150` fixtures, `0` clips, `0` overlaps;
- Chord disabled: `135` fixtures, `0` clips, `0` overlaps;
- HOME incremental redraw equals clean redraw for the protected region.

## Final WP-B target/performance evidence

Full target run `40604214660`:

- WP-A: `LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4/0`, WAVE isolation and 4..32 skyline: PASS;
- Chord/Reverb and 300 BPM timing: PASS;
- emulator panel input + audio: PASS;
- browser/WASM 100 MHz steady realtime `3.403x`;
- SEQ/HOME 80 MHz steady realtime `3.528x`;
- Chord + Reverb V2 80 MHz steady realtime `3.306x`;
- live-edit 80 MHz steady realtime `3.040x`;
- Chromium keyboard/pointer/PLAY routes: PASS;
- image `190756 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `611545 B`, SHA-256 `b0bf86215ab4c547c668c54c3f023ff05fb7f4e87b728bcb7e84365a02cebf1e`;
- build provenance: PASS.

## Active WP-C direction

WP-C is Control Grammar v2. The approved direction is:

- REC = actual recording/capture only;
- EDIT = edit/focus/lock only;
- unrelated page actions become visible contextual actions;
- black keys are the contextual action rail where useful;
- SELECT remains functional-layer navigation;
- LFO + DOT group coherently under MOD;
- Turing belongs to the broader modulation/generative family;
- one authoritative/testable page-action map should drive behavior and rendered guidance where practical.

Begin with a complete mapping audit and deterministic baseline before remapping controls. Preserve all WP-A/WP-B/audio/timing/performance/target gates.

## Locked product direction after WP-C

- WP-D: timing model - default `1 BAR`, explicit FREE, timing concepts kept separate.
- WP-E: audio character - extreme distortion, Reverb V3, ratchet parity and live smoothing.
- WP-F: one-level Undo Last Random.
- WP-G: preserve ANALOG, add curated FM, SOUND randomization and realtime-safe per-step timbre.
- WP-H: prototype scale-aware HARMONY, then PAD, before replacing current Chord.
- WP-I: splash/HOME/Key-Scale/SEQ/onboarding polish.
- WP-J: final integration/performance gate.

## Source reconstruction model

Firmware source is reconstructed by `ci/unpack_source.sh` from source chunks plus an ordered patch chain. Functional work must continue to follow this patch/test architecture. Do not assume reconstructed `mello/` files are ordinary committed source of truth.

## New-chat procedure

Read `START_HERE.md` first. It contains the active package, work order, acceptance criteria and stop condition.
