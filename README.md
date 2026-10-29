# SkurkyMello for the M-VAVE FM-1

A port of [SkurkyMello](reference/skurkymello.html), a generative step sequencer and synth in the browser, to the
M-VAVE FM-1 as its own firmware. It is built on [Dinghy](https://github.com/zvenson/dinghy) 0.3 by zvenson and,
under that, the Felucca / SLOOP platform.

> **Status: prototype, not tested on an FM-1.** The code is Reverb V3 Candidate A.1 (fm1mello `3cd776c`) as plain
> source, with the P0/P1 bugs from the October 2026 review fixed (a filter that could latch at a loud DC offset,
> edits that were never saved): see [docs/review-2026-10/REVIEW.md](docs/review-2026-10/REVIEW.md) and
> ROADMAP.md. Not yet run on hardware: [docs/mello/FIRST_FLASH.md](docs/mello/FIRST_FLASH.md) is the first test.

## Work on it

```sh
make test          # Mello host tests: seconds, no FM-1, no toolchain
make sanitize      # the same with UBSan + ASan
make host          # the whole firmware on this computer: build/host/dinghy.wav and screen PNGs (pip3 install Pillow)
make format        # clang-format the Mello sources (pip3 install clang-format)
```

One change, `make test`, commit. The target build checks what the host cannot: RAM, pools and the package.

## Build the firmware

You need Python 3 with Pillow, the JieLi toolchain and the AC79 SDK (as in Dinghy's README):

```sh
tools/get_toolchain.sh                      # JieLi's clang for the chip, to ~/.jieli/toolchain
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
    https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
./build.sh                                  # -> build/skurkymello-dev.fwsc
./build.sh --release 0.2                    # -> build/skurkymello-0.2.fwsc
```

The toolchain is Linux x86-64 only. On a Mac, `build.sh` runs it in a `linux/amd64` Docker container by itself, so
Docker (Docker Desktop or Colima) must be running. GitHub Actions builds every push the same way
(`.github/workflows/build.yml`, through the shared `zvenson/fm1-build` workflow) and attests the package.

Installing: `python3 tools/fm1_install.py build/skurkymello-dev.fwsc` on an FM-1 running official V15. Every flash can
brick a unit; hold OCT- while switching on to reach the updater. The first time: [docs/mello/FIRST_FLASH.md](docs/mello/FIRST_FLASH.md).

## Where things are

```
firmware/src/dinghy.c        the app: wires Mello to the FM-1 (audio callback, keys, screen, storage)
firmware/src/mello_engine.*  project model, defaults, the generative sequencer
firmware/src/mello_transport.* clock, steps, ratchets, gate
firmware/src/mello_audio.*   the synth, sub, chord, FX and reverb (fixed point)
firmware/src/mello_ui.*      pages and controls
firmware/src/mello_storage.* projects in flash (A/B banks with CRC)
firmware/src/mello_sample.*  user samples
firmware/src/platform.c, hal/, the rest   Dinghy's platform, as close to upstream as possible
tests/mello/                 host tests for the Mello code
reference/skurkymello.html   the original: the reference for sound and feel
docs/mello/                  design: control grammar, decisions, UI language, hardware checklist
docs/review-2026-10/         the October 2026 review and its four detailed reports
docs/mello/history/          the work-package notes of the first port (WP-B .. WP-E)
docs/API.md, RULES.md, MAKE-IT-YOURS.md   Dinghy's platform docs
```

## Upstream

`main` is a fork of Dinghy at `d44df7d` (0.3); `git remote upstream` points there. The Mello changes to Dinghy's
own files are small and listed in `git log -- firmware/src/platform.c firmware/src/gfx.c tools/build.py`. Dinghy has
since moved to 0.4 (TRS MIDI, clock, more storage); moving to it is a separate step (ROADMAP.md).

Package identity: `FM-1_970`. Dinghy's `docs/MAKE-IT-YOURS.md` lists `97N` for zp12. Check the Firmware Hub before the
first public release and pick a free one if they collide.

## Licence

GPL-3.0-only, as Dinghy. See [LICENSING.md](LICENSING.md) for who made what.
