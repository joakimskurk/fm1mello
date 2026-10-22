#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
PKG=${1:-build/skurkymello-0.1-dev.fwsc}
[[ -f "$PKG" ]] || { echo "missing package: $PKG" >&2; exit 2; }
WEBROOT="$ROOT/work/fm1-emu-web/web/dist"
[[ -f "$WEBROOT/index.html" ]] || { echo "missing built web emulator: $WEBROOT" >&2; exit 2; }
python3 -m pip install -q playwright
python3 -m playwright install chromium >/dev/null
rm -rf work/frontend-e2e
mkdir -p work/frontend-e2e
( cd "$WEBROOT" && python3 -m http.server 8765 --bind 127.0.0.1 > "$ROOT/work/frontend-e2e/server.log" 2>&1 ) &
SERVER=$!
trap 'kill $SERVER 2>/dev/null || true' EXIT
for _ in $(seq 1 50); do
  curl -fsS http://127.0.0.1:8765/ >/dev/null && break
  sleep .1
done
python3 ci/frontend_e2e.py "$PKG" | tee work/frontend-e2e/test.log
