#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
# Big screen check (DECISIONS.md): the host firmware is asked for its screen the way FM-1 Mirror asks, and the SysEx
# it sends is decoded back into a screen and compared with the screen itself, pixel for pixel, twice: after the
# hello (the whole screen), and after a page change and PLAY (the changed rows alone). Needs cc and python3.
#   tests/mirror_check.sh
set -e
cd "$(dirname "$0")/.."
OUT=build/mirror
mkdir -p "$OUT"
command -v python3 >/dev/null 2>&1 || { echo "mirror_check: no python3, skipped"; exit 0; }
cc -O2 -Wall -Wno-unused-function -DDINGHY_HOST -DAPP_VER='"test"' -Ifirmware/src -Ifirmware/gen \
    -o "$OUT/host" firmware/src/dinghy.c
# the hello at 2000 (after the splash) and a shot once the paced repaint has gone out (2700); then SEQ, PLAY, and a
# shot of the changed rows (3600). The SysEx is written as it goes, so each shot is its own run ending at the shot:
# the screen and the SysEx are then of the same moment.
FAILED=0
for t in 2700 3600; do
    { echo "2000 hello 1"; if [ "$t" -gt 2800 ]; then echo "2800 btn 11"; echo "3000 btn 12"; fi; echo "$t shot"; echo "$t end"; } >"$OUT/script-$t.txt"
    rm -f "$OUT/cut.syx" "$OUT"/cut-*.ppm
    "$OUT/host" "$OUT/cut" "$OUT/script-$t.txt" >/dev/null
    if python3 -I tests/mirror_decode.py "$OUT/cut.syx" "$OUT/cut-$t.ppm" --png "$OUT/mirror-$t.png"; then
        echo "PASS mirror at $t ms"
    else
        echo "FAIL mirror at $t ms (see $OUT/mirror-$t.png against $OUT/cut-$t.ppm)"
        FAILED=1
    fi
done
# with nobody asking, nothing is sent
cat >"$OUT/script-quiet.txt" <<EOF
2300 shot
2400 end
EOF
rm -f "$OUT/quiet.syx"
"$OUT/host" "$OUT/quiet" "$OUT/script-quiet.txt" >/dev/null
if [ -s "$OUT/quiet.syx" ]; then echo "FAIL mirror: sent without a hello"; FAILED=1; else echo "PASS mirror quiet without a hello"; fi
[ "$FAILED" = 0 ] && echo "mirror check passed"
exit "$FAILED"
