#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""The site's link preview (Open Graph, web/og.png, 1200 x 630): the README's logo (docs/logo.png, make art) over
three of the firmware's screens at their own size (docs/screens, make screens), on the page's night with its green
glow. web/make_site.py puts it in the site; web/index.html names it (og:image). Needs Pillow.

  python3 web/make_og.py [OUT.png]
"""
import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
W, H = 1200, 630
SCREENS = ("home-playing", "perform", "dot-playing")
TOP, BOTTOM, LINE = (4, 18, 11), (2, 8, 6), (31, 74, 52)


def main(out):
    img = Image.new("RGB", (W, H))
    px = img.load()
    for y in range(H):                                    # the page's night: dark green to near black
        t = y / (H - 1)
        c = tuple(round(TOP[i] + (BOTTOM[i] - TOP[i]) * t) for i in range(3))
        for x in range(W):
            px[x, y] = c
    glow = Image.new("RGB", (W, H))
    ImageDraw.Draw(glow).ellipse((W * .18, -H * .25, W * .82, H * .55), fill=(0, 46, 28))
    img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(120)))

    logo = Image.open(ROOT / "docs/logo.png").convert("RGBA")
    logo = logo.resize((600, 250), Image.LANCZOS)
    shots = [Image.open(ROOT / f"docs/screens/{s}.png").convert("RGB").resize((240, 240), Image.NEAREST)
             for s in SCREENS]
    gap = 22
    row_w = 3 * 240 + 2 * gap
    y0 = (H - (logo.height + 18 + 240)) // 2
    img.paste(logo, ((W - logo.width) // 2, y0), logo)
    d = ImageDraw.Draw(img)
    x, y = (W - row_w) // 2, y0 + logo.height + 18
    for s in shots:
        d.rectangle((x - 2, y - 2, x + 241, y + 241), outline=LINE, width=2)
        img.paste(s, (x, y))
        x += 240 + gap
    img.save(out, optimize=True)
    print(f"og: {out} ({W} x {H}, {Path(out).stat().st_size} B)")


if __name__ == "__main__":
    a = sys.argv[1:]
    if len(a) > 1 or any(x.startswith("-") for x in a):
        sys.exit(__doc__)
    main(a[0] if a else ROOT / "web/og.png")
