#!/usr/bin/env python3
from pathlib import Path
import hashlib

EXPECTED = {
    Path('mello/mello_ui.c'): 'c811c6ee8e5d6422a479452c65e0c4720d93d0b979cba3b4bcd1281f95a162da',
}

for path, expected in EXPECTED.items():
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit(f'FAIL: WP-C3 locked output {path}: {actual} != {expected}')
    print(f'{path}: OK {actual}')

print('PASS: WP-C3 exact generated UI output locked')
