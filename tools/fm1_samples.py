#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""Put a WAV into one of SkurkyMello's four sample slots over USB-MIDI, or look at what is there.

  fm1_samples.py send SOUND.wav --slot N [--name NAME] [--start S] [--length S] [--gain DB | --normalize]
                                [--port NAME] [--no-ack]
  fm1_samples.py clear --slot N [--port NAME] [--no-ack]
  fm1_samples.py query --slot N [--port NAME]
  fm1_samples.py pack SOUND.wav --slot N -o RECORD.bin [the same sound options]
  fm1_samples.py decode RECORD.bin -o SOUND.wav
  fm1_samples.py emu-keys SOUND.wav --slot N [--at MS] [--pace MS]   the transfer as fm1-emu's --keys script
  fm1_samples.py ports

The firmware (firmware/src/mello_sample.c) speaks plain channel messages on MIDI channel 16, not SysEx,
because Dinghy's USB code only assembles the six-byte soft keys:
  control change 110 slot   START: the device erases its staging area and answers CC 120 (ACK) or 121 (ERROR)
  poly aftertouch d1 d2     DATA: two 7-bit digits; eight digits carry seven bytes (a mask byte, then the low
                            seven bits of each), the record exactly as it lies in flash
  control change 111 x      END: the record is checked (CRCs) and copied into the slot; ACK or ERROR
  control change 112 x      ABORT, 113 slot CLEAR (ACK/ERROR; only straight after 114 for the same slot),
                            114 slot QUERY (ACK = loaded, ERROR = empty)
The device only accepts START, END and CLEAR while the sequencer is stopped and the sound has died away.

The record: a 64-byte header (SM10, version 1, slot, codec 1 = IMA ADPCM, 22050 Hz, 256 samples a block,
sample count, block count, data bytes, data CRC-32, name up to 20 characters, header CRC-32), then blocks
of 132 bytes (predictor, step index, pad, 128 bytes of nibbles, low nibble first), then SMP! and the data
CRC. A slot is 32 KiB: 247 blocks, 63 232 samples, 2.87 s. The WAV is mixed to mono and resampled to
22 050 Hz here (a box filter, then linear interpolation); the device plays it at any pitch.

Emulators (fm1-emu, the web emulator) take MIDI in through the emulated TRS jack and send nothing back: --no-ack
waits a fixed time instead of the ACKs; emu-keys writes the transfer for fm1-emu's command line
(fm1-emu --fwsc F.fwsc --ms 8000 --keys "$(cat keys.txt)"). Exit codes: 0 ok, 2 usage / a bad file, 3 no port, 4 the device said ERROR, 5 timeout.
"""
import argparse
import binascii
import math
import re
import struct
import sys
import time
import wave

RATE = 22050
BLOCK_SAMPLES = 256
BLOCK_BYTES = 132
HEADER = 64
FOOTER = 8
SLOT_SIZE = 0x8000
SLOTS = 4
DATA_MAX = SLOT_SIZE - HEADER - FOOTER
MAX_BLOCKS = DATA_MAX // BLOCK_BYTES              # 247
MAX_SAMPLES = MAX_BLOCKS * BLOCK_SAMPLES          # 63 232
NAME_CHARS = 20
MAGIC = b"SM10"
COMMIT = b"SMP!"
VERSION = 1
CODEC_IMA = 1

CH = 15                                           # MIDI channel 16
CC_START, CC_END, CC_ABORT, CC_CLEAR, CC_QUERY, CC_ACK, CC_ERROR = 110, 111, 112, 113, 114, 120, 121
PORT_RE = re.compile(r"fm-1|felucca|skurky|mello|composite|usb-midi|iac", re.I)

IMA_STEP = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88,
    97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660,
    4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
    18500, 20350, 22385, 24623, 27086, 29794, 32767]
IMA_INDEX = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8]


class ToolError(Exception):
    def __init__(self, code, msg):
        super().__init__(msg)
        self.code = code


# ------------------------------------------------------------------ sound ---

def read_wav(path):
    """-> (samples as floats in [-1, 1), mono), rate"""
    with wave.open(path, "rb") as w:
        ch, width, rate, n = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        raw = w.readframes(n)
    if width == 1:
        vals = [(b - 128) / 128.0 for b in raw]
    elif width == 2:
        vals = [v / 32768.0 for v in struct.unpack("<%dh" % (len(raw) // 2), raw)]
    elif width == 3:
        vals = [int.from_bytes(raw[i:i + 3], "little", signed=True) / 8388608.0 for i in range(0, len(raw), 3)]
    elif width == 4:
        vals = [v / 2147483648.0 for v in struct.unpack("<%di" % (len(raw) // 4), raw)]
    else:
        raise ToolError(2, "%s: %d-byte samples are not supported" % (path, width))
    if ch > 1:
        vals = [sum(vals[i:i + ch]) / ch for i in range(0, len(vals) - ch + 1, ch)]
    return vals, rate


def resample(x, rate, to=RATE):
    """mono floats at rate -> at `to`: a box filter the width of the ratio (downsampling), then linear interpolation"""
    if rate == to or not x:
        return list(x)
    ratio = rate / to
    if ratio > 1.0:
        k = int(math.ceil(ratio))
        acc, out, run = 0.0, [], []
        for i, v in enumerate(x):              # running mean over k samples
            run.append(v)
            acc += v
            if len(run) > k:
                acc -= run.pop(0)
            out.append(acc / len(run))
        x = out
    n = int(len(x) / ratio)
    y = []
    for i in range(n):
        p = i * ratio
        j = int(p)
        f = p - j
        a = x[j]
        b = x[j + 1] if j + 1 < len(x) else a
        y.append(a + (b - a) * f)
    return y


def to_pcm(x, gain_db=0.0, normalize=False):
    g = 10.0 ** (gain_db / 20.0)
    if normalize:
        peak = max((abs(v) for v in x), default=0.0)
        if peak > 0.0:
            g = 0.98 / peak
    out = []
    for v in x:
        s = int(round(v * g * 32767.0))
        out.append(max(-32768, min(32767, s)))
    return out


def ima_encode(pcm):
    """16-bit samples -> ADPCM blocks (bytes), as mello_sample_decode_block reads them; the step index carries
    over from block to block, each block stores its own start"""
    blocks = bytearray()
    idx = 0
    n = len(pcm)
    for b0 in range(0, n, BLOCK_SAMPLES):
        chunk = pcm[b0:b0 + BLOCK_SAMPLES]
        chunk = chunk + [chunk[-1]] * (BLOCK_SAMPLES - len(chunk))
        pred = chunk[0]
        blocks += struct.pack("<hBB", pred, idx, 0)
        nibbles = []
        for s in chunk[1:]:
            step = IMA_STEP[idx]
            diff = s - pred
            nib = 0
            if diff < 0:
                nib = 8
                diff = -diff
            vp = step >> 3
            if diff >= step:
                nib |= 4
                diff -= step
                vp += step
            if diff >= step >> 1:
                nib |= 2
                diff -= step >> 1
                vp += step >> 1
            if diff >= step >> 2:
                nib |= 1
                vp += step >> 2
            pred = pred - vp if nib & 8 else pred + vp
            pred = max(-32768, min(32767, pred))
            idx = max(0, min(88, idx + IMA_INDEX[nib]))
            nibbles.append(nib)
        nibbles.append(0)                               # 255 nibbles: the last byte's high half is unused
        for i in range(0, BLOCK_SAMPLES, 2):
            blocks.append(nibbles[i] | nibbles[i + 1] << 4)
    assert len(blocks) == ((n + BLOCK_SAMPLES - 1) // BLOCK_SAMPLES) * BLOCK_BYTES
    return bytes(blocks)


def ima_decode(data, sample_count):
    out = []
    for b0 in range(0, len(data), BLOCK_BYTES):
        p = data[b0:b0 + BLOCK_BYTES]
        pred, idx, _ = struct.unpack("<hBB", p[:4])
        idx = min(idx, 88)
        out.append(pred)
        for ni in range(BLOCK_SAMPLES - 1):
            by = p[4 + (ni >> 1)]
            nib = (by >> 4) if ni & 1 else (by & 15)
            step = IMA_STEP[idx]
            diff = step >> 3
            if nib & 1:
                diff += step >> 2
            if nib & 2:
                diff += step >> 1
            if nib & 4:
                diff += step
            pred = pred - diff if nib & 8 else pred + diff
            pred = max(-32768, min(32767, pred))
            idx = max(0, min(88, idx + IMA_INDEX[nib]))
            out.append(pred)
    return out[:sample_count]


# ----------------------------------------------------------------- record ---

def make_record(pcm, slot, name):
    if not 0 <= slot < SLOTS:
        raise ToolError(2, "slot must be 0..%d" % (SLOTS - 1))
    if not pcm:
        raise ToolError(2, "the sound is empty")
    if len(pcm) > MAX_SAMPLES:
        raise ToolError(2, "the sound is %d samples; a slot holds %d (%.2f s at %d Hz): cut it with --start / --length"
                        % (len(pcm), MAX_SAMPLES, MAX_SAMPLES / RATE, RATE))
    name_b = name.encode("ascii", "replace")[:NAME_CHARS]
    data = ima_encode(pcm)
    blocks = len(data) // BLOCK_BYTES
    data_crc = binascii.crc32(data) & 0xFFFFFFFF
    h = bytearray(HEADER)
    h[0:4] = MAGIC
    struct.pack_into("<HBBHHIH", h, 4, VERSION, slot, CODEC_IMA, RATE, BLOCK_SAMPLES, len(pcm), blocks)
    struct.pack_into("<II", h, 20, len(data), data_crc)
    h[28] = len(name_b)
    h[32:32 + len(name_b)] = name_b
    struct.pack_into("<I", h, 56, binascii.crc32(bytes(h[:56])) & 0xFFFFFFFF)
    return bytes(h) + data + COMMIT + struct.pack("<I", data_crc)


def parse_record(rec):
    """-> dict (slot, name, sample_count, blocks, data) or ToolError"""
    if len(rec) < HEADER + FOOTER or rec[:4] != MAGIC:
        raise ToolError(2, "not a sample record (no SM10 header)")
    version, slot, codec, rate, bs, count, blocks = struct.unpack_from("<HBBHHIH", rec, 4)
    data_bytes, data_crc = struct.unpack_from("<II", rec, 20)
    if version != VERSION or codec != CODEC_IMA or rate != RATE or bs != BLOCK_SAMPLES:
        raise ToolError(2, "a record of another version / codec / rate")
    if struct.unpack_from("<I", rec, 56)[0] != binascii.crc32(rec[:56]) & 0xFFFFFFFF:
        raise ToolError(2, "the header's CRC does not match")
    if data_bytes != blocks * BLOCK_BYTES or len(rec) < HEADER + data_bytes + FOOTER:
        raise ToolError(2, "the record is cut short")
    data = rec[HEADER:HEADER + data_bytes]
    foot = rec[HEADER + data_bytes:HEADER + data_bytes + FOOTER]
    if foot[:4] != COMMIT or struct.unpack("<I", foot[4:])[0] != data_crc or binascii.crc32(data) & 0xFFFFFFFF != data_crc:
        raise ToolError(2, "the data's CRC does not match")
    name = bytes(rec[32:32 + min(rec[28], NAME_CHARS)]).decode("ascii", "replace")
    return {"slot": slot, "name": name, "sample_count": count, "blocks": blocks, "data": data}


def pack_7bit(raw):
    """seven bytes -> eight digits: the high bits as a mask, then the low seven bits of each; the last group is
    padded with zeros (the device stops at the record's length)"""
    out = []
    for i in range(0, len(raw), 7):
        grp = raw[i:i + 7]
        grp = grp + b"\0" * (7 - len(grp))
        mask = 0
        for k, b in enumerate(grp):
            mask |= (b >> 7) << k
        out.append(mask)
        out.extend(b & 0x7F for b in grp)
    if len(out) & 1:
        out.append(0)
    return out


def sound_from_args(a):
    x, rate = read_wav(a.wav)
    start = int(a.start * rate)
    x = x[start:]
    if a.length is not None:
        x = x[:int(a.length * rate)]
    y = resample(x, rate)
    if len(y) > MAX_SAMPLES and a.length is None:
        print("note: %.2f s is more than a slot holds; the first %.2f s are used" % (len(y) / RATE, MAX_SAMPLES / RATE),
              file=sys.stderr)
        y = y[:MAX_SAMPLES]
    pcm = to_pcm(y, a.gain, a.normalize)
    name = a.name if a.name else re.sub(r"\.wav$", "", a.wav.rsplit("/", 1)[-1], flags=re.I).upper()
    return pcm, name


def write_wav(path, pcm, rate=RATE):
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(struct.pack("<%dh" % len(pcm), *pcm))


# ------------------------------------------------------------------- MIDI ---

class Link:
    def __init__(self, port_hint, want_input):
        try:
            import mido
        except ImportError:
            raise ToolError(3, "mido is not installed: pip install mido python-rtmidi")
        self.mido = mido
        outs = list(dict.fromkeys(mido.get_output_names()))
        ins = list(dict.fromkeys(mido.get_input_names()))
        self.out_name = self._pick(outs, port_hint)
        if not self.out_name:
            raise ToolError(3, "no MIDI output found%s; ports: %s" % (" matching %r" % port_hint if port_hint else "", outs))
        self.out = mido.open_output(self.out_name)
        self.inp = None
        if want_input:
            name = self._pick(ins, port_hint) or self._pick(ins, re.sub(r"\s+\d+$", "", self.out_name))
            if name:
                self.inp = mido.open_input(name)
        self.sent = 0

    @staticmethod
    def _pick(names, hint):
        if hint:
            mine = [n for n in names if hint.lower() in n.lower()]
            if mine:
                return mine[0]
            return None
        mine = [n for n in names if PORT_RE.search(n)]
        return mine[0] if mine else None

    def cc(self, control, value):
        self.out.send(self.mido.Message("control_change", channel=CH, control=control, value=value & 0x7F))
        self.sent += 1

    def data(self, d1, d2):
        self.out.send(self.mido.Message("polytouch", channel=CH, note=d1, value=d2))
        self.sent += 1

    def wait_ack(self, timeout):
        """-> (ok, arg) from the device's CC 120 / 121, or None on a timeout"""
        if not self.inp:
            return None
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            for m in self.inp.iter_pending():
                if m.type == "control_change" and m.channel == CH and m.control in (CC_ACK, CC_ERROR):
                    return m.control == CC_ACK, m.value
            time.sleep(0.002)
        return None

    def close(self):
        self.out.close()
        if self.inp:
            self.inp.close()


def expect_ack(link, what, timeout, no_ack, settle, either=False):
    """Wait for ACK; with either, ERROR is an answer too (a QUERY's: the slot is empty)."""
    if no_ack or not link.inp:
        time.sleep(settle)
        return
    r = link.wait_ack(timeout)
    if r is None:
        raise ToolError(5, "%s: no answer from the device in %.1f s" % (what, timeout))
    if not r[0] and not either:
        raise ToolError(4, "%s: the device said ERROR (slot %d). It takes samples only while stopped and silent;"
                           " a START also fails when the slot number is bad" % (what, r[1]))


def cmd_send(a):
    pcm, name = sound_from_args(a)
    rec = make_record(pcm, a.slot, name)
    digits = pack_7bit(rec)
    link = Link(a.port, not a.no_ack)
    try:
        print("%s -> slot %d as %r: %d samples (%.2f s), %d bytes, %d MIDI messages over %s" %
              (a.wav, a.slot, name, len(pcm), len(pcm) / RATE, len(rec), len(digits) // 2 + 2, link.out_name))
        if not a.no_ack and not link.inp:
            print("note: no matching MIDI input: not waiting for the device's answers (as --no-ack)", file=sys.stderr)
        link.cc(CC_START, a.slot)
        expect_ack(link, "START", 5.0, a.no_ack, 1.0)
        t0 = time.monotonic()
        for i in range(0, len(digits), 2):
            link.data(digits[i], digits[i + 1])
            if (i >> 1) % a.burst == a.burst - 1:
                time.sleep(a.pace / 1000.0)        # the device programs 256-byte pages between messages
        link.cc(CC_END, a.slot)
        expect_ack(link, "END", 10.0, a.no_ack, 2.0)
        print("done in %.1f s" % (time.monotonic() - t0))
    finally:
        link.close()


def cmd_clear(a):
    link = Link(a.port, not a.no_ack)
    try:
        # CLEAR is taken only as the next message after a QUERY of the same slot (a stray CC 113 clears nothing)
        link.cc(CC_QUERY, a.slot)
        expect_ack(link, "QUERY", 2.0, a.no_ack, 0.2, either=True)
        link.cc(CC_CLEAR, a.slot)
        expect_ack(link, "CLEAR", 5.0, a.no_ack, 1.0)
        print("slot %d cleared" % a.slot)
    finally:
        link.close()


def cmd_query(a):
    link = Link(a.port, True)
    try:
        if not link.inp:
            raise ToolError(3, "query needs a MIDI input from the device (none matched)")
        link.cc(CC_QUERY, a.slot)
        r = link.wait_ack(2.0)
        if r is None:
            raise ToolError(5, "no answer in 2 s")
        print("slot %d: %s" % (a.slot, "loaded" if r[0] else "empty"))
    finally:
        link.close()


def cmd_pack(a):
    pcm, name = sound_from_args(a)
    rec = make_record(pcm, a.slot, name)
    with open(a.output, "wb") as f:
        f.write(rec)
    print("%s: slot %d %r, %d samples, %d bytes" % (a.output, a.slot, name, len(pcm), len(rec)))


def cmd_decode(a):
    with open(a.record, "rb") as f:
        rec = f.read()
    r = parse_record(rec)
    pcm = ima_decode(r["data"], r["sample_count"])
    write_wav(a.output, pcm)
    print("%s: slot %d %r, %d samples -> %s" % (a.record, r["slot"], r["name"], len(pcm), a.output))


def cmd_emu_keys(a):
    """fm1-emu --keys: 'T:midi:HEX' entries, START at --at, a message every --pace ms, END 200 ms after the last"""
    pcm, name = sound_from_args(a)
    rec = make_record(pcm, a.slot, name)
    digits = pack_7bit(rec)
    t = a.at
    keys = ["%d:midi:bf%02x%02x" % (t, CC_START, a.slot)]
    t += 100
    for i in range(0, len(digits), 2):
        keys.append("%d:midi:af%02x%02x" % (t, digits[i], digits[i + 1]))
        t += a.pace
    keys.append("%d:midi:bf%02x00" % (t + 200, CC_END))
    print(",".join(keys))
    print("# %d messages, END at %d ms: run with --ms %d or more" % (len(keys), t + 200, t + 1200), file=sys.stderr)


def cmd_ports(a):
    try:
        import mido
    except ImportError:
        raise ToolError(3, "mido is not installed: pip install mido python-rtmidi")
    print("outputs:", *dict.fromkeys(mido.get_output_names()), sep="\n  ")
    print("inputs:", *dict.fromkeys(mido.get_input_names()), sep="\n  ")


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__.split("\n\n")[0], formatter_class=argparse.RawDescriptionHelpFormatter,
                                epilog=__doc__.split("\n", 2)[2])
    sub = p.add_subparsers(dest="cmd", required=True)

    def sound(sp):
        sp.add_argument("wav")
        sp.add_argument("--slot", type=int, required=True, help="0..3")
        sp.add_argument("--name", help="up to 20 characters shown on the SAMPLE page (default: the file's name)")
        sp.add_argument("--start", type=float, default=0.0, help="seconds into the file")
        sp.add_argument("--length", type=float, help="seconds to take (default: what fits)")
        sp.add_argument("--gain", type=float, default=0.0, help="dB")
        sp.add_argument("--normalize", action="store_true", help="peak to -0.2 dBFS")

    def midi(sp, acks=True):
        sp.add_argument("--port", help="part of the MIDI port's name (default: the first FM-1-like one)")
        if acks:
            sp.add_argument("--no-ack", action="store_true", help="emulators: wait fixed times, no answers expected")

    s = sub.add_parser("send", help="a WAV into a slot over MIDI")
    sound(s)
    midi(s)
    s.add_argument("--burst", type=int, default=16, help="messages between pauses (default 16)")
    s.add_argument("--pace", type=float, default=1.0, help="ms pause between bursts (default 1)")
    s.set_defaults(fn=cmd_send)
    s = sub.add_parser("clear", help="empty a slot")
    s.add_argument("--slot", type=int, required=True)
    midi(s)
    s.set_defaults(fn=cmd_clear)
    s = sub.add_parser("query", help="is a slot loaded")
    s.add_argument("--slot", type=int, required=True)
    midi(s, acks=False)
    s.set_defaults(fn=cmd_query)
    s = sub.add_parser("pack", help="write the flash record instead of sending it")
    sound(s)
    s.add_argument("-o", "--output", required=True)
    s.set_defaults(fn=cmd_pack)
    s = sub.add_parser("decode", help="a record back to a 22 050 Hz WAV")
    s.add_argument("record")
    s.add_argument("-o", "--output", required=True)
    s.set_defaults(fn=cmd_decode)
    s = sub.add_parser("emu-keys", help="the transfer as fm1-emu's --keys script (no MIDI port needed)")
    sound(s)
    s.add_argument("--at", type=int, default=2000, help="ms at which START goes (default 2000: after the splash)")
    s.add_argument("--pace", type=float, default=2.0, help="ms between messages (default 2: under 31250 baud)")
    s.set_defaults(fn=cmd_emu_keys)
    s = sub.add_parser("ports", help="list MIDI ports")
    s.set_defaults(fn=cmd_ports)

    a = p.parse_args(argv)
    try:
        a.fn(a)
    except ToolError as e:
        print("error: %s" % e, file=sys.stderr)
        return e.code
    except FileNotFoundError as e:
        print("error: %s" % e, file=sys.stderr)
        return 2
    except wave.Error as e:
        print("error: not a WAV I can read: %s" % e, file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
