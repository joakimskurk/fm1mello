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
    Path('mello/mello_audio.c'): '3bfc64f6b101a8123b1533014fece746073bfac22055b83434252d8e0252ff85',
    Path('mello/mello_audio.h'): '302e217d551d1f250269ca37216217cffd067656a7db371ad1dc91e7ec7c61b5',
}
B64_SHA = 'ad4ca5f42e5de8f2c689c28a8a671187b748bed49208bedf73ac962ca1bda40d'
GZ_SHA = '09c4879c535cb3b758e59bcda38bb5e4a30d0ae2574083a20f1f54b819c95534'


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
