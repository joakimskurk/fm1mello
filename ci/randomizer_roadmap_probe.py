#!/usr/bin/env python3
from pathlib import Path

engine_h=Path('mello/mello_engine.h').read_text().splitlines()
engine_c=Path('mello/mello_engine.c').read_text().splitlines()
ui_h=Path('mello/mello_ui.h').read_text().splitlines()
ui_c=Path('mello/mello_ui.c').read_text().splitlines()
storage_c=Path('mello/mello_storage.c').read_text().splitlines()

def block(label, lines, lo, hi):
    print(f'\n=== {label} {lo}-{hi} ===')
    for i in range(lo-1,min(hi,len(lines))): print(f'{i+1:4}: {lines[i]}')

print('=== RANDOMIZER ROADMAP PROBE V2 ===')
block('MelloStep + project structs',engine_h,180,250)
block('sanitize project',engine_c,65,110)

# Find complete encoder/input functions and all RANDOM cases.
for idx,line in enumerate(ui_c):
    if 'encoder_delta' in line or 'handle_encoder' in line or 'case MELLO_UI_RANDOM' in line or 'K1' in line or 'knob' in line.lower():
        lo=max(0,idx-12);hi=min(len(ui_c),idx+28)
        print(f'\n=== UI CONTEXT @{idx+1} ===')
        for j in range(lo,hi): print(f'{j+1:4}: {ui_c[j]}')

for idx,line in enumerate(ui_h):
    if 'typedef struct' in line or 'selected_' in line or 'subpage' in line:
        if idx>80:
            lo=max(0,idx-3);hi=min(len(ui_h),idx+18)
            print(f'\n=== UI HEADER @{idx+1} ===')
            for j in range(lo,hi): print(f'{j+1:4}: {ui_h[j]}')

for idx,line in enumerate(storage_c):
    if 'SNAPSHOT_WIRE_SIZE' in line or 'SECVER_WORKING' in line or 'SECVER_SLOTS' in line or 'randomize.mask' in line:
        lo=max(0,idx-4);hi=min(len(storage_c),idx+8)
        print(f'\n=== STORAGE @{idx+1} ===')
        for j in range(lo,hi): print(f'{j+1:4}: {storage_c[j]}')

print('=== END RANDOMIZER ROADMAP PROBE V2 ===')
raise SystemExit('RANDOMIZER ROADMAP PROBE V2 COMPLETE')
