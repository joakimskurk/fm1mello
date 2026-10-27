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
# SELECT/ALGORITHM; e2..e5 are K1..K4. Bare button events use the same proven
# pulse semantics as the existing 80 MHz smoke scripts.
SCRIPT='300:PLAY,650:ENV,800:e4:+8,900:e5:+8,1000:e4:-6,1100:e5:-6,1250:e1:+1,1350:e2:+3,1450:e3:+3,1550:e4:-4,1650:e5:+3,1800:e1:+1,1900:e3:+8,2000:e4:+8,2100:e1:+1,2200:e3:+5,2300:e1:+1,2400:e2:+4,2500:e3:-5,2650:ARP,2800:e1:+1,2900:e5:+3,3000:e1:+1,3100:e5:-2,3200:e1:+1,3300:e2:+6,3400:e4:+5,3500:e5:+4,3600:e1:+1,3700:e3:+5,3800:e4:+5,3900:e4:-3,4050:FX,4200:FX,4350:e2:+6,4450:e3:+5,4550:e4:+4,4650:e5:-4,4750:e1:+1,4850:e2:+5,4950:e3:+5,5050:e4:-3,5150:e5:+3,5250:e1:+1,5350:e4:+4,5450:e5:+3,5550:e1:+1,5650:e2:+5,5750:e3:+5,5850:e4:+4,5950:e5:-3,6050:e1:+1,6150:e2:+5,6250:e3:+4,6400:REC,6550:e2:+4,6650:e3:+5,6750:e1:+1,6850:e2:+2,6950:e3:-3,7100:e1:+1,7200:e2:+3,7300:e3:-3,7450:e1:+1,7550:e2:+3,7650:e3:+3,7750:e4:-3,7850:e5:+3,8100:HOME,8350:e2:+3,8500:e3:-3'
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
