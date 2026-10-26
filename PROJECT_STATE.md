# PROJECT STATE

Last verified work-package closeout: 2026-10-26

## Repository

- Repo: `joakimskurk/fm1mello`
- Active branch: `visual-port-home`
- Always resolve current branch HEAD before editing.
- New chats start with root `START_HERE.md` and follow the repo rather than old chat context.

## Work-package position

**WP-A / Step 4 - Randomizer intensity + RANDOM skyline refinement is complete.**

WP-A implementation landed in `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07` and its clean reconstructed-source closeout is locked at `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

**ACTIVE NEXT WORK PACKAGE: WP-B - Trust and reliability.**

WP-B starts with the reported live-edit audio freeze. Do not begin WP-C or later work until WP-B is complete, tested and documented.

## WP-A delivered behavior

Randomizer remains a primary creative instrument.

- `LOW`: conservative variation around current material.
- `MED`: exact compatibility path through the previous/default randomizer implementation.
- `HIGH`: broader structural/timbral movement while remaining within legal ranges.
- intensity changes mutation depth/span only; it does not change target selection.
- `WAVE` remains independently selectable and isolated.
- intensity is runtime UI state, so WP-A introduced no project/storage migration.
- RANDOM page exposes LOW/MED/HIGH on K1 and retains the target mask controls.
- RANDOM skyline shows the full active 4..32-step sequence without paging, including compressed event information and playhead feedback.

## WP-A verification evidence

Clean reconstruction closeout:

- commit: `42cf506be5b127e7f03c763966d1e75cc4a48a85`
- `Reconstructed source gate` run: `40294234032`
- result: PASS
- WP-A split-patch transport is protected by base64 SHA-256 `6893352e8da1fdceb0c3ca40e174c229f4c027d4f21326300f649b5c590172d8`, gzip SHA-256 `2902990dab7e56a9e8136ba131d49076b56fce63414df56c77c41c46d2102d8f`, plus exact pre/post source hashes.

Full target closeout:

- `Visual port target build` run: `40294234527`
- result: PASS
- repo build/test command: `bash ci/build_target.sh`
- randomizer distance, Chord enabled: `LOW=194824 < MED=516917 < HIGH=1082367`
- randomizer distance, Chord compiled out: `LOW=180984 < MED=484357 < HIGH=1038671`
- RANDOM incremental redraw: `4 fills / 0 texts`
- WAVE isolation: PASS
- full RANDOM skyline lengths `4..32`: PASS
- legal-range/deterministic randomizer coverage: PASS
- 300 BPM timing stress: PASS
- chord/reverb regressions and chord compile-out: PASS
- emulator panel/audio: PASS
- browser/WASM 100 MHz path: PASS, `1.872x` steady realtime
- Chromium keyboard/pointer E2E: PASS
- 80 MHz SEQ/HOME path: PASS, `2.231x` steady realtime
- 80 MHz Chord + Reverb V2 worst-case: PASS, `2.249x` steady realtime.

Target resource/package result:

- image: `190560 B / 581564 B`
- RAM: `76320 B / 98304 B`, headroom `21984 B`
- pool: `326368 B / 344064 B`, headroom `17696 B`
- package: `skurkymello-0.1-dev.fwsc`, `611545 B`
- package SHA-256: `fcd1fbf7624b50995f90267f4e168fe2a72328a8f7eeeb46c4982632cd999df0`
- build provenance attestation: PASS.

Both RAM and pool remain above the required 16 KiB headroom floor.

## Closeout deviation resolved

WP-A initially appeared to fail reconstruction after its implementation. The WP-A patch itself was valid. The actual blocker was the old temporary `ci/randomizer_roadmap_probe.py` still being invoked by `ci/patch_chord_mix_separation.py`; that probe intentionally exited non-zero after printing diagnostics. Its invocation was removed in `2883e5670c3617f8f81aad738c9620ba600664ef`. Temporary tracing/artifact diagnostics were then removed and the correct WP-A transport-integrity hashes restored in `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

This was a resolved workflow deviation, not a randomizer functional defect.

## Active WP-B scope

WP-B is a trust/reliability package, not feature expansion:

1. reproduce and fix live-edit audio freeze;
2. guarantee R1-R4 rendered labels match the actual handlers;
3. remove text clipping/overlap;
4. remove the HOME pink-line artifact;
5. expand automated mapping/text-fit/stale-region coverage.

Major synth expansion remains blocked until WP-B is green.

## Locked product direction after WP-B

- WP-C: Control Grammar v2 - REC/EDIT cleanup, contextual black-key actions, MOD family.
- WP-D: timing model - default `1 BAR`, explicit FREE, timing concepts kept separate.
- WP-E: audio character - extreme distortion, Reverb V3, ratchet parity and live smoothing.
- WP-F: one-level Undo Last Random.
- WP-G: preserve ANALOG, add curated FM, SOUND randomization and realtime-safe per-step timbre.
- WP-H: prototype scale-aware HARMONY, then PAD, before replacing current Chord.
- WP-I: splash/HOME/Key-Scale/SEQ/onboarding polish.
- WP-J: final integration/performance gate.

## Source reconstruction model

Firmware source is reconstructed by `ci/unpack_source.sh` from source chunks plus an ordered patch chain. Functional work must continue to follow this patch/test architecture. Do not assume reconstructed `mello/` files are ordinary committed source of truth.

## New-chat procedure

Read `START_HERE.md` first. It contains the active package, file-reading order and stop condition.
