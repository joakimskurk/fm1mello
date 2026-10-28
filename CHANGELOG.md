# CHANGELOG

## 2026-10-28 - WP-C4 contextual rail and MOD navigation complete

WP-C remains active; C5 authoritative page-action mapping + full WP-C regression closeout is next.

Implementation/test checkpoint: `84d42c9fa21b2a91db5fa1aff2340c6ef3289a60`.

C4 changes:

- moved RANDOM mask NONE/ALL from OCT to contextual B6/B7;
- removed GLOBAL hidden OCT-pair reset and moved confirmed reset to B3;
- removed LOOP layer-3 K4 RESET while retaining contextual B2 RESET;
- made active black-key contextual actions visible and kept inactive slots inert;
- grouped actual Audio LFO, DOT and Turing/EVOLVE under LFO/MOD navigation;
- physical LFO/MOD no longer toggles Turing state;
- FX opens AUDIO FX directly instead of routing through DOT;
- preserved C2/C3 REC/EDIT semantics, SELECT layers and STEP `SEL+black` manual-note priority;
- migrated stale target tests to the contextual rail without weakening RANDOM mask assertions;
- locked generated C4 UI SHA `a3829d2fdc354154021727838b4b17cf8420832f88565a2b2f290b17a06002c5` and visual-smoke SHA `2659928170069c1e52ed4e41ff95e21cfd03d80d540a958b3f43e7b1402bffb1`.

Validation on the final implementation checkpoint:

- reconstruction `40701004129`: PASS;
- contextual rail/MOD `40701004213`: PASS;
- REC `40701004034`: PASS;
- EDIT `40701004030`: PASS;
- action contract `40701004015`: PASS;
- audit `40701004022`: PASS;
- rotary `40701004133`: PASS;
- display `40701004059`: PASS;
- live-edit `40701004057`: PASS;
- full target `40701005084`: PASS;
- RAM headroom `21472 B`; pool headroom `17696 B`;
- package SHA-256 `9747bed2ac5c974aed8b0cf9f340f395c5d566b879e5632162de65b75d9d2864`;
- provenance: PASS.

## 2026-10-28 - WP-C3 EDIT cleanup complete

Implementation/test checkpoint `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`. Generic EDIT navigation removed; genuine lock/edit states retained; STEP/SUB selected-position toggles moved to B4; DOT reset to B2; RANDOM access to B5. Full target `40685833648`: PASS.

## 2026-10-28 - WP-C2 REC cleanup complete

Implementation/test checkpoint `f6d1f81660dab8db1de6da4e4650d01482d1ced3`. REC is reserved for actual recording/capture; legacy functions moved to contextual B0-B3; SAMPLE navigation moved to ENV/SOUND. Full target `40671590524`: PASS.

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
