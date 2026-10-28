# START HERE - FM-1 Mello continuation

Use this file at the start of every new ChatGPT work session on `joakimskurk/fm1mello`.

## Repository

- Repo: `joakimskurk/fm1mello`
- Branch: `visual-port-home`
- Always verify current branch HEAD before editing.
- Repo documentation is source of truth; do not reconstruct direction from chat memory.
- Firmware is reconstructed by `ci/unpack_source.sh`; generated `mello/` is not ordinary committed source.

## Read in order

1. `START_HERE.md`
2. `PROJECT_STATE.md`
3. `ROADMAP.md`
4. `DECISIONS.md`
5. `CONTROL_GRAMMAR.md`
6. `TESTING.md`
7. `docs/NEXT_ITERATION_ROADMAP.md`
8. `docs/WP_D_CLOSEOUT.md`
9. `docs/WP_D_TIMING_AUDIT.md`
10. `docs/WP_C_CONTROL_AUDIT.md`
11. `docs/PRODUCT_WORKSHOP_2026-10-25.md`
12. `docs/UI_VISUAL_LANGUAGE.md` when UI is touched.

## Completed work packages

- WP-A Randomizer intensity + RANDOM skyline: **COMPLETE**.
- WP-B Trust and reliability: **COMPLETE**.
- WP-C Control Grammar v2: **COMPLETE**.
- WP-D Timing model: **COMPLETE 2026-10-28**.

Final WP-C implementation/test checkpoint: `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

Final WP-D implementation/test checkpoint: `0cd810b5deb90087d60b181682de7e9ff52682cd`.

## WP-D final result

- main-sequence timing choices are `FREE / 1 BAR / 2 BAR / 4 BAR / 8 BAR / 16 BAR`;
- project default is **`2 BAR`**;
- BAR duration is musical and remains the selected number of bars across sequence `DIV` changes;
- the existing 16-bit `master_len` storage field/layout is retained, with canonical stored values and safe handling of malformed noncanonical values;
- `FREE` keeps natural sequence wrap behavior;
- main-sequence bar reset remains separate from external/MIDI sync;
- Loop Cycle `SYNC/FREE` remains separate;
- Slot/Chain HOLD remains separate;
- the SEQ UI uses explicit `RESET` + BAR/FREE language rather than collapsing timing concepts into generic `SYNC`.

Exact generated WP-D locks:

- `mello/mello_engine.c`: `daa00773838efe5d511c81afcdc84ba3e8bef42c6e08d368de5843c643f7670c`;
- `mello/mello_engine.h`: `769ea7e8b7a1be51d38bbd48e5eb09e39114dd79ca363dcb70b235e1483cdab8`;
- `mello/mello_ui.c`: `08bd19d7a2f112de318d50516c5a27b3ed2324d0c82bd50a087caadd0b7ecd55`.

Final WP-D regression evidence on `0cd810b...`:

- all 11 fast workflows: PASS;
- full target `40763918161`: PASS;
- RAM headroom `21472 B`;
- pool headroom `17696 B`;
- image `190156 / 581564 B`;
- package `611545 B`;
- package SHA-256 `bee43ce894a5a26472270b0890f1d64e400bfb68246980f5fe285179a4225b7f`;
- provenance attestation: PASS.

See `docs/WP_D_CLOSEOUT.md` and `TESTING.md` for the complete matrix and measurements.

Physical FM-1 hands-on timing/live-edit validation remains deferred until hardware is available.

## Active work package

**WP-E - Audio character.**

Do not begin WP-F early.

### WP-E approved scope

- extreme but bounded distortion;
- Reverb V3 with perceptual A/B acceptance, not metrics alone;
- ratchet parity;
- realtime-safe live smoothing.

### WP-E start procedure

1. E1 audit/baseline first: map current distortion, Reverb V2, ratchet behavior, smoothing paths, controls, CPU/memory cost and existing tests.
2. Add deterministic/perceptual acceptance criteria before changing audio behavior.
3. Implement one audio-character change at a time behind the retained WP-A/WP-B/WP-C/WP-D gates.
4. Preserve >=16 KiB RAM and pool headroom and all emulator/browser/80 MHz/target gates.
5. Do not begin Undo/FM/SOUND/harmonic/onboarding work inside WP-E.

## Stop condition

When WP-E is fully green, record exact implementation checkpoint/run evidence, activate WP-F Undo Last Random, then stop/handoff rather than beginning WP-F automatically.

**Product rule: Keep the depth, reduce the number of grammars.**
