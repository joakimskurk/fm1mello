#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""Big screen check: rebuild the screen from the SysEx the host firmware sent (OUT.syx) and compare it with the
screen the host took at the same moment (OUT-T.ppm).

    tests/mirror_decode.py OUT.syx OUT-T.ppm [--png out.png]

The messages are FM-1 Mirror's (github.com/pfkellogg/fm1-mirror): F0 7D 'F' 'S' 01 rl rh cl ch {n c0 c1 c2}... F7,
row and column as two 7-bit bytes, each run a count and the RGB565 colour as bits 0-6, 7-13 and 14-15. The screen
starts black, as the app's does; a full request repaints everything, so after one the two must match exactly.
Exit 0 when they match, 1 with the differing pixel count."""
import sys


def decode(path):
    data = open(path, "rb").read()
    screen = [[0] * 240 for _ in range(240)]
    msgs = 0
    i = 0
    while i < len(data):
        if data[i] != 0xF0:
            i += 1
            continue
        j = data.index(0xF7, i)
        m = data[i : j + 1]
        i = j + 1
        if len(m) < 10 or m[1:5] != bytes([0x7D, ord("F"), ord("S"), 1]):
            continue
        msgs += 1
        y = m[5] | m[6] << 7
        x = m[7] | m[8] << 7
        k = 9
        while k + 4 <= len(m) - 1:
            n, c0, c1, c2 = m[k], m[k + 1], m[k + 2], m[k + 3]
            colour = c0 | c1 << 7 | c2 << 14
            for _ in range(n):
                if x < 240 and y < 240:
                    screen[y][x] = colour
                x += 1
            k += 4
    return screen, msgs, len(data)


def read_ppm(path):
    data = open(path, "rb").read()
    parts = data.split(b"\n", 1)
    header = parts[0].split()
    assert header[0] == b"P6" and header[1] == b"240" and header[2] == b"240", header
    px = parts[1]
    screen = [[0] * 240 for _ in range(240)]
    for y in range(240):
        for x in range(240):
            o = (y * 240 + x) * 3
            r, g, b = px[o], px[o + 1], px[o + 2]
            # the host wrote (c >> 11) * 255 / 31 and so on: back to 5/6/5 by rounding
            screen[y][x] = (r * 31 + 127) // 255 << 11 | (g * 63 + 127) // 255 << 5 | (b * 31 + 127) // 255
    return screen


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    syx, ppm = args[0], args[1]
    got, msgs, nbytes = decode(syx)
    want = read_ppm(ppm)
    diff = sum(1 for y in range(240) for x in range(240) if got[y][x] != want[y][x])
    if "--png" in sys.argv:
        import struct
        import zlib

        out = sys.argv[sys.argv.index("--png") + 1]
        raw = b"".join(
            b"\0" + b"".join(bytes(((c >> 11) * 255 // 31, (c >> 5 & 63) * 255 // 63, (c & 31) * 255 // 31)) for c in row)
            for row in got
        )

        def chunk(t, d):
            return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

        with open(out, "wb") as f:
            f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 240, 240, 8, 2, 0, 0, 0)))
            f.write(chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))
    print(f"{msgs} messages, {nbytes} bytes; {diff} of 57600 pixels differ")
    return 0 if diff == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
