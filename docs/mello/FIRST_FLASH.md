# First install on an FM-1

The first time a build of this repository goes onto real hardware. About 30 minutes. Every install can brick
a unit; the steps below are ordered so the rescue path is proven before anything else is relied on.

## Before

- The FM-1 runs **official V15** (or another firmware with the FM-1_9NN update loader). On older stock firmware,
  update with M-VAVE's own updater first.
- Anything on the unit you want to keep (stock presets, another firmware's projects) is backed up with that
  firmware's own tools. SkurkyMello uses its own flash area; other firmwares may not survive a switch.
- A good USB cable, straight into the computer (no hub), and stable power for the whole write.
- `pip3 install mido python-rtmidi` for the installer, and a fresh package: `make test && ./build.sh --release 0.2-p1`.
- Know the rescue: **hold OCT- while switching on** = USB rescue (no sound, only the updater). If the screen stays
  black and OCT- does nothing, the ROM UBOOT tools are the next step (fm1-firmware-dev skill, "Recovery ladder").

## Install

```sh
python3 tools/fm1_install.py --info                       # the FM-1 is found and says what it runs
python3 tools/fm1_install.py build/skurkymello-0.2-p1.fwsc
python3 tools/fm1_install.py --info                       # now FM-1_970
```

## Check, in this order

| # | What | Pass |
|---|---|---|
| 1 | Boot | SKURKYMELLO HOME screen within a few seconds, no crash screen |
| 2 | Rescue works | Power off, hold OCT-, power on: the rescue screen. Power cycle normally: back to HOME |
| 3 | Sound | PLAY: the default pattern sounds, no clicks or crackle on the default patch |
| 4 | P0 corner | SOUND: square wave, follower sub square at level 1000, cutoff 900+, play, STOP: the output goes quiet (before: a loud stuck DC until reboot) |
| 5 | Autosave | Change a step's pitch, the cutoff and the reverb mix. STOP, wait ~15 s (reverb tail + guard + debounce), power off, power on: all three changes are back |
| 6 | SAVE | Change something, STOP, press SAVE, wait for the tail, power cycle: the change is back |
| 7 | Chord Machine | Change chord mode and level, wait as in 5, power cycle: kept |
| 8 | Load | Play the default pattern with FX for 10 minutes: no dropouts, no crash, the unit is not hot |
| 9 | Way back | Install official V15 again (M-VAVE's updater or the Firmware Hub), it boots; then this build again, and the project from 5–7 is still there |

Write down the results (and the package's SHA-256) in `docs/mello/HARDWARE_VALIDATION_CHECKLIST.md`.

## If something fails

- Crash screen: note the PC it shows, then hold OCT- and reinstall the last build that worked (or V15).
- No sound, no screen: hold OCT- at power on and reinstall. If that does not come up, stop and use the ROM UBOOT
  recovery (`fm1_recover.py` from Melodee, or the MvaveFM1Unbricker on Windows).
- Changes not kept after a power cycle: wait longer before switching off (the unit only writes while stopped and
  silent; with the factory reverb that is about 12 s after the last note), then report it with the steps.
