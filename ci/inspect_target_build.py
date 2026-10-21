#!/usr/bin/env python3
import argparse
import hashlib
import json
import re
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument('--log', type=Path, required=True)
ap.add_argument('--package', type=Path, required=True)
ap.add_argument('--product', required=True)
ap.add_argument('--out', type=Path, required=True)
a = ap.parse_args()
text = a.log.read_text(errors='replace')

m = re.search(r'image (\d+) B of (\d+); RAM \.data\+\.bss (\d+) B of (\d+); pool (\d+) B of (\d+)', text)
if not m:
    raise SystemExit('FAIL: Dinghy resource summary missing from build log')
image, image_max, ram, ram_max, pool, pool_max = map(int, m.groups())
if not a.package.is_file():
    raise SystemExit('FAIL: package missing')
data = a.package.read_bytes()

# Dinghy fm1pkg_make.ufw(): the first 20 logical 0x2F-byte blocks each get
# one marker byte. Marker 0x7D means end/padding; otherwise marker =
# ord(product[i]) + i + 1 (mod 256). Decode that exact representation.
if len(data) < 20 * 0x30:
    raise SystemExit('FAIL: package too short for FM-1 identity markers')
chars = []
for i in range(20):
    marker = data[i * 0x30 + 0x2F]
    if marker == 0x7D:
        break
    chars.append(chr((marker - i - 1) & 0xFF))
decoded_product = ''.join(chars)
if decoded_product != a.product:
    raise SystemExit(f'FAIL: decoded package identity {decoded_product!r} != expected {a.product!r}')

# Independently require the Dinghy packer to have reported the same identity.
reported = re.findall(r'package\s+.*?\.fwsc\s+\d+ B, identity\s+(FM-1_\d+)', text)
if not reported or reported[-1] != a.product:
    raise SystemExit(f'FAIL: build log identity mismatch: {reported[-1] if reported else "missing"}')

sha = hashlib.sha256(data).hexdigest()
obj = {
    'image': image,
    'image_max': image_max,
    'ram': ram,
    'ram_max': ram_max,
    'pool': pool,
    'pool_max': pool_max,
    'ram_headroom': ram_max - ram,
    'pool_headroom': pool_max - pool,
    'package_bytes': len(data),
    'package_sha256': sha,
    'product': a.product,
    'decoded_product': decoded_product,
}
if ram > ram_max or pool > pool_max or image > image_max:
    raise SystemExit('FAIL: target resource budget exceeded')
a.out.parent.mkdir(parents=True, exist_ok=True)
a.out.write_text(json.dumps(obj, indent=2, sort_keys=True) + '\n')
print(json.dumps(obj, sort_keys=True))
