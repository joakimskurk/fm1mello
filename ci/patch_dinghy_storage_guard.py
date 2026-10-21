#!/usr/bin/env python3
from pathlib import Path
import sys
if len(sys.argv) != 2: raise SystemExit("usage: patch_dinghy_storage_guard.py /path/to/platform.c")
p=Path(sys.argv[1]); s=p.read_text()
old='#define FL_RANGE_OK(off, n) (FL_IN(off, n, FL_OTA_LO, FL_OTA_HI) || FL_IN(off, n, SETTINGS_AT, SETTINGS_AT + 0x1000u))'
new='''#define MELLO_STORAGE_LO 0x00098000u\n#define MELLO_STORAGE_HI 0x000E0000u\n#define FL_RANGE_OK(off, n) (FL_IN(off, n, FL_OTA_LO, FL_OTA_HI) || \\\n                             FL_IN(off, n, SETTINGS_AT, SETTINGS_AT + 0x1000u) || \\\n                             FL_IN(off, n, MELLO_STORAGE_LO, MELLO_STORAGE_HI))'''
if old not in s: raise SystemExit("refusing to patch: expected Dinghy 0.3 FL_RANGE_OK line not found")
if 'MELLO_STORAGE_LO' in s: raise SystemExit("refusing to patch: Mello storage guard already present")
p.write_text(s.replace(old,new,1)); print(f"patched {p}: write guard +0x98000..0xDFFFF only")
