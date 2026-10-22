# SkurkyMello FM-1 — next stabilization iteration

Baseline: `27c76df24101b3c8b99a3895648880d4c0971d3c`.

This iteration is deliberately stabilization-first. No new large feature is added until the steps below are completed in order. Chord Machine remains removable behind `MELLO_FEATURE_CHORD_MACHINE` until the go/no-go step is complete.

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

Implementation strategy for FM-1:

- Do not attempt a literal Tone.js convolution engine if it threatens FM-1 CPU/RAM.
- Replace the current resonant/tap-like structure with a diffuse algorithmic design whose audible target is the HTML reverb.
- Prefer multiple decorrelated delay/comb paths plus all-pass diffusion and damping over a small number of repeated taps.
- Keep pre-delay separate from the diffusion network.
- Keep reverb tone filtering and 24 Hz HPF on the wet return, not inside the dry path.
- Preserve the current shared FX bus so lead, chord and samples can inhabit the same room.

Hard gates:

- Impulse-response regression: no strong short-period repeating peaks that read as comb/ring modulation.
- Decay envelope must remain smooth over short, medium and long settings.
- Wet=0 must be bit/near-bit equivalent to dry path.
- 80 MHz worst-case must remain comfortably above realtime; target >=1.35x steady realtime after reverb is enabled in a heavy patch.
- Pool/RAM must remain within current hard limits.

Decision: Reverb V2 has passed these gates. Chord Machine may now proceed to its final go/no-go step.

## 2. Chord Machine final go/no-go

Only after the reverb is fixed.

Evaluate:

- timing lock against the main sequence;
- STAB/HOLD/ARP/PATTERN musical usefulness;
- whether the open 3-voice default occupies a useful spectral slot;
- whether it complements rather than masks the main synth and sub;
- practical UI comprehension.

If it still does not feel integrated, compile it out with `MELLO_FEATURE_CHORD_MACHINE=0` and remove the feature from the product surface. Do not spend another large iteration defending it.

## 3. UI performance + universal sequence overview

Goal: sequence position should never disappear simply because the sequence is longer than 16 steps.

- Use one shared 4..32-step overview geometry wherever a page is actually showing the main sequence.
- HOME and RANDOM already follow this rule; extend it consistently where appropriate.
- Keep all 32 visible by thinning columns rather than paging the visual overview.
- Hardware keys may still address 16-step edit banks when required; visual overview and edit addressing are separate concepts.
- Profile SEQ/HOME redraws at 80 MHz.
- Remove unnecessary full/static redraws and preserve dirty-region rendering.

Hard gate: no sustained UI/render path should push a normal 80 MHz playback case close to the realtime ceiling.

## 4. Randomizer intensity

Add a musical depth control independent of the selected random targets:

- LOW: conservative variation around the current pattern.
- MED: current/default creative range.
- HIGH: larger structural and timbral movement.

Intensity should constrain target-specific ranges rather than merely reduce the probability that a target changes. `WAVE` remains an independent selectable target.

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
- rerun chord feature-on and compile-out tests if chord survives;
- target build, emulator audio, 100 MHz browser, 80 MHz worst-case and Chromium E2E;
- inspect RAM, pool and image headroom before declaring the iteration complete.

## Deferred, not part of this stabilization iteration

- Four factory samples baked into firmware: implement when the source audio files are supplied.
- Further oscillator anti-aliasing/Tone.js envelope refinement: only after the reverb, chord decision, randomizer and UI-performance work above are stable.
- New large sequencing features: no new scope until this roadmap is complete.
