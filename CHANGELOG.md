# CHANGELOG

## 2026-10-28 - WP-C Control Grammar v2 complete

WP-C is complete. WP-D Timing model is ACTIVE NEXT; no WP-D implementation is included in this closeout.

Final implementation/test checkpoint: `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

C5 closeout:

- replaced duplicated contextual handler/render page mapping with one authoritative `MelloContextAction` page/B0-B7 descriptor;
- descriptor carries handler verb and rendered label together;
- handler dispatch now switches on descriptor verb rather than page;
- rendered contextual guidance iterates the same descriptor;
- added all-page/all-slot parity coverage with Chord enabled and compiled out;
- proved inactive slots inert, B5 RANDOM routing consistent, and STEP `SEL+black` manual-note priority retained;
- exact generated C5 UI/test outputs are hash-locked.

C5 coverage:

- Chord enabled: `16 pages / 31 active / 97 inactive`;
- Chord disabled: `15 pages / 29 active / 91 inactive`.

Final WP-C validation on `2c280184...`:

- authoritative actions `40703570244`: PASS;
- reconstruction `40703570315`: PASS;
- context grammar `40703570196`: PASS;
- REC `40703570212`: PASS;
- EDIT `40703570277`: PASS;
- action contract `40703570204`: PASS;
- audit `40703570330`: PASS;
- rotary `40703570466`: PASS;
- display `40703570254`: PASS;
- live-edit `40703570316`: PASS;
- full target `40703571071`: PASS;
- RAM headroom `21472 B`; pool headroom `17696 B`;
- package SHA-256 `755a098ecbe66a6f8f7697a5787cb240e50a69df2520dc2f10b690f0056b8da1`;
- provenance: PASS.

## 2026-10-28 - WP-C4 contextual rail and MOD navigation complete

Checkpoint `84d42c9fa21b2a91db5fa1aff2340c6ef3289a60`. RANDOM NONE/ALL moved to B6/B7, hidden GLOBAL/LOOP actions removed, contextual guidance rendered, MOD family reorganized, FX opens AUDIO FX directly. Target `40701005084`: PASS.

## 2026-10-28 - WP-C3 EDIT cleanup complete

Checkpoint `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`. Generic EDIT navigation removed; genuine lock/edit states retained; STEP/SUB selected-position toggles moved to B4; DOT reset to B2; RANDOM access to B5.

## 2026-10-28 - WP-C2 REC cleanup complete

Checkpoint `f6d1f81660dab8db1de6da4e4650d01482d1ced3`. REC reserved for actual recording/capture; legacy functions moved to contextual B0-B3; SAMPLE navigation moved to ENV/SOUND. Target `40671590524`: PASS.

## 2026-10-28 - WP-C1 control-action audit complete

Baseline checkpoint `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`. Added deterministic pre-remap coverage and `docs/WP_C_CONTROL_AUDIT.md`.

## 2026-10-28 - WP-B trust and reliability complete

Final checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; target `40604214660`: PASS. B1 live-edit, B2 rotary parity and B3 display integrity remain regression-protected.

## 2026-10-26 - WP-A randomizer intensity + RANDOM skyline complete

Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`; target `40294234527`: PASS.
