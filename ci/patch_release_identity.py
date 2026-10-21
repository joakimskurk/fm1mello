#!/usr/bin/env python3
import argparse,re
from pathlib import Path
ap=argparse.ArgumentParser(); ap.add_argument('build_py',type=Path); ap.add_argument('--package-stem',default='skurkymello'); a=ap.parse_args(); p=a.build_py; s=p.read_text(); orig=s
need=['PRODUCT = "FM-1_990"','PRODUCT = "FM-1_99" + m[2]','name = "dinghy.fwsc"','name = f"dinghy-{a.release}.fwsc"']
missing=[x for x in need if x not in s]
if missing: raise SystemExit('refusing unknown Dinghy build.py; missing: '+', '.join(missing))
s=s.replace('PRODUCT = "FM-1_990" # Dinghy: 99N (OUR_LOADER in the installer wants FM-1_9NN). Your fork: pick your own','PRODUCT = os.environ.get("MELLO_PRODUCT", "") # SkurkyMello: required explicit FM-1_9NN identity')
s=s.replace('name = "dinghy.fwsc"',f'name = "{a.package_stem}-dev.fwsc"')
s=s.replace('PRODUCT = "FM-1_99" + m[2]','''if not re.fullmatch(r"FM-1_9\\d\\d", PRODUCT):\n            raise SystemExit("MELLO_PRODUCT must be a unique FM-1_9NN identity (never Dinghy FM-1_99N)")\n        if PRODUCT.startswith("FM-1_99"):\n            raise SystemExit("MELLO_PRODUCT must not use Dinghy's FM-1_99N range")''')
s=s.replace('name = f"dinghy-{a.release}.fwsc"',f'name = f"{a.package_stem}-{{a.release}}.fwsc"')
needle='fm1pkg_make.SDK = a.sdk'; repl='''if not re.fullmatch(r"FM-1_9\\d\\d", PRODUCT):\n        raise SystemExit("MELLO_PRODUCT must be set to a unique FM-1_9NN identity")\n    if PRODUCT.startswith("FM-1_99"):\n        raise SystemExit("MELLO_PRODUCT must not use Dinghy's FM-1_99N range")\n    fm1pkg_make.SDK = a.sdk'''
if needle not in s: raise SystemExit('refusing unknown Dinghy build.py: SDK assignment not found')
s=s.replace(needle,repl,1)
if s==orig: raise SystemExit('no changes made')
p.write_text(s); print(f'patched release identity/package naming: {p}')
