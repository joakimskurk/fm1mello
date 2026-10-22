#!/usr/bin/env python3
from pathlib import Path
import hashlib, math, re

ROOT=Path(__file__).resolve().parents[1]
C=ROOT/'mello'/'mello_audio.c'
H=ROOT/'mello'/'mello_audio.h'
EXPECTED_C='cee983f6bb399e57cdea8c4ad70715d7faf61f27319df3b9ee51a415de4886db'
EXPECTED_H='56ee83c3e64592ed5e8cde74adefefeecd03b25dc69b5cc98770faf82f4faf92'
FINAL_C='2b6cf93f1ae16517aab7a51c1cca716de99d2ee0e33111881bd9f3aec374a498'
FINAL_H='04c12ca9c47ff535f15a8be63761228754ff4af21b83e8c231717c6cf919670f'

def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def req(s,old,new):
    if old not in s: raise SystemExit('audio parity precondition missing: '+repr(old[:100]))
    return s.replace(old,new,1)
def alpha_q15(fc):
    a=1-math.exp(-2*math.pi*min(fc,20000.0)/44100.0)
    return max(1,min(32767,round(a*32767)))
def freq_table(base,mult,comp=1.0):
    return [alpha_q15(base*(mult**(i/100))*comp) for i in range(101)]
def carr(name,vals,ctype='uint16_t'):
    lines=[]
    for i in range(0,101,10): lines.append('    '+', '.join(str(x) for x in vals[i:i+10])+',')
    return f'static const {ctype} {name}[101] = {{\n'+'\n'.join(lines)+'\n};\n'

if digest(C)!=EXPECTED_C or digest(H)!=EXPECTED_H:
    raise SystemExit('audio parity source hash mismatch; rebase transform')

s=C.read_text()
old_table=re.search(r'\nstatic const uint16_t FILTER_ALPHA_Q15\[101\] = \{\n.*?\n\};\n',s,re.S)
if not old_table: raise SystemExit('legacy filter table missing')
comp=1/math.sqrt(2**(1/4)-1)
reverb_send=[]; tm_off=[]
for i in range(101):
    w=i/100
    reverb_send.append(round((0 if w<=0 else min(1.05,0.03+(w**1.2)*0.95))*1000))
    m=-0.25+0.005*i
    mult=max(0.01,1+4*m)
    tm_off.append(max(-1000,min(200,round(math.log(mult)/math.log(200)*1000))))
tables='\n/* Browser-parity control curves. */\n'
tables+=carr('MAIN_FILTER_ALPHA_Q15',freq_table(80,200,comp))+'\n'
tables+=carr('SUB_FILTER_ALPHA_Q15',freq_table(40,55))+'\n'
tables+=carr('DIST_TONE_ALPHA_Q15',freq_table(320,38))+'\n'
tables+=carr('DELAY_TONE_ALPHA_Q15',freq_table(450,26))+'\n'
tables+=carr('REVERB_TONE_ALPHA_Q15',freq_table(700,18))+'\n'
tables+=carr('REVERB_SEND_PERMILLE',reverb_send)+'\n'
tables+=carr('TM_FILTER_OFFSET_PERMILLE',tm_off,'int16_t')
s=s[:old_table.start()]+tables+s[old_table.end():]

helpers='''\nstatic uint16_t browser_dist_drive(uint16_t amount,uint16_t character){
    uint32_t a=amount>1000u?1000u:amount,c=character>1000u?1000u:character;
    uint32_t a2=(a*a+500u)/1000u;
    uint32_t a19=a2+(a-a2+5u)/10u;
    uint32_t factor=280u+(c*520u+500u)/1000u;
    uint32_t drive=(a19*factor+500u)/1000u;
    return (uint16_t)(drive>800u?800u:drive);
}
static uint16_t reverb_feedback_q15(uint32_t loop_samples,uint16_t decay_ms){
    uint32_t d=decay_ms<400u?400u:decay_ms;
    uint32_t x=(uint32_t)(((uint64_t)5133u*loop_samples+d/2u)/d);
    uint32_t x2=(uint32_t)(((uint64_t)x*x+32768u)/(2u*32768u));
    uint32_t den=32768u+x+x2;
    uint32_t fb=(uint32_t)(((uint64_t)32768u*32768u+den/2u)/den);
    if(fb>32604u)fb=32604u;
    if(fb<4096u)fb=4096u;
    return (uint16_t)fb;
}
static int32_t limiter_minus_1db(int32_t x){
    const int32_t ceiling=29204;
    return clamp32(x,-ceiling,ceiling);
}
'''
idx=s.index('static void apply_pending(')
s=s[:idx]+helpers+s[idx:]

m=re.search(r'static void apply_pending\(MelloAudio \*a\)\{.*?\}\nvoid mello_audio_init',s,re.S)
if not m: raise SystemExit('apply_pending block missing')
apply='''static void apply_pending(MelloAudio *a){
    uint16_t g,old_decay;
    uint32_t pre,avg_delay;
    if(!a)return;
    g=a->config_generation;
    if(g==a->applied_generation)return;
    old_decay=a->config.decay_ms;
    a->config=a->pending_config;
    a->fx=a->pending_fx;
    a->sub_config=a->pending_sub_config;
    a->sample_config=a->pending_sample_config;
    if(a->effective_decay_ms==0u||a->config.decay_ms!=old_decay)a->effective_decay_ms=a->config.decay_ms;
    a->lfo_inc=((uint32_t)a->config.lfo_rate_millihz*974u+5u)/10u;
    if(a->delay_capacity){
        uint32_t ms=a->fx.delay_time_ms;
        a->delay_samples=((uint32_t)ms*441u+5u)/10u;
        if(a->delay_samples<1u)a->delay_samples=1u;
        if(a->delay_samples>=a->delay_capacity)a->delay_samples=a->delay_capacity-1u;
    }
    pre=ms_samples(a->fx.reverb_pre_ms);
    if(a->reverb_capacity){
        a->reverb_delay_a=(pre+661u+(uint32_t)a->fx.reverb_size*2u)%a->reverb_capacity;
        a->reverb_delay_b=(pre+1091u+(uint32_t)a->fx.reverb_size*3u)%a->reverb_capacity;
    }else{
        a->reverb_delay_a=0u;a->reverb_delay_b=0u;
    }
    if(a->reverb_delay_a<311u)a->reverb_delay_a=311u;
    if(a->reverb_delay_b<701u)a->reverb_delay_b=701u;
    avg_delay=(a->reverb_delay_a+a->reverb_delay_b)/2u;
    a->reverb_feedback_q15=reverb_feedback_q15(avg_delay,a->fx.reverb_decay_ms);
    a->dist_drive_permille=browser_dist_drive(a->fx.dist_amount,a->fx.dist_character);
    a->reverb_send_permille=REVERB_SEND_PERMILLE[(a->fx.reverb_wet>1000u?1000u:a->fx.reverb_wet)/10u];
    a->applied_generation=g;
}
void mello_audio_init'''
s=s[:m.start()]+apply+s[m.end():]

pairs=[
('else if(a->config.lfo_target==MELLO_LFO_FILTER){int32_t cv=(int32_t)cut+((lfo*(int32_t)a->config.lfo_depth)/1000)*450/32767;cut=(uint16_t)clamp32(cv,0,1000);}',
 'else if(a->config.lfo_target==MELLO_LFO_FILTER){int32_t cv=(int32_t)cut+((lfo*(int32_t)a->config.lfo_depth)/1000)*220/32767;cut=(uint16_t)clamp32(cv,0,1000);}'),
('if(a->tm_filter_mod_q15){int32_t cv=(int32_t)cut+((int32_t)a->tm_filter_mod_q15*800)/32767;cut=(uint16_t)clamp32(cv,0,1000);}',
 'if(a->tm_filter_mod_q15){int32_t tv=clamp32(a->tm_filter_mod_q15,-8192,8192);uint32_t ti=(uint32_t)(tv+8192)*100u/16384u;int32_t cv=(int32_t)cut+TM_FILTER_OFFSET_PERMILLE[ti>100u?100u:ti];cut=(uint16_t)clamp32(cv,0,1000);}'),
('(a->matrix_env_q15*300)/32767','(a->matrix_env_q15*240)/32767'),
('uint16_t sa=FILTER_ALPHA_Q15[a->sub_config.cut/10u];','uint16_t sa=SUB_FILTER_ALPHA_Q15[a->sub_config.cut/10u];'),
('mix=lead+sub+sample_mix;','mix=(lead+sub+sample_mix)*580/1000;'),
('uint16_t al=FILTER_ALPHA_Q15[cut/10u];','uint16_t al=MAIN_FILTER_ALPHA_Q15[cut/10u];'),
('if(a->fx.dist_amount)wet=soft_drive(mix,a->fx.dist_amount,a->fx.dist_character);','if(a->dist_drive_permille)wet=soft_drive(mix,a->dist_drive_permille,0u);'),
('ta=FILTER_ALPHA_Q15[a->fx.dist_tone/10u];','ta=DIST_TONE_ALPHA_Q15[a->fx.dist_tone/10u];'),
]
for old,new in pairs: s=req(s,old,new)

s=req(s,'a->dist_lp+=q15mul(mix-a->dist_lp,ta);mix=a->dist_lp*(int32_t)a->fx.dist_level/1000;}if(a->delay_buffer',
      'a->dist_lp+=q15mul(mix-a->dist_lp,ta);mix=a->dist_lp*(int32_t)a->fx.dist_level/1000;} { int32_t fx_in=mix; mix=fx_in*840/1000;if(a->delay_buffer')
s=req(s,'int32_t d=a->delay_buffer[rp];uint16_t da=FILTER_ALPHA_Q15[a->fx.delay_tone/10u];a->delay_lp+=q15mul(d-a->delay_lp,da);a->delay_buffer[a->delay_pos]=sat16(mix+a->delay_lp*(int32_t)a->fx.delay_feedback/1000);a->delay_pos=(a->delay_pos+1u)%a->delay_capacity;mix+=(a->delay_lp*(int32_t)a->fx.delay_wet/1000)*(int32_t)a->fx.delay_level/1000;',
      'int32_t d=a->delay_buffer[rp];uint16_t da=DELAY_TONE_ALPHA_Q15[a->fx.delay_tone/10u];int32_t send=fx_in*(int32_t)a->fx.delay_wet/1000;a->delay_lp+=q15mul(d-a->delay_lp,da);a->delay_buffer[a->delay_pos]=sat16(send+d*(int32_t)a->fx.delay_feedback/1000);a->delay_pos=(a->delay_pos+1u)%a->delay_capacity;mix+=(a->delay_lp*(int32_t)a->fx.delay_level)/1000;')

m=re.search(r'if\(a->reverb_buffer&&a->reverb_capacity>1024u&&a->fx\.reverb_wet\)\{.*?\}mix=mix\*\(int32_t\)a->config\.master_level/1000;mix=clamp32\(mix,-32767,32767\);',s,re.S)
if not m: raise SystemExit('reverb/master block missing')
rev='''if(a->reverb_buffer&&a->reverb_capacity>1024u&&(a->fx.reverb_wet||a->tail_guard_frames)){
        uint32_t ra=(a->reverb_pos+a->reverb_capacity-a->reverb_delay_a)%a->reverb_capacity;
        uint32_t rb=(a->reverb_pos+a->reverb_capacity-a->reverb_delay_b)%a->reverb_capacity;
        int32_t rv=((int32_t)a->reverb_buffer[ra]+(int32_t)a->reverb_buffer[rb])/2;
        int32_t send=fx_in*(int32_t)a->reverb_send_permille/1000;
        const uint16_t damp_alpha=14790u;
        uint16_t tone_alpha=REVERB_TONE_ALPHA_Q15[a->fx.reverb_tone/10u];
        a->reverb_damp+=q15mul(rv-a->reverb_damp,damp_alpha);
        a->reverb_buffer[a->reverb_pos]=sat16(send+q15mul(a->reverb_damp,a->reverb_feedback_q15));
        a->reverb_pos=(a->reverb_pos+1u)%a->reverb_capacity;
        a->reverb_tone_lp+=q15mul(rv-a->reverb_tone_lp,tone_alpha);
        {
            const int32_t hp_alpha=32656;
            int32_t hp=q15mul(hp_alpha,a->reverb_hp_y1+a->reverb_tone_lp-a->reverb_hp_x1);
            a->reverb_hp_x1=a->reverb_tone_lp;
            a->reverb_hp_y1=hp;
            mix+=((hp*(int32_t)a->fx.reverb_level)/1000)*1050/1000;
        }
    }
    mix=limiter_minus_1db(mix);
    mix=mix*(int32_t)a->config.master_level/1000;
    mix=clamp32(mix,-32767,32767);
    }'''
s=s[:m.start()]+rev+s[m.end():]
s=req(s,'if(a->fx.reverb_wet&&a->reverb_delay_b+512u>guard)guard=a->reverb_delay_b+512u;',
      'if(a->fx.reverb_wet){uint32_t rg=ms_samples(a->fx.reverb_decay_ms);if(rg>guard)guard=rg;}')

h=H.read_text()
h=req(h,'    uint32_t reverb_delay_a;\n    uint32_t reverb_delay_b;\n    int32_t reverb_lp;\n',
'''    uint32_t reverb_delay_a;\n    uint32_t reverb_delay_b;\n    int32_t reverb_lp; /* retained compatibility state */\n    int32_t reverb_damp;\n    int32_t reverb_tone_lp;\n    int32_t reverb_hp_x1;\n    int32_t reverb_hp_y1;\n    uint16_t reverb_feedback_q15;\n    uint16_t reverb_send_permille;\n    uint16_t dist_drive_permille;\n''')
C.write_text(s); H.write_text(h)
if digest(C)!=FINAL_C or digest(H)!=FINAL_H:
    raise SystemExit(f'audio parity output hash mismatch: {digest(C)} {digest(H)}')
print('PASS: HTML audio chain parity transform applied')
