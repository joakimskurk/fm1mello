# PROJECT STATE

Last verified work-package closeout: 2026-10-28

## Repository

- Repo: `joakimskurk/fm1mello`
- Active branch: `visual-port-home`
- Verify current branch HEAD before editing.
- New chats start with `START_HERE.md`; repo documentation is source of truth.
- Firmware is reconstructed by `ci/unpack_source.sh` from source chunks plus ordered transforms/tests.

## Work-package position

- WP-A Randomizer intensity + skyline: **COMPLETE**.
- WP-B Trust and reliability: **COMPLETE**.
- WP-C Control Grammar v2: **COMPLETE**.
- **ACTIVE WORK PACKAGE: WP-D Timing model.**

Do not begin WP-E or later work before WP-D is green and documented.

## WP-C final implementation

Final implementation/test checkpoint: `2c280184b0c5cd0947f2d4d9149ad12a96188709`.

WP-C delivered:

- C1 complete deterministic action/encoder audit;
- C2 REC = record/capture only, with legacy actions migrated to contextual rail;
- C3 EDIT = edit/focus/lock only, with selected-position actions migrated;
- C4 visible B0-B7 contextual rail, hidden OCT/K4 action removal and coherent MOD navigation;
- C5 one authoritative page/slot descriptor shared by contextual handler semantics and rendered guidance.

C5 final descriptor coverage:

- Chord enabled: `16 pages / 31 active / 97 inactive` B0-B7 slots;
- Chord compiled out: `15 pages / 29 active / 91 inactive`;
- inactive slots are inert;
- every active hint is descriptor-derived;
- advertised B5 opens RANDOM on every mapped page;
- STEP `SEL+black` manual-note priority remains intact.

Exact C5 output locks:

- UI `f6d26e0d1a57a51b08bf65e878144f5d48bfa248244d97b80c7a0842eb601818`;
- authoritative mapping test `bf688e15845324e1982336830f20c067118d90923f8ba6da0b692c94dac4914d`;
- retained C4 context contract `b5479562eb9a4aca5817663e835c04151e0c3337e4c9b131cb4c675daeabddfd`.

## Final WP-C evidence

All 11 gates are green on `2c280184...`:

- authoritative actions `40703570244`;
- reconstruction `40703570315`;
- context grammar `40703570196`;
- REC grammar `40703570212`;
- EDIT grammar `40703570277`;
- action contract `40703570204`;
- control audit `40703570330`;
- rotary parity `40703570466`;
- display integrity `40703570254`;
- live-edit `40703570316`;
- full target `40703571071`.

Final target retained canonical randomizer distances, 300 BPM timing, Chord/Reverb technical gates, emulator/browser/Chromium, live-edit reliability and target resource limits. RAM headroom `21472 B`; pool headroom `17696 B`; image `190060 / 581564 B`; package `611545 B`; package SHA-256 `755a098ecbe66a6f8f7697a5787cb240e50a69df2520dc2f10b690f0056b8da1`; provenance PASS.

Physical FM-1 live-edit reproduction remains a later hands-on regression when hardware is available; host/emulator/target evidence remains green.

## Active WP-D direction

WP-D is the timing-model package. Approved rules:

- main sequence defaults to `1 BAR`;
- choices are `1 / 2 / 4 / 8 / 16 BAR / FREE`;
- main-sequence bar reset, external/MIDI sync, Loop Cycle `SYNC/FREE`, and Slot/Chain hold length remain distinct concepts;
- timing labels must make those distinctions explicit.

Begin WP-D with an audit/baseline of current transport reset behavior, default/storage representation and UI labels before implementation. Preserve all WP-A/WP-B/WP-C/audio/performance/target gates.

## Future sequence

WP-D Timing model -> WP-E Audio character -> WP-F Undo Last Random -> WP-G ANALOG/FM + SOUND randomization/per-step timbre -> WP-H harmonic layer decision -> WP-I polish/onboarding -> WP-J final integration.
