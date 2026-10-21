#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
OUT=${TMPDIR:-/tmp}/mello-target-source.tar.xz
rm -f "$OUT"
cat source/target/chunk-*.bin > "$OUT"
[[ $(wc -c < "$OUT") -eq 52620 ]] || { echo "FAIL: reconstructed source size" >&2; exit 1; }
echo '672b05f35c66cd863fcd659b59699fa91ccc571f877dbb55fddbe25e032e58c7  '"$OUT" | sha256sum -c -
xz -t "$OUT"
rm -rf mello overlay
tar -xJf "$OUT"
[[ -f mello/mello_engine.c && -f overlay/dinghy.c ]] || { echo 'FAIL: source archive contents' >&2; exit 1; }
echo 'PASS: reconstructed byte-identical target source'
