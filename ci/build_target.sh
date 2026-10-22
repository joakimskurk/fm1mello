#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
bash ci/unpack_source.sh
PRODUCT=${MELLO_PRODUCT:-FM-1_970}
DINGHY_COMMIT=d44df7db09b98ccad2f1233b8f83f5f09c04829e
DINGHY_TREE=8cefd253b92b544109a315fd2fb9ffcea7c429b6
if [[ ${GITHUB_REF_TYPE:-} == tag && ${GITHUB_REF_NAME:-} == v* ]]; then VERSION=${GITHUB_REF_NAME#v}; else VERSION=${MELLO_VERSION:-0.1-dev}; fi
[[ $PRODUCT =~ ^FM-1_9[0-9][0-9]$ && $PRODUCT != FM-1_99? ]] || { echo "invalid MELLO_PRODUCT: $PRODUCT" >&2; exit 2; }
rm -rf work build; mkdir -p work build/evidence
git init -q work/dinghy
( cd work/dinghy; git remote add origin https://github.com/zvenson/dinghy.git; git fetch -q --depth 1 origin "$DINGHY_COMMIT"; git checkout -q --detach FETCH_HEAD; [[ $(git rev-parse HEAD) == "$DINGHY_COMMIT" ]]; [[ $(git rev-parse 'HEAD^{tree}') == "$DINGHY_TREE" ]] )
cp UPSTREAM.lock.json build/evidence/
( cd work/dinghy && bash tests/run_host.sh ) 2>&1 | tee build/evidence/dinghy_host.log
( cd work/dinghy && bash ./build.sh ) 2>&1 | tee build/evidence/dinghy_target.log
cp -a work/dinghy work/mello; rm -rf work/mello/.git
cp overlay/dinghy.c work/mello/firmware/src/dinghy.c; cp mello/* work/mello/firmware/src/
python3 ci/patch_freestanding_libc.py work/mello/firmware/src
python3 ci/patch_dinghy_storage_guard.py work/mello/firmware/src/platform.c
python3 ci/patch_target_memory.py work/mello
python3 ci/patch_target_optimization.py work/mello/tools/build.py
python3 ci/patch_release_identity.py work/mello/tools/build.py
( cd work/mello && bash tests/run_host.sh ) 2>&1 | tee build/evidence/mello_dinghy_host.log
( cd work/mello && MELLO_PRODUCT="$PRODUCT" bash ./build.sh --release "$VERSION" ) 2>&1 | tee build/evidence/mello_target.log
PKG="work/mello/build/skurkymello-$VERSION.fwsc"; [[ -f "$PKG" ]]; cp "$PKG" build/
python3 ci/inspect_target_build.py --log build/evidence/mello_target.log --package "build/$(basename "$PKG")" --product "$PRODUCT" --out build/target_report.json | tee build/evidence/target_report.txt
sha256sum "build/$(basename "$PKG")" | tee build/SHA256SUMS
bash ci/emulator_smoke.sh "build/$(basename "$PKG")" | tee build/evidence/emulator_smoke.txt
cp -a work/emu-smoke build/evidence/emulator-smoke
printf 'PASS: target package %s (%s)\n' "$(basename "$PKG")" "$PRODUCT"
