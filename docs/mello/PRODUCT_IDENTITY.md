# SkurkyMello FM-1 product identity

Current project identity: **`FM-1_987`** (since 0.5-f98, 2026-11-07).

It was `FM-1_970` from 2026-10-21. Checked against the Firmware Hub's live catalogue on 2026-11-07, before the first
public release: zp12 2.5 holds the range `FM-1_97[0-689]`, so 970 collided. The free `FM-1_9NN` numbers that day
were 922, 954, 956, 959, 962, 963, 965, 967, 984, 986, 987, 988 and 989; 987 was picked. (The hub's submit page
checks a number against every firmware it lists; `tools/fm1_install.py` wants the loader's `ota-FM-1_9NN`, so it stays
three digits.)

An FM-1 running an older SkurkyMello answers `FM-1_970` until 0.5-f98 or later is installed; the installer takes the
new package over it as over any other firmware.

Re-check the live catalogue before each tagged release. Dinghy's own `FM-1_99N` range is refused by `tools/build.py`.
