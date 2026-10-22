#!/usr/bin/env python3
from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: patch_target_optimization.py PATH/TO/tools/build.py")

p = Path(sys.argv[1])
s = p.read_text()
old = 'CFLAGS = ["-Os", "-ffunction-sections", "-fno-builtin", "-Wall", "-Wno-unused-function"]'
new = 'CFLAGS = ["-O2", "-ffunction-sections", "-fno-builtin", "-Wall", "-Wno-unused-function"]'
if old not in s:
    raise SystemExit("FAIL: expected Dinghy CFLAGS line not found")
s = s.replace(old, new, 1)
if 'CFLAGS = ["-Os"' in s:
    raise SystemExit("FAIL: -Os still active after target optimization patch")
if 'CFLAGS = ["-O2"' not in s:
    raise SystemExit("FAIL: -O2 post-condition missing")
p.write_text(s)
print("patched Mello target optimization: -Os -> -O2")
