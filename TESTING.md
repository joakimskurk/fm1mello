# TESTING

## Validation philosophy

A green technical metric is necessary but not sufficient for product acceptance. Deterministic tests, target/performance measurements and hands-on audition gates are all used where applicable.

The user does not currently have a physical FM-1, so host/emulator/web/target-build validation is the primary available path until hardware arrives.

## WP-A verified baseline

WP-A / Randomizer intensity + RANDOM skyline is complete.

- clean reconstruction `42cf506be5b127e7f03c763966d1e75cc4a48a85`, run `40294234032`: PASS;
- target run `40294234527`: PASS;
- canonical distance `LOW=194824 < MED=516917 < HIGH=1082367`;
- RANDOM redraw `4 fills / 0 texts`;
- WAVE isolation and 4..32 skyline: PASS.

## WP-B final verified baseline

WP-B is complete on available host/emulator/browser/target evidence.

Final implementation/test checkpoint: `7c20efbdc391c59118fef3f000689eeafaa176b9`.

Final workflow matrix:

- `Reconstructed source gate` run `40604213816`: PASS;
- `WP-B live-edit gate` run `40604213682`: PASS;
- `WP-B rotary parity gate` run `40604213768`: PASS;
- `WP-B display integrity gate` run `40604213707`: PASS;
- `Visual port target build` run `40604214660`: PASS.

The reusable target workflow's separate `Tests` step is skipped because no separate `test:` input is supplied; this is expected. `bash ci/build_target.sh` executes the required host/regression/emulator/browser/performance tests inside the successful Build step.

### B1 live-edit reliability

Final target regression:

- live-edit energy `433436686798`;
- transport tick `159600`;
- config applies `2470`;
- SUB curve builds `67`;
- reverb reconfigs `135`;
- no silence/transport stall;
- 80 MHz physical encoder stress `3.040x` steady realtime;
- final tail `88154` nonzero samples, peak `3207`.

The exact physical FM-1 symptom remains a hands-on regression when hardware becomes available.

### B2 rotary display/handler parity

Permanent real-encoder mapping gate remains green with Chord feature-on and compile-out. It covers every current page/layer and intentional action controls. Original coverage baseline is `180` K1-K4 mappings / `45` fixtures enabled and `160` / `40` compiled out.

### B3 display integrity

`tools/ui_display_integrity.c` models the pinned renderer's actual fixed-cell metrics:

- FONT_S: 8 px advance x 16 px height;
- FONT_L: 16 px advance x 32 px height;
- text boxes are destructive because the renderer clears the full box before blitting.

It drives legal extreme values through the real UI input path and asserts screen bounds, horizontal fit, text/text overlap, later graphic overdraw and HOME dirty-vs-clean equivalence.

Final B3 coverage:

- Chord enabled: `150` fixtures, `0` clips, `0` overlaps: PASS;
- Chord disabled: `135` fixtures, `0` clips, `0` overlaps: PASS;
- HOME reported `SINGLE`/pink-line collision: reproduced structurally and eliminated;
- HOME incremental protected region equals clean redraw: PASS;
- full RANDOM `WAVE` label remains present and unclipped;
- target freestanding libc guard: PASS.

### Full WP-B regression / performance closeout

Run `40604214660`:

- universal STEP/RANDOM/SEGMENT 32-step overview: PASS;
- source-parity CLEAR/TAP/RESET: PASS;
- Reverb V2 IR: dense `42497`, corr² `0.00075`, jumpE `2.279`, predelay delta `5292`;
- SUB HTML parity: PASS;
- Chord technical/compile-out acceptance: PASS;
- 300 BPM ratchet/chord/sub/sample event-order stress: PASS;
- WP-A enabled: `LOW=194824 < MED=516917 < HIGH=1082367`, redraw `4/0`: PASS;
- WP-A Chord-off: `LOW=180984 < MED=484357 < HIGH=1038671`, redraw `4/0`: PASS;
- emulator panel/audio: `189440` frames, peak `3530`, `148408` nonzero samples: PASS;
- browser/WASM 100 MHz: `3.403x` steady realtime, steady render p95 `2.530 ms`, p99 `3.419 ms`, max `10.426 ms`: PASS;
- SEQ/HOME 80 MHz: `3.528x` steady realtime, steady render p95 `2.390 ms`, p99 `3.495 ms`, max `7.704 ms`: PASS;
- Chord + Reverb V2 80 MHz: `3.306x` steady realtime, steady render p95 `2.604 ms`, p99 `3.401 ms`, max `4.793 ms`: PASS;
- live-edit 80 MHz: `3.040x` steady realtime, steady render p95 `2.897 ms`, p99 `4.216 ms`, max `10.519 ms`: PASS;
- Chromium keyboard/pointer incl. PLAY start-stop: PASS;
- image `190756 B / 581564 B`;
- RAM `76832 B / 98304 B`, headroom `21472 B`;
- pool `326368 B / 344064 B`, headroom `17696 B`;
- package `611545 B`, SHA-256 `b0bf86215ab4c547c668c54c3f023ff05fb7f4e87b728bcb7e84365a02cebf1e`;
- provenance attestation: PASS.

## Active WP-C testing direction

Before remapping controls, create a deterministic action-map baseline covering every page/layer and REC/EDIT/SELECT/contextual-action path. WP-C must prove rendered guidance and actual handlers remain in parity after the grammar is simplified.

WP-C must preserve all WP-A/WP-B tests above plus audio/timing, emulator/browser, 80 MHz, Chromium and target memory gates.

## Reverb V2 technical baseline

Retained as regression evidence, not final perceptual acceptance. Reverb V3 is later WP-E and requires perceptual listening gates.

## Chord Machine regression baseline

Held `ARP_UP` remains protected: `12` note-ons, `12` note-offs, `3` distinct pitches, and clean compile-out with `MELLO_FEATURE_CHORD_MACHINE=0`.

## Test reporting rule

At each work-package closeout record exact checkpoint SHA, workflow run IDs, relevant measured gates, hands-on audition result where required, and intentionally skipped hardware-only validation.
