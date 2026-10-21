#!/usr/bin/env python3
from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: patch_target_memory.py /path/to/release/tree")
root = Path(sys.argv[1])
platform = root / "firmware/src/platform.c"
audio_h = root / "firmware/src/mello_audio.h"

# SkurkyMello only uses draw_text_box() from Dinghy's small-canvas renderer.
# FONT_L is 32 px high, so 240x32 is sufficient for every Mello, crash and
# updater text-box canvas. The stock Dinghy demo needs 56 px for draw_keys(),
# but that app is replaced in the release tree.
s = platform.read_text()
old = '#define CV_MAX (240u * 64u)                     /* the biggest text box drawn at once */'
new = '#define CV_MAX (240u * 32u)                     /* Mello: FONT_L is the tallest canvas user */'
if old not in s:
    raise SystemExit("refusing to patch platform.c: expected CV_MAX line missing")
s = s.replace(old, new, 1)
platform.write_text(s)

# Max Mello reverb tap at PRE=120 ms, SIZE=1000 is 23392 samples:
# ms_samples(120)=5292; B tap = 5292+5100+13000. 24576 preserves the full
# control/tap range while avoiding unused ring capacity.
s = audio_h.read_text()
old = '#define MELLO_AUDIO_REVERB_SAMPLES 32768u'
new = '#define MELLO_AUDIO_REVERB_SAMPLES 24576u'
if old not in s:
    raise SystemExit("refusing to patch mello_audio.h: expected reverb capacity missing")
s = s.replace(old, new, 1)
audio_h.write_text(s)

if 'CV_MAX (240u * 32u)' not in platform.read_text():
    raise SystemExit("postcondition failed: CV_MAX")
if 'MELLO_AUDIO_REVERB_SAMPLES 24576u' not in audio_h.read_text():
    raise SystemExit("postcondition failed: reverb capacity")
print("patched target pool budget: CV scratch -15360 B, reverb ring -16384 B")
