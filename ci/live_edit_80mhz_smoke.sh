#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
PKG=${1:-build/skurkymello-0.1-dev.fwsc}
[[ -f "$PKG" ]] || { echo "missing package: $PKG" >&2; exit 2; }
WEBROOT="$ROOT/work/fm1-emu-web"
[[ -f "$WEBROOT/target/wasm/wasm32-unknown-unknown/release/fm1_web.wasm" ]] || { echo "missing web emulator build; run web_100mhz_smoke first" >&2; exit 2; }
rm -rf work/live-edit-80
mkdir -p work/live-edit-80
PKG_ABS=$(realpath "$PKG")
# Exercise the real FM-1 matrix encoder route while PLAY stays active. e0/e1 are
# SELECT/ALGORITHM; e2..e5 are K1..K4. Button releases are explicit so the same
# physical control can be pressed again later in the script.
SCRIPT='100:PLAY,140:PLAY^,300:ENV,340:ENV^,450:e4:+8,550:e5:+8,650:e4:-6,750:e5:-6,900:e1:+1,1000:e2:+3,1100:e3:+3,1200:e4:-4,1300:e5:+3,1450:e1:+1,1550:e3:+8,1650:e4:+8,1750:e1:+1,1850:e3:+5,1950:e1:+1,2050:e2:+4,2150:e3:-5,2300:ARP,2340:ARP^,2450:e1:+1,2550:e5:+3,2650:e1:+1,2750:e5:-2,2850:e1:+1,2950:e2:+6,3050:e4:+5,3150:e5:+4,3250:e1:+1,3350:e3:+5,3450:e4:+5,3550:e4:-3,3700:FX,3740:FX^,3850:FX,3890:FX^,4000:e2:+6,4100:e3:+5,4200:e4:+4,4300:e5:-4,4400:e1:+1,4500:e2:+5,4600:e3:+5,4700:e4:-3,4800:e5:+3,4900:e1:+1,5000:e4:+4,5100:e5:+3,5200:e1:+1,5300:e2:+5,5400:e3:+5,5500:e4:+4,5600:e5:-3,5700:e1:+1,5800:e2:+5,5900:e3:+4,6050:REC,6090:REC^,6200:e2:+4,6300:e3:+5,6400:e1:+1,6500:e2:+2,6600:e3:-3,6750:e1:+1,6850:e2:+3,6950:e3:-3,7100:e1:+1,7200:e2:+3,7300:e3:+3,7400:e4:-3,7500:e5:+3,7700:HOME,7740:HOME^,8000:e2:+3,8200:e3:-3'
(
  cd "$WEBROOT"
  FM1_FWSC="$PKG_ABS" \
  FM1_BENCH_KEYS="$SCRIPT" \
  FM1_WARMUP_MS=1000 \
  FM1_BENCH_JSON="$ROOT/work/live-edit-80/report.json" \
  node web/bench.mjs 80 9 "$ROOT/work/live-edit-80/audio.raw" \
  | tee "$ROOT/work/live-edit-80/run.log"
)
python3 - <<'PY'
from pathlib import Path
import json, struct
root=Path('work/live-edit-80')
r=json.loads((root/'report.json').read_text())
raw=(root/'audio.raw').read_bytes()
vals=struct.unpack('<'+'h'*(len(raw)//2),raw) if raw else ()
peak=max((abs(v) for v in vals),default=0)
nonzero=sum(v != 0 for v in vals)
tail_samples=min(len(vals), 44100*2)
tail=vals[-tail_samples:] if tail_samples else ()
tail_nonzero=sum(v != 0 for v in tail)
tail_peak=max((abs(v) for v in tail),default=0)
r['rawPeak']=peak
r['rawNonzeroSamples']=nonzero
r['tailNonzeroSamples']=tail_nonzero
r['tailPeak']=tail_peak
(root/'report.checked.json').write_text(json.dumps(r,indent=2)+'\n')
print(json.dumps(r))
if r['emulatedMs'] < 8900: raise SystemExit('FAIL: live-edit 80 MHz run did not reach requested time')
if r['audioFrames'] == 0 or peak == 0: raise SystemExit('FAIL: live-edit 80 MHz run produced no audio')
if tail_nonzero < 1000 or tail_peak == 0: raise SystemExit(f'FAIL: audio did not remain live after encoder stress: tail_nonzero={tail_nonzero} tail_peak={tail_peak}')
steady=r.get('steadyRealtime') or 0
if steady < 1.35: raise SystemExit(f'FAIL: live-edit encoder path leaves too little 80 MHz margin: {steady:.3f}x < 1.35x')
print(f'PASS: physical encoder live-edit stress stayed audible at 80 MHz with {steady:.3f}x steady realtime margin; tail_nonzero={tail_nonzero}')
PY
