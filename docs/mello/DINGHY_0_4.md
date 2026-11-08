# SkurkyMello and Dinghy 0.4 / 0.5

Review of 2026-11-06. SkurkyMello `main` at `3976141` (0.5-f96), built on Dinghy 0.3 (merge base `d44df7d`). Upstream
`zvenson/dinghy` `main` at `f77bf03`: Dinghy 0.4 (`7c31bb2`, 41 files, +4001 / -238), `22acb91` (CI) and `f77bf03`
(the UC25HQ80 flash chip). This is a report only: nothing here changes the firmware.

## Dinghy 0.5, taken (2026-11-08)

Dinghy 0.5 (`ea1b6bf`, 2026-11-07) is 0.4, the UC25HQ80 flash chip (`f77bf03`), a README line and a fix to its
release workflow. Its firmware brings nothing 0.4 did not have. The "worth taking" list below is now done, one commit
each, in 0.5-f99:

| Step | Commit | On the FM-1 |
|---|---|---|
| 1. The newer flash chip | `b0672a0` (0.5-f97) | Newer units save to flash and take updates |
| 2. The boot scan's 100 us | `bf74925` (0.5-f98) | OCT- at power-on reads (the USB rescue) |
| 3. `-O2` or `-Os` | `ca4b854` (DECISIONS "Before the first release") | `-O2` stays; USB-MIDI IN early on the checklist |
| 4. The flash map, `FL_JEDEC_OK` | `0989f2e` | None: the same package, byte for byte |
| 4. The no-op syncs (`ota.c`, `bootguard.h`, `ldr_core.c`, `fm1_input.h`) | `66f4194` | None: the same package, byte for byte |
| 4. The USB buffers aligned to 32 | `0a9cb28` | RAM +64 B (padding) |
| 5. The installer and its test | `3842ffa` | None (a host tool); `make test` runs `install_test.py` |
| 6. The update loader's screen | `7192c2b` | The loader draws SKURKYMELLO, UPDATE MODE, WRITING, UPDATE FAILED |

`firmware/hal/` is now 0.5's but for `smk_input.h` (the SMK-37's, not taken). `ota.c`, `bootguard.h`,
`recovery.c` (but for the chip id read once) and `ldr_core.c` are 0.5's; `loader.c`, `lcd.c`, `usb.c` and `platform.c`
differ by SkurkyMello's hooks (the Big screen, its storage window, the chip id read once). `FL_JEDEC_OK(id)` names its
argument twice, so every call site reads the chip into a variable first: upstream's `FL_JEDEC_OK(fl_jedec_ram())` reads
it twice on a UC25HQ80 (harmless, but two flash sessions).

Still not taken, as below: the SMK-37 port, blobs and slots (`store.h`), `FELUCCA_UAC 0`, `LCD_IDLE` (later, if
the keys lag while the screen redraws), `CV_MAX`'s `#ifndef` (tests/host.c sets its own), and the OTA and loader host
tests (`ota_test.c`, `ldr_test.c`: they need a built package, so the toolchain). The loader's screen cannot run in
fm1-emu (it boots apps, not the loader alone): `tests/ldr_screen_test.c` draws it on the host, and the hardware
checklist has an item for it.

## In short

SkurkyMello already has the one 0.4 feature it needed (TRS MIDI IN, `3e477bb`, `midi_uart.c` byte for byte as
upstream). Most of the rest of 0.4 is the SMK-37 port, which SkurkyMello does not need. Worth taking, in order:

1. **The newer flash chip** (`f77bf03`). Done in 0.5-f97 (`b0672a0`). Without it, newer FM-1s saved to RAM only.
2. **The boot scan's 100 us settle time** (`FM1_SETTLE_US`). 0.4 says 10 us "read no key on an FM-1". So the OCT-
   USB rescue at power-on may not work on real units. Two lines.
3. **A decision on `-O2`.** SkurkyMello builds with `-O2`. 0.4 found that `-O2` made the FM-1's USB-MIDI IN babble
   on the device, and says to keep `-Os`. Measure `-Os` in fm1-emu, then decide.
4. **`fm1_flash_map.h`** and the small Dinghy-file syncs that merge cleanly. No change on the FM-1, but later syncs
   get easier.
5. **The installer** (`tools/fm1_install.py`) and its self-contained test.
6. **The update loader's screen** (`ldr_screen.c`). It needs one fix for SkurkyMello's Big screen hooks in `lcd.c`.

Skip: the SMK-37, blobs and slots, `FELUCCA_UAC 0`, and the changes to Dinghy's demo app (`dinghy.c`: MIDI channel,
clock display). Later, with your OK: the tag-driven CI release with attestation.

A dry-run merge of all of `upstream/main` into `main` (`git merge-tree`) conflicts in six files: `platform.c`,
`dinghy.c`, `tests/host.c`, `tools/build.py`, `README.md` and `LICENSING.md`. The UC25HQ80 fix (0.5-f97) adds
`fm1_flash.h`, `loader.c` and `recovery.c` to that list. Do not merge the whole release. Take the pieces below, one
commit each.

## Per change

| 0.4 change | Files | SkurkyMello has it? | Take it? | Conflicts expected |
|---|---|---|---|---|
| TRS MIDI IN: UART1 on PH8, parser, cable 1, `midi_get` returns 2 | `midi_uart.c`, `platform.c` | **Yes**, `3e477bb` (0.5-f28); `midi_uart.c` identical | Done | None |
| Clock and transport reach the app (F8 FA FB FC) | `midi_uart.c`, `usb.c` (already in 0.3) | **Yes**: `mello_transport` follows external clock in SLAVE | Done | None |
| MIDI channel (OMNI / 1..16, kept in flash), the clock's BPM on screen | `dinghy.c` (Dinghy's app) | Own design: two channels, own UI | No | Would conflict throughout `dinghy.c` |
| UC25HQ80 flash (0xB36014) beside the Puya 0x856014 | `fm1_flash_map.h` `FL_JEDEC_OK` (`f77bf03`); `platform.c`, `recovery.c`, `loader.c` | **Yes**, 0.5-f97 (`b0672a0`), as `fl_jedec_ok()` in `fm1_flash.h` | Done | Upstream's name and place differ (see note 1) |
| Boot scan settle 10 -> 100 us (OCT- rescue at power-on) | `fm1_input.h` (`#ifndef FM1_SETTLE_US`), `platform.c` (`#define FM1_SETTLE_US 100u`) | **No** | **Yes** | None. `fm1_input.h` auto-merges with SkurkyMello's `fm1_led_glow_key` |
| `-Os` kept on purpose: `-O2` made USB-MIDI IN babble on the device | `tools/build.py` (a comment), `usb.c` (a note) | **No**: `-O2` since the import (`b3728e6`, one of the six target patches) | **Decide** (see note 2) | `build.py` CFLAGS line |
| Flash map in one header; `FL_GUARD` in the RAM driver; `FL_PLAIN_LO` | `fm1_flash_map.h` (new), `fm1_flash.h`, `fm1_cc.h` | No | **Yes**: no FM-1 change (`FL_GUARD` = `FL_RANGE_OK` on the FM-1) | `fm1_flash.h` against the UC25HQ80 fix. SkurkyMello's own `FL_RANGE_OK` in `platform.c` still wins (`#ifndef`) |
| USB endpoint buffers aligned to 32 | `usb.c` | No | Yes, as a sync (upstream says harmless, not a cure) | Auto-merges with the mirror's `usb.c` hunks |
| `OTA_PARM_ID`; `FM1_OCT_BOTH`, `FM1_NBTN` | `ota.c`, `bootguard.h`, `fm1_input.h` | No | Yes, as a sync: same bytes on the FM-1 | None |
| Update loader's screen: UPDATE MODE, WRITING with a bar, UPDATE FAILED | `loader/ldr_screen.c` (new), `loader.c`, `fm1_lcd_hw.h`, `build.py` (`SCREEN_NAME`, `SCREEN_COLOUR`) | No: the screen keeps the app's last frame while the loader writes | **Yes** | `loader.c` now includes `../src/lcd.c`, and SkurkyMello's `lcd.c` calls `mirror_fill` / `mirror_blit_swapped` (see note 3) |
| `LCD_IDLE()`: the app's work while a strip goes out by DMA | `lcd.c`, `fm1_lcd_hw.h` (`fm1_lcd_done`) | No | Later, if keys or the arp lag while the screen redraws. MIDI in is already filled from the ISR | `lcd.c` auto-merges with the mirror hooks |
| `CV_MAX` overridable (`#ifndef`) | `platform.c` | SkurkyMello edits the line to `240 * 32` | With any `platform.c` sync: define it before the include | One line |
| `fm1_led_key_glow` (the dim layer) | `platform.c` | **Yes**, as `fm1_led_glow_key` in `fm1_input.h` (G5) | Optional rename, so `fm1_input.h` goes back to upstream's | None |
| Blobs and slots: `settings_*`, `blob_*`, `slot_*`, `store.h`, `STORE_MAGIC` | `store.h` (new), `platform.c` | Own storage: `mello_storage` banks 0x98000..0xB7FFF, samples 0xB8000..0xDFFFF | **No** (see note 4) | Dinghy's slots (0x97000..0xB6FFF) overlap the Mello banks. `store.h` defines `FL_RANGE_OK` unconditionally |
| `FELUCCA_UAC 0` (MIDI only), `UAC_ONLY` | `platform.c` | No | No: SkurkyMello records over USB audio | — |
| SMK-37 Pro / Elite / MKE-P37: board layer, pads, faders, wheels, LCD pins, flash layout, owner tag | `smk_input.h`, `smk_wheel.h`, `#if BOARD_SMK37` in `platform.c`, `lcd.c`, `fm1_lcd_hw.h`, `ota.c`, `ldr_core.c`, `loader.c`, `fm1pkg_make.py`, `build.py` | No | **No**, unless an SMK port of SkurkyMello becomes a product decision. Its UI is the FM-1 panel | Dead code in shared files if taken by accident. Take hunks, not files |
| Installer: SMK identities, 36-block packages, NUL split of the identity, wider port names | `tools/fm1_install.py` | Unchanged since 0.3 | **Yes**, the whole file | None |
| Test suite: `ota_test`, `ldr_test`, `ldr_screen_test`, `install_test.py`, `run_tests.sh`, SMK tests | `tests/` | Own `make test` (tests/mello, toolchain-free) | `install_test.py` now (pure Python). The OTA and loader tests later: they need a built `.fwsc` (toolchain, Docker on macOS) | `tests/host.c` conflicts (SkurkyMello +76, upstream +207). The FM-1 tests do not need its SMK and MIDI-script parts |
| CI release on a tag, with build attestation | `.github/workflows/release.yml` | Own `build.yml` (builds only) | Later, adapted (names, `FM-1_987`). It runs on a tag push, so ask first | None in files; it acts on push |
| Docs: API, MAKE-IT-YOURS, SMK37, hub URL now `thefirmwarehub.com` | `docs/`, `README.md`, `LICENSING.md` | Own README (the manual) and LICENSING | Only the hub URL, in `docs/MAKE-IT-YOURS.md` (still upstream's 0.3 copy) | README and LICENSING conflict: do not take |
| `gfx.c`, `build.sh` | — | — | Unchanged in 0.4 | — |

## Notes

**1. The flash chip, two spellings.** Upstream writes `FL_JEDEC_OK(id)` in the new `fm1_flash_map.h`. The main
session's fix writes `fl_jedec_ok(id)` in `fm1_flash.h`. Both accept the same two ids. If the flash map is taken
later (step 3 below), that commit replaces `fl_jedec_ok` with `FL_JEDEC_OK` at the three call sites (`platform.c`,
`recovery.c`, `loader.c`) and drops the inline function. Doing the fix upstream's way now avoids that churn. Either
way works.

**2. `-O2` against `-Os`.** Upstream's `tools/build.py` says: keep `-Os`; with `-O2` (unity build, `usb.c` inside),
the FM-1's USB-MIDI IN babbled on the device (OberVave, 2026-10-13). Its `usb.c` adds that 32-byte buffer alignment
did not cure it, and `-Os` did. SkurkyMello is the same kind of unity build with `usb.c` inside, at `-O2`. Nobody has
seen this on SkurkyMello, because there is no FM-1 to test on and fm1-emu does not reproduce such timing. What to do:

- Build with `-Os` and run `tools/emu_cpu.sh` (playing and `MELLO_STRESS`). fm1-emu counts instructions, not
  cycles, so compare against the same `-O2` run, not an absolute budget.
- If the stress case keeps its margin, switch to `-Os`. That commit changes firmware behaviour: say so, and give the
  numbers.
- If it does not, keep `-O2`, record the risk in DECISIONS.md, and make "USB MIDI IN: notes and clock from a
  computer" an early item on `HARDWARE_VALIDATION_CHECKLIST.md`.

Per-file flags do not help: everything compiles as one unit (`dinghy.c`).

**3. The loader's screen and the Big screen.** 0.4's FM-1 loader includes `fm1_lcd_hw.h`, `../src/lcd.c` and
`ldr_screen.c`. SkurkyMello's `lcd.c` calls `mirror_fill` and `mirror_blit_swapped` (from `mirror.c`), which the
loader does not have. Before including `lcd.c`, `loader.c` needs no-op stand-ins, for example
`#define mirror_fill(x, y, w, h, c) ((void)0)` and the same for `mirror_blit_swapped`. A `#ifndef` guard in `lcd.c`
would also work. Set `SCREEN_NAME` and `SCREEN_COLOUR` in `build.py` to SkurkyMello's own name and accent. Check the
loader's size in the build output, because the loader runs from RAM. `tests/ldr_screen_test.c` from 0.4 checks the
frames on the host.

**4. Why not blobs and slots.** SkurkyMello's `mello_storage` (two 64 KiB banks with commit records) and its sample
store already do more than Dinghy's settings, blobs and slots. Dinghy's FM-1 slots sit at 0x97000..0xB6FFF, on top of
Mello bank 0 (0x98000) and part of bank 1. `store.h` also defines its own `FL_RANGE_OK`, so it would replace
SkurkyMello's allow-list (0x98000..0xE0000 plus the settings sector) with one that lets the app write blobs at
0xF4000 and stops it writing above 0xB7000. If a later sync needs `store.h`, keep SkurkyMello's `FL_RANGE_OK`, give it
its own `STORE_MAGIC`, and call none of `slot_*`.

## Recommended commits

Run `make test`, then `make format`, after each. Name the Dinghy files in each commit message. None of these needs a
push.

1. **UC25HQ80 flash**: done, 0.5-f97 (`b0672a0`). Behaviour: newer FM-1s save to flash and take updates.
2. **Boot scan 100 us**: `fm1_input.h` gets `#ifndef FM1_SETTLE_US`, and `platform.c` defines it as `100u` before the
   include, as 0.4 does. Behaviour: only the 50 ms OCT- check at power-on (`recovery_key`). The running scan
   (`fm1_input_tick`) does not use it. This cannot be shown in fm1-emu. Say so in the commit.
3. **Flash map sync**: take `fm1_flash_map.h`, `fm1_cc.h` and `fm1_flash.h` from upstream, and move the UC25HQ80 check
   to `FL_JEDEC_OK` (note 1). No FM-1 behaviour change. `make test` and a firmware build should show the same image
   apart from the moved code.
4. **No-op syncs**: `usb.c` (aligned 32), `ota.c` (`OTA_PARM_ID`), `bootguard.h` and `fm1_input.h` (`FM1_OCT_BOTH`,
   `FM1_NBTN`). Optionally move `fm1_led_glow_key` out of `fm1_input.h` into `platform.c` under upstream's name.
5. **Installer**: take `tools/fm1_install.py`, and add `tests/install_test.py` to `make test`. It needs no device or
   mido. Its web and SMK-stock parts skip themselves.
6. **`-Os` or not** (note 2): measure first. One commit with the numbers, either the switch or the DECISIONS.md entry.
7. **Loader's screen**: `ldr_screen.c`, the `loader.c` and `fm1_lcd_hw.h` hunks for the FM-1 only (not the
   `BOARD_SMK37` branches), the mirror stand-ins (note 3), SkurkyMello's name and colour in `build.py`, and
   `tests/ldr_screen_test.c`.
8. Later, each on its own: `LCD_IDLE` if redraw lag shows up; the OTA and loader host tests as a toolchain-needing
   `make` target; a tag-driven release workflow adapted from `release.yml` (ask before any tag or push).
