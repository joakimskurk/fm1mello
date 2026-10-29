# CHANGELOG

## 2026-10-28 - WP-E1 Audio character audit/baseline complete

WP-E1 is complete. E2 extreme bounded distortion is ACTIVE NEXT; no WP-E audio DSP behavior is changed by this closeout.

Final E1 baseline checkpoint: `b58dfb224f4037b27b6d232f02991274ed676504`.

Delivered:

- permanent reconstructed-source/audio audit workflow `.github/workflows/wp-e-audio-audit.yml`;
- deterministic pre-change audio contract `tools/wp_e_audio_baseline.py`;
- `docs/WP_E_AUDIO_AUDIT.md` with distortion/Reverb V2/ratchet/live-update findings;
- strict Reverb V3 perceptual A/B acceptance contract, with Reverb defined as a core Mello product-quality feature.

E1 findings:

- current distortion is conservative: derived drive capped at `800`, and CHAR currently changes drive scaling rather than an independent waveshape;
- Reverb V2 is two all-pass diffusers into a static four-line Hadamard FDN with fixed damping and mono late return;
- V2 has no late-tail delay modulation, leaving a credible cause for the audible metallic/pitched tail despite good objective IR metrics;
- ratchets already emit NOTE_ON/OFF per sub-trigger and each NOTE_ON re-enters `lead_attack()`;
- coherent WP-B config publication is retained, but there is no general continuous FX smoothing layer yet.

Validation on `b58dfb224...`:

- WP-E audio audit `40773113620`: PASS;
- retained fast workflows: PASS, no failures;
- full target `40773113827`: PASS;
- image `190156 / 581564 B`;
- RAM headroom `21472 B`;
- pool headroom `17696 B`;
- package `611545 B`;
- package SHA-256 `bee43ce894a5a26472270b0890f1d64e400bfb68246980f5fe285179a4225b7f`;
- provenance: PASS.

## 2026-10-28 - WP-D Timing model complete

WP-D is complete. WP-E Audio character is ACTIVE NEXT; no WP-E audio implementation is included in this closeout.

Final WP-D implementation/test checkpoint: `0cd810b5deb90087d60b181682de7e9ff52682cd`.

Delivered:

- D1 deterministic legacy timing audit/baseline;
- D2 `FREE / 1 / 2 / 4 / 8 / 16 BAR` main reset with **2 BAR default**;
- musical BAR duration independent of sequence `DIV`;
- existing 16-bit `master_len` storage layout retained;
- malformed noncanonical stored timing values hardened;
- D3 explicit `RESET` + BAR/FREE language;
- external/MIDI sync, Loop Cycle `SYNC/FREE` and Slot/Chain HOLD remain distinct;
- D4 full retained regression/performance/target closeout.

Exact generated locks:

- engine C `daa00773838efe5d511c81afcdc84ba3e8bef42c6e08d368de5843c643f7670c`;
- engine H `769ea7e8b7a1be51d38bbd48e5eb09e39114dd79ca363dcb70b235e1483cdab8`;
- UI `08bd19d7a2f112de318d50516c5a27b3ed2324d0c82bd50a087caadd0b7ecd55`.

Validation on `0cd810b...`:

- all 11 fast workflows: PASS;
- full target `40763918161`: PASS;
- image `190156 / 581564 B`;
- RAM headroom `21472 B`;
- pool headroom `17696 B`;
- package `611545 B`;
- package SHA-256 `bee43ce894a5a26472270b0890f1d64e400bfb68246980f5fe285179a4225b7f`;
- provenance: PASS.

Physical FM-1 timing/live-edit validation remains deferred until hardware exists. Full closeout: `docs/WP_D_CLOSEOUT.md`.

## 2026-10-28 - WP-C Control Grammar v2 complete

WP-C is complete. WP-D Timing model is ACTIVE NEXT; no WP-D implementation is included in this closeout.

Final implementation/test checkpoint: `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

C5 closeout:

- replaced duplicated contextual handler/render page mapping with one authoritative `MelloContextAction` page/B0-B7 descriptor;
- descriptor carries handler verb and rendered label together;
- handler dispatch now switches on descriptor verb rather than page;
- rendered contextual guidance iterates the same descriptor;
- added all-page/all-slot parity coverage with Chord enabled and compiled out;
- proved inactive slots inert, B5 RANDOM routing consistent, and STEP `SEL+black` manual-note priority retained;
- exact generated C5 UI/test outputs are hash-locked.

C5 coverage:

- Chord enabled: `16 pages / 31 active / 97 inactive`;
- Chord disabled: `15 pages / 29 active / 91 inactive`.

Final WP-C validation on `2c280184...`:

- authoritative actions `40703570244`: PASS;
- reconstruction `40703570315`: PASS;
- context grammar `40703570196`: PASS;
- REC `40703570212`: PASS;
- EDIT `40703570277`: PASS;
- action contract `40703570204`: PASS;
- audit `40703570330`: PASS;
- rotary `40703570466`: PASS;
- display `40703570254`: PASS;
- live-edit `40703570316`: PASS;
- full target `40703571071`: PASS;
- RAM headroom `21472 B`; pool headroom `17696 B`;
- package SHA-256 `755a098ecbe66a6f8f7697a5787cb240e50a69df2520dc2f10b690f0056b8da1`;
- provenance: PASS.

## 2026-10-28 - WP-C4 contextual rail and MOD navigation complete

Checkpoint `84d42c9fa21b2a91db5fa1aff2340c6ef3289a60`. RANDOM NONE/ALL moved to B6/B7, hidden GLOBAL/LOOP actions removed, contextual guidance rendered, MOD family reorganized, FX opens AUDIO FX directly. Target `40701005084`: PASS.

## 2026-10-28 - WP-C3 EDIT cleanup complete

Checkpoint `91afcd0f1b0146b761055d77fbdd7f95e6126ff3`. Generic EDIT navigation removed; genuine lock/edit states retained; STEP/SUB selected-position toggles moved to B4; DOT reset to B2; RANDOM access to B5.

## 2026-10-28 - WP-C2 REC cleanup complete

Checkpoint `f6d1f81660dab8db1de6da4e4650d01482d1ced3`. REC reserved for actual recording/capture; legacy functions moved to contextual B0-B3; SAMPLE navigation moved to ENV/SOUND. Target `40671590524`: PASS.

## 2026-10-28 - WP-C1 control-action audit complete

Baseline checkpoint `a2a2bb97f3c8bb95a9ed0a539241698949b0defc`. Added deterministic pre-remap coverage and `docs/WP_C_CONTROL_AUDIT.md`.

## 2026-10-28 - WP-B trust and reliability complete

Final checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; target `40604214660`: PASS. B1 live-edit, B2 rotary mapping and B3 display integrity remain regression-protected.

## 2026-10-26 - WP-A randomizer intensity + RANDOM skyline complete

Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`; target `40294234527`: PASS.
