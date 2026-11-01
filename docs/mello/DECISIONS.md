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

## The reverb's way in and out (2026-10-30)

Feedback: "the reverb crackles in the highs". The network runs at fs / 2; it was fed the average of two samples and
its output linearly interpolated. The average lets most of 11-22 kHz through (a 15 kHz partial only 6.8 dB down),
folded below 11 kHz as inharmonic tones the reverb's tone filter cannot remove; the hats, CRUSH, the noise and the
distortion are full of it. Now a 31-tap half-band filter (Kaiser 7.5; flat to 8 kHz within 0.03 dB, -48 dB from
14 kHz) on the way in and on the way out (polyphase: 17 multiplies a sample in all). A 15 kHz partial now reaches the
network 64.9 dB down. Parity moves a little both ways (HOT FX level 0.0 dB from -0.4; REVERB CLICK's low band +19.3 dB
from +17.6, its tone 6.5 from 6.1, RT60 2.86 s from 2.88 against the HTML's 2.69); tests/golden/parity.json updated.

## Drums (2026-10-30)

A small drum machine, the FM-1's own (the HTML has none): "minimal, classic, Aphex Twin meets Richard Divine; the
sonic differences come from modulation and the randomizer". Four synthesized voices on a 16-step pattern.

- Voices, each with TUNE, DECAY, TONE, LEVEL: KICK (a sine falling from four times its pitch in ~25 ms; TONE drive),
  SNARE (two body tones and high-passed noise; TONE the snap), HAT (noise and two ring-modulated squares, high-passed;
  TONE noise .. metal, DECAY closed .. open), GLITCH (FM at an inharmonic ratio, its index falling fast; TONE the ratio).
- Per step, per voice: the hit, an accent (ACC), and locks: PITCH +-24, DECAY (up to 4x shorter or longer), CRUSH
  (sample and hold up to 16x and up to 11 bits dropped, on top of the voice's CRUSH), RATCH 1..4 (the later hits of a
  roll softer).
- Per voice CHANCE; for the kit LENGTH, SWING (the odd steps up to half a step late), SPACE (its send), LEVEL.
- The clock: a step every engine pulse, whatever the sequence skips, holds or REVERSE / REPEAT play; FOLLOW steps with
  the sequence's steps instead. It starts on the one at START.
- Rolls: ROLL (B6) rolls the drums as much as ROLL says: each step's hits by voice (the kick likely on the one and the
  third beat, the snare on the backbeat), accents and locks; the kick always on the one. FEED (B7): RNDMZR rolls them
  with the pattern. Both are undone by UNDO. They draw from their own random stream, so the other rolls stay as they
  were.
- The page (BASS: SUB, CHORD, DRUMS): four rows in the voices' colours (coral, gold, cyan, purple), the picked one lit,
  its picked step framed, accents bright, rolls as dots, locks marked, the playing step's column framed; the white keys
  are the steps (a key toggles the picked voice's hit there and picks the step), B0-B3 pick the voice, B4 ON, B5 ACC.
  Layers: VOICE, STEP (the picked step's locks), VARY (CHANCE, CRUSH, SWING, ROLL), KIT (LENGTH, FOLLOW, SPACE, LEVEL).
- The sound: past the lead's filter and distortion, a little over the lead at LEVEL 70%. Cost: up to about 20% more
  audio render with all four voices on every step (about 5% of the chip). Since the redesign below, up to 31%.
- The default beat (feedback: "that one sounded very good"): a full ROLL of the first kit with SWING 30% and CRUSH 40%,
  kept as it came out; GLITCH has no hits in it. The drums are off until ON.
- The kick tightened (2026-10-31: "it eats the bass's room, a long release and pitch changes; more percussive than
  tonal"). The default beat had ten kicks a bar, each ringing ~580 ms (one step at 120 BPM is 125 ms; a lock made one
  1.7 s). Its steps' PITCH moved it -4..+4 semitones, outside the key, against the sub. Now the beat has six kicks
  (steps 1, 5, 7, 12, 13, 15). DECAY is ~190 ms, the drive is halved, and the pitch falls in ~25 ms, not ~40. No
  step's PITCH or DECAY makes the kick longer or retunes it. ROLL keeps it so: it never rolls the kick's PITCH, never
  rolls a DECAY longer than the voice's, and lays a little fewer kicks (22%, not 28%). With a MUSICAL roll over it, the
  mix has 7.5 dB less at 30-90 Hz and 4 dB less at 90-250 Hz, and the same above.
- The voices redesigned (2026-10-31: "the drums should sit more; now they sound very 8-bit and flat"). The 8-bit
  came from CRUSH at 40% on every voice and on many steps, from sines read from the 256-point table without
  interpolation, and from snare and hat noise open to 22 kHz. The flatness came from a kick with no attack, one level
  for every part of a voice, and everything in mono. Now:
  - KICK: an interpolated sine whose pitch falls twice, a click from 7x in ~6 ms and a body from 1.7x in ~60 ms. Its
    level is held up to 12 ms before it falls, which gives the punch. A short burst of noise above ~2 kHz rides on the
    click.
  - SNARE: a body sine falling from 1.5x in ~25 ms, with a mode at 1.59x. The noise is band-passed (~900 Hz up; TONE
    sets 4-10 kHz down) and twice as loud for its first ~25 ms (the snap).
  - HAT: six square waves at the TR-808's ratios, summed and band-passed (two high-passes ~5 kHz, a low-pass ~10 kHz).
    TONE goes from noise to metal.
  - GLITCH: as it was.
  - Kick and snare share a soft cubic saturation (x - x^3/3 on 1.5x), so their peaks lean on each other. The hat sits a
    little right and the glitch a little left.
  - CRUSH is 0 on every voice and step of the default beat. It stays as an effect.
  At the default kit a hit is within 1 dB of its old level (the kick +1.9 dB RMS from its hold). Cost: up to +31% audio
  render with all four voices ringing all the time, against +16%, so about 4% more of the chip. The kick's click and
  the snare's body stop computing once they are down 60 dB.
- The default beat is BROKEN (2026-10-31, chosen from five: rolled, electro, broken, halftime, techno). It has a
  syncopated kick (steps 1, 4, 8, 11, 14), a snare on the backbeat with a ghost at step 10, sixteenths of hat with gaps
  and a roll of four at step 16, and three glitches that jump in pitch. It is straight: SWING 0, because the sequence
  has no swing. At 200, the drums' odd steps came 10% of a step late (12.5 ms at 120 BPM, 19 ms at 80), and a lead or
  sub note on the same odd step flammed against them. A MUSICAL roll plays about a third of its odd steps.
- SWING is global (2026-10-31, "let's try"). It is the same control, on the drums' VARY layer, but it now moves
  everything an odd sixteenth plays: the lead, the sub, the chords, the samples and the drums all come equally late,
  so nothing flams. It works on the engine's pulse grid. Everything a pulse queues is shifted together
  (`queue_push`), so a note keeps its length, and a NOTE_OFF that lands in the next pulse is still its own note's
  (`mello_audio` releases only on a note's own OFF). The loop's wrap and the slots' changes stay on the grid. At 0 it
  renders sample for sample as before (the HTML has no swing). tests/mello/swing_global.c checks it.
- The glitch kit (2026-10-31: "the drum design sounds very 16-bit FM; I want something modern and glitchy: Richard
  Devine, Aphex Twin, glitch music"). The voices were classic synthesized drums: a sine kick, a snare of sines and
  noise, an 808 hat and an FM glitch. That is how a 16-bit console makes drums. Glitch music builds them from other
  things: noise through resonant filters that sweep, micro-sounds and clicks, the music itself cut into loops, rolls
  that rise, every hit somewhere else. Now:
  - KICK: as before (a short punch, a two-stage pitch fall, a click).
  - SNARE: noise through a state-variable band-pass that sweeps down from 4x its TUNE (0.9-3 kHz) in ~18 ms, with a
    short falling body under it, driven. TONE goes from a broad crack (Q 0.7) to a resonant zap (Q 12). Its tail is
    gated: at -18 dB it falls four times as fast.
  - HAT: noise high-passed and ringing at 5-9 kHz (TUNE). TONE crosses it with a tick, a sine burst there gone in ~4 ms
    (the Ikeda click).
  - GLITCH: TONE picks the kind, in quarters, and the page names it. BLEEP is the old FM, its ratio x2-7 within the
    quarter. CHIRP is a sine falling from 8x in ~12 ms, ring-modulated. CRACKLE is noise held for longer and longer
    (digital rain). STUTTER loops the last 6-46 ms (TUNE) of the music and the drums. It is the default: the drums cut
    up the sequence itself. PITCH shortens the loop.
  - The hat and the glitch scatter: each hit lands up to 60% to either side. Kick and snare stay in the middle,
    through the glue.
  - A roll (RATCH) rises 2 semitones a hit and tightens 15 DECAY a hit, as drill'n'bass rolls do. The kick's rolls
    don't rise.
  The drums now render every sample while ON (STUTTER keeps listening, a 2048-sample memory, 4 KB). Cost: +36% audio
  render with all voices ringing all the time, against +32% for the kit before.
- DIST and FEED+ (2026-10-31: "some optional dist on this would be good, and RNDMZR playing into the parameters").
  - DIST: a fifth layer with DRIVE and FOLD on the whole kit, after the glue. DRIVE is up to 8x into a soft clip.
    FOLD adds up to 4x more and crosses it with a fold, where the wave turns back past full scale (a triangle, worked
    out without a loop). Both are 0 by default: off, and nothing computed. The level comes down as the drive rises,
    but the tails come up, as in any distortion: at 40% the kit is ~6 dB louder (RMS) and the mix with the default
    lead ~4 dB (MIX's PERC takes it back). They are saved in SEC_DRUMS's spare word, in percent, so the version stays 1
    and older files read as 0.
  - FEED+: B7 now goes OFF, FEED (RNDMZR rolls the pattern), FEED+ (and the sound), OFF. FEED+ shows as "FED+",
    because the strip has no room for one more letter. With FEED+, a roll (ROLL's or RNDMZR's) also draws each voice's
    TUNE, DECAY and TONE, each with ROLL's chance, within what suits the voice. The kick stays a kick: TUNE 25-60%,
    DECAY up to 25%, TONE up to 35%. GLITCH's TONE can pick any kind. FEED+ is in SEC_DRUMS's flags (bit 3).

## One thread through the UI (2026-10-31)

ROADMAP "Now: one thread, and room to run", C. The map of every page (docs/mello/UI_MAP.md, generated) showed the
same bevegelse meaning different things from page to page. The rules, decided 2026-10-31, and how far each is:

- **R5, one word per thing, fixed units** (done). The footer's and the panels' labels are full words: LENGTH,
  DECAY, ATTACK, SUSTAIN, RELEASE, ACCENT, MUTATE, ROTATE, INTENS, CHORDS (the chain's count; NEXT is how it moves).
  The filter envelope is the one abbreviation kept, F.ATK and F.DEC, since ATTACK and DECAY on the same page are the
  amp's. A layer is never named as an installing (SUB's TRIG layer is PLAY). Units: every 0..1000 level or amount
  reads in % (SUB, CHORD, SAMPLE and EVOLVE's depths were bare numbers); every time in ms, or s above a second (the
  drums' DECAY shows the time it is, per voice; MIDI's fixed length too); every CUT in Hz (the sub's 40 x 55^x, the
  chord's 60 x 107^x, as the lead's already was); pitch locks in ST; relative locks signed with their unit (+0%). An
  unused knob has no label and "-". tests/mello/ui_glossary.c renders every page and layer and holds this.
- **R2, the header says where you are** (done). The top line reads BUTTON > PAGE > LAYER: the printed panel button
  dim, the page in the section's colour, the layer bright ("ARP > DRUMS > KIT", "ENV > SOUND > GRIT"); a page named
  as its button shows once ("FX > DELAY"). HOME keeps its own line (segment, KEYS / REC, the chord sounding). The
  readout's "where" line is the same path. The layer's name left the body (ui_section's right side, the parameter
  panels' title line), so it stands in one place. The key ("C MINOR") moved from the header to the status line, with
  the step of the length (12/16, or "16 ST" stopped), the slot and the save state (SAVED / EDITED / NOSAVE). Layer
  names were shortened to fit 24 characters (OSC/FILT, AMP ENV, LFO, FOLLOWER, LEVEL, MAIN, PITCH, TRIM, SHAPE 1).
- **R1, the printed button is the section** (done). The FM-1's panel cannot be renamed, so the section is the button
  and pressing it again goes to the section's next page, from anywhere else to its first; ALGORITHM turns the same
  pages (it duplicated SELECT before), SELECT is the layer everywhere. The sections (UI_ICEBERG.md): SEQ (STEP,
  SEGMENT, LOOP CYCLE, SLOT SEQ), ENV (SOUND, SAMPLE: a sound source, not a global), LFO (LFO, DOT, EVOLVE: the LFO is
  a page of its own now, not a layer ENV steps over), FX (DISTORTION, DELAY, REVERB: the five layers in three pages,
  DELAY with SEND and CONFIG, REVERB with ROOM and RETURNS), ARP (CHORD first, since the arpeggiator is its modes,
  then BASS, then DRUMS), SEL (KEY / SCALE), GLO (MIX, GLOBAL). The SUB page is BASS, and so are MIX's fader and the
  keys that name it. No layer is hidden: CHORD's GRID and PATTERN are always there, their title says GRID OFF / PTN
  OFF when they are not in use, so SELECT's count is the same every time. OCT- / OCT+ stay what they were (the step
  bank on HOME and STEP, the keyboard's octave in KEYS): that is established on the panel and the proposal to make
  them the octave gave way to it.
- **R3, the black keys have fixed places** (done). On every page: B0 is ON for the page's part where there is one
  (BASS, CHORD, DRUMS, SAMPLE, EVOLVE, LOOP CYCLE, SLOT SEQ); B1-B4 are the page's own, in the order pick, toggles,
  actions; B5-B8 are the four performance keys, FILL, BASS MUTE, REVERSE and REPEAT, held, on every page (they were
  HOME's alone), drawn as one-letter cells in their own colours, F B < R, with the full name on the strip while held;
  B9 UNDO, B10 RNDMZR. The strip has room for about 13 letters of page labels beside these, so: HOME - + MUT RST
  CLR; SEGMENT EVEN A B C D (E and F by SEL + white key, R6); RANDOM NONE ALL CLR; DRUMS ON VOX (the next voice,
  in its colour) ACC ROLL, with FEED moved to a knob on VARY (where SWING was; SWING is GLOBAL's GROOVE); MIX LD BS
  CH DR. INIT (SOUND) and PRE-DIST (FX) moved from B0 to B1, since B0 is ON. MIX's fourth part is DRUMS, as the page.
- **R4 and R6, the white keys, SEL and EDIT** (done). A white key's tap toggles what sits on it and picks it: a step
  on or off (HOME, and now STEP too), a segment cut or joined (SEGMENT), a target (RANDOM), a cell (DOT), a bit
  (EVOLVE), a BASS SEQ step (BASS), the voice's hit (DRUMS). Where there is nothing to toggle it picks (LOOP, SLOT
  SEQ, SLOTS, CHORD, SAMPLE). **SEL held + a white key only picks**, on every page: a step without changing it, the
  segment the step is in (so E and F are reached again), a RANDOM target to EDIT. **EDIT is the page's edit mode**,
  one per page, with the EDIT LED lit while it is on: HOME, KEYS (the keyboard); STEP, the keys set the picked step's
  pitch (this was SEL + key); RANDOM, the amounts; EVOLVE, the lock; SAMPLE, per step. The strip says what the mode
  does ("EDIT: A KEY SETS THE PITCH"). Leaving a page ends its mode.

## The splash (2026-10-31)

The wordmark left the header for the key ("C MINOR"). It is now on a splash at boot: sixteen steps drawn as HOME
draws them, a phrase rising and falling in the four segments' colours, a line, SKURKYMELLO in the HTML's green (large),
and "FM-1" with the version under it. dinghy.c draws it first, before storage and the samples load, and keeps it up
for 1.5 s at least (not in host mode). tests/mello/ui_splash.c checks that it fits.

## The chord chain (2026-10-31)

ROADMAP: "HOLD in steps rather than triggers; chords that follow the segments; the sub on the chord's root".
- **NEXT** (MOTION's K4) decides how the chain moves on:
  - TRIG, as before: after HOLD triggers. A skipped step is no trigger, and a long step is one.
  - STEPS: after HOLD sixteenths on its own clock, whatever the sequence skips, holds or plays backwards. Turning
    STEPS on sets each chord's HOLD to 16 (a bar), since a count of triggers means nothing in sixteenths. HOLD now
    goes to 64 (four bars).
  - SEG: chord k plays for segment k (modulo the chain's length), in whatever order and with whatever repeats the
    segments play. HOLD shows SEG and does nothing.
  In STEPS and SEG, HOLD mode's chord sounds as soon as it is due (chord_chain_clock), not at the next step that
  plays. STAB, the arpeggios and PATTERN play the current chord at their own triggers, as before.
- **SUB ROOT** (CHORD's B1, lit when on) moves the sub by the chord's root, the nearest way (-5 .. +6 semitones):
  in C minor, VI takes it down 4, iv up 5 and v down 5. It applies to every sub note: the sub's MODE, BASS SEQ and
  DRONE. It follows the chord that sounds (or the chain's place before one has).
- **Storage.** SEC_CHORD 3 adds NEXT and SUB ROOT (2 bytes). TRIG and off are the defaults, so a version 2 file plays
  as before. tests/mello/chord_chain.c checks it.

## RANDOM: WAVE per step (2026-10-31)

Feedback: "with WAVE on, the randomizer should give a wave PER STEP if possible; OFF is not one of them". Each step
now has a wave (MelloStep.wave). 0 means the sound's WAVE, as before, and 1-5 are SAW, SQUARE, TRIANGLE, SINE and
PULSE.
- **The NOTE_ON carries it, and the lead plays it** in place of the sound's WAVE for that note. Unison, OSC 2, FM and
  the rest are as they were. PULSE uses the sound's PWM. The sound's WAVE OFF still silences the lead.
- **WAVE rolls the steps' waves, never the sound's.** Each step is rolled with WAVE's EDIT amount:
  - MED uses the old distribution (saw 35%, square 25%, triangle 20%, sine 12%, pulse 8%).
  - LOW moves a step one wave up or down from its own (or from the sound's).
  - HIGH picks any other wave.
  - MUSICAL gives each cell one wave, so the timbre repeats with the phrase.
  The sound's WAVE and PWM stay as they are.
- **MED's compatibility.** With WAVE in the mask, MED's RNG order differs from before (the wave is drawn in each step,
  not once at the end). The default mask has no WAVE, so a default roll is drawn exactly as before.
  random_wave_overview_smoke now checks the new rule.
- **It costs nothing.** The field sits in MelloStep's padding (still 16 bytes) and in the slot format's spare bits
  (extras 4-6), so neither RAM nor the snapshot grew.
- **On the page.** STEP's NOTE layer had ACC twice (FLAGS has it too). Its K4 is now WAVE: SND, SAW, SQR, TRI, SIN,
  PLS.

## BASS SEQ (2026-10-31)

Feedback: "a 16-step bass sequencer instead of 5-SEQ: length 1-16, per step tone, octave, glide and more; its own
clock, not stopped by the main sequence's skipped or held steps unless asked to follow them", and "more parameters
for the bass synth, if there are any under the hood".

- **The sequence.** 16 steps, LENGTH 1-16. Each step has ON, TONE (ROOT .. 7TH, OCT, in the key as 5-SEQ's were),
  OCT -2..+2, GLIDE and ACCENT. It has its own clock: a step every pulse, whatever the sequence skips, holds or
  REVERSE / REPEAT play. FOLLOW steps it with the sequence's steps instead, as 5-SEQ did. While it is on, it decides
  the sub's notes, and the sub's MODE (STEP, EUCLID, ..) waits. SUB MUTE silences it. It is global, as the drums are
  (SEC_BASS 10, 42 bytes), not per slot.
- **Glide and accent, as a 303's.** A gliding step's note is held past the next step's start. If the next step
  plays, that note slides there (GLIDE's time, or 60 ms at 0) without a new attack. A glide into a rest does nothing
  but hold. An accent is velocity 127 (others 114, the sub's): louder by up to half again (ACCENT), and it opens the
  filter by up to 2 octaves more. Outside BASS SEQ, the sub glides as the HTML's always did. The events carry
  ratchet_index 1 + the step, which tells mello_audio they are BASS SEQ's and tags each step apart.
- **The voice: a FILTER layer.** RES (Q 0.7 to ~17), ENV (up to 4 octaves over CUT, falling to -60 dB in E.DEC,
  20-2000 ms) and ACCENT. Once RES or ENV is above 0, the sub's filter becomes two state-variable low-passes, the
  second resonant, with the frequency worked out every 16 samples. At 0 it is the HTML's two biquads, exactly.
  Rendered alone, the default line with RES 65%, ENV 60% and E.DEC 250 ms has 21 dB more at 200 Hz-2 kHz than
  without.
- **The page.** SEQ (ON, LENGTH, FOLLOW, GATE), STEP (the picked step's TONE, OCT, GLIDE, ACCENT), FILTER. A white key
  toggles its step and picks it, as on DRUMS; B4 toggles the picked one. The steps are drawn as 16 gold columns:
  height for the tone, blue marks for an octave, accents white, glides drawn on to the next step, the playing step
  framed gold.
- **The default line**, for when it is turned on: root (accented) on 1, root on 4, an octave on 7 sliding down to the
  fifth on 8, root (accented) on 11, third on 14, seventh on 15 sliding home on 16.
- **RAM.** The first build of it did not link: .bss 288 bytes over the FM-1's 96 KiB. The audio and dinghy.c kept
  whole copies of the drums' and BASS SEQ's configs, patterns and all, four of each, where the audio needs only their
  sound. They now keep MelloAudioDrumKit and MelloAudioBassVoice, which saves about 1.1 KB. 832 bytes are left: the
  next feature that needs RAM needs a saving first. The engine (61 KB, most of it the slots) is where one is.
- **5-SEQ** stays in the engine and the snapshot (the HTML's, per slot). Turning SEQ on in a project whose 5-SEQ is on
  moves 5-SEQ's steps into BASS SEQ, FOLLOW on, so it plays as it did.

## MIX (2026-10-31)

Feedback: "we need a mixer now; K1-4 lead, sub bass, chords/arp, perc". GLO opens it first, then GLOBAL, then
SAMPLE. It has four faders in their parts' colours (LEAD green, SUB gold, CHORD violet, PERC coral), dimmed when the
part is off.
- K1 LEAD is new: the lead and its follower sub alone, before the filter (MelloSoundConfig.lead_level, 100% by
  default, ramped). It is saved with the sound in SEC_GRIT 4, so it belongs to the slot as the rest of the sound does.
- K2 SUB is the dedicated sub's LEVEL, K3 CHORD the chord voice's, K4 PERC the drums': each is the same value as on its
  own page.
- The name: CHORD, the same as the page. The arpeggio is the chord's PATTERN mode, so "chords/arp" is one part.
- Mutes (2026-10-31): B0-B3 mute LEAD, SUB, CHORD and PERC. A key is lit while its part plays, and a muted fader is
  dimmed and reads MUTE. The audio glides each part to 0 or back in ~10 ms, as SUB MUTE does, so a mute never
  clicks; delay and reverb tails ring on. Like the performance keys, mutes are not saved (MelloPerf.mix_mute).
  tests/mello/mix_mute.c checks them.
- SWING has a home on GLOBAL too: a third layer, GROOVE, has it on K1. It is the same value as DRUMS VARY's, since it
  swings everything.
- Saved in SEC_DRUMS (9).
- MIDI out and modulation (2026-10-31).
  - **MIDI out.** With NOTE OUT on in MIDI MASTER, as for the lead, each hit is a General MIDI note on channel 10:
    kick 36, snare 38, closed hat 42, GLITCH as a clap, 39. It carries the hit's velocity and is 50 ms long (or LEN
    FIXED's MS). Its OFF is always scheduled, so drum notes are not counted with the lead's and the sub's. STOP sends
    an OFF for the four notes. The channel is fixed at 10, as drum machines expect: GLOBAL MIDI has no room for a
    knob.
  - **Modulation.** Four destinations for DOT's MOD lanes and EVOLVE's sound routes, and the same four as LFO
    targets: D.TUNE (every voice's pitch, +-12 semitones), D.DECAY (up to 4x shorter or longer), D.CRUSH (up to 100
    on the CRUSH a hit gets) and D.DRIVE (the kit's DRIVE). TUNE, DECAY and CRUSH are read when a hit starts, and
    DRIVE all the time. They are summed with the LFO into MelloAudio.drum_mod (8 bytes), and only while the drums are
    on. tests/mello/drum_routes.c checks them.

## The chord voice (2026-10-30)

Feedback: the chords' harmony is good, their sound is not as good as the rest, and nothing moves on the page while a
pattern plays. The chord layer is not in the HTML, so this is the FM-1's own; it is built as a Juno's pad (one
oscillator a voice, one filter, a chorus), which sounds good and is cheap.

- Voices: six to play from (a chord's four and two more, so a new chord does not cut the last one's release); a voice
  of the same note is taken again from where it is, else a free one, else the quietest releasing, else the oldest.
- AMP: ATTACK, DECAY, SUSTAIN, RELEASE per voice (one-pole curves, an attack aimed at 1.3 as an analog one; worked out
  every 16 samples and ramped between).
- FILTER: a resonant 12 dB state-variable low-pass on the layer, CUT 60 Hz .. 6.4 kHz, RES up to just short of
  whistling, ENV up to +48 semitones from a filter envelope (MOTION's F ATK, F DEC) that every note restarts, LINK: HOME's
  CUTOFF moves it as far as the lead's. The old fixed 3.5 kHz low-pass is gone; the ~120 Hz high-pass stays (the bass is
  the sub's).
- TONE: WAVE; CHORUS (one 23 ms delay line read at 3.5 ms +- up to 1.8 ms, left and right swung against each other by a
  0.45 .. 0.85 Hz triangle, up to half wet): the layer's width; SPACE: its own send to the delay and reverb (100% = the
  lead's, as before); LEVEL.
- VLEAD (voice leading, off by default so the chords stay as written): a new chord in the inversion, an octave down,
  as written or up, whose voices move least from the last chord's, with a cost that grows with the square of its
  distance from where it is written, so a progression does not climb or sink.
- STRUM 0..60 ms: each voice of a chord after the one under it (HOLD, STAB), the whole note moved so its OFF never
  comes first.
- Layers: PLAY (MODE, TRIG, CHAIN, GATE), CHORD, VOICING (INV, SPREAD, VOICES, VLEAD), GRID only with TRIG GRID,
  PATTERN only with MODE PATTERN, TONE, FILTER, MOTION (F ATK, F DEC, STRUM), AMP.
- The page: the chain as chord names in the key ("Cm7", "G#M7", "Bo", "C+", "Fsus4", "E5"), the one sounding
  underlined; the voices of the chord sounding, lowest left, each lit while it sounds (the transport keeps each voice's
  NOTE_ON and NOTE_OFF due times; the engine puts the voice in the event's ratchet_index); PATTERN's columns with the
  one playing framed and its voices lit; AMP and MOTION as envelope graphs. HOME shows the chord sounding after its name.
- Defaults (a new project, or one saved before, whose chords keep their notes): CUT ~1 kHz opened to ~2.8 kHz by the
  envelope, RES 18%, ATTACK 40 ms, DECAY 900 ms, SUSTAIN 75%, RELEASE 900 ms, CHORUS 60%.
- Cost: the layer on adds about 28% to the audio render (the old one 13%) with a release overlapping the next chord;
  the audio interrupt was 27.9% of the chip (ROADMAP, fm1-emu), so about 4% more of it. Measured on the host; to be
  confirmed in fm1-emu.

## Segments made easy (2026-10-30)

The HTML sets segments with a count and absolute split steps, each split only from the segment it starts, and its
splits start at 8, 10, 12, 14, 15: a second segment is 8 + 8, a fourth 8 + 2 + 2 + 4. On the FM-1 that was two layers
(PLAY, STRUCT) with REPEAT and SKIP on both, and hard to follow. The FM-1 departs from it on the SEGMENT page only; the
project's fields, the engine's playing and the stored format are the HTML's.

- Where: the white keys are the steps (as on HOME and STEP, in the step bank). A key cuts: a segment starts at that step,
  the new half with the old one's settings, and is picked. Where one starts already, it joins the one before, which
  keeps its settings. Step 1 never cuts; a seventh segment is not made (the key picks the segment the step is in).
  The keys are lit where segments start, the playhead flips its key.
- Which: B0-B5 pick A-F (dim where there is none); B7 EVEN spreads the segments as near equal as they go.
- What, one layer: K1 REPEAT 1-4, K2 ORDER (the HTML's DIR and MODE in one: FWD, REV, P-P, RND; a P-P backwards from
  the HTML still plays and shows as P-P R), K3 SKIP, K4 LENGTH: the segment's end moves, the ones after keep their
  lengths as far as the sequence lets them, the last giving way; the last segment's LENGTH moves its start.
- LEN: a new sequence length scales the segments with it (16 to 8 steps: 4 x 4 becomes 4 x 2), where the HTML clamps
  them at the end.
- The picture: each segment's letter, and its order where it is not forwards (<, <>, ?), over its steps; a white cut
  where one starts; a skipped one struck through in red; under them a pill each with letter, length and x repeats.
- A new project has four segments of four, once forward each (the HTML has one of sixteen): it plays the same, and is
  ready to change. Projects saved before keep theirs.

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

## RANDOM: MUSICAL (2026-10-31)

Feedback: "a setting to roll musically instead of at random; I often press RANDOM many times and then tweak". INT (K1
on the RANDOM page) now goes LOW, MED, HIGH, MUSICAL. LOW, MED and HIGH are untouched (MED keeps its RNG order).
MUSICAL writes a phrase, not a roll:

- **Form.** The sequence is four cells in a form: A A' A B, A B A B', A A A B or A B A C (35/30/15/20%). At 16 and 32
  steps it is two half-length cells (A A' or A B) 40% of the time. Each cell is drawn once and repeated, so the loop
  has a motif and an answer. A length that is not 8, 12 .. 32 is one cell.
- **Rhythm.** In a cell, the first step and every fourth are strong (they play 85%, A's first step always), even steps
  medium (60%), odd steps weak (35%); a roll is sparse, as is or busy (-15 / 0 / +15%). Accents go mostly on strong
  steps, staccato (MULTI) and the occasional ratchet, ghost note (70%) and octave jump on weak ones. A note before a
  rest may be held, and a weak note after one may slide. Pulses stay 1 and nothing is skipped, so a bar stays a bar.
- **Pitch.** It walks by scale steps (mostly one, sometimes two, a repeat or a leap to a chord tone) around the root's
  octave. Strong steps land on a chord tone (root, third, fifth). A' and a middle B end on the fifth, and the last
  cell ends on the root, so the loop resolves where it starts. SUB goes mostly on roots.
- **Mask.** The mask and EDIT's amounts still decide what changes. PITCH alone keeps the rhythm, for example. Steps
  past the length repeat the phrase, so a longer sequence later still plays it. WAVE rolls as MED does. CHORD rolls a
  progression in the scale (i VI iv v, i iv v i, i VI III VII, i VII VI VII, i iv i v, I V vi IV), all triads or all
  sevenths, each chord as long as the first was.

INT is not saved, as before: it starts at MED after a power cycle. tests/mello/random_musical.c checks 300 rolls. On
average a MUSICAL melody moves 1.8 scale steps per note, against MED's 4.7.

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

## One body (2026-10-31)

C2's last rule, "one body grammar: the part's picture on top, its four values below as sliders, the section's
colour", applied to the parameter layers. The ENV panel had it (the envelope's curve, A D S R under it); every
layer drawn by draw_param_panel has it now: a 40-pixel picture at the top of the panel, drawn from the layer's
values with fills alone (draw_layer_picture), then the four knobs as rows of 22 pixels, label and value on the
line and the slider under. The pictures say what the knobs do: OSC/FILT the wave and the filter it goes into (the
cutoff's corner, the resonance's bump, the mode's shape), LFO the swing as deep as DEPTH, FOLLOWER the sub's wave
under the lead's, LEVEL master and lead as bars, GRIT the wave with its grains and unison's copy, OSC 2 over OSC 1
as mixed, CHARACTER the filter's mode and the saturation squaring the wave, MOD ENV the rise and fall (hanging down
for a negative amount), TRACKING the cutoff against the key, DISTORTION the curve a wave goes through, the delay's
echoes a TIME apart and FEEDBACK of each other, the reverb's tail after PRE as long as DECAY, the clock's sixteenths
with the beats lit (and the chip's measured audio load beside, B5), GROOVE's odd ticks late by SWING. The ENV
panel's "ENVELOPE" title went the way of the others (R2: the layer's name is in the header).

The same evening, the rest: every page draws its four knobs with draw_value_rows, one line each from y = 100
(the knob's label as the footer has it, its slider, its value; a list's value in a pill), 17 pixels apart, and the
page's own picture lives in the 66 pixels above: STEP's strip, title (with the step's sample), flags and pulses;
SEGMENT's letters, strip and pills; LOOP's eight cards; SLOT SEQ's and SLOTS' sixteen; KEY's keyboard alone (the
root, scale, octave and span were texts over it, now its rows); BASS SEQ's sixteen columns (LEN in the title);
the chord chain's names and notes, grid or masks; the drums' four rows; the sample's slots and trim. The
parameter layers' rows are the same rows, so their pictures grew to 58 pixels. Not rows: HOME (the sequence is the
page), MIX (its faders are its values), DOT and EVOLVE (the matrix and the register fill the body; their knobs are
in the footer), RANDOM (its targets and skyline). draw_knob_grid, the 2 x 2 of values, is gone.

## Room to run (2026-10-31)

The 0.5 feedback round left 800 bytes of RAM, a storage bank that silently stopped saving, and a CPU figure from
0.5-f3. Measured first, then made room (ROADMAP A1, B1-B5).

- **How the chip is measured.** fm1-emu runs the package (tools/emu_cpu.sh): 1.6 s of the splash and silence,
  then 5.6 s with PLAY at 1.6 s; the difference is four seconds of playing, in audio-interrupt instructions per
  second against the chip's 240 M. A -DMELLO_STRESS build plays mello_engine_stress at boot: every part on and busy
  at once (ratchets, unison 4 with OSC 2, FM, noise and SAT, the LFO on the filter, BASS SEQ with RES and ENV, four
  held chord voices with chorus, the drums on every step with rolls, ratchets, STUTTER and DRIVE, delay, reverb,
  distortion, both MOD lanes). 0.5-f19: the default pattern 32% of the chip, the worst case 75%, and 25% in
  silence (the early return that never fired, below); 0.5-f27: 32%, 71% and 2%. The emulator
  counts instructions and not cycles (its ISA notes say so: cycle counts unverified), so a divide is one
  instruction there and a widening multiply one: it cannot see what the next decision gains. The chip can:
  dinghy.c times every render block and GLOBAL shows the worst block of each second as CPU nn% (B5).
- **No divide in the per-sample path.** The toolchain emits a hardware divide for a divide by a constant too, and
  mello_audio_render had 233 of them, some 70 a sample (levels and sends in permille, modulation depths over
  32767). No core divides in a cycle. The levels' gains are Q12 now (4096 = 1: the ramps, LEVEL and SPACE of the
  drums and the chords, the fixed 0.58 / 0.84 / 0.672), the modulation depths Q15 (the LFO's, KEY TRACK, OSC 2's
  mix), each worked out once when the config changes (apply_pending) or at the note (lead_tracking: VEL > CUTOFF,
  VEL > MOD ENV); the LO CUT coefficient and OSC 2's pitch ratio are cached. mul_q12 truncates towards zero as a
  divide does: a floor held a -1 LSB in every tail for ever (audio_silence_regression). The parity table moved by
  0.1 dB at most; on the emulator the default pattern costs 2.5% more instructions, as expected (one became three).
- **The reverb keeps its own tail.** Its network ran whenever the delay's tail guard was armed, WET 0 or not; it
  runs while WET is up or its own tail (reverb_live, the decay time after the last input) is alive.
- **Silence costs nothing now.** The render's early return for silence compared the two event-lane arrays'
  addresses (`q_head == q_tail`), never equal, so the whole loop (filters, delay, reverb, distortion, limiter) ran
  through every silent block: 55 M instructions/s, 23% of the chip, with the sequencer stopped. Found by building
  a firmware without the render call (fm1-emu: 60 -> 5 M/s in silence). It returns early when every lane is empty,
  nothing sounds and no MOD lane is still moving; the mutes' gains settle on that path too.
- **Where the time goes** (make bench, the worst case on this computer, one part out at a time): drums 21%, the
  lead's grit and OSC 2 13%, the chords 13%, the sub with BASS SEQ 5%, the MOD lanes 3%, the LFO 2%. The reverb,
  delay and distortion cost little here. In the default pattern (a sampling profile): the reverb 21%, the ten
  biquads (main filter, the three tone filters) 16%, the per-sample control and mixing 21%.
- **The control step** (later the same day). The LFO and every offset the modulators put on the cutoff, the
  resonance, the lead's pitch and pulse width, the grit's amounts, OSC 2's pitch and FM are worked out every 4
  samples (11 kHz) and read ready-made by the samples between; the main filter's coefficients, recomputed on
  every change of cutoff, now change at most every 4 samples under the LFO. What moves a sample at a time stays per
  sample: the envelopes, the slides, the MOD lanes, the MOD envelope. The parity table is unchanged (the LFO
  stimuli within 0.1 dB). With it: the thirteen ramps and the mutes' gains are stepped only while one moves, the
  sample voices are called only when on, and the LFO runs on through silent blocks as the HTML's free-running
  one does. Not done: the key scan's LED pulses (the HAL's fm1_input_tick, ~12 M instructions/s while playing
  with LEDs lit, 2.5 in silence).
- **RAM** 800 B free -> 7456 B: the audio's event queue 128 -> 64 entries (16 a lane keep 300 BPM with 8 ratchets,
  chords, sub and samples in order), the chorus line 1024 -> 256 samples (it reads back 234 at most), dinghy.c's
  second copies of the configs (the audio's pending_* are the same thing) and its static default project gone. The
  table: the engine 60 KB (the 64 slots 49.7, the event queue 3.8 with 60-byte events, UNDO 1.2), the audio 16.3
  KB. The build fails under 2 KiB free (tools/build.py).
- **Storage: 64 KiB banks.** 64 full slots with their grit and MOD entries are 42.4 KiB; the banks were 40 and the
  save was refused with nothing on the screen. The flash map had 48 KiB unused between the banks and the sample
  staging area, so the banks are 64 KiB (0x98000..0xB7FFF). A project that still does not fit says FULL in red
  where SAVED stands. The old layout's second bank (0xA2000) lies across the new banks: boot still reads a record
  there (as bank 0's), the first save after an upgrade goes under it in bank 0 when it fits, and a save erases only
  the sectors its record needs (fewer erases, less wear), so the old record outlives it.

## Samples over MIDI, and the TRS jack (2026-11-01)

- **Samples go in as channel messages, not SysEx.** The transfer (mello_sample.c) uses CC 110-114 on channel 16 for
  START / END / ABORT / CLEAR / QUERY, CC 120 / 121 back as ACK / ERROR, and poly aftertouch for the data: two
  7-bit digits a message, eight digits to seven bytes. Dinghy's usb.c assembles SysEx only up to the six-byte soft
  keys and drops the rest, so SysEx would have meant changing a Dinghy file and a second parser; channel messages
  reach the app through the ring that already exists, from USB and (now) the jack alike, and an emulator's MIDI
  input carries them too. The cost: channel 16 must be free of other gear while a sample goes in. The payload is
  the flash record exactly as it will lie in the slot, so the device verifies it with the CRCs it checks at boot
  anyway, and writes it to the staging area first: a cut transfer leaves the old sample.
- **The host tool is plain Python.** tools/fm1_samples.py needs only mido (the installer's dependency) and the
  standard library: WAV in (8 to 32-bit, stereo mixed down), a box filter and linear interpolation down to
  22 050 Hz, the IMA encoder matching mello_sample_decode_block nibble for nibble, 16 messages then 1 ms of pause
  so the device's 256-byte page writes keep up with the 64-packet ring. A pluck of 2.3 s is 14 825 messages, 33 dB
  SNR back (4-bit ADPCM). The test feeds the tool's own record through the firmware's parser.
- **TRS MIDI IN as a cherry-pick from Dinghy 0.4, not a merge.** 0.4 is 2 000 lines: the SMK-37 board, blobs and
  slots, the loader's screen, store.h; none of it is wanted here, and dinghy.c is wholly ours. Taken: midi_uart.c
  as upstream has it, FELUCCA_UART 1 and its five hunks in platform.c (include, the 2 kHz poll the timer owes,
  the init before TIMER5, midi_get's 2 for cable 1). The app did not change: the transport already follows F8 /
  FA / FB / FC in SLAVE; notes in are not played (the sequencer is the player, as in the HTML). Proof: the parser
  on the host (midi_uart_parser), and the whole UART path in fm1-emu: its MIDI input is the jack (UART1 at
  31250 baud, `--keys 'T:midi:bf6e01'`), and a 2 976-byte sample record sent that way, 1 706 messages 2 ms
  apart, was erased into staging, written, copied into slot 1 and read back (FM1_FLASH_TRACE). The web emulator
  feeds MIDI the same way, so f28 is the first package to hear MIDI there. Before f28 the emulators' MIDI never
  reached SkurkyMello: it had only the USB ring. The pins (PH8) are proven on hardware by upstream's units
  (Felucca Salt, 0.4). RAM: +192 B (the 128-byte ring and the parser's state).
- **Room for samples.** Flash: slots 0xC0000..0xDFFFF (4 x 32 KiB), staging 0xB8000..0xBFFFF, the project banks
  0x98000..0xB7FFF; the OTA area 0xE0000..0xE4FFF and the settings sector 0xFC000 are Dinghy's. Pool (336 KiB):
  delay 128 KiB, reverb 48 KiB, the four slots' compressed data 128 KiB, canvases; 17 KiB free. RAM .data+.bss
  91040 of 98304. A fifth slot needs 32 KiB of pool (the delay's 1.49 s is the only thing that size) and flash
  that no map lists (0xE5000..0xFC000 is unlisted in Felucca's): not without a unit.

## Pictures in the manual (2026-11-01)

The README's tour and every row of the UI map show the screen as a picture. The pictures are not photographs, not
drawings and not the HTML's: the firmware itself draws them, in host mode, on a scripted tour of the panel
(`tools/screens.py`, `make screens`). The script presses the buttons, turns SELECT and holds the keys the way a
player does, waits for the layer readout to clear, and takes the screen; the PNGs (240 x 240 doubled, ~8 KB each,
77 of them) go to `docs/screens/`, and the map gets a `Screen` column keyed on its Page and Layer names. So a picture
is always of the code it sits next to, and a UI change regenerates them in the same commit as the map. For that the
host keeps the splash up for 1.5 s like the target does (it used to skip it), which `make host`'s first default shot
now shows. Pillow is not needed: the PNG writer is zlib and struct.

## Engineering process decisions

- GitHub repo state is the durable source of truth between chats.
- Read current branch HEAD and project docs before editing.
- Work in small verifiable packages.
- Do not claim completion without exact commit and CI evidence.
- Diagnostic probes may intentionally fail CI, but that state must be clearly documented and removed/disabled before declaring the real feature green.
- Perceptual audio acceptance needs listening criteria in addition to numerical DSP tests.
- Do not implement several approved future work packages at once merely because their product direction is now locked.
