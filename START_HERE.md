# START HERE - FM-1 Mello continuation

Use this file at the start of every new ChatGPT work session on `joakimskurk/fm1mello`.

## Repository

- Repo: `joakimskurk/fm1mello`
- Branch: `visual-port-home`
- Always fetch and verify the current branch HEAD before editing.
- Do not reconstruct project direction from old chat memory when repo documentation is available.
- Follow the active work package below. Do not jump ahead.

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

**WP-B - Trust and reliability: COMPLETE.**

Final implementation/test checkpoint: `7c20efbdc391c59118fef3f000689eeafaa176b9`.

WP-B delivered three completed gates:

- B1 live-edit audio reliability;
- B2 K1-K4 rendered-display / handler parity;
- B3 display integrity, text fit and HOME stale-artifact protection.

Final WP-B evidence on the implementation checkpoint:

- `Reconstructed source gate` run `40604213816`: PASS;
- `WP-B live-edit gate` run `40604213682`: PASS;
- `WP-B rotary parity gate` run `40604213768`: PASS;
- `WP-B display integrity gate` run `40604213707`: PASS;
- `Visual port target build` run `40604214660`: PASS.

B3-specific acceptance:

- Chord enabled: `150` display-integrity fixtures, `0` clips, `0` overlaps;
- Chord compiled out: `135` fixtures, `0` clips, `0` overlaps;
- HOME incremental redraw matches clean redraw in the protected region;
- the reported pink line over `SINGLE` was reproduced as a real 16 px font/layout collision and removed;
- full `WAVE` remains visible on RANDOM without clipping;
- freestanding target contains no new libc string dependency.

Final full-target measurements:

- WP-A randomizer: `LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4 fills / 0 texts`;
- 300 BPM event-order stress: PASS;
- live-edit host stress: energy `433436686798`, transport tick `159600`, `2470` config applies;
- emulator panel/audio: PASS, peak `3530`, `148408` nonzero samples;
- browser/WASM 100 MHz: `3.403x` steady realtime;
- SEQ/HOME 80 MHz: `3.528x` steady realtime, steady render p95 `2.390 ms`, p99 `3.495 ms`, max `7.704 ms`;
- Chord + Reverb V2 80 MHz: `3.306x` steady realtime;
- physical encoder live-edit stress 80 MHz: `3.040x` steady realtime, tail `88154` nonzero samples, peak `3207`;
- Chromium keyboard + pointer routes incl. PLAY start/stop: PASS;
- image `190756 B / 581564 B`;
- RAM `76832 B / 98304 B`, headroom `21472 B`;
- pool `326368 B / 344064 B`, headroom `17696 B`;
- package `skurkymello-0.1-dev.fwsc`, `611545 B`;
- package SHA-256 `b0bf86215ab4c547c668c54c3f023ff05fb7f4e87b728bcb7e84365a02cebf1e`;
- build provenance attestation: PASS.

The exact user-reported physical-hardware live-edit freeze cannot be hands-on re-auditioned until a physical FM-1 is available. Keep that as an explicit later hardware regression check; do not reopen WP-B without a demonstrated regression.

## Active work package

**WP-C - Control Grammar v2.**

Start here. Do not begin WP-D early.

### WP-C goal

Keep the instrument deep while reducing page-specific control grammars. REC must mean recording/capture, EDIT must mean edit/focus/lock, SELECT remains functional-layer navigation, and unrelated page actions must become visible/contextual rather than hiding behind inconsistent button meanings.

### WP-C work order

1. Audit the complete current REC / EDIT / SELECT / black-key / page-action map against `CONTROL_GRAMMAR.md` and create a deterministic baseline test before changing behavior.
2. Move REC to actual recording/capture behavior only.
3. Move EDIT to edit/focus/lock behavior only.
4. Migrate unrelated page actions to visible contextual actions; use the black-key action rail where the approved grammar calls for it.
5. Keep SELECT as functional-layer navigation and organize LFO + DOT coherently under MOD; treat Turing as part of the broader modulation/generative family.
6. Make one authoritative/testable page-action mapping source drive both behavior and rendered guidance where practical.
7. Rerun WP-A, all WP-B gates, audio/timing, emulator/browser, 80 MHz, Chromium and target gates.

### WP-C acceptance

- REC has no unrelated page-action overloads;
- EDIT has no unrelated page-action overloads;
- SELECT remains consistent layer navigation;
- contextual actions are visible and deterministic;
- rendered hints/guidance match actual handlers;
- all current page/layer action mappings are covered by automated parity tests;
- B1/B2/B3/WP-A/audio/timing/performance/target gates remain green;
- target retains at least `16 KiB` RAM and pool headroom.

### Do not implement inside WP-C

- WP-D timing redesign / default 1 BAR;
- distortion expansion;
- Reverb V3;
- Undo Last Random;
- ANALOG/FM engine expansion;
- SOUND randomization;
- per-step timbre;
- harmonic-layer replacement;
- splash/onboarding polish beyond guidance required by the control grammar itself.

## Stop condition

When WP-C is fully green:

- record exact implementation checkpoint and workflow run IDs;
- update project/testing docs;
- set **WP-D Timing model** as the next active package;
- stop and hand off instead of beginning WP-D automatically.

## Approved future sequence

1. WP-C Control Grammar v2.
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
