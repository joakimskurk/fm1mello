# PROJECT STATE

Last product-owner approval/handoff: 2026-10-25

## Repository

- Repo: `joakimskurk/fm1mello`
- Active branch: `visual-port-home`
- Product-code checkpoint before the 2026-10-25 product review: `1c0748c2140e106e0c38fa0637b4c06139c2625f`
- Product-review documentation checkpoint before approval lock: `45df96067c7232491ee38610dc179b8dfbb56e5f`
- Always resolve current branch HEAD before editing.
- New chats should start with root `START_HERE.md`.

## Current implementation position

The authoritative detailed roadmap is `docs/NEXT_ITERATION_ROADMAP.md`.

**ACTIVE NEXT WORK PACKAGE: WP-A / Step 4 - Randomizer intensity + RANDOM skyline refinement.**

Do not skip directly into Control Grammar v2, FM synthesis or Reverb V3. A P0 reliability defect may interrupt WP-A only if it prevents meaningful implementation/testing.

After WP-A is green, the next package is **WP-B Trust and reliability**, with live-edit audio freeze as the highest-priority blocker.

## Product-owner decisions locked 2026-10-25

All recommendations in `docs/PRODUCT_WORKSHOP_2026-10-25.md` were approved.

Locked requirements:

- main sequence defaults to `1 BAR`; FREE remains explicit;
- REC is reserved for real recording/capture;
- EDIT is reserved for edit/focus/lock behavior;
- legacy REROLL/TAP/RESET/CLEAR actions must migrate to visible contextual actions without losing functionality;
- black keys are the approved contextual action rail where useful;
- LFO + DOT belong under a common `MOD` concept;
- Turing belongs to the broader modulation/generative family while retaining its own identity;
- Reverb V3 is required because V2 remains perceptually metallic/ring-mod-like despite technical acceptance;
- distortion gets an intentionally extreme upper range with gain management;
- ratchet audibility is a parity/timing/envelope-retrigger task first;
- current synth is preserved as `ANALOG` and later gains curated `FM` mode;
- `SOUND` becomes an independent future randomizer target;
- per-step timbre locks/modulation are approved after stabilization;
- harmonic-layer prototyping begins with scale-aware `HARMONY`, then soft filtered `PAD`;
- current Chord remains regression-protected until a successor wins by audition;
- splash screen is required before final polish acceptance.

## Product status after hands-on audition

FM-1 Mello is already musically useful and produces compelling loops. The main product problem is not lack of functionality. It is inconsistent interaction grammar plus a few trust-breaking audio/UI defects.

Detailed review: `docs/PRODUCT_WORKSHOP_2026-10-25.md`.

### P0 trust/reliability backlog

- Audio can freeze while parameters are turned; STOP/PLAY often recovers it.
- Some R1-R4 rendered labels do not match the rotary handler.
- Text clipping/overlap exists on some screens.
- A static pink line can appear over `SINGLE` on HOME.

These are WP-B blockers before major synth expansion.

### Timing model

Keep these concepts separate in names, state and UI:

1. external clock/MIDI sync;
2. main-sequence `BAR RESET / FREE`;
3. Loop Cycle `SYNC / FREE`;
4. Slot/Chain hold length.

Approved main-sequence default: `1 BAR` = reset after 16 sixteenth-note ticks.

### Reverb

Reverb V2 remains the technically validated regression baseline, but is not product-accepted. Reverb V3 is an approved later work package with perceptual listening as a hard gate.

### Ratchet

Original HTML retriggers the synth envelope for every ratchet sub-trigger. Native ratchet behavior must be checked against that reference before redesign.

### Synth/harmonic future

After WP-A through WP-E stabilization:

- ANALOG remains intact;
- curated FM is added under hard CPU/RAM budgets;
- SOUND randomization and per-step timbre are added safely;
- scale-aware HARMONY and PAD are prototyped before changing/removing current Chord.

## WP-A / Step 4 requirements

Randomizer is a primary creative instrument.

- `LOW`: conservative variation around current material.
- `MED`: preserve existing/default randomizer character as closely as practical.
- `HIGH`: larger structural/timbral movement while remaining musical.
- intensity changes target-specific mutation depth/span, never target selection.
- `WAVE` remains independently selectable.
- preserve the existing RNG call order for MED where practical.
- compact full 4..32-step overview remains visible.
- RANDOM skyline becomes denser: 16 steps comfortably legible, 17..32 still visible without display paging.
- pulse/ratchet/event information must remain meaningful under compression.
- do not implement Undo, FM, Control Grammar v2 or Reverb V3 inside WP-A.

## Current technical state

`ci/randomizer_roadmap_probe.py` is still diagnostic and deliberately ends with:

`raise SystemExit('RANDOMIZER ROADMAP PROBE V2 COMPLETE')`

Known diagnostic run at product-code checkpoint `1c0748c2140e106e0c38fa0637b4c06139c2625f`:

- Workflow run: `39925230733`
- Workflow: `Reconstructed source gate`
- Result: `failure`
- Reason: intentional probe exit after printing findings.

This intentional failure must be removed/disabled when WP-A findings have been converted into real deterministic tests. WP-A cannot be declared green while the production gate intentionally fails.

## Source reconstruction model

Firmware source is reconstructed by `ci/unpack_source.sh` from source chunks plus an ordered patch chain. New functional work must follow that patch/test architecture. Do not assume reconstructed source is committed as ordinary source files.

## New-chat procedure

Read `START_HERE.md` first. It gives the exact file order, active work package and stop conditions.

## Definition of done for WP-A

WP-A is not complete until:

- LOW/MED/HIGH exists in engine behavior and FM-1 UI;
- MED preserves previous creative behavior as closely as practical;
- intensity never changes the selected target mask;
- WAVE can be randomized independently;
- values remain legal at every sequence length 4..32;
- deterministic/reproducible tests exist where RNG architecture permits;
- RANDOM shows the whole active sequence with useful density at 4..32 steps;
- redraw performance remains within established baseline;
- existing chord/reverb technical regressions and target build remain green;
- intentional diagnostic probe failure is removed from the real gate;
- exact commit and workflow/run evidence is recorded in project/testing docs;
- stop after WP-A and hand off to WP-B rather than silently starting later packages.
