#!/usr/bin/env python3
"""Holds the C port's renders (build/golden-c) against the HTML's (tests/golden/html), stimulus by stimulus.

  python3 tools/tone-ref/compare.py [--json OUT] [name ...]

Per stimulus, all in dB:
  level  RMS of the C render minus the HTML's, over the part where the HTML sounds (within 50 dB of its peak)
  env    mean |difference| of the 5 ms RMS envelopes there, after taking out the level difference: shape
  tone   mean |difference| of the 1/3-octave band levels (63 Hz..12.5 kHz) after taking out the level difference
  worst  the band with the largest tone difference, and how large
  tail   RT60 (s) as T20 x 3 from the Schroeder curve after the last note, HTML -> C (where there is a tail)
Needs numpy."""
import json
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
HTML = ROOT / "tests" / "golden" / "html"
CDIR = ROOT / "build" / "golden-c"
SPEC = json.loads((Path(__file__).resolve().parent / "stimuli.json").read_text())
SR = SPEC["sample_rate"]


def load(path):
    b = path.read_bytes()
    assert b[:4] == b"RIFF" and b[8:12] == b"WAVE", path
    pos, fmt, data = 12, None, None
    while pos < len(b):
        cid, size = b[pos:pos + 4], int.from_bytes(b[pos + 4:pos + 8], "little")
        if cid == b"fmt ":
            fmt = int.from_bytes(b[pos + 8:pos + 10], "little"), int.from_bytes(b[pos + 10:pos + 12], "little")
        elif cid == b"data":
            data = b[pos + 8:pos + 8 + size]
        pos += 8 + size + (size & 1)
    assert fmt == (3, 2), f"{path}: want 32-bit float stereo"
    return np.frombuffer(data, dtype="<f4").reshape(-1, 2).astype(np.float64)


def db(x):
    return 10 * np.log10(np.maximum(x, 1e-20))


def envelope(x, win=220):
    e = (x ** 2).mean(axis=1)
    n = len(e) // win
    return db(e[: n * win].reshape(n, win).mean(axis=1))


def bands(x):
    mono = x.mean(axis=1)
    spec = np.abs(np.fft.rfft(mono * np.hanning(len(mono)))) ** 2
    f = np.fft.rfftfreq(len(mono), 1 / SR)
    centres = 1000 * 2 ** (np.arange(-12, 12) / 3)  # 63 Hz .. 12.5 kHz
    out = []
    for c in centres:
        m = (f >= c / 2 ** (1 / 6)) & (f < c * 2 ** (1 / 6))
        out.append(db(spec[m].sum()))
    return centres, np.array(out)


def rt60(x, start):
    e = (x[start:] ** 2).sum(axis=1)
    if e.sum() <= 0:
        return None
    edc = db(np.cumsum(e[::-1])[::-1] / e.sum())
    try:  # T20 (-5..-25 dB) x 3: stays clear of the 16-bit floor the C render ends in
        i5, i25 = np.nonzero(edc <= -5)[0][0], np.nonzero(edc <= -25)[0][0]
    except IndexError:
        return None
    return 3 * (i25 - i5) / SR


def compare(st):
    h, c = load(HTML / f"{st['name']}.wav"), load(CDIR / f"{st['name']}.wav")
    n = min(len(h), len(c))
    h, c = h[:n], c[:n]
    eh, ec = envelope(h), envelope(c)
    live = eh > eh.max() - 50
    level = db((c[: len(live) * 220][np.repeat(live, 220)] ** 2).mean()) - db((h[: len(live) * 220][np.repeat(live, 220)] ** 2).mean())
    env = np.abs((ec[live] - level) - eh[live]).mean()
    centres, bh = bands(h)
    _, bc = bands(c)
    audible = bh > bh.max() - 60
    d = (bc - level) - bh
    tone = np.abs(d[audible]).mean()
    wi = int(np.argmax(np.where(audible, np.abs(d), -1)))
    notes = st.get("lead", []) + st.get("sub", [])
    end = max(n_["t"] + n_["dur"] for n_ in notes) + 0.15
    tail = None
    if st["fx"] != "dry" and end < st["seconds"] - 1.0:
        s = int(end * SR)
        tail = (rt60(h, s), rt60(c, s))
    return {
        "name": st["name"], "level_db": round(float(level), 1), "env_db": round(float(env), 1),
        "tone_db": round(float(tone), 1), "worst_band_hz": int(round(centres[wi])), "worst_db": round(float(d[wi]), 1),
        "peak_html_dbfs": round(float(db((h ** 2).max(axis=1).max())), 1),
        "peak_c_dbfs": round(float(db((c ** 2).max(axis=1).max())), 1),
        "rt60_html_c": None if tail is None else [None if v is None else round(v, 2) for v in tail],
    }


def main(argv):
    out_json = None
    if argv[:1] == ["--json"]:
        out_json, argv = argv[1], argv[2:]
    rows = [compare(st) for st in SPEC["stimuli"] if not argv or st["name"] in argv]
    print(f"{'stimulus':18} {'level':>6} {'env':>5} {'tone':>5}  {'worst band':>16}  {'peak html/c dBFS':>17}  rt60 html->c")
    for r in rows:
        rt = "" if r["rt60_html_c"] is None else f"{r['rt60_html_c'][0]} -> {r['rt60_html_c'][1]} s"
        print(f"{r['name']:18} {r['level_db']:+6.1f} {r['env_db']:5.1f} {r['tone_db']:5.1f}  "
              f"{r['worst_db']:+6.1f} @ {r['worst_band_hz']:5d} Hz  {r['peak_html_dbfs']:7.1f} / {r['peak_c_dbfs']:6.1f}  {rt}")
    if out_json:
        Path(out_json).write_text(json.dumps(rows, indent=1) + "\n")


if __name__ == "__main__":
    main(sys.argv[1:])
