#!/usr/bin/env python3
from pathlib import Path
import hashlib

ROOT = Path(__file__).resolve().parents[1]
C = ROOT / "mello" / "mello_ui.c"
H = ROOT / "mello" / "mello_ui.h"
EXPECTED_C = "2626bfd4a417805c99077dc6dc0c5348a5654935abbd96ecc4b164572b462fb1"
EXPECTED_H = "cb36b0d140822989c77ef11c9286b2167e05d251514df0837d427ee3a387bbbd"

def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def req_replace(s, old, new):
    if old not in s:
        raise SystemExit(f"UI semantics patch precondition missing: {old[:80]!r}")
    return s.replace(old, new)

if digest(C) != EXPECTED_C or digest(H) != EXPECTED_H:
    raise SystemExit("UI semantics patch source hash mismatch; rebase transform against current visual-port source")

s = C.read_text()
s = req_replace(s, '''#define UI_PURPLE UI_RGB(204,85,255)\n#define UI_CORAL UI_RGB(255,119,85)\n#define UI_BLUE UI_RGB(68,187,255)\n#define UI_RED UI_RGB(255,68,68)\n''', '''#define UI_PURPLE UI_RGB(177,92,255) /* generative / random */\n#define UI_MAGENTA UI_RGB(255,72,184) /* ratchet / repeat */\n#define UI_CORAL UI_RGB(255,119,85)   /* accent / emphasis */\n#define UI_BLUE UI_RGB(68,187,255)    /* movement / modulation */\n#define UI_SOFT UI_RGB(142,184,158)   /* neutral edit selection */\n#define UI_RED UI_RGB(255,68,68)\n\n/* Structural colours: segment identity is deliberately softer than function colours. */\n#define UI_SEG_A UI_RGB(72,196,126)\n#define UI_SEG_B UI_RGB(225,171,72)\n#define UI_SEG_C UI_RGB(92,146,220)\n#define UI_SEG_D UI_RGB(174,105,210)\n#define UI_SEG_E UI_RGB(146,184,82)\n#define UI_SEG_F UI_RGB(222,113,76)\n#define UI_SEG_A_DIM UI_RGB(28,77,49)\n#define UI_SEG_B_DIM UI_RGB(84,64,28)\n#define UI_SEG_C_DIM UI_RGB(35,57,84)\n#define UI_SEG_D_DIM UI_RGB(66,41,80)\n#define UI_SEG_E_DIM UI_RGB(55,70,32)\n#define UI_SEG_F_DIM UI_RGB(82,43,30)\n''')

start = s.index('static void home_pitch_range(')
end = s.index('\nstatic void draw_home_step', start)
s = s[:start] + '''static uint8_t ui_segment_for_step(const MelloProject *p, uint8_t step)\n{\n    uint8_t bounds[MELLO_MAX_SEGMENTS + 1u], sc, i;\n    sc = ui_segment_bounds(p, bounds);\n    for (i = 0u; i < sc; ++i) if (step >= bounds[i] && step < bounds[i + 1u]) return i;\n    return 0u;\n}\n\nstatic uint16_t ui_segment_colour(uint8_t segment)\n{\n    static const uint16_t c[MELLO_MAX_SEGMENTS] = { UI_SEG_A, UI_SEG_B, UI_SEG_C, UI_SEG_D, UI_SEG_E, UI_SEG_F };\n    return c[segment < MELLO_MAX_SEGMENTS ? segment : 0u];\n}\n\nstatic uint16_t ui_segment_colour_dim(uint8_t segment)\n{\n    static const uint16_t c[MELLO_MAX_SEGMENTS] = { UI_SEG_A_DIM, UI_SEG_B_DIM, UI_SEG_C_DIM, UI_SEG_D_DIM, UI_SEG_E_DIM, UI_SEG_F_DIM };\n    return c[segment < MELLO_MAX_SEGMENTS ? segment : 0u];\n}\n\nstatic uint16_t ui_dot_colour(uint8_t target)\n{\n    static const uint16_t c[4] = { UI_CORAL, UI_CYAN, UI_MAGENTA, UI_GOLD };\n    return c[target < 4u ? target : 0u];\n}\n\nstatic uint16_t ui_page_accent(MelloUi *ui)\n{\n    MelloEngine *e = eng(ui);\n    switch (ui->page) {\n    case MELLO_UI_SEQ_SEGMENT: return ui_segment_colour(ui->selected_segment);\n    case MELLO_UI_SEQ_LOOP:\n    case MELLO_UI_SUB: return UI_GOLD;\n    case MELLO_UI_SEQ_SLOTSEQ:\n    case MELLO_UI_SLOTS: return UI_BLUE;\n    case MELLO_UI_RANDOM: return UI_PURPLE;\n    case MELLO_UI_DOT: return ui_dot_colour(ui->selected_dot_target);\n    case MELLO_UI_TURING: return UI_BLUE;\n    case MELLO_UI_AUDIO_FX: return ui->fx_subpage == 0u ? UI_CORAL : (ui->fx_subpage <= 2u ? UI_BLUE : UI_PURPLE);\n    case MELLO_UI_SOUND_RESERVED: return ui->sound_subpage == 0u ? UI_CYAN : (ui->sound_subpage == 2u ? UI_BLUE : (ui->sound_subpage == 3u ? UI_GOLD : UI_ACC));\n    case MELLO_UI_SAMPLE_RESERVED: return UI_BLUE;\n    case MELLO_UI_HOME:\n    case MELLO_UI_SEQ_STEP:\n        if (e) return ui_segment_colour(ui_segment_for_step(&e->project, ui->selected_step));\n        return UI_ACC;\n    case MELLO_UI_GLOBAL: return UI_SOFT;\n    default: return UI_ACC;\n    }\n}\n\nstatic void home_pitch_range(const MelloProject *p, uint8_t *lo, uint8_t *hi)\n{\n    uint8_t i, n = p->seq_len > 16u ? 16u : p->seq_len;\n    uint8_t mn = 108u, mx = 12u, desired;\n    int16_t center, vlo, vhi;\n    if (!n) { *lo = 48u; *hi = 72u; return; }\n    for (i = 0u; i < n; ++i) {\n        uint8_t m = mello_resolve_step_midi(p, &p->steps[i]);\n        if (m < mn) mn = m;\n        if (m > mx) mx = m;\n    }\n    desired = (uint8_t)((p->pitch_span_oct < 1u ? 1u : (p->pitch_span_oct > 3u ? 3u : p->pitch_span_oct)) * 12u);\n    if ((uint8_t)(mx - mn) < desired) {\n        center = (int16_t)(((uint16_t)mn + (uint16_t)mx) / 2u);\n        vlo = (int16_t)(center - desired / 2);\n        vhi = (int16_t)(vlo + desired);\n        if (vlo < 12) { vhi += (int16_t)(12 - vlo); vlo = 12; }\n        if (vhi > 108) { vlo -= (int16_t)(vhi - 108); vhi = 108; if (vlo < 12) vlo = 12; }\n        mn = (uint8_t)vlo; mx = (uint8_t)vhi;\n    } else {\n        if (mn > 12u) --mn;\n        if (mx < 108u) ++mx;\n    }\n    if (mx <= mn) { mn = 48u; mx = 72u; }\n    *lo = mn; *hi = mx;\n}\n''' + s[end:]

start = s.index('static void draw_home_step(')
end = s.index('\nstatic void draw_home(', start)
s = s[:start] + '''static void draw_home_step(MelloUi *ui, const MelloUiDrawOps *o, uint8_t i)\n{\n    MelloEngine *e = eng(ui);\n    MelloStep *s;\n    uint8_t lo, hi, midi, active, selected, segment, pulse_w;\n    int16_t x, base_y = 143, top_y, h;\n    uint16_t bar;\n    if (!e || i >= 16u) return;\n    x = (int16_t)(8 + i * 14u);\n    o->fill(o->ctx, x, 47, 12, 111, UI_BLACK);\n    if (i >= e->project.seq_len) { ui_frame(o, x + 1, 137, 10, 7, UI_PANEL2); return; }\n    s = &e->project.steps[i];\n    segment = ui_segment_for_step(&e->project, i);\n    bar = s->skip ? ui_segment_colour_dim(segment) : ui_segment_colour(segment);\n    home_pitch_range(&e->project, &lo, &hi);\n    midi = mello_resolve_step_midi(&e->project, s);\n    h = (int16_t)(14 + ((int32_t)(midi - lo) * 72) / (hi > lo ? (int32_t)(hi - lo) : 1));\n    if (h > 86) h = 86;\n    top_y = (int16_t)(base_y - h);\n    active = ui->transport->display_step == i;\n    selected = ui->selected_step == i;\n    o->fill(o->ctx, x + 1, 51, 10, 93, UI_PANEL);\n    o->fill(o->ctx, x + 2, top_y, 8, h, bar);\n    if (!s->skip) o->fill(o->ctx, x + 2, top_y, 8, 2, bar);\n    if (active) ui_frame(o, x, (int16_t)(top_y - 2), 12, (int16_t)(h + 4), UI_WHITE);\n    else if (selected) ui_frame(o, x, (int16_t)(top_y - 2), 12, (int16_t)(h + 4), UI_SOFT);\n    pulse_w = s->pulses <= 4u ? (uint8_t)(s->pulses ? s->pulses * 3u - 1u : 0u) : 10u;\n    if (pulse_w) o->fill(o->ctx, x + 1, 148, pulse_w, 2, UI_SOFT);\n    if (s->ratchet > 1u) o->fill(o->ctx, x + 1, 153, 2, 2, UI_MAGENTA);\n    if (s->accent) o->fill(o->ctx, x + 4, 153, 2, 2, UI_CORAL);\n    if (s->sub) o->fill(o->ctx, x + 7, 153, 2, 2, UI_GOLD);\n    if (s->slide) o->fill(o->ctx, x + 10, 153, 2, 2, UI_BLUE);\n}\n''' + s[end:]

for old, new in [
('uint8_t i, bounds[MELLO_MAX_SEGMENTS + 1u], sc;','uint8_t i, bounds[MELLO_MAX_SEGMENTS + 1u], sc, focus_seg;'),
('sc = ui_segment_bounds(&e->project, bounds);\n    for (i = 0u; i < sc; ++i) {','sc = ui_segment_bounds(&e->project, bounds);\n    focus_seg = ui_segment_for_step(&e->project, ui->selected_step);\n    for (i = 0u; i < sc; ++i) {'),
('c = i == ui->selected_segment ? UI_ACC : UI_DIM;','c = ui_segment_colour(i);'),
('o->fill(o->ctx, x, 38, (int16_t)(x2 - x + 1), 1, c);','o->fill(o->ctx, x, 38, (int16_t)(x2 - x + 1), i == focus_seg ? 2 : 1, c);'),
('o->text(o->ctx, x, 31, (int16_t)(x2 - x + 1), 0, b, c, 1);','o->text(o->ctx, x, 31, (int16_t)(x2 - x + 1), 0, b, i == focus_seg ? UI_WHITE : c, 1);'),
]: s = req_replace(s, old, new)

start = s.index('static void draw_header(')
end = s.index('\nstatic void draw_body', start)
s = s[:start] + '''static void draw_header(MelloUi *ui, const MelloUiDrawOps *o)\n{\n    MelloEngine *e = eng(ui);\n    char b[32], *p;\n    uint16_t accent = ui_page_accent(ui);\n    o->fill(o->ctx, 0, 0, 240, 30, UI_PANEL);\n    o->text(o->ctx, 7, 3, 92, 0, "SKURKYMELLO", UI_WHITE, 0);\n    o->text(o->ctx, 102, 3, 131, 0, mello_ui_page_name(ui->page), accent, 2);\n    if (e && (ui->page == MELLO_UI_HOME || ui->page == MELLO_UI_SEQ_STEP || ui->page == MELLO_UI_SEQ_SEGMENT)) {\n        uint8_t seg = ui->page == MELLO_UI_SEQ_SEGMENT ? ui->selected_segment : ui_segment_for_step(&e->project, ui->selected_step);\n        p = copy_s(b, "SEG "); *p++ = (char)('A' + (seg < MELLO_MAX_SEGMENTS ? seg : 0u)); *p = 0;\n        o->text(o->ctx, 7, 17, 58, 0, b, ui_segment_colour(seg), 0);\n    }\n    p = copy_s(b, ui->transport->running ? "RUN " : "STOP ");\n    p = put_u32(p, mello_transport_effective_bpm(ui->transport));\n    copy_s(p, " BPM");\n    o->text(o->ctx, 126, 17, 107, 0, b, ui->transport->running ? UI_ACC : UI_SOFT, 2);\n    o->fill(o->ctx, 7, 28, 226, 1, accent);\n}\n''' + s[end:]

repls = [
('if (si < e->project.seq_len) o->fill(o->ctx, x + 2, 42, 8, 2, e->project.steps[si].skip ? UI_DIM : UI_ACC);\n            if (si == ui->selected_step) ui_frame(o, x, 35, 12, 12, UI_WHITE);\n            if (ui->transport->display_step == si) ui_frame(o, x + 1, 36, 10, 10, UI_CYAN);', 'if (si < e->project.seq_len) { uint8_t seg = ui_segment_for_step(&e->project, si); o->fill(o->ctx, x + 2, 42, 8, 2, e->project.steps[si].skip ? ui_segment_colour_dim(seg) : ui_segment_colour(seg)); }\n            if (si == ui->selected_step) ui_frame(o, x, 35, 12, 12, UI_SOFT);\n            if (ui->transport->display_step == si) ui_frame(o, x + 1, 36, 10, 10, UI_WHITE);'),
('uint16_t c = i < s->ratchet ? UI_PURPLE : UI_PANEL2;','uint16_t c = i < s->ratchet ? UI_MAGENTA : UI_PANEL2;'),
('uint16_t c = i == (uint8_t)s->gate ? UI_CYAN : UI_PANEL2;','uint16_t c = i == (uint8_t)s->gate ? UI_ACC : UI_PANEL2;'),
('uint16_t fc[4] = { UI_CYAN, UI_CORAL, UI_GOLD, UI_RED };','uint16_t fc[4] = { UI_BLUE, UI_CORAL, UI_GOLD, UI_RED };'),
('s->sample_slot ? UI_CYAN : UI_DIM','s->sample_slot ? UI_BLUE : UI_DIM'),
('uint16_t c = i == s ? UI_ACC : UI_DIM;','uint16_t c = ui_segment_colour(i);'),
('if (i < e->project.seq_len) o->fill(o->ctx, x + 2, 53, 8, 2, seg == s ? UI_ACC : UI_DIM);\n            if (seg == s) ui_frame(o, x, 43, 12, 16, UI_ACC);\n            if (ui->transport->display_step == i) ui_frame(o, x + 1, 44, 10, 14, UI_CYAN);','if (i < e->project.seq_len && seg != 0xFFu) o->fill(o->ctx, x + 2, 53, 8, 2, ui_segment_colour(seg));\n            if (seg == s) ui_frame(o, x, 43, 12, 16, UI_SOFT);\n            if (ui->transport->display_step == i) ui_frame(o, x + 1, 44, 10, 14, UI_WHITE);'),
('o->text(o->ctx, 55, 66, 20, 0, b, UI_ACC, 1);','o->text(o->ctx, 55, 66, 20, 0, b, ui_segment_colour(s), 1);'),
('o->text(o->ctx, 84, 66, 40, 0, a, UI_GOLD, 0);','o->text(o->ctx, 84, 66, 40, 0, a, UI_MAGENTA, 0);'),
('e->project.seg_direction[s] == MELLO_SEG_REV ? "REV" : "FWD", UI_CYAN','e->project.seg_direction[s] == MELLO_SEG_REV ? "REV" : "FWD", UI_BLUE'),
('seg_mode_name(e->project.seg_mode[s]), UI_PURPLE','seg_mode_name(e->project.seg_mode[s]), e->project.seg_mode[s] == MELLO_SEG_RANDOM ? UI_PURPLE : UI_BLUE'),
('i + 1u == e->project.seg_repeats[s] ? UI_GOLD : UI_PANEL2','i + 1u == e->project.seg_repeats[s] ? UI_MAGENTA : UI_PANEL2'),
('i + 1u == e->project.seg_repeats[s] ? UI_GOLD : UI_DIM','i + 1u == e->project.seg_repeats[s] ? UI_MAGENTA : UI_DIM'),
('MELLO_SEG_FWD ? UI_CYAN : UI_PANEL2','MELLO_SEG_FWD ? UI_BLUE : UI_PANEL2'),
('MELLO_SEG_REV ? UI_CYAN : UI_PANEL2','MELLO_SEG_REV ? UI_BLUE : UI_PANEL2'),
('MELLO_SEG_FWD ? UI_CYAN : UI_DIM','MELLO_SEG_FWD ? UI_BLUE : UI_DIM'),
('MELLO_SEG_REV ? UI_CYAN : UI_DIM','MELLO_SEG_REV ? UI_BLUE : UI_DIM'),
('ui_frame(o, x, 109, 38, 16, i == (uint8_t)e->project.seg_mode[s] ? UI_PURPLE : UI_PANEL2);\n                o->text(o->ctx, x + 2, 113, 34, 0, mn[i], i == (uint8_t)e->project.seg_mode[s] ? UI_PURPLE : UI_DIM, 1);','uint16_t mc = i == 2u ? UI_PURPLE : UI_BLUE;\n                ui_frame(o, x, 109, 38, 16, i == (uint8_t)e->project.seg_mode[s] ? mc : UI_PANEL2);\n                o->text(o->ctx, x + 2, 113, 34, 0, mn[i], i == (uint8_t)e->project.seg_mode[s] ? mc : UI_DIM, 1);'),
('ui->segment_subpage ? UI_GOLD : UI_ACC','ui->segment_subpage ? UI_GOLD : ui_segment_colour(s)'),
('ls->octave > 0 ? 55 : 118, 17, 2, UI_CYAN','ls->octave > 0 ? 55 : 118, 17, 2, UI_BLUE'),
('o->text(o->ctx, 176, 146, 56, 0, b, UI_CYAN, 2);','o->text(o->ctx, 176, 146, 56, 0, b, UI_BLUE, 2);'),
('e->slot_seq.active?UI_CYAN:UI_DIM','e->slot_seq.active?UI_BLUE:UI_DIM'),
('uint16_t col=ss->slot<0?UI_DIM:UI_CYAN;','uint16_t col=ss->slot<0?UI_DIM:UI_BLUE;'),
('active?UI_CYAN:(selected?UI_WHITE','active?UI_WHITE:(selected?UI_SOFT'),
('shade==0u?UI_ACC:shade==1u?UI_CYAN:shade==2u?UI_PURPLE:UI_GOLD','shade==0u?UI_ACC:shade==1u?UI_BLUE:shade==2u?UI_PURPLE:UI_GOLD'),
('a,s->slot<0?UI_DIM:UI_CYAN,0)','a,s->slot<0?UI_DIM:UI_BLUE,0)'),
('static const uint16_t rc[11] = { UI_ACC, UI_ACC, UI_PURPLE, UI_CYAN, UI_GOLD, UI_CORAL, UI_GOLD, UI_CYAN, UI_RED, UI_PURPLE, UI_BLUE };','static const uint16_t rc[11] = { UI_ACC, UI_ACC, UI_MAGENTA, UI_ACC, UI_BLUE, UI_CORAL, UI_GOLD, UI_BLUE, UI_RED, UI_PURPLE, UI_SOFT };'),
('"SPAN",e->project.pitch_span_oct);o->text(o->ctx,8,132,58,0,a,UI_CYAN,0)','"SPAN",e->project.pitch_span_oct);o->text(o->ctx,8,132,58,0,a,UI_BLUE,0)'),
('"VISIBLE",e->visible_slot_count);o->text(o->ctx,70,33,88,0,a,UI_CYAN,1)','"VISIBLE",e->visible_slot_count);o->text(o->ctx,70,33,88,0,a,UI_BLUE,1)'),
('queued?UI_CYAN:','queued?UI_BLUE:'),
('static const uint16_t tc[4] = { UI_CORAL, UI_CYAN, UI_PURPLE, UI_GOLD };','static const uint16_t tc[4] = { UI_CORAL, UI_CYAN, UI_MAGENTA, UI_GOLD };'),
('c->follow ? UI_CYAN : UI_DIM','c->follow ? UI_BLUE : UI_DIM'),
('e->tm.on ? UI_CYAN : UI_DIM','e->tm.on ? UI_BLUE : UI_DIM'),
('active ? UI_CYAN : (idx == ui->selected_tm_bit ? UI_WHITE : UI_PANEL2)','active ? UI_WHITE : (idx == ui->selected_tm_bit ? UI_SOFT : UI_PANEL2)'),
('17, h, UI_ACC);\n            o->fill(o->ctx, x + 3, 130, 17, 4, gate ? UI_CYAN','17, h, UI_BLUE);\n            o->fill(o->ctx, x + 3, 130, 17, 4, gate ? UI_ACC'),
('active ? UI_CYAN : UI_DIM','active ? UI_WHITE : UI_DIM'),
('e->tm.mutation_prob, 500u, UI_ACC','e->tm.mutation_prob, 500u, UI_PURPLE'),
('e->tm.mutation_prob / 10u); o->text(o->ctx, 141, 154, 28, 0, a, UI_ACC','e->tm.mutation_prob / 10u); o->text(o->ctx, 141, 154, 28, 0, a, UI_PURPLE'),
('/* X-SUB is a five-cell yellow sequencer. Disabled cells beyond X LEN stay physically present. */','/* SUB 5-SEQ is a compact 2/3/5-step yellow sequencer. Disabled cells remain physically present. */'),
('e->sub.x_on ? "X-SUB" : "X OFF"','e->sub.x_on ? "SUB 5-SEQ" : "5-SEQ OFF"'),
('xs->octave > 0 ? 108 : 142, 32, 2, UI_CYAN','xs->octave > 0 ? 108 : 142, 32, 2, UI_BLUE'),
('ui->sub_subpage == 1u ? "X CONFIG" : ui->sub_subpage == 2u ? "X STEP"','ui->sub_subpage == 1u ? "5-SEQ CFG" : ui->sub_subpage == 2u ? "5-SEQ STEP"'),
('sync_name(ui->transport->sync_mode),UI_CYAN,2','sync_name(ui->transport->sync_mode),UI_BLUE,2'),
('"DIV",e->project.clock_div);o->text(o->ctx,8,104,60,0,a,UI_CYAN,0)','"DIV",e->project.clock_div);o->text(o->ctx,8,104,60,0,a,UI_BLUE,0)'),
('ui->transport->midi_fixed_length?UI_CYAN:UI_DIM','ui->transport->midi_fixed_length?UI_BLUE:UI_DIM'),
('2000u,UI_CYAN);','2000u,UI_BLUE);'),
('e->sound.follower_sub_octave); o->text(o->ctx,164,82,68,0,a,UI_CYAN,2)','e->sound.follower_sub_octave); o->text(o->ctx,164,82,68,0,a,UI_BLUE,2)'),
('e->sound.filter_res,1000u,UI_PURPLE','e->sound.filter_res,1000u,UI_CYAN'),
('e->sound.slide_ms,300u,UI_CYAN','e->sound.slide_ms,300u,UI_BLUE'),
('uint16_t cols[3]={UI_CORAL,UI_CYAN,UI_PURPLE};','uint16_t cols[3]={UI_CORAL,UI_BLUE,UI_PURPLE};'),
('a,UI_CYAN,0);o->text(o->ctx,120,142,112,0,b,UI_MID,2);','a,UI_BLUE,0);o->text(o->ctx,120,142,112,0,b,UI_MID,2);'),
('e->fx.delay_sync?delay_div_name(e->fx.delay_div):"FREE",UI_CYAN,0','e->fx.delay_sync?delay_div_name(e->fx.delay_div):"FREE",UI_BLUE,0'),
('o->text(o->ctx,162,80,70,0,a,UI_CYAN,2);','o->text(o->ctx,162,80,70,0,a,UI_BLUE,2);'),
('o->fill(o->ctx,center,101,2,36,UI_CYAN);','o->fill(o->ctx,center,101,2,36,UI_BLUE);'),
('o->text(o->ctx,64,139,56,0,"TRACK",UI_CYAN,0);','o->text(o->ctx,64,139,56,0,"TRACK",UI_BLUE,0);'),
('l[0]="X ON";','l[0]="5SEQ ON";'), ('l[1]="X LEN";','l[1]="5SEQ LEN";'),
]
for old, new in repls: s = req_replace(s, old, new)

old = '    for (k=0u;k<4u;++k) footer_pair(o,k,l[k],v[k],k==0u?UI_ACC:k==1u?UI_CYAN:k==2u?UI_PURPLE:UI_GOLD);\n'
new = '''    for (k=0u;k<4u;++k) {\n        uint16_t c = ui_page_accent(ui);\n        if (ui->page == MELLO_UI_TURING && ui->tm_subpage >= 1u) {\n            static const uint16_t route_col[4] = { UI_ACC, UI_CYAN, UI_BLUE, UI_CORAL };\n            c = route_col[k];\n        }\n        footer_pair(o,k,l[k],v[k],c);\n    }\n'''
s = req_replace(s, old, new)

# Cyan is intentionally reserved to filter semantics after this transform.
C.write_text(s)
h = H.read_text()
h = req_replace(h, 'uint8_t sub_subpage;        /* MAIN, XCFG, XSTEP */', 'uint8_t sub_subpage;        /* MAIN, 5-SEQ CFG, 5-SEQ STEP */')
H.write_text(h)
print('PASS: semantic UI palette, segmented skyline and SUB 5-SEQ naming applied')
