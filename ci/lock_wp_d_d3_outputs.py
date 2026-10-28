#!/usr/bin/env python3
from pathlib import Path
import hashlib

UI = Path('mello/mello_ui.c')
EXPECTED_UI_SHA = '08bd19d7a2f112de318d50516c5a27b3ed2324d0c82bd50a087caadd0b7ecd55'


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


actual = sha(UI)
if actual != EXPECTED_UI_SHA:
    raise SystemExit(f'FAIL: WP-D D3 UI output lock {actual} != {EXPECTED_UI_SHA}')

s = UI.read_text()
checks = {
    'canonical choices': 'static const uint16_t master_lengths[] = {0u,16u,32u,64u,128u,256u};',
    'FREE formatter': 'if (master_len == 0u) { copy_s(b, "FREE"); return; }',
    'BAR formatter': 'copy_s(p, " BAR");',
    'main reset footer': 'l[1]="RESET";main_reset_name(v[1],e->project.master_len);',
    'Loop Cycle sync stays separate': 'l[1]="SYNC";copy_s(v[1],e->loop_cycle.sync?"SYNC":"FREE");',
    'external sync stays separate': 'l[2]="SYNC";copy_s(v[2],sync_name(ui->transport->sync_mode));',
    'slot hold stays separate': 'l[1]="HOLD";value_u(v[1],"",ss->hold);',
}
for name, needle in checks.items():
    if needle not in s:
        raise SystemExit(f'FAIL: WP-D D3 {name}: {needle}')
    print(f'PASS: WP-D D3 {name}')

legacy = 'l[1]="RESET";value_u(v[1],"",e->project.master_len);'
if legacy in s:
    raise SystemExit('FAIL: WP-D D3 legacy numeric main RESET still rendered')

print(f'PASS: WP-D D3 generated UI locked: {actual}')
