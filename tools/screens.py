#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""Every page of SkurkyMello as a PNG, drawn by the firmware itself on this computer.

The firmware runs in host mode (tests/host.c) on a script that presses the panel's buttons, turns SELECT and holds
the keys the way a player would, and asks for the screen after each page has settled. The 240 x 240 screens come out
as PNG (doubled, so the pixels stay crisp) in docs/screens/, and docs/mello/UI_MAP.md gets a picture in every row.
Python 3 only: no Pillow, no toolchain.

    python3 tools/screens.py                 # docs/screens/*.png and the UI map's pictures
    python3 tools/screens.py --no-map        # the pictures only
    python3 tools/screens.py --sheet DIR     # also contact sheets of twelve screens each, to look through

The tour below is the manual's order: HOME, then each section button through its pages and layers, then a few
shots of the instrument playing. A shot's page and layer are the UI map's names, so the two stay matched.
"""
import argparse
import os
import re
import struct
import subprocess
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W = H = 240

# Dinghy's panel ids (platform.c), the same as MelloUiButton / MelloUiEncoder
OCT_DOWN, OCT_UP, FX, SEL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY, REC = range(14)
ENC_SELECT = 0
B10 = 1 << 25  # the last black key, RNDMZR: key 25 of the 27 the UI counts (16 white, 11 black)
B5 = 1 << 13  # PERF: the sixth black key
WHITE3 = 1 << 4  # the third white key (E)

GAP = 80  # ms between two inputs
SETTLE = 1400  # ms from the last input to the shot: the layer readout stays 1200 ms after a SELECT detent


class Tour:
    """Writes the host script and remembers which shot is which."""

    def __init__(self):
        self.lines = []
        self.t = 2000  # the splash is up for 1.5 s
        self.shots = []  # (t, slug, page, layer, caption)

    def _ev(self, what):
        self.lines.append("%u %s" % (self.t, what))
        self.t += GAP

    def btn(self, b):
        self._ev("btn %d" % b)

    def sel(self, n=1):
        for _ in range(n):
            self._ev("enc %d 1" % ENC_SELECT)

    def hold(self, b, ms):
        self.lines.append("%u hold %d" % (self.t, b))
        self.lines.append("%u release %d" % (self.t + ms, b))
        self.t += ms + GAP

    def keys(self, mask, ms):
        """Keys down for ms, then up."""
        self.lines.append("%u keys 0x%x" % (self.t, mask))
        self.lines.append("%u keys 0" % (self.t + ms))
        self.t += ms + GAP

    def keys_down(self, mask):
        self._ev("keys 0x%x" % mask)

    def wait(self, ms):
        self.t += ms

    def shot(self, slug, page, layer, caption="", settle=SETTLE):
        self.t += settle
        self.shot_at(self.t, slug, page, layer, caption)
        self.t += GAP

    def shot_at(self, t, slug, page, layer, caption=""):
        self.lines.append("%u shot" % t)
        self.shots.append((t, slug, page, layer, caption))

    def end(self):
        self.lines.append("%u end" % (self.t + 200))


def layers(tour, prefix, page, names, first_shot=True):
    """The page's layers in SELECT's order, one shot each (the first without a turn)."""
    for i, name in enumerate(names):
        if i or not first_shot:
            tour.sel()
        slug = prefix if name is None else "%s-%s" % (prefix, slugify(name))
        tour.shot(slug, page, name or "—")


def slugify(s):
    return re.sub(r"[^a-z0-9]+", "-", s.lower()).strip("-")


def the_tour():
    t = Tour()
    t.shot_at(900, "splash", None, None, "The splash: the version, while the project loads")
    t.shot("home", "HOME", "—", "HOME, stopped: the sequence, the macros, the keys' strip")
    t.hold(EDIT, 700)  # EDIT held: MACROS, the knobs pick what they turn
    t.shot("home-macros", "HOME", "MACROS", "EDIT held on HOME: MACROS, each knob picks its parameter")
    t.btn(EDIT)
    t.btn(EDIT)  # tapped: KEYS (its own shot comes later, playing)
    t.btn(REC)  # REC > SEQ
    t.shot("home-rec", "HOME", "REC", "REC in KEYS: the keys write the sequence; again BASS, again CHORD")
    t.btn(REC)
    t.btn(REC)
    t.btn(REC)  # round to off
    t.btn(EDIT)  # out of KEYS

    # SEQ: STEP, SEGMENT, LOOP CYCLE, SLOT SEQ
    t.btn(SEQ)
    layers(t, "step", "STEP", ["TRIG", "NOTE", "FLAGS", "SEQ"])
    t.btn(SEQ)
    t.shot("segment", "SEGMENT", "SEGMENT")  # (the map's second SEGMENT row is the same picture)
    t.btn(SEQ)
    layers(t, "loop", "LOOP CYCLE", ["STEP", "HARM", "LOOP"])
    t.btn(SEQ)
    t.shot("slotseq", "SLOT SEQ", "—")

    # RANDOM: RNDMZR (B10) held; SELECT toggles LINKS; EDIT is the amounts
    t.keys(B10, 700)
    t.shot("random-mask", "RANDOM", "MASK")
    t.sel()
    t.shot("random-links", "RANDOM", "LINKS")
    t.sel()
    t.btn(EDIT)
    t.shot("random-amounts", "RANDOM", "AMOUNTS")
    t.btn(EDIT)

    t.btn(SEL)
    t.shot("key-scale", "KEY / SCALE", "—")

    t.hold(SAVE, 900)
    layers(t, "slots", "SLOTS", ["LOAD", "RENAME"])

    # ENV: SOUND (SELECT's order; the LFO layer belongs to the LFO button), then SAMPLE
    t.btn(ENV)
    layers(t, "sound", "SOUND",
           ["OSC/FILT", "OSC 2", "CHARACTER", "GRIT", "MOD ENV", "TRACKING", "AMP ENV", "FOLLOWER", "LEVEL"])
    t.btn(ENV)
    layers(t, "sample", "SAMPLE", ["MAIN", "PITCH", "TRIM", "ENVELOPE"])

    # LFO: the lead's LFO, DOT (the five matrices and the two MOD shapes), EVOLVE
    t.btn(LFO)
    t.shot("sound-lfo", "SOUND", "LFO")
    t.btn(LFO)
    layers(t, "dot", "DOT MATRIX", ["ACCENT", "MOD 1", "SHAPE 1", "MOD 2", "SHAPE 2", "RATCHET", "SUB"])
    t.btn(LFO)
    layers(t, "evolve", "EVOLVE", ["MAIN", "ROUTES", "DEPTH", "SOUND"])

    # FX: DISTORTION, DELAY (send, config), REVERB (room, returns)
    t.btn(FX)
    t.shot("fx-distortion", "AUDIO FX", "DISTORTION")
    t.btn(FX)
    layers(t, "fx", "AUDIO FX", ["DELAY SEND", "DELAY CONFIG"])
    t.btn(FX)
    layers(t, "fx", "AUDIO FX", ["REVERB ROOM", "REVERB RETURNS"])

    # ARP: CHORD, BASS, DRUMS
    t.btn(ARP)
    layers(t, "chord", "CHORD",
           ["PLAY", "CHORD", "VOICING", "GRID", "PATTERN", "TONE", "FILTER", "MOTION", "AMP", "HARMONY"])
    t.btn(ARP)
    layers(t, "bass", "BASS", ["PLAY", "SEQ", "STEP", "VOICE", "TONE", "FILTER"])
    t.btn(ARP)
    layers(t, "drums", "DRUMS", ["VOICE", "STEP", "VARY", "KIT", "DIST"])

    # GLO: MIX, GLOBAL
    t.btn(GLO)
    t.shot("mix", "MIX", "LEVEL")
    t.sel()
    t.shot("mix-delay", "MIX", "DELAY")
    t.sel()
    t.shot("mix-reverb", "MIX", "REVERB")
    t.btn(GLO)
    layers(t, "global", "GLOBAL", ["CLOCK", "MIDI", "GROOVE"])

    # Playing: the shots for the tour in the README
    t.btn(HOME)
    t.btn(PLAY)
    t.wait(2600)
    t.shot("home-playing", None, None, "PLAY: the playing step bright, the LED on the beat", settle=0)
    t.keys(B10, 120)  # a tap on RNDMZR: a roll
    t.shot("home-rolled", None, None, "RNDMZR tapped: a new sequence from the RANDOM page's mask", settle=700)
    t.keys_down(WHITE3)
    t.shot("home-held-step", None, None, "A white key held: that step's pitch, pulses, ratchet and gate on the knobs")
    t.keys_down(0)
    t.btn(EDIT)
    t.shot("home-keys", None, None, "EDIT on HOME: KEYS, the keys lit where the scale has a tone; playing, a key is the root")
    t.btn(EDIT)
    t.btn(SEQ)
    t.shot("step-playing", None, None, "SEQ > STEP while playing")
    t.btn(ARP)
    t.btn(ARP)
    t.btn(ARP)
    t.sel()  # DIST was left open: round to VOICE
    t.shot("drums-playing", None, None, "ARP > DRUMS while playing")
    t.btn(HOME)
    t.keys(B5, 120)  # PERF tapped: PERFORM stays open
    t.shot("perform", "PERFORM", "—", "B5 PERF: the white keys are the performance actions, on every part")
    t.btn(HOME)
    t.btn(PLAY)
    t.end()
    return t


def version_from_readme():
    try:
        with open(os.path.join(ROOT, "README.md"), encoding="utf-8") as f:
            m = re.search(r"skurkymello-([0-9][\w.-]*)\.fwsc", f.read())
            return m.group(1) if m else "dev"
    except OSError:
        return "dev"


def build_host(version):
    """tests/host.c's build line; the binary is named by the version it shows on the splash."""
    out = os.path.join(ROOT, "build", "screens", "host-" + version)
    srcs = [os.path.join(ROOT, "tests", "host.c")]
    for d in ("firmware/src", "firmware/gen"):
        p = os.path.join(ROOT, d)
        if os.path.isdir(p):
            srcs += [os.path.join(p, f) for f in os.listdir(p) if f.endswith((".c", ".h"))]
    if os.path.exists(out) and os.path.getmtime(out) >= max(os.path.getmtime(s) for s in srcs):
        return out
    os.makedirs(os.path.dirname(out), exist_ok=True)
    cmd = [os.environ.get("CC", "cc"), "-O2", "-Wall", "-Wno-unused-function", "-DDINGHY_HOST",
           '-DAPP_VER="%s"' % version, "-Ifirmware/src", "-Ifirmware/gen", "-o", out, "firmware/src/dinghy.c"]
    print("building the host firmware (%s) ..." % version, file=sys.stderr)
    subprocess.run(cmd, cwd=ROOT, check=True)
    return out


def read_ppm(path):
    with open(path, "rb") as f:
        d = f.read()
    m = re.match(rb"P6\s+(\d+)\s+(\d+)\s+255\s", d)
    if not m or (int(m.group(1)), int(m.group(2))) != (W, H):
        raise SystemExit("%s: not a 240 x 240 P6 PPM" % path)
    return d[m.end():]


def png_bytes(rgb, w, h, scale):
    rows = []
    for y in range(h):
        row = rgb[y * w * 3:(y + 1) * w * 3]
        if scale > 1:
            row = b"".join(row[i * 3:i * 3 + 3] * scale for i in range(w))
        rows += [b"\0" + row] * scale
    raw = b"".join(rows)

    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)

    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w * scale, h * scale, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def contact_sheets(shots_rgb, out_dir, cols=4, rows=3):
    """Twelve screens a sheet, at 1:1, to look through quickly (no labels: the tour's order)."""
    os.makedirs(out_dir, exist_ok=True)
    per = cols * rows
    for n in range(0, len(shots_rgb), per):
        batch = shots_rgb[n:n + per]
        sheet = bytearray(b"\x20\x20\x20" * (W * cols) * (H * rows))
        for i, (_slug, rgb) in enumerate(batch):
            cx, cy = (i % cols) * W, (i // cols) * H
            for y in range(H):
                dst = ((cy + y) * W * cols + cx) * 3
                sheet[dst:dst + W * 3] = rgb[y * W * 3:(y + 1) * W * 3]
        path = os.path.join(out_dir, "sheet-%02d.png" % (n // per + 1))
        with open(path, "wb") as f:
            f.write(png_bytes(bytes(sheet), W * cols, H * rows, 1))
        print("%s: %s" % (path, ", ".join(s for s, _ in batch)))


def update_map(path, shots):
    """A Screen column, first in the UI map's page table, keyed on (Page, Layer)."""
    by_key = {(p, l): slug for _t, slug, p, l, _c in shots if p}
    by_key[("SEGMENT", "(2)")] = by_key.get(("SEGMENT", "SEGMENT"))  # the map lists SEGMENT twice: one picture
    with open(path, encoding="utf-8") as f:
        lines = f.read().split("\n")
    out, in_table, missing = [], False, []
    for line in lines:
        cells = [c.strip() for c in line.strip().strip("|").split("|")] if line.startswith("|") else None
        while cells and (cells[0] in ("", "Screen") or cells[0].startswith("<img ")):
            cells.pop(0)  # a Screen column from an earlier run: replaced
        if cells and cells[:2] == ["Page", "Button"]:
            in_table = True
            out.append("| Screen | " + " | ".join(cells) + " |")
            continue
        if in_table and cells and cells[0].startswith("---"):
            out.append("|---|" + "|".join(cells[-5:]) + "|")
            continue
        if in_table and cells and len(cells) >= 3:
            slug = by_key.get((cells[0], cells[2]))
            img = ('<img src="../screens/%s.png" width="96" alt="%s %s">' % (slug, cells[0], cells[2])) if slug else ""
            if not slug:
                missing.append("%s / %s" % (cells[0], cells[2]))
            out.append("| %s | %s |" % (img, " | ".join(cells)))
            continue
        if in_table and not line.startswith("|"):
            in_table = False
        out.append(line)
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    if missing:
        print("no screen for: " + "; ".join(missing), file=sys.stderr)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--out", default=os.path.join(ROOT, "docs", "screens"), help="where the PNGs go")
    ap.add_argument("--map", default=os.path.join(ROOT, "docs", "mello", "UI_MAP.md"), help="the UI map to picture")
    ap.add_argument("--no-map", action="store_true", help="leave the UI map alone")
    ap.add_argument("--scale", type=int, default=2, help="pixel doubling (default 2: 480 x 480)")
    ap.add_argument("--version", default=version_from_readme(), help="the version the splash shows")
    ap.add_argument("--sheet", metavar="DIR", help="also contact sheets, twelve screens each, in DIR")
    ap.add_argument("--script", metavar="FILE", help="write the host script here too")
    a = ap.parse_args()

    tour = the_tour()
    work = os.path.join(ROOT, "build", "screens")
    os.makedirs(work, exist_ok=True)
    os.makedirs(a.out, exist_ok=True)
    script = os.path.join(work, "tour.txt")
    with open(script, "w") as f:
        f.write("# SkurkyMello's screens: written by tools/screens.py\n" + "\n".join(tour.lines) + "\n")
    if a.script:
        with open(a.script, "w") as f:
            f.write("\n".join(tour.lines) + "\n")

    host = build_host(a.version)
    for f in os.listdir(work):
        if f.startswith("s-") and f.endswith(".ppm"):
            os.unlink(os.path.join(work, f))
    subprocess.run([host, os.path.join(work, "s"), script], cwd=ROOT, check=True, stdout=subprocess.DEVNULL)

    wanted = {slug + ".png" for _t, slug, _p, _l, _c in tour.shots}
    for f in os.listdir(a.out):  # the folder is the tour's: a screen the tour no longer takes goes
        if f.endswith(".png") and f not in wanted:
            os.unlink(os.path.join(a.out, f))
            print("removed %s (not in the tour)" % f, file=sys.stderr)
    sheet = []
    for t, slug, _p, _l, _c in tour.shots:
        rgb = read_ppm(os.path.join(work, "s-%u.ppm" % t))
        with open(os.path.join(a.out, slug + ".png"), "wb") as f:
            f.write(png_bytes(rgb, W, H, a.scale))
        sheet.append((slug, rgb))
    print("%d screens -> %s" % (len(tour.shots), os.path.relpath(a.out, ROOT)))
    if a.sheet:
        contact_sheets(sheet, a.sheet)
    if not a.no_map:
        update_map(a.map, tour.shots)
        print("pictures in %s" % os.path.relpath(a.map, ROOT))


if __name__ == "__main__":
    main()
