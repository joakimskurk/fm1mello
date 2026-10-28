# CHANGELOG

## 2026-10-28 - WP-C2 REC cleanup complete

WP-C remains active; C3 EDIT cleanup is next.

Implementation/test checkpoint: `f6d1f81660dab8db1de6da4e4650d01482d1ced3`.

C2 changes:

- REC now has no unrelated page-action meanings and is reserved for actual recording/capture; no capture implementation exists yet, so REC is intentionally inert rather than overloaded;
- introduced stable contextual slots: B0 state toggle, B1 trigger/reroll/tap, B2 reset, B3 destructive/confirmed;
- migrated RANDOM reroll/CLEAR, GLOBAL TAP, LOOP reset, DOT/TURING reroll and SUB/CHORD/SAMPLE toggles off REC without deleting functionality;
- moved SAMPLE navigation into the ENV/SOUND family;
- preserved STEP `SEL+black key` chromatic manual-note entry as an explicit exception;
- migrated the WP-A randomizer UI smoke from REC to RANDOM B1 without changing LOW/MED/HIGH, WAVE, skyline or redraw assertions;
- locked generated C2 UI SHA `3da137c8faa5800bcfddd8291176e77718bc320d890f20f9f9d0c9cba4aaffb0` and migrated randomizer-smoke SHA `71fc32a27fedfb0509ab0ff9eccfc85ba53d90c9b6d6bf48f0915f15c1d5fa13`.

Validation:

- control audit `40671589758`: PASS;
- control-action contract `40671589741`: PASS;
- REC grammar `40671589777`: PASS;
- rotary parity `40671589763`: PASS;
- display integrity `40671589734`: PASS;
- live-edit `40671589787`: PASS;
- reconstruction `40671589813`: PASS;
- full target `40671590524`: PASS;
- RAM headroom `21472 B`; pool headroom `17696 B`;
- package SHA-256 `cb09e296517f2f699fdf2b926e2baba7de033b52b498be765850cf5709615c12`.

C3 is active: remove generic EDIT navigation, retain true edit/focus/lock meanings, migrate hidden STEP/SUB EDIT+key actions, and classify/migrate DOT reset.

## 2026-10-28 - WP-C1 control-action audit complete

C1 baseline checkpoint: `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`.

Added deterministic pre-remap coverage for REC/EDIT/SELECT/black keys/OCT/ALGORITHM/PRESETS and hidden page actions, plus `docs/WP_C_CONTROL_AUDIT.md`. The audit identified the exact legacy overloads that C2-C4 must migrate rather than delete.

## 2026-10-28 - WP-B trust and reliability complete

WP-B final implementation/test checkpoint: `7c20efbdc391c59118fef3f000689eeafaa176b9`.

B1 live-edit, B2 rotary parity and B3 display integrity are complete. Final gates: reconstruction `40604213816`, live-edit `40604213682`, rotary `40604213768`, display integrity `40604213707`, target `40604214660`: PASS. RAM headroom `21472 B`, pool `17696 B`, package SHA-256 `b0bf86215ab4c547c668c54c3f023ff05fb7f4e87b728bcb7e84365a02cebf1e`.

The physical FM-1 live-edit symptom remains a later hands-on regression because hardware is not currently available.

## 2026-10-28 - WP-B2 rotary display/handler parity complete

Checkpoint `85a3c376b47829af6d6abdd3264ec75cb37d42bf`; rotary `40599056435`, reconstruction `40599056475`, B1 `40599056403`, target `40599056945`: PASS.

## 2026-10-27 - WP-B1 live-edit audio reliability complete

Checkpoint `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`; live-edit `40596812842`, reconstruction `40596812864`, target `40596813400`: PASS.

## 2026-10-26 - WP-A randomizer intensity + RANDOM skyline complete

Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`; runs `40294234032` and `40294234527`: PASS.

## 2026-10-25 - approved next-iteration handoff

Product owner approved the 2026-10-25 product-workshop direction.

## 2026-10-25 - post-audition product review

Added `docs/PRODUCT_WORKSHOP_2026-10-25.md` and the stabilization backlog.

## 2026-10-23 - handoff checkpoint for new chat

Added root continuity files and documented the then-active randomizer work.
