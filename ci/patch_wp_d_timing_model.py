#!/usr/bin/env python3
from pathlib import Path
import hashlib

ENGINE = Path('mello/mello_engine.c')
HEADER = Path('mello/mello_engine.h')
PRE_ENGINE_SHA = '7c9b7428920f787225c383ae4e6ba08a8e1ac5a2a95064b3d7d14ea0f8d3c64d'
PRE_HEADER_SHA = '5a431221418d3f2c90d6094303a6eab03f13757d211284381251f004d29fa2f8'
POST_ENGINE_SHA = 'b76dd8f25a1e2b218e0e32652578001cf073ca50f6198e95af9866b3cfd51937'
POST_HEADER_SHA = '769ea7e8b7a1be51d38bbd48e5eb09e39114dd79ca363dcb70b235e1483cdab8'


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replace_one(src: str, old: str, new: str, label: str) -> str:
    count = src.count(old)
    if count != 1:
        raise SystemExit(f'FAIL: WP-D D2 {label} marker count {count}')
    return src.replace(old, new, 1)


if sha(ENGINE) != PRE_ENGINE_SHA:
    raise SystemExit(f'FAIL: WP-D D2 engine base hash {sha(ENGINE)} != {PRE_ENGINE_SHA}')
if sha(HEADER) != PRE_HEADER_SHA:
    raise SystemExit(f'FAIL: WP-D D2 header base hash {sha(HEADER)} != {PRE_HEADER_SHA}')

s = HEADER.read_text()
s = replace_one(
    s,
    '    /* 0=off. Non-zero counts processed pulse entries, matching source. */\n    uint16_t master_len;',
    '    /* 0=FREE. Non-zero is musical length in 1/16-note units (16 = 1 bar of 4/4). */\n    uint16_t master_len;',
    'master_len semantic comment',
)
HEADER.write_text(s)

s = ENGINE.read_text()
s = replace_one(
    s,
    '    p->master_len=0u;',
    '    p->master_len=32u; /* WP-D: default 2 BAR = 32 sixteenth notes. */',
    '2 BAR default',
)
s = replace_one(
    s,
    '    e->loop_pulse_length=e->project.master_len>0u?(uint32_t)e->project.master_len:natural;',
    '    /* WP-D: master_len is a musical 1/16-note duration. Runtime sequence\n'
    '     * pulses are scaled by clock_div so BAR duration does not change with DIV. */\n'
    '    e->loop_pulse_length=e->project.master_len>0u\n'
    '        ? (uint32_t)e->project.master_len/(uint32_t)e->project.clock_div\n'
    '        : natural;',
    'bar-to-processed-pulse conversion',
)
ENGINE.write_text(s)

if sha(ENGINE) != POST_ENGINE_SHA:
    raise SystemExit(f'FAIL: WP-D D2 engine output hash {sha(ENGINE)} != {POST_ENGINE_SHA}')
if sha(HEADER) != POST_HEADER_SHA:
    raise SystemExit(f'FAIL: WP-D D2 header output hash {sha(HEADER)} != {POST_HEADER_SHA}')

print(f'PASS: WP-D D2 musical timing model applied; engine={sha(ENGINE)} header={sha(HEADER)}')
