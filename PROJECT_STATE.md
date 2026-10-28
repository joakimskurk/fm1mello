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
- **E1 audit/baseline: COMPLETE.**
- **ACTIVE NEXT SUBTASK: E2 extreme bounded distortion.**

Do not begin WP-F or later work before WP-E is green and documented.

## Retained WP-D checkpoint

Final WP-D implementation/test checkpoint: `0cd810b5deb90087d60b181682de7e9ff52682cd`.

WP-D delivered musical `FREE / 1 / 2 / 4 / 8 / 16 BAR` reset with **2 BAR default**, DIV-invariant musical duration, retained 16-bit storage layout, safe malformed-value handling, explicit RESET language and separation from MIDI sync / Loop Cycle / HOLD.

Full target `40763918161`: PASS. RAM headroom `21472 B`; pool headroom `17696 B`; package SHA-256 `bee43ce894a5a26472270b0890f1d64e400bfb68246980f5fe285179a4225b7f`.

## WP-E1 final baseline

E1 baseline checkpoint: `b58dfb224f4037b27b6d232f02991274ed676504`.

Permanent assets:

- `tools/wp_e_audio_baseline.py`;
- `.github/workflows/wp-e-audio-audit.yml`;
- `docs/WP_E_AUDIO_AUDIT.md`.

E1 audio-audit run `40773113620`: **PASS**. The retained fast regression workflows on the same checkpoint completed without failure.

Reconstructed audio baseline hashes:

- `mello_audio.c` `3bfc64f6b101a8123b1533014fece746073bfac22055b83434252d8e0252ff85`;
- `mello_audio.h` `302e217d551d1f250269ca37216217cffd067656a7db371ad1dc91e7ec7c61b5`;
- `mello_engine.c` `daa00773838efe5d511c81afcdc84ba3e8bef42c6e08d368de5843c643f7670c`;
- `mello_engine.h` `769ea7e8b7a1be51d38bbd48e5eb09e39114dd79ca363dcb70b235e1483cdab8`;
- `mello_ui.c` `08bd19d7a2f112de318d50516c5a27b3ed2324d0c82bd50a087caadd0b7ecd55`.

E1 retained technical audio evidence:

- Reverb V2 IR dense `42497`, corr² `0.00075`, max block-energy jump `2.279`, predelay delta `5292`;
- 300 BPM / 8-ratchet timing stress: PASS;
- event ordering: PASS;
- live-edit host: energy `433436686798`, ticks `159600`, applies `2470`, SUB builds `67`, reverb reconfigs `135`: PASS.

## E1 findings that drive WP-E

### Distortion

Current distortion is intentionally mild relative to the WP-E target. `AMT` and `CHAR` become a derived drive capped at `800`; `AMT=500/CHAR=350` gives `127`. The actual waveshaper is called with `character=0`, so CHAR currently changes drive scaling rather than an independent transfer characteristic.

### Reverb V2

V2 is two serial all-pass diffusers feeding a static four-line Hadamard FDN with one fixed damping coefficient and one mono late-return combination. It has no late-tail delay modulation. That architecture can pass density/correlation tests while still exposing the metallic/ring-mod-like pitched identity heard in audition.

**Reverb is a defining Mello feature.** Reverb V3 is therefore a P0 product-quality gate inside WP-E. Objective metrics remain guardrails, but controlled listening decides acceptance. Metallic/pitched tail, repeating comb modes, flutter, grainy echoes, LF boom, brittle HF hang, live-edit clicks/zipper or a detached wet field are hard rejects.

### Ratchet

Every ratchet sub-trigger emits its own NOTE_ON/OFF pair, and every NOTE_ON reaches `lead_attack()`. Structural envelope retrigger parity is therefore present. E4 still needs musical/audible parity coverage rather than assuming structural parity is sufficient.

### Live smoothing

WP-B block-atomic config publication and selective heavy-DSP rebuilding are intact. There is no general continuous smoothing layer for live FX values; many mix/level/tone changes become active directly at the next config publication. E4 must address audible zipper/click/tail-discontinuity risks without render-thread allocation.

## Active WP-E direction

E2: extreme but bounded distortion, with a nuanced lower range and useful genuinely extreme upper range under gain/headroom control.

E3: Reverb V3 against `docs/WP_E_AUDIO_AUDIT.md`; prefer reuse of the existing reverb ring and spend budget first on perceptually useful diffusion, frequency-dependent damping, modal breakup/modulation and stereo decorrelation.

E4: ratchet audible parity + realtime-safe live smoothing.

Preserve all WP-A/WP-B/WP-C/WP-D gates and >=16 KiB RAM/pool headroom.

## Future sequence

WP-E Audio character -> WP-F Undo Last Random -> WP-G ANALOG/FM + SOUND randomization/per-step timbre -> WP-H harmonic layer decision -> WP-I polish/onboarding -> WP-J final integration.
