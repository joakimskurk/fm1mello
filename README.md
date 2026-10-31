# SkurkyMello for the M-VAVE FM-1

A generative step sequencer and synth as a firmware for the M-VAVE FM-1: a 16-step sequence with segments, a lead
voice with filter, grit and a second oscillator, a bass with its own sequencer, a chord layer, a four-voice drum kit,
a sample layer, delay, reverb, distortion, a dot-matrix and an evolving-register modulator, a randomizer with undo,
performance keys, 64 pattern slots and a loop cycle, all on the FM-1's keys, knobs and 240 x 240 screen. It is a
port of [SkurkyMello](reference/skurkymello.html), the browser instrument, which stays the reference for sound and
feel, and it is built on [Dinghy](https://github.com/zvenson/dinghy) 0.3 by zvenson and, under that, the Felucca /
SLOOP platform.

> **Status: tested on emulators, not yet on an FM-1.** Every package runs in
> [fm1-emu](https://github.com/joakimeriksson/fm1-emu) and its web emulator; the first install on real hardware
> follows [docs/mello/FIRST_FLASH.md](docs/mello/FIRST_FLASH.md). Every flash can brick a unit: hold OCT- while
> switching on to reach the updater.

This README is the manual. The design record is in [docs/mello/DECISIONS.md](docs/mello/DECISIONS.md), the plan in
[ROADMAP.md](ROADMAP.md), and every page's knobs and keys, generated from the code, in
[docs/mello/UI_MAP.md](docs/mello/UI_MAP.md).

## Install

Packages are `build/skurkymello-<version>.fwsc`. Install one on an FM-1 running M-VAVE's official V15 (or another
firmware with the FM-1 update loader), over USB-MIDI:

```sh
pip3 install mido python-rtmidi
python3 tools/fm1_install.py build/skurkymello-0.5-f27.fwsc
python3 tools/fm1_install.py --info        # what the FM-1 runs now
```

The same package plays in the web emulator (`joakimeriksson.github.io/fm1-emu`) and in `fm1-emu` on a computer. The
splash shows the version at boot.

## The panel

The FM-1's panel is printed, so its names are the firmware's names. What each control is, everywhere:

| Control | What it is |
|---|---|
| **HOME** | The sequence, the four sound macros, the performance keys. From anywhere. |
| **SEQ, ENV, LFO, FX, ARP, SEL, GLO** | The sections. A press opens the section's first page; a press on the section you are in goes to its next page. |
| **ALGORITHM** (encoder) | The page within the section. |
| **SELECT** (encoder) | The layer within the page. The picked step or cell never moves with it. |
| **K1-K4** | The four values of the layer, always musical, never navigation. The screen's footer names them. |
| **PRESETS** (encoder) | On HOME: the pattern slot (it switches on the next bar). |
| **White keys** | Positions: steps, cells, bits, slots, chords. A tap toggles what sits on the key and picks it. |
| **Black keys** | The action rail, with fixed places (below). |
| **OCT- / OCT+** | The step bank where there are more than 16 (HOME, STEP), the octave in KEYS. |
| **PLAY** | Start and stop. The LED blinks on the beat. |
| **SAVE** | Saves the project ("SAVED" on the status line). Held: the pattern slots. |
| **EDIT** | The page's mode: KEYS on HOME (the keys become a keyboard), the pitch mode on STEP. |
| **REC** | In KEYS: step record, each key writes the next step. |
| **MASTER** (the knob) | The output level, in hardware. |

The header reads where you are as `BUTTON > PAGE > LAYER` (`ARP > DRUMS > KIT`), the status line the key, the step
and the slot, and the save state: SAVED, EDITED (written when the sequencer stops), NOSAVE (the flash holds a newer
firmware's project) or FULL (the project does not fit).

### The black keys, the same on every page

| Key | On every page | |
|---|---|---|
| B0 | **ON** for the page's part, where there is one (BASS, CHORD, DRUMS, SAMPLE, EVOLVE, LOOP CYCLE, SLOT SEQ) | |
| B1-B4 | The page's own keys, in the order pick, toggles, actions (HOME: transpose `-` `+`, MUTATE, RESTART, CLEAR) | |
| B5 | **F**ILL, held: every step plays, ratchets doubled | performance |
| B6 | **B**ASS MUTE, held: no new bass notes, the sounding bass fades | performance |
| B7 | **<** REVERSE, held: what was heard, backwards | performance |
| B8 | **R**EPEAT, held: the last beat round and round (a white key with it: 1-16 pulses) | performance |
| B9 | **UNDO**: the last roll or edit back (again: redo) | |
| B10 | **RNDMZR**: a roll of the randomizer; held: the RANDOM page | |

The performance keys act while held and keep time: the sequence's own place moves on underneath, so letting go
lands where it would have been. They are never saved or undone.

### The white keys, page by page

| Page | A white key |
|---|---|
| HOME | Tap: the step on or off. Hold: the step's pitch, pulses, ratchet and gate on K1-K4, its flags on the first four black keys. In KEYS: a keyboard; tap transposes the sequence, stopped it plays the lead. |
| STEP | Picks the step (a tap toggles it too); with EDIT on, a key sets its pitch. |
| SEGMENT | Cuts or joins a segment at that step; SEL held: picks the segment. |
| LOOP CYCLE | Picks a loop position (the first eight keys); with the cycle off, auditions it. |
| SLOT SEQ, SLOTS, SAMPLE | Picks a position, a slot. |
| RANDOM | A target on or off (MASK), follows or mirrors the pitch (LINKS), picks the target (AMOUNTS). |
| KEY | The root. |
| DOT | A cell on or off. |
| EVOLVE | Flips a bit and picks it. |
| BASS | A BASS SEQ step on or off, and picks it. |
| CHORD | Picks a chord of the chain (or a pattern step). |
| DRUMS | The picked voice's hit on or off at that step, and picks the step. |

## HOME

Sixteen steps as bars (height = pitch, dim = off, the playing one bright), the segments A-D over them in their
colours. K1-K4 are the four macros, live while playing: CUTOFF, DECAY, SPACE (delay and reverb send) and DRIVE.
SELECT is the tempo, ALGORITHM the randomizer's intensity (LOW, MED, HIGH, MUSICAL), PRESETS the pattern slot.
Hold a white key to edit that step on the knobs; tap it to mute or unmute it. RNDMZR rolls what the RANDOM page's
mask says (by default the first nine targets); UNDO takes it back.

## The sections

Each section is a button. Press it again (or turn ALGORITHM) for its next page; SELECT turns the page's layers.

| Button | Pages | Layers |
|---|---|---|
| **SEQ** | STEP · SEGMENT · LOOP CYCLE · SLOT SEQ | STEP: TRIG, NOTE, FLAGS, SEQ · LOOP CYCLE: STEP, HARM, LOOP |
| **ENV** | SOUND · SAMPLE | SOUND: OSC/FILT, AMP ENV, LFO, FOLLOWER, LEVEL, GRIT, OSC 2, CHARACTER, MOD ENV, TRACKING · SAMPLE: MAIN, PITCH, TRIM, ENVELOPE |
| **LFO** | LFO · DOT · EVOLVE | DOT: the five matrices ACCENT, MOD 1, MOD 2, RATCHET, SUB · EVOLVE: MAIN, ROUTES, DEPTH, SOUND |
| **FX** | DISTORTION · DELAY · REVERB | DELAY: SEND, CONFIG · REVERB: ROOM, RETURNS |
| **ARP** | CHORD · BASS · DRUMS | CHORD: PLAY, CHORD, VOICING, GRID, PATTERN, TONE, FILTER, MOTION, AMP · BASS: PLAY, SEQ, STEP, VOICE, TONE, FILTER · DRUMS: VOICE, STEP, VARY, KIT, DIST |
| **SEL** | KEY / SCALE | |
| **GLO** | MIX · GLOBAL | GLOBAL: CLOCK, MIDI, GROOVE |
| hold **SAVE** | SLOTS | LOAD, RENAME |
| hold **RNDMZR** | RANDOM | MASK, LINKS, AMOUNTS |

Every page has one body: the part's picture on top, drawn from its values, and the four knobs as rows under it.
Levels and amounts read in %, times in ms (or s), cutoffs in Hz, pitch in semitones.

### What the parts do

- **The sequence** (SEQ > STEP). 16 steps (up to 32), each with a pitch in the key, pulses (its length), ratchets,
  a gate (SINGLE, HOLD, REST, MULTI), an octave, a chance, a sample slot and its own wave; flags for accent, sub,
  slide and skip. SEQ's last layer sets the length, the bar reset, the clock division and the pitch span.
- **Segments** (SEQ > SEGMENT). The sequence cut into up to six segments A-F, each with repeats, an order (FWD, REV,
  ping-pong, random) and a skip; EVEN spreads the cuts.
- **Loop cycle** (SEQ > LOOP CYCLE). Up to eight steps that transpose the whole sequence, change its key or scale
  and the bass, each held for a number of pulses: a chord progression over the loop.
- **Slot sequence** (SEQ > SLOT SEQ). Pattern slots played in a row, each held for a number of pulses.
- **The lead** (ENV > SOUND). Saw, square, triangle, sine or pulse through a 24 dB filter (LP, HP, BP, NOTCH), an ADSR,
  an LFO (filter, pitch, PWM, amp, and the drums), a follower sub an octave or two under it, grit (noise, noise
  FM, unison with detune), a second oscillator with sync and FM, saturation, a MOD envelope, velocity and key
  tracking. INIT (B1) puts the sound back.
- **The sample layer** (ENV > SAMPLE). Four slots of user samples, per step or fixed, with pitch, trim, loop and an
  envelope.
- **DOT** (LFO > DOT). Five 4 x 4 matrices read along a route (horizontal, spiral, random, walk ...) at the
  sequence's pace: accents, ratchets, the sub, and two MOD lanes that shape a sound parameter with an attack and a
  release.
- **EVOLVE** (LFO > EVOLVE). A Turing-machine register that mutates by a probability, routed to gate, filter,
  octave and decay, and to two sound destinations.
- **FX**. Distortion with a character and a tone (PRE-DIST puts the delay before it), a delay synced to the tempo
  or free with feedback, low and high cut, and a reverb with size, decay, pre-delay and tone.
- **Chords** (ARP > CHORD). A chain of up to eight chords in the key (degree, shape, inversion, spread, octave),
  played as stabs, held, arpeggiated or by a pattern of voice masks, triggered by the steps or by a grid; voice
  leading; a Juno-like voice with chorus, a resonant filter with its envelope, an ADSR and its own space. B1 BASS
  puts the bass on the chord's root.
- **Bass** (ARP > BASS). The dedicated sub voice: a step, euclidean, matrix, register or drone mode, or BASS SEQ, a
  16-step bass line of its own with tone, octave, glide and accent per step, a resonant filter with envelope, drive.
- **Drums** (ARP > DRUMS). Kick, snare, hat and a glitch voice (bleep, chirp, crackle, stutter) on a 16-step
  pattern with accents, per-step pitch, decay, crush and ratchet locks, rolls, chance, swing (GLOBAL > GROOVE, for
  everything), a kit drive with fold, and a send to the delay and reverb. RNDMZR rolls them too with FEED on.
- **MIX** (GLO > MIX). The levels of lead, bass, chords and drums as faders, each with a mute (LD, BS, CH, DR).
- **GLOBAL** (GLO > GLOBAL). Tempo (TAP), division, external sync, note out; the MIDI channels of lead and bass,
  the note length; the groove's swing. CLOCK shows the chip's measured audio load as CPU nn%.
- **KEY** (SEL). Root, scale, global octave, pitch span.
- **RANDOM** (hold RNDMZR). Which targets a roll touches (MASK), which follow or mirror the pitch (LINKS), how much
  each moves (AMOUNTS); the intensity on K1. MUSICAL rolls a phrase, not noise.

## Every page's knobs and keys

Generated from the code (`docs/mello/UI_MAP.md`): the strip is what the black keys B0..B10 read on that page, then
K1-K4 with their default values.

| Page | Button | Layer | Strip B0..B10 | K1 · K2 · K3 · K4 |
|---|---|---|---|---|
| HOME | HOME | — | `- + MUT RST CLR F B < R UNDO RNDMZR` | CUTOFF 3264HZ · DECAY 180MS · SPACE 24% · DRIVE 0% |
| STEP | SEQ | TRIG | `SKIP F B < R UNDO RNDMZR` | PITCH +0 · PULSE 1 · RATCH 1 · GATE SINGLE |
| STEP | SEQ | NOTE | `SKIP F B < R UNDO RNDMZR` | OCT +0 · PROB 100% · SAMP 1 · WAVE SND |
| STEP | SEQ | FLAGS | `SKIP F B < R UNDO RNDMZR` | ACCENT OFF · SUB OFF · SLIDE OFF · SKIP OFF |
| STEP | SEQ | SEQ | `SKIP F B < R UNDO RNDMZR` | LENGTH 16 · RESET 2 BAR · DIV 1 · SPAN 2 |
| SEGMENT | SEQ | SEGMENT | `EVEN A B C D F B < R UNDO RNDMZR` | REPEAT 1 · ORDER FWD · SKIP OFF · LENGTH 4 |
| SEGMENT | SEQ | (2) | `EVEN A B C D F B < R UNDO RNDMZR` | REPEAT 1 · ORDER FWD · SKIP OFF · LENGTH 4 |
| LOOP CYCLE | SEQ | STEP | `ON RST F B < R UNDO RNDMZR` | HOLD 16 · ROOT+ +0 · OCT +0 · TRANS +0 |
| LOOP CYCLE | SEQ | HARM | `ON RST F B < R UNDO RNDMZR` | KEY FOLLOW · SCALE FOLLOW · SUB 0 · BASS TR +0 |
| LOOP CYCLE | SEQ | LOOP | `ON RST F B < R UNDO RNDMZR` | LENGTH 4 · SYNC SYNC · ON OFF · — |
| SLOT SEQ | SEQ | — | `ON F B < R UNDO RNDMZR` | SLOT OFF · HOLD 16 · ON OFF · LENGTH 16 |
| RANDOM | hold RNDMZR | MASK | `NONE ALL CLR F B < R UNDO RNDMZR` | INTENS MED · — · — · — |
| RANDOM | hold RNDMZR | LINKS | `NONE ALL CLR F B < R UNDO RNDMZR` | INTENS MED · — · — · — |
| RANDOM | hold RNDMZR | AMOUNTS | `NONE ALL CLR F B < R UNDO RNDMZR` | AMOUNT PIT 10~ · — · — · — |
| KEY / SCALE | SEL | — | `F B < R UNDO RNDMZR` | ROOT 60 · SCALE minor · OCT +0 · SPAN 2 |
| SLOTS | hold SAVE | LOAD | `F B < R UNDO RNDMZR` | — · VISIBLE 8 · — · — |
| SLOTS | hold SAVE | RENAME | `F B < R UNDO RNDMZR` | CURSOR 1 · CHAR - · — · — |
| DOT MATRIX | LFO | ACCENT | `RND RST FLW REV F B < R UNDO RNDMZR` | MODE HORIZ · LENGTH 16 · CHANCE 100% · EVOLVE 0% |
| DOT MATRIX | LFO | MOD 1 | `RND RST FLW REV F B < R UNDO RNDMZR` | MODE HORIZ · LENGTH 16 · DEPTH 24% · EVOLVE 0% |
| DOT MATRIX | LFO | MOD 2 | `RND RST FLW REV F B < R UNDO RNDMZR` | MODE HORIZ · LENGTH 16 · CHANCE 100% · EVOLVE 0% |
| DOT MATRIX | LFO | RATCHET | `RND RST FLW REV F B < R UNDO RNDMZR` | MODE HORIZ · LENGTH 16 · CHANCE 100% · EVOLVE 0% |
| DOT MATRIX | LFO | SUB | `RND RST FLW REV F B < R UNDO RNDMZR` | MODE HORIZ · LENGTH 16 · DEPTH 24% · EVOLVE 0% |
| EVOLVE | LFO | MAIN | `ON RND LOOP F B < R UNDO RNDMZR` | MUTATE 10% · LENGTH 8 · RATE 1X · OFFSET +0 |
| EVOLVE | LFO | ROUTES | `ON RND LOOP F B < R UNDO RNDMZR` | GATE ON · FILTER OFF · OCT OFF · DECAY OFF |
| EVOLVE | LFO | DEPTH | `ON RND LOOP F B < R UNDO RNDMZR` | GATE 35% · FILTER 35% · OCT 35% · DECAY 35% |
| EVOLVE | LFO | SOUND | `ON RND LOOP F B < R UNDO RNDMZR` | DEST A OFF · DEPTH A +50% · DEST B OFF · DEPTH B +50% |
| BASS | ARP | PLAY | `ON STEP F B < R UNDO RNDMZR` | MODE STEP · PULSES 6 · ROTATE 0 · OCT -1 |
| BASS | ARP | SEQ | `ON STEP F B < R UNDO RNDMZR` | SEQ OFF · LENGTH 16 · FOLLOW OFF · GATE 90% |
| BASS | ARP | STEP | `ON STEP F B < R UNDO RNDMZR` | TONE ROOT · OCT +0 · GLIDE OFF · ACCENT ON |
| BASS | ARP | VOICE | `ON STEP F B < R UNDO RNDMZR` | LEVEL 42% · WAVE SINE · DECAY 160MS · RELEASE 120MS |
| BASS | ARP | TONE | `ON STEP F B < R UNDO RNDMZR` | GLIDE 20MS · CUT 242.0HZ · DRIVE 0% · OCT -1 |
| BASS | ARP | FILTER | `ON STEP F B < R UNDO RNDMZR` | RES 0% · ENV 0% · F.DEC 300MS · ACCENT 50% |
| CHORD | ARP | PLAY | `ON BASS F B < R UNDO RNDMZR` | MODE PATTERN · TRIG GRID · CHORDS 4 · GATE 72% |
| CHORD | ARP | CHORD | `ON BASS F B < R UNDO RNDMZR` | DEG 1 · SHAPE TRIAD · HOLD 2 · OCT -1 |
| CHORD | ARP | VOICING | `ON BASS F B < R UNDO RNDMZR` | INV 0 · SPREAD 1 · VOICES 3 · VLEAD OFF |
| CHORD | ARP | GRID | `ON BASS F B < R UNDO RNDMZR` | MODE HORIZ · LENGTH 16 · DEPTH 100% · FOLLOW OFF |
| CHORD | ARP | PATTERN | `ON BASS F B < R UNDO RNDMZR` | MASK 1 · LENGTH 8 · — · — |
| CHORD | ARP | TONE | `ON BASS F B < R UNDO RNDMZR` | WAVE SAW · CHORUS 60% · SPACE 100% · LEVEL 32% |
| CHORD | ARP | FILTER | `ON BASS F B < R UNDO RNDMZR` | CUT 986.0HZ · RES 18% · ENV 38% · LINK OFF |
| CHORD | ARP | MOTION | `ON BASS F B < R UNDO RNDMZR` | F.ATK 8MS · F.DEC 700MS · STRUM 0MS · NEXT TRIG |
| CHORD | ARP | AMP | `ON BASS F B < R UNDO RNDMZR` | ATTACK 40MS · DECAY 900MS · SUSTAIN 75% · RELEASE 900MS |
| GLOBAL | GLO | CLOCK | `TAP RST2 F B < R UNDO RNDMZR` | BPM 80 · DIV 1 · SYNC OFF · NOTEOUT OFF |
| GLOBAL | GLO | MIDI | `TAP RST2 F B < R UNDO RNDMZR` | LEAD 1 · SUB 2 · NOTE STEP · LENGTH 180MS |
| GLOBAL | GLO | GROOVE | `TAP RST2 F B < R UNDO RNDMZR` | SWING 0% · — · — · — |
| SOUND | ENV | OSC/FILT | `INIT F B < R UNDO RNDMZR` | WAVE SAW · PWM 50% · CUT 3264HZ · RES 20% |
| SOUND | ENV | AMP ENV | `INIT F B < R UNDO RNDMZR` | ATTACK 5MS · DECAY 180MS · SUSTAIN 40% · RELEASE 120MS |
| SOUND | ENV | LFO | `INIT F B < R UNDO RNDMZR` | LFO OFF · RATE 2.00HZ · DEPTH 35% · TARGET FILTER |
| SOUND | ENV | FOLLOWER | `INIT F B < R UNDO RNDMZR` | SUBOSC ON · LEVEL 22% · OCT -1 · WAVE SINE |
| SOUND | ENV | LEVEL | `INIT F B < R UNDO RNDMZR` | SLIDE 50MS · MASTER 75% · — · — |
| SOUND | ENV | GRIT | `INIT F B < R UNDO RNDMZR` | NOISE 0% · N.FM 0% · UNISON 1 · DETUNE 0 CT |
| SOUND | ENV | OSC 2 | `INIT F B < R UNDO RNDMZR` | WAVE OFF · SEMI +0 ST · FINE +7 CT · MIX 0% |
| SOUND | ENV | CHARACTER | `INIT F B < R UNDO RNDMZR` | SYNC OFF · FM 0% · SAT 0% · FILTER LP |
| SOUND | ENV | MOD ENV | `INIT F B < R UNDO RNDMZR` | AMT +0% · ATTACK 0MS · DECAY 300MS · TARGET CUTOFF |
| SOUND | ENV | TRACKING | `INIT F B < R UNDO RNDMZR` | V.CUT +0% · V.LVL 100% · KEYTRK 0% · V.ENV +0% |
| AUDIO FX | FX | DISTORTION | `PRE-DIST F B < R UNDO RNDMZR` | AMT 0% · MIX 100% · CHAR 35% · TONE 82% |
| AUDIO FX | FX | DELAY SEND | `PRE-DIST F B < R UNDO RNDMZR` | WET 12% · FDBK 32% · LO CUT OFF · HI CUT 72% |
| AUDIO FX | FX | DELAY CONFIG | `PRE-DIST F B < R UNDO RNDMZR` | SYNC ON · DIV 8N · TIME 250MS · LEVEL 75% |
| AUDIO FX | FX | REVERB ROOM | `PRE-DIST F B < R UNDO RNDMZR` | WET 24% · SIZE 70% · DECAY 6.0S · TONE 62% |
| AUDIO FX | FX | REVERB RETURNS | `PRE-DIST F B < R UNDO RNDMZR` | PRE 12MS · LEVEL 115% · D.LEVEL 90% · — |
| SAMPLE | ENV | MAIN | `ON F B < R UNDO RNDMZR` | LEVEL 70% · DIST 0% · TRACK ON · LOOP OFF |
| SAMPLE | ENV | PITCH | `ON F B < R UNDO RNDMZR` | COARSE +0 · FINE +0 · LEVEL 70% · DIST 0% |
| SAMPLE | ENV | TRIM | `ON F B < R UNDO RNDMZR` | START 0% · END 100% · TRACK ON · LOOP OFF |
| SAMPLE | ENV | ENVELOPE | `ON F B < R UNDO RNDMZR` | ATTACK 5MS · DECAY 120MS · SUSTAIN 70% · RELEASE 150MS |
| DRUMS | ARP | VOICE | `ON VOX ACC ROLL F B < R UNDO RNDMZR` | TUNE 40% · DECAY 174MS · TONE 10% · LEVEL 90% |
| DRUMS | ARP | STEP | `ON VOX ACC ROLL F B < R UNDO RNDMZR` | PITCH +0 ST · DECAY +0% · CRUSH 0% · RATCH 1 |
| DRUMS | ARP | VARY | `ON VOX ACC ROLL F B < R UNDO RNDMZR` | CHANCE 100% · CRUSH 0% · FEED OFF · ROLL 50% |
| DRUMS | ARP | KIT | `ON VOX ACC ROLL F B < R UNDO RNDMZR` | LENGTH 16 · FOLLOW OFF · SPACE 25% · LEVEL 70% |
| DRUMS | ARP | DIST | `ON VOX ACC ROLL F B < R UNDO RNDMZR` | DRIVE 0% · FOLD 0% · — · — |
| MIX | GLO | — | `LD BS CH DR F B < R UNDO RNDMZR` | LEAD 100% · BASS 42% · CHORD 32% · DRUMS 70% |

## Saving, slots and MIDI

The project (the sequence, every part, 64 slots and the slot sequence) lives in flash and is saved by SAVE, and on
its own after an edit once the sequencer stops; two banks with checksums, so a cut power never loses more than the
last edit. SAVE held opens the slots: LOAD a slot onto the sequence, RENAME it with the keys; PRESETS on HOME queues
a slot for the next bar. 64 slots, all full, fit the bank.

MIDI over USB: clock in (GLOBAL > CLOCK, SYNC), notes out for the lead and the bass on their channels (NOTE OUT)
and the drums on channel 10 as General MIDI, notes in on the lead's channel, and user samples in over the
firmware's own USB-MIDI transfer (mello_sample.h; a host tool for it is not in this repository yet).

## Work on it

```sh
make test          # the host tests (tests/mello): seconds, no FM-1, no toolchain
make sanitize      # the same with UBSan and ASan
make parity        # the C engine against the HTML's renders (needs numpy)
make bench         # where the audio's time goes on this computer
make host          # the whole firmware on this computer: a WAV and screen PNGs (pip3 install Pillow)
make format        # clang-format the Mello sources (pip3 install clang-format)
```

One change, `make test`, `make format`, commit. The target build checks what the host cannot: RAM, the pools and the
package. Audio code keeps to the rules in CLAUDE.md: no flash, no unbounded loops, no 64-bit division in the audio
callback, fixed point in range.

### Build the firmware

Python 3 with Pillow, the JieLi toolchain and the AC79 SDK (as in Dinghy's README):

```sh
tools/get_toolchain.sh                      # JieLi's clang for the chip, to ~/.jieli/toolchain
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
    https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
./build.sh                                  # -> build/skurkymello-dev.fwsc
./build.sh --release 0.5-f27                # -> build/skurkymello-0.5-f27.fwsc
MELLO_CFLAGS=-DMELLO_STRESS ./build.sh --release 0.5-x1    # the worst case plays at boot, for measuring
```

The toolchain is Linux x86-64 only; on a Mac, `build.sh` runs it in a `linux/amd64` Docker container. GitHub Actions
builds every push the same way (`.github/workflows/build.yml`) and attests the package.

### Measure

`tools/emu_cpu.sh PACKAGE.fwsc` runs the package in fm1-emu, in silence and then playing, and reports the audio
interrupt's share of the chip in instructions per second (the emulator counts instructions, not cycles). The chip's
own cycles show on GLOBAL > CLOCK as CPU nn%. 0.5-f27: the default pattern 32%, the worst case (everything on at
once) 71%, silence 2%; 7.4 KB of RAM free, and the build fails under 2 KB.

## Where things are

```
firmware/src/dinghy.c          the app: wires Mello to the FM-1 (audio callback, keys, screen, storage, the CPU load)
firmware/src/mello_engine.*    the project, its defaults, the generative sequencer, the drums' and chords' logic
firmware/src/mello_transport.* clock, steps, ratchets, gate, MIDI out
firmware/src/mello_audio.*     the synth, bass, chords, drums, samples, FX and reverb (fixed point)
firmware/src/mello_ui.*        pages, keys, pictures
firmware/src/mello_storage.*   projects in flash (A/B banks with CRC)
firmware/src/mello_sample.*    user samples
firmware/src/platform.c, hal/  Dinghy's platform, as close to upstream as possible
tests/mello/                   host tests;  tests/bench/  the CPU bench;  tests/parity/  the HTML parity
tools/emu_cpu.sh, fm1_install.py, build.py
reference/skurkymello.html     the original: the reference for sound and feel
docs/mello/                    DECISIONS.md, UI_MAP.md, CONTROL_GRAMMAR.md, UI_ICEBERG.md, FIRST_FLASH.md
```

## Upstream and licence

`main` is a fork of Dinghy at `d44df7d` (0.3); `git remote upstream` points there. The Mello changes to Dinghy's own
files are small and listed in `git log -- firmware/src/platform.c firmware/src/gfx.c tools/build.py`. Package
identity: `FM-1_970`.

GPL-3.0-only, as Dinghy. See [LICENSING.md](LICENSING.md) for who made what.
