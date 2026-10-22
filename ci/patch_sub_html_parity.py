#!/usr/bin/env python3
from pathlib import Path
import hashlib, math, re

ROOT=Path(__file__).resolve().parents[1]
C=ROOT/'mello'/'mello_audio.c'
H=ROOT/'mello'/'mello_audio.h'
E=ROOT/'mello'/'mello_engine.c'
EXPECTED_C='2b6cf93f1ae16517aab7a51c1cca716de99d2ee0e33111881bd9f3aec374a498'
EXPECTED_H='04c12ca9c47ff535f15a8be63761228754ff4af21b83e8c231717c6cf919670f'
EXPECTED_E='07d81eef8397c8e6e6c2e9e2faa4ecf2f493c485e1c1b5d824664c059be3d2c7'
FINAL_C='37144c688b3323a12e731d2c73aef855032c1892744d676f31a64dced8566c82'
FINAL_H='af7f8c1c0bee5d66fcda6fb3ce3c629677590146a98a040e5cc33f7989a8c3ec'
FINAL_E='ddba56b4ab34981189d2275d22e530e3767be1d00d79a063899fd5e1beb96416'

def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def req(s,old,new):
    if old not in s: raise SystemExit('sub parity precondition missing: '+repr(old[:120]))
    return s.replace(old,new,1)
def alpha_q15(fc):
    a=1-math.exp(-2*math.pi*min(fc,20000.0)/44100.0)
    return max(1,min(32767,round(a*32767)))
def carr(name,vals):
    lines=[]
    for i in range(0,101,10): lines.append('    '+', '.join(str(x) for x in vals[i:i+10])+',')
    return f'static const uint16_t {name}[101] = {{\n'+'\n'.join(lines)+'\n};'

if digest(C)!=EXPECTED_C or digest(H)!=EXPECTED_H or digest(E)!=EXPECTED_E:
    raise SystemExit('sub parity source hash mismatch; rebase transform')

s=C.read_text(); h=H.read_text(); e=E.read_text()

# Tone.Filter rolloff=-24: compensate each one-pole stage so the 4-stage cascade
# reaches the requested source cutoff rather than rolling off several octaves early.
comp=1/math.sqrt(2**(1/4)-1)
vals=[alpha_q15(40*(55**(i/100))*comp) for i in range(101)]
s,n=re.subn(r'static const uint16_t SUB_FILTER_ALPHA_Q15\[101\] = \{.*?\n\};',carr('SUB_FILTER_ALPHA_Q15',vals),s,flags=re.S)
if n!=1: raise SystemExit('sub filter table missing')

h=req(h,'''    bool gate;\n    int32_t lp;\n} MelloAudioSubVoice;''','''    bool gate;\n    int32_t filter1;\n    int32_t filter2;\n    int32_t filter3;\n    int32_t filter4;\n    int32_t drive_prev;\n} MelloAudioSubVoice;''')
h=req(h,'''    uint16_t reverb_feedback_q15;\n    uint16_t reverb_send_permille;\n    uint16_t dist_drive_permille;\n''','''    uint16_t reverb_feedback_q15;\n    uint16_t reverb_send_permille;\n    uint16_t dist_drive_permille;\n    int16_t sub_dist_curve[257];\n''')

m=re.search(r'static void sub_attack\(MelloAudio \*a,uint8_t note,uint8_t vel\)\{.*?\}\nstatic void sub_release\(MelloAudio \*a\)\{.*?\}',s,re.S)
if not m: raise SystemExit('sub envelope block missing')
sub_env='''static void sub_begin_decay(MelloAudio *a){\n    env_set_segment(&a->sub.env_step_q24,&a->sub.env_target_q24,a->sub.env_q24,permille_q24(240u),ms_samples(a->sub_config.decay_ms));\n}\nstatic void sub_attack(MelloAudio *a,uint8_t note,uint8_t vel){\n    uint32_t n;\n    bool glide;\n    a->sub.note=note;a->sub.velocity=vel;a->sub.gate=true;a->sub.target_inc=note_inc(note);\n    /* Tone.Monophonic only portamentos while the voice is already audible. */\n    glide=a->sub_config.glide_ms&&a->sub.inc&&a->sub.env_q24>(Q24_ONE/20);\n    if(glide){\n        n=ms_samples(a->sub_config.glide_ms);if(n<1u)n=1u;\n        a->sub.slide_left=n;\n        a->sub.slide_step=((int32_t)a->sub.target_inc-(int32_t)a->sub.inc)/(int32_t)n;\n    }else{a->sub.inc=a->sub.target_inc;a->sub.slide_left=0u;}\n    /* HTML dedicated sub is Tone.Synth attack=.003, decay=user, sustain=.24, release=user. */\n    a->sub.env_stage=MELLO_ENV_ATTACK;\n    env_set_segment(&a->sub.env_step_q24,&a->sub.env_target_q24,a->sub.env_q24,Q24_ONE,ms_samples(3u));\n}\nstatic void sub_release(MelloAudio *a){\n    a->sub.gate=false;a->sub.env_stage=MELLO_ENV_RELEASE;\n    env_set_segment(&a->sub.env_step_q24,&a->sub.env_target_q24,a->sub.env_q24,0,ms_samples(a->sub_config.release_ms));\n}'''
s=s[:m.start()]+sub_env+s[m.end():]

marker='static uint16_t browser_dist_drive(uint16_t amount,uint16_t character){'
if marker not in s: raise SystemExit('audio helper insertion point missing')
helpers='''static int16_t tone_distortion_point(uint16_t amount,uint16_t index){\n    /* Tone.Distortion waveshaper from Tone.js 14. amount is permille here. */\n    const uint32_t PI_Q16=205887u, DEG20_Q16=22876u;\n    uint32_t k=(uint32_t)amount/10u;\n    uint32_t x=index>=256u?32767u:(uint32_t)index*128u;\n    uint32_t den=PI_Q16+k*x*2u;\n    uint64_t num=(uint64_t)(3u+k)*x*DEG20_Q16;\n    uint32_t y=den?(uint32_t)((num+den/2u)/den):0u;\n    if(y>32767u)y=32767u;\n    return (int16_t)y;\n}\nstatic void build_sub_dist_curve(MelloAudio *a){\n    uint16_t i;\n    if(!a)return;\n    for(i=0u;i<=256u;++i)a->sub_dist_curve[i]=tone_distortion_point(a->sub_config.drive,i);\n}\nstatic int32_t sub_tone_distort(const MelloAudio *a,int32_t x){\n    uint32_t ax,idx,frac;int32_t y0,y1,y;\n    if(!a||!a->sub_config.drive)return x;\n    x=clamp32(x,-32767,32767);ax=(uint32_t)(x<0?-x:x);idx=ax>>7;if(idx>255u)idx=255u;frac=ax&127u;\n    y0=a->sub_dist_curve[idx];y1=a->sub_dist_curve[idx+1u];y=y0+((y1-y0)*(int32_t)frac)/128;\n    return x<0?-y:y;\n}\n\n'''
s=s.replace(marker,helpers+marker,1)

s=req(s,'''    a->sub_config=a->pending_sub_config;\n    a->sample_config=a->pending_sample_config;''','''    a->sub_config=a->pending_sub_config;\n    a->sample_config=a->pending_sample_config;\n    build_sub_dist_curve(a);''')

s=req(s,'if(a->sub.env_stage==MELLO_ENV_DECAY||a->sub.env_stage==MELLO_ENV_RELEASE)(void)env_tick(&a->sub.env_q24,a->sub.env_step_q24,a->sub.env_target_q24,&a->sub.env_stage);',
      'if(a->sub.env_stage==MELLO_ENV_ATTACK||a->sub.env_stage==MELLO_ENV_DECAY||a->sub.env_stage==MELLO_ENV_RELEASE){MelloEnvStage before=a->sub.env_stage;(void)env_tick(&a->sub.env_q24,a->sub.env_step_q24,a->sub.env_target_q24,&a->sub.env_stage);if(before==MELLO_ENV_ATTACK&&a->sub.env_stage==MELLO_ENV_DECAY)sub_begin_decay(a);}')

m=re.search(r'if\(a->sub\.env_q24>0&&a->sub_config\.on&&a->sub_config\.wave!=MELLO_WAVE_OFF\)\{uint16_t sa=SUB_FILTER_ALPHA_Q15\[a->sub_config\.cut/10u\];.*?sub=a->sub\.lp;\}',s,re.S)
if not m: raise SystemExit('sub render block missing')
sub_render='''if(a->sub.env_q24>0&&a->sub_config.on&&a->sub_config.wave!=MELLO_WAVE_OFF){\n        uint16_t sa=SUB_FILTER_ALPHA_Q15[a->sub_config.cut/10u];\n        int32_t fb,inp,filtered;\n        a->sub.phase+=a->sub.inc;\n        sub=osc_sample(a->sub_config.wave,a->sub.phase,500u);\n        sub=q15mul(sub,a->sub.env_q24>>8);\n        sub=q15mul(sub,(int32_t)a->sub.velocity*258);\n        /* HTML order: Synth -> 24 dB LPF (Q .8) -> Tone.Distortion(2x) -> level. */\n        fb=q15mul(a->sub.filter4,1307);\n        inp=clamp32(sub-fb,-65535,65535);\n        a->sub.filter1+=q15mul(inp-a->sub.filter1,sa);\n        a->sub.filter2+=q15mul(a->sub.filter1-a->sub.filter2,sa);\n        a->sub.filter3+=q15mul(a->sub.filter2-a->sub.filter3,sa);\n        a->sub.filter4+=q15mul(a->sub.filter3-a->sub.filter4,sa);\n        filtered=a->sub.filter4;\n        if(a->sub_config.drive){\n            /* Lightweight 2x approximation: shape midpoint + current then down-average. */\n            int32_t mid=a->sub.drive_prev+((filtered-a->sub.drive_prev)>>1);\n            filtered=(sub_tone_distort(a,mid)+sub_tone_distort(a,filtered))/2;\n        }\n        a->sub.drive_prev=a->sub.filter4;\n        sub=filtered*(int32_t)a->sub_config.level/1000;\n    }'''
s=s[:m.start()]+sub_render+s[m.end():]

e=req(e,'emit_basic(e,MELLO_EVENT_SUB_NOTE_ON,on_tick,step_idx,pulse_idx,total_pulses,note,108u,0u,false,pass);',
      'emit_basic(e,MELLO_EVENT_SUB_NOTE_ON,on_tick,step_idx,pulse_idx,total_pulses,note,114u,0u,false,pass);')

C.write_text(s);H.write_text(h);E.write_text(e)
for p,want in ((C,FINAL_C),(H,FINAL_H),(E,FINAL_E)):
    got=digest(p)
    if got!=want: raise SystemExit(f'sub parity final hash mismatch for {p.name}: {got} != {want}')
print('PASS: HTML dedicated-sub signal path, envelope, portamento and Tone drive parity applied')
