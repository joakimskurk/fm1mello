# TESTING

## Validation philosophy

A green technical metric is necessary but not sufficient for product acceptance. Deterministic tests, target/performance measurements and hands-on audition gates are used where applicable.

The user does not currently have a physical FM-1, so host/emulator/web/target-build validation is primary until hardware arrives.

## Locked baselines

WP-A randomizer remains `LOW < MED < HIGH`, target isolation/WAVE and 4..32 skyline protected. WP-B live-edit, rotary mapping and display integrity remain hard gates. WP-C locks REC/EDIT/contextual-action/navigation parity. WP-D locks the musical bar timing model and timing-language separation. WP-E1 locks the pre-change audio topology/behavior plus the perceptual acceptance contract for Reverb V3.

## Final WP-D verified baseline

Implementation/test checkpoint: `0cd810b5deb90087d60b181682de7e9ff52682cd`.

WP-D model: `FREE / 1 / 2 / 4 / 8 / 16 BAR`, default **2 BAR**, DIV-invariant musical bar duration, safe stored-value handling and explicit separation from external/MIDI sync, Loop Cycle `SYNC/FREE` and Slot/Chain HOLD.

Final target `40763918161`: PASS.

Retained target/resource baseline:

- Reverb V2 IR: dense `42497`, corr² `0.00075`, jumpE `2.279`, predelay delta `5292`;
- 300 BPM ratchet/chord/sub/sample timing stress: PASS;
- live-edit host: energy `433436686798`, ticks `159600`, applies `2470`, SUB builds `67`, reverb reconfigs `135`: PASS;
- image `190156 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `611545 B`, SHA-256 `bee43ce894a5a26472270b0890f1d64e400bfb68246980f5fe285179a4225b7f`;
- provenance: PASS.

## WP-E1 verified baseline

Checkpoint: `b58dfb224f4037b27b6d232f02991274ed676504`.

Permanent E1 coverage:

- `tools/wp_e_audio_baseline.py`;
- `.github/workflows/wp-e-audio-audit.yml`;
- `docs/WP_E_AUDIO_AUDIT.md`.

Audio-audit run `40773113620`: **PASS**. It reconstructs final source, maps distortion/reverb/ratchet/smoothing/control paths, runs the permanent E1 source/behavior contract, then reruns existing audio HTML-parity, timing, event-order and live-edit baselines.

All retained fast workflows completed without failure on the same checkpoint. Full target `40773113827`: **PASS**.

E1 target evidence:

- emulator panel/audio: `189440` frames, peak `3530`, `148408` nonzero samples: PASS;
- browser/WASM 100 MHz steady realtime `3.065x`: PASS;
- SEQ/HOME 80 MHz steady realtime `3.161x`: PASS;
- Chord + Reverb V2 80 MHz steady realtime `3.107x`: PASS;
- physical-encoder live-edit stress 80 MHz steady realtime `3.166x`, tail nonzero `88174`, tail peak `3847`: PASS;
- Chromium keyboard/pointer including PLAY start-stop: PASS;
- image `190156 / 581564 B`;
- RAM `76832 / 98304 B`, headroom `21472 B`;
- pool `326368 / 344064 B`, headroom `17696 B`;
- package `611545 B`;
- package SHA-256 `bee43ce894a5a26472270b0890f1d64e400bfb68246980f5fe285179a4225b7f`;
- provenance attestation: PASS.

Hosted emulator timings vary materially with runner load. Established acceptance thresholds, not the unusually fast E1 runner, remain authoritative.

Reconstructed-source hashes:

- `mello_audio.c` `3bfc64f6b101a8123b1533014fece746073bfac22055b83434252d8e0252ff85`;
- `mello_audio.h` `302e217d551d1f250269ca37216217cffd067656a7db371ad1dc91e7ec7c61b5`;
- `mello_engine.c` `daa00773838efe5d511c81afcdc84ba3e8bef42c6e08d368de5843c643f7670c`;
- `mello_engine.h` `769ea7e8b7a1be51d38bbd48e5eb09e39114dd79ca363dcb70b235e1483cdab8`;
- `mello_ui.c` `08bd19d7a2f112de318d50516c5a27b3ed2324d0c82bd50a087caadd0b7ecd55`.

### Distortion baseline

- `AMT/CHAR` -> derived drive capped at `800`;
- `AMT=500`, `CHAR=350` -> drive `127`;
- maximum drive `800` gives pre-knee gain `42368/32768` and knee `26367`;
- actual `soft_drive()` receives `character=0`, so CHAR currently changes drive mapping, not an independent waveshape.

E2 is expected to change this baseline deliberately. It must remain bounded, gain-aware, controllable at low settings and meaningfully extreme at the upper end.

### Reverb V2 baseline

V2 uses predelay -> two serial all-pass diffusers -> four static Hadamard FDN lines -> one mono late-return combination -> output tone filtering. All lines share one fixed damping coefficient. Full V2 layout uses `23173` samples inside the target `24576`-sample ring.

Known product limitation: the static late delays and fixed damping can retain an audible metallic/pitched modal identity even while density/correlation tests pass. There is no late-tail delay modulation and no decorrelated stereo late return.

### Reverb V3 perceptual gate

`docs/WP_E_AUDIO_AUDIT.md` is authoritative. Reverb V3 is rejected for persistent metallic/bell/ring-mod pitch, obvious comb repetition/flutter, grainy sparse echoes, LF boom, brittle HF hang, live-edit clicks/zipper/violent tail jumps or a wet field that feels detached from the dry instrument.

Minimum controlled A/B material:

1. short bright pluck/transient;
2. staccato lead;
3. sustained chord/chord-bed;
4. bass/sub-rich phrase;
5. dense rhythmic/ratcheted phrase.

For each compare DRY / V2 / V3 with sensible level matching and record notes for depth, diffusion, tail smoothness, absence of metallic pitch, tonal decay, transient preservation, dry/wet integration and usefulness from short to long settings.

**A numerically green V3 cannot override a failed listening result.**

### Ratchet baseline

Each sub-ratchet emits its own NOTE_ON/OFF pair; every NOTE_ON enters `lead_attack()`. Structural retrigger parity is present. E4 still requires audible/musical parity evidence.

### Live smoothing baseline

WP-B coherent config publication and selective heavy-DSP rebuilding remain intact. There is no general continuous smoothing layer for FX mix/level/tone and other live values; E4 must add realtime-safe smoothing/crossfade behavior where needed.

## Active WP-E testing direction

E2 distortion changes must keep all retained WP-A/B/C/D and E1 gates green. E3 Reverb V3 must satisfy both objective DSP guardrails and the E1 listening contract. E4 adds audible ratchet parity and live smoothing.

WP-E final closeout requires emulator/browser/80 MHz/Chromium/target coverage and >=16 KiB RAM and pool headroom.

## Retained hardware/perceptual notes

Physical FM-1 timing/live-edit reproduction remains a later hands-on regression when hardware becomes available. Reverb V3 perceptual acceptance can be prepared with rendered A/B evidence before hardware but must not be replaced by technical metrics alone.

## Reporting rule

At each closeout record exact checkpoint SHA, workflow run IDs, measured gates and any hardware-only validation that remains deferred.
