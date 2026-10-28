#!/usr/bin/env python3
from pathlib import Path
import hashlib

EXPECTED = {
    Path('mello/mello_ui.c'): '3da137c8faa5800bcfddd8291176e77718bc320d890f20f9f9d0c9cba4aaffb0',
    Path('tools/random_wave_overview_smoke.c'): '71fc32a27fedfb0509ab0ff9eccfc85ba53d90c9b6d6bf48f0915f15c1d5fa13',
}

for path, expected in EXPECTED.items():
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit(f'FAIL: WP-C2 locked output {path}: {actual} != {expected}')
    print(f'{path}: OK {actual}')

print('PASS: WP-C2 exact generated UI and migrated WP-A smoke outputs locked')
