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
8. `docs/WP_E_AUDIO_AUDIT.md`
9. `docs/WP_D_CLOSEOUT.md`
10. `docs/WP_D_TIMING_AUDIT.md`
11. `docs/WP_C_CONTROL_AUDIT.md`
12. `docs/PRODUCT_WORKSHOP_2026-10-25.md`
13. `docs/UI_VISUAL_LANGUAGE.md` when UI is touched.

## Completed work packages

- WP-A Randomizer intensity + RANDOM skyline: **COMPLETE**.
- WP-B Trust and reliability: **COMPLETE**.
- WP-C Control Grammar v2: **COMPLETE**.
- WP-D Timing model: **COMPLETE 2026-10-28**.

Final WP-C implementation/test checkpoint: `2c280184b0c5cd0947f2d4d9149ad12a96188709`.
Final WP-D implementation/test checkpoint: `0cd810b5deb90087d60b181682de7e9ff52682cd`.

## WP-D retained result

- main-sequence timing choices `FREE / 1 / 2 / 4 / 8 / 16 BAR`;
- default **2 BAR**;
- BAR count remains musical across sequence `DIV` changes;
- existing 16-bit `master_len` storage layout retained with safe malformed-value handling;
- main reset remains separate from external/MIDI sync, Loop Cycle `SYNC/FREE` and Slot/Chain HOLD;
- SEQ uses explicit `RESET` + BAR/FREE language.

Final WP-D target `40763918161`: PASS. RAM headroom `21472 B`, pool headroom `17696 B`, package SHA-256 `bee43ce894a5a26472270b0890f1d64e400bfb68246980f5fe285179a4225b7f`.

## Active work package

**WP-E - Audio character.**

Do not begin WP-F early.

### E1 - audit/baseline - COMPLETE

E1 checkpoint: `b58dfb224f4037b27b6d232f02991274ed676504`.

Permanent E1 assets:

- `tools/wp_e_audio_baseline.py`;
- `.github/workflows/wp-e-audio-audit.yml`;
- `docs/WP_E_AUDIO_AUDIT.md`.

E1 audio-audit run `40773113620`: PASS.

Key findings:

- current distortion is conservative: `AMT/CHAR` map to a derived drive capped at `800/1000`; CHAR does not independently shape the current waveshaper;
- Reverb V2 is 2 serial all-pass stages into a **static four-line Hadamard FDN**, with one fixed damping coefficient and one mono late-return combination;
- Reverb V2 has no late-tail delay modulation, so its good density/correlation metrics do not prevent a persistent metallic/pitched tail;
- ratchets structurally emit a NOTE_ON/OFF pair per sub-trigger and each NOTE_ON re-enters `lead_attack()`;
- WP-B config publication is coherent, but there is **no dedicated continuous smoothing layer for live FX parameters** yet.

### Reverb is a defining Mello feature

Reverb V3 is a P0 product-quality gate inside WP-E, not a utility-effect cleanup. It must be clearly preferred to V2 in controlled listening. A numerically green but metallic, pitched, fluttering or detached tail is a rejection.

The authoritative perceptual and technical acceptance contract is `docs/WP_E_AUDIO_AUDIT.md`. Minimum A/B stimuli are bright pluck/transient, staccato lead, sustained chord-bed, bass/sub-rich material and dense rhythmic/ratcheted material.

### Active next: E2 - extreme bounded distortion

E2 must:

- make the upper range genuinely extreme while preserving a controllable lower range;
- prevent the result from being merely louder through gain/headroom management;
- remain bounded and realtime-safe;
- preserve the full WP-A/WP-B/WP-C/WP-D regression matrix.

Then E3 builds **Reverb V3** against the E1 listening contract. E4 closes audible ratchet parity and realtime-safe live smoothing.

### WP-E constraints

- preserve >=16 KiB RAM and pool headroom;
- preserve emulator/browser/80 MHz/Chromium/target gates;
- do not begin Undo/FM/SOUND/harmonic/onboarding work inside WP-E.

## Stop condition

When WP-E is fully green, record exact implementation checkpoint/run evidence, activate WP-F Undo Last Random, then stop/handoff rather than beginning WP-F automatically.

**Product rule: Keep the depth, reduce the number of grammars.**
