#!/usr/bin/env python3
from pathlib import Path
import base64
import gzip
import hashlib
import subprocess

PATCH_B64 = Path('patches/wp-b3-display-integrity.patch.gz.b64')
UI = Path('mello/mello_ui.c')
ROTARY = Path('tools/rotary_mapping_parity.c')
RANDOM_TEST = Path('tools/random_wave_overview_smoke.c')
TEST = Path('tools/ui_display_integrity.c')
PRE_UI_SHA = 'ab38ade0a418017e639263ccf6e1fb7580f5f39b5510ed979db5bd3b366e00b5'
POST_UI_SHA = 'e835f009eeb2c6525a9654488422f2187b0c489e7a37b73a90c758917d380701'
PATCH_B64_SHA = 'bc1c0e4f75a3024317b51d0f2a8d6cb18d2f0ae5eebda5b6477d661f64fdab8b'
ROTARY_PRE_SHA = '3c5ac8c5d60017dbe3bc687b10023210106edcb6a02ecf29aa0b7ad4108bf629'


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


if sha(UI) != PRE_UI_SHA:
    raise SystemExit(f'FAIL: WP-B3 UI base hash {sha(UI)} != {PRE_UI_SHA}')
if sha(PATCH_B64) != PATCH_B64_SHA:
    raise SystemExit('FAIL: WP-B3 patch transport hash')
if not TEST.is_file():
    raise SystemExit('FAIL: WP-B3 display-integrity test missing')
test_text = TEST.read_text()
for marker in ('FONT_S_ADV = 8', 'home_incremental_equivalence', 'MELLO_FEATURE_CHORD_MACHINE'):
    if marker not in test_text:
        raise SystemExit(f'FAIL: WP-B3 display-integrity test missing marker {marker!r}')
if sha(ROTARY) != ROTARY_PRE_SHA:
    raise SystemExit('FAIL: WP-B3 rotary fixture base hash')

try:
    patch_bytes = gzip.decompress(base64.b64decode(PATCH_B64.read_bytes(), validate=True))
except Exception as exc:
    raise SystemExit(f'FAIL: WP-B3 patch decode: {exc}') from exc
proc = subprocess.run(
    ['patch', '--batch', '--forward', '-p1'],
    input=patch_bytes,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if proc.returncode:
    print(proc.stdout.decode(errors='replace'), end='')
    raise SystemExit(f'FAIL: WP-B3 patch returned {proc.returncode}')
if sha(UI) != POST_UI_SHA:
    raise SystemExit(f'FAIL: WP-B3 UI output hash {sha(UI)} != {POST_UI_SHA}')

s = ROTARY.read_text()
replacements = (
    ('else if(y==203)', 'else if(y==204)'),
    ('"5SEQ LEN"', '"5SEQLEN"'),
    ('"MIDI OUT"', '"NOTEOUT"'),
)
for old, new in replacements:
    if s.count(old) != 1:
        raise SystemExit(f'FAIL: WP-B3 rotary fixture expected one {old!r}')
    s = s.replace(old, new)
ROTARY.write_text(s)

# WP-A's hard smoke is hash-locked before WP-B is applied. B3 changes only
# presentation geometry, so first locate the exact post-WP-A footer capture.
r = RANDOM_TEST.read_text()
if r.count('y==203') != 1:
    print('WP-B3 diagnostic: post-WP-A RANDOM capture lines follow')
    for lineno, line in enumerate(r.splitlines(), 1):
        if any(token in line for token in ('text_cb', 'INT', 'MED', 'LABEL', 'y==', 'y ==')):
            print(f'{lineno}: {line}')
    raise SystemExit('FAIL: WP-B3 footer capture needs exact post-WP-A adaptation')
r = r.replace('y==203', 'y==204')
RANDOM_TEST.write_text(r)

print('PASS: WP-B3 display-integrity patch + B2/WP-A test adaptations applied')
