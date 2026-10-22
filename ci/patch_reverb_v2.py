#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
C = ROOT / "mello" / "mello_audio.c"
H = ROOT / "mello" / "mello_audio.h"

s = C.read_text()
h = H.read_text()

def once(text, old, new, label):
    n = text.count(old)
    if n != 1:
        raise SystemExit(f"FAIL: Reverb V2 {label}: expected one anchor, found {n}")
    return text.replace(old, new, 1)

if "RV2_LAYOUT_FULL" in s or "reverb_line_feedback_q15" in h:
    raise SystemExit("FAIL: Reverb V2 appears to be applied already")

header_insert = (
    "    uint16_t reverb_feedback_q15; /* representative/debug value; per-line gains below */\n"
    "    uint16_t reverb_pre_samples;\n"
    "    uint16_t reverb_pre_cap;\n"
    "    uint16_t reverb_ap_offset[2];\n"
    "    uint16_t reverb_ap_cap[2];\n"
    "    uint16_t reverb_ap_delay[2];\n"
    "    uint16_t reverb_ap_pos[2];\n"
    "    uint16_t reverb_line_offset[4];\n"
    "    uint16_t reverb_line_cap[4];\n"
    "    uint16_t reverb_line_delay[4];\n"
    "    uint16_t reverb_line_pos[4];\n"
    "    uint16_t reverb_line_feedback_q15[4];\n"
    "    int32_t reverb_line_damp[4];\n"
)
h = once(h, "    uint16_t reverb_feedback_q15;\n", header_insert, "header state")

helpers = r'''
/*
 * Reverb V2: compact Schroeder/FDN hybrid.
 *
 * The browser uses Tone.Reverb (generated IR). A literal convolution engine is
 * wasteful on FM-1, so the native target uses a separate pre-delay, two short
 * all-pass diffusers and a four-line Hadamard feedback delay network. The seven
 * ring regions are scaled together only when a host test supplies less memory;
 * the 24576-sample target ring fits the full layout with headroom.
 */
#define RV2_LAYOUT_FULL 23173u
#define RV2_PRE_FULL     5293u
#define RV2_AP1_FULL      353u
#define RV2_AP2_FULL      521u
#define RV2_L0_FULL      3001u
#define RV2_L1_FULL      3701u
#define RV2_L2_FULL      4603u
#define RV2_L3_FULL      5701u

static uint16_t rv2_scaled_cap(uint32_t total,uint16_t full){
    uint32_t scale,c;
    if(total>=RV2_LAYOUT_FULL)return full;
    scale=(total*1000u)/RV2_LAYOUT_FULL;
    c=((uint32_t)full*scale)/1000u;
    if(c<8u)c=8u;
    return (uint16_t)c;
}
static uint16_t rv2_tap(uint16_t cap,uint16_t lo,uint16_t hi,uint16_t size){
    uint32_t pm,d;
    if(cap<4u)return 1u;
    if(size>1000u)size=1000u;
    pm=(uint32_t)lo+(((uint32_t)(hi-lo)*size+500u)/1000u);
    d=((uint32_t)cap*pm+500u)/1000u;
    if(d<3u)d=3u;
    if(d>=cap)d=cap-1u;
    if((d&1u)==0u){
        if(d+1u<cap)d+=1u;
        else if(d>3u)d-=1u;
    }
    return (uint16_t)d;
}
static void reverb_configure(MelloAudio *a){
    static const uint16_t full_line[4]={RV2_L0_FULL,RV2_L1_FULL,RV2_L2_FULL,RV2_L3_FULL};
    static const uint16_t lo_pm[4]={300u,340u,380u,420u};
    static const uint16_t hi_pm[4]={880u,910u,930u,950u};
    uint16_t i;
    uint32_t off,sum=0u,pre;
    if(!a||!a->reverb_capacity)return;
    a->reverb_pre_cap=rv2_scaled_cap(a->reverb_capacity,RV2_PRE_FULL);
    a->reverb_ap_cap[0]=rv2_scaled_cap(a->reverb_capacity,RV2_AP1_FULL);
    a->reverb_ap_cap[1]=rv2_scaled_cap(a->reverb_capacity,RV2_AP2_FULL);
    for(i=0u;i<4u;++i)a->reverb_line_cap[i]=rv2_scaled_cap(a->reverb_capacity,full_line[i]);

    off=a->reverb_pre_cap;
    a->reverb_ap_offset[0]=(uint16_t)off;off+=a->reverb_ap_cap[0];
    a->reverb_ap_offset[1]=(uint16_t)off;off+=a->reverb_ap_cap[1];
    for(i=0u;i<4u;++i){a->reverb_line_offset[i]=(uint16_t)off;off+=a->reverb_line_cap[i];}
    if(off>a->reverb_capacity){
        a->reverb_pre_cap=0u;
        return;
    }

    pre=ms_samples(a->fx.reverb_pre_ms);
    if(pre>=a->reverb_pre_cap)pre=a->reverb_pre_cap?a->reverb_pre_cap-1u:0u;
    a->reverb_pre_samples=(uint16_t)pre;
    a->reverb_ap_delay[0]=rv2_tap(a->reverb_ap_cap[0],430u,790u,a->fx.reverb_size);
    a->reverb_ap_delay[1]=rv2_tap(a->reverb_ap_cap[1],370u,730u,a->fx.reverb_size);
    for(i=0u;i<4u;++i){
        a->reverb_line_delay[i]=rv2_tap(a->reverb_line_cap[i],lo_pm[i],hi_pm[i],a->fx.reverb_size);
        a->reverb_line_feedback_q15[i]=reverb_feedback_q15(a->reverb_line_delay[i],a->fx.reverb_decay_ms);
        sum+=a->reverb_line_feedback_q15[i];
        if(a->reverb_line_pos[i]>=a->reverb_line_cap[i])a->reverb_line_pos[i]=0u;
    }
    if(a->reverb_ap_pos[0]>=a->reverb_ap_cap[0])a->reverb_ap_pos[0]=0u;
    if(a->reverb_ap_pos[1]>=a->reverb_ap_cap[1])a->reverb_ap_pos[1]=0u;
    if(a->reverb_pos>=a->reverb_pre_cap)a->reverb_pos=0u;
    a->reverb_feedback_q15=(uint16_t)(sum/4u);
    a->reverb_delay_a=a->reverb_line_delay[0];
    a->reverb_delay_b=a->reverb_line_delay[3];
}
static int32_t rv2_allpass(MelloAudio *a,uint16_t which,int32_t x,uint16_t g){
    uint16_t cap,delay,pos,rp,off;
    int32_t z,y,w;
    cap=a->reverb_ap_cap[which];delay=a->reverb_ap_delay[which];
    pos=a->reverb_ap_pos[which];off=a->reverb_ap_offset[which];
    if(cap<4u||delay<1u)return x;
    rp=pos>=delay?(uint16_t)(pos-delay):(uint16_t)(pos+cap-delay);
    z=a->reverb_buffer[(uint32_t)off+rp];
    y=z-q15mul(x,g);
    y=clamp32(y,-32767,32767);
    w=x+q15mul(y,g);
    a->reverb_buffer[(uint32_t)off+pos]=sat16(w);
    pos++;if(pos>=cap)pos=0u;a->reverb_ap_pos[which]=pos;
    return y;
}
static int32_t reverb_network_step(MelloAudio *a,int32_t send){
    int32_t x,y[4],d[4],h[4],w,rv;
    uint16_t i,pos,rp,cap,delay,off;
    uint32_t p;
    const uint16_t damp_alpha=14755u;
    if(!a||!a->reverb_buffer||a->reverb_pre_cap<2u)return 0;

    p=a->reverb_pos;
    if(a->reverb_pre_samples){
        uint32_t pr=p>=a->reverb_pre_samples?p-a->reverb_pre_samples:p+a->reverb_pre_cap-a->reverb_pre_samples;
        x=a->reverb_buffer[pr];
    }else x=send;
    a->reverb_buffer[p]=sat16(send);
    p++;if(p>=a->reverb_pre_cap)p=0u;a->reverb_pos=p;

    x=rv2_allpass(a,0u,x,18022u);
    x=rv2_allpass(a,1u,x,20890u);

    for(i=0u;i<4u;++i){
        cap=a->reverb_line_cap[i];delay=a->reverb_line_delay[i];
        pos=a->reverb_line_pos[i];off=a->reverb_line_offset[i];
        rp=pos>=delay?(uint16_t)(pos-delay):(uint16_t)(pos+cap-delay);
        y[i]=a->reverb_buffer[(uint32_t)off+rp];
        a->reverb_line_damp[i]+=q15mul(y[i]-a->reverb_line_damp[i],damp_alpha);
        d[i]=a->reverb_line_damp[i];
    }

    h[0]=clamp32((d[0]+d[1]+d[2]+d[3])/2,-32767,32767);
    h[1]=clamp32((d[0]-d[1]+d[2]-d[3])/2,-32767,32767);
    h[2]=clamp32((d[0]+d[1]-d[2]-d[3])/2,-32767,32767);
    h[3]=clamp32((d[0]-d[1]-d[2]+d[3])/2,-32767,32767);

    for(i=0u;i<4u;++i){
        if(i==0u)w=x/2;
        else if(i==1u)w=(x*3)/8;
        else if(i==2u)w=-(x/2);
        else w=-((x*3)/8);
        w+=q15mul(h[i],a->reverb_line_feedback_q15[i]);
        pos=a->reverb_line_pos[i];off=a->reverb_line_offset[i];
        a->reverb_buffer[(uint32_t)off+pos]=sat16(w);
        pos++;if(pos>=a->reverb_line_cap[i])pos=0u;a->reverb_line_pos[i]=pos;
    }

    rv=(y[0]+y[1]-y[2]-y[3])/2;
    return clamp32(rv,-32767,32767);
}
/* Host regression hook. Intentionally omitted from the public header. */
int32_t mello_audio_reverb_ir_probe(MelloAudio *a,int32_t send){
    return reverb_network_step(a,send);
}

'''
anchor = "static int32_t limiter_minus_1db(int32_t x){"
if s.count(anchor) != 1:
    raise SystemExit("FAIL: Reverb V2 helper insertion anchor")
s = s.replace(anchor, helpers + anchor, 1)

s = once(s, "    uint32_t pre,avg_delay;\n", "", "legacy apply locals")
old_cfg = '''    pre=ms_samples(a->fx.reverb_pre_ms);
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
'''
s = once(s, old_cfg, "    reverb_configure(a);\n", "configuration topology")

old_render = '''if(a->reverb_buffer&&a->reverb_capacity>1024u&&(a->fx.reverb_wet||a->tail_guard_frames)){
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
'''
new_render = '''if(a->reverb_buffer&&a->reverb_capacity>1024u&&(a->fx.reverb_wet||a->tail_guard_frames)){
        int32_t send=fx_in*(int32_t)a->reverb_send_permille/1000;
        int32_t rv=reverb_network_step(a,send);
        uint16_t tone_alpha=REVERB_TONE_ALPHA_Q15[a->fx.reverb_tone/10u];
        a->reverb_tone_lp+=q15mul(rv-a->reverb_tone_lp,tone_alpha);
        {
            const int32_t hp_alpha=32656;
            int32_t hp=q15mul(hp_alpha,a->reverb_hp_y1+a->reverb_tone_lp-a->reverb_hp_x1);
            a->reverb_hp_x1=a->reverb_tone_lp;
            a->reverb_hp_y1=hp;
            mix+=((hp*(int32_t)a->fx.reverb_level)/1000)*1050/1000;
        }
    }
'''
s = once(s, old_render, new_render, "render topology")

for token in ("RV2_LAYOUT_FULL","rv2_allpass(a,0u","rv2_allpass(a,1u","h[3]=clamp32","reverb_network_step(a,send)"):
    if token not in s:
        raise SystemExit(f"FAIL: Reverb V2 C postcondition missing {token}")
if "reverb_line_feedback_q15[4]" not in h:
    raise SystemExit("FAIL: Reverb V2 header postcondition")

C.write_text(s)
H.write_text(h)
print("PASS: Reverb V2 separate predelay + 2x allpass diffusion + 4-line Hadamard FDN applied")
