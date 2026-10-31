# Roadmap

From the October 2026 review ([docs/review-2026-10/REVIEW.md](docs/review-2026-10/REVIEW.md)): keep the model,
sequencer and storage; rewrite the sound to match the HTML, rebuild HOME as an instrument. One phase at a time.

## Now: one thread, and room to run (2026-10-31)

The 0.5 feedback round added a lot fast: drums, BASS SEQ, MIX, chords' NEXT, glitch, swing. Two weaknesses came with it.
There is no common thread in navigation, keys and design. And the room is gone: RAM 800 bytes free, a full storage
bank that silently stops saving, CPU never measured on the chip since 0.5-f3. In this order:

**A. What can lose work (first)**
- [x] A1. Storage: 64 full slots with their grit and MOD entries need 42.4 KB, more than the 40 KB bank. A save is then
      refused (SAVE_TOO_LARGE) and nothing on the screen says so. Done 2026-10-31: the flash map had 48 KB unused
      between the banks and the sample staging area, so the banks are 64 KB (0x98000..0xB7FFF); a save that still
      does not fit says FULL in red where SAVED stands; the old layout's record is read at boot and the first save
      after an upgrade goes under it; a save erases only the sectors it needs. storage_full_slots: 64 slots back
      after a power cycle. DECISIONS.md "Room to run".

**B. Measure, then make room**
- [x] B1. CPU on the chip's model again (fm1-emu): done 2026-10-31 with tools/emu_cpu.sh (two runs, silence then
      PLAY, so the splash does not dilute it) and mello_engine_stress, the worst case (everything on at once).
      0.5-f19, playing: the default pattern 77 M instructions/s = 32% of the chip, the worst case 179 M/s = 75%.
      In silence the interrupts took 60 M/s more: 55 of them were the render running through the silence (its
      early return compared two arrays' addresses, never equal; fixed in f24, silence costs ~5 M/s = 2%: the
      timer with the key scan, 2.5 in silence and ~12 while playing with LEDs lit). The emulator
      counts instructions, not cycles: a divide is one there. The 60% budget stands for the worst case; it is not
      met on instruction count, and the chip's own cycles are read on GLOBAL now (CPU nn%, B5). The main loop spins
      in the emulator (its idle instruction does not sleep there), so its share says nothing.
- [x] B2. Where the audio's time goes: make bench (tests/bench/cpu_bench.c), the worst case with one part out at a
      time, on this computer: the drums 21%, the lead's grit and OSC 2 13% (unison 6, OSC 2 5), the chords 13%
      (chorus 2), the sub with BASS SEQ 5%, the MOD lanes 3%, the LFO 2%; reverb, delay and distortion cost
      little here. On the chip: mello_audio_render had 233 hardware divides, some 70 a sample (a divide by a
      constant is a divide for this toolchain); its 64-bit multiply is one instruction, no libcall.
- [x] B3. Done 2026-10-31 for what B2 showed: the per-sample path has no divide (Q12 gains, Q15 depths, worked out
      when the config or the note changes), the reverb runs on its own tail only, the drums' modulation is read
      every 16 samples. Renders: the parity table within 0.1 dB. Left, in order of worth: the modulation block at
      a control rate (every 4 samples: the LFO, the cutoff's sum, the main filter's coefficients under the LFO),
      the key scan's LED pulses (fm1_input_tick, a HAL file: ~12 M/s while playing).
- [x] B4. RAM, 2026-10-31: 800 B free -> 7456 B. The table (tests/bench, sizes): the engine 60 KB, of it the 64
      slots 49.7 KB, its event queue 3.8 KB, UNDO's copies 1.2 KB; the audio 21.9 KB -> 16.3 (its event queue 8 KB
      -> 4, the chorus line 2 KB -> 0.5); dinghy.c's synced copies and default project 1.2 KB -> 0. Left: the stack's
      high-water mark (painted stack, read in fm1-emu); the 60 B MelloEvent (both queues); UNDO as one packed slot.
- [x] B5. Guards, 2026-10-31: the build fails under 2 KiB of free RAM (tools/build.py); tools/emu_cpu.sh reports a
      package's load; GLOBAL shows the chip's measured audio load (CPU nn%: the worst block of each second).

**C. One thread through navigation, keys and design**
- [x] C1. A map of all 18 pages: panel button, section, pages, layers (hidden ones marked); what B0-B10 and the white
      keys do on each; the words and units each value uses. The deviations marked. Built from the code's own tables,
      as a page to review. docs/mello/UI_MAP.md (generated), the review page with the deviations (A-L) and the
      proposal (R1-R7) handed over 2026-10-31.
- [x] C2. Rules, decided by the product owner 2026-10-31 (the proposal as revised for the printed panel). The frame: the M-VAVE FM-1's panel is printed (OCT-,
      OCT+, FX, SEL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY, REC; SELECT, ALGORITHM, PRESETS, K1-K4) and
      the firmware cannot rename a button, so the printed name is the section's name. Proposals:
      - the printed button is the section (SEQ, ENV, LFO, FX, ARP, SEL, GLO) and pressing it again goes to the
        section's next page; ALGORITHM is the page, SELECT is always the layer; no hidden layers (dimmed instead);
        the header shows BUTTON > PAGE > LAYER; OCT-/OCT+ are the octave on every page; held keys open the deep
        pages (RNDMZR, SAVE).
      - B0 is ON for the page's part wherever there is one; B9 UNDO and B10 RNDMZR stay; the keys between in a fixed
        order (pick, then toggles, then actions).
      - white keys: one meaning on every page (for example: tap toggles a step, EDIT + tap picks it).
      - one word per thing (LENGTH, DECAY, ACCENT ..., a glossary), levels in %, times in ms or s, pitch in semitones,
        unused knobs blank; one strip style.
      - one body grammar: the part's picture on top, its four values below as sliders, the section's colour.
- [x] C3. Applied 2026-10-31 in six commits (R5 words and units, R2 the header, R1 the sections, R3 the black keys,
      R4 / R6 the white keys, SEL and EDIT); locked in ui_glossary (words, units) and the control contracts (every
      page's strip, the sections' order, the encoders' roles). The body grammar (R5's "one body") 2026-10-31, in
      part: every parameter layer (SOUND, FX, GLOBAL, the ENV panel) is the part's picture on top, drawn from its
      values, and its four values as rows below; the pictures: the wave and its filter, the LFO's swing, the
      follower's wave under the lead's, master and lead, grit's grains, OSC 2 over OSC 1, the filter's mode and the
      saturation, the MOD envelope, key tracking, the distortion's curve, the echoes, the tail, the clock's ticks
      (with the measured CPU load), the groove. Then the rest: every page's four knobs as the same one-line rows
      under its own picture (STEP, SEGMENT, LOOP, SLOT SEQ, SLOTS, KEY, BASS SEQ, CHORD, DRUMS, SAMPLE); the 2 x 2
      of values is gone. HOME, MIX, DOT, EVOLVE and RANDOM keep their own bodies (DECISIONS.md "One body").
- [ ] C4. A short hand checklist on the FM-1 (with the first flash, below).

Order: A1, B1, C1 (and C2's decisions), then B2-B4 and C3 in turns, B5, first flash and C4.

## Earlier: feedback from playing 0.5 (2026-10-30)

In the order to do them; each is the next work before Phase 3 / 5's open items.
- [x] **Playback stops by itself and the last note hangs for ever**: a HOLD step over several pulses sent its
      NOTE_OFF tagged with its last pulse, and mello_audio (which releases a note only on its own OFF, for legato)
      ignored it, so the note sounded until another came: over rests, for ever, as if playback had stopped. The OFF
      now carries the ON's pulse; a HOLD that REVERSE / REPEAT or an edit would leave open is ended
      (hold_release_regression).
- [x] **A modulation nobody set** (heard in the web emulator, not in offline renders of the same engine): the
      emulator could not keep up. HOME's performance keys (0.5-k1) searched the black keys' notes on every input:
      the main loop ran 116 M instructions an emulated second against 0.4-h6's 70 M (52% idle, h6 71%), and the
      emulator's audio broke up into what sounded like ratchets and modulation. A table now (0.5-f3: 70% idle).
      OSC 2 is OFF by default.
- [x] **The randomizer ruins the sound and there is no way back**: by default RNDMZR rolls the first nine targets
      (PIT .. SKP), not WAVE / CHORD / SMP / PRB (a roll left the saw in only 20% of cases, square / pulse 4-6 dB
      louder); SOUND's INIT (press twice) restores the default sound, and UNDO undoes it (ui_sound_init). A project
      saved earlier keeps its mask: turn WAVE off on the RANDOM page.
- [x] **Randomizer EDIT**: on the RANDOM page, EDIT sets how much each target is rolled: a key picks the target, K1
      its amount, the chance a roll changes a step (0-100%, 100 by default = as before); drawn as a bar under each
      pill (random_amounts).
- [x] **Noise too strong**: NOISE's 100% is the first version's 30% (added noise -21 / -15 / -9 dB under the saw at
      25 / 50 / 100%), NOISE FM's 100% the first version's 15%; the noise has a low cut (two one-pole highpasses at
      ~450 Hz, 12 dB an octave): nothing of it under 200 Hz.
- [x] **SUB: a DRONE mode**: SUB's MODE has DRONE after TM: one sub note from START to STOP at its full LEVEL, through
      rests and skips; KEY, OCT and the 5SEQ's tones move it (GLIDE), SUB MUTE fades it out and back (sub_drone).
- [x] **Segments are confusing and hard to adjust**: on SEGMENT a step key cuts (a segment starts there) or joins (where
      one starts already); B0-B5 pick A-F, B7 EVEN spreads them; one layer: REPEAT, ORDER (FWD / REV / P-P / RND), SKIP,
      LENGTH (the ones after keep theirs). A new LEN keeps their proportions. A new project has 4 x 4, once forward each:
      it plays as one of 16 (segment_edit).

### More feedback (2026-10-30, evening)

- [x] **UNDO / REDO**: the key toggles: UNDO, then REDO, back to UNDO when an edit leaves nothing to redo; its label says
      which it will do. Done: REDO on the key after an UNDO; an edit then leaves nothing to redo (the key dims until
      the next roll); an edit after a roll keeps UNDO (undo_redo).
- [x] **Reverb crackles in the highs.** Found: the reverb runs at half rate, and went in by averaging two samples and out
      by linear interpolation: a 15 kHz partial (hats, CRUSH, noise, distortion) reached it only 6.8 dB down, folded
      to an inharmonic ~7 kHz the tone filter cannot take out, and its output imaged above 11 kHz. Now a 31-tap
      half-band filter each way: 64.9 dB down, and the return above 16 kHz -62 dB (was -59, -23 before the tone
      filter). No clipping in the network even with the drums and distortion full. To confirm by ear.
- [x] **EVOLVE longer than 8 steps**: the page shows 8 lanes, windowed on the selected step, so the playing one goes
      out of sight. Show the whole register (up to 32), the one playing always in view. Done: a lane a step across the
      width (7 px each at 32), the playing one framed white, step numbers every fourth where the lanes are narrow.
- [x] **Delay: LO CUT and HI CUT** on its return, to shape it (ask: "pre dist"). Done: DELAY is WET, FDBK, LO CUT
      (12 dB an octave, 20 Hz .. 2 kHz, on the return and in the feedback, so each repeat is thinner; OFF by default),
      HI CUT (the HTML's TONE); LEVEL moved to DELAY CONFIG, D.LEVEL to RETURNS. PRE-DIST (FX's B0, off by
      default): the delay before the distortion, so its filtered echoes are driven too.
- [x] **The black keys' strip**: use the whole width, each label over its key, so it is plain which key does what.
      Done: eleven cells across the width in the keys' order, the keyboard's groups apart, a free key a short dim cell,
      the room left shared out; where there is no room for frames (HOME, SEGMENT, DRUMS), plain tinted cells a pixel
      apart; on = filled.
- [x] **RANDOMIZE WAVE per step**: with WAVE in the mask a roll gives each step its own wave (never OFF).
      STEP's NOTE layer shows and sets it (K4 WAVE; DECISIONS.md "RANDOM: WAVE per step").
- [x] **The bass sequencer: 16 steps** instead of 5-SEQ: length 1-16, per step tone, octave, glide and more; its own
      clock, not stopped by the main sequence's skipped or held steps unless asked to follow them. BASS SEQ on the SUB
      page (DECISIONS.md "BASS SEQ").
- [x] **More delay times**: DIV is now 16T, 16N, 8T, 8N, 4T, 8D, 4N, 4D, 2N (shortest first; the stored numbers keep
      the HTML's four).
- [x] **A MUSICAL randomizer**: as now, and a setting to roll musically instead of at random (rhythmic, in phrase),
      so fewer rolls are needed before tweaking. INT's fourth step, MUSICAL (DECISIONS.md "RANDOM: MUSICAL").
- [x] **More parameters for the bass synth**, if there are any under the hood: RES, ENV, E.DEC and ACCENT (SUB's
      FILTER layer).
- [x] **A simple 16-step drum machine**: minimal, classic voices, "Aphex Twin meets Richard Divine": the sonic
      differences come from per-step modulation and the randomizer. Its own clock as the bass's.
      Done (DECISIONS.md "Drums"): DRUMS after SUB and CHORD on BASS; KICK, SNARE, HAT, GLITCH; per step PITCH, DECAY,
      CRUSH, RATCH and accent; CHANCE, SWING, ROLL, FEED (RNDMZR rolls them too); a step every pulse, or with FOLLOW
      the sequence's steps (drum_machine). Since then: MIDI out for the drums (GM notes on channel 10) and the LFO,
      DOT and EVOLVE routes to them (D.TUNE, D.DECAY, D.CRUSH, D.DRIVE; drum_routes).

## Phase 0: clean ground (done in this repo)

- [x] Stop WP-E3 (Reverb V3). It tuned towards a target the HTML never had.
- [x] Plain source on a Dinghy fork, no base64/xz layers or build-time patch scripts.
- [x] Mello host tests and Dinghy host mode run with `make test` / `make host`, on Linux and macOS.
- [x] clang-format, with compiled code proven unchanged.
- [x] One CI workflow through `zvenson/fm1-build`.
- [x] First target build from this tree, on a Mac with Colima: `./build.sh --release 0.1-dev` gives a package
      byte-identical to the A.1 package (SHA-256 86a4dbec…b813), identity FM-1_970, budgets pass.
- [ ] Carry over the checks that only the old `ci/build_target.sh` ran: the RAM/pool budget report
      (`inspect_target_build.py`) and the 80/100 MHz emulator smokes.

## Phase 1: safe on hardware

Each fix has a regression test in `tests/mello/` that failed before it; `make test` and `make sanitize` pass.

- [x] P0: ladder filter and LFO->AMP `q15mul` overflow latched -21903 DC (`audio_overflow_regression`).
- [x] P1: reverb DC kept the tail guard armed, so autosave, SAVE and sample upload never ran
      (`audio_silence_regression`).
- [x] P1: step, sound and FX edits were never marked dirty; storage now detects any change itself
      (`storage_autosave_regression`).
- [x] P1: a firmware rollback could erase the newer firmware's project (`storage_rollback_regression`).
- [x] P1: Chord Machine settings were not saved; new section SEC_CHORD (`storage_chord_regression`).
- [x] P1: sample decoding in the audio callback was unbounded; now at most 20 blocks per call
      (`sample_decode_regression`).
- [ ] First install on a real FM-1: [docs/mello/FIRST_FLASH.md](docs/mello/FIRST_FLASH.md).
- Left for later (P2 in the review): MIDI CC113 on channel 16 can clear a sample slot; every save erases 10 sectors
  (0.5–4 s main-loop stall); the screen does not say when saving is off after a rollback.

## Phase 2: sound = the HTML

The reference is in place: `make reference` renders `reference/skurkymello.html`'s chain with Tone.Offline
(tools/tone-ref, Tone 14.9.17), `make parity` holds the C engine against it (tests/golden/parity.json).

- [x] Main filter: two biquads, Q in dB (tone on the filter stimuli 4.5-11.7 -> 0.2-1.5 dB)
- [x] Tone envelopes for the lead (exponential decay/release, shared with the sub)
- [x] Always-on waveshaper and the HTML's gain staging (level +10..14 dB -> within +-1.3 dB)
- [x] Follower sub: level squared, no glide
- [x] Reverb: Tone.Reverb's RT60, flat loop, x1.05 return (RT60 6.0 -> 2.3 s vs the HTML's 2.4)
- [x] FX tone filters -24 dB/oct; rounded biquads (no DC limit cycle)
- [x] Transport lookahead (0/107 late notes vs 107/107)
- [x] PolyBLEP saw/square/pulse, interpolated sine
- [x] Reverb: slow delay modulation on the FDN lines, allpass-interpolated (per-note level spread 2.5-3.0 -> 1.6-1.8 dB,
      Tone.Reverb 1.5-1.7; neighbouring notes jumped up to 12.7 dB, now 6.1)
- [x] Distortion gain compensation across AMT (a departure from the HTML: docs/mello/DECISIONS.md)
- [x] Audio LFO as the HTML: PITCH +-(80 + 620 depth) cents, FILTER linear in Hz, AMP on the whole mix, PWM symmetric;
      AMP and PWM swing around the base, as applyLfoRouting's comments intend (DECISIONS.md)
- [x] Tone's pulse width: duty (1 + width) / 2
- [x] Portamento as a straight line in pitch; overlapping notes play legato (Tone cancels the pending release)
- [x] Knob changes ramp as the HTML's rampTo (cutoff, levels, sends, feedback; delay time read between samples)
- [x] Limiter as Tone.Limiter(-1) instead of a hard clip at -1 dBFS
- [x] Sub filter Q 0.8 in dB; sample envelopes with Tone's decay/release curves
- [x] Saw and square at Web Audio's normalized level (-1.43 dB): this was most of the "aliasing at res 0.5"
- make parity (16 stimuli): level -0.2..0.0 dB on most, tone 0.2-2.0 dB; by design drive_dry/drive_high sit ~10 dB
  under the HTML (gain compensation). Left: lfo_pwm 4.8 dB (Tone's pulse is not band-limited and aliases),
  reverb_click 5.3 dB (the tail's low end, and a ~1 LSB floor the float HTML does not have), lead_pluck -0.7 dB level.
- CPU (fm1-emu, default pattern, 1.5 s): 68.0 M ISR instructions/s, 27.9% of the chip (0.2-p1: 58.0). The main loop
  sleeps between interrupts, so the web emulator skips ~56% of the cycles at 240 MHz.
- Not done: the chord layer's envelope (the chord is not in the HTML: nothing to match).
- Note: the output is as quiet as the HTML (about -10 dB RMS vs 0.2-p1). Raise MASTER, or decide on an outer gain as a
  product choice.

## Phase 3: feel = the HTML

PERFORM HOME (docs/review-2026-10/02_ux.md §5): white keys toggle steps, K1–K4 fixed to CUTOFF/DECAY/SPACE/DRIVE,
black keys for RANDOM/UNDO/MUTATE/FILL..., EDIT makes the keys a playable keyboard, REC records steps. At most two
levels; honest help texts; autosave everything.

The instrument is an iceberg (2026-10-29): it must be very easy to make something good and cool from HOME, and possible
to dig deeper, but never impossible or unintuitive to find your way. Today there are many parameters on many pages,
it is hard to find things, and at times hard to see on screen what you are adjusting.
The layout is [docs/mello/UI_ICEBERG.md](docs/mello/UI_ICEBERG.md) (approved 2026-10-29); the look follows screenshots
of the HTML (`tools/tone-ref/shoot.mjs`).
- [x] The readout: what a knob moves, its value and where you are, large for 1.2 s; full names and music units.
- [x] DOT matrix: its read position moves on screen again (DOT was redrawn on edits only).
- [x] PERFORM HOME: K1-K4 = CUTOFF / DECAY / SPACE / DRIVE; hold a step + turn edits it, a tap mutes it; step LEDs
      with the playhead; SELECT / ALGORITHM / PRESETS = TEMPO / RANDOM intensity / PATTERN.
- [x] The black-key strip in the keyboard's groups: RND, UNDO (redo), MUT, transpose -/+, MASK, CLR2 (undoable).
- [x] Page buttons open the first layer and step layers on re-press; MOD = LFO / DOT / EVOLVE; SAMPLE under GLO.
- [x] Switches by direction (no flip per detent), acceleration, the HTML's colours and dark-green ground, the key in
      the header, SAVE short = save / held = SLOTS, SAVED / UNSAVED / SAVE OFF on the status line, true help texts.
- [x] KEYS (EDIT): the keys play the lead (stopped), set the root (playing), step-record (REC).
- [x] The HTML's visual expression: text on its real background (no black boxes), HOME's steps as the HTML's lanes
      (gradient bars, note letters, the playing step brighter), SOUND / MOD LFO / FX / GLOBAL as the HTML's synth panel
      (spaced titles, label/value rows, green ring sliders, pills), rounded frames everywhere, the readout as a panel.
- [x] The same treatment for STEP, SEGMENT, LOOP, SLOT SEQ, SLOTS, SUB, CHORD and SAMPLE: a panel with a spaced
      title and the layer, the page's picture (strip, segments, slot cards, 5-SEQ, chord, sample) with pills, and
      K1-K4 two by two under it (sliders where a value has a range); sub, chord and sample times in ms.
- [x] The performance keys on HOME: FILL, SUB MUTE, REVERSE, REPEAT (+ white key = length), RESTART; REVERSE and
      REPEAT keep time; RNDMZR moved to the last black key on every page (DECISIONS.md "Performance keys").
- [ ] Note names in HOME's step bars as the HTML's lanes; the remaining pages' bodies in music units.
      The first half is done (checked 2026-10-31): HOME's bars carry the note's letter and a dot for a sharp, as the
      HTML's lanes (NOTE_NAMES, no octave). Still open: a page-by-page pass for music units.
- [x] Randomizer "skyline": its length and bar width follow the sequence length.
- [x] A splash screen with the SKURKYMELLO wordmark (it left the header): at boot, while storage and the samples
      load, for 1.5 s at least (mello_ui_draw_splash; ui_splash).

## Phase 5: deeper sound and modulation (2026-10-29 wishlist)

Beyond the HTML: decide each one in docs/mello/DECISIONS.md, keep the defaults sounding as now, and keep the surface
simple (Phase 3's iceberg).
- [x] Synth: optional "grit", inspired by the Access Virus: noise mixed in, noise FM of the pitch, unison voices with
      detune. The default stays a plain saw, as now. SOUND / GRIT; NOISE, NOISE FM and DETUNE are LFO targets
      (DECISIONS.md "Grit"). DOT and EVOLVE reach the grit too (below).
- [x] More of the Virus (DECISIONS.md "Virus layer"): OSC 2 with hard sync and FM, SAT into the filter, filter modes
      LP / HP / BP / NOTCH, a MOD envelope (cutoff, pitch, OSC 2, FM, noise, res); the LFO reaches RES, OSC 2, FM, SAT.
- [x] DOT and EVOLVE to the sound (DECISIONS.md "DOT and EVOLVE routes"): DOT's MOD 1 / MOD 2 lanes with TARGET and
      an AR / ASR SHAPE, DOT's own EVOLVE, EVOLVE's SOUND routes; evolution waits for the loop's one.
- [x] Modulation: generally more parameters as modulation targets. Done: the LFO's 11, the MOD envelope's 6, DOT's
      and EVOLVE's 11; velocity and key tracking (SOUND / TRACKING, DECISIONS.md "Velocity and key tracking").
- [x] Randomizer: route to several destinations at once, and an invert option per route, so one parameter mirrors
      another inverted.
      Done as links to the pitch, + or - per target (RANDOM / LINKS; DECISIONS.md "Randomizer links").
- [x] DOT orders: P-PONG, SPIRAL, DIAG, BOUNCE, WALK, and REV for any of them (DECISIONS.md "DOT orders").
- [x] Chord mode: unusable as it is. Redesign it, both the sound and how it is played, so it is usable and makes
      musical sense (see DECISIONS.md "Harmonic layer" for the earlier candidates).
      Done, the sound and the sight (DECISIONS.md "The chord voice"): ADSR per voice, a resonant filter with its own
      envelope, a Juno-style stereo chorus, SPACE, LINK to HOME's CUTOFF, voice leading (VLEAD), STRUM; the chain as
      chord names, the voices lit as they sound, PATTERN's playhead, the chord on HOME (chord_voice).
      Then the playing (DECISIONS.md "The chord chain"): MOTION's NEXT moves the chain by TRIG (as before), STEPS
      (HOLD in sixteenths, on its own clock) or SEG (chord k for segment k); B1 SUB puts the sub on the chord's root
      (chord_chain).

## Phase 4: validate, then grow

`docs/mello/HARDWARE_VALIDATION_CHECKLIST.md` on a unit. Then: Dinghy 0.4 (TRS MIDI, clock), Undo Last Random,
new engines. Before the first public release: confirm the package identity (`FM-1_970`; Dinghy's docs list 97N for
zp12) on the Firmware Hub.
