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

## Big screen (2026-11-01)

The user asked for "big screen": FM-1 Mirror (Patrick F. Kellogg, github.com/pfkellogg/fm1-mirror) shows the FM-1's
1.54-inch screen large on a computer, tablet or phone, for anyone who cannot read it, if the firmware sends its
screen over the USB MIDI connection it already has. The protocol is his and MIT licensed, so the right thing was to
speak it exactly rather than invent another: the app sends `F0 7D 'F' 'S' 00 F7` about once a second (`01`: and
send everything), the FM-1 answers each painted row as `F0 7D 'F' 'S' 01 row col {count colour}... F7`, row and
column as two 7-bit bytes, each run a count of at most 127 and the RGB565 colour in three 7-bit bytes, a message at
most 70 bytes, and it stops 3 s after the last hello. His firmware-neutral encoder (`screen_mirror.c`, MIT, with a
section attribute and a `sm_pending()` added under the same licence) is in the tree as it is; the glue is ours:

- **Where the rows come from.** This firmware has no framebuffer (the UI draws dirty regions through a small
  canvas), so the rows are taken where the LCD gets them: `lcd_fill` and `lcd_blit` hand every write to
  `mirror_fill` and `mirror_blit_swapped`, which turn the byte-swapped canvas back into native colours a row at a
  time. With nobody asking the hooks return at once. This is a change to Dinghy's `lcd.c`, two lines, said so in
  the commit.
- **Where the hello comes from.** The USB's SysEx parser (usb.c, in the 2 kHz USB interrupt) sets a flag for the
  main loop, which `platform_poll` turns into the mirror's hello; the hello is not an updater frame. The rows go
  out through the updater's SysEx ring (`usb_mirror_send`, non-blocking: "busy" when the ring is short of room or
  the updater is sending), which the USB interrupt drains ahead of note output, as the updater's frames do.
- **Room, and pacing.** The queue is 16 KB in the pool (17 KB were free; RAM has 7 KB), and a page is more than
  that, so a repaint is paced: a full request paints one zone at a time (header, status, footer, body), each once
  the queue has drained, which the UI's own dirty flags make easy (`mirror_take_dirty` is OR-ed into them in the
  main loop). A row the queue cannot take waits up to 40 ms for the interrupt to take some; a row still lost asks
  for its zone again once the queue is empty. An app that stopped reading gets no waiting for a second. The
  wait is the one place the mirror can hold the main loop, and only with a viewer attached during a repaint
  that overflows 16 KB; the audio runs in its own interrupt.
- **The host.** tests/host.c has the same hooks and writes the SysEx to a file; `tests/mirror_check.sh` (in
  `make test`) decodes it back into a screen and compares with the screen shot at the same moment: 0 of 57600
  pixels differ after a hello, and after a page change and PLAY. `screen_mirror` tests the encoder and the pacing.
  The emulator has no USB MIDI, so the device itself and the time a full repaint takes are unmeasured until someone
  with an FM-1 and the app tries it; the message format is checked byte for byte against the reference's own test vectors.

## Record it yourself (2026-11-01)

The user: "we must be able to record steps into the main sequence, the bass and the chords by hand, with the full
keyboard: a record mode of its own. Free in chromatic, quantized within the set root and scale; in chromatic every
key is lit, in a key the keys that are not active go dark. It must be as good as it can be and break nothing in the
design." KEYS (EDIT on HOME) was already a chromatic keyboard with a step record for the sequence, so the mode grows
rather than a new page appearing:

- **The key rule, everywhere.** A note off the project's scale rounds to the nearest note on it, the smaller way,
  down on a tie; CHROMATIC leaves it as played. It is one function (`mello_scale_quantize`) and the keyboard shows
  it: the LEDs light the scale's keys in every octave and leave the others dark, and the strip draws the 27 keys as
  cells, lit or dark, the root in gold (red while recording), black keys short. Chords and the bass were never off
  the scale (they are degrees); the lead's steps are degrees too, so the rounding happens at the keys, before a
  note is written or sounded. Playing without REC a key is still the new root, any root.
- **REC cycles the target.** In KEYS, REC goes SEQ, BASS, CHORD, then off, and the header reads `REC > SEQ`,
  `REC > BASS` or `REC > CHORD` in red. One button, three parts, and the parts stay apart: the chords are their own
  thing and the bass has its own line, as the user wants it; nothing follows anything but the loop cycle. (The
  proposal said the target would follow the page you came from; KEYS is HOME's mode, so a cycle on REC is the
  honest version of that.)
- **Stopped: step record.** Each key writes the next step and moves on, from step 1, round the length. SEQ: the
  step's pitch and octave (a rest becomes a note). BASS: the step on, its tone (the scale's degree; in CHROMATIC
  through the minor scale's seven, since the sub's tones are diatonic) and its octave from the root. CHORD: the
  slot's degree and octave, and the chain is as long as you have played (a recording of three keys is three
  chords); a second key held over the first names the shape by its interval: a second is SUS2, a fourth SUS4, a
  fifth POWER, a seventh SEVENTH, a third the plain triad.
- **Playing: on the clock.** A key writes the step under the clock, or the next one once more than half a pulse
  has gone (the transport's next pulse deadline against now), for the sequence, the bass line (its own clock) and
  the chord sounding. The loop goes round, so you lay a line down pass by pass.
- **Rests, long keys, inversions (the same day).** Every key is a note here, so the rest is on SEL while REC is on
  (the proposal said a black key, which would have taken a note away): the sequence's step rests, the bass step goes
  off, and a chain of chords has no rests. Stopped, a key held 300 ms makes its step a held note (SEQ, gate HOLD) or
  a glide into the next (BASS). Playing, a key kept down carries on as the clock reaches each next step: the bass
  steps are tied by GLIDE, legato; the sequence's are the same note with gate HOLD and SLD, which the lead attacks
  again on each step, since a note in this sequencer cannot outlast its step without changing the step's
  PULSES and so the phrase's length. A chord's second key lower than the first is its inversion: a third under the
  root the first, a fifth the second, a seventh SEVENTH in the third. A third key's fifth over a third leaves the
  triad a triad.
- **One UNDO.** REC's first press takes the undo snapshot, so UNDO after a recording gives back the sequence, the
  bass and the chords as they were.
- **What did not change.** The black keys' places, OCT-/OCT+ moving the keyboard, the macros, saving. With REC off,
  KEYS is exactly what it was. Test: keys_record.

## Harmony as the spine (2026-11-01)

The user, after the progressions: "improve the chords much more deeply; they must talk with everything else, in all
directions, and it must stay as easy to make my own things. My favourite is MED or HIGH, randomize, complex loop
chains, long ambient melodic pieces that evolve for 10, 20, 30 minutes: we must not lose that." And: "the chords do
not quite follow the rest rhythmically, the ones." The chord machine already listened one way: the chain is resolved
in the loop cycle's key and scale, the sub and the BASS SEQ follow the chord's root (SUB ROOT), and the chords take
EVOLVE's and DOT's modulation (C.CUT, C.LEVEL). What it did not do was speak back to the melody, change over a long
run, or keep time with the bar. Three things, all on one new layer, CHORD > HARMONY, all off by default so nothing
saved sounds different:

- **FOLLOW** (OFF, BEATS, ALL). The lead's notes are moved to the nearest tone of the chord sounding, the smaller
  way: on BEATS only the notes on the beats (every fourth step), so the melody still walks between them and lands
  with the harmony; on ALL every note. It is applied where the note is resolved, so it holds for the sequence,
  for ratchets and holds and for the samples' pitch, and it reads the chord as voiced, so voice leading and
  inversions count. A chain of MED or HIGH chords under a MED or HIGH melody then sounds composed rather than
  coincident, and a hand-written melody over hand-written chords is untouched until you turn it on.
- **DRIFT** (0 .. 100%) and **DEPTH** (DEG, SHAPE, VOICE, ALL). Each time the chain comes round (its wrap, or the
  RESET that starts it over), with the chance DRIFT one chord changes within DEPTH: DEG swaps it for a degree of the
  same harmonic function (tonic i / VI / III, subdominant iv / ii / VI, dominant v / VII), so the progression keeps
  its sense; SHAPE gives it another shape (triad, seventh, sus2, sus4); VOICE another inversion, spread or octave;
  ALL one of the three. The first chord keeps its degree: home stays home while the rest wanders, which is what
  lets a progression run for half an hour and still be itself. DRIFT 30% on DEPTH VOICE is a slow shimmer; 70% on
  ALL is a progression that has become another one after ten minutes. UNDO does not follow DRIFT (it is the
  music moving, not an edit), and a save keeps the chain as it stands.
- **The ones.** A progression rolled over the chain's default (HOLD 2, by triggers, the arpeggio's) now runs on the
  clock chain (NEXT STEPS: HOLD counted in sixteenths), so a chord changes on the grid whatever the lead rests or
  holds, and the RESET that starts the drums and the bass over now also resets the clock chain's count, so the
  first chord falls on the one. The polymetric case the user liked, three chords over two bars, still works:
  a chain whose length does not meet the reset keeps its own round and realigns when it does.

**The default chain, the same day.** A chain made by hand started as the arpeggio's: HOLD 2 by triggers, so the
chords ran away from the bar unless you went to the GRID layer first. Now a fresh chain is four chords at HOLD 8 on
the clock chain: two chords a bar at 16 steps, on the one, whatever the lead rests. The arpeggio is still there
(MODE ARP, HOLD down to 2 if you want a chord a beat). Saved projects keep their chain; the progression roll still
treats a HOLD of 2 or less as "the old default" and sets 8. HOME's header reads `HOME > A` with the chord sounding
after the segment, in the chords' colour, instead of the chord in the segment's place.

MED and HIGH roll progressions too now, through the same walk MUSICAL uses (roll_progression), HIGH with more colour
and its voicings thrown wide: four random degrees with random holds was the one place the randomizer still threw
dice. LOW still nudges what is there. The pattern masks keep their own randomization. Stored in SEC_CHORD version
4; a version 3 record reads as FOLLOW OFF, DRIFT 0, DEPTH ALL. Test: chord_harmony.

## Progressions with a function (2026-11-01)

MUSICAL rolled the chord chain from six fixed progressions (i VI iv v, i iv v i, I V vi IV ...): the user, rightly,
"the most common pop progression ever made, a bit boring". Now the chain is walked, not looked up: three to eight
chords (four and eight most often; eight at HOLD 8 is a four-bar progression) through the three harmonic functions, tonic (degrees i, VI, III), subdominant (iv, ii,
VI) and dominant (v, VII, ii), with the transitions a harmony teacher would mark: tonic to subdominant most, to
dominant some; subdominant to dominant most; dominant home most of the time and once in six to the subdominant,
the deceptive cadence. The phrase starts on the tonic four times in five and ends there three times in four,
else open on the dominant; the tonic's substitutes (VI, III) take its place in the middle; no degree twice
running. The degrees are the scale's own, so a dorian roll has its major IV and a phrygian one its II, and the
colour comes from the scale rather than from a table. A roll draws how coloured it is: from plain triads to
sevenths, sus2 and sus4 with inversions. The holds are even, or long-short (12 and 4 of 16) two rolls in five,
and when the chain still stands on its default HOLD 2 (the arpeggio's) MUSICAL sets 8: two chords a bar at 16
steps, four over the 2 BAR reset. Settings the user has changed (a HOLD other than 2) are kept.

## A kick with weight (2026-11-01)

The user: "it all sounds very light and cheerful; I wish I had a much deeper, punchier EDM / techno kick; the drums
sound thin, light, bright; the music gets very arcade-game, and I wish it could sound like that sometimes but also
like a dark techno beat when wanted. Go over it deeply and adjust, without taking anything out." What was thin,
measured on one default kick hit rendered alone (drum_weight): peak 3411 of 32767, above -20 dB for 70 ms, and 147
of energy at the fundamental and its first two harmonics. The kick was a clean sine at 57 Hz with a short 1.7x
sweep and a 258 ms tail; a clean 57 Hz sine is what a small speaker cannot carry, and the sweep was over before
the ear heard it as a punch. Changed, without removing anything:

- **The kick.** The fundamental 38 .. 90 Hz (was 35 .. 90), its pitch falling from 3.5x over 90 ms (was 1.7x over
  60) under the 7x click, the sine always driven from 22% so the harmonics a speaker carries are there, TONE adding
  drive on top (it used to add all of it), the tail up to 1.5 s (was 1.2), the level held up to 14 ms. Default DECAY
  180 and TONE 25% (were 100 and 10%): 340 ms, driven. Now: peak 3901, 120 ms above -20 dB, low end 254.
- **The snare.** The body lasts half the hit (was a quarter) at twice its weight, and the noise's band-pass starts
  at 600 Hz (was 900) so a low TUNE gives a darker crack.
- **The kit.** LEVEL 80% by default (was 70): it sits with the lead, not behind it.
- **DUCK**, new on the DIST layer's third knob: the kick's sidechain. At each kick the lead, the sub and the chords
  dip by DUCK and come back over ~160 ms. 0 by default, so nothing saved sounds different; 40 .. 60% is the pump of
  a techno track. Stored in SEC_DRUMS version 2; a version 1 record reads as 0.

Dark versus cheerful is then a matter of settings, not of what the instrument can do: a long driven kick with DUCK
on, a low CUTOFF with SAT, a saw rather than a square, HOLD chords, a big dark reverb and 120 .. 130 BPM is the
one; a bright square lead over a short kick at 80 BPM the other. The sound examples of 2026-11-01 give both.

## The chords move too (2026-11-01)

The user: "the chord sound is very static; EVOLVE must be routable to it too, and preferably inverted from what
goes to the ordinary synth." The chord voice had its own filter and envelopes but nothing modulated them: the
modulation destinations (DOT's MOD lanes, EVOLVE's DEST A and B) were the lead's parameters and the drums'. Two
more: C.CUT, the chords' filter cutoff on CUT's own range, and C.LEVEL, their level from nothing to twice. The depth
of a route was already signed (-100% .. +100%), so the inversion asked for is a route with a negative depth: EVOLVE
A on CUTOFF at +50% and B on C.CUT at -50% opens the lead as it closes the chords, and the two parts answer each
other instead of standing still together. The audio keeps the chords' share of the modulation sum per block
(chord_mod_cut, chord_mod_level) and reads it in the filter coefficients and the mix. Stored routes with ids beyond
an older firmware's list fall back to OFF there (the sanitizer). Test: chord_routes.

## RESET resets everything (2026-11-01)

RESET (STEP > SEQ, 1 to 8 BAR, or FREE) used to reset the sequence's segment traversal alone: the drums, the BASS
SEQ line and the chord chain kept their own clocks, so a 12-step bass under a 16-step lead drifted against it
until their lengths met. The user: "it does not sound as if everything resets to 1 when the sequence has run its
course; that should always happen unless something is FREE." Agreed, and that is the rule now: at the reset the
drums, the bass line and the chord chain (its position, its hold count and the grid's tick) start over with the
sequence; FREE leaves every part on its own clock. The parts still have their own clocks between resets (a 12-step
drum pattern still turns over inside the two bars), which is what gives polymetric patterns their life, but they
all meet on the one. The transport's own start resets the same things, so a reset is a start. Test: reset_all_parts.

One exception, the same day: the chord chain starts over only at the resets that meet its own length (or when it
is no longer than the reset). A progression of eight chords at HOLD 8 is four bars; under a 2 BAR reset it used
to be cut off after the fourth chord every time ("the same progression all the way"), so now it starts over every
second reset, and a three-chord chain over two bars realigns every third.

## A bass line that fits (2026-11-01)

The user wanted to roll the bass as the drums roll: "not MUSICAL as such, but weighted towards something usable;
the bass is often an octave too high and very sporadic under the ordinary randomizer; with a melody in the main
sequence, a rolled bass line should fit it, more or less." So BASS gets ROLL on B2, beside ON and STEP, and
`mello_engine_bass_roll` writes a BASS SEQ line the way a bass player would under the sequence as it is:

- **The beats** (every fourth step) first: the one is the root, accented; the others a root or a fifth, or, where
  the lead has a chord tone on that step (root, third, fifth or seventh of the scale), the lead's degree itself, so
  the line moves with the melody.
- **The offbeats** are walks into the next beat (the seventh, second or fifth under a root; the fourth, sixth or
  root under a fifth), an octave jump before a root, the lead's note where it is a chord tone, or the beat's note
  again as a pulse; the "and" more often than the "e".
- **Never sporadic**: five to eleven notes of sixteen, the ands filled to five, the "e"s dropped past eleven. Three
  densities, as MUSICAL's.
- **An octave down**: every step at octave -1, the jumps at 0 (the sub's own OCT, -1 by default, still applies).
- **Glides** where a note runs into a different next note, three in ten; accents on the one, often on the three,
  and a few more.

ROLL turns BASS SEQ on, so the line is heard at once, and UNDO takes it back: the undo snapshot now keeps the bass
with the project, the sound, the chords and the drums. The test (bass_roll) checks 200 rolls for all of it.

## A melody, not a walk (2026-11-01)

MUSICAL (2026-10-31) rolled a phrase in a form (A A' A B ...) but walked each cell by random steps from a chord tone
and bounced off a range of one octave and a bit, and A' was a copy of A. The user: it plays safe, often repetitive,
within a few notes, within one octave. What the tunes people remember do instead, and what v2 does:

- **A contour.** The phrase has a shape: an ARCH that climbs to its climax in the answer and comes home, a RISE
  that keeps climbing, a WAVE that dips first, a FALL that opens high and settles. The contour gives every cell a
  register in scale degrees from home (B four to six up, C down), and the climax is the third or fourth cell, never
  the first.
- **A line towards a target.** In a cell the notes head for the next cell's register four times in five, mostly by
  step, a quarter of the time by a skip of a third, and now and then by a leap of a fourth to a sixth onto a chord
  tone, which the next note answers with a step back the other way (the oldest rule of melody). Never the same note
  three times, never straight back to the note before the last.
- **Development, not copies.** A' and B' are A and B moved along the contour: the same rhythm, flags and samples,
  the pitches shifted by scale degrees (a sequence), strong steps snapped back to chord tones.
- **The range.** The walk may use the whole pitch span, and a line pushed past it takes the octave field up rather
  than bouncing back; the climax cell starts an octave up in almost half the rolls.
- **What stays.** Strong steps on chord tones, the first step playing, the phrase ending on the root at octave 0,
  A back at step 9 in the four-cell forms, pulses 1 and nothing skipped, the mask and EDIT's amounts.

Measured over 300 rolls of 16 steps (random_musical): a phrase spanned 4.9 scale steps and used 3.8 distinct notes
and reached the octave above in 41 rolls; now 7.1, 4.8 and 135, while strong steps still play 95%, weak 40%, and the
melody moves 2.0 scale steps a note against MED's 4.65. The test holds the floor at a span of 7 and 4.5 notes.

**v2.1, the same day.** The user heard the sound examples as "a looped 8-step": with A A' A B, A B A B' and the
two halves A A', the first cell came back word for word at step 9 in most rolls and A' was a transposition, so
the two halves of the bar sounded alike. Now the forms are A A' B C, A B A' C, A B C A' (A late or not at all) and,
less often, A A' A B and A B A B'; two halves are always A B. A' and B' are developments in one of three ways: a
sequence (the line moved), an inversion (mirrored about its first note), or a displacement (the rhythm a step
later), so the answer is recognisably A and still not A again; and an answer cell may anticipate its middle beat
(a syncopation) when the step before it rests. Over 300 rolls A is back at step 9 in 75, the second half never
repeats the first, the span is 8.3 and the distinct notes 5.4; the test keeps the return between 30 and 150 and
the repeat at most 15. The bar itself still loops every 16 steps, as a bar does: a longer melody is LENGTH 32,
where MUSICAL writes four cells of eight.

**v2.2, the same day: "not complex enough".** What was plain was the rhythm: every note a sixteenth, rests by
chance, no figure to hold on to. Now a cell's rhythm is a motif from a library (bit a step): for cells of four
`x.xx`, `xx.x`, `x..x`, `xxxx`, `x.x.`, `xx..` for the motif and `.xxx`, `.x.x`, `.xx.`, `xxx.` among the
answer's; for cells of eight the 3+3+2 figures `x..x..x.`, `x.xx.xx.`, `xx.xx.x.`, `x.x.x.xx` and their offbeat
answers; sixteen is two eights. A weak step flips one time in ten, a busy roll adds a hit and a sparse one takes one
away, so the motif is a family, not a stamp. On top: a cross-rhythm of accents every third step in a third of the
cells, ratchets on weak steps twice as often, slides into weak steps four times in ten, chance notes on a quarter
of the weak and medium steps (60% and 75%) so the loop differs from round to round, and one leap in five an
octave. Measured: span 8.6, distinct notes 5.3, the octave above in 174 rolls, strong steps 77%, weak 53% (the
floor is now 65% weak), motion 2.2 against MED's 4.65. Cells of odd length (12 steps as three) keep the chances.

## Black ground (2026-11-01)

The user found the screen busy: the HTML's five green tones (ground #040E08, two panels, lines, dim and mid text)
sat under everything, so the frame competed with the sequence. The new rule: the ground is black (#000000), the
frame is grey, and a colour appears only where it means something. Labels #6E7470, standing figures and the status
line #8E948F, values #E1FFEE or the knob's colour; a card #101010 only where the content needs one; one hairline
#262626 between the zones (under the header at 29, over the strip at 171, over the footer at 188 with a hairline
between its columns, over the status at 220) instead of filled bands. The zones keep their geometry, so no drawing
routine moves; the body stops at row 170 and the footer's text rows are 189 and 205. The header reads the page in
white and the layer in the page's colour (HOME: `HOME > A` with the segment in its colour); the tempo is green only
while playing. The functional colours and the segment colours are untouched. HOME first: segment lines under the
letters, bars in their segment's full colour (the playing one white-capped and white-topped, the picked one framed),
the keys' strip as quiet cells with the label in its colour. Then every other page: the cards went (ui_section and
the parameter panels draw the title and the picture on the ground), the sliders' tracks are a grey hairline and
their thumbs take the value's colour (they were all green). The four value rows stay under each picture: the mockups
dropped them for the footer's numbers, but on the layers whose body is nothing but those values (GLOBAL MIDI, the
sample's trim) the body would be empty, and a slider is the one picture a value has. Mockups: the design canvas of
2026-11-01; the result is the manual's screens.

## HOME's knobs are yours (2026-11-01)

The HTML's four macros (CUTOFF, DECAY, SPACE, DRIVE) are HOME's K1-K4 by default, and they stay the default. But a
front screen is where you play, and what you want under your hand differs by set: the user asked to choose. EDIT
tapped on HOME is KEYS already, so EDIT held half a second is MACROS: each knob then picks, round a list, what it
will turn, and a tap of EDIT leaves (EDIT's tap waits for the release now, like SAVE's). The list is the live
parameters that make sense in a performance and are not already somewhere on the panel: nothing the black keys do
(transpose, MUTATE, RESTART, CLEAR, FILL, BASS MUTE, REVERSE, REPEAT, UNDO, RNDMZR), nothing the encoders do (tempo,
intensity, the slot). Picking turns nothing; the LFO comes on when its macro is first turned, since a silent knob
is worse than a switch flipped. The choice is the project's, in its own storage section (SEC_MACROS) so an older
firmware skips it and a record without it gives the four, and not a slot's: how you play, not what. The map's text
rows are kept by hand with the code, as every row is (the planned extractor was never written; the
mapping tests pin the rows instead), and `make screens` supplies the pictures.

## The status line counts beats, not steps (2026-11-01)

While playing, the status line used to read the sequence's step of its length (`15/16`). The user's point: a step can
take several clock pulses (PULSE), so the step number says nothing about where in the phrase you are, and what matters
is the place in the length before RESET. Now, with RESET set, it counts the beats of that length in clock pulses
(`21/32` for 2 BAR: the engine's pulse count modulo its loop length, the thing the reset itself is timed on), and the
step is left to the bright bar on HOME and the cursor on STEP. Past 99 beats (8 BAR) the field is six characters, so
it reads bar.beat/bars (`7.13/8`). With RESET on FREE there is no phrase to count, and the step of the length stays.
Stopped, the length in steps (`16 ST`) as before. The HTML has no master reset and no such counter: a firmware
decision.

## Pictures in the manual (2026-11-01)

The README's tour and every row of the UI map show the screen as a picture. The pictures are not photographs, not
drawings and not the HTML's: the firmware itself draws them, in host mode, on a scripted tour of the panel
(`tools/screens.py`, `make screens`). The script presses the buttons, turns SELECT and holds the keys the way a
player does, waits for the layer readout to clear, and takes the screen; the PNGs (240 x 240 doubled, ~8 KB each,
77 of them) go to `docs/screens/`, and the map gets a `Screen` column keyed on its Page and Layer names. So a picture
is always of the code it sits next to, and a UI change regenerates them in the same commit as the map. For that the
host keeps the splash up for 1.5 s like the target does (it used to skip it), which `make host`'s first default shot
now shows. Pillow is not needed: the PNG writer is zlib and struct.

## One key that always holds (2026-11-02)

The product owner, playing LOOP CYCLE: ROOT+ and TRANS did the same, a changed root showed nowhere, and the bass
could not be transposed within the key. Both added semitones to the finished note, so they were the same; TRANS and
BASS TR left the key; the bass added two scale offsets as semitones and could leave the scale. Decided: "always
rounded into the key; the global key and root lead, unless CHROMATIC or a loop step that is not FOLLOW."
- **The key now** (`mello_engine_key_now`): the KEY page's root and scale, the loop step's KEY and SCALE where they
  are not FOLLOW, ROOT+ moving the tonic. The status line's key, KEYS' rounding, its strip and the keys' lights read
  it; what KEYS records is the same place in the project's key, which the loop step moves as it moves the rest.
- **ROOT+ moves the key, TRANS the line within it.** TRANS and BASS TR stay in semitones, and the note they land on is
  rounded into the key now in their direction (+1 in C minor: C to D, Eb to F; -1: D to C), so no two neighbouring
  notes collapse into one and a line keeps its shape. The bass's summed tones and the chord's root shift take the
  nearest tone of the key. CHROMATIC: semitones, unrounded. The chord's POWER fifth stays a fifth.
- Test: key_system (exact notes, the key now, and a sweep of every scale, root and loop setting: about a million
  lead, bass and chord notes, none off the key; without the rounding 43% were).

## The bass an octave lower (2026-11-02)

"Too bright, even at OCT -1." OCT -1 put the bass one octave under the root (C3, 131 Hz at C4). The bass voice
(BASS's modes, BASS SEQ, DRONE) now sounds an octave lower with the same OCT values: OCT -1 is C2, 65 Hz. The floor
stays at MIDI 12. Projects saved earlier drop an octave too, which is the point. The follower (the lead's
sub-oscillator, with its own OCT) stays where it was.

## EVOLVE's GATE: LENGTH and RETRIG (2026-11-03)

The HTML's EVOLVE GATE route, on by default at 35%, sets a step's gate to MULTI (a new attack on every pulse) when
the register's bit is lit and to SINGLE when it is not, at DEPTH's chance. On steps of several pulses that is heard as
ratchets, and it also played steps that were rests or failed their probability. The product owner heard GATE as the
note's length. A departure from the HTML, accepted to try (ROADMAP E7):

- **LENGTH** (the old GATE bit, so a saved GATE ON plays LENGTH; on by default at 35%): the register's value shortens
  each note, at DEPTH 100% down to a quarter of its length, at 35% to about three quarters. It never adds an attack,
  never plays a rest and never silences a note. HOLD steps are ties and keep their length.
- **RETRIG** (a new route bit, off by default): the HTML's re-gate without its other half, a lit bit makes a playing
  step MULTI at DEPTH's chance; an unlit bit leaves the step as it is, and a rest stays a rest.
- On the screen, ROUTES' K1 is GATE: OFF, LENGTH, RETRIG or BOTH; DEPTH's GATE sets both.
- The sub's TM mode reads the register's bit whatever the routes (it read it only with GATE on).

Test: tests/mello/evolve_gate.c (the same notes as EVOLVE off, none longer, none under a quarter; RETRIG adds
attacks and plays no rest).

## The chords behind the lead (2026-11-03)

The chords' defaults were loud and open: LEVEL 32%, CUT ~1 kHz opened to ~2.8 kHz by ENV 38% at every chord, SPACE
100%. Rendered with the default project, the default chords were 3.8 dB over the lead. New defaults (ROADMAP E4):
LEVEL 18%, CUT ~800 Hz, ENV 22%, SPACE 60%; CHORUS, RES and the envelopes as they were. Measured the same way, the
chords now sit 2.0 dB under the lead, with 6.3% of their energy above 1 kHz (11.7% before). A held chord against a
short lead reads louder than its RMS, which is why LEVEL went below the proposed 23%. Projects saved earlier keep
their own values (but a save from the chords' first format, which held no voice, gets these); a new project gets
them.

## B0 is ON, and a part's first page plays it (2026-11-03)

CONTROL_GRAMMAR's G1 and G2, the first of the rules G1-G8 (ROADMAP "Next: everything open"). G1: on every page with a
part or a lane B0 is its ON. DOT had none (B0 was empty): B0 now switches the picked matrix, saved in bit 6 of its
mode byte (older builds read the mode as HORIZ, as with REV in bit 7). An OFF matrix keeps its route moving, so it
comes back in place, and sends nothing; the page draws it dimmed. G2: BASS's first page turns BASS SEQ on and off
on B1 (SEQ, lit when on), in place of STEP, which only repeated a white key. The strip's plain cells (pages too full
for frames) lost the pixel each kept beside its label and got a hairline between cells of a group instead, so DOT's
eleven keys fit. Tests: control_authoritative_mapping (verify_g1_g2), dot_orders (OFF sends nothing, the route
moves, OFF through a power cycle).

## SUB MELLO, or the four voices (2026-11-03)

Decided 2026-11-02 (ROADMAP E6, F6): SUB MELLO in place of the chord synth, the synth's code kept. Built that way
(0.5-f48, the synth behind a build flag), then decided again the same day: both, one per project, so a project
chooses its chord sound and the two always fit, since both read the same chain. The chord part's harmony (the chain,
its progressions, MOTION's NEXT, HARMONY's FOLLOW and DRIFT, B1 BASS on the chord's root, the routes to C.CUT and
C.LEVEL) is the same for both. B2 POLY on the CHORD page picks the sound, saved with the project (SEC_CHORD version
5; a project saved before it plays POLY, as it did):

- **SUB MELLO** (POLY off, a new project's): one voice, and one more for the last note's release, through the same
  filter, envelopes, high-pass and SPACE, with no chorus; a note as loud as one voice of a triad, so LEVEL means the
  same for both (default project, default part: 2.9 dB under the lead; the four voices were 2.0). The engine plays
  the chord's tones one at a time: BROKEN (low, high, middle, high of the voiced chord on every pulse; the default
  mode) and WALK (on every fourth note the chord tone nearest a step on, a scale step between, turning at the
  chord's ends), new; the arpeggios as they were; STAB the top tone, HOLD the top tone held (VLEAD moves it
  least), a PATTERN step its lowest tone. CHORUS and STRUM are blank on the screen and their knobs inert.
- **POLY**: the four voices with chorus and STRUM as before 0.5-f48. BROKEN and WALK play single tones here too.

Measured in fm1-emu (mello_engine_stress, instruction counts): the worst case without the chord part 64.7% of the
chip, with SUB MELLO 67.9% (3.2 points), with POLY 72.6% (7.9 points); the worst case stays POLY's. The default
pattern 32.2% (the part is off by default). RAM as before 0.5-f48 (both voices are built). Tests: sub_mello (one
note at a time in every mode, BROKEN's figure, WALK's beats and steps, every note in the key, two voices at most,
POLY's STAB together); storage_chord_regression (POLY through a power cycle); the four-voice tests set POLY.

## A send per part (2026-11-03)

ROADMAP E11 / F8. Before: the lead, the bass and the samples were one bus through the lead's filter (HOME's CUTOFF)
and distortion, sent to the delay and the reverb with the FX pages' WET; the chords and the drums each had one SPACE
to both. The bass could not be kept out of the reverb, and the lead's CUTOFF swept it. Now:

- MIX has three layers on SELECT: LEVEL (as before), DELAY and REVERB, each part's send (LEAD, BASS, CHORD, DRUMS;
  the samples go with the lead). The FX pages' WET stays, scaling every send of its effect; the chords send 800 /
  1000 of their share, as their SPACE did. SPACE left the CHORD (TONE) and DRUMS (KIT) pages: one place for sends.
- The bass leaves the lead's filter and distortion (a departure from the HTML, where Tone's chain filtered both):
  it has its own filter and DRIVE since BASS's FILTER layer. It enters the mix at the level the lead's path gave it at
  the defaults (measured: within 0.01 dB), still under the LFO's AMP and the kick's DUCK.
- A project saved before (FX section version 3 or older) gets the sends it played with: the lead and the bass all
  of theirs, the chords and the drums their SPACE. A new project: the lead 100%, the bass no delay and 20% reverb,
  the chords 60%, the drums 25% (their SPACE defaults).

Tests: fx_sends (the defaults; a part at 0 sounds as with the effects off, at 100% not; the lead's CUTOFF leaves the
bass alone; MIX's layers set the sends; saved). SEC_FX version 4.

The delay's reach (F9, F10): DIV gains 32N, 16D, 2T, 2D and 1 BAR (stored as 9..13, so the older numbers keep their
meaning; an older build reads them as 8N). The line holds 1.49 s; a time past it runs the line at half rate (every
second sample written, two averaged; the read point in half slots), so it reaches 2.97 s and 1 BAR fits from 81 BPM;
HI CUT already takes what half rate loses. A division past even that plays the longest the line holds and shows a
"~" after its name. LO CUT is on at 120 Hz on a new project (it was OFF, and hard to find); its one-poles keep their
state in Q8 with the output rounded, since in Q0 a few LSB stayed in the feedback for ever and the unit never
counted as silent (found when it became the default: audio_silence_regression). make parity: hot_fx loses its
delay's low end under 120 Hz (the HTML has no LO CUT; audio_html_parity sets it off), sub_dry its share of the
distortion's tone filter. Test: delay_reach (the echoes of 2N, 2D and 1 BAR on time, 1 BAR at 60 BPM at the reach).
A ping-pong option needs two read points and two filters' states: it is its own item (F25).

## Performance on every part, in one place (2026-11-03)

CONTROL_GRAMMAR's G4 (ROADMAP E2, F3, F4, F18). Before: B5-B8 were FILL, BASS MUTE, REVERSE and REPEAT, held, on every
page, acting on the lead's sequence only (the drums, BASS SEQ and the chord chain ran on their own clocks); MUTATE was
on HOME alone.

- **B5 PERF** opens PERFORM: tapped (let go within 0.4 s), until PERF again or a page button; held, while held. Its
  white keys, a 4 x 4 on the screen in the keys' order: FILL, REVERSE, REP 8 / 4 / 2 / 1 (pulses) while held;
  RESTART, MUTATE, TRANS - / + on the press; MUTE LD / BS / CH / DR latched (MIX's mutes). Keys 15-16 free.
- **B6 MUTATE** on every page; B7-B8 free for the pages; HOME's own keys lose MUT (- + RST CLR stay where they were).
  Where the strip is too full for frames, PERF and MUT show as P and M.
- **Every part**: the history keeps, with each of the sequence's pulses, the drums' and BASS SEQ's step and the chord
  chain's chord, arpeggio and pattern places; REVERSE and REPEAT replay them all, each part's own place moving on
  underneath, so letting go lands every part in time. FILL: a snare and glitch roll on every drum step (two hits at
  least, chance aside), every step of the bass line, the chords struck on every step (HOLD's again). A CHAIN TRIG
  chain moves by the triggers REVERSE / REPEAT replay.
- Holding an action no longer clears MIX's mutes (perf_poll built the state from nothing and set it whole).

Tests: perf_parts (REPEAT 4 loops the drums', the bass's and the chords' last four; REVERSE plays the drums backwards;
let go, every part in time; FILL), ui_perf_keys (PERF tapped and held, the white keys, the mutes kept, a page button
closes it, B6 MUT off HOME), the contracts' strips.

## REC where you are (2026-11-03)

CONTROL_GRAMMAR's G3 (ROADMAP F5). Before: recording was HOME's EDIT > REC only, REC cycling SEQ, BASS, CHORD; the
drums could not be recorded; a key played while recording on the clock was not heard, and stopped every key sounded
as the lead.

- REC on STEP, BASS, CHORD and DRUMS records that page's part, from its picked step, with HOME's KEYS > REC's own
  writing (the key rule, a held key's tie or glide, a second key's chord shape, SEL a rest, one UNDO). REC again,
  another page or PLAY's stop ends the take. The strip says what the keys do, in red; REC's LED is lit.
- DRUMS: white keys 1-4 a hit of KICK, SNARE, HAT, GLITCH where the cursor is (stopped; a key held with another, the
  same step) or the clock is (playing); 5-8 the last hit's ACCENT, RATCH (1-4 round), PITCH down, up.
- Heard: stopped, every key, in the part's own voice (the bass's, the chords', a drum's); playing and recording, a
  key that landed on the step already sounding (the clock had passed it, so the sequence will not play it this
  round). One that rounds to the next step is not, since the sequence plays it there in a moment.

Test: rec_pages (each page's take, its sound, UNDO, the ends of a take; the drums' voices, same-step keys and locks).

## K1-K4 the same on every voice (2026-11-03)

CONTROL_GRAMMAR's G6 (ROADMAP E12, F11). CUT was K3 on the lead's OSC/FILT, K2 on BASS's TONE (RES on FILTER), K1 on
CHORD's FILTER, and the drums' TONE K3. Now each voice's first sound layer is K1 CUT, K2 RES, K3 DECAY or ENV, K4
LEVEL, and the rest moves to the next:

- the lead: FILTER (was OSC/FILT) CUT RES DECAY LEVEL (the lead's level, MIX's LEAD); OSC (was LEVEL, now second on
  SELECT) WAVE PWM SLIDE MASTER;
- the bass: FILTER CUT RES ENV LEVEL; VOICE WAVE DECAY RELEASE DRIVE; TONE GLIDE OCT F.DEC ACCENT;
- the chords: FILTER CUT RES ENV LEVEL; TONE WAVE CHORUS LINK;
- the drums' voice: TONE (their cut) TUNE DECAY LEVEL; the sampler keeps LEVEL first (no filter).

Done as a table in front of the pages' own code (KNOB_MAPS in mello_ui.c): each knob of a mapped layer names the
layer and knob where the parameter's code is, and the footer, the knobs, the readout and the colours all go through
it. The layers' pictures are still drawn by their own index. Test: rotary_mapping_parity (labels and handlers),
ui_readout.

## The screen in its new order (2026-11-03)

CONTROL_GRAMMAR's G7 (ROADMAP E3, F12), as chosen from the sketches (B, and the readout at the bottom):

- One line on top: `PAGE > LAYER` where it fits in 16 characters, else the layer alone (the printed button left the
  line: the lit panel button says it); on the right the key sounding in short (`CMIN`, `C#DOR`) and the count. The
  tempo left the header (SELECT's readout on HOME, GLOBAL's CLOCK); NOSAVE or FULL show in red in the key's place.
  The status line is gone; the count it carried redraws the header.
- The picture 18 rows taller, K1-K4 under it, the black keys' strip at the bottom, nearest the keys.
- The readout (a knob's name, value and slider, 1.2 s) covers K1-K4 and the strip, not the body: name and value
  large on one line, else the name small beside a large value, else both small.

Done in the draw layer every page goes through (fit_fill / fit_text, FIT_ROWS): the pages still draw in their old
rows, and each zone is moved to its new place, the body stretched (fills grow; text keeps its size and moves). So
every page has its new height without its own code changing; the readout and the splash draw in the screen's own
rows. Two truncations it showed were fixed (a SEGMENT pill with 'A13x4', a cutoff of four digits in Hz now whole).
Tests moved to the new rows (the strip's, K1-K4's, the header's count); Big screen's mirror check is pixel-exact.

## The lights by one rule (2026-11-03)

CONTROL_GRAMMAR's G5 (ROADMAP E8, F13). Every page used on and off only and marked the playhead by flipping its key,
so it went dark on a step that plays and lit on one that does not; the black keys were never lit. Now three levels,
on every page and on both kinds of key: lit = now, active or held (the playhead, the step or cell in focus or held,
the read position, a switch that is on, PERF open, the root in KEYS); glow = set or there (a step that plays, a cell
that is on, a saved slot, a chain's chord, a scale tone, a black key that has a function); dark = nothing. The glow is
the HAL's existing layer (fm1_led_dim, FM1_GLOW_NS a frame), reached by a small helper in hal/fm1_input.h
(fm1_led_glow_key, Dinghy's file) and a key_glow draw op. Its cost, measured in fm1-emu: the default pattern playing
from 34.0% to 36.0% of the chip (instruction count: the timer's tick waits out the glow pulse where the shift is
shorter than it); a cheaper timing (no reads once the pulse is over) gave nothing and was not kept. How bright the
glow reads against lit is to be checked on an FM-1 (F24). Tests: ui_home_perform (steps glow, the playhead lit),
keys_record (scale tones glow, the root lit).

## Modulation seen moving (2026-11-03)

ROADMAP E7 (2), F14. EVOLVE, DOT and the LFO moved a value without the screen showing it. The audio now keeps what
modulation does to each destination at the end of a block (MelloAudio.mod_view, permille of the destination's range,
11 values, written once a block, never read back by the audio); the UI looks at it every 66 ms (~15 times a second,
paced by its own tick, so the UI's state does not depend on the clock) and redraws the body only when a thumb on the
page moved. A value row whose parameter is a modulation destination draws a second, white, thin thumb where the value
is now; none when nothing moves it, so a still page looks as before. Which row is which destination is found from
the row's label on the page (CUT, RES, NOISE, FM, ...), one table. Test: mod_view.

## Notes by name (2026-11-03)

ROADMAP F17 (Phase 3's music units). The KEY page showed ROOT as a MIDI number (60) and a LOOP slot's KEY as a pitch
class (0-11); both are named now (C4; D), as the step pages already named their notes. The other bare numbers on the
pages were looked at and are counts (HOLD 16, PULSES 6, SPAN 2) and stay numbers.

## GROOVE (2026-11-03)

ROADMAP E10, F15. Wanted: more rhythm than MUSICAL (rests, several hits on a step, more complexity). INT on the
RANDOM page now goes LOW, MED, HIGH, MUSICAL, GROOVE. GROOVE is MUSICAL's phrase (form, melody, contour, the end on
the root, MUSICAL's random order untouched) with a rhythm layer on each new cell, in this order:

- **Ties.** A note on a beat before a rest is held over two pulses (pulses 2, HOLD) and the rest after it skipped,
  so a bar stays a bar; now and then a tie swallows the weak note after it instead. A tie never crosses a cell.
- **Offbeat ratchets.** An odd step that plays doubles (ratchet 2), and the step after it rests more often than not:
  a figure and its answer.
- **A roll in.** A cell's last note three or four times, accented (40% of motifs, 60% of answers).
- **Ghost notes.** A weak rest between notes plays the note before it again, or a weak note becomes one: a chance
  note at 45%, unaccented, so the loop breathes from round to round. Ghosts are written whatever the mask says about
  chance (the default mask leaves chance alone); MUSICAL's own chance notes still follow the mask.

The mask still picks what changes: without PULSES no ties, without GATE no ghosts; a tie the mask cuts in two is
undone. Over 300 rolls of 16 steps (tests/mello/random_groove.c): 22% of the notes ratchet (MUSICAL 7%), a roll in
in two rolls of three, a tie in two of three, ghost notes in three of five; 115 of 158 offbeat ratchets answered by
a rest. INT is not saved, as before.

## The stacks measured, the event smaller (2026-11-03)

ROADMAP B4's leftovers, F21. **Stacks.** They lie outside the 96 KiB of .data/.bss (app.ld: the user stack 23.75
KiB, the system stack, where the interrupts run, 7.75 KiB, each under a 256 B guard band), so the build's RAM guard
never saw them. tools/emu_stack.sh runs a package for 11 s in fm1-emu (PLAY, RNDMZR three times, every page, SAVE,
a held step) and reads how deep each went: the emulator's RAM starts at zero, so the lowest word not zero is the
high-water mark (a frame of only zeros would hide: it can read a little short). 0.5-f59, the default pattern and the
worst case alike: user 3.2-3.4 KiB, system 1.1 KiB; neither guard band touched. The deepest is the main loop's (the UI),
not the audio. Room enough: no change. **The event.** MelloEvent held three enums of 4 bytes (type, voice, gate);
held in a byte each it is 44 B, not 60: both queues together 2 KiB back (RAM 89952 -> 87904 B of 98304). **UNDO** as
one packed slot was looked at and left: its copies are 1.1 KiB, and packing them costs a codec on every UNDO.

## A save without a stall, a clear on purpose (2026-11-03)

The review's P2 items (ROADMAP F22). **The stall.** A save (2 s after an edit while stopped, or SAVE) erased the
record's sectors at once, up to 11 for a project with every pattern slot full: each 4 KiB erase holds the main loop
~50-400 ms on the chip, so the screen and the keys froze 0.5-4 s, often in the middle of editing. mello_storage_poll
now erases one sector a call and writes the record once the bank is erased; PLAY half-way stops it (nothing is
written while playing, as before), and it goes on where it was, erasing nothing twice. The bank is always the one not
in use, so a save cut anywhere leaves the last project whole. The check that no bank holds a newer firmware's
project runs before the first sector, as before the first erase. A held key or a knob still waits out one sector.
Test: storage_save_steps. **The clear.** CC 113 on channel 16 (the sample tool's CLEAR) emptied a slot for any
controller that sent it while stopped. It is now taken only as the very next channel message after a QUERY (CC 114)
of the same slot; tools/fm1_samples.py clear sends both (the host tool is in tools/, Dinghy's directory). Realtime
bytes between do not count. Test: sample_transfer_roundtrip. **NOSAVE.** The third item, the screen saying nothing
when saving is off after a rollback, was already done: the header shows NOSAVE (or FULL) in red.

## The emulator smoke (2026-11-03)

ROADMAP Phase 0's last item, F23. The old ci/build_target.sh ran two smokes on the web emulator at 80 MHz: playing
while the encoders turned, the sound still alive after, and real-time margin. tools/emu_smoke.sh does the same on
fm1-emu, which this repository already measures with: 10.5 s, PLAY after the splash, every page, K1-K4 and SELECT
turned while it plays, a roll, REC; it fails if the run stops short, a stack guard trips, the sound is silent or the
last 2 s are under a tenth of the run's peak, or the audio interrupt passes 60% of the chip (85 for a MELLO_STRESS
build). 0.5-f60: 29.5%, the sound alive to the end. `make emu PKG=...` runs it and tools/emu_stack.sh. A first
version pressed PLAY during the splash, which drops keys: it heard one note die away and still passed; the tail check
is now relative to the run's peak, so that case fails. The budget report the old script had is tools/build.py's,
on every build. CI does not run the smoke yet: the shared zvenson/fm1-build workflow has no fm1-emu, and changing a
workflow file needs the push token's workflow scope.

## The output level (2026-11-03)

ROADMAP F20. A departure from the HTML, asked for: "as loud as it should be, in the upper range before it clips;
never mind the HTML here". Measured at the default MASTER over 8 s renders: the default pattern peaked at -15.8 dBFS
(RMS -32.8), twenty rolls with every part on at -12 .. -16, the worst case at -14.9. MASTER was applied after the
limiter, so even at its top the output could not reach the limiter's -1 dBFS. Now the mix goes through MASTER, then
a fixed +11 dB (OUTPUT_GAIN_Q12), then the limiter (-1 dBFS, as before), then out. The default pattern peaks at
-4.8 dBFS (RMS -21.8), rolls with every part on at -0.9 .. -3.4, the worst case at -3.9: nothing clips, the limiter
only takes what is turned up past that (DRIVE, MIX's levels up). The tail detector that keeps the effects running
(and lets SAVE in when they are done) looks for more than 8 LSB as before the gain: the reverb's last bits sit a few
LSB above zero for ever, and at +11 dB they would have kept it from ever being silent (audio_silence_regression
caught that). `make parity` renders with the gain at 0 dB (-DMELLO_OUTPUT_GAIN_Q12=4096), so it still compares the
engine with the HTML and not the FM-1's output stage.

## Distortion keeps its level (2026-11-03)

Asked for with the output level: gain compensation wherever a drive is turned ("fine on the main synth, missing on
the drums, among others"). The main DIST had it (its curve scaled so a sine at -8 dBFS keeps its AMT 0 level), and
so did the oscillators' SAT. Measured alone (tests/mello/drive_makeup.c, the output's gain at 0 dB so the limiter
hides nothing), against the same part with no drive, 25 .. 100%:

| drive | before | now |
|---|---|---|
| the kit's DRIVE | +5.5 .. +7.1 dB (its fixed -6 dB at full was not enough) | -0.2 .. -1.2 dB |
| the kit's DRIVE with FOLD 60% | +3.6 .. +6.9 dB | -0.6 .. -1.0 dB |
| the kick's TONE (its own drive) | within 0.2 dB | unchanged |
| the bass's DRIVE | -1.6 .. +1.1 dB | unchanged |

So the kit's DRIVE was the one that needed it, and a sample slot's DIST (soft_drive's up to 1.73x: a sample at -6
dBFS ~4.5 dB louder at full, worked out from the curve; no render test, as it needs a sample loaded). Both now have
a makeup worked out on the main loop when the config changes (the stage's mean |y| held to its level without the
drive) and the render only multiplies. The kit's DRIVE is modulated, so it is a table of 17 points over DRIVE (with
the config's FOLD), read with a multiply, no divide. A single sine did not fit the kit (its hits are transients, and
the tails are what a drive lifts most): its reference is a hit, a sine dying away from about a kick's peak. The
kick's TONE and the bass's DRIVE stay as they were; the test keeps them within 2 dB. The snare's fixed crunch has no
knob and stays part of its sound.

## Engineering process decisions

- GitHub repo state is the durable source of truth between chats.
- Read current branch HEAD and project docs before editing.
- Work in small verifiable packages.
- Do not claim completion without exact commit and CI evidence.
- Diagnostic probes may intentionally fail CI, but that state must be clearly documented and removed/disabled before declaring the real feature green.
- Perceptual audio acceptance needs listening criteria in addition to numerical DSP tests.
- Do not implement several approved future work packages at once merely because their product direction is now locked.
