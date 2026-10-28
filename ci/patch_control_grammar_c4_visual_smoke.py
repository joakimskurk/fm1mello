#!/usr/bin/env python3
from pathlib import Path
import hashlib

VISUAL = Path('tools/ui_visual_smoke.c')
s = VISUAL.read_text()

old = '''    transport.engine.randomize.mask = MELLO_RAND_PITCH | MELLO_RAND_GATE;\n    mello_ui_input(&ui, 0u, 0u, (1u << MELLO_UI_BTN_OCT_DOWN), 0u, 0u, 0u, encoder_delta);\n    if (transport.engine.randomize.mask != 0u) {\n        fprintf(stderr, "RANDOM NONE shortcut failed: mask=%u\\n", (unsigned)transport.engine.randomize.mask);\n        return 19;\n    }\n    mello_ui_input(&ui, 0u, 0u, (1u << MELLO_UI_BTN_OCT_UP), 0u, 0u, 0u, encoder_delta);\n    if (transport.engine.randomize.mask != MELLO_RAND_ALL) {\n        fprintf(stderr, "RANDOM ALL shortcut failed: mask=%u\\n", (unsigned)transport.engine.randomize.mask);\n        return 20;\n    }\n'''
new = '''    /* WP-C4 moved RANDOM NONE/ALL from OCT to contextual B6/B7.\n       Resolve the physical note indices from the keyboard map instead of\n       hardcoding them, matching the control-contract helper semantics. */\n    {\n        uint8_t note, black_pos = 0u, b6 = 0xFFu, b7 = 0xFFu;\n        for (note = 0u; note < 27u; ++note) {\n            if (mello_ui_white_key_position(note) >= 0) continue;\n            if (black_pos == 6u) b6 = note;\n            if (black_pos == 7u) b7 = note;\n            ++black_pos;\n        }\n        if (b6 == 0xFFu || b7 == 0xFFu) return 31;\n        transport.engine.randomize.mask = MELLO_RAND_PITCH | MELLO_RAND_GATE;\n        mello_ui_input(&ui, 0u, 0u, 0u, 0u, 0u, (1u << b6), encoder_delta);\n        if (transport.engine.randomize.mask != 0u) {\n            fprintf(stderr, "RANDOM B6 NONE failed: mask=%u\\n", (unsigned)transport.engine.randomize.mask);\n            return 19;\n        }\n        mello_ui_input(&ui, 0u, 0u, 0u, 0u, 0u, (1u << b7), encoder_delta);\n        if (transport.engine.randomize.mask != MELLO_RAND_ALL) {\n            fprintf(stderr, "RANDOM B7 ALL failed: mask=%u\\n", (unsigned)transport.engine.randomize.mask);\n            return 20;\n        }\n    }\n'''
if s.count(old) != 1:
    raise SystemExit(f'FAIL: WP-C4 visual RANDOM NONE/ALL marker count {s.count(old)}')
s = s.replace(old, new, 1)
VISUAL.write_text(s)

if 'MELLO_UI_BTN_OCT_DOWN' in new or 'MELLO_UI_BTN_OCT_UP' in new:
    raise SystemExit('FAIL: WP-C4 visual smoke retained OCT random-mask shortcut')

print(f'PASS: WP-C4 visual smoke resolves contextual B6/B7 from the keyboard map; SHA={hashlib.sha256(VISUAL.read_bytes()).hexdigest()}')
