# ROADMAP

This is the short handoff index. Detailed scope lives in `docs/NEXT_ITERATION_ROADMAP.md`; new chats start with `START_HERE.md`.

## Completed

### WP-A - Randomizer intensity + RANDOM skyline

Complete. Implementation `ec48a8d6ddadd3376c2cb58ae36c27ab29177c07`; reconstruction closeout `42cf506be5b127e7f03c763966d1e75cc4a48a85`.

### WP-B - Trust and reliability

Complete. Final implementation/test checkpoint `7c20efbdc391c59118fef3f000689eeafaa176b9`; full target `40604214660`: PASS.

### WP-C - Control Grammar v2

**COMPLETE 2026-10-28.** Final implementation/test checkpoint `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

C1-C5 delivered the complete control-language migration:

- complete pre-remap audit and deterministic baseline;
- REC reserved for real recording/capture;
- EDIT reserved for edit/focus/lock;
- visible stable B0-B7 contextual action rail;
- hidden RANDOM OCT, GLOBAL OCT-reset and LOOP K4 action debt removed;
- coherent MOD navigation for Audio LFO, DOT and EVOLVE/Turing;
- SELECT remains layer navigation and manual note-entry exceptions remain musical;
- one authoritative `MelloContextAction` descriptor drives both contextual handler verbs and rendered labels.

C5 parity: `16 pages / 31 active / 97 inactive` slots with Chord, `15 / 29 / 91` compiled out. Exact C5 UI SHA `f6d26e0d1a57a51b08bf65e878144f5d48bfa248244d97b80c7a0842eb601818`.

Final green runs: authoritative `40703570244`, reconstruction `40703570315`, context `40703570196`, REC `40703570212`, EDIT `40703570277`, action `40703570204`, audit `40703570330`, rotary `40703570466`, display `40703570254`, live-edit `40703570316`, target `40703571071`.

Target resource closeout: RAM headroom `21472 B`, pool headroom `17696 B`, package SHA-256 `755a098ecbe66a6f8f7697a5787cb240e50a69df2520dc2f10b690f0056b8da1`.

## Active implementation

### WP-D - Timing model - ACTIVE

Approved target:

- main-sequence default **`2 BAR`**;
- expose `1 / 2 / 4 / 8 / 16 BAR / FREE`;
- BAR values remain musical bar lengths when pulse division changes;
- external/MIDI sync remains independent;
- Loop Cycle `SYNC/FREE` remains independent;
- Slot/Chain hold length remains independent;
- UI terminology must not collapse these into one generic sync concept.

D1 audit has captured the current model: legacy main reset is `master_len` in processed pulses, `0` means natural/free, and transport sync / Loop Cycle sync / hold counters are separate. Permanent baseline coverage lives in `tools/wp_d_timing_baseline.c`; full findings live in `docs/WP_D_TIMING_AUDIT.md`. D1 CI closeout is the immediate gate, followed by D2.

D2 implements the smallest compatible migration to the approved bar model while preserving all other timing concepts and all WP-A/WP-B/WP-C gates.

Stop after WP-D closeout and activate WP-E; do not jump ahead.

## Approved sequence after WP-D

- WP-E Audio character: extreme bounded distortion, Reverb V3, ratchet parity, live smoothing.
- WP-F Undo Last Random.
- WP-G ANALOG/FM + SOUND randomization + realtime-safe per-step timbre.
- WP-H harmonic layer decision: HARMONY first, PAD second.
- WP-I instrument polish/onboarding.
- WP-J final integration/performance gate.

**Product rule: Keep the depth, reduce the number of grammars.**
