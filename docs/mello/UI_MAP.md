# UI map (generated 2026-10-31, from the code at b6ef874; values checked against the code 2026-11-04)

Every page and layer, rendered in host mode: the panel button that opens it, the strip (the black keys B0..B10) and
K1-K4's labels with their default values. The text rows are kept by hand with the code (the tests rotary_mapping_parity, control_authoritative_mapping and ui_glossary pin them); the pictures come from `make screens`.

| Screen | Page | Button | Layer | Strip B0..B10 | K1 · K2 · K3 · K4 |
|---|---|---|---|---|---|
| <img src="../screens/home.png" width="96" alt="HOME —"> | HOME | HOME | — | `- + RST CLR ROLL M UNDO RNDMZR P` | CUTOFF 3264HZ · DECAY 180MS · SPACE 24% · DRIVE 0% |
| <img src="../screens/perform.png" width="96" alt="PERFORM —"> | PERFORM | B10 PERF | — | `- + RST CLR ROLL M UNDO RNDMZR P` | the page's K1-K4; the white keys: FILL, ARP, REVERSE, ACCENT, REP 8, REP 4, REP 2, REP 1, LP SWP, HP SWP, THROW, WASH, LD OFF, BS OFF, CH OFF, DR OFF; the black keys B0-B7, held: STUT, TAPE, PLCK, CRSH, CHOP, OCT-, OCT+, WARP (K1 the held one's amount) |
| <img src="../screens/rndmzr-held.png" width="96" alt="RNDMZR held —"> | RNDMZR held | hold RNDMZR | — | `- + RST CLR ROLL M UNDO RNDMZR P` | the page's K1-K4; the white keys roll one part: LEAD, BASS, DRUMS, KIT, CHORDS, SOUND; B0-B5 lock or unlock them; SAVE keeps the band in the first free slot |
|  | — | UNDO held | hold UNDO | — | `- + RST CLR M UNDO RNDMZR P` | the white keys 1-3: the band before the last, second last and third last roll |
|  | — | REC held | hold REC | — | `- + RST CLR M UNDO RNDMZR P` | white keys 1-4: what a take records, SEQ, BASS, CHORD, DRUMS; B0: the click (and a bar's count-in); let go after half a second with neither: the take from empty |
| <img src="../screens/home-macros.png" width="96" alt="HOME MACROS"> | HOME | hold EDIT | MACROS | `- + RST CLR ROLL M UNDO RNDMZR P` | each knob picks what it turns (CUTOFF, RES, DECAY, ATTACK, RELEASE, SPACE, DRIVE, DELAY, FDBK, REVERB, SIZE, LFO HZ, LFO AMT, PWM, NOISE, DETUNE, OSC 2, FM, SAT, MODENV, SUB LVL, LEAD, BASS, CHORD, DRUMS, SWING) |
| <img src="../screens/step-trig.png" width="96" alt="STEP TRIG"> | STEP | SEQ | TRIG | `SKIP ROLL M UNDO RNDMZR P` | PITCH +0 · PULSE 1 · RATCH 1 · GATE SINGLE |
| <img src="../screens/step-note.png" width="96" alt="STEP NOTE"> | STEP | SEQ | NOTE | `SKIP ROLL M UNDO RNDMZR P` | OCT +0 · PROB 100% · SAMP 1 · WAVE SND |
| <img src="../screens/step-flags.png" width="96" alt="STEP FLAGS"> | STEP | SEQ | FLAGS | `SKIP ROLL M UNDO RNDMZR P` | ACCENT OFF · SUB OFF · SLIDE OFF · SKIP OFF |
| <img src="../screens/step-seq.png" width="96" alt="STEP SEQ"> | STEP | SEQ | SEQ | `SKIP ROLL M UNDO RNDMZR P` | LENGTH 16 · RESET 2 BAR · DIV 1 · SPAN 2 |
| <img src="../screens/segment.png" width="96" alt="SEGMENT SEGMENT"> | SEGMENT | SEQ | SEGMENT | `EVEN A B C D ROLL M UNDO RNDMZR P` | REPEAT 1 · ORDER FWD · SKIP OFF · LENGTH 4 |
| <img src="../screens/segment.png" width="96" alt="SEGMENT (2)"> | SEGMENT | SEQ | (2) | `EVEN A B C D ROLL M UNDO RNDMZR P` | REPEAT 1 · ORDER FWD · SKIP OFF · LENGTH 4 |
| <img src="../screens/loop-step.png" width="96" alt="LOOP CYCLE STEP"> | LOOP CYCLE | SEQ | STEP | `ON RST ROLL M UNDO RNDMZR P` | HOLD 16 · ROOT+ +0 · OCT +0 · TRANS +0 |
| <img src="../screens/loop-harm.png" width="96" alt="LOOP CYCLE HARM"> | LOOP CYCLE | SEQ | HARM | `ON RST ROLL M UNDO RNDMZR P` | KEY FOLLOW · SCALE FOLLOW · SUB 0 · BASS TR +0 |
| <img src="../screens/loop-loop.png" width="96" alt="LOOP CYCLE LOOP"> | LOOP CYCLE | SEQ | LOOP | `ON RST ROLL M UNDO RNDMZR P` | LENGTH 4 · SYNC SYNC · ON OFF · — |
| <img src="../screens/slotseq.png" width="96" alt="SLOT SEQ —"> | SLOT SEQ | SEQ | — | `ON ROLL M UNDO RNDMZR P` | SLOT OFF · HOLD 16 · ON OFF · LENGTH 16 |
| <img src="../screens/random-mask.png" width="96" alt="RANDOM MASK"> | RANDOM | hold RNDMZR | MASK | `NONE ALL CLR MUT UNDO RNDMZR PERF` | INTENS MED · STYLE ANY · ROLLS ALL · — |
| <img src="../screens/random-links.png" width="96" alt="RANDOM LINKS"> | RANDOM | hold RNDMZR | LINKS | `NONE ALL CLR MUT UNDO RNDMZR PERF` | INTENS MED · STYLE ANY · ROLLS ALL · — |
| <img src="../screens/random-amounts.png" width="96" alt="RANDOM AMOUNTS"> | RANDOM | hold RNDMZR | AMOUNTS | `NONE ALL CLR MUT UNDO RNDMZR PERF` | AMOUNT PIT 10~ · — · — · — |
| <img src="../screens/key-scale.png" width="96" alt="KEY / SCALE —"> | KEY / SCALE | SEL | — | `MUT UNDO RNDMZR PERF` | ROOT C4 · SCALE minor · OCT +0 · SPAN 2 |
| <img src="../screens/slots-load.png" width="96" alt="SLOTS LOAD"> | SLOTS | hold SAVE | LOAD | `MUT UNDO RNDMZR PERF` | — · VISIBLE 8 · — · — |
| <img src="../screens/slots-rename.png" width="96" alt="SLOTS RENAME"> | SLOTS | hold SAVE | RENAME | `MUT UNDO RNDMZR PERF` | CURSOR 1 · CHAR - · — · — |
| <img src="../screens/dot-accent.png" width="96" alt="DOT MATRIX ACCENT"> | DOT MATRIX | LFO | ACCENT | `ON RST FLW REV ROLL M UNDO RNDMZR P` | MODE HORIZ · LENGTH 16 · CHANCE 100% · EVOLVE 0% |
| <img src="../screens/dot-mod-1.png" width="96" alt="DOT MATRIX MOD 1"> | DOT MATRIX | LFO | MOD 1 | `ON RST FLW REV ROLL M UNDO RNDMZR P` | MODE HORIZ · LENGTH 16 · DEPTH 24% · EVOLVE 0% |
| <img src="../screens/dot-mod-2.png" width="96" alt="DOT MATRIX MOD 2"> | DOT MATRIX | LFO | MOD 2 | `ON RST FLW REV ROLL M UNDO RNDMZR P` | MODE HORIZ · LENGTH 16 · CHANCE 100% · EVOLVE 0% |
| <img src="../screens/dot-ratchet.png" width="96" alt="DOT MATRIX RATCHET"> | DOT MATRIX | LFO | RATCHET | `ON RST FLW REV ROLL M UNDO RNDMZR P` | MODE HORIZ · LENGTH 16 · CHANCE 100% · EVOLVE 0% |
| <img src="../screens/dot-sub.png" width="96" alt="DOT MATRIX SUB"> | DOT MATRIX | LFO | SUB | `ON RST FLW REV ROLL M UNDO RNDMZR P` | MODE HORIZ · LENGTH 16 · DEPTH 24% · EVOLVE 0% |
| <img src="../screens/evolve-main.png" width="96" alt="EVOLVE MAIN"> | EVOLVE | LFO | MAIN | `ON LOOP ROLL M UNDO RNDMZR P` | MUTATE 10% · LENGTH 8 · RATE 1X · OFFSET +0 |
| <img src="../screens/evolve-routes.png" width="96" alt="EVOLVE ROUTES"> | EVOLVE | LFO | ROUTES | `ON LOOP ROLL M UNDO RNDMZR P` | GATE LENGTH · FILTER OFF · OCT OFF · DECAY OFF |
| <img src="../screens/evolve-depth.png" width="96" alt="EVOLVE DEPTH"> | EVOLVE | LFO | DEPTH | `ON LOOP ROLL M UNDO RNDMZR P` | GATE 35% · FILTER 35% · OCT 35% · DECAY 35% |
| <img src="../screens/evolve-sound.png" width="96" alt="EVOLVE SOUND"> | EVOLVE | LFO | SOUND | `ON LOOP ROLL M UNDO RNDMZR P` | DEST A OFF · DEPTH A +50% · DEST B OFF · DEPTH B +50% |
| <img src="../screens/bass-play.png" width="96" alt="BASS PLAY"> | BASS | ARP | PLAY | `ON SEQ CHD ROLL M UNDO RNDMZR P` | MODE STEP · PULSES 6 · ROTATE 0 · OCT -1 |
| <img src="../screens/bass-seq.png" width="96" alt="BASS SEQ"> | BASS | ARP | SEQ | `ON SEQ CHD ROLL M UNDO RNDMZR P` | SEQ OFF · LENGTH 16 · CLOCK OWN · GATE 90% |
| <img src="../screens/bass-step.png" width="96" alt="BASS STEP"> | BASS | ARP | STEP | `ON SEQ CHD ROLL M UNDO RNDMZR P` | TONE ROOT · OCT +0 · GLIDE OFF · ACCENT ON |
| <img src="../screens/bass-filter.png" width="96" alt="BASS FILTER"> | BASS | ARP | FILTER | `ON SEQ CHD ROLL M UNDO RNDMZR P` | CUT 242.0HZ · RES 0% · ENV 0% · LEVEL 75% |
| <img src="../screens/bass-voice.png" width="96" alt="BASS VOICE"> | BASS | ARP | VOICE | `ON SEQ CHD ROLL M UNDO RNDMZR P` | WAVE SINE · DECAY 160MS · RELEASE 120MS · DRIVE 0% |
| <img src="../screens/bass-tone.png" width="96" alt="BASS TONE"> | BASS | ARP | TONE | `ON SEQ CHD ROLL M UNDO RNDMZR P` | GLIDE 20MS · OCT -1 · F.DEC 300MS · ACCENT 50% |
| <img src="../screens/global-clock.png" width="96" alt="GLOBAL CLOCK"> | GLOBAL | GLO | CLOCK | `TAP RST2 MUT UNDO RNDMZR PERF` | BPM 80 · DIV 1 · SYNC OFF · NOTEOUT OFF |
| <img src="../screens/global-midi.png" width="96" alt="GLOBAL MIDI"> | GLOBAL | GLO | MIDI | `TAP RST2 MUT UNDO RNDMZR PERF` | LEAD 1 · SUB 2 · NOTE STEP · LENGTH 180MS |
| <img src="../screens/global-groove.png" width="96" alt="GLOBAL GROOVE"> | GLOBAL | GLO | GROOVE | `TAP RST2 MUT UNDO RNDMZR PERF` | SWING 0% · LEAD GLOBAL · BASS GLOBAL · DRUMS GLOBAL |
| <img src="../screens/global-system.png" width="96" alt="GLOBAL SYSTEM"> | GLOBAL | GLO | SYSTEM | `TAP RST2 MUT UNDO RNDMZR PERF` | CORE 2 OFF · SEED AUTO · CPU nn% · CPU 2 nn% |
| <img src="../screens/sound-filter.png" width="96" alt="SOUND FILTER"> | SOUND | ENV | FILTER | `INIT ROLL M UNDO RNDMZR P` | CUT 3264HZ · RES 20% · DECAY 180MS · LEVEL 100% |
| <img src="../screens/sound-osc.png" width="96" alt="SOUND OSC"> | SOUND | ENV | OSC | `INIT ROLL M UNDO RNDMZR P` | WAVE SAW · PWM 50% · SLIDE 50MS · MASTER 75% |
| <img src="../screens/sound-amp-env.png" width="96" alt="SOUND AMP ENV"> | SOUND | ENV | AMP ENV | `INIT ROLL M UNDO RNDMZR P` | ATTACK 5MS · DECAY 180MS · SUSTAIN 40% · RELEASE 120MS |
| <img src="../screens/sound-lfo.png" width="96" alt="SOUND LFO"> | SOUND | ENV | LFO | `INIT ROLL M UNDO RNDMZR P` | LFO OFF · RATE 2.00HZ · DEPTH 35% · TARGET FILTER |
| <img src="../screens/sound-follower.png" width="96" alt="SOUND FOLLOWER"> | SOUND | ENV | FOLLOWER | `INIT ROLL M UNDO RNDMZR P` | SUBOSC ON · LEVEL 22% · OCT -1 · WAVE SINE |
| <img src="../screens/sound-grit.png" width="96" alt="SOUND GRIT"> | SOUND | ENV | GRIT | `INIT ROLL M UNDO RNDMZR P` | NOISE 0% · N.FM 0% · UNISON 1 · DETUNE 0 CT |
| <img src="../screens/sound-osc-2.png" width="96" alt="SOUND OSC 2"> | SOUND | ENV | OSC 2 | `INIT ROLL M UNDO RNDMZR P` | WAVE OFF · SEMI +0 ST · FINE +7 CT · MIX 0% |
| <img src="../screens/sound-character.png" width="96" alt="SOUND CHARACTER"> | SOUND | ENV | CHARACTER | `INIT ROLL M UNDO RNDMZR P` | SYNC OFF · FM 0% · SAT 0% · FILTER LP |
| <img src="../screens/sound-mod-env.png" width="96" alt="SOUND MOD ENV"> | SOUND | ENV | MOD ENV | `INIT ROLL M UNDO RNDMZR P` | AMT +0% · ATTACK 0MS · DECAY 300MS · TARGET CUTOFF |
| <img src="../screens/sound-tracking.png" width="96" alt="SOUND TRACKING"> | SOUND | ENV | TRACKING | `INIT ROLL M UNDO RNDMZR P` | V.CUT +0% · V.LVL 100% · KEYTRK 0% · V.ENV +0% |
| <img src="../screens/fx-distortion.png" width="96" alt="AUDIO FX DISTORTION"> | AUDIO FX | FX | DISTORTION | `PRE-DIST PING M UNDO RNDMZR P` | AMT 0% · MIX 100% · CHAR 35% · TONE 82% |
| <img src="../screens/fx-delay-send.png" width="96" alt="AUDIO FX DELAY SEND"> | AUDIO FX | FX | DELAY SEND | `PRE-DIST PING M UNDO RNDMZR P` | WET 12% · FDBK 32% · LO CUT 120HZ · HI CUT 72% |
| <img src="../screens/fx-delay-config.png" width="96" alt="AUDIO FX DELAY CONFIG"> | AUDIO FX | FX | DELAY CONFIG | `PRE-DIST PING M UNDO RNDMZR P` | SYNC ON · DIV 8N · TIME 250MS · LEVEL 75% |
| <img src="../screens/fx-delay-loop.png" width="96" alt="AUDIO FX DELAY LOOP"> | AUDIO FX | FX | DELAY LOOP | `PRE-DIST PING M UNDO RNDMZR P` | GLIDE OFF · PITCH OFF · DIFFUSE 0% · FREEZE OFF |
| <img src="../screens/fx-delay-route.png" width="96" alt="AUDIO FX DELAY ROUTE"> | AUDIO FX | FX | DELAY ROUTE | `PRE-DIST PING M UNDO RNDMZR P` | REVERB 0% · - · - · - |
| <img src="../screens/fx-reverb-room.png" width="96" alt="AUDIO FX REVERB ROOM"> | AUDIO FX | FX | REVERB ROOM | `PRE-DIST PING M UNDO RNDMZR P` | WET 24% · SIZE 70% · DECAY 2.5S · TONE 62% |
| <img src="../screens/fx-reverb-returns.png" width="96" alt="AUDIO FX REVERB RETURNS"> | AUDIO FX | FX | REVERB RETURNS | `PRE-DIST PING M UNDO RNDMZR P` | PRE 12MS · LEVEL 115% · D.LEVEL 90% · TRIM 0 DB |
| <img src="../screens/fx-reverb-oliverb.png" width="96" alt="AUDIO FX REVERB OLIVERB"> | AUDIO FX | FX | REVERB OLIVERB | `PRE-DIST PING M UNDO RNDMZR P` | DIFFUSE 65% · SWING 10% · RATE 50% · SHIMMER OFF |
| <img src="../screens/sample-main.png" width="96" alt="SAMPLE MAIN"> | SAMPLE | ENV | MAIN | `ON MUT UNDO RNDMZR PERF` | LEVEL 70% · DIST 0% · TRACK ON · LOOP OFF |
| <img src="../screens/sample-pitch.png" width="96" alt="SAMPLE PITCH"> | SAMPLE | ENV | PITCH | `ON MUT UNDO RNDMZR PERF` | COARSE +0 · FINE +0 · LEVEL 70% · DIST 0% |
| <img src="../screens/sample-trim.png" width="96" alt="SAMPLE TRIM"> | SAMPLE | ENV | TRIM | `ON MUT UNDO RNDMZR PERF` | START 0% · END 100% · TRACK ON · LOOP OFF |
| <img src="../screens/sample-envelope.png" width="96" alt="SAMPLE ENVELOPE"> | SAMPLE | ENV | ENVELOPE | `ON MUT UNDO RNDMZR PERF` | ATTACK 5MS · DECAY 120MS · SUSTAIN 70% · RELEASE 150MS |
| <img src="../screens/drums-voice.png" width="96" alt="DRUMS VOICE"> | DRUMS | ARP | VOICE | `ON VOX ACC ROLL M UNDO RNDMZR P` | TONE 25% · TUNE 40% · DECAY 335MS · LEVEL 90% |
| <img src="../screens/drums-step.png" width="96" alt="DRUMS STEP"> | DRUMS | ARP | STEP | `ON VOX ACC ROLL M UNDO RNDMZR P` | PITCH +0 ST · DECAY +0% · CRUSH 0% · RATCH 1 |
| <img src="../screens/drums-vary.png" width="96" alt="DRUMS VARY"> | DRUMS | ARP | VARY | `ON VOX ACC ROLL M UNDO RNDMZR P` | CHANCE 100% · CRUSH 0% · FEED OFF · ROLL 50% |
| <img src="../screens/drums-kit.png" width="96" alt="DRUMS KIT"> | DRUMS | ARP | KIT | `ON VOX ACC ROLL M UNDO RNDMZR P` | LENGTH 16 · CLOCK OWN · — · LEVEL 95% |
| <img src="../screens/drums-dist.png" width="96" alt="DRUMS DIST"> | DRUMS | ARP | DIST | `ON VOX ACC ROLL M UNDO RNDMZR P` | DRIVE 0% · FOLD 0% · DUCK 0% · — |
| <img src="../screens/drums-line.png" width="96" alt="DRUMS LINE"> | DRUMS | ARP | LINE | `ON VOX ACC ROLL M UNDO RNDMZR P` | V.LEN KIT · DIR FWD · RESTART FREE · — |
| <img src="../screens/chord-play.png" width="96" alt="CHORD PLAY"> | CHORD | ARP | PLAY | `ON BASS POLY FIT RL M UNDO RNDMZR P` | MODE BROKEN · TRIG STEP · CHORDS 1 · GATE 72% |
| <img src="../screens/chord-chord.png" width="96" alt="CHORD CHORD"> | CHORD | ARP | CHORD | `ON BASS POLY FIT RL M UNDO RNDMZR P` | DEG 1 · SHAPE TRIAD · HOLD 8 · OCT -1 |
| <img src="../screens/chord-voicing.png" width="96" alt="CHORD VOICING"> | CHORD | ARP | VOICING | `ON BASS POLY FIT RL M UNDO RNDMZR P` | INV 0 · SPREAD 1 · VOICES 3 · VLEAD OFF |
| <img src="../screens/chord-grid.png" width="96" alt="CHORD GRID"> | CHORD | ARP | GRID | `ON BASS POLY FIT RL M UNDO RNDMZR P` | MODE HORIZ · LENGTH 16 · DEPTH 100% · FOLLOW OFF |
| <img src="../screens/chord-pattern.png" width="96" alt="CHORD PATTERN"> | CHORD | ARP | PATTERN | `ON BASS POLY FIT RL M UNDO RNDMZR P` | MASK 1 · LENGTH 8 · — · — |
| <img src="../screens/chord-filter.png" width="96" alt="CHORD FILTER"> | CHORD | ARP | FILTER | `ON BASS POLY FIT RL M UNDO RNDMZR P` | CUT 783.0HZ · RES 18% · ENV 22% · LEVEL 13% |
| <img src="../screens/chord-tone.png" width="96" alt="CHORD TONE"> | CHORD | ARP | TONE | `ON BASS POLY FIT RL M UNDO RNDMZR P` | WAVE SAW · — · LINK OFF · — |
| <img src="../screens/chord-motion.png" width="96" alt="CHORD MOTION"> | CHORD | ARP | MOTION | `ON BASS POLY FIT RL M UNDO RNDMZR P` | F.ATK 8MS · F.DEC 700MS · — · NEXT STEPS |
| <img src="../screens/chord-amp.png" width="96" alt="CHORD AMP"> | CHORD | ARP | AMP | `ON BASS POLY FIT RL M UNDO RNDMZR P` | ATTACK 40MS · DECAY 900MS · SUSTAIN 75% · RELEASE 900MS |
| <img src="../screens/chord-harmony.png" width="96" alt="CHORD HARMONY"> | CHORD | ARP | HARMONY | `ON BASS POLY FIT RL M UNDO RNDMZR P` | FOLLOW OFF · DRIFT 0% · DEPTH ALL · — |
| <img src="../screens/mix.png" width="96" alt="MIX LEVEL"> | MIX | GLO | LEVEL | `LD BS CH DR MUT UNDO RNDMZR PERF` | LEAD 100% · BASS 75% · CHORD 13% · DRUMS 95% |
| <img src="../screens/mix-delay.png" width="96" alt="MIX DELAY"> | MIX | GLO | DELAY | `LD BS CH DR MUT UNDO RNDMZR PERF` | LEAD 100% · BASS 0% · CHORD 60% · DRUMS 25% |
| <img src="../screens/mix-reverb.png" width="96" alt="MIX REVERB"> | MIX | GLO | REVERB | `LD BS CH DR MUT UNDO RNDMZR PERF` | LEAD 100% · BASS 20% · CHORD 60% · DRUMS 25% |

## White keys

| Page | A white key |
|---|---|
| HOME | Tap: the step on or off. Hold: the step's values on K1-K4, the first four black keys its flags. EDIT (KEYS): a keyboard, lit by the scale; REC cycles what it writes (SEQ, BASS, CHORD). |
| STEP | Picks the step (a tap toggles it too); EDIT on: a key sets its pitch. |
| SEGMENT | Cuts or joins a segment at that step; SEL held: picks the segment. |
| LOOP CYCLE | Picks a loop position (the first eight keys); off, auditions it. |
| SLOT SEQ | Picks a position. |
| RANDOM | A target on or off (MASK; the key after them is BAND: off, a tap of RNDMZR rolls the lead alone), + / - (LINKS), picks the target (AMOUNTS). |
| KEY / SCALE | The root. |
| SLOTS | Picks a slot. |
| DOT MATRIX | A cell on or off. |
| EVOLVE | Flips a bit and picks it. |
| BASS | A BASS SEQ step on or off, and picks it. |
| CHORD | Picks a chord of the chain (or a pattern step). |
| GLOBAL, SOUND, FX, MIX | Nothing. |
| SAMPLE | Picks a slot. |
| DRUMS | The picked voice's hit on or off at that step, and picks the step. |

