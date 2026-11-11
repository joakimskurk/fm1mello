#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
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
    audio_html_parity | wp_e3_reverb_v3_smoke | reverb_decay | oliverb_knobs | delay_loop_fx) echo -DMELLO_REVERB_TEST_HOOK=1 ;;
    live_edit_audio_stress) echo -DMELLO_LIVE_EDIT_TEST_HOOK=1 ;;
    cpu1_split) echo -pthread ;; # a thread is its second core
    drive_makeup | delay_pingpong | perform_keys) echo -DMELLO_OUTPUT_GAIN_Q12=4096 ;;
    audio_overflow_regression | audio_grit_smoke | audio_virus_smoke | chord_voice | drum_machine | random_musical | bass_seq | step_wave | drum_routes) echo -fsanitize=undefined -fno-sanitize-recover=undefined ;;
    esac
}

# The Chord Machine must also compile out (MELLO_FEATURE_CHORD_MACHINE=0) without touching the rest.
FEATURE_OFF="style_roll roll_parts ui_visual_smoke control_grammar_smoke chord_final_acceptance audio_timing_stress random_wave_overview_smoke chord_tone_smoke storage_chord_regression"

run() {    # name, binary suffix, extra flags
    bin="$OUT/$1$2"
    # shellcheck disable=SC2086
    if ! "$CC" -std=c11 -O1 -Wall -Wextra -Werror $EXTRA $3 $(defines "$1") -Ifirmware/src "tests/mello/$1.c" $SRC -o "$bin" -lm; then
        echo "FAIL $1$2 (does not compile)"; FAILED=1; return 0   # the other tests still run
    fi
    if (cd "$OUT" && "./$1$2" >"$1$2.txt" 2>&1); then
        echo "PASS $1$2"
    else
        echo "FAIL $1$2 (see $bin.txt)"; tail -5 "$bin.txt"; FAILED=1
    fi
}

# sample_transfer_roundtrip also feeds in a record made by the host tool (tools/fm1_samples.py): a 440 Hz sine,
# 1 s at 22 050 Hz, named PYTONE, for slot 1. Without python3 the test says so and skips that part.
rm -f "$OUT/py_record.bin"
if command -v python3 >/dev/null 2>&1; then
    python3 -c '
import math, struct, sys, wave
w = wave.open(sys.argv[1], "wb"); w.setnchannels(1); w.setsampwidth(2); w.setframerate(22050)
w.writeframes(b"".join(struct.pack("<h", int(16383.0 * math.sin(6.2831853 * 440.0 * i / 22050))) for i in range(22050)))
w.close()' "$OUT/py_tone.wav" &&
        python3 tools/fm1_samples.py pack "$OUT/py_tone.wav" --slot 1 --name PYTONE -o "$OUT/py_record.bin" >/dev/null ||
        rm -f "$OUT/py_record.bin"
fi

FAILED=0
if [ $# -gt 0 ]; then NAMES="$*"; else NAMES=$(cd tests/mello && ls ./*.c | sed 's|^\./||; s|\.c$||'); fi
for n in $NAMES; do
    run "$n" "" ""
    case " $FEATURE_OFF " in *" $n "*) run "$n" "_chord_off" "-DMELLO_FEATURE_CHORD_MACHINE=0" ;; esac
done
# Dinghy's own host tests, in the full run (taken with Dinghy 0.5; docs/mello/DINGHY_0_4.md): the installer against a
# simulated FM-1 (python3, no device, no mido) and the update loader's screen
dinghy_test() {    # name, command...
    name="$1"
    shift
    if "$@" >"$OUT/$name.txt" 2>&1; then
        echo "PASS $name"
    else
        echo "FAIL $name (see $OUT/$name.txt)"; tail -5 "$OUT/$name.txt"; FAILED=1
    fi
}
if [ $# -eq 0 ]; then
    if command -v python3 >/dev/null 2>&1; then
        dinghy_test install_test python3 -I tests/install_test.py
    else
        echo "install_test: no python3, skipped"
    fi
    # the update loader's screen (loader/ldr_screen.c through lcd.c on a fake panel), with tools/build.py's name and colour
    ldr_name=$(sed -n 's/^SCREEN_NAME = "\([A-Z0-9 ]*\)".*/\1/p' tools/build.py)
    ldr_colour=$(sed -n 's/^SCREEN_COLOUR = \(0x[0-9A-Fa-f]*\).*/\1/p' tools/build.py)
    if "$CC" -std=gnu11 -O1 -Wall -Wextra -Wno-unused-function -Werror -DLDR_NAME="\"$ldr_name\"" -DLDR_COLOUR="${ldr_colour}u" \
        tests/ldr_screen_test.c -o "$OUT/ldr_screen_test"; then
        dinghy_test ldr_screen_test "$OUT/ldr_screen_test" "$OUT"
    else
        echo "FAIL ldr_screen_test (does not compile)"; FAILED=1
    fi
fi
if [ "$FAILED" = 0 ]; then
    echo "all Mello host tests passed"
else    # loud, and last, so a run read through tail cannot pass for green
    grep -l '^FAIL' "$OUT"/*.txt 2>/dev/null | sed 's|.*/||; s|\.txt$||; s|^|  red: |'
    echo "MELLO HOST TESTS FAILED"
fi
exit "$FAILED"
