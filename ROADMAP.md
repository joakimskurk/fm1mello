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

- Reference first: render `reference/skurkymello.html` with `Tone.Offline` in headless Chromium for fixed inputs,
  keep the results as golden files, compare the C render (spectrum, envelope, level). Pin Tone.js to an exact 14.x
  version first: the page loads `tone@14`, which floats.
- Then: two biquads with Q in dB, Tone's exponential envelopes, the always-on waveshaper with its 1/3 gain staging,
  12–20 ms transport lookahead, reverb towards the noise-convolution target (RT60 = 6.9·ln(decay+1)/ln 200).
- After that: PolyBLEP oscillators, 24 dB FX tone filters, LFO depth, follower level, exponential portamento,
  soft limiter, parameter smoothing.

## Phase 3: feel = the HTML

PERFORM HOME (docs/review-2026-10/02_ux.md §5): white keys toggle steps, K1–K4 fixed to CUTOFF/DECAY/SPACE/DRIVE,
black keys for RANDOM/UNDO/MUTATE/FILL..., EDIT makes the keys a playable keyboard, REC records steps. At most two
levels; honest help texts; autosave everything.

## Phase 4: validate, then grow

`docs/mello/HARDWARE_VALIDATION_CHECKLIST.md` on a unit. Then: Dinghy 0.4 (TRS MIDI, clock), Undo Last Random,
new engines. Before the first public release: confirm the package identity (`FM-1_970`; Dinghy's docs list 97N for
zp12) on the Firmware Hub.
