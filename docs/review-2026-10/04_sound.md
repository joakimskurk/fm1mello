# 04 — Sound parity review: SkurkyMello HTML (Tone.js 14) vs FM-1 fixed-point port

Reviewer: sound-parity. Scope: `original.html` (Tone@14 from unpkg, line 23) vs `mello/mello_audio.c`, `mello_engine.c`, `mello_transport.c`, `overlay/dinghy.c`.
No source files were modified. Measurements came from a host harness built from copies of the sources in `scratchpad/sndtest/` (`harness.c`, `rv.c`, `rv2.c`, `filt.py`).

**Bottom line.** The port reproduces the HTML's *control plumbing* (defaults, velocities, gate fractions, ratchet math, sub-voice envelope) well. Most of the audio *transfer functions* are different, though, and those differences explain the "doesn't sound or feel like the HTML" complaint. In order of impact:

1. The **main filter** is a different filter. The HTML uses two cascaded Web Audio biquads with Q in **dB**: +7 dB peak at the default resonance and +30 dB at max. The C port uses a 4×one-pole ladder with weak feedback, which has **no resonant peak at any setting**.
2. The **lead, sample and chord envelopes are linear** in C. Tone's decay and release are fast exponential approaches.
3. **Gain staging differs.** The HTML's main `Tone.Distortion` is *always in the path* (wet=1). At AMT=0 it is a **×1/3 linear curve with a hard clip at |x|=1**. The C port bypasses it, so it runs about 9.5 dB hotter into a hard-clamp "limiter".
4. **The HTML reverb is a convolution `Tone.Reverb`**, not Freeverb. It uses a stereo white-noise IR with an exponential envelope: RT60 ≈ 2.5 s at DECAY=6 s, flat spectrum, SIZE ignored. The C FDN is longer, darker and LF-heavy, and SIZE changes it.
5. C oscillators are naive and alias. Tone's are band-limited PeriodicWaves.
6. **Timing:** every pulse-start event is late by 0–5.8 ms (block-quantised) because the transport has no lookahead.

---

## 1. HTML audio graph, reconstructed

### 1.1 Nodes (`buildSynth`, lines 1244–1371)

| Node | Code (line) | Settings at default |
|---|---|---|
| Lead `synth` | `new Tone.Synth({oscillator, envelope})` (1260) | osc type `oscType` (default `"sawtooth"`, 495). Pulse: `{type:"pulse", width: pwm∈[.05,.95]}` (1256). Envelope A .005 / D .18 / S .4 / R .12 (491–494). Tone defaults: attackCurve **linear**, decayCurve **exponential**, releaseCurve **exponential**. `portamento = slideIn ? slideTime(.05) : 0` per step (1594). |
| Follower sub `subOsc` | `Tone.Synth` (1264) | wave `subOscWave` (sine). **Same ADSR as lead**, its own envelope. No portamento is ever set, so it is 0. Note = lead + `subOscOct*12` (−1 oct, 1586). |
| `subOscGain` | `Tone.Gain(on ? subOscLevel : 0)` (1272) | 0.22. Note: trigger velocity is *also* `vel*subOscLevel` (1592), so the effective level is **level² × vel**. |
| Dedicated `sub` | `Tone.Synth` (1268) | wave `subWave` (sine). Envelope A .003 / D `subDecay` .16 / S **.24** / R `subRelease` .12. `portamento = subGlide` .02 (1273, 1677). Velocity 0.9 fixed (1678). Duration `stepDur·pulses·subGate(.9)`, clamped .03–2.5 s (1675). |
| `subFilter` | `Tone.Filter` LP, rolloff −24, **Q 0.8** (1274) | f = `40·55^subCut` → 243 Hz at .45 (1145) |
| `subDriveNode` | `Tone.Distortion(subDrive, wet = drive>0 ? 1 : 0, oversample "2x")` (1278) | bypassed at 0 |
| `subGain` | `Tone.Gain(subLevel)` (1279) | 0.42 |
| Sample slots ×4 | `AmplitudeEnvelope` → `Distortion(dist, wet = dist>0, 2x)` → `Gain(level .7)` → **mix** (1322–1340) | A .005 / D .12 / S .7 / R .15 (Tone curves: lin / exp / exp). `ToneBufferSource` per trigger, `playbackRate = 2^(semis/12)` with a linear ramp for slide (1554–1560). Buffers normalised to 0.95 peak (215). |
| `mix` | `Tone.Gain(0.58)` (1280) | Sums lead + subOsc + sub + samples. The AMP LFO target is this gain (1219). |
| Main `filter` | `Tone.Filter` LP, rolloff **−24**, **Q = filterRes·15** (1282–1285) | f = `80·200^cut` (1124) → 3264 Hz at .7. Q = 3 at res .2. Knob changes `rampTo(.05)` (989, 993). |
| Main `dist` | `Tone.Distortion({distortion: distDriveFromAmount(amt,char), wet: distMix(=1), oversample:"4x"})` (1286) | `distDrive = clamp(amt^1.9·(0.28+0.52·char), 0, .8)` (1137). **At AMT=0 the node is still fully wet.** |
| `distTone` | LP −24, Q 0.2 (1287) | `320·38^tone` → 6.3 kHz at .82 |
| `distLevelNode` | `Gain(clamp(distLevel,.2,1.4))` (1288) | 0.9 |
| `dry` | `Gain(0.84)` (1289) | |
| Delay | `Tone.TapeDelay \|\| Tone.FeedbackDelay` (1290). **TapeDelay does not exist in Tone 14, so this is FeedbackDelay.** | delayTime `"8n"` (converted once at the current BPM; not re-synced on BPM change), feedback .32, wet 1 |
| `dlySend` / `dlyTone` / `dlyReturn` | Gain .12 / LP −24 Q .2 `450·26^tone` (4.4 kHz) / Gain .75 | The tone filter sits **after** the delay, outside the feedback loop |
| Reverb | `useConvReverb = !!Tone.Reverb` (1304). Tone 14 has `Reverb`, so this is **always `new Tone.Reverb({decay, preDelay, wet:1})`**. Freeverb is dead code. | decay 6.0 s, preDelay .012. `reverbSize` and `reverbType` have **no effect** (they only feed the Freeverb branch) |
| `revSend` | `Gain(0.03 + wet^1.2·0.95)` (1128) | 0.201 at wet .24 |
| `revTone` / `revHPF` / `revReturn` | LP −24 Q .2 `700·18^tone` (4.2 kHz) / HP −24 **24 Hz** Q .35 / Gain `level·1.05` | return 1.2075 at level 1.15 |
| `fxMix` → `limiter` | `Gain(1)` → `Tone.Limiter(-1)` (1315–1317) | |
| Destination | `volume = gainToDb(masterVol)` (1368) | 0.75 → −2.5 dB, *after* the limiter |
| LFO | `Tone.LFO(sine, rate .008–20 Hz log)` (1316) | Targets: **pitch** → `detune` ±(80+620·d) cents (1204). **amp** → `mix.gain` 0.58 ± 0.58·d (1212). **pwm** → width ±min(.42d, margins) (1222). **filter** → *not connected*: a rAF loop (~60 Hz, stepped) sets `filter.frequency = base ± base·(0.08+0.92d)·sin`, linear in Hz, floor 20 Hz, upward span ×(1+1.5·matrixEnv) (1374–1394) |

### 1.2 Signal flow

```
lead Synth ──────────────┐
subOsc Synth → subOscGain┤
sub Synth → subFilter → subDrive → subGain ┤──► mix(0.58) → filter(LP-24, Q dB) → dist(WS 4x, wet 1) → distTone(LP-24)
samples ×4 → env → sDist → gain ──────────┘                                                             │
                                                                                               distLevel(0.9)
                     ┌───────────────────────────────────────┬──────────────────────────────────────────┤
                 dry(0.84)               dlySend(.12)→FeedbackDelay→dlyTone→dlyReturn(.75)   revSend→Reverb(conv,stereo)→revTone→revHPF→revReturn
                     └──────────────────────────► fxMix(1) ◄───────────────────────────────────────────┘
                                                      → Limiter(-1) → Destination(vol −2.5 dB)
```

### 1.3 Tone.js internals that matter (Tone 14 source semantics)

* **Envelope.** The attack is a linear ramp to `velocity`. On retrigger the attack time is shortened by the level already reached. Decay uses `exponentialApproachValueAtTime(vel·S, t, D)`: `setTargetAtTime` with **τ = ln(D+1)/ln 200**, then a linear ramp to the target over the final 10 % of D. Release uses `targetRampTo(0, R)` with the same τ law. At defaults: decay τ = **31 ms**, release τ = **21 ms**, sub decay τ = 28 ms.
* **Portamento** (`Monophonic.setNote`). Uses `frequency.exponentialRampTo(hz, portamento)`, i.e. **linear in pitch**, and *only if the current envelope level is > 0.05*. Otherwise the pitch jumps.
* **Filter.** Rolloff −24 means two `BiquadFilterNode`s in series sharing frequency and Q. For lowpass/highpass, **Web Audio interprets Q in dB**: α = sin ω0 / (2·10^(Q/20)). Main filter Q at res .2 is 3 dB (Q_lin 1.41 per stage); at res 1.0 it is 15 dB (Q_lin 5.6 per stage). The tone filters' Q .2 dB gives Q_lin 1.02, i.e. mildly peaky 4th-order.
* **Distortion.** Curve `y = (3+k)·x·(20π/180) / (π + k|x|)`, k = 100·amount, 4096 points. A WaveShaper clamps input to ±1. **At k=0 this is y = x/3** (−9.54 dB, linear) with a hard clip at |x| = 1 (output ±0.333). Small-signal gain is (3+k)/9: 5.5× at the maximum HTML drive of 0.462 (char .35). The asymptote stays near 0.35, so drive makes the sound *louder and compressed*.
* **Effect wet** uses a `CrossFade`, which is **equal-power**.
* **Tone.Reverb.generate().** Renders offline: two independent white-noise sources (L/R), gain 0 until preDelay, then 1 → `exponentialApproachValueAtTime(0, pre, decay)`. RT60 = 6.9·ln(decay+1)/ln 200 → **2.54 s at DECAY 6 s**, 1.43 s at 2 s, 0.90 s at 1 s. The spectrum is **flat** (white) and the onset is dense immediately after preDelay. Chrome's `ConvolverNode` (normalize=true) scales the IR to an RMS gain of 0.00125·√(N): **≈0.64 (−3.8 dB) at 6 s**. Output is truly stereo, decorrelated L/R.
* **Tone.Limiter(-1)** = `Compressor({ratio 20, attack .003, release .01, threshold −1})`. It inherits the Compressor default knee of 30 dB (*verify against the exact Tone build*). This is a Chrome DynamicsCompressor with about 6 ms lookahead and a soft knee starting at −1 dBFS. Because the HTML's gain staging leaves ~9 dB of headroom, it is mostly idle.
* **Oscillators.** saw/square/triangle are `PeriodicWave`s, band-limited per octave by the browser. `"pulse"` is a `PulseOscillator`: a band-limited saw fed through a step WaveShaper, so **it aliases as well**. Oscillators restart (phase 0) only when they have fully stopped (after release); overlapping notes keep phase.

### 1.4 HTML gain staging (default patch, single C4 saw note, vel 0.72)
osc 1.0 × vel 0.717 × mix 0.58 = 0.416 → filter (≈ +2–7 dB near fc, harmonics) → **WS ×1/3** → 0.139 → ×0.9 × 0.84 = 0.105 → limiter (idle) → ×0.75 ≈ **0.08 peak (≈ −21 dBFS)**.
Port, measured: **peak 0.197 (−14.1 dBFS)**. The port is about **+7…+9.5 dB hotter** (harness T1).

---

## 2. Element-by-element parity, ranked by audible impact

| # | Element | HTML | C port | Verdict / audible effect |
|---|---|---|---|---|
| **1** | **Main filter type and resonance** | 2× biquad LP, Q in dB (`res·15`). Response vs passband at fc (measured analytically, `filt.py`): res 0: +2.5 dB peak at .71 fc, −22 dB at 2fc, −48 dB at 4fc. **res .2: +7.2 dB peak at .87 fc**. res .5: +15.4 dB. **res 1: +30 dB** (then clipped by the WS at |x|>1) | `mello_audio.c:1020`: 4 cascaded naive one-poles, feedback `k = res·0.748` from `filter4[n-1]`. Response: **no peak at any res**. Passband loss −1.2 dB (res .2) to −4.8 dB (res 1). Soft knee (−3 dB at fc, −9 dB at 2fc, **−12…−24 dB at 4fc**). Harness T1b: res 0→1 changes the 654 Hz harmonic/fundamental ratio by only +3.5 dB. | **Critical mismatch.** The HTML has the "honk/squelch" of a peaky −24 filter even at default; at high res it screams into the clipper. The port is a dull, resonance-less tilt, and brighter far above cutoff. The cutoff *mapping* (−3 dB point at `80·200^v`) was matched, but nothing else. |
| **2** | **Lead envelope curves** | Decay/release exponential approach. At defaults, lead level at t = 20/50/100/150 ms = **0.77 / 0.54 / 0.43 / 0.41**. After note-off: 0.39 at 20 ms, 0.10 at 50 ms, 0.009 at 100 ms. | `lead_begin_decay`/`lead_release` (469–471): **linear** segments: 0.95 / 0.85 / 0.68 / 0.52 at the same times; after note-off 0.83 / 0.58 / 0.17. Harness trace confirms a straight 270 ms fade. | **Critical.** The HTML is a snappy pluck: drop to sustain in ~70 ms, release gone in ~60 ms. The port sounds slow, swollen and "fadey", and notes blur together at 16ths. Sample slots (`sample_begin_decay`, 543) and chord (590) are also linear. **The sub voice already uses the correct Tone curve** (`sub_curve_*`, `TONE_ENV_COEFF_Q31`), so the primitive exists. Lead attack is not shortened by the reached level (Tone shortens it); negligible at 5 ms. |
| **3** | **Gain staging / always-on waveshaper** | WS always wet: ×1/3 plus a hard clip at the input ±1 (post-filter), 4× oversampled | `1021`: `if(dist_drive)…` so AMT=0 is identity. No ×1/3, no clip. | **High.** The port is ~9.5 dB louder into the end of the chain, so stacked lead + sub (0.42) + reverb return hits the ±29204 hard clamp. In the HTML, resonant peaks are hard-clipped *before* the FX (a 4×-oversampled "acid bite"), and the chain keeps about 9 dB headroom. Raising AMT in the HTML also makes it *louder* (small-signal gain 1/3 → 5.5×); the port's E2 shaper stays level-bounded. |
| **4** | **Reverb** | Convolution, stereo noise IR. RT60 = 2.54 s (6 s), flat across bands. Energy remaining after 0.5 s ≈ −11.6 dB (all bands). Wet RMS gain 0.64. SIZE/TYPE ignored. | 8-line Householder FDN (V3). Measured (`rv.c`, defaults): **RT60 3.6 s @125 Hz, 3.3 s @500, 3.3 s @1 k, 3.0 s @2 k, 2.7 s @4 k, 1.95 s @8 k**. Energy left after 0.5 s: −4 dB (LF) … −22 dB (8 kHz). DECAY 2 s → 3.9 s @125 / 2.1 s @1 k (Tone: 1.43 s flat). DECAY 1 s → 2.0 / 1.3 s (Tone: 0.90). Wet energy gain −8.2 dB vs Tone −3.8 dB. L/R corr 0.08 (good). The return uses ×1150/1000 where the HTML uses ×1.05, so +0.8 dB (line 1038–1039). Fixed-point DC limit cycle at −4 LSB (harmless, removed by the HPF). | **High.** The HTML tail is a bright, smooth "white" wash that dies in ~2.5 s. The port's tail is longer (×1.3–2.7), LF-boomy and HF-dark, so it sounds muddier and more "room/plate", not "noise wash". The design docs (`WP_E_AUDIO_AUDIT.md`) never mention that the reference is a noise-IR convolution, so V3 is being tuned toward a different target. SIZE in C changes delay lengths; in the HTML it does nothing. |
| **5** | **Oscillator band-limiting** | Band-limited PeriodicWave (saw/square/tri). Pulse aliases (waveshaped saw). | `osc_sample` (442–451): naive saw/square/tri/pulse. Sine = 256-pt LUT, no interpolation (≈ −44 dBc phase-truncation spurs). | **High at upper notes.** Default notes span C4…C6 (root 60, 2-octave span). A naive saw at 1 kHz has ≈ −13 dB of total folded alias power. The 1-pole-soft C filter (row 1) lets more of it through (−19 dB at 4fc vs −60 dB). Result: "digital fizz/grit", inharmonic beating on slides. Square plateau is ±1.0 vs a normalised band-limited square (~+0.5–1 dB). |
| **6** | **Distortion curve** | Rational `(3+k)x·0.349/(π+k|x|)`, k = 100·amt^1.9·(0.28+0.52c) ≤ 80, input hard-clipped at ±1, 4× oversampled, equal-power wet | E2 (`global_distort`, 947): cubic AMT → drive 0..1000, pre-gain 1–7.84×, CHAR-dependent hard knee (0.63–0.875 FS) with linear post-knee slope. No oversampling. Linear dry/wet. | **Medium-high when AMT>0.** A different family: piecewise-linear knee (harsher, aliasing) vs a smooth rational curve. AMT range and the loudness-vs-AMT behaviour differ, and the mix law is linear vs equal-power (−3 dB at 50 %). This was a deliberate E2 divergence, but it is not HTML parity. |
| **7** | **FX tone filters** (dist/delay/reverb) | LP −24 dB/oct (2 biquads, Q .2 dB) | one-pole 6 dB/oct (`dist_lp`, `delay_lp`, `reverb_tone_lp`), same −3 dB frequency | **Medium.** Above cutoff the HTML is −22 dB at 2fc and −48 dB at 4fc; the port is about −7/−12 dB. Delay repeats, reverb tail and distortion fizz are much brighter in C. Reverb HPF: 1st-order 24 Hz vs 4th-order (more sub-rumble passes into the tail in C). |
| **8** | **Event timing** (see §3) | sample-accurate, 100 ms lookahead | pulse events always land on the next 256-frame block: **0–5.8 ms late**, plus main-loop stalls | **Medium (feel).** |
| **9** | **LFO depths/targets** | pitch ±(80+620d) cents → **±297 c at d=.35**, ±700 c at d=1. AMP modulates the *whole mix* (lead+subs+samples). Filter: linear-Hz ±(0.08+0.92d)·base (at d=1 sweeps down to 20 Hz). | pitch `mod=lfo·depth/80000` → **±60 c at d=.35**, ≈ +165/−182 c at d=1 (4–5× too shallow, no 80 c floor). AMP: lead (+follower) only. Filter: ±0.22·d of knob range, exponential → ×/÷3.2 at d=1. PWM ≈ parity. | **Medium** when the LFO is used. Vibrato/siren depth and tremolo scope are clearly different. |
| **10** | **Follower sub (`subOsc`)** | level² × vel: 0.22² = 0.048 (**−26 dB** re lead). Separate envelope. **No glide.** | `mello_audio.c:981`: summed into the lead *before* the lead env, level 0.22 (**−13 dB**). Glides with the lead (`sub_slide_*`). | **Medium-low.** It is 13 dB louder in C, which adds low-end weight and mud. It slides when the HTML jumps. |
| **11** | **Portamento** | exponential (linear-pitch) ramp; skipped when env < 0.05 | `lead_attack` (470): linear in phase increment (linear-Hz); always glides when the slide flag is set, even from silence | **Medium-low.** Upward slides in C lag then rush; a slide step after a rest swoops in C but jumps in the HTML. (The sub voice *does* implement the env>0.05 rule.) |
| **12** | **Sub filter Q** | Q 0.8 **dB** → Q_lin 1.096 per stage: +1.6 dB at fc | `SUB_FILTER_BIQUAD_Q28` computed with Q 0.8 *linear* (coefficients give Q≈0.81): −3.2 dB at fc | **Low-medium.** About 4.8 dB less body around the sub cutoff and a softer knee. |
| **13** | **Limiter** | soft compressor (ratio 20, 3/10 ms, knee 30 dB), rarely engaged | hard clamp at 29204 (−1 dBFS), then master | **Low-medium.** Harmless at HTML gain staging, but it audibly clips in the port because of row 3. |
| **14** | **Parameter smoothing** | knob changes `rampTo` .05–.1 s (filter, Q, levels, tone) | config swapped at block boundaries (5.8 ms steps), no smoothing (acknowledged in E1 audit) | **Low-medium** (zipper on cutoff/res/level sweeps). |
| **15** | **Samples** | original-rate float buffer, normalised to 0.95, Chrome linear interpolation, exp env, region-end release | 22.05 kHz **4-bit IMA-ADPCM** (`mello_sample.c:59`), 2× linear-interp upsampling (images), linear env, hard stop at buffer end (`584`), `soft_drive` instead of the Tone curve | **Low-medium** (a lo-fi, grainier and duller sample layer; storage constraint). |
| 16 | Misc | equal-power dist mix; delay `"8n"` frozen at build BPM; oscillator phase reset after full stop | linear mix; delay re-synced per block (better); free-running phase | low |
| — | Chord machine | **does not exist in the HTML** | `MELLO_FEATURE_CHORD_MACHINE` | no reference, so out of parity scope |

**Matches (verified):** all defaults (`mello_sound/fx/sub/sample_defaults` vs useState 488–538), velocity 91/121 ↔ 0.72/0.95 (vel·258 ≈ 0.7165), sub velocity 114 ↔ 0.9, mix 0.58, dry 0.84, dist level, send curve table (0.201 at .24), delay send/feedback/return topology (tone outside the loop), master after limiter, matrix filter hit (≈ ×3.6, 20 ms up / 140 ms down, linear in log-Hz), TM filter/decay mod, sub envelope (Tone τ law, 90 %/linear tail, attack shortening, glide-only-if-audible), sub drive curve (`tone_distortion_point` = exact Tone formula).

---

## 3. Sequencer timing and feel

| Aspect | HTML | C | Parity |
|---|---|---|---|
| Step clock | `scheduleRepeat(cb,"16n")` (2026), Tone lookahead 0.1 s; `time` exact | fractional µs deadlines (`advance_fractional_deadline`), PPQN 16800 | exact grid both |
| Swing | none (Transport.swing unused) | none | ✓ |
| Single gate | `totalDur·0.82` (20 ms–4 s) | `·41/50`, clamp 20–4000 ms | ✓ |
| Multi, r=1 | `pulseDur·0.4` | `·2/5` | ✓ |
| Ratchet | `sub=pulseDur/r`, len `clamp(sub·0.72, 10, 250 ms)` | `·18/25`, same clamp | ✓ |
| Hold | on at p0, off at last pulse +0.88 | `·22/25` | ✓ (both leave a hanging note if the last pulse fails probability) |
| Probability | `Math.random() <= prob` per pulse | `probability_pass` per pulse (PRNG) | ✓ |
| Scale quantisation | degree → scale table, `scaleQuantize` (identity for in-scale) | engine resolver (not re-audited here) | assumed ✓ |
| **Event placement** | sample-accurate Web Audio automation | see below | ✗ |

**Block-quantisation (C).** `mello_transport_poll(now_us)` (dinghy.c:246) processes a pulse only once `now_us ≥ due`. The audio callback renders 256 frames starting at `fm1_micros()` (dinghy.c:75) and pops events per sample by `due`. The block that contains `due` was always rendered *before* the main loop saw the pulse, so **every pulse-start event (note-on, sub-on, sample-on, PULSE/TM/matrix) fires at sample 0 of the next block: 0–5.8 ms late (uniform), plus any main-loop stall** (LCD/flash work in the same loop).

Events dated *later* within the pulse (ratchets 2..n, note-offs, sub-offs) are pushed early and land sample-exactly. Consequences:

* There is ~±2.9 ms random looseness on every step. At 80 BPM 16ths (187 ms) that is ±1.5 %. It is audible as a slightly "drunk" grid, and it is clearly audible against an external clock or drum machine.
* **Ratchets are uneven.** r=0 is late but r≥1 are on time, so the first ratchet gap shrinks by up to 5.8 ms. At 8 ratchets / 120 BPM (15.6 ms spacing) that is up to a 37 % shorter first interval (a "flam"). The first note's gate is shortened by the same amount.
* Lead and sub stay aligned with each other (same block), so there is no inter-voice flam.

There is no other groove difference. The HTML has no swing or humanise, and its rAF filter-LFO is not timing-critical.

---

## 4. Recommended DSP changes (fixed point, FM-1)

**Budget.** 80 MHz / 44.1 kHz = 1814 cycles/sample. At the stated ~1.8× margin the port uses ≈ 1000 cycles/sample. Keeping the project's ≥ 1.35× contract leaves **≈ 330 cycles/sample** to spend. The estimates below assume Cortex-M-class single-cycle MAC/SMLAL; they are guesses to validate with the existing 80 MHz emulator gate.

| Pri | Change | How | Est. cost (cyc/sample) |
|---|---|---|---|
| **P1** | **Replace the main filter with Tone's 2× biquad, Q in dB** | Reuse `sub_biquad_step` (Q28 DF-I, 64-bit acc). Coefficients from `fc = 80·200^cut_eff` (after LFO/TM/matrix), `Q_lin = 10^(res·15/20)`, α = sin/(2Q). Recompute every 16–32 samples from a 1024-pt sin/cos LUT plus a reciprocal (≈ 120 cyc/update), with linear interpolation of the modulated cutoff between updates. Then **emulate the WS input clip:** clamp the filter output at ±1.0 FS-equivalent (32767) before the shaper. With Q up to 5.6 per stage the coefficient precision at low fc is fine in Q28. | +15…25 (vs current ladder ≈ 20) |
| **P1** | **Tone envelope curves for lead, samples and chord** | Use the existing `sub_curve_begin/tick` + `TONE_ENV_COEFF_Q31` for lead decay/release (target `vel·S`, 90 % curve then linear tail). Lead attack: scale the duration by `(1 − level)` as `sub_attack` already does. Index needs ms ≤ 1200 (table size); for longer D/R extend the table or compute the coefficient per note. | +5 per voice |
| **P1** | **Restore HTML gain staging** | Always run the main shaper: build a 257-pt curve LUT with `tone_distortion_point(drive)` where drive = `1000·clamp(amt^1.9·(0.28+0.52c), 0, .8)` (AMT=0 gives the exact x/3 line). Clamp the input to ±32767 (Tone clamps at ±1). Use 2× oversampling (the existing linear-mid trick) or a 7-tap halfband for nearer 4×. Equal-power dist mix (cos/sin LUT). Keep E2 as an optional "EXTREME" CHAR mode if the owner likes it. After this the output drops ~9.5 dB to HTML level, so the owner can raise master or the outer gain. | +15 (2× interp) … +45 (halfband) |
| **P1** | **Transport lookahead** | Poll with `mello_transport_poll(&mt, now_us + 12000)` (≥ 1 block + worst main-loop stall; 15–20 ms if LCD flushes are long). Drive UI step display from the un-advanced time. Check queue headroom: the 4×32 lanes hold ≈ 1–2 pulses of worst case (8 ratchets + sub + sample + chord ≈ 30 events/pulse). Consider not queueing PULSE/STEP_ENTER/SEGMENT markers to audio, or carrying them on a separate lane. | 0 |
| **P2** | **Reverb V3 retarget to the Tone noise-IR** | Keep the FDN and ring, but: (a) per-line gain `g_i = 10^(−3·L_i/(fs·RT60))` with **RT60 = 6.908·ln(decay_s+1)/ln 200** (not 1.6·decay); (b) remove the in-loop frequency-dependent damping and LF shelf, or make them very light: the reference is spectrally flat and colour comes only from the −24 dB 4.2 kHz LP + 24 Hz HP outside; (c) maximise early density (inject into all 8 lines with sign/permutation; one more short allpass if needed) so the onset is "full" right after PRE; (d) gain-match to Chrome's normalisation, target wet RMS ≈ `0.00125·√(fs·(decay+pre))` per channel (compute the FDN's analytic energy gain per config in `reverb_configure`); (e) SIZE: either ignore it (parity) or limit it to small line-length scaling; (f) fix the return constant 1150 → 1050; (g) add rounding/dither to kill the −4 LSB DC limit cycle. CPU about the same or lower (damping removed). | −10 … 0 |
| **P2** | **PolyBLEP oscillators** for saw/square/tri (tri via leaky-integrated BLEP square, or a 2nd-order BLAMP) | Per-sample: two compares and one polynomial on edges; `dt = inc` and reciprocal precomputed per note or slide block. Also do pulse (better than the HTML, but harmless). Linear-interpolate the sine LUT (sub, follower, LFO). | lead +20, follower +6, chord +20/voice |
| **P2** | **24 dB/oct FX tone filters** | Replace `dist_lp`, `delay_lp`, `reverb_tone_lp` (×2 stereo) with 2× biquad Q .2 dB from 101-entry const Q28 tables (same style as `SUB_FILTER_BIQUAD_Q28`, flash only). Reverb HPF: 2× biquad 24 Hz Q .35 dB. If CPU is tight, use one 12 dB biquad each (half cost, most of the audible gain). | +80 (full) / +40 (12 dB) |
| **P2** | **LFO parity** | Pitch: ±(80+620d) cents through a 2^(x/1200) LUT (or a 2nd-order polynomial). AMP: apply `ampmod` to the summed `mix` (lead+sub+samples). Filter: linear-Hz span `base·(1 ± (0.08+0.92d)·s)`, floor 20 Hz, upward ×(1+1.5·matrix); plugs into the P1 coefficient update. | +10 |
| **P3** | Follower sub = HTML | level² gain, own copy of the lead ADSR (Tone curves), no glide | +5 |
| **P3** | Portamento = Tone | Exponential: per-sample multiply inc by `r = (target/start)^(1/n)` (from a 2^x LUT at note-on); only glide if `env > 0.05·FS`. Samples already ramp playbackRate linearly (HTML does linear too). | +3 |
| **P3** | Sub filter Q | Regenerate `SUB_FILTER_BIQUAD_Q28` with Q_lin = 10^(0.8/20) = 1.096 | 0 |
| **P3** | Limiter | Once P1 gain staging is in place, swap the hard clamp for a cheap soft-knee peak compressor (stereo-linked peak follower 3 ms/10 ms, log-domain LUT gain computer, threshold −1 dB, knee ≈ wide), or at minimum a tanh-style soft clip LUT above −1 dB | +10 (soft clip) … +35 |
| **P3** | Smoothing | One-pole (≈ 5 ms) on filter cut/res, dist/delay/reverb levels and tone indices, as per-block linear ramps (cheap) | +5…10 |
| P4 | Samples | Exponential env (as for lead), release at region end instead of a hard stop, Tone dist curve for slot drive. Optional: better than 4-bit ADPCM if flash allows (8-bit µ-law or 16-bit at shorter max length). | +5 |

**Suggested first pass (best sound per cycle):** P1 filter + P1 envelopes + P1 gain staging/shaper + P1 lookahead + P2 reverb retune. That is ≈ +60…100 cycles/sample, well inside budget. It should already close most of the "sound and feel" gap: squelchy resonance, plucky envelopes, bright clipped bite, short white reverb wash, and a tight grid. Then BLEP and 24 dB FX filters, which take most of the remaining budget (≈ +100–180).

**Verification proposal.** Render the HTML graph headless with `Tone.Offline` (same patch JSON, fixed seed, `Math.random` stubbed). Render the C engine through the host harness with the same pattern. Compare per-note envelope traces, 1/3-octave spectra, filter sweeps at res 0/.5/1, and reverb per-band EDCs, and add them as regression gates next to the existing WP-E audits. The current E-series gates (density, correlation, symmetry) all pass while measuring a different target from the reference.

---

## Appendix — measurement results (host build of the port, defaults unless noted)

```
T1 lead C4 saw vel91, FX dry: peak 0.197 (−14.1 dBFS); envelope |peak|/10 ms: .197 .193 .186 … linear to 0 at ~270 ms
T1b saw C2, cut .4: res 0 → h10/fund −22.9 dB; res .5 → −20.6; res 1 → −19.4 (fund drops 4.8 dB)
Filter (analytic, filt.py) res .2 @cut .7: Tone +7.2 dB peak @.87fc, −23 dB @2fc, −60 @4fc | C −1.2 dB flat, −8 @2fc, −19 @4fc
Reverb C, SIZE .7 / DECAY 6 s: RT60 125 Hz 3.58 s, 500 3.33, 1k 3.26, 2k 3.04, 4k 2.70, 8k 1.95; Tone: 2.54 s all bands
Reverb C, SIZE .3 / DECAY 2 s: 3.89 / 2.71 / 2.07 / 1.61 / 1.20 / 0.72 s; Tone 1.43 s
Reverb C, SIZE 1 / DECAY 1 s: 2.01 / 1.46 / 1.29 / 1.14 / 0.96 / 0.66 s; Tone 0.90 s
Reverb C wet energy gain −8.2 dB (Tone ≈ −3.8 dB at 6 s); L/R corr 0.08; DC limit cycle −4 LSB
Tone env τ: D .18 → 31.2 ms; R .12 → 21.4 ms; sub D .16 → 28 ms
```
