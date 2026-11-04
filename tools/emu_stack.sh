#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
# The stacks' high-water marks for a package, on fm1-emu (ROADMAP B4 / F21): 11 s of a player's session (PLAY, three
# RNDMZR rolls, every page, SAVE, a held step), then the two stacks dumped. The emulator's RAM starts at zero, so the
# lowest word that is not zero is as deep as the stack went (a frame that only ever held zeros would hide, so this
# can read a little short). The guard bands under each stack must stay zero. fm1-emu's kN is the HAL's key id: 0-13
# the panel buttons, 14 + n note key n (0 = F3 .. 26 = G5): RNDMZR (note 25) is k39, white key 1 k14.
#   tools/emu_stack.sh build/skurkymello-0.5-f59.fwsc
#   FM1_EMU   the fm1-emu directory, as tools/emu_cpu.sh
set -e
PKG="$1"
[ -f "$PKG" ] || { echo "usage: tools/emu_stack.sh PACKAGE.fwsc"; exit 2; }
EMU="${FM1_EMU:-$HOME/fm1-emu-0.3.0/fm1-emu-0.3.0-linux-x86_64}"
KEYS='1600:PLAY,2500-2620:k39,3200-3320:k39,3800:ARP,4200:ARP,4600:ARP,5000:LFO,5400:LFO,5800:FX,6200:ENV,'
KEYS="${KEYS}6600:SEQ,7000:GLO,7400:HOME,7800-7920:k39,8400:SAVE,8800:SAVE,9300-9420:k18,9600-9720:k14,10000:HOME"
WORK="$(mktemp -d "$HOME/.fm1-emu-stack.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
cp "$PKG" "$WORK/pkg.fwsc"
# app.ld: _guard0 0x01C74000, the user stack 0x01C74100..0x01C7A000, _guard1, the system stack ..0x01C7C000
if [ "$(uname -s)" = Linux ]; then
    "$EMU/fm1-emu" --fwsc "$WORK/pkg.fwsc" --ms 11000 --keys "$KEYS" --dump 1C74000 8192 >"$WORK/run.txt" 2>&1 || true
else
    docker run --rm -v "$EMU":/e -v "$WORK":/w -w /w ubuntu:24.04 \
        /e/fm1-emu --fwsc /w/pkg.fwsc --ms 11000 --keys "$KEYS" --dump 1C74000 8192 >"$WORK/run.txt" 2>&1 || true
fi
grep -q "^time 11000 ms" "$WORK/run.txt" || { tail -20 "$WORK/run.txt"; echo "emu_stack: the run did not finish"; exit 1; }
python3 -I - "$WORK/run.txt" <<'EOF'
import re, sys
words = {}
for line in open(sys.argv[1]):
    m = re.match(r"\s+0x([0-9a-f]+):((?: [0-9a-f]{8})+)", line)
    if m:
        a = int(m.group(1), 16)
        for i, w in enumerate(m.group(2).split()):
            words[a + 4 * i] = int(w, 16)
bad = 0
for name, lo, hi in (("user", 0x01C74100, 0x01C7A000), ("system", 0x01C7A100, 0x01C7C000)):
    deep = next((a for a in range(lo, hi, 4) if words.get(a, 0)), hi)
    guard = any(words.get(a, 0) for a in range(lo - 256, lo, 4))
    print("%-6s stack: %5u of %5u B used%s" % (name, hi - deep, hi - lo, ", THE GUARD BAND WAS WRITTEN" if guard else ""))
    bad |= guard
sys.exit(1 if bad else 0)
EOF
