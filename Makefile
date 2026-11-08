# SkurkyMello: the everyday commands. The firmware itself is built by ./build.sh (see README.md).
.PHONY: test sanitize bench host screens art site parity reference emu format format-check clean
test:            ## Mello host tests (tests/mello), Dinghy's installer and loader screen, the Big screen check; no FM-1 or toolchain
	sh tests/run_mello.sh
	sh tests/mirror_check.sh
sanitize:        ## the same tests with UBSan and ASan: any signed overflow or bad access fails
	MELLO_TEST_CFLAGS="-g -fsanitize=undefined,address -fno-sanitize-recover=undefined" sh tests/run_mello.sh
bench:           ## where the audio's time goes on this computer: the default pattern, the worst case, one part out at a time
	mkdir -p build/bench
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Ifirmware/src tests/bench/cpu_bench.c firmware/src/mello_*.c -lm -o build/bench/cpu_bench
	build/bench/cpu_bench $(BENCH_SECONDS)
parity:          ## render tools/tone-ref/stimuli.json through the C engine and compare with the HTML (needs numpy)
	mkdir -p build/golden-c
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -DMELLO_OUTPUT_GAIN_Q12=4096 -Ifirmware/src tests/parity/render_stimuli.c firmware/src/mello_*.c -lm -o build/render_stimuli
	build/render_stimuli build/golden-c >/dev/null
	python3 tools/tone-ref/compare.py --json tests/golden/parity.json
reference:       ## re-render the HTML golden files (tests/golden/html) with Tone.Offline (needs node and a Chromium)
	python3 tools/tone-ref/stimuli_to_c.py
	cd tools/tone-ref && npm ci --no-fund --no-audit && npm run render
host:            ## the whole firmware on this computer: build/host/dinghy.wav and screen PNGs (needs Pillow)
	sh tests/run_host.sh
screens:         ## every page as PNG in docs/screens, and the UI map's pictures (python3, nothing else)
	python3 tools/screens.py
art:             ## the README's logo (docs/logo.png, from docs/card.png), its GIF (docs/hero.gif, the host build playing), the site's link preview (web/og.png)
	python3 tools/readme_art.py
	python3 web/make_og.py
PKG ?= build/skurkymello-dev.fwsc
VER ?= dev
site:            ## the website in build/site, as CI publishes it: the page, the installer with PKG, the demos (needs numpy, ffmpeg or afconvert)
	python3 web/make_site.py $(PKG) $(VER) build/site
emu:             ## a built package on fm1-emu: played and edited live, its load and its stacks (PKG=build/....fwsc)
	sh tools/emu_smoke.sh $(PKG)
	sh tools/emu_stack.sh $(PKG)
format:          ## clang-format the Mello sources (the Dinghy platform keeps upstream's style)
	clang-format -i firmware/src/dinghy.c firmware/src/mello_*.c firmware/src/mello_*.h tests/mello/*.c tests/bench/*.c
format-check:
	clang-format --dry-run --Werror firmware/src/dinghy.c firmware/src/mello_*.c firmware/src/mello_*.h tests/mello/*.c tests/bench/*.c
clean:
	rm -rf build
