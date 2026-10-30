# DECISIONS

## Product and scope decisions

### Randomizer is core

The randomizer is a central creative function, not a secondary utility. Architecture, UI and testing decisions must preserve its ability to generate inspiring musical starting points.

### Step 4 intensity semantics

Use three musical depth modes:

- `LOW`: conservative changes around current material.
- `MED`: current/default behavior as closely as practical.
- `HIGH`: broader structural and timbral movement while staying useful.

Intensity is not a global probability knob. It changes target-specific mutation ranges/depth.

### MED is the compatibility anchor

Where practical, preserve the original randomizer code path and RNG consumption order for `MED`. If exact parity is impossible, document the difference explicitly and prove that the creative range has not been weakened.

### Target mask is independent

Changing intensity must never silently enable, disable, add or remove random targets. `WAVE` remains a standalone independent target.

### Functionality preservation

Do not remove working musical functionality merely to simplify the interface. Prefer reorganizing related functions under a coherent interaction grammar. Removal/default-hidden/compile-out decisions require explicit product-owner approval.

### User audition can reopen technical acceptance

A technically green feature is not product-complete if hands-on audition exposes a meaningful musical or interaction defect. Keep measurements/regressions, but reopen product acceptance when necessary.

This rule currently applies to Reverb V2: its technical baseline is preserved, but perceptual product acceptance is reopened because metallic/ring-mod-like character remains audible.

## Locked decisions from the 2026-10-25 product workshop

The product owner approved all workshop recommendations on 2026-10-25. The following are now requirements, not proposals. The WP-D default below was subsequently revised by the product owner on 2026-10-28 and supersedes the workshop's original `1 BAR` default.

### Main-sequence timing

- Main sequence uses a bar-locked musical default.
- Default is **`2 BAR`**, meaning reset after two 4/4 bars.
- Explicit choices are `1 / 2 / 4 / 8 / 16 BAR / FREE`.
- A BAR choice is a musical duration, not a fixed raw processed-pulse count; changing sequence pulse division must not change the selected number of bars.
- `FREE` remains an explicit option for natural/polymetric sequence length.
- Main-sequence bar reset is distinct from Loop Cycle `SYNC / FREE`, Slot/Chain hold length and external MIDI/transport sync.

### REC and EDIT

- `REC` is reserved for actual recording/capture semantics.
- REC must not be the generic page-specific action button for REROLL, TAP, RESET or CLEAR.
- `EDIT` is reserved for edit focus/state/lock semantics.
- EDIT must not be a hidden modifier for unrelated or destructive actions.
- Page-specific actions move to explicit visible contextual actions.

### Black-key contextual action rail

The black keyboard keys will be prototyped and then standardized as a contextual action rail where useful.

Rules:

- every active black-key action is visible on screen;
- prefer stable verb placement across pages;
- destructive actions are visually distinct and require suitable hold/confirmation behavior;
- manual note-entry modes keep musical black-key behavior when required;
- do not consume a black key unless the page has a useful contextual action for it.

### Modulation family

- LFO and DOT are grouped conceptually under `MOD`.
- LFO remains a continuous cyclic modulation source.
- DOT remains a discrete matrix modulation source.
- Turing belongs to the broader generative/modulation family while retaining its evolving-register identity.
- Grouping must not flatten these into one algorithm or hide their different time models.

### Rotary mapping single source of truth

R1-R4 labels and actual rotary handlers must not be separately maintained mappings that can drift. Wherever practical, display label, formatter, legal range, acceleration and handler come from the same page/layer mapping definition.

### Reverb V3 is a defining Mello quality gate

Reverb is a defining part of Mello, not a utility effect. Reverb V2 measurements remain regression evidence, but Reverb V3 must pass perceptual listening criteria in addition to objective DSP tests.

Reverb V3 is not accepted merely for being stable, dense, decorrelated or efficient. It must be clearly preferred to V2 on representative Mello material and remove the metallic/ring-mod-like pitched tail that reopened V2. Persistent metallic pitch, obvious comb repetition/flutter, grainy sparse echoes, uncontrolled LF buildup, brittle HF hang, live-edit zipper/clicks or detached dry/wet integration are hard rejects.

The controlled A/B protocol and detailed criteria in `docs/WP_E_AUDIO_AUDIT.md` are authoritative for WP-E3. Objective metrics are guardrails; a failed listening result cannot be overruled by green numbers.

### Distortion range

Distortion gains an intentionally extreme upper range. The lower range must remain controllable, and gain compensation/headroom must prevent the feature from becoming only a loudness increase.

### Ratchet parity

Ratchet audibility is treated first as a parity/timing/envelope-retrigger task. The original HTML behavior, with envelope retrigger for every sub-trigger, is the reference before any musical redesign.

### Synth evolution

- Preserve the current engine as first-class `ANALOG` mode.
- Add a curated, immediate `FM` mode after stabilization gates are green.
- Do not expose generic operator complexity unless it proves musically useful on FM-1.
- Add `SOUND` as an independent randomizer target.
- Add per-step timbre locks/modulation after reliability/control/audio stabilization.
- Per-step timbre must be realtime-safe and must not allocate or rebuild audio objects on each step.

### Harmonic layer

Current Chord Machine remains regression-protected as a prototype until a successor wins by audition.

First prototypes to build and compare:

1. scale-aware `HARMONY` derived from lead/sub/key context;
2. soft filtered `PAD`.

Then compare as needed:

- `SUB HARMONY`;
- `CHORD V2` with filter/envelope/voicing;
- default-hidden/compile-out if no concept earns its space.

### Splash screen

A splash/boot identity screen is required before final product-polish acceptance.

## Deliberate departures from the HTML's sound

- **Distortion gain compensation (2026-10-29).** The HTML's Tone.Distortion gets louder as AMT goes up: a -8 dBFS
  sine ~11 dB at AMT 100, quiet tails up to ~16 dB, and its -1 dB limiter barely touches that. The FM-1 scales the
  distortion curve so the -8 dBFS reference sine keeps its AMT 0 mean level at every AMT
  (`build_main_dist_curve`, `MAIN_DIST_MAKEUP_REF`): AMT changes the character, LEVEL changes the loudness. So
  `drive_dry` in `make parity` is ~9 dB below the HTML by design; its tone still matches.

- **Audio LFO on AMP and PWM swings around the base (2026-10-29).** In the HTML, Tone's `connectSignal` sets
  `mix.gain` and the pulse `width` to 0 before the LFO drives them, so AMP swings the gain around 0 (silent at depth
  0, the polarity flipping every half cycle) and PWM swings the width around 0 (a square), not around 0.58 and the
  set width as `applyLfoRouting`'s comments intend. The FM-1 does what the comments say; the reference renderer
  (tools/tone-ref/chain.js) does the same, so `make parity` measures against the intent.

## Beyond the HTML (Phase 5)

- **Grit (2026-10-30).** The lead gets optional grit, after the Access Virus: noise, noise FM and unison. The HTML has
  none of it; all of it is off by default, and then the lead is sample for sample the HTML's plain saw
  (`audio_grit_smoke`, `make parity` unchanged). SOUND / GRIT:
  - **NOISE**: white noise mixed with the oscillator before the filter, so cutoff and resonance shape it, through a
    low cut (two one-pole highpasses at ~450 Hz, 12 dB an octave) so it stays out of the low register. Linear; at
    100% about half the saw's RMS (-9 dB). History: first squared and at most the saw (35% inaudible behind the
    default cutoff), then linear to 1.7x the saw (too much from 20-30%), now the latter's 30% at full.
  - **NOISE FM**: lowpassed noise (~900 Hz) moves the oscillators' frequency, through zero (the Virus's FM source
    "noise"), by up to ~70 Hz RMS at full: an absolute deviation, so that a bass note gets as rough as a high one;
    squared. History: +-100% of the note's frequency (inaudible on bass), then ~3 kHz RMS (usable only up to 15%),
    now that 15% at full.
  - **UNISON** 1-4 oscillators, **DETUNE** spreads them up to +-60 cents (squared). The level is 1/sqrt(n), so more
    voices are wider, not louder. The extra oscillators run free from spread phases (never a flanged attack). Mono:
    the FM-1's lead path is mono up to the reverb.
  - All three are LFO targets (NOISE, NOISE FM, DETUNE), swinging around the knob's value as PWM does.
  - Saved in their own section (SEC_GRIT, only when some sound has grit), for the working sound and each slot; an
    older build skips it and plays a plain saw.
- **Virus layer (2026-10-30).** More from the Access Virus, for those who turn far enough; all off by default, then the
  lead is the HTML's (`audio_virus_smoke`, `make parity` unchanged). SOUND's layers are OSC / FILTER, OSC 2,
  CHARACTER, GRIT, MOD ENV, then the HTML's AMP ENVELOPE, FOLLOWER SUB and LEVEL.
  - **OSC 2**: WAVE, SEMI (+-24), FINE (+-50 cents, 7 by default so it thickens when mixed in) and MIX (0 = OSC 1
    only, 100% = OSC 2 only). MIX 0 is "off"; there is no OFF wave. One oscillator, not unison.
  - **CHARACTER**: SYNC (OSC 2 restarts with OSC 1's cycle, at the matching fraction of its own), FM (OSC 1 modulates
    OSC 2's frequency, linear through zero, up to 4x; squared knob), SAT (soft clipping into the filter, up to +24 dB
    of drive, squared knob, level kept within ~2 dB), FILTER LP / HP / BP / NOTCH. Turning SYNC or FM up with OSC 2's
    MIX at 0 brings OSC 2 in at 50%: otherwise they would do nothing audible.
  - The filter modes share the lowpass table's poles (cutoff and resonance as the HTML's) and change only the zeros.
    BP is 0 dB at the cutoff, so it is quiet below a high cutoff, as on the Virus.
  - **MOD ENV**: a second envelope, restarted with each note: AMT (-100..+100%), ATK (0-2 s, linear), DEC (5 ms-5 s,
    to -60 dB), TARGET CUTOFF (full range) / PITCH (+-24 st) / OSC 2 (+-48 st: the sync sweep) / FM / NOISE / RES.
  - The LFO also reaches RES, OSC 2 (+-12 st), FM and SAT.
  - Saved with the grit in SEC_GRIT, version 2 (26 bytes an entry); version 1 entries are still read.
- **DOT and EVOLVE routes (2026-10-30).** DOT and EVOLVE reach the sound's new parameters, both can keep a pattern
  moving by itself, and both change only at the loop's one (asked for: "in sync with the one after the sequence").
  - **DOT's MOD lanes.** The HTML's FILTER matrix is MOD 1; MOD 2 is new (a fifth matrix: DOT's tabs ACC, MOD 1,
    MOD 2, RATCH, SUB). A MOD lane's amount is its DEPTH, in permille of its target's range: 24% (240) by default,
    the HTML's filter dot at 100%. Each lane has a SHAPE layer (SELECT after the lane): TARGET (cutoff, res, decay,
    pitch +-12 st, OSC 2 +-24 st, FM, noise, noise FM, sat, detune, PWM), ATK, REL and HOLD: HIT (each lit cell rises
    in ATK, then falls in REL: AR) or GATE (lit cells in a row hold, an unlit one releases: ASR). Defaults: MOD 1 on
    CUTOFF, MOD 2 on NOISE, 20 ms / 140 ms HIT: the HTML's filter dot. Departures: a hit starts from where the lane
    is, not from 0, and an unlit pulse no longer snaps the lane to 0 (both clicked); a lit cell shapes the sound
    whether or not a note plays on that pulse.
  - **DOT's EVOLVE** (K4, per matrix; FOLLOW moved to a black key, FLW): at the loop's one each lit cell moves to a
    free neighbouring cell with a chance of EVOLVE / 3, so a pattern drifts without thinning out or filling up.
  - **EVOLVE's timing.** The HTML mutates its registers on every pulse. Now, by default (black key LOOP, lit), the
    same amount of mutation, one per pulse of the loop, happens at the loop's one: the loop plays as it is, and the
    next one is a variation. LOOP off is the HTML's.
  - **EVOLVE -> sound.** A fourth layer, SOUND: two routes, DEST A / B (any of the MOD targets) and DEPTH A / B
    (+-100%): the register value at each pulse, held, as the HTML's FILTER route does.
  - The loop is the master length (`loop_pulse_length`), 2 bars by default.
  - Saved in SEC_MOD (40 bytes a working pattern or slot, only when it differs from the defaults). MOD 1's amount
    stays in SEC_WORKING / SEC_SLOTS with the HTML's meaning (1000 = depth 240); SEC_MOD carries the exact depth.

## SUB DRONE (2026-10-30)

A fifth sub mode, after TM: the sub as a drone, sounding from START to STOP. The engine sends one SUB NOTE ON at the
first step and no OFF until STOP, SUB off or another mode; the pitch is worked out at every step as in the other modes
(KEY, OCT, transpose, the 5SEQ's tone and octave), and a new pitch sends the old note's OFF and the new ON in the same
tick (MIDI out never hangs), which the voice plays as a glide (GLIDE) from where it is. The 5SEQ's off steps do not stop
it. The voice holds the attack's full level instead of decaying to 24%: LEVEL is the drone's level, DECAY does nothing,
RELEASE is its tail after STOP. SUB MUTE fades it out and in as any bass. The HTML has no drone.

## The randomizer and the sound (2026-10-30)

Feedback: the randomizer ruined the synth sound and there was no way back. The HTML's default mask rolls everything;
the FM-1's now rolls the first nine targets (PIT, PUL, RAT, GAT, OCT, ACC, SUB, SLD, SKP), the pattern: WAVE, CHORD,
SMP and PRB are off until switched on (a departure from the HTML). SOUND has INIT (black key 1, press twice within 3 s,
as CLR2): the sound back to the defaults, undoable. OSC 2 is OFF by default and has OFF as a wave; turning its WAVE, MIX,
SYNC or FM up brings it in (SAW, mix 50%).

## Randomizer amounts (2026-10-30)

RANDOM's EDIT (the EDIT button, lit): a white key picks a target, K1 sets its amount in 5% steps, drawn as a bar under
each pill. The amount is the chance that a roll changes a step's value (for WAVE and CHORD, the sound's), so 30% pitch
re-rolls about a third of the notes and keeps the rest. The mask still says what is rolled at all; links act only
where the roll reached. 100% draws nothing extra, so default rolls are exactly what they were. Saved in SEC_MOD
version 3 (57 bytes an entry); versions 1 and 2 are read as 100%.

## Randomizer links (2026-10-30)

The randomizer (RNDMZR, the RANDOM page) rolled each target on its own. Now a rolled target can be linked to the step's
pitch, the skyline the RANDOM page draws: **+** follows it, **-** mirrors it (asked for: "one parameter mirrors another
inverted"), so a roll comes out as one musical gesture rather than unrelated noise. RANDOM's second layer, LINKS
(SELECT): a target's key steps free -> + -> - -> free, and linking a target rolls it.
- The source is the pitch after the roll (0 the lowest the scale and span allow, 1 the highest). Linkable: pulses and
  ratchets (1-4 by it), chance (30-100%), and accent / sub (up to 70% likely), slide (60%), skip (40%), drawn with it,
  so a roll stays a roll but leans with the melody. Pitch is the source; gate, octave, sample, chord and wave stay free.
- Every intensity (LOW, MED = the HTML's, HIGH) applies the links after its roll; nothing is linked by default.
- Saved in SEC_MOD version 2 (44 bytes an entry); version 1 is still read.
- The skyline already takes the sequence's length (bars as wide as 224 px / steps).

## Velocity and key tracking (2026-10-30)

SOUND / TRACKING, after MOD ENV. The FM-1's velocities are the HTML's: a plain step 91 (0.72), an accent 121 (0.95);
MIDI and KEYS notes can be anything. Tracking counts from the plain step, so a plain step sounds as in the HTML
whatever the knobs, and the defaults change nothing (`make parity` unchanged).
- **VEL > CUTOFF** (-100..+100%): an accent opens the filter by up to 30% of its range (closes it, negative).
- **VEL > LEVEL** (0..200%): how far an accent's level is from a plain step's; 100% is the HTML's (+2.5 dB), 0 none.
- **KEY TRACK** (0..100%): the cutoff follows the note from C4; 100% is one to one (an octave up, the cutoff an
  octave up), so high notes stop sounding duller than low ones.
- **VEL > MOD ENV** (-100..+100%): an accent's MOD envelope up to twice as deep (or none).
- Saved with the grit in SEC_GRIT, version 3 (34 bytes an entry); versions 1 and 2 are still read.

## DOT orders (2026-10-30)

Beyond the HTML's HORIZ, VERT, SNAKE and RANDOM, five routes; the chord trigger grid gets them too (one routine,
`dot_route`):
- **P-PONG** there and back without doubling the ends (2 x LEN - 2 long, so it drifts against the sequence and meets
  it again); **SPIRAL** from the outside in (LEN 12 is the outer ring); **DIAG** the diagonals, one more cell each
  then fewer; **BOUNCE** down a row each read while bouncing side to side, 12 reads long against 16 steps;
  **WALK** each read to a random neighbour, so it wanders but stays near where it was.
- LEN is the route's first cells; for BOUNCE and WALK it is the cells they may visit (1..LEN).
- **REV** is a direction, not a route: a black key on the DOT page runs any route backwards (WALK and RANDOM have
  none to reverse). It is saved in bit 7 of the mode byte, which older builds read as HORIZ.
- On screen the route is numbered in its cells (the first time it reaches each; REV counts from the end), and MODE is
  one pill by name: eight pills would not fit, and the numbers show the route before it is heard.

## Performance keys (2026-10-30)

HOME's black keys, in the keyboard's groups: `- + MUT | FL SB | << RP |< | CL UNDO | RNDMZR`. RNDMZR is the last key
(F#5) and UNDO the one before it on every page, so the key that is always there sits where it is easy to find. Eleven
labels fit 240 px only in two letters each (the font is 8 px); a held key's full name and what it does replaces the
strip while it is held (`REPEAT 4  WHITE KEY = LEN`).
- FILL, SUB MUTE, REVERSE and REPEAT act while held, RESTART on the press. They are performance, not editing: nothing is
  saved, undone or marked as changed, and they act on HOME only (not in KEYS, not with a step held).
- **FILL**: every step plays (chance and rests aside; skipped steps stay out of the sequence), ratchets doubled.
- **SUB MUTE**: no new bass notes (also on MIDI), and the sounding bass, dedicated and follower sub, fades in ~10 ms.
- **REVERSE**: what was heard, backwards, from the press (the last 64 pulses, round and round). **REPEAT**: the last
  pulses round and round, 4 (a beat) unless a white key held with it says 1-16.
- Both keep time: the sequence's own place moves on underneath, and letting go lands where it would have been, so
  the 2-bar loop, DOT and EVOLVE (which evolve at the loop's one) stay in step. They play the history of pulses, so
  they work whatever the segments do (ping-pong, random).
- **RESTART**: the first step and the loop's one at the next pulse (16th); Loop Cycle and the slot sequence stay.

## Sequence overview

HOME, STEP, SEGMENT and RANDOM keep a compact 4..32-step visual overview. Do not reintroduce visual paging. At 16 steps the overview must be comfortably readable, while 17..32 must still remain visible and retain meaningful pulse/event information.

## Step boundaries

Finish and verify Step 4 before implementing later product expansion. A P0 reliability defect may be fixed earlier only if it blocks meaningful Step 4 testing. Undo Last Random stays separate from Step 4.

## Engineering process decisions

- GitHub repo state is the durable source of truth between chats.
- Read current branch HEAD and project docs before editing.
- Work in small verifiable packages.
- Do not claim completion without exact commit and CI evidence.
- Diagnostic probes may intentionally fail CI, but that state must be clearly documented and removed/disabled before declaring the real feature green.
- Perceptual audio acceptance needs listening criteria in addition to numerical DSP tests.
- Do not implement several approved future work packages at once merely because their product direction is now locked.
