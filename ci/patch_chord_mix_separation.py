#!/usr/bin/env python3
from pathlib import Path
import hashlib
import runpy

p = Path('mello/mello_audio.c')
data = p.read_text()
expected = '6d39c17e02aa3bd3617f8a0c0787a2ff9b99f14e05de91345770420151b73a72'
actual = hashlib.sha256(data.encode()).hexdigest()
if actual != expected:
    raise SystemExit(f'FAIL: chord mix separation base hash {actual} != {expected}')

needle = 'mix+=chord_mix*800/1000;'
pos = data.find(needle)
if pos < 0 or data.find(needle, pos + 1) >= 0:
    raise SystemExit('FAIL: expected exactly one chord mix insertion point')

start = data.rfind('#if MELLO_FEATURE_CHORD_MACHINE', 0, pos)
if start < 0:
    raise SystemExit('FAIL: chord mix feature guard start not found')
end = data.find('#endif', pos)
if end < 0:
    raise SystemExit('FAIL: chord mix feature guard end not found')
end += len('#endif')
while end < len(data) and data[end] in ' \t':
    end += 1
if end < len(data) and data[end] == '\n':
    end += 1

dist_start = data.find('{int32_t wet=mix;uint16_t ta;', end)
if dist_start < 0:
    raise SystemExit('FAIL: main distortion block not found after chord mix')
dist_end_marker = 'mix=a->dist_lp*(int32_t)a->fx.dist_level/1000;}'
dist_end = data.find(dist_end_marker, dist_start)
if dist_end < 0:
    raise SystemExit('FAIL: main distortion block end not found')
dist_end += len(dist_end_marker)

if data[end:dist_start].strip():
    raise SystemExit('FAIL: unexpected code between chord mix and main distortion')

new_block = '''#if MELLO_FEATURE_CHORD_MACHINE
    /* Chord keeps its own spectral identity and bypasses lead distortion, but shares delay/reverb space. */
    mix+=chord_mix*800/1000;
#endif
'''
data = data[:start] + data[dist_start:dist_end] + '\n' + new_block + data[dist_end:]
p.write_text(data)
print('PASS: chord bed bypasses lead filter/distortion and joins before shared delay/reverb')

# Next stabilization stage: replace the old two-tap reverb after chord routing is final.
runpy.run_path(str(Path(__file__).with_name('patch_reverb_v2.py')), run_name='__main__')
