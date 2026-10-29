# Roadmap

From the October 2026 review ([docs/review-2026-10/REVIEW.md](docs/review-2026-10/REVIEW.md)): keep the model,
sequencer and storage; rewrite the sound to match the HTML, rebuild HOME as an instrument. One phase at a time.

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
- CPU (fm1-emu, default pattern): 58.7 M ISR instructions/s, as 0.2-p1 (58.0).
- [ ] Waveshaper oversampling: at res 0.5 the clip aliases (+16 dB at 2 kHz); the HTML oversamples 4x.
- [ ] Reverb burst: more energy than the HTML at 60-250 Hz and near 10 kHz.
- [ ] Samples and chord: Tone envelopes; LFO depths/targets (pitch 4-5x too shallow, AMP on the whole mix);
      exponential portamento; soft limiter; parameter smoothing; sub filter Q in dB.
- Note: the output is now as quiet as the HTML (about -10 dB RMS vs 0.2-p1). Raise MASTER, or decide on an
  outer gain as a product choice.

## Phase 3: feel = the HTML

PERFORM HOME (docs/review-2026-10/02_ux.md §5): white keys toggle steps, K1–K4 fixed to CUTOFF/DECAY/SPACE/DRIVE,
black keys for RANDOM/UNDO/MUTATE/FILL..., EDIT makes the keys a playable keyboard, REC records steps. At most two
levels; honest help texts; autosave everything.

## Phase 4: validate, then grow

`docs/mello/HARDWARE_VALIDATION_CHECKLIST.md` on a unit. Then: Dinghy 0.4 (TRS MIDI, clock), Undo Last Random,
new engines. Before the first public release: confirm the package identity (`FM-1_970`; Dinghy's docs list 97N for
zp12) on the Firmware Hub.
