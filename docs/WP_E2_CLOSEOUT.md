# WP-E2 closeout - Extreme bounded distortion

Date: 2026-10-28

## Result

WP-E2 is **COMPLETE**. WP-E3 Reverb V3 is ACTIVE NEXT.

Final E2 implementation/test checkpoint: `cbbf44a68b64a75195383511a04f607b68562935`.

E2 changes only the **global** distortion path. The legacy `soft_drive()` used by sample-slot distortion is intentionally left unchanged, so E2 does not silently retune sample-slot sound.

## Delivered distortion model

### AMT

Global `AMT` now maps 0..1000 to drive with a cubic curve:

- `0 -> 0`
- `50 -> 0`
- `100 -> 1`
- `250 -> 16`
- `500 -> 125`
- `750 -> 422`
- `1000 -> 1000`

This deliberately preserves fine low/mid control and approximately the old midpoint feel while opening a much larger upper range.

### CHAR

`CHAR` is now independent of AMT-derived drive. It shapes the global transfer function by moving the knee inward and reducing the post-knee slope. It no longer merely scales the amount-to-drive mapping.

### Waveshaper safety

The global shaper:

- uses fixed-point/integer processing in the sample loop;
- has approximately `1.0x..7.84x` pre-gain across the derived drive range;
- is bounded to the existing audio range;
- is monotonic across the tested transfer grid, so there is no foldback region;
- processes magnitude and reapplies sign, guaranteeing exact odd symmetry `f(-x) = -f(x)` and avoiding static DC asymmetry;
- leaves zero AMT as exact identity regardless of CHAR.

The first E2 CI attempt exposed a one-LSB-class signed-rounding asymmetry in the negative transfer path. The test was not weakened: the DSP was changed to magnitude-domain shaping, after which exact symmetry passed.

## Deterministic coverage

Permanent E2 assets:

- `ci/patch_wp_e2_distortion.py`;
- `tools/wp_e2_distortion_smoke.c`;
- E2 execution in `ci/patch_rotary_parity.py` reconstruction order;
- E2 contract execution in `.github/workflows/wp-e-audio-audit.yml`.

The smoke test covers:

- exact AMT mapping anchors;
- exact zero-drive identity;
- graduated low/mid reference points;
- genuinely extreme top-end saturation behavior;
- independent CHAR transfer-shape behavior;
- bounded output;
- monotonic transfer/no foldback;
- exact odd symmetry over an input/control matrix.

Generated E2 audio locks:

- `mello_audio.c`: `8a0adbcd9eadbefe7339e5ff4b5a3a82e951871375061ab06bd106f51fb2cb1f`;
- `mello_audio.h`: `463e403fa7f5c927bfc69be48b6f0b18d475f21fe9ac45f7e6c285eaf7e0b63d`.

## CI and target evidence

WP-E audio audit run `40776409911`: **PASS**.

It passes the active E2 distortion contract plus retained audio HTML parity, 300 BPM timing stress, event ordering and live-edit regression coverage. Retained WP-A/WP-B/WP-C/WP-D/reconstruction workflows on the E2 checkpoint completed successfully.

Full target run `40776410416`: **PASS**.

Target/resource evidence:

- emulator panel/audio: `189440` frames, peak `3530`, `148408` nonzero samples: PASS;
- browser/WASM 100 MHz steady realtime: `2.083x`: PASS;
- SEQ/HOME 80 MHz steady realtime: `2.212x`: PASS;
- Chord + enabled Reverb V2 worst-case 80 MHz steady realtime: `2.165x`: PASS;
- physical-encoder live-edit stress 80 MHz steady realtime: `2.159x`, tail nonzero `88160`, tail peak `4260`: PASS;
- Chromium keyboard/pointer including PLAY start-stop: PASS;
- image `190124 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `611545 B`;
- package SHA-256 `95473c044eea9958265595efe292c8cbc8bce79d7473e4d472b93c9839ff24e6`;
- provenance attestation: PASS.

The required >=16 KiB RAM and pool headroom is preserved.

## Deferred hands-on note

The user does not yet have a physical FM-1. Physical hardware audition therefore remains a later hands-on regression, but it does not block the deterministic/emulator/target E2 closeout.

## Next

WP-E3 Reverb V3 is now the only active next subtask. Build it against the perceptual contract in `docs/WP_E_AUDIO_AUDIT.md`; Reverb V3 is not accepted on numerical metrics alone.
