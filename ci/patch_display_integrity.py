#!/usr/bin/env python3
from pathlib import Path
import base64
import gzip
import hashlib
import subprocess

PATCH_B64 = Path('patches/wp-b3-display-integrity.patch.gz.b64')
UI = Path('mello/mello_ui.c')
ROTARY = Path('tools/rotary_mapping_parity.c')
TEST = Path('tools/ui_display_integrity.c')
PRE_UI_SHA = 'ab38ade0a418017e639263ccf6e1fb7580f5f39b5510ed979db5bd3b366e00b5'
POST_PATCH_UI_SHA = 'e835f009eeb2c6525a9654488422f2187b0c489e7a37b73a90c758917d380701'
FINAL_UI_SHA = 'b5824ffc4341f69547a63d5e0f281a2c19db920334b0cc5b8829bef844c7911d'
PATCH_B64_SHA = 'bc1c0e4f75a3024317b51d0f2a8d6cb18d2f0ae5eebda5b6477d661f64fdab8b'
ROTARY_PRE_SHA = '3c5ac8c5d60017dbe3bc687b10023210106edcb6a02ecf29aa0b7ad4108bf629'


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


if sha(UI) != PRE_UI_SHA:
    raise SystemExit(f'FAIL: WP-B3 UI base hash {sha(UI)} != {PRE_UI_SHA}')
if sha(PATCH_B64) != PATCH_B64_SHA:
    raise SystemExit('FAIL: WP-B3 patch transport hash')
if not TEST.is_file():
    raise SystemExit('FAIL: WP-B3 display-integrity test missing')
test_text = TEST.read_text()
for marker in ('FONT_S_ADV = 8', 'home_incremental_equivalence', 'MELLO_FEATURE_CHORD_MACHINE'):
    if marker not in test_text:
        raise SystemExit(f'FAIL: WP-B3 display-integrity test missing marker {marker!r}')
if sha(ROTARY) != ROTARY_PRE_SHA:
    raise SystemExit('FAIL: WP-B3 rotary fixture base hash')

try:
    patch_bytes = gzip.decompress(base64.b64decode(PATCH_B64.read_bytes(), validate=True))
except Exception as exc:
    raise SystemExit(f'FAIL: WP-B3 patch decode: {exc}') from exc
proc = subprocess.run(
    ['patch', '--batch', '--forward', '-p1'],
    input=patch_bytes,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if proc.returncode:
    print(proc.stdout.decode(errors='replace'), end='')
    raise SystemExit(f'FAIL: WP-B3 patch returned {proc.returncode}')
if sha(UI) != POST_PATCH_UI_SHA:
    raise SystemExit(f'FAIL: WP-B3 patch output hash {sha(UI)} != {POST_PATCH_UI_SHA}')

# Preserve WP-A's visible WAVE contract without reintroducing clipping. The
# final RANDOM target is alone on row two, so it can safely use a 34 px cell
# (32 px text box after the 1 px insets). Also keep new B3 text fitting fully
# freestanding: target firmware deliberately removes libc string helpers.
u = UI.read_text()
ui_replacements = (
    ('"CHD", "WAV"', '"CHD", "WAVE"'),
    ('"SMP", "WAV"', '"SMP", "WAVE"'),
    ('            w = 30;\n            x = (int16_t)(8 + col * 32);',
     '            w = (i == (uint8_t)(rn_count - 1u)) ? 34 : 30;\n            x = (int16_t)(8 + col * 32);'),
    ('''static void footer_pair(const MelloUiDrawOps *o, uint8_t k, const char *label, const char *value, uint16_t colour)\n{\n    int16_t x = (int16_t)(k * 60u);\n    char compact[8];\n    const char *shown = value ? value : "";\n    size_t n = strlen(shown), i;''',
     '''static uint16_t ui_text_len(const char *s)\n{\n    uint16_t n = 0u;\n    if (!s) return 0u;\n    while (s[n]) ++n;\n    return n;\n}\n\nstatic void footer_pair(const MelloUiDrawOps *o, uint8_t k, const char *label, const char *value, uint16_t colour)\n{\n    int16_t x = (int16_t)(k * 60u);\n    char compact[8];\n    const char *shown = value ? value : "";\n    uint16_t n = ui_text_len(shown), i;'''),
    ('''    MelloUiFitCtx *f = (MelloUiFitCtx *)ctx;\n    size_t n = text ? strlen(text) : 0u;\n    if (large && n * 16u > (size_t)(w > 0 ? w : 0)) large = 0u;''',
     '''    MelloUiFitCtx *f = (MelloUiFitCtx *)ctx;\n    uint16_t n = ui_text_len(text);\n    if (large && (uint32_t)n * 16u > (uint32_t)(w > 0 ? w : 0)) large = 0u;'''),
)
for old, new in ui_replacements:
    if u.count(old) != 1:
        raise SystemExit(f'FAIL: WP-B3 expected one UI marker {old!r}')
    u = u.replace(old, new)
UI.write_text(u)
if sha(UI) != FINAL_UI_SHA:
    raise SystemExit(f'FAIL: WP-B3 final UI hash {sha(UI)} != {FINAL_UI_SHA}')

s = ROTARY.read_text()
replacements = (
    ('else if(y==203)', 'else if(y==204)'),
    ('"5SEQ LEN"', '"5SEQLEN"'),
    ('"MIDI OUT"', '"NOTEOUT"'),
)
for old, new in replacements:
    if s.count(old) != 1:
        raise SystemExit(f'FAIL: WP-B3 rotary fixture expected one {old!r}')
    s = s.replace(old, new)
ROTARY.write_text(s)

print('PASS: WP-B3 display-integrity patch + full WAVE target + freestanding text fit + B2 fixture adaptation applied')
