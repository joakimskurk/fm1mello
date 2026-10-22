#!/usr/bin/env python3
from pathlib import Path
import hashlib

p = Path('mello/mello_audio.c')
data = p.read_text()
expected = '6d39c17e02aa3bd3617f8a0c0787a2ff9b99f14e05de91345770420151b73a72'
actual = hashlib.sha256(data.encode()).hexdigest()
if actual != expected:
    raise SystemExit(f'FAIL: chord mix separation base hash {actual} != {expected}')

chord = '''#if MELLO_FEATURE_CHORD_MACHINE
    /* Chord keeps its own spectral identity outside the main filter, but shares the main FX. */
    mix+=chord_mix*800/1000;
#endif
'''
if chord not in data:
    # Early go/no-go source used the shorter comment; accept that exact structural block only.
    chord = '''#if MELLO_FEATURE_CHORD_MACHINE
    mix+=chord_mix*800/1000;
#endif
'''
if chord not in data:
    raise SystemExit('FAIL: expected pre-distortion chord mix block not found')

start = data.index(chord)
after = start + len(chord)
dist_start = data.find('{int32_t wet=mix;uint16_t ta;', after)
if dist_start < 0:
    raise SystemExit('FAIL: main distortion block not found after chord mix')
dist_end_marker = 'mix=a->dist_lp*(int32_t)a->fx.dist_level/1000;}'
dist_end = data.find(dist_end_marker, dist_start)
if dist_end < 0:
    raise SystemExit('FAIL: main distortion block end not found')
dist_end += len(dist_end_marker)

between = data[after:dist_start]
if between.strip():
    raise SystemExit('FAIL: unexpected code between chord mix and main distortion')

replacement = data[dist_start:dist_end] + '''\n#if MELLO_FEATURE_CHORD_MACHINE
    /* Chord keeps its own spectral identity and bypasses lead distortion, but shares delay/reverb space. */
    mix+=chord_mix*800/1000;
#endif'''
data = data[:start] + replacement + data[dist_end:]

final = hashlib.sha256(data.encode()).hexdigest()
expected_final = '0becfec0fd78b3428c82643d96e9aaf0d1a5d48f51156fb2598bef141feffb76'
if final != expected_final:
    raise SystemExit(f'FAIL: chord mix separation result hash {final} != {expected_final}')
p.write_text(data)
print('PASS: chord bed bypasses lead filter/distortion and joins before shared delay/reverb')
