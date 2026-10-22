#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
bash ci/unpack_source.sh
make clean test sanitize
if grep -RInE '\b(float|double|uint64_t|int64_t)\b|long[[:space:]]+long' mello; then echo 'FAIL: forbidden target-risk scalar type in mello/' >&2; exit 1; fi
if grep -RInE '\b(malloc|calloc|realloc|free)[[:space:]]*\(' mello; then echo 'FAIL: dynamic allocation in mello/' >&2; exit 1; fi
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/platform.c" <<'EOG'
#define FL_RANGE_OK(off, n) (FL_IN(off, n, FL_OTA_LO, FL_OTA_HI) || FL_IN(off, n, SETTINGS_AT, SETTINGS_AT + 0x1000u))
EOG
python3 ci/patch_dinghy_storage_guard.py "$TMP/platform.c" >/dev/null
grep -q 'MELLO_STORAGE_LO 0x00098000u' "$TMP/platform.c"
grep -q 'MELLO_STORAGE_HI 0x000E0000u' "$TMP/platform.c"
cp overlay/dinghy.c "$TMP/dinghy.c"; cp mello/* "$TMP/"; cp tests/stub/platform.c "$TMP/platform.c"
( cd "$TMP"; cc -DDINGHY_HOST=1 -std=c11 -O2 -Wall -Wextra -Werror -Wno-unused-function -pedantic -c dinghy.c -o host.o )
( cd "$TMP"; cc -std=c11 -O2 -Wall -Wextra -Werror -Wno-unused-function -pedantic -c dinghy.c -o target_syntax.o )
python3 - "$TMP/cross.wav" <<'PY'
import math,struct,sys,wave
with wave.open(sys.argv[1],'wb') as w:
    w.setnchannels(2); w.setsampwidth(2); w.setframerate(44100)
    w.writeframes(b''.join(struct.pack('<hh', int(12000*math.sin(2*math.pi*220*i/44100)), -int(6000*math.sin(2*math.pi*220*i/44100))) for i in range(44100)))
PY
python3 tools/mello_sample_upload.py "$TMP/cross.wav" --slot 3 --name CROSS_TEST --out "$TMP/cross.mello-sample"
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Imello mello/mello_sample.c tools/verify_sample_record.c -o "$TMP/verify_sample"
"$TMP/verify_sample" "$TMP/cross.mello-sample" 2 CROSS_TEST 22050
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Imello $ROOT/mello/*.c tools/struct_sizes.c -o "$TMP/sizes"; "$TMP/sizes"
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Imello mello/mello_engine.c mello/mello_transport.c mello/mello_storage.c mello/mello_ui.c mello/mello_audio.c mello/mello_sample.c tools/audio_bench.c -o "$TMP/bench"; "$TMP/bench"
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Imello mello/mello_engine.c mello/mello_transport.c mello/mello_storage.c mello/mello_ui.c mello/mello_audio.c mello/mello_sample.c tools/ui_visual_smoke.c -o "$TMP/ui_visual_smoke"; "$TMP/ui_visual_smoke"
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Imello mello/mello_engine.c mello/mello_transport.c mello/mello_storage.c mello/mello_ui.c mello/mello_audio.c mello/mello_sample.c tools/ui_wave_parity.c -o "$TMP/ui_wave_parity"; "$TMP/ui_wave_parity"
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic -Imello mello/mello_engine.c mello/mello_transport.c mello/mello_storage.c mello/mello_ui.c mello/mello_audio.c mello/mello_sample.c tools/ui_control_parity.c -o "$TMP/ui_control_parity"; "$TMP/ui_control_parity"
echo 'PASS: fm1mello host gate'
