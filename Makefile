# SkurkyMello: the everyday commands. The firmware itself is built by ./build.sh (see README.md).
.PHONY: test sanitize host format format-check clean
test:            ## Mello host tests (tests/mello), seconds, no FM-1 or toolchain needed
	sh tests/run_mello.sh
sanitize:        ## the same tests with UBSan and ASan: any signed overflow or bad access fails
	MELLO_TEST_CFLAGS="-g -fsanitize=undefined,address -fno-sanitize-recover=undefined" sh tests/run_mello.sh
host:            ## the whole firmware on this computer: build/host/dinghy.wav and screen PNGs (needs Pillow)
	sh tests/run_host.sh
format:          ## clang-format the Mello sources (the Dinghy platform keeps upstream's style)
	clang-format -i firmware/src/dinghy.c firmware/src/mello_*.c firmware/src/mello_*.h tests/mello/*.c
format-check:
	clang-format --dry-run --Werror firmware/src/dinghy.c firmware/src/mello_*.c firmware/src/mello_*.h tests/mello/*.c
clean:
	rm -rf build
