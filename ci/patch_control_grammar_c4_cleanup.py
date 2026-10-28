#!/usr/bin/env python3
from pathlib import Path
import hashlib

UI = Path('mello/mello_ui.c')
PRE_SHA = '5cc857d3c84db34ed52b0c1854a5b4b944a6f5786b4197d7e6ce04ba1364290d'


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

actual = sha(UI)
if actual != PRE_SHA:
    raise SystemExit(f'FAIL: WP-C4 cleanup base hash {actual} != {PRE_SHA}')

s = UI.read_text()
marker = 'static const char *function_name(const MelloUi *ui)\n'
if s.count(marker) != 1:
    raise SystemExit(f'FAIL: WP-C4 function_name marker count {s.count(marker)}')
start = s.index(marker)
brace = s.find('{', start)
if brace < 0:
    raise SystemExit('FAIL: WP-C4 function_name opening brace missing')
depth = 0
end = -1
for i in range(brace, len(s)):
    if s[i] == '{': depth += 1
    elif s[i] == '}':
        depth -= 1
        if depth == 0:
            end = i + 1
            break
if end < 0:
    raise SystemExit('FAIL: WP-C4 function_name closing brace missing')
while end < len(s) and s[end] == '\n':
    end += 1
s = s[:start] + s[end:]
if 'function_name(' in s:
    raise SystemExit('FAIL: WP-C4 function_name reference remains')
UI.write_text(s)
print(f'PASS: WP-C4 obsolete function-name helper removed; UI_SHA={sha(UI)}')
