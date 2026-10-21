#!/usr/bin/env python3
from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: patch_freestanding_libc.py /path/to/firmware/src")
root = Path(sys.argv[1])
files = [
    "mello_engine.c",
    "mello_transport.c",
    "mello_storage.c",
    "mello_ui.c",
    "mello_audio.c",
    "mello_sample.c",
]
for name in files:
    p = root / name
    s = p.read_text()
    needle = "#include <string.h>\n"
    if needle not in s:
        raise SystemExit(f"refusing to patch {name}: string.h include missing")
    p.write_text(s.replace(needle, "", 1))

p = root / "mello_ui.c"
s = p.read_text()
old1 = "if (idx < 0) idx = (int32_t)strlen(name_chars) - 1;"
old2 = "if (idx >= (int32_t)strlen(name_chars)) idx = 0;"
new1 = "if (idx < 0) idx = (int32_t)(sizeof(name_chars) - 1u) - 1;"
new2 = "if (idx >= (int32_t)(sizeof(name_chars) - 1u)) idx = 0;"
if old1 not in s or old2 not in s:
    raise SystemExit("refusing to patch mello_ui.c: expected strlen expressions missing")
s = s.replace(old1, new1, 1).replace(old2, new2, 1)
p.write_text(s)

for name in files:
    s = (root / name).read_text()
    if "#include <string.h>" in s:
        raise SystemExit(f"postcondition failed: string.h remains in {name}")
if "strlen(" in (root / "mello_ui.c").read_text():
    raise SystemExit("postcondition failed: strlen remains in mello_ui.c")
print(f"patched freestanding libc compatibility: {root}")
