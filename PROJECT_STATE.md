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
- E1 audit/baseline: **COMPLETE**.
- E2 extreme bounded distortion: **COMPLETE**.
- **ACTIVE NEXT SUBTASK: E3 Reverb V3.**

Do not begin WP-F or later work before WP-E is green and documented.

## Retained WP-D checkpoint

Final WP-D implementation/test checkpoint: `0cd810b5deb90087d60b181682de7e9ff52682cd`.

WP-D delivered musical `FREE / 1 / 2 / 4 / 8 / 16 BAR` reset with **2 BAR default**, DIV-invariant musical duration, retained 16-bit storage layout, safe malformed-value handling, explicit RESET language and separation from MIDI sync / Loop Cycle / HOLD.

Full target `40763918161`: PASS. RAM headroom `21472 B`; pool headroom `17696 B`.

## WP-E1 retained baseline

E1 baseline checkpoint: `b58dfb224f4037b27b6d232f02991274ed676504`.

Permanent E1 assets are `tools/wp_e_audio_baseline.py`, `.github/workflows/wp-e-audio-audit.yml` and `docs/WP_E_AUDIO_AUDIT.md`. E1 audit `40773113620`: PASS.

E1 established that Reverb V2 is two serial all-pass diffusers into a static four-line Hadamard FDN with one fixed damping coefficient, static late delays and mono late return. Its objective IR metrics are good but do not override the audible metallic/ring-mod-like tail that reopened product acceptance.

**Reverb is a defining Mello feature.** Reverb V3 is a P0 product-quality gate. Metallic/pitched tail, repeating comb modes, flutter, grainy echoes, LF boom, brittle HF hang, live-edit clicks/zipper or a detached wet field are hard rejects. Controlled listening decides final acceptance.

## WP-E2 final closeout

Final E2 implementation/test checkpoint: `cbbf44a68b64a75195383511a04f607b68562935`.

Full closeout: `docs/WP_E2_CLOSEOUT.md`.

E2 delivered:

- cubic global AMT mapping with fine low/mid control and a genuinely extreme upper range;
- independent CHAR waveshape control instead of CHAR merely scaling derived drive;
- bounded monotonic transfer with exact odd symmetry and no foldback;
- zero-AMT exact identity;
- global-only change: legacy sample-slot `soft_drive()` remains unchanged;
- permanent deterministic transfer-grid coverage.

WP-E audio audit `40776409911`: PASS. Full target `40776410416`: PASS.

E2 target evidence:

- image `190124 / 581564 B`;
- RAM headroom `21472 B`;
- pool headroom `17696 B`;
- package `611545 B`, SHA-256 `95473c044eea9958265595efe292c8cbc8bce79d7473e4d472b93c9839ff24e6`;
- browser/WASM 100 MHz steady realtime `2.083x`;
- SEQ/HOME 80 MHz `2.212x`;
- Chord + Reverb V2 80 MHz `2.165x`;
- physical-encoder live-edit 80 MHz `2.159x`: PASS;
- Chromium keyboard/pointer/PLAY: PASS;
- provenance: PASS.

## Active WP-E3 direction

Build Reverb V3 against `docs/WP_E_AUDIO_AUDIT.md`, not against a single density/correlation score.

Current engineering direction before implementation:

- reuse the existing target reverb ring unless measurement proves otherwise;
- spend compute/memory first on materially higher modal/echo density and early diffusion;
- use an orthogonal/loss-controlled FDN structure with frequency-dependent decay rather than one shared fixed damping identity;
- produce genuinely decorrelated stereo late return instead of one mono return copied to both outputs;
- consider subtle time variance only where it improves modal breakup without audible chorus/vibrato or instability;
- retain safe predelay/SIZE/DECAY/TONE semantics and leave live-movement smoothing/crossfade work coordinated with E4;
- create controlled DRY/V2/V3 audition material for the five E1 stimuli and record perceptual notes before accepting V3.

E4 remains ratchet audible parity + realtime-safe live smoothing after E3.

Preserve all WP-A/WP-B/WP-C/WP-D gates and >=16 KiB RAM/pool headroom.

## Future sequence

WP-E Audio character -> WP-F Undo Last Random -> WP-G ANALOG/FM + SOUND randomization/per-step timbre -> WP-H harmonic layer decision -> WP-I polish/onboarding -> WP-J final integration.
