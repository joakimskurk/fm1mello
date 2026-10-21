#!/usr/bin/env python3
import argparse,hashlib,json,re
from pathlib import Path
ap=argparse.ArgumentParser(); ap.add_argument('--log',type=Path,required=True); ap.add_argument('--package',type=Path,required=True); ap.add_argument('--product',required=True); ap.add_argument('--out',type=Path,required=True); a=ap.parse_args(); text=a.log.read_text(errors='replace')
m=re.search(r'image (\d+) B of (\d+); RAM \.data\+\.bss (\d+) B of (\d+); pool (\d+) B of (\d+)',text)
if not m: raise SystemExit('FAIL: Dinghy resource summary missing from build log')
image,image_max,ram,ram_max,pool,pool_max=map(int,m.groups())
if not a.package.is_file(): raise SystemExit('FAIL: package missing')
data=a.package.read_bytes()
if a.product.encode() not in data: raise SystemExit('FAIL: expected product identity not present in package bytes')
sha=hashlib.sha256(data).hexdigest(); obj={'image':image,'image_max':image_max,'ram':ram,'ram_max':ram_max,'pool':pool,'pool_max':pool_max,'ram_headroom':ram_max-ram,'pool_headroom':pool_max-pool,'package_bytes':len(data),'package_sha256':sha,'product':a.product}
if ram>ram_max or pool>pool_max or image>image_max: raise SystemExit('FAIL: target resource budget exceeded')
a.out.parent.mkdir(parents=True,exist_ok=True); a.out.write_text(json.dumps(obj,indent=2,sort_keys=True)+'\n'); print(json.dumps(obj,sort_keys=True))
