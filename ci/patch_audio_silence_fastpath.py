#!/usr/bin/env python3
from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit('usage: patch_audio_silence_fastpath.py <mello_audio.c>')
p = Path(sys.argv[1])
s = p.read_text()
old = 'if(!a||!out)return;apply_pending(a);if(a->all_notes_off_pending)'
new = ('if(!a||!out)return;apply_pending(a);'
       'if(a->silent&&a->q_head==a->q_tail&&!a->all_notes_off_pending)'
       '{a->rendered_frames+=frames;return;}'
       'if(a->all_notes_off_pending)')
if old not in s:
    if new in s:
        print('audio silent fast path already present')
        raise SystemExit(0)
    raise SystemExit('FAIL: mello_audio_render anchor not found')
s = s.replace(old, new, 1)
p.write_text(s)
print('patched audio silent fast path: skip full DSP while silent and event queue empty')
