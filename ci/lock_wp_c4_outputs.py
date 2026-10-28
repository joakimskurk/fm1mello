#!/usr/bin/env python3
from pathlib import Path
import hashlib

EXPECTED = {
    Path('mello/mello_ui.c'): 'a3829d2fdc354154021727838b4b17cf8420832f88565a2b2f290b17a06002c5',
    Path('tools/rotary_mapping_parity.c'): '63cb37c6ed51df12b3ebef8510ee0decf3fab41772a37bfcbd1294adde2c1749',
    Path('tools/ui_control_parity.c'): '86323be0741c041c6a809cbe14af39cc0cd7fa909d70649b625ad9278ed0d511',
    Path('tools/control_encoder_baseline.c'): 'f83eedd2a2130a978fc30f02896f0316ab274b18978fe97b55e409f4bfb4fdb8',
    Path('tools/control_grammar_smoke.c'): '6fd837ae7ff7eabaf8e4d7e3e04d7f6bac8d3dfa18e3988d16f3537d942f85c9',
}

for path, expected in EXPECTED.items():
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit(f'FAIL: WP-C4 locked output {path}: {actual} != {expected}')
    print(f'{path}: OK {actual}')

print('PASS: WP-C4 exact generated UI/control outputs locked')
