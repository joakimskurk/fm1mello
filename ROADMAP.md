# ROADMAP

This is the short handoff index. Detailed scope lives in `docs/NEXT_ITERATION_ROADMAP.md`; new chats start with `START_HERE.md`.

## Completed

### WP-A - Randomizer intensity + RANDOM skyline

Complete. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

### WP-B - Trust and reliability

Complete. Final implementation/test checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; full target `40604214660`: PASS.

### WP-C - Control Grammar v2

**COMPLETE 2026-10-28.** Final implementation/test checkpoint `2c280184b0c5cd0947f2d4d9149ad12a96188709`. REC/EDIT semantics, visible B0-B7 contextual rail, coherent MOD navigation and authoritative action mapping remain regression-protected.

### WP-D - Timing model

**COMPLETE 2026-10-28.** Final implementation/test checkpoint `0cd810b5deb90087d60b181682de7e9ff52682cd`.

Delivered `FREE / 1 / 2 / 4 / 8 / 16 BAR`, **2 BAR default**, musical duration across DIV, retained storage layout, explicit RESET language and separation from MIDI sync / Loop Cycle / HOLD.

Full target `40763918161`: PASS. RAM headroom `21472 B`, pool headroom `17696 B`.

## Active implementation

### WP-E - Audio character - ACTIVE

Goal: make Mello substantially more characterful without sacrificing realtime trust or the stabilized control/timing grammars.

#### E1 - audit + deterministic/perceptual baseline - COMPLETE

Checkpoint `b58dfb224f4037b27b6d232f02991274ed676504`. Audio-audit `40773113620`: PASS.

Permanent baseline: `tools/wp_e_audio_baseline.py`, `.github/workflows/wp-e-audio-audit.yml`, `docs/WP_E_AUDIO_AUDIT.md`.

Findings:

- current distortion upper range is conservative and CHAR is not yet an independent waveshaper dimension;
- Reverb V2 is a static four-line FDN after two all-pass diffusers, with fixed damping and mono late return;
- V2 has no late-tail delay modulation, so technical density/correlation success does not resolve the audible metallic/pitched tail;
- ratchet scheduling already sends NOTE_ON/OFF per sub-trigger and re-enters lead attack;
- FX snapshots are coherently published but lack a general continuous live-parameter smoothing layer.

**Reverb V3 is a defining Mello product-quality gate.** The E1 A/B contract is authoritative: a metallic, pitched, fluttering or detached tail is a hard reject even if numerical tests are green.

#### E2 - extreme bounded distortion - ACTIVE NEXT

Expand distortion into a genuinely extreme upper range while keeping low settings nuanced, bounded and gain-aware. Preserve control semantics, realtime safety and all retained gates.

#### E3 - Reverb V3

Build against the E1 perceptual contract. Prioritize early diffusion, non-stationary modal identity, frequency-dependent decay, musical SIZE/DECAY/TONE behavior, safe live movement and stereo decorrelation where budget permits. Reuse the existing reverb ring unless measured evidence justifies more memory.

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
