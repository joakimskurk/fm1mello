#!/usr/bin/env python3
from pathlib import Path
import re

p = Path('mello/mello_ui.c')
s = p.read_text()
lines = s.splitlines()

print('=== UI ROADMAP PROBE ===')
print(f'bytes={len(s)} lines={len(lines)}')

sig = re.compile(r'^(?:static\s+)?(?:void|bool|int|uint\w+_t|Mello\w+)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\([^;]*\)\s*\{')
functions = []
i = 0
while i < len(lines):
    m = sig.match(lines[i].strip())
    if not m:
        i += 1
        continue
    name = m.group(1)
    start = i
    depth = 0
    seen = False
    j = i
    while j < len(lines):
        for ch in lines[j]:
            if ch == '{':
                depth += 1; seen = True
            elif ch == '}':
                depth -= 1
        if seen and depth == 0:
            break
        j += 1
    functions.append((name,start,j))
    i = j + 1

for name,a,b in functions:
    low = name.lower()
    body = '\n'.join(lines[a:b+1])
    if any(k in low for k in ('home','seq','random','step')) or any(k in body for k in ('MELLO_PAGE_HOME','MELLO_PAGE_SEQ','MELLO_PAGE_RANDOM')):
        print(f'\n--- FUNCTION {name} lines {a+1}-{b+1} ---')
        print(body)

print('\n=== TOKEN CONTEXTS ===')
tokens = ['project.length','play_step','selected_step','MELLO_PAGE_HOME','MELLO_PAGE_SEQ','MELLO_PAGE_RANDOM','240,240','240, 240','draw_step','overview']
shown=set()
for idx,line in enumerate(lines):
    if any(t in line for t in tokens):
        lo=max(0,idx-3); hi=min(len(lines),idx+4)
        key=(lo,hi)
        if key in shown: continue
        shown.add(key)
        print(f'\n@@ {idx+1} @@')
        for k in range(lo,hi): print(f'{k+1:4}: {lines[k]}')
print('=== END UI ROADMAP PROBE ===')
