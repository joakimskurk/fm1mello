# Real FM-1 validation checklist

All boxes are mandatory before a public release.

- [ ] Exact upstream lock verified; unchanged Dinghy target baseline builds.
- [ ] SkurkyMello target build passes all Dinghy safety checks.
- [ ] Target report shows RAM < 98,304 B and pool < 344,064 B with useful headroom.
- [ ] Release `.fwsc` SHA-256 recorded and never replaced after validation.
- [ ] Exact `.fwsc` boots in `fm1-emulator`; screen, PLAY/STOP, keys, knobs, LEDs and sound are exercised.
- [ ] Real FM-1: install from official V15 succeeds.
- [ ] Early, at -O2 (DECISIONS "Before the first release"): USB-MIDI IN from a computer, notes and clock for minutes,
      no stuck or doubled notes ("babble"). If it babbles, build with `MELLO_CFLAGS=-Os` and try that package.
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
- [ ] The update loader's screen (Dinghy 0.5's, since 0.5-f99): SKURKYMELLO in green and UPDATE MODE while it
      waits, WRITING with its bar and percent during an install, UPDATE FAILED after a cut one; the install still
      finishes (the loader draws before it starts USB).
- [ ] OCT− rescue verified: held at power-on it opens the USB rescue (the boot scan settles 100 us since 0.5-f98).
- [ ] The lights from the start (since 0.5-f101): after the splash HOME's button lit and the steps glowing; after
      RESET ALL, HOME's lights and none left from the page before.
- [ ] CORE 2 (since 0.5-f108, OFF by default; DECISIONS "The second core"): OFF, GLOBAL > SYSTEM says 1 CORE
      and nothing else changes. ON, after a power cycle: 2 CORES (NO REPLY or GAVE UP leave one core, the sound as
      it was); with it ON, a project save while playing, an install of another firmware and the OCT- rescue; OFF
      again after a power cycle.
- [ ] Back-to-official V15 verified.
