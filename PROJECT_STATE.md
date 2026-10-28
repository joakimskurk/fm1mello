# PROJECT STATE

Last verified work-package closeout: 2026-10-28

## Repository

- Repo: `joakimskurk/fm1mello`
- Active branch: `visual-port-home`
- Verify current branch HEAD before editing.
- New chats start with `START_HERE.md`; repo documentation is source of truth.
- Firmware is reconstructed by `ci/unpack_source.sh` from source chunks plus ordered transforms/tests.

## Work-package position

- WP-A Randomizer intensity + skyline: **COMPLETE**.
- WP-B Trust and reliability: **COMPLETE**.
- WP-C Control Grammar v2: **COMPLETE**.
- WP-D Timing model: **COMPLETE**.
- **ACTIVE WORK PACKAGE: WP-E Audio character.**

Do not begin WP-F or later work before WP-E is green and documented.

## WP-D final implementation

Final implementation/test checkpoint: `0cd810b5deb90087d60b181682de7e9ff52682cd`.

WP-D delivered:

- D1 deterministic timing audit/baseline for legacy main reset, transport, Loop Cycle and HOLD separation;
- D2 compatible musical timing model `FREE / 1 / 2 / 4 / 8 / 16 BAR`, with **2 BAR default**;
- BAR duration remains musical across sequence `DIV` changes;
- existing 16-bit `master_len` storage layout retained;
- malformed noncanonical stored timing values cannot collapse runtime reset length to zero;
- D3 explicit `RESET` BAR/FREE UI language while MIDI sync, Loop Cycle `SYNC/FREE` and Slot/Chain HOLD remain independent concepts;
- D4 full retained WP-A/WP-B/WP-C/audio/timing/emulator/browser/80 MHz/Chromium/target regression closeout.

Exact WP-D generated locks:

- engine C `daa00773838efe5d511c81afcdc84ba3e8bef42c6e08d368de5843c643f7670c`;
- engine header `769ea7e8b7a1be51d38bbd48e5eb09e39114dd79ca363dcb70b235e1483cdab8`;
- UI `08bd19d7a2f112de318d50516c5a27b3ed2324d0c82bd50a087caadd0b7ecd55`.

## Final WP-D evidence

Fast workflows on `0cd810b...`:

- display integrity `40763919218`: PASS;
- authoritative actions `40763917759`: PASS;
- WP-D timing model `40763917598`: PASS;
- live-edit `40763917508`: PASS;
- reconstruction `40763917460`: PASS;
- context grammar `40763917641`: PASS;
- rotary parity `40763917555`: PASS;
- control-action contract `40763917660`: PASS;
- REC grammar `40763917588`: PASS;
- EDIT grammar `40763917591`: PASS;
- control audit `40763917470`: PASS.

Full target `40763918161`: PASS.

Target closeout:

- image `190156 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `611545 B`;
- package SHA-256 `bee43ce894a5a26472270b0890f1d64e400bfb68246980f5fe285179a4225b7f`;
- provenance attestation: PASS.

Physical FM-1 hands-on timing/live-edit reproduction remains deferred until hardware is available; host/emulator/browser/target evidence is green.

See `docs/WP_D_CLOSEOUT.md` and `TESTING.md` for detailed measurements.

## Active WP-E direction

WP-E is the Audio character package. Approved scope:

- extreme bounded distortion;
- Reverb V3 with perceptual A/B;
- ratchet parity;
- realtime-safe live smoothing.

Start with an E1 audit/baseline. Do not change audio behavior until current distortion/Reverb V2/ratchet/smoothing paths, controls, budgets and deterministic gates are mapped. Preserve all WP-A/WP-B/WP-C/WP-D gates and >=16 KiB RAM/pool headroom.

## Future sequence

WP-E Audio character -> WP-F Undo Last Random -> WP-G ANALOG/FM + SOUND randomization/per-step timbre -> WP-H harmonic layer decision -> WP-I polish/onboarding -> WP-J final integration.
