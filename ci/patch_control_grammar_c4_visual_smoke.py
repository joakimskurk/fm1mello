#!/usr/bin/env python3
from pathlib import Path
import hashlib

VISUAL = Path('tools/ui_visual_smoke.c')
s = VISUAL.read_text()

old = '''    transport.engine.randomize.mask = MELLO_RAND_PITCH | MELLO_RAND_GATE;\n    mello_ui_input(&ui, 0u, 0u, (1u << MELLO_UI_BTN_OCT_DOWN), 0u, 0u, 0u, encoder_delta);\n    if (transport.engine.randomize.mask != 0u) {\n        fprintf(stderr, "RANDOM NONE shortcut failed: mask=%u\\n", (unsigned)transport.engine.randomize.mask);\n        return 19;\n    }\n    mello_ui_input(&ui, 0u, 0u, (1u << MELLO_UI_BTN_OCT_UP), 0u, 0u, 0u, encoder_delta);\n    if (transport.engine.randomize.mask != MELLO_RAND_ALL) {\n        fprintf(stderr, "RANDOM ALL shortcut failed: mask=%u\\n", (unsigned)transport.engine.randomize.mask);\n        return 20;\n    }\n'''
new = '''    /* WP-C4 moved RANDOM NONE/ALL from OCT to contextual B6/B7.\n       Physical black-key note indices are 15 (B6) and 18 (B7). */\n    transport.engine.randomize.mask = MELLO_RAND_PITCH | MELLO_RAND_GATE;\n    mello_ui_input(&ui, 0u, 0u, 0u, 0u, 0u, (1u << 15), encoder_delta);\n    if (transport.engine.randomize.mask != 0u) {\n        fprintf(stderr, "RANDOM B6 NONE failed: mask=%u\\n", (unsigned)transport.engine.randomize.mask);\n        return 19;\n    }\n    mello_ui_input(&ui, 0u, 0u, 0u, 0u, 0u, (1u << 18), encoder_delta);\n    if (transport.engine.randomize.mask != MELLO_RAND_ALL) {\n        fprintf(stderr, "RANDOM B7 ALL failed: mask=%u\\n", (unsigned)transport.engine.randomize.mask);\n        return 20;\n    }\n'''
if s.count(old) != 1:
    raise SystemExit(f'FAIL: WP-C4 visual RANDOM NONE/ALL marker count {s.count(old)}')
s = s.replace(old, new, 1)
VISUAL.write_text(s)

if 'MELLO_UI_BTN_OCT_DOWN' in new or 'MELLO_UI_BTN_OCT_UP' in new:
    raise SystemExit('FAIL: WP-C4 visual smoke retained OCT random-mask shortcut')

print(f'PASS: WP-C4 visual smoke uses contextual B6/B7 RANDOM mask actions; SHA={hashlib.sha256(VISUAL.read_bytes()).hexdigest()}')
