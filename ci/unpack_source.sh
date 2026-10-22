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
for p in patches/visual-port-*.patch; do
  [[ -f "$p" ]] || continue
  patch --batch --forward -p1 < "$p"
done
python3 ci/patch_ui_semantics.py
echo '1fc28273189a8593e4d77652187d5446823d51f3d1dc2cd0f2ea021d041e926e  mello/mello_ui.c' | sha256sum -c -
python3 ci/patch_audio_html_parity.py
patch --batch --forward -p1 < patches/post-semantic-step-segment.patch
echo '284ad8a2154260352e0d82de67bd3ea7c761e0940cd744af00113c6bd4eb0cbc  mello/mello_ui.c' | sha256sum -c -
echo '2b6cf93f1ae16517aab7a51c1cca716de99d2ee0e33111881bd9f3aec374a498  mello/mello_audio.c' | sha256sum -c -
echo '04c12ca9c47ff535f15a8be63761228754ff4af21b83e8c231717c6cf919670f  mello/mello_audio.h' | sha256sum -c -
echo 'PASS: reconstructed source + semantic UI + STEP/SEGMENT + HTML audio parity transforms'
