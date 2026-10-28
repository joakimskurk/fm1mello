#!/usr/bin/env python3
"""WP-E1 deterministic source/behavior baseline.

This intentionally describes the pre-WP-E audio implementation.  Later WP-E
subtasks may replace these baseline expectations in the same commit that changes
sound behavior; E1 exists so changes are deliberate rather than accidental.
"""
from pathlib import Path
import hashlib
import re

ROOT = Path(__file__).resolve().parents[1]
AUDIO = (ROOT / "mello/mello_audio.c").read_text()
ENGINE = (ROOT / "mello/mello_engine.c").read_text()
UI = (ROOT / "mello/mello_ui.c").read_text()
TARGET_MEMORY = (ROOT / "ci/patch_target_memory.py").read_text()


def need(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"FAIL: WP-E1 baseline missing {label}: {needle!r}")


def drive(amount: int, character: int) -> int:
    a = min(max(amount, 0), 1000)
    c = min(max(character, 0), 1000)
    a2 = (a * a + 500) // 1000
    a19 = a2 + (a - a2 + 5) // 10
    factor = 280 + (c * 520 + 500) // 1000
    return min((a19 * factor + 500) // 1000, 800)


# Distortion: current CHARACTER only bends the amount->drive mapping.  The
# waveshaper itself receives character=0, and derived drive is capped at 800.
need(AUDIO, "return (uint16_t)(drive>800u?800u:drive);", "distortion drive cap")
need(AUDIO, "soft_drive(mix,a->dist_drive_permille,0u)", "derived distortion call")
if drive(500, 350) != 127 or drive(1000, 1000) != 800:
    raise SystemExit("FAIL: WP-E1 mirrored distortion curve changed")
max_gain_q15 = 32768 + 800 * 12
max_knee = 32767 - 800 * 8
if max_gain_q15 != 42368 or max_knee != 26367:
    raise SystemExit("FAIL: WP-E1 distortion endpoint calculation changed")

# Reverb V2: 2 all-pass stages into a four-line static Hadamard FDN, one fixed
# damping coefficient and one mono late-return combination.
need(AUDIO, "rv2_allpass(a,0u,x,18022u)", "Reverb V2 all-pass 0")
need(AUDIO, "rv2_allpass(a,1u,x,20890u)", "Reverb V2 all-pass 1")
need(AUDIO, "static const uint16_t full_line[4]", "four-line FDN")
need(AUDIO, "const uint16_t damp_alpha=14755u", "fixed Reverb V2 damping")
need(AUDIO, "rv=(y[0]+y[1]-y[2]-y[3])/2", "mono Reverb V2 return")
need(TARGET_MEMORY, "MELLO_AUDIO_REVERB_SAMPLES 24576u", "target reverb ring budget")
need(TARGET_MEMORY, "full layout uses 23173 samples", "Reverb V2 full-layout budget")

# Ratchets: every sub-trigger emits its own NOTE_ON/OFF pair.  The audio event
# dispatcher sends each NOTE_ON through lead_attack(), so envelope attack is
# re-entered for every ratchet rather than only on the first sub-trigger.
ratchet = re.search(
    r"for\(r=0u;r<ratchets;\+\+r\)\{[^{}]*emit_note_pair\([^{}]*\);\}",
    ENGINE,
)
if not ratchet:
    raise SystemExit("FAIL: WP-E1 ratchet no longer emits one note pair per sub-trigger")
need(AUDIO, "else if(e->type==MELLO_EVENT_NOTE_ON)", "NOTE_ON dispatcher")
need(AUDIO, "lead_attack(a,e->note,e->velocity,e->slide)", "lead envelope retrigger")

# Live config publication is coherent (WP-B), but FX values themselves are
# currently swapped as a block.  No dedicated continuous FX smoothing layer is
# present yet; E4 is expected to change this baseline deliberately.
need(AUDIO, "a->fx=a->pending_fx;", "block-atomic FX publication")
need(AUDIO, "reverb_changed=a->fx.reverb_pre_ms!=a->pending_fx.reverb_pre_ms", "selective reverb invalidation")
need(AUDIO, "dist_changed=a->fx.dist_amount!=a->pending_fx.dist_amount", "selective distortion derivation")
need(AUDIO, "reverb_wet_changed=a->fx.reverb_wet!=a->pending_fx.reverb_wet", "selective reverb-send derivation")

# Physical UI contract used by E2/E3/E4.
for label in ("AMT", "MIX", "CHAR", "TONE", "WET", "SIZE", "DECAY", "PRE", "LEVEL"):
    need(UI, f'\"{label}\"', f"FX UI label {label}")

for path in ("mello_audio.c", "mello_engine.c", "mello_ui.c"):
    p = ROOT / "mello" / path
    print(f"SHA {path} {hashlib.sha256(p.read_bytes()).hexdigest()}")

print(
    "WP-E1 distortion baseline: drive(500,350)=127, drive(1000,1000)=800, "
    "max pre-knee gain=42368/32768, max knee=26367"
)
print(
    "WP-E1 reverb baseline: 2x all-pass + static 4-line Hadamard FDN, fixed damping, "
    "mono late return, target ring=24576 samples / V2 layout=23173"
)
print("WP-E1 ratchet baseline: one NOTE_ON/OFF pair per sub-trigger; each NOTE_ON re-enters lead_attack")
print("KNOWN GAP: no dedicated continuous smoothing layer for live FX parameter changes")
print("KNOWN GAP: Reverb V2 has no late-tail delay modulation and exposes a mono late return")
print("PASS: WP-E1 deterministic pre-change audio baseline")
