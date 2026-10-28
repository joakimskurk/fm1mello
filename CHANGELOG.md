# CHANGELOG

## 2026-10-28 - WP-C3 EDIT cleanup complete

WP-C remains active; C4 contextual rail / hidden actions / MOD navigation is next.

Implementation/test checkpoint: `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`.

C3 changes:

- removed generic `EDIT -> RANDOM` navigation;
- retained only genuine EDIT semantics: Turing `LOCK` and Sample `per-step` edit state;
- moved STEP `EDIT+white key = SKIP` to contextual B4 on the selected step;
- moved SUB `EDIT+white key = step enable` to contextual B4 on the selected SUB step;
- moved DOT reset from EDIT to contextual B2;
- added contextual B5 as coherent non-EDIT RANDOM access;
- preserved C2 REC semantics and STEP `SEL+black key` chromatic manual-note priority;
- added deterministic Chord-on/off current action/EDIT coverage;
- locked generated C3 UI SHA-256 `c811c6ee8e5d6422a479452c65e0c4720d93d0b979cba3b4bcd1281f95a162da`.

Validation:

- EDIT grammar `40685833340`: PASS;
- reconstruction `40685833285`: PASS;
- REC grammar `40685833209`: PASS;
- control-action contract `40685833243`: PASS;
- rotary parity `40685833249`: PASS;
- display integrity `40685833215`: PASS;
- live-edit `40685833246`: PASS;
- control audit `40685833273`: PASS;
- full target `40685833648`: PASS;
- WP-A randomizer and Chord-off randomizer: PASS;
- 300 BPM timing: PASS;
- emulator/browser/80 MHz/Chromium: PASS;
- RAM headroom `21472 B`; pool headroom `17696 B`;
- package SHA-256 `6673f3688954809a6b5ce00ed73ba39666764ec2414d224e1f206689e1fe7a52`;
- provenance attestation: PASS.

C4 is active: move remaining hidden OCT/K4 actions to the contextual rail, make active actions visibly guided, and organize Audio LFO/DOT/Turing coherently under MOD/generative navigation.

## 2026-10-28 - WP-C2 REC cleanup complete

Implementation/test checkpoint `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.

REC is reserved for actual record/capture; B0-B3 contextual slots preserve legacy REC functions, SAMPLE navigation moved to ENV/SOUND, and the WP-A randomizer trigger moved to B1. Final C2 target `40671590524`: PASS.

## 2026-10-28 - WP-C1 control-action audit complete

C1 baseline checkpoint `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`. Added deterministic pre-remap coverage and `docs/WP_C_CONTROL_AUDIT.md`.

## 2026-10-28 - WP-B trust and reliability complete

WP-B final checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; target `40604214660`: PASS. B1 live-edit, B2 rotary parity and B3 display integrity remain regression-protected.

Physical FM-1 live-edit reproduction remains a later hands-on regression because hardware is not currently available.

## 2026-10-26 - WP-A randomizer intensity + RANDOM skyline complete

Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`; runs `40294234032` and `40294234527`: PASS.

## 2026-10-25 - approved next-iteration handoff

Product owner approved the 2026-10-25 product-workshop direction.

## 2026-10-25 - post-audition product review

Added `docs/PRODUCT_WORKSHOP_2026-10-25.md` and the stabilization backlog.

## 2026-10-23 - handoff checkpoint for new chat

Added root continuity files and documented the then-active randomizer work.
