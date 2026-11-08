#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""SkurkyMello's site (GitHub Pages, https://joakimskurk.github.io/fm1mello/): its page, its installer in the
browser, the firmware, the demos.

  index.html                       web/index.html with fm1pkg.js and fm1ota.js (SLOOP's, unchanged) and the metadata
                                   inlined: the installer (Web MIDI, Chrome or Edge), the demos, the pages, the links
  firmware/skurkymello-VER.fwsc    the package the page installs (its SHA-256 in the page: a damaged download is
                                   never offered)
  audio/                           the demos below: the firmware itself playing on this computer (tests/host.c, the
                                   same C the FM-1 runs, its USB audio), AAC, and each one's spectrum as a picture
  screens/                         the pictures (docs/logo.png, docs/hero.gif, docs/screens, web/og.png)
  .nojekyll                        (served as it is)

  python3 web/make_site.py build/skurkymello-dev.fwsc VERSION OUT_DIR [--look DIR]

--look DIR also writes the screen at each step of the demos as PNG, to see that they play what they say.
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

# ---- the demos: the host build (tests/host.c) on a script of the panel's inputs, as one person plays one FM-1
OCT_DOWN, OCT_UP, FX, SEL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY, REC = range(14)   # tools/screens.py's
SELECT, ALGORITHM, K1, K2, K3, K4, PRESETS = range(7)                                       # MelloUiEncoder
# the 27 keys from F3 as the UI counts them: white 1..16 and black B0..B10 (B9 RNDMZR, B10 PERF)
WHITE = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26]
BLACK = [1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25]
RNDMZR, PERF = 1 << BLACK[9], 1 << BLACK[10]
# PERFORM: the white keys a row each (FILL ARP REVERSE ACCENT, REP 8 4 2 1, LP SWP HP SWP THROW WASH, the mutes),
# the black keys B0-B7 STUTTER, TAPE STOP, PLUCK, CRUSH, CHOP, OCT DOWN, OCT UP, WARP
PF = dict(zip(["FILL", "ARP", "REVERSE", "ACCENT", "REP 8", "REP 4", "REP 2", "REP 1", "LP SWP", "HP SWP", "THROW",
               "WASH", "LD OFF", "BS OFF", "CH OFF", "DR OFF"], (1 << b for b in WHITE)))
PF.update(zip(["STUTTER", "TAPE STOP", "PLUCK", "CRUSH", "CHOP", "OCT DOWN", "OCT UP", "WARP"],
              (1 << b for b in BLACK[:8])))
STYLES = ["ANY", "AMBIENT", "DUB", "TRIPHOP", "TECHNO", "ACID", "ELECTRO", "BREAKS", "IDM", "HOUSE", "DEEP",
          "MINIMAL", "JUNGLE", "DRONE", "OUTRUN", "GLITCH"]   # STYLE on the RANDOM page (K2), in its order
INTENSITY = ["LOW", "MED", "HIGH", "MUSICAL", "GROOVE"]        # ALGORITHM on HOME; a new project is on MED


class Take:
    """One demo's script for tests/host.c: what is pressed and turned, and when (ms)."""

    def __init__(self, bpm=80):
        self.lines, self.looks, self.t = [], [], 2000   # the splash is up for 1.5 s
        self.style, self.intensity, self.t0, self.bpm = 0, 1, None, 80
        if bpm != 80:
            self.ev(f"enc {SELECT} {bpm - 80}")         # SELECT on HOME is the tempo, a BPM a detent
            self.bpm = bpm

    def ev(self, what, gap=80):
        self.lines.append(f"{round(self.t)} {what}")
        self.t += gap

    def look(self, why):
        self.lines.append(f"{round(self.t) + 300} shot")
        self.looks.append((round(self.t) + 300, why))

    def bar(self, n):
        """the time of bar n's one (counted from PLAY, from 0)"""
        return self.t0 + n * 240000.0 / self.bpm

    def at_bar(self, n, beats=0.0):
        self.t = self.bar(n) + beats * 60000.0 / self.bpm

    def play(self):
        self.ev(f"btn {PLAY}")
        if self.t0 is None:
            self.t0 = self.t - 80

    def keys(self, mask, ms=80):
        self.lines.append(f"{round(self.t)} keys {mask:#x}")
        self.lines.append(f"{round(self.t + ms)} keys 0")
        self.t += ms + 80

    def turn(self, enc, n, every=0):
        """n detents; every > 0: one at a time, so many ms apart (a hand turning it slowly)"""
        if not every:
            self.ev(f"enc {enc} {n}")
            return
        for _ in range(abs(n)):
            self.ev(f"enc {enc} {1 if n > 0 else -1}", every)

    def intensity_to(self, name):
        self.turn(ALGORITHM, INTENSITY.index(name) - self.intensity)
        self.intensity = INTENSITY.index(name)

    def roll(self, style):
        """RNDMZR held half a second: the RANDOM page; K2 the style; a tap of RNDMZR rolls the band in it (playing,
        it lands on the next bar's one); HOME again"""
        self.keys(RNDMZR, 800)
        self.turn(K2, STYLES.index(style) - self.style)
        self.style = STYLES.index(style)
        self.keys(RNDMZR)
        self.ev(f"btn {HOME}")

    def script(self, end):
        return "\n".join(sorted(self.lines + [f"{round(end)} end"], key=lambda s: int(s.split()[0]))) + "\n"


def first_light():
    """A new project as it starts, the macros turned by hand."""
    k = Take()
    k.play()
    k.look("HOME playing: the new project")
    k.at_bar(4)
    k.turn(K1, 18, 220)                                 # CUTOFF opening
    k.look("CUTOFF opened")
    k.at_bar(8)
    k.turn(K2, 10, 200)                                 # DECAY longer
    k.turn(K3, 14, 160)                                 # SPACE: more delay and reverb
    k.look("DECAY and SPACE up")
    k.at_bar(12)
    k.turn(K1, -24, 180)                                # CUTOFF closing again
    k.look("CUTOFF closed")
    k.at_bar(16)
    k.play()                                            # stop: the delay and the reverb ring out
    return k, k.bar(17.5)


def four_styles():
    """RNDMZR on GROOVE rolls the whole band: four styles, eight bars each."""
    k = Take(bpm=120)
    k.intensity_to("GROOVE")
    k.roll("HOUSE")                                     # stopped: the roll is there at once
    k.look("HOUSE rolled")
    k.play()
    for n, style in ((8, "ACID"), (16, "BREAKS"), (24, "OUTRUN")):
        k.at_bar(n - 1, 1)                              # a bar before; it lands on bar n's one
        k.roll(style)
        k.at_bar(n, 0.5)
        k.look(f"{style} landed")
    k.at_bar(32)
    k.play()
    return k, k.bar(33.5)


def performed():
    """A TECHNO roll, then PERFORM: the effects on the keys, held in time."""
    k = Take(bpm=124)
    k.intensity_to("GROOVE")
    k.roll("TECHNO")
    k.play()
    k.at_bar(1)
    k.keys(PERF)                                        # PERFORM, until PERF again
    k.look("PERFORM")
    moves = [(4, 0, 6.0, ["HP SWP"]), (7, 0, 4.0, ["FILL"]), (8, 2, 2.0, ["REP 4"]), (9, 3, 1.0, ["REP 2"]),
             (11, 2, 2.0, ["STUTTER"]), (13, 0, 2.0, ["THROW"]), (14, 2, 1.5, ["TAPE STOP"]),
             (16, 0, 4.0, ["CHOP"]), (18, 0, 7.0, ["LP SWP", "HP SWP"]), (21, 0, 3.0, ["WARP", "OCT DOWN"]),
             (23, 0, 4.0, ["WASH"])]
    for bar, beat, beats, names in moves:
        k.at_bar(bar, beat)
        mask = 0
        for name in names:
            mask |= PF[name]
        k.keys(mask, beats * 60000.0 / k.bpm)
        if names[0] in ("HP SWP", "FILL", "CHOP", "WASH"):
            k.t -= beats * 60000.0 / k.bpm / 2 + 80
            k.look(" + ".join(names) + " held")
    k.at_bar(24)
    k.play()
    return k, k.bar(26)


DEMOS = [
    {"id": "first-light", "take": first_light, "title": "FIRST LIGHT",
     "about": "A new project as it starts, at 80 BPM: the sixteen steps, the follower sub under them, the delay and "
              "the reverb. Then the macros on K1-K3 by hand: the cutoff opens, the decay and the space grow, the "
              "cutoff closes again."},
    {"id": "four-styles", "take": four_styles, "title": "FOUR STYLES",
     "about": "RNDMZR on GROOVE, at 120 BPM: each tap rolls the whole band, the melody, the bass line, the beat, the "
              "chords and the sound, in a style, and lands on the next bar's one. HOUSE, ACID, BREAKS, OUTRUN, eight "
              "bars each."},
    {"id": "performed", "take": performed, "title": "PERFORMED",
     "about": "A TECHNO roll at 124 BPM, then PERFORM: an HP sweep, a fill, repeats, stutter, a throw into the delay, "
              "tape stop, chop, both sweeps together, warp an octave down, and a wash into the reverb, every one held "
              "in time on the keys."},
]
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


def host_build(tmp):
    host = tmp / "host"
    subprocess.run(["cc", "-O2", "-Wall", "-Wno-unused-function", "-DDINGHY_HOST", '-DAPP_VER="0.5"',
                    "-Ifirmware/src", "-Ifirmware/gen", "-o", str(host), "firmware/src/dinghy.c"], cwd=ROOT,
                   check=True)
    return host


def render(host, tmp, d, look):
    take, end = d["take"]()
    run = tmp / d["id"]
    run.mkdir()
    (run / "script.txt").write_text(take.script(end))
    subprocess.run([str(host), str(run / "take"), str(run / "script.txt")], cwd=run, check=True,
                   stdout=subprocess.DEVNULL)
    if look:
        look.mkdir(parents=True, exist_ok=True)
        for t, why in take.looks:
            Image.open(run / f"take-{t}.ppm").save(look / f"{d['id']}-{t:06d}.png")
            print(f"  look: {d['id']} {t / 1000:6.2f} s  {why}")
    return run / "take.wav"


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
    ap.add_argument("--look", metavar="DIR", help="also the screen at each step of the demos, as PNG")
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
    demos = []
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        host = host_build(tmp)
        for d in DEMOS:
            wav = render(host, tmp, d, Path(a.look) if a.look else None)
            x, fs = read_wav(wav)
            spectrum(x, fs, out / "audio" / f"{d['id']}.webp")
            encode(wav, out / "audio" / f"{d['id']}.m4a")
            peak = float(np.abs(x).max())
            demos.append({"id": d["id"], "title": d["title"], "about": d["about"], "src": f"audio/{d['id']}.m4a",
                          "spectrum": f"audio/{d['id']}.webp", "secs": round(len(x) / fs, 1)})
            print(f"  demo: {d['title']}: {len(x) / fs:.1f} s, peak {20 * np.log10(peak + 1e-9):.1f} dBFS")
    meta = json.dumps({"version": a.version, "product": product, "pkg": "firmware/" + name,
                       "sha256": hashlib.sha256(raw).hexdigest(), "size": len(raw)})
    lib = strip_module((HERE / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((HERE / "fm1ota.js").read_text(encoding="utf-8"))
    html = (HERE / "index.html").read_text(encoding="utf-8")
    for mark in ("/*LIB*/", "/*META*/", "/*DEMOS*/"):
        if html.count(mark) != 1:
            raise SystemExit(f"web/index.html must hold {mark} once")
    for s in re.findall(r'src="screens/([^"]+)"', html):
        if not (out / "screens" / s).exists():
            raise SystemExit(f"web/index.html shows screens/{s}, which the site does not have (SCREENS, PICTURES)")
    html = html.replace("/*LIB*/", lib).replace("/*META*/", meta).replace("/*DEMOS*/", json.dumps(demos))
    (out / "index.html").write_text(html, encoding="utf-8")
    (out / ".nojekyll").write_text("")
    size = sum(p.stat().st_size for p in out.rglob("*") if p.is_file())
    print(f"site: {out}: index.html ({len(html)} B), firmware/{name} ({len(raw)} B, {product}), {len(demos)} demos, "
          f"{len(PICTURES) + len(SCREENS)} pictures, {size // 1024} KB in all")


if __name__ == "__main__":
    sys.exit(main())
