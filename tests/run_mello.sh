#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build and run SkurkyMello's host tests (tests/mello/*.c) against firmware/src/mello_*.c.
#   tests/run_mello.sh [name ...]      (default: all)
# Each test is built with -Werror and fails the run on a non-zero exit. Output goes to build/mello-tests/.
set -e
cd "$(dirname "$0")/.."
CC="${CC:-cc}"
EXTRA="${MELLO_TEST_CFLAGS:-}"    # e.g. make sanitize
OUT=build/mello-tests
mkdir -p "$OUT"
SRC="firmware/src/mello_engine.c firmware/src/mello_transport.c firmware/src/mello_storage.c firmware/src/mello_ui.c firmware/src/mello_audio.c firmware/src/mello_sample.c"

defines() {
    case "$1" in
    audio_html_parity | wp_e3_reverb_v3_smoke) echo -DMELLO_REVERB_TEST_HOOK=1 ;;
    live_edit_audio_stress) echo -DMELLO_LIVE_EDIT_TEST_HOOK=1 ;;
    audio_overflow_regression | audio_grit_smoke | audio_virus_smoke) echo -fsanitize=undefined -fno-sanitize-recover=undefined ;;
    esac
}

# The Chord Machine must also compile out (MELLO_FEATURE_CHORD_MACHINE=0) without touching the rest.
FEATURE_OFF="ui_visual_smoke control_grammar_smoke chord_final_acceptance audio_timing_stress random_wave_overview_smoke chord_tone_smoke storage_chord_regression"

run() {    # name, binary suffix, extra flags
    bin="$OUT/$1$2"
    # shellcheck disable=SC2086
    "$CC" -std=c11 -O1 -Wall -Wextra -Werror $EXTRA $3 $(defines "$1") -Ifirmware/src "tests/mello/$1.c" $SRC -o "$bin"
    if (cd "$OUT" && "./$1$2" >"$1$2.txt" 2>&1); then
        echo "PASS $1$2"
    else
        echo "FAIL $1$2 (see $bin.txt)"; tail -5 "$bin.txt"; FAILED=1
    fi
}

FAILED=0
if [ $# -gt 0 ]; then NAMES="$*"; else NAMES=$(cd tests/mello && ls ./*.c | sed 's|^\./||; s|\.c$||'); fi
for n in $NAMES; do
    run "$n" "" ""
    case " $FEATURE_OFF " in *" $n "*) run "$n" "_chord_off" "-DMELLO_FEATURE_CHORD_MACHINE=0" ;; esac
done
[ "$FAILED" = 0 ] && echo "all Mello host tests passed"
exit "$FAILED"
