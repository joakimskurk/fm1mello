#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
PKG=${1:-build/skurkymello-0.1-dev.fwsc}
[[ -f "$PKG" ]] || { echo "missing package: $PKG" >&2; exit 2; }
EMU_COMMIT=43ed5580064885e93bc8d20be931ae950d10cd4f
rm -rf work/fm1-emu work/emu-smoke work/lcd_*.bmp
mkdir -p work/emu-smoke
git clone -q https://github.com/joakimeriksson/fm1-emu.git work/fm1-emu
( cd work/fm1-emu && git checkout -q --detach "$EMU_COMMIT" )
# Build the native batch emulator once. It emulates the real FM-1 panel, IRQs and audio path.
( cd work/fm1-emu && cargo build -q --release -p fm1-emu )
EMU=work/fm1-emu/target/release/fm1-emu
# Script: allow boot, snapshot HOME, press SEQ, turn SELECT, press HOME, press PLAY.
# A working main loop/input path must change the LCD after SEQ and PLAY must start the sequencer/audio.
"$EMU" --fwsc "$PKG" --ms 4500 --wav work/emu-smoke/mello.wav \
  --keys "900:SEQ,1500:enc0:+2,2200:HOME,2800:PLAY" \
  --snap 800 --snap 1200 --snap 1900 --snap 2600 --snap 3800 \
  > work/emu-smoke/run.log 2>&1
python3 - <<'PY'
from pathlib import Path
import hashlib, struct, wave, json
root=Path('work/emu-smoke')
# fm1-emu always writes --snap images to ./work relative to the process cwd.
for p in Path('work').glob('lcd_*.bmp'):
    (root/p.name).write_bytes(p.read_bytes())
shots=sorted(root.glob('lcd_*.bmp'))
if len(shots)<5:
    log=(root/'run.log').read_text(errors='replace') if (root/'run.log').exists() else ''
    print(log)
    raise SystemExit(f'FAIL: expected 5 LCD snapshots, got {len(shots)}')
h=[hashlib.sha256(p.read_bytes()).hexdigest() for p in shots]
if h[0]==h[1]==h[2]==h[3]==h[4]:
    raise SystemExit('FAIL: LCD remained bit-identical through scripted panel input')
wp=root/'mello.wav'
with wave.open(str(wp),'rb') as w:
    raw=w.readframes(w.getnframes())
    sw=w.getsampwidth(); ch=w.getnchannels(); fr=w.getnframes()
if sw!=2:
    raise SystemExit(f'FAIL: unexpected WAV sample width {sw}')
vals=struct.unpack('<'+'h'*(len(raw)//2),raw)
peak=max((abs(v) for v in vals),default=0)
nonzero=sum(1 for v in vals if v)
report={'screens':[p.name for p in shots],'screen_sha256':h,'wav_frames':fr,'wav_channels':ch,'wav_peak':peak,'wav_nonzero_samples':nonzero}
(root/'report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
if peak==0:
    print((root/'run.log').read_text(errors='replace'))
    raise SystemExit('FAIL: PLAY produced completely silent audio')
print('PASS: emulator panel input changed LCD and PLAY produced audio')
PY
