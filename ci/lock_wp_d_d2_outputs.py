#!/usr/bin/env python3
from pathlib import Path
import hashlib

EXPECTED = {
    Path('mello/mello_engine.c'): 'b76dd8f25a1e2b218e0e32652578001cf073ca50f6198e95af9866b3cfd51937',
    Path('mello/mello_engine.h'): '769ea7e8b7a1be51d38bbd48e5eb09e39114dd79ca363dcb70b235e1483cdab8',
}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


for path, expected in EXPECTED.items():
    actual = sha(path)
    if actual != expected:
        raise SystemExit(f'FAIL: WP-D D2 output lock {path}: {actual} != {expected}')
    print(f'PASS: WP-D D2 output lock {path}: {actual}')

engine = Path('mello/mello_engine.c').read_text()
header = Path('mello/mello_engine.h').read_text()
for marker in (
    'p->master_len=32u; /* WP-D: default 2 BAR = 32 sixteenth notes. */',
    '(uint32_t)e->project.master_len/(uint32_t)e->project.clock_div',
):
    if marker not in engine:
        raise SystemExit(f'FAIL: WP-D D2 engine marker missing: {marker}')
if '0=FREE. Non-zero is musical length in 1/16-note units' not in header:
    raise SystemExit('FAIL: WP-D D2 master_len semantic marker missing')

print('PASS: WP-D D2 generated timing outputs locked')
