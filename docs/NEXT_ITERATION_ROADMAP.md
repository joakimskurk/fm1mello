# SkurkyMello FM-1 - stabilization and product-evolution roadmap

Approved direction: 2026-10-25. Updated after WP-E1 audio audit: 2026-10-28.

This roadmap is stabilization-first. Approval of later product direction is not permission to implement later work packages early.

See `START_HERE.md` for exact continuation procedure, `PROJECT_STATE.md` for verified checkpoint, `CONTROL_GRAMMAR.md` for normative control rules, `docs/WP_E_AUDIO_AUDIT.md` for the current audio baseline and Reverb V3 acceptance contract, `docs/WP_D_CLOSEOUT.md` for final timing evidence and `TESTING.md` for acceptance evidence.

## WP-A. Randomizer intensity + RANDOM skyline - COMPLETE

Completed 2026-10-26. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

## WP-B. Trust and reliability - COMPLETE

Completed 2026-10-28. Final checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; target `40604214660`: PASS.

## WP-C. Control Grammar v2 - COMPLETE

Completed 2026-10-28. Final implementation/test checkpoint `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

## WP-D. Timing model - COMPLETE

Completed 2026-10-28. Final implementation/test checkpoint `0cd810b5deb90087d60b181682de7e9ff52682cd`.

Delivered `FREE / 1 / 2 / 4 / 8 / 16 BAR`, **2 BAR default**, musical BAR duration across DIV, retained storage layout and explicit timing-language separation. Full target `40763918161`: PASS; RAM headroom `21472 B`, pool headroom `17696 B`.

## WP-E. Audio character - ACTIVE

Goal: make Mello more characterful without sacrificing realtime trust, stabilized control grammar or timing semantics.

### E1. Audit and deterministic/perceptual baseline - COMPLETE

Checkpoint `b58dfb224f4037b27b6d232f02991274ed676504`. Audio-audit run `40773113620`: PASS.

Permanent E1 assets:

- `tools/wp_e_audio_baseline.py`;
- `.github/workflows/wp-e-audio-audit.yml`;
- `docs/WP_E_AUDIO_AUDIT.md`.

E1 mapped current distortion transfer/range, Reverb V2 topology/budgets, ratchet scheduling/envelope retrigger, live FX update paths, controls and existing technical tests.

Key baseline findings:

- distortion is currently conservative; derived drive is capped at `800` and CHAR changes drive scaling rather than independently shaping the waveshaper;
- Reverb V2 is predelay + two all-pass diffusers + a static four-line Hadamard FDN with fixed damping and a mono late return;
- V2 has no late-tail delay modulation, leaving a credible architectural cause for the metallic/ring-mod-like pitched tail heard in audition despite good objective metrics;
- ratchets already emit NOTE_ON/OFF per sub-trigger and each NOTE_ON re-enters lead attack;
- config publication is coherent but there is no general continuous smoothing layer for live FX parameters.

#### Reverb V3 product requirement

**Reverb is a defining part of Mello.** E3 is not complete until V3 is clearly preferred to V2 in controlled listening. Persistent metallic/pitched tail, comb repetition/flutter, grainy sparse echoes, LF boom, brittle HF hang, live-edit zipper/clicks or detached dry/wet integration are hard rejects. Metrics are necessary guardrails, not acceptance authority.

The full A/B protocol and objective constraints live in `docs/WP_E_AUDIO_AUDIT.md`.

### E2. Extreme bounded distortion - ACTIVE NEXT

Expand the usable range so the upper end becomes intentionally extreme while the lower range remains nuanced and controllable.

Requirements:

- bounded transfer at all legal settings;
- meaningful character change rather than only more gain;
- gain compensation/headroom so louder is not automatically perceived as better;
- no unsafe DC/runaway/clipping behavior;
- realtime-safe live changes;
- preserve all WP-A/B/C/D and E1 gates.

### E3. Reverb V3

Design specifically for subjective quality under FM-1 constraints. Priorities:

- fast, smooth early diffusion;
- non-stationary late modal identity rather than fixed ringing;
- frequency-dependent decay/damping;
- musical SIZE/DECAY/TONE/PRE behavior;
- safe live parameter movement;
- useful subtle and atmospheric WET/LEVEL range;
- preferably a decorrelated stereo late field using shared state rather than automatically doubling delay memory.

Prefer reuse of the existing `24576`-sample reverb ring. Spend CPU/state first on perceptually useful diffusion, damping, modulation and stereo decorrelation. Require retained >=1.35x worst-case 80 MHz steady realtime margin and >=16 KiB RAM/pool headroom.

### E4. Ratchet parity + realtime-safe live smoothing

Add audible/musical ratchet parity evidence and smooth/crossfade live audio-character parameters where direct block-level value changes can cause zipper noise, clicks or tail discontinuities. No render-thread allocation.

### WP-E closeout

Rerun all WP-A/WP-B/WP-C/WP-D/E1 gates plus audio-specific deterministic/perceptual A/B, emulator/browser, 80 MHz, Chromium and target build. Require >=16 KiB RAM and pool headroom. Record exact evidence, activate WP-F, then stop/handoff.

### WP-E exclusions

No Undo Last Random, ANALOG/FM expansion, SOUND randomization, per-step timbre, harmonic-layer replacement or general onboarding work inside WP-E.

## WP-F. Undo Last Random

One-level pre-random snapshot after control/timing/audio stabilization.

## WP-G. Synth evolution

Preserve ANALOG, add curated FM under budgets, independent SOUND randomization and realtime-safe per-step timbre.

## WP-H. Harmonic layer decision

Prototype scale-aware HARMONY first, PAD second; current Chord remains regression-protected until a successor wins.

## WP-I. Instrument polish and onboarding

Splash, HOME hierarchy, Key/Scale, SEQ explanatory hierarchy, contextual hints and newcomer usability.

## WP-J. Final integration gate

Full control, timing, randomizer, audio, visual, feature-flag, target, emulator/browser, memory/headroom and hands-on product audition gates.
