#!/usr/bin/env python3
from pathlib import Path
import base64
import gzip
import hashlib
import subprocess

AUDIO = Path('mello/mello_audio.c')
HEADER = Path('mello/mello_audio.h')
PATCH_B64 = Path('patches/wp-e3-reverb-v3.patch.gz.b64')
PRE_AUDIO_SHA = '8a0adbcd9eadbefe7339e5ff4b5a3a82e951871375061ab06bd106f51fb2cb1f'
PRE_HEADER_SHA = '463e403fa7f5c927bfc69be48b6f0b18d475f21fe9ac45f7e6c285eaf7e0b63d'
POST_AUDIO_SHA = '3cb0ff57f5183e86d77a7dffb4d41f012eebe98cfe72bd3614c251d8d8758274'
POST_HEADER_SHA = 'd76d724a4833b1c05c6616c42722b14544ae5d6e91d314abb861237e550fcdd5'


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


if sha(AUDIO) != PRE_AUDIO_SHA:
    raise SystemExit(f'FAIL: WP-E3 audio base hash {sha(AUDIO)} != {PRE_AUDIO_SHA}')
if sha(HEADER) != PRE_HEADER_SHA:
    raise SystemExit(f'FAIL: WP-E3 header base hash {sha(HEADER)} != {PRE_HEADER_SHA}')
if not PATCH_B64.is_file():
    raise SystemExit('FAIL: WP-E3 patch payload missing')

patch = gzip.decompress(base64.b64decode(PATCH_B64.read_text()))
proc = subprocess.run(
    ['patch', '--batch', '--forward', '-p1'],
    input=patch,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if proc.returncode:
    print(proc.stdout.decode(errors='replace'), end='')
    raise SystemExit(f'FAIL: WP-E3 patch returned {proc.returncode}')

if sha(AUDIO) != POST_AUDIO_SHA:
    raise SystemExit(f'FAIL: WP-E3 audio output hash {sha(AUDIO)} != {POST_AUDIO_SHA}')
if sha(HEADER) != POST_HEADER_SHA:
    raise SystemExit(f'FAIL: WP-E3 header output hash {sha(HEADER)} != {POST_HEADER_SHA}')

print(f'PASS: WP-E3 Reverb V3 candidate A audio locked: {POST_AUDIO_SHA}')
print(f'PASS: WP-E3 Reverb V3 candidate A header locked: {POST_HEADER_SHA}')
