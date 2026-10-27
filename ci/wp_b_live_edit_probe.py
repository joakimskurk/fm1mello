#!/usr/bin/env python3
from pathlib import Path
import re

FILES = {
    'ui_h': Path('mello/mello_ui.h'),
    'ui_c': Path('mello/mello_ui.c'),
    'engine_h': Path('mello/mello_engine.h'),
    'engine_c': Path('mello/mello_engine.c'),
    'audio_h': Path('mello/mello_audio.h'),
    'audio_c': Path('mello/mello_audio.c'),
    'transport_h': Path('mello/mello_transport.h'),
    'transport_c': Path('mello/mello_transport.c'),
}

def lines(path): return path.read_text().splitlines()

def show_context(label, src, hit_indexes, before=8, after=18, max_blocks=80):
    printed=[]; blocks=0
    for idx in hit_indexes:
        lo=max(0,idx-before); hi=min(len(src),idx+after+1)
        if printed and lo <= printed[-1][1]: continue
        print(f'\n=== {label} @{idx+1} ({lo+1}-{hi}) ===')
        for j in range(lo,hi): print(f'{j+1:4}: {src[j]}')
        printed.append((lo,hi)); blocks += 1
        if blocks >= max_blocks:
            print(f'\n=== {label}: truncated after {max_blocks} blocks ==='); break

print('=== WP-B LIVE EDIT SOURCE PROBE ===')
for name,path in FILES.items(): print(f'{name}: {path}, lines={len(lines(path))}')
ui_c=lines(FILES['ui_c']); engine_c=lines(FILES['engine_c']); audio_c=lines(FILES['audio_c']); transport_c=lines(FILES['transport_c'])
ui_h=lines(FILES['ui_h']); engine_h=lines(FILES['engine_h']); audio_h=lines(FILES['audio_h'])

ui_hits=[i for i,line in enumerate(ui_c) if re.search(r'(encoder|enc\[|mello_ui_input|set_config|_set_|storage\.dirty|MELLO_UI_)',line,re.I)]
show_context('UI rotary/input/setter context',ui_c,ui_hits,before=10,after=22,max_blocks=70)
eng_hits=[i for i,line in enumerate(engine_c) if re.search(r'(mello_engine_.*set|set_config|memcpy|memset|defaults\()',line,re.I)]
show_context('ENGINE setter/state context',engine_c,eng_hits,before=8,after=20,max_blocks=70)
aud_hits=[i for i,line in enumerate(audio_c) if re.search(r'(mello_audio_|set_config|set_.*config|render|memset|memcpy|malloc|calloc|free|reverb|delay|ring|predelay|queue|reset|init)',line,re.I)]
show_context('AUDIO config/render/state context',audio_c,aud_hits,before=8,after=24,max_blocks=90)
tr_hits=[i for i,line in enumerate(transport_c) if re.search(r'(mello_transport_|poll|running|event_sink|set_|start|stop|memset|engine\.)',line,re.I)]
show_context('TRANSPORT live-state context',transport_c,tr_hits,before=7,after=18,max_blocks=70)
for label,src in [('UI HEADER',ui_h),('ENGINE HEADER',engine_h),('AUDIO HEADER',audio_h)]:
    hits=[i for i,line in enumerate(src) if re.search(r'(typedef struct|encoder|mello_.*set|mello_.*render|MelloSound|MelloFx|MelloSub|MelloSample|MelloChord|MelloUi|MelloAudio)',line,re.I)]
    show_context(label,src,hits,before=4,after=12,max_blocks=50)
print('\n=== ALLOCATION AUDIT ===')
for name,src in [('ui',ui_c),('engine',engine_c),('audio',audio_c),('transport',transport_c)]:
    for i,line in enumerate(src):
        if re.search(r'\b(malloc|calloc|realloc|free)\s*\(',line): print(f'{name}:{i+1}: {line}')
print('\n=== END WP-B LIVE EDIT SOURCE PROBE ===')
