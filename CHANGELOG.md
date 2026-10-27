# CHANGELOG

## 2026-10-27 - WP-B1 live-edit audio reliability complete

B1 is complete on the currently available host/emulator/browser/target evidence. WP-B remains active; B2 rotary display/handler parity is next.

Implemented:

- odd/even pending-config publication so audio does not consume a partial live-edit snapshot;
- 32-bit config generation plus a 32768-publication alias regression;
- 257-point SUB drive-curve preparation moved out of normal audio render;
- selective derived-state invalidation for LFO, delay, reverb, distortion and SUB processing;
- permanent host live-edit stress across SOUND/FX/SUB/SAMPLE;
- real 80 MHz front-panel encoder live-edit stress in the target gate;
- corrected emulator encoder-event syntax from ineffective `encN` to the pinned emulator's `eN` format in existing web/80 MHz scripts.

Validation checkpoint: `b90d4fc23e2f0715d00544bdb5cc878404f8ee64`.

- dedicated live-edit run `40596812842`: PASS;
- reconstruction run `40596812864`: PASS;
- full target run `40596813400`: PASS;
- 2400-block host stress: no silence window, queue overflow or transport stall;
- 32768 coalesced config publications: PASS;
- 80 MHz physical encoder stress: `1.673x` steady realtime;
- final one-second stress tail: `88168` nonzero samples, peak `3902`;
- 100 MHz browser/WASM: `1.660x` steady realtime;
- 80 MHz SEQ/HOME: `1.756x` steady realtime;
- 80 MHz Chord + Reverb V2: `1.663x` steady realtime;
- RAM headroom `21472 B`; pool headroom `17696 B`;
- package SHA-256 `9d118b8559bd6fbd2bf89c3c763edb8a626da9ed75ec37e2324696d2ed5d72da`.

The exact originally reported physical FM-1 freeze cannot be hands-on re-auditioned until hardware is available. That limitation is retained explicitly in `TESTING.md`.

B2 audit already confirmed two real display/handler mismatches: GLOBAL K4 shows `REC / TAP` while toggling MIDI note out, and Loop Cycle STEP displays `TRANS`/`OCT` opposite to the K3/K4 handlers. B2 must build systematic parity coverage rather than patching only those strings.

## 2026-10-26 - WP-A randomizer intensity + RANDOM skyline complete

WP-A / Step 4 is complete and handed off.

Implemented:

- `LOW / MED / HIGH` randomizer intensity;
- MED preserves the previous/default randomizer implementation directly;
- LOW stays closer to current material and HIGH produces broader legal variation;
- intensity changes mutation depth/span without changing selected target masks;
- WAVE remains independently selectable and isolated;
- intensity is runtime UI state, so no project/storage migration was introduced;
- RANDOM page exposes intensity through K1;
- RANDOM skyline retains the full active 4..32-step sequence without paging, with pitch/event/playhead information under compression.

Validation:

- implementation commit: `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`;
- clean reconstruction closeout: `42cf506be5b127e7f03c763966d1e75cc4a48a85`;
- reconstruction run `40294234032`: PASS;
- target closeout run `40294234527`: PASS;
- measured randomizer distance baseline: `LOW=194824 < MED=516917 < HIGH=1082367`;
- RANDOM incremental redraw: `4 fills / 0 texts`;
- 4..32 skyline, WAVE isolation, legal-range and deterministic coverage: PASS;
- 300 BPM timing, Chord/Reverb regressions, emulator/browser/Chromium and 80 MHz gates: PASS;
- RAM headroom `21984 B`; pool headroom `17696 B`, both above the 16 KiB floor;
- product-identical package SHA-256: `fcd1fbf7624b50995f90267f4e168fe2a72328a8f7eeeb46c4982632cd999df0`.

Closeout repair:

- the temporary red reconstruction was traced to the old `randomizer_roadmap_probe.py` still being invoked and deliberately exiting non-zero;
- the WP-A patch itself was valid;
- the obsolete probe invocation was removed;
- temporary reconstruction tracing/artifact export was removed;
- correct WP-A base64/gzip integrity hashes plus exact source pre/post hashes now lock reconstruction.

## 2026-10-25 - approved next-iteration handoff

Product owner approved every recommendation from the 2026-10-25 product workshop. Documentation/handoff updates only; no firmware implementation changed in this approval checkpoint.

Locked product decisions included default `1 BAR`, REC/EDIT semantic cleanup, contextual black keys, MOD grouping, Reverb V3, broader distortion, ratchet parity, ANALOG/FM direction, SOUND randomization, per-step timbre, HARMONY/PAD exploration and splash-screen polish.

## 2026-10-25 - post-audition product review

Documentation/planning checkpoint after extended hands-on emulator testing. Added `docs/PRODUCT_WORKSHOP_2026-10-25.md` and the P0 trust/reliability backlog that became WP-B.

## 2026-10-23 - handoff checkpoint for new chat

Added the root continuity files and documented the then-active Step 4 randomizer work plus its intentional diagnostic reconstruction failure.
