# SkurkyMello FM-1 - stabilization and product-evolution roadmap

Review baseline: product-code checkpoint `1c0748c2140e106e0c38fa0637b4c06139c2625f`, hands-on product review 2026-10-25.

This roadmap remains stabilization-first. The new audition notes broaden the future plan, but they do not authorize jumping ahead of the active work package.

See `docs/PRODUCT_WORKSHOP_2026-10-25.md` for the full review, simulated musician/developer workshop and product-owner checkboxes.

## 1. Reverb V2 - TECHNICALLY COMPLETE, PRODUCT QUALITY REOPENED

Reverb V2 was completed in `58ea689b282d00dd436b790be3bcf95b91e913f3` with separate pre-delay, two all-pass diffusers and a four-line Hadamard FDN. The technical baseline remains useful:

- corr² `0.00075`
- block-energy jump `2.279`
- 120 ms pre-delay shift `5292` samples
- 80 MHz chord/reverb stress `1.833x` steady realtime
- pool headroom `17696 B`

However, the 2026-10-25 audition still reports metallic/ring-mod-like character and insufficient hi-fi diffusion. Therefore Reverb V2 is no longer considered product-complete. Preserve its regression data and reopen perceptual quality as Reverb V3 in the audio-character work package below.

The original HTML remains the reference intent: `Tone.Reverb` when available, Freeverb only as fallback, followed by wet tone LPF, 24 Hz HPF, return gain, FX mix and limiter.

## 2. Chord Machine - TECHNICALLY PROTECTED, PRODUCT PURPOSE OPEN

Technical regression baseline remains valid: held `ARP_UP` traverses three chord tones and compile-out with `MELLO_FEATURE_CHORD_MACHINE=0` remains required.

The latest audition confirms that the current concept can occasionally produce useful results, particularly with sine, but is often too raw/bright and lacks enough tone shaping. Do not remove it and do not incrementally polish it yet.

Future design spike must compare:

1. soft filtered PAD;
2. scale-aware HARMONY derived from lead/sub context;
3. SUB HARMONY;
4. CHORD V2 with filter/envelope/voicing;
5. default-hidden/compile-out if none earns its panel space.

## 3. UI performance + universal sequence overview - PERFORMANCE COMPLETE, VISUAL POLISH OPEN

Completed performance checkpoint: `00c6c72ff5a2e7fda1f6f95d0add7b00fa37f171`.

Existing redraw/performance measurements remain the regression baseline. The latest audition adds new visual-quality requirements:

- no overlap/clipping;
- no stale dirty-region artifacts;
- RANDOM skyline must be denser and more informative;
- HOME needs a later visual polish pass;
- all 4..32 active steps stay visible at once.

## 4. Randomizer intensity - CURRENT

Add a musical depth control independent of selected random targets:

- LOW: conservative variation around current material;
- MED: current/default creative range and compatibility anchor;
- HIGH: larger structural/timbral movement while remaining useful.

Intensity constrains target-specific mutation ranges rather than simply reducing the chance that a target changes. `WAVE` remains independent.

### Step 4 visual refinement

RANDOM remains a single full-sequence view. Improve information density without visual paging:

- 4..16 steps: comfortably readable columns;
- 17..32 steps: progressively compressed columns with every active step still visible;
- retain visible pitch contour/playhead;
- preserve pulse/ratchet/event cues at compressed widths;
- keep incremental redraw cost within the established Step 3 budget.

Do not add Undo, FM or broader control redesign inside Step 4.

## Gate A. Trust and reliability - IMMEDIATELY AFTER STEP 4

This gate exists because a user cannot learn an advanced instrument while wondering whether the UI or audio engine is broken.

### A1. Live-edit audio freeze

Reproduce and fix the reported case where audio freezes while turning parameters and recovers after STOP/PLAY.

Audit all live parameter paths for:

- audio-engine rebuilds/allocations;
- scheduler/event corruption;
- unsafe state mutation;
- missing smoothing/ramping;
- UI/audio contention.

### A2. Rotary mapping parity

Audit every page/layer. R1-R4 display labels, value formatting and physical handlers must share one authoritative mapping definition where practical.

### A3. Display integrity

- remove clipping/overlap at extreme values;
- remove static pink line over `SINGLE` on HOME;
- extend visual smoke tests to stale dirty regions and all page/layer combinations.

Major synth expansion waits until Gate A is green.

## Gate B. Control grammar v2 + timing model

### B1. REC / EDIT

Current page-specific REC meanings and `EDIT+REC` modifier behavior are documented debt. Final mapping depends on product-owner sign-off in the workshop checklist.

Recommended direction:

- REC = stable recording/capture verb;
- EDIT = edit focus/state/lock, not unrelated destructive modifier;
- explicit contextual commands move to visible actions.

### B2. Black-key action rail

Prototype the unused black keys as contextual actions with on-screen labels. Do not lock exact mappings until the complete page audit is available.

### B3. Modulation grouping

Prototype LFO + DOT under a coherent `MOD` concept while retaining distinct source behavior. Turing may belong to the same broader generative family without being reduced to an LFO.

### B4. Timing concepts

The original HTML separates:

- main-sequence `RESET AFTER`: off / 1 / 2 / 4 / 8 / 16 bars;
- Loop Cycle `SYNC / FREE`;
- Slot/Chain hold length;
- external transport/MIDI sync.

The port must make those distinctions explicit.

New product requirement: main sequence should default to an approved bar-locked length, with FREE available deliberately. Exact default is pending sign-off; workshop recommendation is 1 BAR.

## Gate C. Audio character

### C1. Distortion range

Current maximum is too polite. Extend range so maximum can be intentionally extreme while preserving subtle lower settings and managing output gain/headroom.

### C2. Reverb V3

Rework the native reverb specifically for perceptual diffusion quality. Objective tests remain required but listening is a hard gate. Reject normal musical settings that expose obvious periodic ringing, comb pitch, short-delay character or ring-mod-like metallicity.

### C3. Ratchet parity

Original HTML ratchets divide a pulse into sub-triggers and retrigger the synth envelope for each one. Verify native timing/envelope behavior before changing the musical design.

### C4. Live parameter smoothing

All continuous sound/FX controls must remain safe and audible while transport runs, without requiring STOP/PLAY recovery.

## 5. Undo Last Random

After the randomizer and trust/control gates are stable, store one pre-random snapshot sufficient to restore the immediately previous randomization.

Requirements:

- one level only;
- cheap in RAM/storage;
- restores every target touched by the last random operation;
- includes synth waveform/SOUND data once those targets exist;
- no interference with slot/project persistence.

## 6. Synth evolution

Only after Gate A/B/C:

### ANALOG

Preserve the current synth engine and its musical behavior as a first-class `ANALOG` mode.

### FM

Design a curated FM engine that exploits the hardware identity without becoming a generic operator editor. Start from a small set of musically meaningful macro parameters and prove CPU/RAM viability before expanding.

### SOUND randomization

Add sound/timbre as an independent randomizer target so rhythmic/melodic masks can remain untouched.

### Per-step timbre

Add a realtime-safe per-step sound-lock/modulation model. It must not allocate or rebuild audio objects per step.

## 7. Harmonic-layer design spike

Prototype and audition PAD and scale-aware HARMONY first, then compare SUB HARMONY and CHORD V2. Keep current Chord Machine behind regression protection until a winner is chosen.

No concept wins because it has more parameters. It wins if it creates useful harmonic depth quickly and remains legible in the FM-1 interface.

## 8. Instrument polish and onboarding

- splash screen;
- slicker HOME hierarchy without sacrificing sequence visibility;
- Key/Scale refinement;
- SEQ pages that visually explain their effect on the running pattern;
- consistent margins/truncation/value formatting;
- visible contextual control hints;
- newcomer test: usable without prior knowledge of the HTML prototype.

## 9. Final integration gate

After all approved work packages:

- full text-fit/clipping suite;
- full control-grammar mapping suite;
- randomizer deterministic/parity suite;
- ratchet/audio event-order suite;
- HTML audio/sub reference checks where applicable;
- chord feature-on and compile-out regressions;
- target build;
- emulator audio;
- 100 MHz browser path;
- 80 MHz worst-case path;
- Chromium E2E;
- RAM/pool/image headroom review;
- hands-on product audition for reverb, distortion, live editing and overall learnability.

## Deferred / dependency-gated

- Four factory samples baked into firmware: wait for source audio.
- Any feature that requires unresolved Control Grammar v2 semantics.
- Large synth/sequencer expansion before Gate A/B/C.
- Chord implementation work before the harmonic-layer design decision.
