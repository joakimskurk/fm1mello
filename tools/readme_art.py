#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""The README's two pictures, from the firmware itself (python3 with Pillow, and cc):

  docs/logo.png   the hub card (docs/card.png, the splash at rest): its skyline and line as the FM-1 draws them, the
                  wordmark in a pixel bold, on a dark card with the HTML's green glow and rounded corners (960 x 400)
  docs/hero.gif   the host build (tests/host.c) playing HERO below: the splash, HOME, a roll, PERFORM (480 x 480)

  python3 tools/readme_art.py [--logo] [--hero]      (default: both)
Run it after the splash or the pages it films change (make screens first, for the card).
"""
import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

try:
    from PIL import Image, ImageChops, ImageDraw, ImageFilter
except ImportError:
    sys.exit("readme_art: needs Pillow (pip3 install Pillow)")

ROOT = Path(__file__).resolve().parent.parent
ACC = (0, 221, 136)                     # the app's green: UI_ACC, the HTML's accent, fmhub.json's colour
FPS = 12
# tests/host.c's script: PLAY; RNDMZR (it lands where the cycle comes round); PERF; HP SWP held, STUTTER, WASH
HERO = """2000 btn 12
4000 keys 0x400000
4080 keys 0
10600 keys 0x2000000
10680 keys 0
11300 keys 0x10000
12700 keys 0
13100 keys 0x2
14100 keys 0
14500 keys 0x80000
15500 keys 0
16200 end
"""


def logo(card, out, scale=3):
    im = Image.open(card).convert("RGB")
    sky = im.crop((24, 67, 216, 140))   # the bars (67..132) and the line under them (137)
    word = im.crop((24, 150, 216, 175))  # SKURKYMELLO (153..172)
    wm = Image.eval(word.convert("L"), lambda v: 255 if v > 40 else 0)
    wm = ImageChops.lighter(wm, ImageChops.offset(wm, 1, 0))   # bold: each dot also lights the one to its right
    up = lambda i: i.resize((i.width * scale, i.height * scale), Image.NEAREST)
    sky, wm = up(sky), up(wm)
    gap = 10 * scale
    w, h = 960, 400
    bg = Image.new("RGB", (w, h), (6, 10, 9))
    glow = Image.new("RGB", (w, h))
    ImageDraw.Draw(glow).ellipse((w * 0.22, 0, w * 0.78, h * 0.9), fill=(0, 30, 20))
    bg = ImageChops.add(bg, glow.filter(ImageFilter.GaussianBlur(110)))
    x = (w - sky.width) // 2
    top = (h - (sky.height + gap + wm.height)) // 2
    bg.paste(sky, (x, top), Image.eval(sky.convert("L"), lambda v: 255 if v > 8 else 0))
    wy = top + sky.height + gap
    halo = Image.new("RGB", (w, h))
    halo.paste(Image.new("RGB", wm.size, (0, 80, 50)), (x, wy), wm)
    bg = ImageChops.add(bg, halo.filter(ImageFilter.GaussianBlur(9)))
    bg.paste(Image.new("RGB", wm.size, ACC), (x, wy), wm)
    m = Image.new("L", (w * 4, h * 4), 0)   # rounded corners, so the card sits on light and dark pages alike
    ImageDraw.Draw(m).rounded_rectangle((0, 0, w * 4 - 1, h * 4 - 1), 28 * 4, fill=255)
    out_im = bg.convert("RGBA")
    out_im.putalpha(m.resize((w, h), Image.LANCZOS))
    out_im.save(out, optimize=True)
    print(f"logo -> {out.relative_to(ROOT)} ({out.stat().st_size // 1024} KB)")


def hero(out):
    with tempfile.TemporaryDirectory() as tmp:
        t = Path(tmp)
        host = t / "host"
        subprocess.run(["cc", "-O2", "-Wall", "-Wno-unused-function", "-DDINGHY_HOST", '-DAPP_VER="0.5"',
                        "-Ifirmware/src", "-Ifirmware/gen", "-o", str(host), "firmware/src/dinghy.c"],
                       cwd=ROOT, check=True)
        (t / "script.txt").write_text(HERO)
        subprocess.run([str(host), str(t / "f"), str(t / "script.txt"), str(FPS)], cwd=t, check=True,
                       stdout=subprocess.DEVNULL)
        raw = [Image.open(p).convert("RGB") for p in sorted(t.glob("f-*.ppm"))]
    if not raw:
        sys.exit("readme_art: the host run wrote no frames")
    mosaic = Image.new("RGB", (240 * 4, 240 * 4))   # one palette for the whole film, from sixteen of its frames
    for k, im in enumerate(raw[::max(1, len(raw) // 16)][:16]):
        mosaic.paste(im, ((k % 4) * 240, (k // 4) * 240))
    pal = mosaic.quantize(colors=255, method=Image.MEDIANCUT)
    frames = [im.resize((480, 480), Image.NEAREST).quantize(palette=pal, dither=Image.Dither.NONE) for im in raw]
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=1000 // FPS, loop=0)
    print(f"hero -> {out.relative_to(ROOT)} ({len(raw)} frames, {out.stat().st_size // 1024} KB)")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--logo", action="store_true", help="docs/logo.png only")
    ap.add_argument("--hero", action="store_true", help="docs/hero.gif only")
    a = ap.parse_args()
    both = not (a.logo or a.hero)
    if a.logo or both:
        logo(ROOT / "docs" / "card.png", ROOT / "docs" / "logo.png")
    if a.hero or both:
        hero(ROOT / "docs" / "hero.gif")
    return 0


if __name__ == "__main__":
    sys.exit(main())
