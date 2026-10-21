# Real FM-1 validation checklist

All boxes are mandatory before a public release.

- [ ] Exact upstream lock verified; unchanged Dinghy target baseline builds.
- [ ] SkurkyMello target build passes all Dinghy safety checks.
- [ ] Target report shows RAM < 98,304 B and pool < 344,064 B with useful headroom.
- [ ] Release `.fwsc` SHA-256 recorded and never replaced after validation.
- [ ] Exact `.fwsc` boots in `fm1-emulator`; screen, PLAY/STOP, keys, knobs, LEDs and sound are exercised.
- [ ] Real FM-1: install from official V15 succeeds.
- [ ] Real FM-1: install from another custom firmware succeeds.
- [ ] Identity request reports SkurkyMello's unique `FM-1_9NN`.
- [ ] USB MIDI IN/OUT and MIDI clock MASTER/SLAVE tested.
- [ ] Audio ISR measured with `fm1_ticks()` under worst-case load; target is comfortably below 2.9 ms / 256 frames.
- [ ] Maximum-ratchet + Matrix + Turing + sub + four sample slots + delay/reverb stress tested without crackle/reset.
- [ ] Project SAVE/reboot/load tested repeatedly.
- [ ] Power interruption during project journal write falls back correctly.
- [ ] Sample upload A–D works; replacement and reboot persistence verified.
- [ ] Power interruption during sample staging/commit recovers correctly.
- [ ] Fill flash storage through representative repeated saves/sample replacements; no updater/settings corruption.
- [ ] With SkurkyMello running, install another firmware successfully (critical escape/update path).
- [ ] Interrupt another firmware install during Writing, reconnect, and resume successfully.
- [ ] OCT− rescue verified.
- [ ] Back-to-official V15 verified.
