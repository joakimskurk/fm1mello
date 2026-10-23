#!/usr/bin/env python3
from pathlib import Path

c = Path('mello/mello_ui.c').read_text().splitlines()
h = Path('mello/mello_ui.h').read_text().splitlines()

def block(label, lines, lo, hi):
    print(f'\n=== {label} {lo}-{hi} ===')
    for i in range(lo-1, min(hi, len(lines))):
        print(f'{i+1:4}: {lines[i]}')

print('=== UI ROADMAP PROBE V2 ===')
for i,line in enumerate(h):
    if 'MELLO_UI_DIRTY' in line or 'home_old_step' in line or 'home_new_step' in line or 'seen_step' in line:
        lo=max(0,i-3);hi=min(len(h),i+5)
        print(f'\n--- HEADER @{i+1} ---')
        for j in range(lo,hi): print(f'{j+1:4}: {h[j]}')
block('TICK', c, 1145, 1190)
block('OVERVIEW HELPERS', c, 1400, 1450)
block('HOME STEP/BODY', c, 1468, 1555)
block('STEP BODY', c, 1570, 1655)
block('SEGMENT BODY', c, 1655, 1715)
block('RENDER', c, 2228, 2252)
print('=== END UI ROADMAP PROBE V2 ===')
raise SystemExit('UI ROADMAP PROBE COMPLETE')
