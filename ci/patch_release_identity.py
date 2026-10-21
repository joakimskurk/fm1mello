#!/usr/bin/env python3
import argparse
import re
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("build_py", type=Path)
ap.add_argument("--package-stem", default="skurkymello")
a = ap.parse_args()
p = a.build_py
s = p.read_text()
orig = s

need = [
    'PRODUCT = "FM-1_990"',
    'PRODUCT = "FM-1_99" + m[2]',
    'name = "dinghy.fwsc"',
    'name = f"dinghy-{a.release}.fwsc"',
]
missing = [x for x in need if x not in s]
if missing:
    raise SystemExit("refusing unknown Dinghy build.py; missing: " + ", ".join(missing))

# Do not depend on upstream's exact comment spacing. Replace exactly one global
# PRODUCT assignment and prove afterwards that Dinghy's default identity is gone.
s, n = re.subn(
    r'^PRODUCT = "FM-1_990".*$',
    'PRODUCT = os.environ.get("MELLO_PRODUCT", "")  # required explicit SkurkyMello FM-1_9NN identity',
    s,
    count=1,
    flags=re.M,
)
if n != 1:
    raise SystemExit(f"refusing to patch PRODUCT declaration: replaced {n} lines")

s = s.replace('name = "dinghy.fwsc"', f'name = "{a.package_stem}-dev.fwsc"', 1)

old_release_product = 'PRODUCT = "FM-1_99" + m[2]'
new_release_product = '''if not re.fullmatch(r"FM-1_9\\d\\d", PRODUCT):
            raise SystemExit("MELLO_PRODUCT must be a unique FM-1_9NN identity")
        if PRODUCT.startswith("FM-1_99"):
            raise SystemExit("MELLO_PRODUCT must not use Dinghy's FM-1_99N range")'''
s = s.replace(old_release_product, new_release_product, 1)
s = s.replace('name = f"dinghy-{a.release}.fwsc"', f'name = f"{a.package_stem}-{{a.release}}.fwsc"', 1)

needle = 'fm1pkg_make.SDK = a.sdk'
repl = '''if not re.fullmatch(r"FM-1_9\\d\\d", PRODUCT):
        raise SystemExit("MELLO_PRODUCT must be set to a unique FM-1_9NN identity")
    if PRODUCT.startswith("FM-1_99"):
        raise SystemExit("MELLO_PRODUCT must not use Dinghy's FM-1_99N range")
    fm1pkg_make.SDK = a.sdk'''
if needle not in s:
    raise SystemExit("refusing unknown Dinghy build.py: SDK assignment not found")
s = s.replace(needle, repl, 1)

if s == orig:
    raise SystemExit("no changes made")
if 'PRODUCT = "FM-1_990"' in s:
    raise SystemExit("postcondition failed: Dinghy FM-1_990 identity remains")
if 'PRODUCT = os.environ.get("MELLO_PRODUCT", "")' not in s:
    raise SystemExit("postcondition failed: MELLO_PRODUCT declaration missing")
if old_release_product in s:
    raise SystemExit("postcondition failed: Dinghy release identity override remains")

p.write_text(s)
print(f"patched release identity/package naming: {p}")
