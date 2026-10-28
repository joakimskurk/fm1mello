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
PATCH_AUDIO_SHA = '3cb0ff57f5183e86d77a7dffb4d41f012eebe98cfe72bd3614c251d8d8758274'
CANDIDATE_A_CLEAN_SHA = 'f1edf754b9775fbdb7478a6c88400c668e1dec782f4da143f7befefa4e62e465'
CANDIDATE_A1_AUDIO_SHA = '57321ac12fe4194c4b89a20dbecb4f951a2c4064161728a76fd1d25b56fbdede'
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

# Lock the exact Candidate A patch output before source-hygiene/calibration changes.
if sha(AUDIO) != PATCH_AUDIO_SHA:
    raise SystemExit(f'FAIL: WP-E3 patched audio hash {sha(AUDIO)} != {PATCH_AUDIO_SHA}')
if sha(HEADER) != POST_HEADER_SHA:
    raise SystemExit(f'FAIL: WP-E3 header output hash {sha(HEADER)} != {POST_HEADER_SHA}')

text = AUDIO.read_text()
old = '    if(left)*left=0;if(right)*right=0;\n'
new = '    if(left)*left=0;\n    if(right)*right=0;\n'
if text.count(old) != 1:
    raise SystemExit(f'FAIL: WP-E3 stereo-null hygiene anchor count {text.count(old)} != 1')
text = text.replace(old, new, 1)
AUDIO.write_text(text)
if sha(AUDIO) != CANDIDATE_A_CLEAN_SHA:
    raise SystemExit(f'FAIL: WP-E3 Candidate A clean hash {sha(AUDIO)} != {CANDIDATE_A_CLEAN_SHA}')

# Candidate A.1 keeps the accepted dense/stereo topology unchanged and retunes only
# the loss/level calibration that listening exposed as too short and too restrained.
# The 1.6x internal decay compensation offsets frequency-dependent loop losses; the
# slightly higher feedback ceiling remains below unity. A modestly faster damping
# follower preserves more upper-mid air, while the small return trim restores useful
# wet range without changing the dry path.
text = AUDIO.read_text()
replacements = (
    (
        '    uint32_t d=decay_ms<400u?400u:decay_ms;\n',
        '    uint32_t raw=decay_ms<400u?400u:decay_ms;\n'
        '    uint32_t d=(raw*16u+5u)/10u;\n',
        'decay compensation',
    ),
    (
        '    if(fb>32604u)fb=32604u;\n',
        '    if(fb>32720u)fb=32720u;\n',
        'feedback ceiling',
    ),
    (
        '    base_alpha=(uint16_t)(8500u+(uint32_t)(a->fx.reverb_tone>1000u?1000u:a->fx.reverb_tone)*10u);\n',
        '    base_alpha=(uint16_t)(10000u+(uint32_t)(a->fx.reverb_tone>1000u?1000u:a->fx.reverb_tone)*10u);\n',
        'feedback damping calibration',
    ),
    (
        '            mix_l+=((hp_l*(int32_t)a->fx.reverb_level)/1000)*1050/1000;\n'
        '            mix_r+=((hp_r*(int32_t)a->fx.reverb_level)/1000)*1050/1000;\n',
        '            mix_l+=((hp_l*(int32_t)a->fx.reverb_level)/1000)*1150/1000;\n'
        '            mix_r+=((hp_r*(int32_t)a->fx.reverb_level)/1000)*1150/1000;\n',
        'wet return calibration',
    ),
)
for old, new, label in replacements:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f'FAIL: WP-E3 A.1 {label} anchor count {count} != 1')
    text = text.replace(old, new, 1)
AUDIO.write_text(text)
final_audio_sha = sha(AUDIO)
if final_audio_sha != CANDIDATE_A1_AUDIO_SHA:
    raise SystemExit(f'FAIL: WP-E3 Candidate A.1 audio hash {final_audio_sha} != {CANDIDATE_A1_AUDIO_SHA}')

print(f'PASS: WP-E3 Reverb V3 candidate A patch locked: {PATCH_AUDIO_SHA}')
print(f'PASS: WP-E3 Reverb V3 candidate A warning-clean audio locked: {CANDIDATE_A_CLEAN_SHA}')
print(f'PASS: WP-E3 Reverb V3 candidate A.1 calibrated audio: {CANDIDATE_A1_AUDIO_SHA}')
print(f'PASS: WP-E3 Reverb V3 candidate A header locked: {POST_HEADER_SHA}')
