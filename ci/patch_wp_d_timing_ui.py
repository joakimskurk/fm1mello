#!/usr/bin/env python3
from pathlib import Path
import hashlib

UI = Path('mello/mello_ui.c')
PRE_UI_SHA = 'f6d26e0d1a57a51b08bf65e878144f5d48bfa248244d97b80c7a0842eb601818'
POST_UI_SHA = '08bd19d7a2f112de318d50516c5a27b3ed2324d0c82bd50a087caadd0b7ecd55'


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replace_one(src: str, old: str, new: str, label: str) -> str:
    count = src.count(old)
    if count != 1:
        raise SystemExit(f'FAIL: WP-D D3 {label} marker count {count}')
    return src.replace(old, new, 1)


if sha(UI) != PRE_UI_SHA:
    raise SystemExit(f'FAIL: WP-D D3 UI base hash {sha(UI)} != {PRE_UI_SHA}')

s = UI.read_text()
value_i = '''static void value_i(char *b, const char *label, int32_t v)\n{\n    char *p = copy_s(b, label);\n    if (*label) *p++ = ' ';\n    put_i32(p, v);\n}\n'''
formatter = value_i + '''\nstatic void main_reset_name(char *b, uint16_t master_len)\n{\n    char *p;\n    if (master_len == 0u) { copy_s(b, "FREE"); return; }\n    p = put_u32(b, (uint32_t)master_len / 16u);\n    copy_s(p, " BAR");\n}\n'''
s = replace_one(s, value_i, formatter, 'main reset formatter')
s = replace_one(
    s,
    'l[1]="RESET";value_u(v[1],"",e->project.master_len);',
    'l[1]="RESET";main_reset_name(v[1],e->project.master_len);',
    'SEQ footer timing language',
)
UI.write_text(s)

if sha(UI) != POST_UI_SHA:
    raise SystemExit(f'FAIL: WP-D D3 UI output hash {sha(UI)} != {POST_UI_SHA}')

print(f'PASS: WP-D D3 timing language applied; ui={sha(UI)}')
