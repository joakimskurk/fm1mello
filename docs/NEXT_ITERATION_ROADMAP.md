# SkurkyMello FM-1 — next stabilization iteration

Baseline: `27c76df24101b3c8b99a3895648880d4c0971d3c`.

This iteration is deliberately stabilization-first. No new large feature is added until the steps below are completed in order.

## 1. Reverb overhaul — COMPLETE

Completed in `58ea689b282d00dd436b790be3bcf95b91e913f3`. Reverb V2 uses a separate pre-delay, two all-pass diffusers and a four-line Hadamard FDN, while retaining the HTML-derived send curve, wet tone LPF, 24 Hz HPF and return-level model. The first full target gate measured `corr²=0.00075`, block-energy jump `2.279`, exact 120 ms pre-delay shift of 5292 samples, and `1.833x` steady realtime on the 80 MHz chord/reverb stress path. Target pool remained within limits with 17696 B headroom.

Original problem: the previous FM-1 reverb could sound metallic/ring-mod-like or like a very short delay. This was a stop-ship quality issue for the FX chain.

Source reference: `skurkymello.html` uses `Tone.Reverb` when available, with `Tone.Freeverb` only as fallback. Its wet path is:

`post-dist signal -> reverb send -> reverb -> 24 dB tone LPF -> 24 Hz 24 dB HPF -> return gain -> FX mix -> -1 dB limiter`

HTML control model to preserve:

- Send: 0..1, using the hotter nonlinear send curve from the HTML.
- Room size: 0..1.
- Decay: 0.4..60 s.
- Pre-delay: 0..120 ms.
- Tone: approximately 700..12600 Hz on the HTML logarithmic curve.
- Level: 0..3x before the final limiter.
- Freeverb fallback damping reference: 4200 Hz.

Decision: Reverb V2 passed all hard gates and was positively auditioned on the FM-1 emulator. Keep it as the stabilized reverb baseline.

## 2. Chord Machine final go/no-go — PAUSED

Technical acceptance is complete. The held `ARP_UP` path traverses three chord tones monophonically (`12 note-ons / 12 note-offs / 3 distinct pitches`), the chord voice inhabits the shared Reverb V2 room, timing stress remains ordered, and the feature still compiles out cleanly with `MELLO_FEATURE_CHORD_MACHINE=0`.

Product decision: paused after audition. The feature is not being removed, but no further design/development effort should be spent on it in this stabilization iteration. Existing feature-on and compile-out tests remain as regression protection. Resume only after an explicit future decision to revisit Chord Machine.

## 3. UI performance + universal sequence overview — COMPLETE

Completed in `00c6c72ff5a2e7fda1f6f95d0add7b00fa37f171`.

The final reconstructed UI already had one shared 4..32-step geometry across HOME, STEP, SEGMENT and RANDOM. The remaining issue was redraw cost, not missing overview geometry.

Implemented:

- STEP and RANDOM now redraw only the previous/current playhead cells during normal transport movement.
- Static/edit pages no longer redraw the full body merely because the transport advanced.
- Dynamic sequencer pages retain full body redraw where their own playback state can change.
- All 32 main steps remain visible by thinning columns rather than paging the visual overview.
- Hardware edit banking remains separate from visual overview.

Measured host redraw gates:

- HOME incremental playhead: `9 fills / 1 text`.
- STEP incremental playhead: `15 fills / 1 text`.
- RANDOM incremental playhead: `9 fills / 1 text`.
- Static FX page during transport: `1 fill / 1 text`.

80 MHz SEQ/HOME playback benchmark:

- `3.023x` steady realtime.
- steady render p95 `2.859 ms`, p99 `4.451 ms`, max `7.061 ms`.
- target RAM headroom `21984 B`; pool headroom `17696 B`.

Decision: step 3 passed its hard performance and visibility gates.

## 4. Randomizer intensity — NEXT

Add a musical depth control independent of the selected random targets:

- LOW: conservative variation around the current pattern.
- MED: current/default creative range.
- HIGH: larger structural and timbral movement.

Intensity must constrain target-specific mutation ranges rather than merely reduce the probability that a target changes. `WAVE` remains an independent selectable target. MED should preserve the existing randomizer's character as closely as practical so this core creative function is not weakened while LOW/HIGH are added.

## 5. Undo last random

Store one pre-random snapshot sufficient to restore the immediately previous randomization.

Requirements:

- one-level undo only;
- cheap in RAM/storage;
- restores every selected random target touched by the last operation, including synth waveform when WAVE was selected;
- no interference with normal project/slot persistence.

## 6. Final integration/polish gate

After steps 1–5:

- rerun text-fit/clipping validation across all pages;
- rerun control-grammar tests;
- rerun HTML audio/sub parity tests;
- keep chord feature-on and compile-out regression tests while Chord Machine is paused;
- target build, emulator audio, 100 MHz browser, 80 MHz worst-case and Chromium E2E;
- inspect RAM, pool and image headroom before declaring the iteration complete.

## Deferred, not part of this stabilization iteration

- Four factory samples baked into firmware: implement when the source audio files are supplied.
- Further oscillator anti-aliasing/Tone.js envelope refinement: only after the reverb, randomizer and UI-performance work above are stable.
- Chord Machine product/design iteration: paused until explicitly resumed.
- New large sequencing features: no new scope until this roadmap is complete.
