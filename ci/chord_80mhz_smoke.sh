#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
PKG=${1:-build/skurkymello-0.1-dev.fwsc}
[[ -f "$PKG" ]] || { echo "missing package: $PKG" >&2; exit 2; }
WEBROOT="$ROOT/work/fm1-emu-web"
[[ -f "$WEBROOT/target/wasm/wasm32-unknown-unknown/release/fm1_web.wasm" ]] || { echo "missing web emulator build; run web_100mhz_smoke first" >&2; exit 2; }
rm -rf work/chord80
mkdir -p work/chord80
PKG_ABS=$(realpath "$PKG")
(
  cd "$WEBROOT"
  FM1_FWSC="$PKG_ABS" \
  FM1_BENCH_KEYS='350:ARP,650:ARP,950:REC,1250:PLAY' \
  FM1_WARMUP_MS=1800 \
  FM1_BENCH_JSON="$ROOT/work/chord80/report.json" \
  node web/bench.mjs 80 5 "$ROOT/work/chord80/audio.raw" \
  | tee "$ROOT/work/chord80/run.log"
)
python3 - <<'PY'
from pathlib import Path
import json, struct
root=Path('work/chord80')
r=json.loads((root/'report.json').read_text())
raw=(root/'audio.raw').read_bytes()
vals=struct.unpack('<'+'h'*(len(raw)//2),raw) if raw else ()
peak=max((abs(v) for v in vals),default=0)
nonzero=sum(v != 0 for v in vals)
r['rawPeak']=peak
r['rawNonzeroSamples']=nonzero
(root/'report.checked.json').write_text(json.dumps(r,indent=2)+'\n')
print(json.dumps(r))
if r['emulatedMs'] < 4900: raise SystemExit('FAIL: chord/reverb 80 MHz run did not reach requested time')
if r['audioFrames'] == 0 or peak == 0: raise SystemExit('FAIL: chord/reverb stress path produced no audio')
steady=r.get('steadyRealtime') or 0
if steady < 1.35: raise SystemExit(f'FAIL: chord + enabled Reverb V2 leaves too little 80 MHz margin: {steady:.3f}x < 1.35x')
print(f'PASS: chord + enabled Reverb V2 worst-case path at 80 MHz has {steady:.3f}x steady realtime margin')
PY
