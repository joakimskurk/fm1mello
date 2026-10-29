# ROADMAP

This is the short handoff index. Detailed scope lives in `docs/NEXT_ITERATION_ROADMAP.md`; new chats start with `START_HERE.md`.

## Completed

### WP-A - Randomizer intensity + RANDOM skyline

Complete. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

### WP-B - Trust and reliability

Complete. Final implementation/test checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; full target `40604214660`: PASS.

### WP-C - Control Grammar v2

**COMPLETE 2026-10-28.** Final implementation/test checkpoint `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

### WP-D - Timing model

**COMPLETE 2026-10-28.** Final implementation/test checkpoint `0cd810b5deb90087d60b181682de7e9ff52682cd`. Full target `40763918161`: PASS. RAM headroom `21472 B`, pool headroom `17696 B`.

## Active implementation

### WP-E - Audio character - ACTIVE

Goal: make Mello substantially more characterful without sacrificing realtime trust or the stabilized control/timing grammars.

#### E1 - audit + deterministic/perceptual baseline - COMPLETE

Checkpoint `b58dfb224f4037b27b6d232f02991274ed676504`. Audio-audit `40773113620`: PASS.

Permanent baseline: `tools/wp_e_audio_baseline.py`, `.github/workflows/wp-e-audio-audit.yml`, `docs/WP_E_AUDIO_AUDIT.md`.

Reverb V3 remains a defining product-quality gate: metallic/pitched/fluttering/detached tails are rejected even if numerical metrics pass.

#### E2 - extreme bounded distortion - COMPLETE

Final checkpoint `cbbf44a68b64a75195383511a04f607b68562935`. Closeout: `docs/WP_E2_CLOSEOUT.md`.

Delivered a cubic AMT range with nuanced low/mid control and extreme top end, an independent CHAR transfer-shape dimension, bounded/monotonic exact-symmetry behavior, and isolation from legacy sample-slot distortion.

WP-E audit `40776409911`: PASS. Full target `40776410416`: PASS. RAM headroom `21472 B`, pool headroom `17696 B`, package SHA-256 `95473c044eea9958265595efe292c8cbc8bce79d7473e4d472b93c9839ff24e6`.

#### E3 - Reverb V3 - ACTIVE NEXT

Build against the E1 perceptual contract. Prioritize substantially higher modal/echo density, strong early diffusion, frequency-dependent decay, useful SIZE/DECAY/TONE behavior, stereo decorrelation and non-stationary modal identity only where it does not create chorus/vibrato artifacts.

Reuse the current reverb ring unless measured evidence justifies more memory. Create controlled DRY/V2/V3 audition assets for bright pluck, staccato lead, sustained chord-bed, bass/sub-rich material and dense rhythmic/ratcheted material. Numerical tests are guardrails; controlled listening decides acceptance.

#### E4 - ratchet parity + realtime-safe live smoothing

Add audible/musical ratchet parity evidence and eliminate zipper/click/tail-discontinuity risks from live audio-character changes without render-thread allocation.

### WP-E closeout

Rerun all WP-A/WP-B/WP-C/WP-D gates plus audio-specific deterministic/perceptual A/B, emulator/browser, 80 MHz, Chromium and target build. Require >=16 KiB RAM and pool headroom. Record exact evidence, activate WP-F, then stop/handoff.

## Approved sequence after WP-E

- WP-F Undo Last Random.
- WP-G ANALOG/FM + SOUND randomization + realtime-safe per-step timbre.
- WP-H harmonic layer decision: HARMONY first, PAD second.
- WP-I instrument polish/onboarding.
- WP-J final integration/performance gate.

**Product rule: Keep the depth, reduce the number of grammars.**
