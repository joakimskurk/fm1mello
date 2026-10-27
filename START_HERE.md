# START HERE - FM-1 Mello continuation

Use this file at the start of every new ChatGPT work session on `joakimskurk/fm1mello`.

## Repository

- Repo: `joakimskurk/fm1mello`
- Branch: `visual-port-home`
- Always fetch and verify the current branch HEAD before editing.
- Do not reconstruct project direction from old chat memory when repo documentation is available.
- Follow the active work package and active subtask below. Do not jump ahead.

## Read these files in this order

1. `START_HERE.md`
2. `PROJECT_STATE.md`
3. `ROADMAP.md`
4. `DECISIONS.md`
5. `CONTROL_GRAMMAR.md`
6. `TESTING.md`
7. `docs/NEXT_ITERATION_ROADMAP.md`
8. `docs/PRODUCT_WORKSHOP_2026-10-25.md`
9. `docs/UI_VISUAL_LANGUAGE.md` when UI is touched.

## Last completed work package

**WP-A / Step 4 - Randomizer intensity + RANDOM skyline refinement: COMPLETE.**

Implementation commit: `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`.
Clean reconstruction closeout: `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

Do not reopen WP-A unless a real regression is demonstrated.

## Active work package

**WP-B - Trust and reliability.**

WP-B is not complete yet. Work on the active subtask only.

### B1 - Live-edit audio freeze: COMPLETE

Verified implementation/test checkpoint: `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`.

Evidence:

- `Reconstructed source gate` run `40596812864`: PASS;
- dedicated `WP-B live-edit gate` run `40596812842`: PASS;
- full `Visual port target build` run `40596813400`: PASS;
- 2400-block host stress with active transport/audio: PASS;
- 32768 coalesced config publications: PASS;
- physical encoder-path stress in the 80 MHz emulator: PASS, `1.673x` steady realtime;
- final second of the 80 MHz stress remained audible: `88168` nonzero samples, peak `3902`;
- target RAM headroom `21472 B`; pool headroom `17696 B`.

The exact user-reported physical-hardware freeze cannot be hands-on re-auditioned until a physical FM-1 is available. Host, emulator, browser and target evidence are green and the hardware audition remains an explicit later regression check.

### B2 - Rotary display/handler parity: COMPLETE

Verified implementation/test checkpoint: `85a3c376b47829af6d6abdd3264ec75cb37d42bf`.

Delivered:

- GLOBAL CLOCK K4 now truthfully renders `MIDI OUT` and its ON/OFF state;
- Loop Cycle STEP K3/K4 now render `OCT` and `TRANS` in the same positions as their physical handlers;
- inactive SLOTS controls no longer present misleading values, and slot-name CHAR shows the actual edited character;
- permanent deterministic rotary mapping fixture exercises the real `mello_ui_input` encoder path across every current page/layer;
- explicit action handling is represented in the fixture rather than treated as a fake continuous parameter;
- Chord feature-on and compile-out configurations are both gated.

Evidence:

- dedicated `WP-B rotary parity gate` run `40599056435`: PASS;
- `180` K1-K4 mappings across `45` page/layer fixtures with Chord enabled: PASS;
- `160` K1-K4 mappings across `40` page/layer fixtures with Chord compiled out: PASS;
- `Reconstructed source gate` run `40599056475`: PASS;
- B1 live-edit regression run `40599056403`: PASS;
- full `Visual port target build` run `40599056945`: PASS;
- 100 MHz browser/WASM: `1.663x` steady realtime;
- SEQ/HOME 80 MHz: `1.776x` steady realtime;
- Chord + Reverb V2 80 MHz: `1.760x` steady realtime;
- live-edit encoder stress 80 MHz: `1.707x` steady realtime and final-second audio remains present;
- target RAM headroom `21472 B`; pool headroom `17696 B`;
- target package SHA-256 `26ccf3716c4b15b3671e9b4d6d91cf5f162ec1e1e2f692ba486a70c0ea8cd8f8`.

## Active WP-B subtask

**B3 - Display integrity.**

Start here. Do not begin WP-C early.

### B3 work order

1. Reproduce and eliminate text clipping/overlap at extreme formatted values.
2. Reproduce and remove the static pink-line artifact reported over `SINGLE` on HOME.
3. Add deterministic text-fit, overlap and stale/dirty-region coverage across every page/layer.
4. Rerun B1, B2, WP-A, audio/timing, emulator/browser, 80 MHz and target gates.
5. Close WP-B only after all trust gates are green and exact evidence is recorded.

### B3 acceptance

- no footer/header/body text exceeds its intended display region at legal extreme values;
- adjacent K1-K4 label/value columns do not collide;
- no intentional graphic primitive crosses text regions unless explicitly designed to do so;
- HOME redraws do not leave or redraw the reported pink line over `SINGLE`;
- dirty/incremental redraws produce the same relevant pixels/regions as a clean redraw;
- every current page/layer is represented in deterministic visual-integrity coverage;
- all prior B1/B2/WP-A/audio/timing/performance/target gates remain green.

### WP-B principles

- trust/reliability before new synthesis scope;
- reproduce before redesigning;
- preserve currently green audio, randomizer, timing, chord, feature-flag, UI-performance and target gates;
- display fixes must be covered by bounds/dirty-region tests, not screenshot judgement only.

### Do not implement inside WP-B

- Control Grammar v2 / REC-EDIT remapping;
- contextual black-key action rail;
- MOD navigation redesign;
- main-sequence 1 BAR timing model;
- distortion expansion;
- Reverb V3;
- Undo Last Random;
- ANALOG/FM engine work;
- SOUND randomization;
- per-step timbre;
- harmonic-layer redesign;
- splash/onboarding polish.

## WP-B completion condition

Before WP-B can close:

1. B1 live-edit audio reliability remains green;
2. B2 K1-K4 display/handler parity remains green;
3. B3 text fit and overlap pass at extreme formatted values;
4. HOME stale/pink-line artifact is eliminated and regression-covered;
5. existing WP-A LOW/MED/HIGH + skyline gates remain green;
6. Chord/Reverb technical regressions remain green;
7. 300 BPM timing, emulator/browser, 80 MHz and Chromium paths remain green;
8. full target build remains green with >=16 KiB RAM and pool headroom;
9. project docs record exact commit/run evidence.

## Stop condition

When WP-B is fully green:

- record exact implementation checkpoint and workflow run IDs;
- update `PROJECT_STATE.md`, `ROADMAP.md`, `TESTING.md` and `CHANGELOG.md`;
- set **WP-C Control Grammar v2** as the next active package;
- stop and hand off instead of beginning WP-C automatically.

## Approved future sequence after WP-B

1. WP-C Control Grammar v2: REC/EDIT cleanup, contextual black keys, MOD family.
2. WP-D Timing: default 1 BAR, explicit FREE, keep Loop Cycle/Slot/external sync concepts separate.
3. WP-E Audio character: extreme distortion, Reverb V3, ratchet parity, live smoothing.
4. WP-F Undo Last Random.
5. WP-G ANALOG/FM + SOUND randomization + per-step timbre.
6. WP-H scale-aware HARMONY first, PAD second, then compare alternatives.
7. WP-I splash/HOME/Key-Scale/SEQ/onboarding polish.
8. WP-J final integration/performance gate.

## Non-negotiable product principle

**Keep the depth, reduce the number of grammars.**

Do not remove useful functionality merely to make the interface simpler. Make related functions behave consistently and visibly instead.
