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
FINAL_C='f1713bfdc705926c8fa099708b71ca59d8efeae732319793985f0bb7309d5642'
FINAL_H='c4257112882c57474f8e7644d786af57fcf1bd0814c553a51f20a2806a091db9'
FINAL_E='ddba56b4ab34981189d2275d22e530e3767be1d00d79a063899fd5e1beb96416'

SR=44100.0
Q=0.8
Q28=1<<28
Q31=1<<31

def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def req(s,old,new):
    if old not in s: raise SystemExit('sub parity precondition missing: '+repr(old[:120]))
    return s.replace(old,new,1)

def biquad_coeff(fc):
    w=2*math.pi*fc/SR
    sn=math.sin(w); cs=math.cos(w); alpha=sn/(2*Q); a0=1+alpha
    vals=((1-cs)/(2*a0),(1-cs)/a0,(1-cs)/(2*a0),(-2*cs)/a0,(1-alpha)/a0)
    return tuple(int(round(v*Q28)) for v in vals)

def biquad_table():
    rows=[]
    for i in range(101):
        fc=40*(55**(i/100))
        c=biquad_coeff(fc)
        rows.append('    { '+', '.join(str(v) for v in c)+' },')
    return '''typedef struct { int32_t b0,b1,b2,a1,a2; } ToneSubBiquadCoeff;\nstatic const ToneSubBiquadCoeff SUB_FILTER_BIQUAD_Q28[101] = {\n'''+"\n".join(rows)+'\n};'

def env_coeff(ms):
    # Tone.Param.exponentialApproachValueAtTime: timeConstant=ln(rampTime+1)/ln(200)
    sec=max(ms,1)/1000.0
    tc=math.log(sec+1.0)/math.log(200.0)
    coef=math.exp(-1.0/(SR*tc))
    return max(0,min(Q31-1,int(round(coef*(Q31-1)))))

def env_table():
    vals=[env_coeff(ms) for ms in range(1201)]
    lines=[]
    for i in range(0,len(vals),8):
        lines.append('    '+', '.join(str(v)+'u' for v in vals[i:i+8])+',')
    return 'static const uint32_t TONE_ENV_COEFF_Q31[1201] = {\n'+"\n".join(lines)+'\n};'

if digest(C)!=EXPECTED_C or digest(H)!=EXPECTED_H or digest(E)!=EXPECTED_E:
    raise SystemExit('sub parity source hash mismatch; rebase transform')

s=C.read_text(); h=H.read_text(); e=E.read_text()

# Tone.Filter rolloff=-24 is two cascaded native BiquadFilter nodes with the same
# frequency and Q. The HTML dedicated sub sets Q=.8.
s,n=re.subn(r'static const uint16_t SUB_FILTER_ALPHA_Q15\[101\] = \{.*?\n\};',biquad_table()+'\n\n'+env_table(),s,flags=re.S)
if n!=1: raise SystemExit('sub filter table missing')

h=req(h,'''    bool gate;\n    int32_t lp;\n} MelloAudioSubVoice;''','''    bool gate;\n    uint32_t env_curve_left;\n    uint32_t env_linear_left;\n    uint32_t env_curve_coeff_q31;\n    int32_t biquad1_x1;\n    int32_t biquad1_x2;\n    int32_t biquad1_y1;\n    int32_t biquad1_y2;\n    int32_t biquad2_x1;\n    int32_t biquad2_x2;\n    int32_t biquad2_y1;\n    int32_t biquad2_y2;\n    int32_t drive_prev;\n} MelloAudioSubVoice;''')
h=req(h,'''    uint16_t reverb_feedback_q15;\n    uint16_t reverb_send_permille;\n    uint16_t dist_drive_permille;\n''','''    uint16_t reverb_feedback_q15;\n    uint16_t reverb_send_permille;\n    uint16_t dist_drive_permille;\n    int16_t sub_dist_curve[257];\n''')

m=re.search(r'static void sub_attack\(MelloAudio \*a,uint8_t note,uint8_t vel\)\{.*?\}\nstatic void sub_release\(MelloAudio \*a\)\{.*?\}',s,re.S)
if not m: raise SystemExit('sub envelope block missing')
sub_env='''static void sub_curve_begin(MelloAudio *a,int32_t target,uint16_t ms){\n    uint32_t n,curve;\n    if(!a)return;\n    if(ms>1200u)ms=1200u;\n    n=ms_samples(ms);if(n<2u)n=2u;\n    curve=(n*9u)/10u;if(curve<1u)curve=1u;if(curve>=n)curve=n-1u;\n    a->sub.env_target_q24=target;\n    a->sub.env_curve_left=curve;\n    a->sub.env_linear_left=n-curve;\n    a->sub.env_curve_coeff_q31=TONE_ENV_COEFF_Q31[ms];\n    a->sub.env_step_q24=0;\n}\nstatic void sub_curve_tick(MelloAudio *a){\n    int32_t d;\n    if(!a)return;\n    if(a->sub.env_curve_left){\n        d=a->sub.env_q24-a->sub.env_target_q24;\n        a->sub.env_q24=a->sub.env_target_q24+(int32_t)((int64_t)d*(int64_t)a->sub.env_curve_coeff_q31)>>31);\n        if(--a->sub.env_curve_left==0u){\n            uint32_t n=a->sub.env_linear_left?a->sub.env_linear_left:1u;\n            a->sub.env_step_q24=(a->sub.env_target_q24-a->sub.env_q24)/(int32_t)n;\n        }\n    }else if(a->sub.env_linear_left){\n        if(--a->sub.env_linear_left==0u){\n            a->sub.env_q24=a->sub.env_target_q24;\n            if(a->sub.env_stage==MELLO_ENV_DECAY)a->sub.env_stage=MELLO_ENV_SUSTAIN;\n            else if(a->sub.env_stage==MELLO_ENV_RELEASE)a->sub.env_stage=MELLO_ENV_OFF;\n        }else a->sub.env_q24+=a->sub.env_step_q24;\n    }\n}\nstatic void sub_begin_decay(MelloAudio *a){\n    sub_curve_begin(a,permille_q24(240u),a->sub_config.decay_ms);\n}\nstatic void sub_attack(MelloAudio *a,uint8_t note,uint8_t vel){\n    uint32_t n,attack_n;\n    bool glide;\n    a->sub.note=note;a->sub.velocity=vel;a->sub.gate=true;a->sub.target_inc=note_inc(note);\n    /* Tone.Monophonic only portamentos while the voice is already audible. */\n    glide=a->sub_config.glide_ms&&a->sub.inc&&a->sub.env_q24>(Q24_ONE/20);\n    if(glide){\n        n=ms_samples(a->sub_config.glide_ms);if(n<1u)n=1u;\n        a->sub.slide_left=n;\n        a->sub.slide_step=((int32_t)a->sub.target_inc-(int32_t)a->sub.inc)/(int32_t)n;\n    }else{a->sub.inc=a->sub.target_inc;a->sub.slide_left=0u;}\n    /* Tone.Envelope shortens a retrigger attack by the already-reached level. */\n    attack_n=ms_samples(3u);\n    if(a->sub.env_q24>0&&a->sub.env_q24<Q24_ONE)attack_n=(uint32_t)((uint64_t)attack_n*(uint32_t)(Q24_ONE-a->sub.env_q24))/Q24_ONE);\n    if(attack_n<1u)attack_n=1u;\n    a->sub.env_curve_left=0u;a->sub.env_linear_left=0u;\n    a->sub.env_stage=MELLO_ENV_ATTACK;\n    env_set_segment(&a->sub.env_step_q24,&a->sub.env_target_q24,a->sub.env_q24,Q24_ONE,attack_n);\n}\nstatic void sub_release(MelloAudio *a){\n    a->sub.gate=false;a->sub.env_stage=MELLO_ENV_RELEASE;\n    sub_curve_begin(a,0,a->sub_config.release_ms);\n}'''
s=s[:m.start()]+sub_env+s[m.end():]

marker='static uint16_t browser_dist_drive(uint16_t amount,uint16_t character){'
if marker not in s: raise SystemExit('audio helper insertion point missing')
helpers='''static int32_t sub_biquad_step(int32_t x,const ToneSubBiquadCoeff *c,int32_t *x1,int32_t *x2,int32_t *y1,int32_t *y2){\n    int64_t acc;int32_t y;\n    acc=(int64_t)c->b0*x+(int64_t)c->b1*(*x1)+(int64_t)c->b2*(*x2)-(int64_t)c->a1*(*y1)-(int64_t)c->a2*(*y2);\n    y=(int32_t)(acc>>28);y=clamp32(y,-131071,131071);\n    *x2=*x1;*x1=x;*y2=*y1;*y1=y;return y;\n}\nstatic int16_t tone_distortion_point(uint16_t amount,uint16_t index){\n    /* Tone.Distortion waveshaper from Tone.js 14. amount is permille here. */\n    const uint32_t PI_Q16=205887u, DEG20_Q16=22876u;\n    uint32_t k=(uint32_t)amount/10u;\n    uint32_t x=index>=256u?32767u:(uint32_t)index*128u;\n    uint32_t den=PI_Q16+k*x*2u;\n    uint64_t num=(uint64_t)(3u+k)*x*DEG20_Q16;\n    uint32_t y=den?(uint32_t)((num+den/2u)/den):0u;\n    if(y>32767u)y=32767u;\n    return (int16_t)y;\n}\nstatic void build_sub_dist_curve(MelloAudio *a){\n    uint16_t i;\n    if(!a)return;\n    for(i=0u;i<=256u;++i)a->sub_dist_curve[i]=tone_distortion_point(a->sub_config.drive,i);\n}\nstatic int32_t sub_tone_distort(const MelloAudio *a,int32_t x){\n    uint32_t ax,idx,frac;int32_t y0,y1,y;\n    if(!a||!a->sub_config.drive)return x;\n    x=clamp32(x,-32767,32767);ax=(uint32_t)(x<0?-x:x);idx=ax>>7;if(idx>255u)idx=255u;frac=ax&127u;\n    y0=a->sub_dist_curve[idx];y1=a->sub_dist_curve[idx+1u];y=y0((y1-y0)*(int32_t)frac)/128;\n    return x<0?-y:y;\n}\n\n'''
s=s.replace(marker,helpers+marker,1)

s=req(s,'''    a->sub_config=a->pending_sub_config;\n    a->sample_config=a->pending_sample_config;''','''    a->sub_config=a->pending_sub_config;\n    a->sample_config=a->pending_sample_config;\n    build_sub_dist_curve(a);''')

s=req(s,'if(a->sub.env_stage==MELLO_ENV_DECAY||a->sub.env_stage==MELLO_ENV_RELEASE)(void)env_tick(&a->sub.env_q24,a->sub.env_step_q24,a->sub.env_target_q24,&a->sub.env_stage);',
      'if(a->sub.env_stage==MELLO_ENV_ATTACK){MelloEnvStage before=a->sub.env_stage;(void)env_tick(&a->sub.env_q24,a->sub.env_step_q24,a->sub.env_target_q24,&a->sub.env_stage);if(before==MELLO_ENV_ATTACK&&a->sub.env_stage==MELLO_ENV_DECAY)sub_begin_decay(a);}else if(a->sub.env_stage==MELLO_ENV_DECAY||a->sub.env_stage==MELLO_ENV_RELEASE)sub_curve_tick(a);')

m=re.search(r'if\(a->sub\.env_q24>0&&a->sub_config\.on&&a->sub_config\.wave!=MELLO_WAVE_OFF\)\{uint16_t sa=SUB_FILTER_ALPHA_Q15\[a->sub_config\.cut/10u\];.*?sub=a->sub\.lp;\}',s,re.S)
if not m: raise SystemExit('sub render block missing')
sub_render='''if(a->sub.env_q24>0&&a->sub_config.on&&a->sub_config.wave!=MELLO_WAVE_OFF){\n        const ToneSubBiquadCoeff *bc=&SUB_FILTER_BIQUAD_Q28[a->sub_config.cut/10u];\n        int32_t filtered;\n        a->sub.phase+=a->sub.inc;\n        sub=osc_sample(a->sub_config.wave,a->sub.phase,500u);\n        sub=q15mul(sub,a->sub.env_q24>>8);\n        sub=q15mul(sub,(int32_t)a->sub.velocity*258);\n        /* HTML: Tone.Filter rolloff=-24 => two cascaded biquads, both Q=.8. */\n        filtered=sub_biquad_step(sub,bc,&a->sub.biquad1_x1,&a->sub.biquad1_x2,&a->sub.biquad1_y1,&a->sub.biquad1_y2);\n        filtered=sub_biquad_step(filtered,bc,&a->sub.biquad2_x1,&a->sub.biquad2_x2,&a->sub.biquad2_y1,&a->sub.biquad2_y2);\n        if(a->sub_config.drive){\n            /* Lightweight 2x approximation around Tone.WaveShaper oversample=2x. */\n            int32_t mid=a->sub.drive_prev+((filtered-a->sub.drive_prev)>>1);\n            filtered=(sub_tone_distort(a,mid)+sub_tone_distort(a,filtered))/2;\n        }\n        a->sub.drive_prev=filtered;\n        sub=filtered*(int32_t)a->sub_config.level/1000;\n    }'''
s=s[:m.start()]+sub_render+s[m.end():]

e=req(e,'emit_basic(e,MELLO_EVENT_SUB_NOTE_ON,on_tick,step_idx,pulse_idx,total_pulses,note,108u,0u,false,pass);',
      'emit_basic(e,MELLO_EVENT_SUB_NOTE_ON,on_tick,step_idx,total_pulses,note,114u,0u,false,pass);')

C.write_text(s);H.write_text(h);E.write_text(e)
if FINAL_C!='__FINAL_C__':
    for p,want in ((C,FINAL_C),(H,FINAL_H),(E,FINAL_E)):
        got=digest(p)
        if got!=want: raise SystemExit(f'sub parity final hash mismatch for {p.name}: {got} != {want}')
print('PASS: HTML dedicated-sub biquad, exponential envelope, portamento and Tone drive parity applied')
