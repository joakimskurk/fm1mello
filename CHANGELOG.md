# CHANGELOG

## 2026-10-28 - WP-B trust and reliability complete

WP-B is complete. WP-C Control Grammar v2 is the next active package; no WP-C implementation is included in this closeout.

Final implementation/test checkpoint: `7c20efbdc391c59118fef3f000689eeafaa176b9`.

B3 display-integrity closeout:

- reproduced the HOME pink-line issue as a real collision between the 16 px `SINGLE` summary row, the purple probability bar and the following footer region;
- reflowed UI geometry around the pinned renderer's real 8x16 and 16x32 fixed-cell fonts;
- added deterministic large-to-small fitting/compaction for constrained value boxes;
- removed the HOME graphic/text collision and regression-covered incremental-vs-clean redraw;
- preserved full RANDOM `WAVE` with a 34 px final target cell;
- kept new text fitting freestanding-safe without libc `strlen`;
- moved the SEGMENT 32-cell strip to y=52 while retaining the original 32-cell/bounds test;
- added permanent `tools/ui_display_integrity.c` and dedicated CI.

B3 deterministic coverage:

- Chord enabled: `150` fixtures, `0` clips, `0` overlaps;
- Chord compiled out: `135` fixtures, `0` clips, `0` overlaps;
- HOME dirty-vs-clean protected-region equivalence: PASS.

Final WP-B validation:

- reconstruction `40604213816`: PASS;
- B1 live-edit `40604213682`: PASS;
- B2 rotary parity `40604213768`: PASS;
- B3 display integrity `40604213707`: PASS;
- full target `40604214660`: PASS;
- WP-A randomizer `LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4/0`: PASS;
- 300 BPM event-order stress: PASS;
- browser/WASM 100 MHz `3.403x` steady realtime;
- SEQ/HOME 80 MHz `3.528x`;
- Chord + Reverb V2 80 MHz `3.306x`;
- live-edit encoder stress 80 MHz `3.040x`, tail `88154` nonzero samples;
- RAM headroom `21472 B`; pool headroom `17696 B`;
- package SHA-256 `b0bf86215ab4c547c668c54c3f023ff05fb7f4e87b728bcb7e84365a02cebf1e`;
- provenance attestation: PASS.

The original physical-hardware live-edit symptom remains an explicit later hands-on regression because no physical FM-1 is currently available.

## 2026-10-28 - WP-B2 rotary display/handler parity complete

B2 fixed GLOBAL K4 MIDI OUT parity, Loop Cycle K3/K4 OCT/TRANS parity, truthful SLOTS footer state and added deterministic real-encoder page/layer coverage.

Checkpoint `85a3c376b47829af6d6abdd3264ec75cb37d42bf`; rotary gate `40599056435`, reconstruction `40599056475`, B1 regression `40599056403`, target `40599056945`: PASS.

## 2026-10-27 - WP-B1 live-edit audio reliability complete

B1 implemented odd/even pending-config publication, 32-bit config generations, SUB drive-curve preparation outside render, selective derived-state invalidation, host live-edit stress and real 80 MHz front-panel encoder stress.

Checkpoint `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`; live-edit `40596812842`, reconstruction `40596812864`, target `40596813400`: PASS.

## 2026-10-26 - WP-A randomizer intensity + RANDOM skyline complete

Implemented LOW/MED/HIGH randomizer intensity, direct MED compatibility, target-mask isolation, independent WAVE, no storage migration and full 4..32-step RANDOM skyline.

Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`; runs `40294234032` and `40294234527`: PASS.

## 2026-10-25 - approved next-iteration handoff

Product owner approved every recommendation from the 2026-10-25 product workshop. Documentation/handoff updates only.

## 2026-10-25 - post-audition product review

Added `docs/PRODUCT_WORKSHOP_2026-10-25.md` and the P0 trust/reliability backlog that became WP-B.

## 2026-10-23 - handoff checkpoint for new chat

Added root continuity files and documented the then-active Step 4 randomizer work.
