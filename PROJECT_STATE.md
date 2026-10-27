# PROJECT STATE

Last verified subtask closeout: 2026-10-27

## Repository

- Repo: `joakimskurk/fm1mello`
- Active branch: `visual-port-home`
- Always resolve current branch HEAD before editing.
- New chats start with root `START_HERE.md` and follow the repo rather than old chat context.

## Work-package position

**WP-A / Step 4 - Randomizer intensity + RANDOM skyline refinement is complete.**

WP-A implementation landed in `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07` and its clean reconstructed-source closeout is locked at `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

**ACTIVE WORK PACKAGE: WP-B - Trust and reliability.**

**B1 live-edit audio freeze is complete. ACTIVE SUBTASK: B2 rotary display/handler parity.**

Do not begin B3, WP-C or later work before the active subtask is complete, tested and documented.

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

Dedicated host gate:

- workflow: `WP-B live-edit gate`;
- run: `40596812842`;
- result: PASS;
- 2400 live-edit audio blocks with active transport: PASS;
- transport reached tick `159600`;
- stable config applies: `2470`;
- SUB curve builds: `67`;
- reverb reconfigurations: `135`;
- no event-queue overflow;
- no silent stress window;
- no STOP/PLAY recovery;
- 32768 coalesced publications: PASS, newest config remains visible and applies once.

Reconstruction:

- `Reconstructed source gate` run `40596812864`: PASS.

Full target gate:

- `Visual port target build` run `40596813400`: PASS;
- physical encoder live-edit stress at 80 MHz: PASS;
- stress length: `9002 ms` emulated;
- steady realtime margin: `1.673x`;
- complete audio output: `743872` nonzero samples, peak `3902`;
- final second after encoder stress: `88168` nonzero samples, peak `3902`;
- corrected SEQ/HOME 80 MHz encoder-path gate: PASS, `1.756x` steady realtime;
- Chord + Reverb V2 80 MHz worst-case: PASS, `1.663x` steady realtime;
- browser/WASM 100 MHz: PASS, `1.660x` steady realtime;
- emulator panel/audio and Chromium keyboard/pointer E2E: PASS;
- WP-A randomizer, timing, Chord and Reverb regressions: PASS.

Target resource/package result after B1:

- image: `191232 B / 581564 B`;
- RAM: `76832 B / 98304 B`, headroom `21472 B`;
- pool: `326368 B / 344064 B`, headroom `17696 B`;
- package: `skurkymello-0.1-dev.fwsc`, `611545 B`;
- package SHA-256: `9d118b8559bd6fbd2bf89c3c763edb8a626da9ed75ec37e2324696d2ed5d72da`;
- build provenance attestation: PASS.

Both RAM and pool remain above the required 16 KiB headroom floor.

### B1 validation limitation

The original physical-hardware symptom still requires a later hands-on regression when an FM-1 is available. B1 is accepted on the currently available deterministic host, emulator, browser and target evidence; do not rewrite this as proof that the exact hardware symptom was physically reproduced.

## Active B2 scope - rotary display/handler parity

The source audit has already confirmed at least two real mismatches:

1. `GLOBAL` subpage 0: K4/R4 toggles MIDI note output, but the footer renders `REC / TAP`.
2. Loop Cycle STEP: K3 changes octave and K4 changes transpose, while the footer presents K3=`TRANS` and K4=`OCT`.

B2 must therefore be systematic, not two string fixes.

Required direction:

- validate every page/layer K1-K4 mapping;
- make label, displayed value, range/direction and handler agree;
- distinguish intentional momentary/action controls from continuous parameter controls;
- prefer one authoritative/testable mapping source or fixture where practical;
- add deterministic CI coverage that fails on future rotary/display drift.

## Remaining WP-B scope

After B2:

1. eliminate text clipping/overlap at extreme values;
2. remove the HOME pink-line artifact;
3. extend text-fit/stale-region coverage;
4. rerun all B1/WP-A/audio/timing/target gates before WP-B closeout.

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

Read `START_HERE.md` first. It contains the active package, active subtask, file-reading order and stop condition.
