# CHANGELOG

## 2026-10-28 - WP-B2 rotary display/handler parity complete

B2 is complete and WP-B continues with B3 display integrity.

Implemented:

- GLOBAL CLOCK K4 footer now renders `MIDI OUT` and the same ON/OFF state the physical rotary changes;
- Loop Cycle STEP K3/K4 footer now matches physical handlers: K3=`OCT`, K4=`TRANS`;
- SLOTS footer no longer represents inactive K1 as a fake parameter, and CHAR shows the edited character;
- permanent `tools/rotary_mapping_parity.c` fixture drives actual encoder input across every current page/layer;
- dedicated CI validates Chord feature-on and compile-out variants;
- deterministic reconstruction locks the B2 source patch and mapping fixture.

Validation checkpoint: `85a3c376b47829af6d6abdd3264ec75cb37d42bf`.

- rotary parity run `40599056435`: PASS;
- Chord enabled: `180` mappings across `45` fixtures: PASS;
- Chord disabled: `160` mappings across `40` fixtures: PASS;
- reconstruction run `40599056475`: PASS;
- B1 live-edit regression run `40599056403`: PASS;
- full target run `40599056945`: PASS;
- browser/WASM 100 MHz `1.663x` steady realtime;
- SEQ/HOME 80 MHz `1.776x`;
- Chord + Reverb V2 80 MHz `1.760x`;
- live-edit encoder stress 80 MHz `1.707x`, final-second audio still present;
- RAM headroom `21472 B`; pool headroom `17696 B`;
- package SHA-256 `26ccf3716c4b15b3671e9b4d6d91cf5f162ec1e1e2f692ba486a70c0ea8cd8f8`.

B3 is now active: text-fit/overlap at legal extremes, the reported HOME pink-line artifact over `SINGLE`, and deterministic stale/dirty-region coverage.

## 2026-10-27 - WP-B1 live-edit audio reliability complete

B1 is complete on the currently available host/emulator/browser/target evidence. WP-B remains active.

Implemented odd/even pending-config publication, 32-bit config generations, SUB drive-curve preparation outside render, selective derived-state invalidation, host live-edit stress and real 80 MHz front-panel encoder stress.

Validation checkpoint: `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`.

- dedicated live-edit run `40596812842`: PASS;
- reconstruction run `40596812864`: PASS;
- full target run `40596813400`: PASS;
- 2400-block host stress: no silence window, queue overflow or transport stall;
- 32768 coalesced config publications: PASS;
- 80 MHz physical encoder stress: `1.673x` steady realtime;
- final one-second stress tail: `88168` nonzero samples, peak `3902`;
- RAM headroom `21472 B`; pool headroom `17696 B`.

The exact originally reported physical FM-1 freeze cannot be hands-on re-auditioned until hardware is available. That limitation is retained explicitly in `TESTING.md`.

## 2026-10-26 - WP-A randomizer intensity + RANDOM skyline complete

WP-A / Step 4 is complete and handed off.

Implemented LOW/MED/HIGH randomizer intensity, direct MED compatibility, target-mask isolation, independent WAVE, no storage migration and the full 4..32-step RANDOM skyline.

Validation:

- implementation commit `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`;
- clean reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`;
- reconstruction run `40294234032`: PASS;
- target closeout run `40294234527`: PASS;
- `LOW=194824 < MED=516917 < HIGH=1082367`;
- RANDOM incremental redraw `4 fills / 0 texts`;
- RAM headroom `21984 B`; pool headroom `17696 B`.

## 2026-10-25 - approved next-iteration handoff

Product owner approved every recommendation from the 2026-10-25 product workshop. Documentation/handoff updates only; no firmware implementation changed in this approval checkpoint.

## 2026-10-25 - post-audition product review

Documentation/planning checkpoint after extended hands-on emulator testing. Added `docs/PRODUCT_WORKSHOP_2026-10-25.md` and the P0 trust/reliability backlog that became WP-B.

## 2026-10-23 - handoff checkpoint for new chat

Added the root continuity files and documented the then-active Step 4 randomizer work plus its intentional diagnostic reconstruction failure.
