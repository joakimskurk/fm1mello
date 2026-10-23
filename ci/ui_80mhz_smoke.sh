#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
PKG=${1:-build/skurkymello-0.1-dev.fwsc}
[[ -f "$PKG" ]] || { echo "missing package: $PKG" >&2; exit 2; }
WEBROOT="$ROOT/work/fm1-emu-web"
[[ -f "$WEBROOT/target/wasm/wasm32-unknown-unknown/release/fm1_web.wasm" ]] || { echo "missing web emulator build; run web_100mhz_smoke first" >&2; exit 2; }
rm -rf work/ui80
mkdir -p work/ui80
PKG_ABS=$(realpath "$PKG")
(
  cd "$WEBROOT"
  FM1_FWSC="$PKG_ABS" \
  FM1_BENCH_KEYS='200:PLAY,700:SEQ,1200:enc0:+1,1800:HOME,2400:SEQ,3000:HOME,3600:SEQ,4300:HOME' \
  FM1_WARMUP_MS=1000 \
  FM1_BENCH_JSON="$ROOT/work/ui80/report.json" \
  node web/bench.mjs 80 6 "$ROOT/work/ui80/audio.raw" \
  | tee "$ROOT/work/ui80/run.log"
)
python3 - <<'PY'
from pathlib import Path
import json, struct
root=Path('work/ui80')
r=json.loads((root/'report.json').read_text())
raw=(root/'audio.raw').read_bytes()
vals=struct.unpack('<'+'h'*(len(raw)//2),raw) if raw else ()
peak=max((abs(v) for v in vals),default=0)
nonzero=sum(v != 0 for v in vals)
r['rawPeak']=peak
r['rawNonzeroSamples']=nonzero
(root/'report.checked.json').write_text(json.dumps(r,indent=2)+'\n')
print(json.dumps(r))
if r['emulatedMs'] < 5900: raise SystemExit('FAIL: UI 80 MHz run did not reach requested time')
if r['audioFrames'] == 0 or peak == 0: raise SystemExit('FAIL: UI 80 MHz playback produced no audio')
steady=r.get('steadyRealtime') or 0
if steady < 1.35: raise SystemExit(f'FAIL: SEQ/HOME UI path leaves too little 80 MHz margin: {steady:.3f}x < 1.35x')
print(f'PASS: SEQ/HOME playback/redraw path at 80 MHz has {steady:.3f}x steady realtime margin')
PY
