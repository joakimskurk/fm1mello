#!/usr/bin/env python3
from pathlib import Path
import re

files = {
    'ENGINE_H': Path('mello/mello_engine.h'),
    'ENGINE_C': Path('mello/mello_engine.c'),
    'UI_H': Path('mello/mello_ui.h'),
    'UI_C': Path('mello/mello_ui.c'),
    'STORAGE_C': Path('mello/mello_storage.c'),
    'STORAGE_H': Path('mello/mello_storage.h'),
}

print('=== RANDOMIZER ROADMAP PROBE ===')
for label,p in files.items():
    if not p.exists():
        continue
    lines=p.read_text().splitlines()
    hits=[]
    for i,line in enumerate(lines):
        low=line.lower()
        if any(k in low for k in ('random','rand_','rng','xorshift')):
            hits.append(i)
    merged=[]
    for i in hits:
        lo=max(0,i-6); hi=min(len(lines),i+18)
        if merged and lo<=merged[-1][1]+1:
            merged[-1]=(merged[-1][0],max(merged[-1][1],hi))
        else:
            merged.append((lo,hi))
    print(f'\n######## {label}: {p} ########')
    for lo,hi in merged:
        print(f'\n--- lines {lo+1}-{hi} ---')
        for j in range(lo,hi): print(f'{j+1:4}: {lines[j]}')

# Also dump complete functions whose names mention random/rng, even if helpers are compact.
for p in (Path('mello/mello_engine.c'),Path('mello/mello_ui.c')):
    lines=p.read_text().splitlines()
    sig=re.compile(r'^(?:static\s+)?[^;{}]+\s+([A-Za-z_][A-Za-z0-9_]*)\s*\([^;]*\)\s*\{')
    i=0
    while i<len(lines):
        m=sig.match(lines[i].strip())
        if not m or not any(k in m.group(1).lower() for k in ('random','rand','rng')):
            i+=1; continue
        name=m.group(1); start=i; depth=0; seen=False; j=i
        while j<len(lines):
            for ch in lines[j]:
                if ch=='{': depth+=1; seen=True
                elif ch=='}': depth-=1
            if seen and depth==0: break
            j+=1
        print(f'\n######## FUNCTION {name} {p}:{start+1}-{j+1} ########')
        print('\n'.join(lines[start:j+1]))
        i=j+1
print('=== END RANDOMIZER ROADMAP PROBE ===')
raise SystemExit('RANDOMIZER ROADMAP PROBE COMPLETE')
