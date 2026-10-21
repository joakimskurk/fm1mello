#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
BUNDLE=source/mello-source.tar.gz
EXPECTED=4bc99f8814531331696ad1749101be66765205fa5637aa5bb970239d389f6def
[[ -f "$BUNDLE" ]] || { echo "missing $BUNDLE" >&2; exit 1; }
echo "$EXPECTED  $BUNDLE" | sha256sum -c -
if [[ ! -d mello || ! -f overlay/dinghy.c ]]; then
  tar -xzf "$BUNDLE"
fi
