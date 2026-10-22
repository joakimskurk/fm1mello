#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
PKG=${1:-build/skurkymello-0.1-dev.fwsc}
[[ -f "$PKG" ]] || { echo "missing package: $PKG" >&2; exit 2; }
EMU_COMMIT=db8d472e62f35d18e392e3abc66474daed65ffe9
rm -rf work/fm1-emu-web work/web100
mkdir -p work/web100
git clone -q https://github.com/joakimeriksson/fm1-emu.git work/fm1-emu-web
( cd work/fm1-emu-web && git checkout -q --detach "$EMU_COMMIT" )
# Reproduce the browser/WebAssembly execution path, not the native batch emulator.
rustup target add wasm32-unknown-unknown >/dev/null
( cd work/fm1-emu-web && bash web/build.sh >/dev/null )
PKG_ABS=$(realpath "$PKG")
(
  cd work/fm1-emu-web
  FM1_FWSC="$PKG_ABS" \
  FM1_BENCH_KEYS='900:SEQ,1500:enc0:+2,2200:HOME,2800:PLAY' \
  FM1_WARMUP_MS=500 \
  FM1_BENCH_JSON="$ROOT/work/web100/report.json" \
  node web/bench.mjs 100 4 "$ROOT/work/web100/audio.raw" \
  | tee "$ROOT/work/web100/run.log"
)
python3 - <<'PY'
from pathlib import Path
import json, struct
root=Path('work/web100')
r=json.loads((root/'report.json').read_text())
raw=(root/'audio.raw').read_bytes()
vals=struct.unpack('<'+'h'*(len(raw)//2),raw) if raw else ()
peak=max((abs(v) for v in vals),default=0)
nonzero=sum(v != 0 for v in vals)
r['rawPeak']=peak
r['rawNonzeroSamples']=nonzero
(root/'report.checked.json').write_text(json.dumps(r,indent=2)+'\n')
print(json.dumps(r))
if r['emulatedMs'] < 3900:
    raise SystemExit('FAIL: browser/WASM run did not reach requested emulated time')
if r['audioFrames'] == 0:
    raise SystemExit('FAIL: browser/WASM produced no real audio frames')
if peak == 0:
    raise SystemExit('FAIL: browser/WASM PLAY path produced all-zero audio')
print('PASS: browser/WASM at 100 MHz accepted scripted input and produced audio')
PY
