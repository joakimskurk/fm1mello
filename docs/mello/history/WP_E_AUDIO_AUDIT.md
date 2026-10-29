# WP-E Audio character audit and acceptance contract

Date: 2026-10-28

WP-E1 maps the pre-change audio implementation before WP-E alters sound behavior. The implementation checkpoint entering WP-E is WP-D `0cd810b5deb90087d60b181682de7e9ff52682cd`; the final E1 baseline checkpoint is `b58dfb224f4037b27b6d232f02991274ed676504`.

## Product priority

Reverb is a **defining part of Mello**, not a utility effect. Reverb V3 is not accepted merely because it is stable, dense or low-correlation. It must sound substantially better than Reverb V2 in controlled listening and must remove the metallic/ring-mod-like quality that reopened the earlier technical acceptance.

The WP-E rule is therefore: **objective DSP gates prevent regressions; perceptual A/B decides whether the reverb is good enough.**

## E1 reconstructed-source baseline

Final reconstructed source at E1:

- `mello_audio.c`: `3bfc64f6b101a8123b1533014fece746073bfac22055b83434252d8e0252ff85`
- `mello_audio.h`: `302e217d551d1f250269ca37216217cffd067656a7db371ad1dc91e7ec7c61b5`
- `mello_engine.c`: `daa00773838efe5d511c81afcdc84ba3e8bef42c6e08d368de5843c643f7670c`
- `mello_engine.h`: `769ea7e8b7a1be51d38bbd48e5eb09e39114dd79ca363dcb70b235e1483cdab8`
- `mello_ui.c`: `08bd19d7a2f112de318d50516c5a27b3ed2324d0c82bd50a087caadd0b7ecd55`

Permanent E1 baseline:

- `tools/wp_e_audio_baseline.py`;
- `.github/workflows/wp-e-audio-audit.yml`;
- this document.

Final E1 audio-audit run `40773113620`: PASS.

Retained audio baselines:

- Reverb V2 IR: dense `42497`, corr² `0.00075`, max block-energy jump `2.279`, predelay delta `5292` samples;
- 300 BPM / 8-ratchet audio timing stress: PASS;
- audio event ordering: PASS;
- live-edit publication stress: energy `433436686798`, ticks `159600`, config applies `2470`, SUB curve builds `67`, reverb reconfigs `135`: PASS.

## Distortion baseline

Current global distortion is intentionally conservative compared with the approved WP-E target:

- `AMT` and `CHAR` are converted by `browser_dist_drive()` into a derived drive value capped at `800/1000`;
- `AMT=500`, `CHAR=350` gives derived drive `127`;
- maximum derived drive is `800`;
- the actual `soft_drive()` call receives `character=0`, so CHAR currently shapes the amount-to-drive mapping rather than independently changing the waveshaper;
- at maximum derived drive the pre-knee gain is `42368/32768` (~1.29x) and the positive knee is `26367`, followed by an 1/8 post-knee slope;
- physical controls are `AMT / MIX / CHAR / TONE`; distortion output level is retained as `D.LEVEL` on the adjacent delay-config page.

E2 may change this behavior deliberately, but must remain bounded, gain-aware and musically controllable at low settings.

## Reverb V2 baseline

V2 is a compact mono Schroeder/FDN hybrid:

1. separate predelay;
2. two serial all-pass diffusers;
3. four static FDN delay lines mixed with a Hadamard matrix;
4. one fixed damping coefficient (`14755` Q15) for every line;
5. per-line feedback derived from line length and DECAY;
6. a single mono late-return combination `(y0 + y1 - y2 - y3) / 2`;
7. output tone low-pass plus a low-frequency high-pass before return level.

Full V2 layout uses `23173` samples. The target release ring is `24576` samples, leaving `1403` ring samples beyond the V2 layout. The project-wide target resource baseline remains RAM headroom `21472 B` and pool headroom `17696 B`.

### Why technical V2 acceptance was insufficient

The objective V2 figures are good, but the architecture still contains perceptual risk:

- the four late delay lengths are static;
- there is no subtle late-tail delay modulation to decorrelate persistent modes over time;
- all lines share the same fixed damping law;
- the late field is collapsed to one mono combination;
- SIZE/PRE/DECAY can reconfigure delay topology abruptly while the tail is live;
- WET/TONE/LEVEL and the broader FX snapshot are applied without a dedicated continuous smoothing layer.

This combination can produce a dense IR while still exposing an audible metallic or pitched identity on real musical material. Reverb V3 must address the perceptual problem rather than optimize only the existing metrics.

## Ratchet baseline

Ratchet scheduling structurally matches the intended browser behavior:

- for `ratchets > 1`, every sub-trigger emits its own `NOTE_ON` and `NOTE_OFF` pair;
- sub-trigger spacing is `pulse_ticks / ratchets`;
- raw gate duration is 72% of that sub-interval and is bounded to 10–250 ms;
- every `MELLO_EVENT_NOTE_ON` reaches `lead_attack()`, so the lead envelope re-enters ATTACK on every ratchet sub-trigger.

E4 still needs a musical/audible parity fixture because structural retriggering alone does not prove that fast ratchets have the same perceptual articulation as the browser reference.

## Live-edit / smoothing baseline

WP-B guarantees coherent odd/even config publication and selective heavy-DSP rebuilding. That reliability stays protected.

There is, however, no general continuous smoothing layer for live FX parameters. `a->fx` is swapped to the latest complete snapshot at the audio block boundary. Current special handling only precomputes/reconfigures selected derived state:

- PRE/SIZE/DECAY -> reverb reconfigure;
- AMT/CHAR -> distortion-drive derivation;
- WET -> reverb-send derivation;
- delay time/sync -> delay length derivation.

Mix, level, tone and many other values then take effect directly. E4 must add realtime-safe smoothing/crossfade behavior where abrupt changes can create zipper noise, clicks or tail discontinuity.

# Reverb V3 acceptance contract

## Hard perceptual rejects

A V3 candidate is rejected if normal musical settings expose any of these:

- a persistent metallic, bell-like or ring-mod-like pitch in the tail;
- obvious repeating comb modes or periodic flutter;
- a grainy sequence of discrete echoes where a diffuse room/tail is intended;
- boomy low-frequency buildup that masks the source;
- brittle high-frequency fizz that hangs unnaturally;
- audible clicks, zippering or violent tail pitch jumps during ordinary live parameter moves;
- a wet return that feels detached from the dry instrument instead of extending its space.

## Required musical qualities

V3 must provide:

- fast, smooth early diffusion while preserving the dry transient through PRE control;
- a dense late field whose modal identity does not remain stationary;
- frequency-dependent decay: lows controlled, mids rich, highs able to decay naturally rather than ring indefinitely;
- useful SIZE changes that sound like changing space, not simply retuning four resonators;
- DECAY that remains stable from short room-like settings through long ambient tails;
- TONE that changes color without creating unstable or hollow tails;
- WET and LEVEL ranges that work both as subtle glue and as a major atmospheric part of Mello;
- live movement of WET/SIZE/DECAY/TONE/PRE/LEVEL that is musically safe;
- preferably a decorrelated stereo late return if it can be achieved inside target CPU/memory budgets. A stereo field may use different orthogonal reads/mixes of the same FDN state; it does not automatically justify doubling delay memory.

## Controlled A/B protocol

Every serious V3 candidate must be rendered against the same dry source and Reverb V2. At minimum use:

1. short bright pluck / single-note transient;
2. staccato lead sequence;
3. sustained chord or chord-bed material;
4. low bass/sub-rich phrase;
5. dense rhythmic or ratcheted phrase.

For each stimulus compare `DRY / V2 / V3` with matched source, parameter intent and sensible level matching. Do not hide tail defects through loudness differences. Record a short listening note for:

- spatial depth;
- early diffusion;
- tail smoothness;
- absence of metallic pitch;
- tonal decay;
- transient preservation;
- integration with the dry source;
- usefulness across short and long settings.

V3 is accepted only when it is clearly preferred to V2 on the important Mello material and has no hard-reject artifact. Technical metrics alone cannot override a failed listening result.

## Objective guardrails retained for V3

V3 must still remain:

- stable and bounded at all legal SIZE/DECAY/TONE/PRE/WET/LEVEL values;
- free of dangerous DC buildup and runaway feedback;
- dense enough to avoid sparse-tap character;
- low enough in repeating correlation to avoid obvious periodicity;
- smooth in block/tail energy;
- compatible with the existing dry-equivalence gate when reverb is off;
- inside the established target performance gates, including >= `1.35x` steady realtime margin at 80 MHz worst-case and >=16 KiB RAM and pool headroom.

The preferred implementation should reuse the existing reverb ring and spend CPU/state on perceptually useful diffusion, damping, modulation and stereo decorrelation before requesting more delay memory.

## E1 closeout evidence

E1 final baseline checkpoint `b58dfb224f4037b27b6d232f02991274ed676504` changes tests/audit/documentation only; it does not alter DSP behavior.

Validation on that checkpoint:

- permanent WP-E audio audit `40773113620`: PASS;
- all completed retained fast regression workflows: PASS, no failures;
- full target `40773113827`: PASS;
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

Hosted emulator timings vary with runner load; the established thresholds, not the unusually fast E1 runner, remain authoritative.

**E1 is COMPLETE. E2 extreme bounded distortion is ACTIVE NEXT.**

## WP-E sequence after E1

- E2: extreme but bounded distortion;
- E3: Reverb V3, using this perceptual contract as the acceptance authority;
- E4: ratchet audible parity + realtime-safe live smoothing;
- final WP-E regression/target closeout, then activate WP-F.
