# PROJECT STATE

Last verified subtask closeout: 2026-10-28

## Repository

- Repo: `joakimskurk/fm1mello`
- Active branch: `visual-port-home`
- Always resolve current branch HEAD before editing.
- New chats start with root `START_HERE.md` and follow the repo rather than old chat context.

## Work-package position

**WP-A / Step 4 - Randomizer intensity + RANDOM skyline refinement is complete.**

WP-A implementation landed in `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07` and its clean reconstructed-source closeout is locked at `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

**ACTIVE WORK PACKAGE: WP-B - Trust and reliability.**

**B1 live-edit audio freeze: COMPLETE.**
**B2 rotary display/handler parity: COMPLETE.**
**ACTIVE SUBTASK: B3 display integrity.**

Do not begin WP-C or later work before B3 and the complete WP-B regression closeout are green and documented.

## B1 delivered reliability changes

The reported live-edit freeze could not be reproduced on physical hardware because the user does not yet have an FM-1, but the reconstructed audio path exposed two concrete realtime risks consistent with the report:

- pending audio configs could be consumed without an explicit update-in-progress guard;
- every audio-config change rebuilt heavyweight derived DSP state inside audio render, including the full 257-point SUB distortion curve and reverb configuration even when unrelated parameters changed.

B1 now:

- publishes pending config snapshots with an odd/even generation protocol so audio keeps the previous complete snapshot while a new one is being written;
- uses 32-bit generation counters and regression-tests the old 32768-publication alias distance;
- precomputes the 257-point SUB drive curve outside audio render and applies the prepared curve atomically;
- invalidates LFO, delay, reverb, distortion and SUB derived state only when the relevant source fields change;
- retains allocation-free normal live rotary paths;
- adds a permanent deterministic host stress and a real 80 MHz front-panel encoder stress to the target gate.

Verified B1 implementation/test checkpoint: `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`.

## B1 verification evidence

- `WP-B live-edit gate` run `40596812842`: PASS;
- `Reconstructed source gate` run `40596812864`: PASS;
- `Visual port target build` run `40596813400`: PASS;
- 2400 live-edit audio blocks: PASS;
- transport reached tick `159600`;
- 32768-publication alias regression: PASS;
- physical encoder live-edit stress at 80 MHz: PASS;
- target RAM headroom `21472 B`; pool headroom `17696 B`.

The exact originally reported physical-hardware symptom remains a later hands-on regression when an FM-1 becomes available.

## B2 delivered rotary parity

Verified B2 implementation/test checkpoint: `85a3c376b47829af6d6abdd3264ec75cb37d42bf`.

Confirmed defects fixed:

- GLOBAL subpage 0 K4 now renders `MIDI OUT` with the actual ON/OFF state it controls;
- Loop Cycle STEP K3/K4 now render `OCT` / `TRANS` in the same physical positions as their handlers;
- SLOTS no longer presents an inactive K1 as a fake parameter, and the slot-name CHAR footer shows the character being edited.

Permanent coverage:

- `tools/rotary_mapping_parity.c` drives actual K1-K4 encoder input and captures the rendered footer for every current page/layer fixture;
- 45 page/layer fixtures / 180 mappings with Chord enabled;
- 40 page/layer fixtures / 160 mappings with Chord compiled out;
- explicit action rotaries are classified intentionally;
- reconstruction locks the B2 patch and fixture hashes.

## B2 verification evidence

- `WP-B rotary parity gate` run `40599056435`: PASS;
- `Reconstructed source gate` run `40599056475`: PASS;
- B1 regression run `40599056403`: PASS;
- full `Visual port target build` run `40599056945`: PASS;
- browser/WASM 100 MHz: `1.663x` steady realtime;
- SEQ/HOME 80 MHz: `1.776x` steady realtime;
- Chord + Reverb V2 80 MHz: `1.760x` steady realtime;
- live-edit physical encoder stress 80 MHz: `1.707x` steady realtime, final-second audio present;
- image `191280 B / 581564 B`;
- RAM `76832 B / 98304 B`, headroom `21472 B`;
- pool `326368 B / 344064 B`, headroom `17696 B`;
- package SHA-256 `26ccf3716c4b15b3671e9b4d6d91cf5f162ec1e1e2f692ba486a70c0ea8cd8f8`;
- build provenance attestation: PASS.

## Active B3 scope - display integrity

B3 starts from two user-visible trust issues:

1. text can clip/overlap at extreme formatted values;
2. HOME can show a static pink-line artifact over `SINGLE`.

Required direction:

- reproduce both classes deterministically before changing rendering;
- validate text boxes and adjacent-column separation at legal extreme values across every page/layer;
- identify the exact draw primitive / dirty-region sequence that creates the HOME artifact;
- compare incremental redraw behavior against clean redraw behavior where practical;
- add permanent host coverage for text fit, overlap and stale/dirty-region regressions;
- preserve B1, B2, WP-A, audio, timing, feature-flag and target-performance gates.

## Remaining WP-B closeout after B3

After B3 implementation:

1. rerun all B1/B2/WP-A/audio/timing gates;
2. rerun emulator/browser/80 MHz/Chromium paths;
3. require >=16 KiB RAM and pool headroom on full target build;
4. record exact B3/WP-B closeout evidence;
5. set WP-C active and stop/handoff instead of beginning WP-C automatically.

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

Read `START_HERE.md` first. It contains the active package, active subtask, file-reading order and stop condition.
