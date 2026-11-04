#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
# The package on fm1-emu, played and edited live (ROADMAP Phase 0 / F23: what the old ci/*_80mhz_smoke.sh scripts
# checked on the web emulator). 10.5 s: the splash, PLAY, then every page, the knobs and SELECT turned while it
# plays, a roll, REC.
# It fails when the run does not reach its end, when the sound is silent or dies after the edits (the last 2 s), when
# a stack guard trips, or when the audio interrupt takes more of the chip than LIMIT percent (instructions: fm1-emu
# counts instructions, not cycles; see tools/emu_cpu.sh).
#   tools/emu_smoke.sh build/skurkymello-0.5-f60.fwsc [LIMIT]     (LIMIT 60; a MELLO_STRESS build: 85)
#   FM1_EMU   the fm1-emu directory, as tools/emu_cpu.sh (on a Mac it runs in Docker)
set -e
PKG="$1"
LIMIT="${2:-60}"
[ -f "$PKG" ] || { echo "usage: tools/emu_smoke.sh PACKAGE.fwsc [LIMIT]"; exit 2; }
EMU="${FM1_EMU:-$HOME/fm1-emu-0.3.0/fm1-emu-0.3.0-linux-x86_64}"
# enc0 SELECT, enc2..enc5 K1..K4; kN the HAL's key id (14 + note n): k36 RNDMZR (note 22, B9). The times are from the
# end of the splash (1.5 s; a key before it is lost), shifted on below.
KEYS='300:PLAY,700:ENV,900:enc2:+8,1000:enc3:+6,1100:enc4:-5,1200:enc5:+4,1400:enc0:+1,1500:enc2:-6,1700:ARP,'
KEYS="${KEYS}1900:enc2:+5,2000:enc3:+5,2200:enc0:+1,2300:enc4:+4,2500:ARP,2700:enc2:+3,2900:ARP,3100:enc3:-4,"
KEYS="${KEYS}3300:FX,3500:enc2:+6,3600:enc3:+5,3800:FX,4000:enc4:+4,4200:LFO,4400:enc2:+3,4600:LFO,4800:enc5:+3,"
KEYS="${KEYS}5000:SEQ,5200:enc2:+2,5400:enc0:+1,5600:GLO,5800:enc2:-3,6000:HOME,6200-6320:k36,6600:REC,6800:REC,"
KEYS="${KEYS}7000:enc2:+3,7200:enc3:-3,7400:HOME"
KEYS=$(echo "$KEYS" | tr ',' '\n' | awk -F: -v d=1500 '{ split($1, t, "-"); $1 = t[1] + d (t[2] != "" ? "-" t[2] + d : "") } 1' OFS=: | paste -sd, -)
WORK="$(mktemp -d "$HOME/.fm1-emu-smoke.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
cp "$PKG" "$WORK/pkg.fwsc"
if [ "$(uname -s)" = Linux ]; then
    (cd "$WORK" && "$EMU/fm1-emu" --fwsc pkg.fwsc --ms 10500 --keys "$KEYS" --wav out.wav >run.txt 2>&1) || true
else
    docker run --rm -v "$EMU":/e -v "$WORK":/w -w /w ubuntu:24.04 \
        /e/fm1-emu --fwsc /w/pkg.fwsc --ms 10500 --keys "$KEYS" --wav /w/out.wav >"$WORK/run.txt" 2>&1 || true
fi
python3 -I - "$WORK/run.txt" "$WORK/out.wav" "$LIMIT" <<'EOF'
import re, struct, sys
log = open(sys.argv[1]).read()
limit = float(sys.argv[3])
fails = []
if not re.search(r"^time 10500 ms", log, re.M):
    fails.append("the run did not reach 10500 ms")
if re.search(r"stack limit|guard", log, re.I):
    fails.append("a stack guard tripped")
m = re.search(r"([0-9.]+)% in an ISR", log)
share = float(m.group(1)) if m else 100.0
if share > limit:
    fails.append("the audio interrupt takes %.1f%% of the chip, over %.0f%%" % (share, limit))
try:
    d = open(sys.argv[2], "rb").read()
    i = d.index(b"data") + 8
    s = struct.unpack("<%dh" % ((len(d) - i) // 2), d[i:i + (len(d) - i) // 2 * 2])
except (OSError, ValueError):
    s = ()
tail = s[-2 * 44100 * 2:]
peak, tail_peak = max(map(abs, s), default=0), max(map(abs, tail), default=0)
if not peak:
    fails.append("no sound")
elif tail_peak * 10 < peak:  # under a tenth of the run's peak: silent, or a dither of noise
    fails.append("the sound died after the edits (last 2 s)")
print("emu_smoke: %.1f%% of the chip in the audio interrupt, peak %d, last 2 s peak %d" % (share, peak, tail_peak))
for f in fails:
    print("FAIL: " + f)
sys.exit(1 if fails else 0)
EOF
