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
python3 ci/patch_sub_html_parity.py
# Lock the exact known-good parity base before adding the removable experiment.
echo '284ad8a2154260352e0d82de67bd3ea7c761e0940cd744af00113c6bd4eb0cbc  mello/mello_ui.c' | sha256sum -c -
echo 'f1713bfdc705926c8fa099708b71ca59d8efeae732319793985f0bb7309d5642  mello/mello_audio.c' | sha256sum -c -
echo 'c4257112882c57474f8e7644d786af57fcf1bd0814c553a51f20a2806a091db9  mello/mello_audio.h' | sha256sum -c -
echo 'ddba56b4ab34981189d2275d22e530e3767be1d00d79a063899fd5e1beb96416  mello/mello_engine.c' | sha256sum -c -
echo 'd31cea05b2da085bcadd27f6f0dc61e97f704ba4d08d89783daab3de550f721c  mello/mello_engine.h' | sha256sum -c -
# Experimental chord layer is one isolated patch. MELLO_FEATURE_CHORD_MACHINE=0
# compiles its state, events, DSP and page out without touching project storage.
CHORD_B64=${TMPDIR:-/tmp}/chord-machine.patch.b64
CHORD_GZ=${TMPDIR:-/tmp}/chord-machine.patch.gz
cat patches/chord_machine.patch.gz.b64.part-* > "$CHORD_B64"
echo 'c5d660edfbd1b25cc32550562707451b0989dac3982c7031dbc84ea937babb50  '"$CHORD_B64" | sha256sum -c -
base64 -d "$CHORD_B64" > "$CHORD_GZ"
echo '8cfb9f32f0b9280ab3309b65a21b1c796ab711c1a56f267393e4ffde6decb83a  '"$CHORD_GZ" | sha256sum -c -
gzip -t "$CHORD_GZ"
gzip -dc "$CHORD_GZ" | patch --batch --forward -p1
# Lock the proven experiment before cleanup.
echo '8e789d05a0c254a31ae38fac71d530534c7bc5714ab65c96fb381d647bc9e904  mello/mello_engine.h' | sha256sum -c -
echo '2a53b6b6d6e8214e527a742cbad697e0a7d59d37b3327dd214c9905bde94cdef  mello/mello_engine.c' | sha256sum -c -
echo '3bc1cb1962d8b193c3cd9c1b967d2f065b775c1b3577900c7a50a0b8216f5807  mello/mello_audio.h' | sha256sum -c -
echo '65d8499a09d8a0fbdf59d3c683a6d7c1bc07e4d6091f9f299571d4a67e1662ca  mello/mello_audio.c' | sha256sum -c -
echo 'ca6f6800bd6d44d7d8ffc767d43a65f811631b832898da7442f71900631fc7dc  mello/mello_ui.h' | sha256sum -c -
echo '2dfc33435e15ba0763910a83bd9ce89a9e557dfc27f32081a0d67a594cf02aac  mello/mello_ui.c' | sha256sum -c -
echo 'd49ccace38d402dc8ffd678934c20c61f8c9fd6988a53780c03ffd0c2d80e08b  mello/mello_transport.c' | sha256sum -c -
echo '1fa390edab81f7ea02770ca8dd09965b798f3cf839d2d284bc8bf7db5725d4ee  overlay/dinghy.c' | sha256sum -c -
# Cleanup remains separate so the proven experiment patch stays reproducible.
patch --batch --forward -p1 < patches/chord-machine-warning-fix.patch
# Hardware-first control grammar: keys own steps/cells; knobs stay musical.
CTRL_GZ=${TMPDIR:-/tmp}/control-grammar.patch.gz
echo 'd68bf633bd4244f19a1a27d3b3ba3a22af286e894cce50215797ebdbcdadee0e  patches/control-grammar.patch.gz.b64' | sha256sum -c -
base64 -d patches/control-grammar.patch.gz.b64 > "$CTRL_GZ"
echo '511e460bf7bc1910a44752d15dcfa6512b321c0d832720b7313c68a5751f47b6  '"$CTRL_GZ" | sha256sum -c -
gzip -t "$CTRL_GZ"
gzip -dc "$CTRL_GZ" | patch --batch --forward -p1
echo '7e120f6328b045ccfe88353318f7f2f5a81c0e0539b3eb51e84108c9b8e82ad2  mello/mello_ui.c' | sha256sum -c -
echo '21bfce74f58e7375705f7cb8e0d0420f2f14c5239e1e2ef23c682caf86fd205d  mello/mello_ui.h' | sha256sum -c -
# Loop Cycle audition/bass-transpose + text-fit pass. Storage wire format stays unchanged:
# sub tone uses low 3 bits and signed bass transpose uses the previously unused high 5 bits.
CYCLE_GZ=${TMPDIR:-/tmp}/cycle-audition-bass-ui.patch.gz
base64 -d patches/cycle-audition-bass-ui-source.patch.gz.b64 > "$CYCLE_GZ"
echo '481ba6b29dccc7dae27eb3be88abd1dd3b58c37af2453465951a139078e532cf  '"$CYCLE_GZ" | sha256sum -c -
gzip -t "$CYCLE_GZ"
gzip -dc "$CYCLE_GZ" | patch --batch --forward -p1
echo '535131cc59f4bc1204b41355622f57095132abbea08174679d01598f49de02b7  mello/mello_engine.h' | sha256sum -c -
echo 'd2f7c5d0d3837677b07fdf8bb8f48a5905de3d4443781311ccd08432d942a631  mello/mello_engine.c' | sha256sum -c -
echo 'de5afca281f725dfba0f982ccbfa25ad06fef7ab1fffead8ce1ad7fd5fe17974  mello/mello_ui.c' | sha256sum -c -
# Keep firmware freestanding: the footer sizing must not introduce libc strlen.
patch --batch --forward -p1 < patches/cycle-freestanding-ui.patch
echo '7c6ca30391fa7011f1d984c306640d69446454999a7a6055ed995e5cfe532c8d  mello/mello_ui.c' | sha256sum -c -
# Cycle regression overlays for repo-side tests; applied after source reconstruction.
CYCLE_TEST_GZ=${TMPDIR:-/tmp}/cycle-tests.patch.gz
base64 -d patches/cycle-tests.patch.gz.b64 > "$CYCLE_TEST_GZ"
echo '6e8b22d422bcace9c4750f4937985aa37c75ca3e09cc5d84fad50a6e0a772b34  '"$CYCLE_TEST_GZ" | sha256sum -c -
gzip -t "$CYCLE_TEST_GZ"
gzip -dc "$CYCLE_TEST_GZ" | patch --batch --forward -p1
echo 'PASS: reconstructed source + HTML audio/sub parity + removable chord machine + hardware-first controls + cycle audition/bass transpose/text-fit'
