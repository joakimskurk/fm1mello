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
- target RAM headroom `21472 B`; pool headroom `17696 B`;
- target package SHA-256 `9d118b8559bd6fbd2bf89c3c763edb8a626da9ed75ec37e2324696d2ed5d72da`.

B1 fix direction:

- stable odd/even generation publication prevents audio from consuming a partial pending config snapshot;
- generation counters are 32-bit and regression-tested beyond the old 16-bit alias distance;
- the 257-point SUB distortion curve is precomputed outside audio render and atomically applied;
- LFO, delay, reverb, distortion and SUB derived state are rebuilt only when their relevant inputs change;
- no normal live rotary path allocates memory.

The exact user-reported physical-hardware freeze cannot be hands-on re-auditioned until a physical FM-1 is available. Host, emulator, browser and target evidence are green and the hardware audition remains an explicit later regression check.

## Active WP-B subtask

**B2 - Rotary display/handler parity.**

Start here. Do not begin B3 or WP-C early.

### Confirmed B2 reproductions already found

1. `GLOBAL` subpage 0: physical K4/R4 toggles MIDI note output, while the footer currently renders `REC / TAP`.
2. Loop Cycle STEP: physical K3 changes `OCT` and K4 changes `TRANS`, while the footer currently presents K3=`TRANS` and K4=`OCT`.

Treat these as proof that a systematic mapping gate is required. Do not fix only the two visible strings and call B2 done.

### B2 goals

- validate every page/layer K1-K4 mapping;
- rendered label/value under Kn must describe the parameter physical rotary n actually changes;
- legal min/max, direction and useful display update behavior must agree with the handler;
- explicit momentary/action rotaries must be represented intentionally rather than hidden as false parameter mappings;
- build one authoritative/testable mapping fixture or source where practical;
- add deterministic host coverage so future label/handler drift fails CI.

### Remaining WP-B work order

1. **B2 active:** rotary display/handler parity.
2. B3: remove text clipping/overlap at extreme values.
3. B3: remove the static pink-line artifact reported over `SINGLE` on HOME.
4. Extend mapping/text-fit/stale-region coverage and close WP-B only when all trust gates are green.

### WP-B principles

- trust/reliability before new synthesis scope;
- reproduce before redesigning;
- preserve currently green audio, randomizer, timing, chord, feature-flag, UI-performance and target gates;
- normal live rotary movement must not allocate/rebuild DSP in an unbounded way;
- one authoritative mapping source should drive or validate both displayed K1-K4 semantics and handlers where practical;
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

Those directions are approved but belong to later work packages.

## WP-B completion condition

Before WP-B can close:

1. B1 live-edit audio reliability remains green;
2. K1-K4 display/handler parity is validated across page/layer mappings;
3. text fit and overlap pass at extreme formatted values;
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
