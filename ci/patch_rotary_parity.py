#!/usr/bin/env python3
from pathlib import Path
import base64
import gzip
import hashlib
import subprocess

PATCH = Path('patches/wp-b2-rotary-parity.patch.gz.b64')
UI = Path('mello/mello_ui.c')
TEST = Path('tools/rotary_mapping_parity.c')
PRE_UI_SHA = 'd3d016c2d4e67d310c031b2d2aaf36c011783a8bbbd6a45712f84d2fe9cc6932'
POST_UI_SHA = 'ab38ade0a418017e639263ccf6e1fb7580f5f39b5510ed979db5bd3b366e00b5'
TEST_SHA = '3c5ac8c5d60017dbe3bc687b10023210106edcb6a02ecf29aa0b7ad4108bf629'
B64_SHA = 'a92dafc489654a4a53d95814fab4781264609dba294011f69092955d34c9cc7a'
GZ_SHA = '1d525003c2e5086ca178456beebe180439cf13efa72b75870fb5d87fb74fefc0'


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def file_sha(path: Path) -> str:
    return sha(path.read_bytes())


actual = file_sha(UI)
if actual != PRE_UI_SHA:
    raise SystemExit(f'FAIL: WP-B2 UI base hash {actual} != {PRE_UI_SHA}')
if file_sha(TEST) != TEST_SHA:
    raise SystemExit('FAIL: WP-B2 rotary mapping test hash')

b64 = PATCH.read_bytes().strip()
if sha(b64) != B64_SHA:
    raise SystemExit('FAIL: WP-B2 base64 package hash')
try:
    gz = base64.b64decode(b64, validate=True)
except Exception as exc:
    raise SystemExit(f'FAIL: WP-B2 base64 decode: {exc}')
if sha(gz) != GZ_SHA:
    raise SystemExit('FAIL: WP-B2 gzip package hash')
try:
    patch = gzip.decompress(gz)
except Exception as exc:
    raise SystemExit(f'FAIL: WP-B2 gzip decode: {exc}')

proc = subprocess.run(
    ['patch', '--batch', '--forward', '-p1'],
    input=patch,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if proc.returncode:
    print(proc.stdout.decode(errors='replace'), end='')
    raise SystemExit(f'FAIL: WP-B2 patch returned {proc.returncode}')

actual = file_sha(UI)
if actual != POST_UI_SHA:
    raise SystemExit(f'FAIL: WP-B2 UI output hash {actual} != {POST_UI_SHA}')

print('PASS: WP-B2 rotary display/handler parity patch applied')
