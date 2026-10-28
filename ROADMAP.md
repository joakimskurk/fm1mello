# ROADMAP

This is the short handoff index. Detailed scope lives in `docs/NEXT_ITERATION_ROADMAP.md`; new chats start with `START_HERE.md`.

## Completed

### WP-A - Randomizer intensity + RANDOM skyline

Complete. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

### WP-B - Trust and reliability

Complete. Final implementation/test checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; full target `40604214660`: PASS.

### WP-C - Control Grammar v2

**COMPLETE 2026-10-28.** Final implementation/test checkpoint `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

C1-C5 delivered REC/EDIT cleanup, visible stable B0-B7 contextual actions, coherent MOD navigation and one authoritative `MelloContextAction` descriptor shared by handler semantics and rendered guidance.

Final target `40703571071`: PASS. RAM headroom `21472 B`; pool headroom `17696 B`.

### WP-D - Timing model

**COMPLETE 2026-10-28.** Final implementation/test checkpoint `0cd810b5deb90087d60b181682de7e9ff52682cd`.

Delivered:

- deterministic pre-change timing audit and permanent baseline;
- `FREE / 1 / 2 / 4 / 8 / 16 BAR` main reset with **2 BAR default**;
- musical BAR duration independent of sequence `DIV`;
- unchanged 16-bit storage layout with safe malformed-value handling;
- explicit `RESET` BAR/FREE UI language;
- external/MIDI sync, Loop Cycle `SYNC/FREE` and Slot/Chain HOLD remain separate timing concepts.

Exact generated locks: engine C `daa00773838efe5d511c81afcdc84ba3e8bef42c6e08d368de5843c643f7670c`, engine H `769ea7e8b7a1be51d38bbd48e5eb09e39114dd79ca363dcb70b235e1483cdab8`, UI `08bd19d7a2f112de318d50516c5a27b3ed2324d0c82bd50a087caadd0b7ecd55`.

All 11 fast workflows PASS; full target `40763918161`: PASS. RAM headroom `21472 B`, pool headroom `17696 B`, image `190156 B`, package SHA-256 `bee43ce894a5a26472270b0890f1d64e400bfb68246980f5fe285179a4225b7f`.

Full closeout: `docs/WP_D_CLOSEOUT.md`.

## Active implementation

### WP-E - Audio character - ACTIVE

Approved scope:

- extreme bounded distortion;
- Reverb V3 with perceptual A/B acceptance;
- ratchet parity;
- realtime-safe live smoothing.

Immediate step is **E1 audit and deterministic/perceptual baseline**. Map current audio behavior, parameter/control paths, realtime update paths, CPU/memory budgets and existing tests before changing sound.

Preserve all WP-A/WP-B/WP-C/WP-D gates, emulator/browser/80 MHz/Chromium/target validation and >=16 KiB RAM/pool headroom.

Stop after WP-E closeout and activate WP-F; do not jump ahead.

## Approved sequence after WP-E

- WP-F Undo Last Random.
- WP-G ANALOG/FM + SOUND randomization + realtime-safe per-step timbre.
- WP-H harmonic layer decision: HARMONY first, PAD second.
- WP-I instrument polish/onboarding.
- WP-J final integration/performance gate.

**Product rule: Keep the depth, reduce the number of grammars.**
