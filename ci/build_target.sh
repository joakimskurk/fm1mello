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
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I mello tools/ui_visual_smoke.c mello/*.c -o build/evidence/ui_visual_smoke
build/evidence/ui_visual_smoke | tee build/evidence/ui_visual_smoke.txt
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I mello tools/ui_wave_parity.c mello/*.c -o build/evidence/ui_wave_parity
build/evidence/ui_wave_parity | tee build/evidence/ui_wave_parity.txt
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I mello tools/ui_control_parity.c mello/*.c -o build/evidence/ui_control_parity
build/evidence/ui_control_parity | tee build/evidence/ui_control_parity.txt
# Reverb IR probe is test-only and must never be present in target firmware.
${CC:-cc} -std=c11 -Wall -Wextra -Werror -DMELLO_REVERB_TEST_HOOK=1 -I mello tools/audio_html_parity.c mello/*.c -o build/evidence/audio_html_parity
build/evidence/audio_html_parity | tee build/evidence/audio_html_parity.txt
# Dedicated-sub parity is a hard gate: Tone.js biquads, envelope, glide and drive.
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I mello tools/sub_html_parity.c mello/*.c -o build/evidence/sub_html_parity
build/evidence/sub_html_parity | tee build/evidence/sub_html_parity.txt
# Experimental chord machine: functional behavior plus compile-out reversibility are hard gates.
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I mello tools/chord_machine_smoke.c mello/*.c -o build/evidence/chord_machine_smoke
build/evidence/chord_machine_smoke | tee build/evidence/chord_machine_smoke.txt
${CC:-cc} -std=c11 -Wall -Wextra -Werror -DMELLO_FEATURE_CHORD_MACHINE=0 -I mello tools/ui_visual_smoke.c mello/*.c -o build/evidence/chord_feature_off_smoke
build/evidence/chord_feature_off_smoke | tee build/evidence/chord_feature_off_smoke.txt
# Hardware-first controls are a hard UX contract. Prove both feature-on and compile-out paths.
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I mello tools/control_grammar_smoke.c mello/*.c -o build/evidence/control_grammar_smoke
build/evidence/control_grammar_smoke | tee build/evidence/control_grammar_smoke.txt
${CC:-cc} -std=c11 -Wall -Wextra -Werror -DMELLO_FEATURE_CHORD_MACHINE=0 -I mello tools/control_grammar_smoke.c mello/*.c -o build/evidence/control_grammar_feature_off
build/evidence/control_grammar_feature_off | tee build/evidence/control_grammar_feature_off.txt
# Timing architecture: future releases must never block current attacks in the audio ISR.
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I mello tools/audio_event_order_smoke.c mello/*.c -o build/evidence/audio_event_order_smoke
build/evidence/audio_event_order_smoke | tee build/evidence/audio_event_order_smoke.txt
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I mello tools/audio_timing_stress.c mello/*.c -o build/evidence/audio_timing_stress
build/evidence/audio_timing_stress | tee build/evidence/audio_timing_stress.txt
# Chord tone gets one explicit go/no-go pass: independent spectral slot, open default voicing.
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I mello tools/chord_tone_smoke.c mello/*.c -o build/evidence/chord_tone_smoke
build/evidence/chord_tone_smoke | tee build/evidence/chord_tone_smoke.txt
# Randomizer WAVE must stay isolated; all 32 main steps must remain visible in overview UIs.
${CC:-cc} -std=c11 -Wall -Wextra -Werror -I mello tools/random_wave_overview_smoke.c mello/*.c -o build/evidence/random_wave_overview_smoke
build/evidence/random_wave_overview_smoke | tee build/evidence/random_wave_overview_smoke.txt
# Preserve compile-out reversibility with the new timing/overview architecture too.
${CC:-cc} -std=c11 -Wall -Wextra -Werror -DMELLO_FEATURE_CHORD_MACHINE=0 -I mello tools/audio_timing_stress.c mello/*.c -o build/evidence/audio_timing_feature_off
build/evidence/audio_timing_feature_off | tee build/evidence/audio_timing_feature_off.txt
${CC:-cc} -std=c11 -Wall -Wextra -Werror -DMELLO_FEATURE_CHORD_MACHINE=0 -I mello tools/random_wave_overview_smoke.c mello/*.c -o build/evidence/random_wave_feature_off
build/evidence/random_wave_feature_off | tee build/evidence/random_wave_feature_off.txt
${CC:-cc} -std=c11 -Wall -Wextra -Werror -DMELLO_FEATURE_CHORD_MACHINE=0 -I mello tools/chord_tone_smoke.c mello/*.c -o build/evidence/chord_tone_feature_off
build/evidence/chord_tone_feature_off | tee build/evidence/chord_tone_feature_off.txt
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
python3 ci/patch_audio_silence_fastpath.py work/mello/firmware/src/mello_audio.c
python3 ci/patch_target_optimization.py work/mello/tools/build.py
python3 ci/patch_release_identity.py work/mello/tools/build.py
( cd work/mello && bash tests/run_host.sh ) 2>&1 | tee build/evidence/mello_dinghy_host.log
( cd work/mello && MELLO_PRODUCT="$PRODUCT" bash ./build.sh --release "$VERSION" ) 2>&1 | tee build/evidence/mello_target.log
PKG="work/mello/build/skurkymello-$VERSION.fwsc"; [[ -f "$PKG" ]]; cp "$PKG" build/
python3 ci/inspect_target_build.py --log build/evidence/mello_target.log --package "build/$(basename "$PKG")" --product "$PRODUCT" --out build/target_report.json | tee build/evidence/target_report.txt
sha256sum "build/$(basename "$PKG")" | tee build/SHA256SUMS
bash ci/emulator_smoke.sh "build/$(basename "$PKG")" | tee build/evidence/emulator_smoke.txt
cp -a work/emu-smoke build/evidence/emulator-smoke
bash ci/web_100mhz_smoke.sh "build/$(basename "$PKG")" | tee build/evidence/web_100mhz_smoke.txt
cp -a work/web100 build/evidence/web-100mhz
bash ci/chord_80mhz_smoke.sh "build/$(basename "$PKG")" | tee build/evidence/chord_80mhz_smoke.txt
cp -a work/chord80 build/evidence/chord-80mhz
bash ci/frontend_e2e.sh "build/$(basename "$PKG")" | tee build/evidence/frontend_e2e.txt
cp -a work/frontend-e2e build/evidence/frontend-e2e
printf 'PASS: target package %s (%s)\n' "$(basename "$PKG")" "$PRODUCT"
