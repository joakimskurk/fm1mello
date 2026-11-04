# UI map (generated 2026-10-31, from the code at b6ef874)

Every page and layer, rendered in host mode: the panel button that opens it, the strip (the black keys B0..B10) and
K1-K4's labels with their default values. The text rows are kept by hand with the code (the tests rotary_mapping_parity, control_authoritative_mapping and ui_glossary pin them); the pictures come from `make screens`.

| Screen | Page | Button | Layer | Strip B0..B10 | K1 · K2 · K3 · K4 |
|---|---|---|---|---|---|
| <img src="../screens/home.png" width="96" alt="HOME —"> | HOME | HOME | — | `- + RST CLR P M UNDO RNDMZR` | CUTOFF 3264HZ · DECAY 180MS · SPACE 24% · DRIVE 0% |
| <img src="../screens/perform.png" width="96" alt="PERFORM —"> | PERFORM | B5 PERF | — | `- + RST CLR P M UNDO RNDMZR` | the page's K1-K4; the white keys: FILL, ARP, REVERSE, ACCENT, REP 8, REP 4, REP 2, REP 1, LP SWP, HP SWP, THROW, WASH, LD OFF, BS OFF, CH OFF, DR OFF |
| <img src="../screens/home-macros.png" width="96" alt="HOME MACROS"> | HOME | hold EDIT | MACROS | `- + RST CLR P M UNDO RNDMZR` | each knob picks what it turns (CUTOFF, RES, DECAY, ATTACK, RELEASE, SPACE, DRIVE, DELAY, FDBK, REVERB, SIZE, LFO HZ, LFO AMT, PWM, NOISE, DETUNE, OSC 2, FM, SAT, MODENV, SUB LVL, LEAD, BASS, CHORD, DRUMS, SWING) |
| <img src="../screens/step-trig.png" width="96" alt="STEP TRIG"> | STEP | SEQ | TRIG | `SKIP PERF MUT UNDO RNDMZR` | PITCH +0 · PULSE 1 · RATCH 1 · GATE SINGLE |
| <img src="../screens/step-note.png" width="96" alt="STEP NOTE"> | STEP | SEQ | NOTE | `SKIP PERF MUT UNDO RNDMZR` | OCT +0 · PROB 100% · SAMP 1 · WAVE SND |
| <img src="../screens/step-flags.png" width="96" alt="STEP FLAGS"> | STEP | SEQ | FLAGS | `SKIP PERF MUT UNDO RNDMZR` | ACCENT OFF · SUB OFF · SLIDE OFF · SKIP OFF |
| <img src="../screens/step-seq.png" width="96" alt="STEP SEQ"> | STEP | SEQ | SEQ | `SKIP PERF MUT UNDO RNDMZR` | LENGTH 16 · RESET 2 BAR · DIV 1 · SPAN 2 |
| <img src="../screens/segment.png" width="96" alt="SEGMENT SEGMENT"> | SEGMENT | SEQ | SEGMENT | `EVEN A B C D P M UNDO RNDMZR` | REPEAT 1 · ORDER FWD · SKIP OFF · LENGTH 4 |
| <img src="../screens/segment.png" width="96" alt="SEGMENT (2)"> | SEGMENT | SEQ | (2) | `EVEN A B C D P M UNDO RNDMZR` | REPEAT 1 · ORDER FWD · SKIP OFF · LENGTH 4 |
| <img src="../screens/loop-step.png" width="96" alt="LOOP CYCLE STEP"> | LOOP CYCLE | SEQ | STEP | `ON RST PERF MUT UNDO RNDMZR` | HOLD 16 · ROOT+ +0 · OCT +0 · TRANS +0 |
| <img src="../screens/loop-harm.png" width="96" alt="LOOP CYCLE HARM"> | LOOP CYCLE | SEQ | HARM | `ON RST PERF MUT UNDO RNDMZR` | KEY FOLLOW · SCALE FOLLOW · SUB 0 · BASS TR +0 |
| <img src="../screens/loop-loop.png" width="96" alt="LOOP CYCLE LOOP"> | LOOP CYCLE | SEQ | LOOP | `ON RST PERF MUT UNDO RNDMZR` | LENGTH 4 · SYNC SYNC · ON OFF · — |
| <img src="../screens/slotseq.png" width="96" alt="SLOT SEQ —"> | SLOT SEQ | SEQ | — | `ON PERF MUT UNDO RNDMZR` | SLOT OFF · HOLD 16 · ON OFF · LENGTH 16 |
| <img src="../screens/random-mask.png" width="96" alt="RANDOM MASK"> | RANDOM | hold RNDMZR | MASK | `NONE ALL CLR PERF MUT UNDO RNDMZR` | INTENS MED · STYLE ANY · — · — |
| <img src="../screens/random-links.png" width="96" alt="RANDOM LINKS"> | RANDOM | hold RNDMZR | LINKS | `NONE ALL CLR PERF MUT UNDO RNDMZR` | INTENS MED · STYLE ANY · — · — |
| <img src="../screens/random-amounts.png" width="96" alt="RANDOM AMOUNTS"> | RANDOM | hold RNDMZR | AMOUNTS | `NONE ALL CLR PERF MUT UNDO RNDMZR` | AMOUNT PIT 10~ · — · — · — |
| <img src="../screens/key-scale.png" width="96" alt="KEY / SCALE —"> | KEY / SCALE | SEL | — | `PERF MUT UNDO RNDMZR` | ROOT C4 · SCALE minor · OCT +0 · SPAN 2 |
| <img src="../screens/slots-load.png" width="96" alt="SLOTS LOAD"> | SLOTS | hold SAVE | LOAD | `PERF MUT UNDO RNDMZR` | — · VISIBLE 8 · — · — |
| <img src="../screens/slots-rename.png" width="96" alt="SLOTS RENAME"> | SLOTS | hold SAVE | RENAME | `PERF MUT UNDO RNDMZR` | CURSOR 1 · CHAR - · — · — |
| <img src="../screens/dot-accent.png" width="96" alt="DOT MATRIX ACCENT"> | DOT MATRIX | LFO | ACCENT | `ON RND RST FLW REV P M UNDO RNDMZR` | MODE HORIZ · LENGTH 16 · CHANCE 100% · EVOLVE 0% |
| <img src="../screens/dot-mod-1.png" width="96" alt="DOT MATRIX MOD 1"> | DOT MATRIX | LFO | MOD 1 | `ON RND RST FLW REV P M UNDO RNDMZR` | MODE HORIZ · LENGTH 16 · DEPTH 24% · EVOLVE 0% |
| <img src="../screens/dot-mod-2.png" width="96" alt="DOT MATRIX MOD 2"> | DOT MATRIX | LFO | MOD 2 | `ON RND RST FLW REV P M UNDO RNDMZR` | MODE HORIZ · LENGTH 16 · CHANCE 100% · EVOLVE 0% |
| <img src="../screens/dot-ratchet.png" width="96" alt="DOT MATRIX RATCHET"> | DOT MATRIX | LFO | RATCHET | `ON RND RST FLW REV P M UNDO RNDMZR` | MODE HORIZ · LENGTH 16 · CHANCE 100% · EVOLVE 0% |
| <img src="../screens/dot-sub.png" width="96" alt="DOT MATRIX SUB"> | DOT MATRIX | LFO | SUB | `ON RND RST FLW REV P M UNDO RNDMZR` | MODE HORIZ · LENGTH 16 · DEPTH 24% · EVOLVE 0% |
| <img src="../screens/evolve-main.png" width="96" alt="EVOLVE MAIN"> | EVOLVE | LFO | MAIN | `ON RND LOOP PERF MUT UNDO RNDMZR` | MUTATE 10% · LENGTH 8 · RATE 1X · OFFSET +0 |
| <img src="../screens/evolve-routes.png" width="96" alt="EVOLVE ROUTES"> | EVOLVE | LFO | ROUTES | `ON RND LOOP PERF MUT UNDO RNDMZR` | GATE LENGTH · FILTER OFF · OCT OFF · DECAY OFF |
| <img src="../screens/evolve-depth.png" width="96" alt="EVOLVE DEPTH"> | EVOLVE | LFO | DEPTH | `ON RND LOOP PERF MUT UNDO RNDMZR` | GATE 35% · FILTER 35% · OCT 35% · DECAY 35% |
| <img src="../screens/evolve-sound.png" width="96" alt="EVOLVE SOUND"> | EVOLVE | LFO | SOUND | `ON RND LOOP PERF MUT UNDO RNDMZR` | DEST A OFF · DEPTH A +50% · DEST B OFF · DEPTH B +50% |
| <img src="../screens/bass-play.png" width="96" alt="BASS PLAY"> | BASS | ARP | PLAY | `ON SEQ ROLL PERF MUT UNDO RNDMZR` | MODE STEP · PULSES 6 · ROTATE 0 · OCT -1 |
| <img src="../screens/bass-seq.png" width="96" alt="BASS SEQ"> | BASS | ARP | SEQ | `ON SEQ ROLL PERF MUT UNDO RNDMZR` | SEQ OFF · LENGTH 16 · FOLLOW OFF · GATE 90% |
| <img src="../screens/bass-step.png" width="96" alt="BASS STEP"> | BASS | ARP | STEP | `ON SEQ ROLL PERF MUT UNDO RNDMZR` | TONE ROOT · OCT +0 · GLIDE OFF · ACCENT ON |
| <img src="../screens/bass-filter.png" width="96" alt="BASS FILTER"> | BASS | ARP | FILTER | `ON SEQ ROLL PERF MUT UNDO RNDMZR` | CUT 242.0HZ · RES 0% · ENV 0% · LEVEL 42% |
| <img src="../screens/bass-voice.png" width="96" alt="BASS VOICE"> | BASS | ARP | VOICE | `ON SEQ ROLL PERF MUT UNDO RNDMZR` | WAVE SINE · DECAY 160MS · RELEASE 120MS · DRIVE 0% |
| <img src="../screens/bass-tone.png" width="96" alt="BASS TONE"> | BASS | ARP | TONE | `ON SEQ ROLL PERF MUT UNDO RNDMZR` | GLIDE 20MS · OCT -1 · F.DEC 300MS · ACCENT 50% |
| <img src="../screens/global-clock.png" width="96" alt="GLOBAL CLOCK"> | GLOBAL | GLO | CLOCK | `TAP RST2 PERF MUT UNDO RNDMZR` | BPM 80 · DIV 1 · SYNC OFF · NOTEOUT OFF |
| <img src="../screens/global-midi.png" width="96" alt="GLOBAL MIDI"> | GLOBAL | GLO | MIDI | `TAP RST2 PERF MUT UNDO RNDMZR` | LEAD 1 · SUB 2 · NOTE STEP · LENGTH 180MS |
| <img src="../screens/global-groove.png" width="96" alt="GLOBAL GROOVE"> | GLOBAL | GLO | GROOVE | `TAP RST2 PERF MUT UNDO RNDMZR` | SWING 0% · — · — · — |
| <img src="../screens/sound-filter.png" width="96" alt="SOUND FILTER"> | SOUND | ENV | FILTER | `INIT PERF MUT UNDO RNDMZR` | CUT 3264HZ · RES 20% · DECAY 180MS · LEVEL 100% |
| <img src="../screens/sound-osc.png" width="96" alt="SOUND OSC"> | SOUND | ENV | OSC | `INIT PERF MUT UNDO RNDMZR` | WAVE SAW · PWM 50% · SLIDE 50MS · MASTER 75% |
| <img src="../screens/sound-amp-env.png" width="96" alt="SOUND AMP ENV"> | SOUND | ENV | AMP ENV | `INIT PERF MUT UNDO RNDMZR` | ATTACK 5MS · DECAY 180MS · SUSTAIN 40% · RELEASE 120MS |
| <img src="../screens/sound-lfo.png" width="96" alt="SOUND LFO"> | SOUND | ENV | LFO | `INIT PERF MUT UNDO RNDMZR` | LFO OFF · RATE 2.00HZ · DEPTH 35% · TARGET FILTER |
| <img src="../screens/sound-follower.png" width="96" alt="SOUND FOLLOWER"> | SOUND | ENV | FOLLOWER | `INIT PERF MUT UNDO RNDMZR` | SUBOSC ON · LEVEL 22% · OCT -1 · WAVE SINE |
| <img src="../screens/sound-grit.png" width="96" alt="SOUND GRIT"> | SOUND | ENV | GRIT | `INIT PERF MUT UNDO RNDMZR` | NOISE 0% · N.FM 0% · UNISON 1 · DETUNE 0 CT |
| <img src="../screens/sound-osc-2.png" width="96" alt="SOUND OSC 2"> | SOUND | ENV | OSC 2 | `INIT PERF MUT UNDO RNDMZR` | WAVE OFF · SEMI +0 ST · FINE +7 CT · MIX 0% |
| <img src="../screens/sound-character.png" width="96" alt="SOUND CHARACTER"> | SOUND | ENV | CHARACTER | `INIT PERF MUT UNDO RNDMZR` | SYNC OFF · FM 0% · SAT 0% · FILTER LP |
| <img src="../screens/sound-mod-env.png" width="96" alt="SOUND MOD ENV"> | SOUND | ENV | MOD ENV | `INIT PERF MUT UNDO RNDMZR` | AMT +0% · ATTACK 0MS · DECAY 300MS · TARGET CUTOFF |
| <img src="../screens/sound-tracking.png" width="96" alt="SOUND TRACKING"> | SOUND | ENV | TRACKING | `INIT PERF MUT UNDO RNDMZR` | V.CUT +0% · V.LVL 100% · KEYTRK 0% · V.ENV +0% |
| <img src="../screens/fx-distortion.png" width="96" alt="AUDIO FX DISTORTION"> | AUDIO FX | FX | DISTORTION | `PRE-DIST PING P M UNDO RNDMZR` | AMT 0% · MIX 100% · CHAR 35% · TONE 82% |
| <img src="../screens/fx-delay-send.png" width="96" alt="AUDIO FX DELAY SEND"> | AUDIO FX | FX | DELAY SEND | `PRE-DIST PING P M UNDO RNDMZR` | WET 12% · FDBK 32% · LO CUT 120HZ · HI CUT 72% |
| <img src="../screens/fx-delay-config.png" width="96" alt="AUDIO FX DELAY CONFIG"> | AUDIO FX | FX | DELAY CONFIG | `PRE-DIST PING P M UNDO RNDMZR` | SYNC ON · DIV 8N · TIME 250MS · LEVEL 75% |
| <img src="../screens/fx-reverb-room.png" width="96" alt="AUDIO FX REVERB ROOM"> | AUDIO FX | FX | REVERB ROOM | `PRE-DIST PING P M UNDO RNDMZR` | WET 24% · SIZE 70% · DECAY 6.0S · TONE 62% |
| <img src="../screens/fx-reverb-returns.png" width="96" alt="AUDIO FX REVERB RETURNS"> | AUDIO FX | FX | REVERB RETURNS | `PRE-DIST PING P M UNDO RNDMZR` | PRE 12MS · LEVEL 115% · D.LEVEL 90% · — |
| <img src="../screens/sample-main.png" width="96" alt="SAMPLE MAIN"> | SAMPLE | ENV | MAIN | `ON PERF MUT UNDO RNDMZR` | LEVEL 70% · DIST 0% · TRACK ON · LOOP OFF |
| <img src="../screens/sample-pitch.png" width="96" alt="SAMPLE PITCH"> | SAMPLE | ENV | PITCH | `ON PERF MUT UNDO RNDMZR` | COARSE +0 · FINE +0 · LEVEL 70% · DIST 0% |
| <img src="../screens/sample-trim.png" width="96" alt="SAMPLE TRIM"> | SAMPLE | ENV | TRIM | `ON PERF MUT UNDO RNDMZR` | START 0% · END 100% · TRACK ON · LOOP OFF |
| <img src="../screens/sample-envelope.png" width="96" alt="SAMPLE ENVELOPE"> | SAMPLE | ENV | ENVELOPE | `ON PERF MUT UNDO RNDMZR` | ATTACK 5MS · DECAY 120MS · SUSTAIN 70% · RELEASE 150MS |
| <img src="../screens/drums-voice.png" width="96" alt="DRUMS VOICE"> | DRUMS | ARP | VOICE | `ON VOX ACC ROLL P M UNDO RNDMZR` | TONE 25% · TUNE 40% · DECAY 335MS · LEVEL 90% |
| <img src="../screens/drums-step.png" width="96" alt="DRUMS STEP"> | DRUMS | ARP | STEP | `ON VOX ACC ROLL P M UNDO RNDMZR` | PITCH +0 ST · DECAY +0% · CRUSH 0% · RATCH 1 |
| <img src="../screens/drums-vary.png" width="96" alt="DRUMS VARY"> | DRUMS | ARP | VARY | `ON VOX ACC ROLL P M UNDO RNDMZR` | CHANCE 100% · CRUSH 0% · FEED OFF · ROLL 50% |
| <img src="../screens/drums-kit.png" width="96" alt="DRUMS KIT"> | DRUMS | ARP | KIT | `ON VOX ACC ROLL P M UNDO RNDMZR` | LENGTH 16 · FOLLOW OFF · — · LEVEL 70% |
| <img src="../screens/drums-dist.png" width="96" alt="DRUMS DIST"> | DRUMS | ARP | DIST | `ON VOX ACC ROLL P M UNDO RNDMZR` | DRIVE 0% · FOLD 0% · DUCK 0% · — |
| <img src="../screens/chord-play.png" width="96" alt="CHORD PLAY"> | CHORD | ARP | PLAY | `ON BASS POLY FIT P M UNDO RNDMZR` | MODE PATTERN · TRIG GRID · CHORDS 4 · GATE 72% |
| <img src="../screens/chord-chord.png" width="96" alt="CHORD CHORD"> | CHORD | ARP | CHORD | `ON BASS POLY FIT P M UNDO RNDMZR` | DEG 1 · SHAPE TRIAD · HOLD 8 · OCT -1 |
| <img src="../screens/chord-voicing.png" width="96" alt="CHORD VOICING"> | CHORD | ARP | VOICING | `ON BASS POLY FIT P M UNDO RNDMZR` | INV 0 · SPREAD 1 · VOICES 3 · VLEAD OFF |
| <img src="../screens/chord-grid.png" width="96" alt="CHORD GRID"> | CHORD | ARP | GRID | `ON BASS POLY FIT P M UNDO RNDMZR` | MODE HORIZ · LENGTH 16 · DEPTH 100% · FOLLOW OFF |
| <img src="../screens/chord-pattern.png" width="96" alt="CHORD PATTERN"> | CHORD | ARP | PATTERN | `ON BASS POLY FIT P M UNDO RNDMZR` | MASK 1 · LENGTH 8 · — · — |
| <img src="../screens/chord-filter.png" width="96" alt="CHORD FILTER"> | CHORD | ARP | FILTER | `ON BASS POLY FIT P M UNDO RNDMZR` | CUT 783.0HZ · RES 18% · ENV 22% · LEVEL 18% |
| <img src="../screens/chord-tone.png" width="96" alt="CHORD TONE"> | CHORD | ARP | TONE | `ON BASS POLY FIT P M UNDO RNDMZR` | WAVE SAW · — · LINK OFF · — |
| <img src="../screens/chord-motion.png" width="96" alt="CHORD MOTION"> | CHORD | ARP | MOTION | `ON BASS POLY FIT P M UNDO RNDMZR` | F.ATK 8MS · F.DEC 700MS · — · NEXT TRIG |
| <img src="../screens/chord-amp.png" width="96" alt="CHORD AMP"> | CHORD | ARP | AMP | `ON BASS POLY FIT P M UNDO RNDMZR` | ATTACK 40MS · DECAY 900MS · SUSTAIN 75% · RELEASE 900MS |
| <img src="../screens/chord-harmony.png" width="96" alt="CHORD HARMONY"> | CHORD | ARP | HARMONY | `ON BASS POLY FIT P M UNDO RNDMZR` | FOLLOW OFF · DRIFT 0% · DEPTH ALL · — |
| <img src="../screens/mix.png" width="96" alt="MIX LEVEL"> | MIX | GLO | LEVEL | `LD BS CH DR PERF MUT UNDO RNDMZR` | LEAD 100% · BASS 42% · CHORD 18% · DRUMS 70% |
| <img src="../screens/mix-delay.png" width="96" alt="MIX DELAY"> | MIX | GLO | DELAY | `LD BS CH DR PERF MUT UNDO RNDMZR` | LEAD 100% · BASS 0% · CHORD 60% · DRUMS 25% |
| <img src="../screens/mix-reverb.png" width="96" alt="MIX REVERB"> | MIX | GLO | REVERB | `LD BS CH DR PERF MUT UNDO RNDMZR` | LEAD 100% · BASS 20% · CHORD 60% · DRUMS 25% |

## White keys

| Page | A white key |
|---|---|
| HOME | Tap: the step on or off. Hold: the step's values on K1-K4, the first four black keys its flags. EDIT (KEYS): a keyboard, lit by the scale; REC cycles what it writes (SEQ, BASS, CHORD). |
| STEP | Picks the step (a tap toggles it too); EDIT on: a key sets its pitch. |
| SEGMENT | Cuts or joins a segment at that step; SEL held: picks the segment. |
| LOOP CYCLE | Picks a loop position (the first eight keys); off, auditions it. |
| SLOT SEQ | Picks a position. |
| RANDOM | A target on or off (MASK), + / - (LINKS), picks the target (AMOUNTS). |
| KEY / SCALE | The root. |
| SLOTS | Picks a slot. |
| DOT MATRIX | A cell on or off. |
| EVOLVE | Flips a bit and picks it. |
| BASS | A BASS SEQ step on or off, and picks it. |
| CHORD | Picks a chord of the chain (or a pattern step). |
| GLOBAL, SOUND, FX, MIX | Nothing. |
| SAMPLE | Picks a slot. |
| DRUMS | The picked voice's hit on or off at that step, and picks the step. |

