#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""SkurkyMello's site (GitHub Pages, https://joakimskurk.github.io/fm1mello/): its page, its installer in the
browser, the firmware, the take.

  index.html                       web/index.html with fm1pkg.js and fm1ota.js (SLOOP's, unchanged) and the metadata
                                   inlined: the installer (Web MIDI, Chrome or Edge), the take, the pages, the links
  firmware/skurkymello-VER.fwsc    the package the page installs (its SHA-256 in the page: a damaged download is
                                   never offered)
  audio/                           the take (web/take.c: one performance on the firmware's own UI, engine and audio
                                   code, rendered on this computer), AAC, and its spectrum as a picture
  screens/                         the pictures (docs/logo.png, docs/hero.gif, docs/screens, web/og.png)
  .nojekyll                        (served as it is)

  python3 web/make_site.py build/skurkymello-dev.fwsc VERSION OUT_DIR

The package must be SkurkyMello's (its identity FM-1_987) with its update loader in it, which the installer talks
to. The original browser instrument is not copied here: the page links to it where it lives, on jhf.no. Needs
Pillow, numpy, cc, and ffmpeg (or afconvert on a Mac). After VaveHero's and Star Distroyer's web/make_site.py,
themselves after SLOOP's (Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments; isod89).
"""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F
PRODUCT = "FM-1_987"  # docs/mello/PRODUCT_IDENTITY.md
PICTURES = {ROOT / "docs/logo.png": "logo.png", ROOT / "docs/hero.gif": "hero.gif", HERE / "og.png": "og.png",
            ROOT / "docs/buymeacoffee.png": "coffee.png"}
SCREENS = ("home", "home-playing", "home-rolled", "rndmzr-held", "home-held-step", "home-rec", "home-macros",
           "step-playing", "segment", "loop-loop", "sound-filter", "sound-osc-2", "bass-seq", "chord-chord",
           "chord-harmony", "drums-voice", "dot-playing", "evolve-main", "fx-delay-send", "fx-reverb-room",
           "random-mask", "key-scale", "mix", "perform", "sample-main", "global-clock")

# ---- the take: web/take.c, built with the firmware's own code; its log gives the page's chapters
TAKE = {"id": "performance", "title": "PERFORMANCE",
        "about": "A fresh project on GROOVE at 120 BPM. RNDMZR is pressed a bar before each change, so the whole band "
                 "rolls into a new style every four bars, and a PERFORM key is held the last four or eight beats into "
                 "it. EVOLVE moves the gate, the filter and the decay, set anew each section, and the lead's knobs "
                 "are turned by hand."}
# the spectrum's picture: 40 Hz .. 16 kHz on a log scale, 72 dB of range, in the screen's greens
SPEC_W, SPEC_H, SPEC_N = 1600, 200, 8192
SPEC_LO, SPEC_HI, SPEC_DB = 40.0, 16000.0, 72.0
GREEN = [(0.00, (3, 10, 7)), (0.30, (6, 34, 22)), (0.55, (0, 110, 68)), (0.78, (0, 221, 136)),
         (0.92, (140, 255, 205)), (1.00, (255, 255, 255))]


def product_of(raw):
    """the package identity: one marker byte after each of the first 20 blocks (fm1pkg.js productOf)"""
    return "".join(chr((m - i - 1) & 0xFF) for i in range(BLOCKS) if (m := raw[i * BLK + KEEP]) != 0x7D)


def strip_module(src):
    src = re.sub(r"^export\s+", "", src, flags=re.M)
    return re.sub(r"^import .*?;\n", "", src, flags=re.M)


def take_build(tmp):
    exe = tmp / "take"
    subprocess.run(["cc", "-std=c11", "-O2", "-Wall", "-Ifirmware/src", "web/take.c",
                    *sorted(str(s.relative_to(ROOT)) for s in (ROOT / "firmware/src").glob("mello_*.c")), "-lm",
                    "-o", str(exe)], cwd=ROOT, check=True)
    return exe


def chapters(log):
    """the take's sections from its log: each style from when it plays, with the PERFORM keys held into the next"""
    out = []
    for line in log.splitlines():
        m = re.match(r"\s*([0-9.]+)\s+(style|hold|mute)\s+(.*?)(?: \([0-9.]+ beats\))?$", line)
        if not m:
            continue
        sec, kind, what = float(m.group(1)), m.group(2), m.group(3)
        if kind == "style":
            out.append({"t": 0.0 if not out else sec, "style": what, "keys": []})
        elif out and what not in out[-1]["keys"]:
            out[-1]["keys"].append(what)
    if len(out) < 2:
        raise SystemExit("make_site: the take's log names fewer than two styles")
    return out


def read_wav(path):
    with wave.open(str(path)) as f:
        if f.getsampwidth() != 2:
            raise SystemExit(f"{path}: not 16-bit")
        ch, fs = f.getnchannels(), f.getframerate()
        x = np.frombuffer(f.readframes(f.getnframes()), dtype="<i2").astype(np.float32)
    return x.reshape(-1, ch).mean(axis=1) / 32768.0, fs


def spectrum(x, fs, pic):
    """the piece's spectrum, left to right: a column a slice, the louder the brighter"""
    win = np.hanning(SPEC_N).astype(np.float32)
    x = np.concatenate([x, np.zeros(SPEC_N, np.float32)])
    hop = (len(x) - 2 * SPEC_N) / (SPEC_W - 1)
    freqs = np.fft.rfftfreq(SPEC_N, 1.0 / fs)
    edges = np.geomspace(SPEC_LO, SPEC_HI, SPEC_H + 1)
    cut = np.append(np.searchsorted(freqs, edges[:-1]), np.searchsorted(freqs, edges[-1]))
    db = np.empty((SPEC_H, SPEC_W), np.float32)
    for c in range(SPEC_W):
        s = int(c * hop)
        m = np.abs(np.fft.rfft(x[s:s + SPEC_N] * win))
        db[:, c] = 20.0 * np.log10(np.maximum.reduceat(m, cut)[:-1] + 1e-9)   # each row: its bins' loudest
    ref = np.percentile(db, 99.8)
    t = np.clip((db - (ref - SPEC_DB)) / SPEC_DB, 0.0, 1.0)[::-1] ** 1.3   # high at the top; the floor darker
    rgb = np.zeros(t.shape + (3,), np.float32)
    for (t0, c0), (t1, c1) in zip(GREEN, GREEN[1:]):
        k = ((t >= t0) & (t <= t1))[..., None]
        f = ((t - t0) / (t1 - t0))[..., None]
        rgb = np.where(k, np.array(c0) + f * (np.array(c1) - np.array(c0)), rgb)
    Image.fromarray(rgb.astype(np.uint8), "RGB").save(pic, quality=84, method=6)   # (WebP: a fifth of a PNG)


def encode(wav, m4a):
    if shutil.which("ffmpeg"):
        cmd = ["ffmpeg", "-loglevel", "error", "-y", "-i", str(wav), "-c:a", "aac", "-b:a", "160k", "-movflags",
               "+faststart", str(m4a)]
    elif shutil.which("afconvert"):
        cmd = ["afconvert", "-f", "m4af", "-d", "aac", "-b", "160000", str(wav), str(m4a)]
    else:
        raise SystemExit("make_site: no ffmpeg or afconvert for the audio")
    subprocess.run(cmd, check=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("pkg")
    ap.add_argument("version")
    ap.add_argument("out")
    a = ap.parse_args()
    pkg, out = Path(a.pkg), Path(a.out)
    raw = pkg.read_bytes()
    product = product_of(raw)
    if product != PRODUCT:
        raise SystemExit(f"{pkg}: identity {product!r} is not SkurkyMello's ({PRODUCT})")
    if b"FELUCCA-LOADER-1" not in raw:
        raise SystemExit(f"{pkg}: no update loader in it: the installer could not write it")
    name = f"skurkymello-{re.sub(r'[^A-Za-z0-9.-]', '-', a.version)}.fwsc"
    if out.exists():
        shutil.rmtree(out)
    for sub in ("firmware", "screens", "audio"):
        (out / sub).mkdir(parents=True)
    shutil.copy(pkg, out / "firmware" / name)
    for src, dst in PICTURES.items():
        shutil.copy(src, out / "screens" / dst)
    for s in SCREENS:
        shutil.copy(ROOT / "docs" / "screens" / f"{s}.png", out / "screens" / f"{s}.png")
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        exe = take_build(tmp)
        wav = tmp / "take.wav"
        log = subprocess.run([str(exe), str(wav)], check=True, capture_output=True, text=True).stdout
        x, fs = read_wav(wav)
        spectrum(x, fs, out / "audio" / f"{TAKE['id']}.webp")
        encode(wav, out / "audio" / f"{TAKE['id']}.m4a")
    take = dict(TAKE, src=f"audio/{TAKE['id']}.m4a", spectrum=f"audio/{TAKE['id']}.webp", secs=round(len(x) / fs, 1),
                chapters=chapters(log))
    print(f"  take: {take['secs']} s, peak {20 * np.log10(float(np.abs(x).max()) + 1e-9):.1f} dBFS, "
          + ", ".join(c["style"] for c in take["chapters"]))
    meta = json.dumps({"version": a.version, "product": product, "pkg": "firmware/" + name,
                       "sha256": hashlib.sha256(raw).hexdigest(), "size": len(raw)})
    lib = strip_module((HERE / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((HERE / "fm1ota.js").read_text(encoding="utf-8"))
    html = (HERE / "index.html").read_text(encoding="utf-8")
    for mark in ("/*LIB*/", "/*META*/", "/*TAKE*/"):
        if html.count(mark) != 1:
            raise SystemExit(f"web/index.html must hold {mark} once")
    for s in re.findall(r'src="screens/([^"]+)"', html):
        if not (out / "screens" / s).exists():
            raise SystemExit(f"web/index.html shows screens/{s}, which the site does not have (SCREENS, PICTURES)")
    html = html.replace("/*LIB*/", lib).replace("/*META*/", meta).replace("/*TAKE*/", json.dumps(take))
    (out / "index.html").write_text(html, encoding="utf-8")
    (out / ".nojekyll").write_text("")
    size = sum(p.stat().st_size for p in out.rglob("*") if p.is_file())
    print(f"site: {out}: index.html ({len(html)} B), firmware/{name} ({len(raw)} B, {product}), 1 take, "
          f"{len(PICTURES) + len(SCREENS)} pictures, {size // 1024} KB in all")


if __name__ == "__main__":
    sys.exit(main())
