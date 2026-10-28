#!/usr/bin/env python3
from pathlib import Path
import hashlib

EXPECTED = {
    Path('mello/mello_ui.c'): 'f6d26e0d1a57a51b08bf65e878144f5d48bfa248244d97b80c7a0842eb601818',
    Path('tools/control_authoritative_mapping.c'): 'bf688e15845324e1982336830f20c067118d90923f8ba6da0b692c94dac4914d',
    Path('tools/control_context_contract.c'): 'b5479562eb9a4aca5817663e835c04151e0c3337e4c9b131cb4c675daeabddfd',
}

for path, expected in EXPECTED.items():
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit(f'FAIL: WP-C5 locked output {path}: {actual} != {expected}')
    print(f'{path}: OK {actual}')

print('PASS: WP-C5 authoritative action mapping outputs locked')
