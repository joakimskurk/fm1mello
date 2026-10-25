# FM-1 Mello product workshop - 2026-10-25

Status: **APPROVED PRODUCT DIRECTION**. The product owner approved all workshop recommendations on 2026-10-25. This document still does not claim that the approved changes are already implemented.

## Why this workshop exists

A substantial hands-on emulator audition showed that FM-1 Mello is already musically productive and can make compelling loops, but several interaction and audio-quality issues make it harder to understand than it needs to be. The goal is not to reduce functionality. The goal is to make the same depth feel coherent, then add new depth without creating a menu maze.

## Raw audition findings, normalized

### P0 reliability and trust

- Audio can freeze while parameters are being changed. Stopping and pressing PLAY again often recovers it.
- Some pages show rotary labels that do not match the physical rotary that actually changes the value.
- Some screen layouts overlap or clip text.
- A static pink line appears over `SINGLE` on HOME.
- These issues damage trust because the user cannot tell whether a surprising result is intentional or a bug.

### P1 control grammar

- REC has unrelated meanings across pages: reroll, tap tempo, reset and other contextual actions.
- EDIT is overloaded as both editing state and modifier, including destructive/randomizer actions.
- LFO is currently reached through a sound/envelope-oriented path and therefore feels misplaced.
- DOT matrices behave as modulation/generative modulators and conceptually belong with modulation.
- R1-R4 labels and actual encoder mappings must be guaranteed to share one source of truth.
- Black keyboard keys are underused and are approved as a contextual action rail where useful.
- SEQ pages work, but their purpose is not self-explanatory enough without documentation.

### P1 timing model

The original HTML separates two concepts that must remain separate:

1. Main-sequence master reset: `off / 1 / 2 / 4 / 8 / 16 bars`, used to force the melody back to step 1 against fixed-length loops.
2. Loop Cycle `SYNC / FREE`: SYNC aligns the Loop Cycle counter to main-sequence pattern boundaries, FREE runs independently.

Approved FM-1 product rule: main sequence defaults to `1 BAR`, while FREE remains available as an intentional polymetric/free-running mode.

### P1 randomizer

- Randomizer remains a core creative instrument.
- LOW/MED/HIGH intensity remains the active work package.
- The skyline needs higher information density. At least 16 steps must be comfortably legible, and 17-32 steps must still remain visible in the same overview.
- Pulse level, ratchet and other event information must remain readable when the sequence is compressed.
- `SOUND` is approved as a future independent randomizer target after stabilization.

### P1 audio quality

- Distortion does not reach an extreme enough range and is approved for an intentionally extreme upper range with gain management.
- Reverb V2 is technically stable but is not product-accepted. Reverb V3 is approved as a perceptual redesign task.
- Ratchets can be hard to hear. Original HTML retriggers the synth envelope for every ratchet, so the native path must be checked for envelope retrigger and sub-step duration parity before any redesign.
- Chord Machine can produce useful results, especially with sine, but the current voice is raw/bright and lacks enough tone shaping.

### P2 UI and onboarding

- HOME is usable but should feel more polished and instrument-like.
- Key/Scale is usable but can be clearer.
- SEQ pages need stronger visual explanation of what they are doing now, not just parameter names.
- Splash screen is required.
- No page may rely on clipped text, hidden mappings or prior knowledge of the browser prototype.

### P2 synth evolution

Approved future direction after stabilization:

- explicit `ANALOG / FM` sound-engine choice;
- preserve the current synth as first-class ANALOG mode;
- curated FM mode that is musically immediate rather than a generic operator-programming exercise;
- `SOUND` randomization as its own selectable randomizer target;
- per-step timbre locks/modulation so a sequence can change sound, not only note/rhythm.

## Original HTML facts that guide the port

### Main sequence bar reset

The HTML exposes `RESET AFTER` values of off, 1, 2, 4, 8 and 16 bars. Its own help text says the purpose is to keep melody aligned with fixed-length drum loops. The HTML default is `off`; the approved FM-1 default is now `1 BAR` with FREE/off explicitly available.

### Loop Cycle sync

Loop Cycle has a separate `SYNC / FREE` control. SYNC aligns its tick counter to main-sequence pattern wrap. FREE is independent.

### Ratchet behavior

The HTML describes ratchet as micro-retriggers per pulse. With ratchet > 1 it divides the pulse duration by the ratchet count and calls synth `triggerAttackRelease` for every sub-trigger. This remains the reference behavior for FM-1 parity.

### Reverb reference

The HTML uses `Tone.Reverb` when available and `Tone.Freeverb` only as fallback. The wet path is separate from dry and includes send, reverb, tone LPF, 24 Hz HPF, return level, FX mix and limiter. Native Reverb V2 remains a useful technical baseline but no longer passes the perceptual product gate.

## Simulated expert workshop

This is a simulated design discussion. Trent Reznor, Alessandro Cortini, Richard Devine and Nils Frahm did not participate and are not being quoted as endorsing FM-1 Mello. The perspectives are inferred from documented public interviews about their workflows.

### Trent Reznor perspective: sound should create the next decision

Applied here:

- distortion should have a genuinely dangerous upper range, not stop at polite saturation;
- FM mode should be a sound-discovery surface, not an engineering worksheet;
- per-step timbre is valuable because a loop should evolve sonically as well as melodically;
- unexpected sound is good, but the control that caused it must remain legible.

### Alessandro Cortini perspective: one coherent instrument

Applied here:

- DOT, LFO and other modulation generators should read as one modulation family;
- a good feature should feel discoverable from the panel, not from the manual;
- Chord should be judged by whether it creates a useful musical relationship, not by whether it is technically a chord synth;
- the machine can be deep, but it should feel like one instrument.

### Richard Devine perspective: controlled emergence

Applied here:

- the randomizer remains central and musically weighted;
- LOW/MED/HIGH alters the size of the creative jump, not simply the chance that anything happens;
- modulation matrices are valuable when their movement is visible and can be steered;
- HIGH should surprise without routinely destroying musical usefulness.

### Nils Frahm perspective: limitation is an interface feature

Applied here:

- FM expansion uses a deliberately curated parameter set;
- 1 BAR lock is the understandable main-sequence default, while FREE remains available for intentional complexity;
- every encoder must do what the screen says;
- no feature is improved by exposing every possible parameter at once.

## Simulated developer panel

### Embedded DSP engineer

1. Fix parameter-change audio freezes before heavier synthesis.
2. Add parameter smoothing/ramping at the control-to-audio boundary and audit any path that rebuilds runtime/audio state from UI changes.
3. Reverb V3 needs perceptual quality gates; low correlation alone does not prove good diffusion.
4. Ratchet tests need event timing plus envelope-retrigger evidence.
5. FM mode needs CPU/RAM budgets before its UI is locked.

### Interaction designer

1. Physical controls need global verbs.
2. PLAY only means transport.
3. REC is reserved for record/capture.
4. EDIT is reserved for edit/focus/lock.
5. SELECT continues to change functional layers.
6. White keys remain direct position/note selection.
7. Black keys are approved as visible contextual actions where useful.
8. Encoder labels and handlers come from the same mapping source.

### Sequencer designer/live performer

1. Separate transport sync, main-loop reset length, Loop Cycle sync and Slot/Chain hold length in both naming and UI.
2. Default main sequence to 1 BAR, with FREE clearly available.
3. Show what a SEQ page changes in relation to the running loop, not only abstract values.
4. Keep 4-32 steps visible at once. Compression is better than hidden visual banks.

## Workshop consensus

> Keep the depth, reduce the number of grammars.

Approved conceptual grouping:

- `SEQ`: note/rhythm/structure/timing.
- `SOUND`: analog/FM source, filter, envelope, per-step timbre capability.
- `MOD`: LFO, DOT matrices and Turing/generative modulation.
- `FX`: distortion, delay, reverb.
- `RANDOM`: target selection, intensity, reroll, undo and later SOUND target.
- `GLOBAL`: tempo, external clock/MIDI and project-level behavior.

This grouping is approved as product direction. Menu/navigation implementation still proceeds work-package by work-package.

## Approved work packages

### WP-A: Step 4 completion - ACTIVE NEXT IMPLEMENTATION

- LOW/MED/HIGH intensity.
- MED compatibility.
- target isolation.
- dense 4-32-step RANDOM skyline.
- no unrelated feature implementation.

### WP-B: trust and reliability gate

- reproduce/fix audio freeze while turning controls;
- rotary label/handler parity on every page/layer;
- eliminate clipping/overlap;
- remove HOME pink-line artifact;
- add automated mapping/text-fit coverage where possible.

No major new synth feature ships before WP-B is green.

### WP-C: Control Grammar v2

- migrate REC to record/capture only;
- migrate EDIT to edit/focus/lock only;
- audit SELECT/OCT behavior;
- implement/prototype black-key contextual action rail from a complete page-action table;
- group LFO + DOT under MOD and position Turing in the same broader family;
- add compact on-screen action hints.

### WP-D: timing model

- explicit main-sequence `1 BAR / 2 / 4 / 8 / 16 / FREE` concept;
- default `1 BAR`;
- keep Loop Cycle `SYNC / FREE` independent;
- clarify Slot/Chain hold semantics;
- verify full-sequence playback and 16-step assumptions;
- use names that cannot be confused with external clock sync.

### WP-E: audio character

- distortion extended range with usable gain compensation;
- Reverb V3 perceptual redesign/reference audition;
- ratchet envelope/timing parity;
- parameter smoothing and no audio dropout under live editing;
- preserve delay and current working sound.

### WP-F: random undo

- one-level pre-random snapshot after core randomizer and trust gates are stable.

### WP-G: synth evolution

- preserve current engine as ANALOG;
- design curated FM engine;
- SOUND randomizer target;
- per-step timbre locks/modulation;
- hard CPU/RAM/performance budgets and graceful feature limits.

### WP-H: harmonic layer decision

Prototype in this order:

1. scale-aware `HARMONY`;
2. soft filtered `PAD`;
3. `SUB HARMONY` if useful;
4. `CHORD V2` if useful;
5. default-hidden/compile-out if none earns its space.

Current Chord remains regression-protected until a winner is chosen.

### WP-I: instrument polish

- splash screen;
- HOME visual refinement;
- Key/Scale refinement;
- SEQ explanatory visual hierarchy;
- consistent text margins/truncation policy;
- final onboarding pass for a user who has never seen the HTML prototype.

## Product-owner decisions - APPROVED

- [x] Make main-sequence bar lock the default and keep FREE as an explicit option.
- [x] Use `1 BAR` (16 sixteenth-note ticks) as the initial default.
- [x] Reserve REC for actual recording/capture and remove reroll/tap/reset meanings from REC.
- [x] Stop using EDIT as a hidden modifier for unrelated/destructive actions.
- [x] Prototype black keys as a contextual action rail with visible on-screen labels.
- [x] Group LFO + DOT under a common `MOD` concept.
- [x] Keep Turing in the generative/modulation family but separate from simple cyclic LFO.
- [x] Re-open Reverb as Reverb V3 with listening quality as a hard gate.
- [x] Extend distortion into an intentionally extreme range.
- [x] Treat ratchet audibility/envelope retrigger as a bug/parity task before changing the musical concept.
- [x] Preserve current synth as `ANALOG` and add a curated `FM` mode.
- [x] Add `SOUND` as an independent randomizer target.
- [x] Add per-step timbre locks/modulation after the control/reliability gate.
- [x] Explore scale-aware `HARMONY` as the first Chord successor prototype.
- [x] Explore `PAD` as the second Chord successor prototype.
- [x] Keep current Chord Machine only as a regression-protected prototype until one direction wins.
- [x] Add splash screen before final product polish is declared complete.

## Public references used to simulate the musician perspectives

- Sound On Sound, Classic Tracks: Nine Inch Nails - Closer: https://www.soundonsound.com/techniques/classic-tracks-nine-inch-nails-closer
- Alessandro Cortini interview, Headphone Commute: https://headphonecommute.com/2021/06/11/interview-with-alessandro-cortini-2/
- Alessandro Cortini / Strega interview, Pitchfork: https://pitchfork.com/thepitch/alessandro-cortini-interview-strega-synthesizer-nine-inch-nails/
- Richard Devine interview, MusicRadar: https://www.musicradar.com/news/tech/richard-devine-interview-578399
- Nils Frahm interview, MusicRadar: https://www.musicradar.com/news/tech/nils-frahm-talks-spaces-synths-improvisation-and-production-587377
- Nils Frahm live workflow, DJ Mag: https://djmag.com/longreads/how-i-play-live-nils-frahm
