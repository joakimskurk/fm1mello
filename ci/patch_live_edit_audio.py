#!/usr/bin/env python3
from pathlib import Path
import base64
import gzip
import hashlib
import subprocess

PATCH = Path('patches/wp-b-live-edit-audio.patch.gz.b64')
PRE = {
    Path('mello/mello_audio.c'): 'c97c3510cd36257b8555342b80621b71c85b7a7171834778598df420eb677b5d',
    Path('mello/mello_audio.h'): '8455145a37c7612f83d495252d69ff4e90273a7c6d8b5b44a924dd16bcfe2f8d',
}
POST = {
    Path('mello/mello_audio.c'): '728ff16c9b6a1b04b760b7c9e4b440a85e7cf50cd0be0fe7b914e9a5367e3284',
    Path('mello/mello_audio.h'): 'e15c31501edd75816445af65c95ed255f584ba7df9636b3d0f66dd7a76bb2c89',
}
B64_SHA = '66864287502720f6c7e1a4c176adfe13bbd6bfee012100e93e35e7fc5e3f7537'
GZ_SHA = '7042d6b57657607d7b07444592704ccc8342022604a0249a322ecd54c0ce4f99'


def digest_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def digest_file(path: Path) -> str:
    return digest_bytes(path.read_bytes())


for path, expected in PRE.items():
    actual = digest_file(path)
    if actual != expected:
        raise SystemExit(f'FAIL: WP-B live-edit base hash {path} {actual} != {expected}')

# Repository text transport may add a trailing newline to the base64 payload.
# Hash and decode the normalized payload while still locking its exact bytes.
b64 = PATCH.read_bytes().strip()
if digest_bytes(b64) != B64_SHA:
    raise SystemExit('FAIL: WP-B live-edit base64 package hash')
try:
    gz = base64.b64decode(b64, validate=True)
except Exception as exc:
    raise SystemExit(f'FAIL: WP-B live-edit base64 decode: {exc}')
if digest_bytes(gz) != GZ_SHA:
    raise SystemExit('FAIL: WP-B live-edit gzip package hash')
try:
    patch = gzip.decompress(gz)
except Exception as exc:
    raise SystemExit(f'FAIL: WP-B live-edit gzip decode: {exc}')

proc = subprocess.run(
    ['patch', '--batch', '--forward', '-p1'],
    input=patch,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if proc.returncode:
    print(proc.stdout.decode(errors='replace'), end='')
    raise SystemExit(f'FAIL: WP-B live-edit patch returned {proc.returncode}')

for path, expected in POST.items():
    actual = digest_file(path)
    if actual != expected:
        raise SystemExit(f'FAIL: WP-B live-edit output hash {path} {actual} != {expected}')

print('PASS: WP-B live-edit audio publication/rebuild patch applied')
