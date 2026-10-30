# Roadmap

From the October 2026 review ([docs/review-2026-10/REVIEW.md](docs/review-2026-10/REVIEW.md)): keep the model,
sequencer and storage; rewrite the sound to match the HTML, rebuild HOME as an instrument. One phase at a time.

## Now: feedback from playing 0.5 (2026-10-30)

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
- [ ] **Segments are confusing and hard to adjust**: work deep on making them easy (how they are set, seen and played).
      A new project starts with 3 or 4 segments, no repeats (decide which).

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
- [x] Randomizer "skyline": its length and bar width follow the sequence length.
- [ ] A splash screen with the SKURKYMELLO wordmark (it left the header).

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
- [ ] Chord mode: unusable as it is. Redesign it, both the sound and how it is played, so it is usable and makes
      musical sense (see DECISIONS.md "Harmonic layer" for the earlier candidates).

## Phase 4: validate, then grow

`docs/mello/HARDWARE_VALIDATION_CHECKLIST.md` on a unit. Then: Dinghy 0.4 (TRS MIDI, clock), Undo Last Random,
new engines. Before the first public release: confirm the package identity (`FM-1_970`; Dinghy's docs list 97N for
zp12) on the Firmware Hub.
