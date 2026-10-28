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
9. `docs/WP_E2_CLOSEOUT.md`
10. `docs/WP_D_CLOSEOUT.md`
11. `docs/WP_D_TIMING_AUDIT.md`
12. `docs/WP_C_CONTROL_AUDIT.md`
13. `docs/PRODUCT_WORKSHOP_2026-10-25.md`
14. `docs/UI_VISUAL_LANGUAGE.md` when UI is touched.

## Completed work packages

- WP-A Randomizer intensity + RANDOM skyline: **COMPLETE**.
- WP-B Trust and reliability: **COMPLETE**.
- WP-C Control Grammar v2: **COMPLETE**.
- WP-D Timing model: **COMPLETE 2026-10-28**.

## Active work package

**WP-E - Audio character.**

Do not begin WP-F early.

### E1 - audit/baseline - COMPLETE

E1 checkpoint: `b58dfb224f4037b27b6d232f02991274ed676504`. Audio audit `40773113620`: PASS.

The authoritative Reverb V3 perceptual/technical acceptance contract is `docs/WP_E_AUDIO_AUDIT.md`. Reverb is a defining Mello feature; a numerically green but metallic, pitched, fluttering or detached tail is rejected.

### E2 - extreme bounded distortion - COMPLETE

Final checkpoint: `cbbf44a68b64a75195383511a04f607b68562935`.

Closeout: `docs/WP_E2_CLOSEOUT.md`.

E2 delivered:

- cubic AMT mapping with nuanced lower/mid range and genuinely extreme upper range;
- independent CHAR waveshape control;
- bounded monotonic exact-symmetry transfer behavior;
- zero-AMT identity;
- no collateral change to legacy sample-slot `soft_drive()`.

WP-E audit `40776409911`: PASS. Full target `40776410416`: PASS. RAM headroom `21472 B`, pool headroom `17696 B`.

### Active next: E3 - Reverb V3

E3 must build a clearly better Mello reverb, not merely make V2 pass more metrics.

Priorities:

- higher modal/echo density and better early diffusion;
- frequency-dependent decay and LF/HF control;
- stereo-decorrelated late field;
- reduced stationary modal identity without audible chorus/vibrato artifacts;
- useful SIZE/DECAY/TONE behavior;
- controlled DRY/V2/V3 audition material covering the five E1 stimuli;
- all retained realtime/resource/regression gates.

E4 follows with audible ratchet parity and realtime-safe live smoothing.

### WP-E constraints

- preserve >=16 KiB RAM and pool headroom;
- preserve emulator/browser/80 MHz/Chromium/target gates;
- do not begin Undo/FM/SOUND/harmonic/onboarding work inside WP-E.

## Stop condition

When WP-E is fully green, record exact implementation checkpoint/run evidence, activate WP-F Undo Last Random, then stop/handoff rather than beginning WP-F automatically.

**Product rule: Keep the depth, reduce the number of grammars.**
