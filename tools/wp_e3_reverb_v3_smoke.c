#include "mello_audio.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if !defined(MELLO_REVERB_TEST_HOOK) || !MELLO_REVERB_TEST_HOOK
#error "WP-E3 smoke requires MELLO_REVERB_TEST_HOOK=1"
#endif

#define RV_TARGET_CAP 24576u
#define IR_N 52000u

static int16_t delay_buf[4096];
static int16_t reverb_buf[RV_TARGET_CAP];

static void setup(MelloAudio *a,uint16_t decay,uint16_t size,uint16_t tone,uint16_t pre)
{
    MelloSoundConfig sound;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sample;
    int32_t out[2]={0,0};
    memset(delay_buf,0,sizeof delay_buf);
    memset(reverb_buf,0,sizeof reverb_buf);
    mello_sound_defaults(&sound);mello_fx_defaults(&fx);mello_sub_defaults(&sub);mello_sample_defaults(&sample);
    fx.delay_wet=0u;fx.delay_level=0u;fx.dist_amount=0u;
    fx.reverb_wet=1000u;fx.reverb_level=1000u;fx.reverb_decay_ms=decay;fx.reverb_size=size;fx.reverb_tone=tone;fx.reverb_pre_ms=pre;
    mello_audio_init(a,delay_buf,4096u,reverb_buf,RV_TARGET_CAP);
    mello_audio_set_config(a,&sound,&fx,&sub,&sample);
    mello_audio_render(a,out,1u,0u,80u);
}

static double tone_roughness(uint16_t tone)
{
    MelloAudio a;
    double e=0.0,de=0.0;
    int32_t prev=0,l=0,r=0;
    uint32_t i;
    setup(&a,6000u,500u,tone,0u);
    for(i=0u;i<IR_N;++i){
        mello_audio_reverb_stereo_probe(&a,i==0u?20000:0,&l,&r);
        if(i>=8000u&&i<40000u){double v=(double)l,d=(double)(l-prev);e+=v*v;de+=d*d;}
        prev=l;
    }
    return e>1.0?de/e:0.0;
}

static void structural_and_stereo_contract(void)
{
    MelloAudio a;
    uint32_t i,total=0u,dense_l=0u,dense_r=0u,different=0u;
    double xx=0.0,yy=0.0,xy=0.0,corr2;
    int32_t l=0,r=0;
    setup(&a,6000u,500u,620u,0u);

    assert(a.reverb_pre_cap>=5290u);
    assert(a.reverb_ap_cap[0]>300u&&a.reverb_ap_cap[1]>500u&&a.reverb_ap_cap[2]>900u);
    total=a.reverb_pre_cap+a.reverb_ap_cap[0]+a.reverb_ap_cap[1]+a.reverb_ap_cap[2];
    for(i=0u;i<8u;++i){
        total+=a.reverb_line_cap[i];
        assert(a.reverb_line_delay[i]>=3u&&a.reverb_line_delay[i]<a.reverb_line_cap[i]);
        assert((a.reverb_line_delay[i]&1u)==1u);
        if(i)assert(a.reverb_line_delay[i-1u]<a.reverb_line_delay[i]);
    }
    assert(total<=RV_TARGET_CAP);
    assert(a.reverb_feedback_q15>29000u&&a.reverb_feedback_q15<32768u);

    for(i=0u;i<IR_N;++i){
        mello_audio_reverb_stereo_probe(&a,i==0u?20000:0,&l,&r);
        assert(l>=-32767&&l<=32767&&r>=-32767&&r<=32767);
        if(i>=2000u&&i<50000u){
            double dl=(double)l,dr=(double)r;
            if(l>1||l<-1)dense_l++;
            if(r>1||r<-1)dense_r++;
            if(l!=r)different++;
            xx+=dl*dl;yy+=dr*dr;xy+=dl*dr;
        }
    }
    assert(dense_l>30000u&&dense_r>30000u&&different>30000u);
    assert(xx>1.0&&yy>1.0);
    corr2=(xy*xy)/(xx*yy);
    assert(corr2<0.04); /* |corr| < .20: real stereo late field, not mono duplication. */
    assert(yy>xx*0.35&&yy<xx*2.85); /* neither side may collapse. */
    printf("Reverb V3 stereo: dense=%u/%u corr2=%.6f energyR/L=%.3f layout=%u\n",
           (unsigned)dense_l,(unsigned)dense_r,corr2,yy/xx,(unsigned)total);
}

static int push_note(MelloAudio *a,MelloEventType type,uint32_t due)
{
    MelloEvent e;
    memset(&e,0,sizeof e);e.type=type;e.note=60u;e.velocity=100u;
    return mello_audio_push_engine_event(a,&e,due)?1:0;
}

static void rendered_stereo_contract(void)
{
    MelloAudio a;
    MelloSoundConfig sound;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sample;
    uint32_t i,diff=0u;
    int32_t out[2];
    memset(delay_buf,0,sizeof delay_buf);memset(reverb_buf,0,sizeof reverb_buf);
    mello_sound_defaults(&sound);mello_fx_defaults(&fx);mello_sub_defaults(&sub);mello_sample_defaults(&sample);
    fx.delay_wet=0u;fx.delay_level=0u;fx.dist_amount=0u;fx.reverb_wet=1000u;fx.reverb_level=1000u;fx.reverb_decay_ms=6000u;fx.reverb_size=500u;fx.reverb_tone=620u;
    mello_audio_init(&a,delay_buf,4096u,reverb_buf,RV_TARGET_CAP);
    mello_audio_set_config(&a,&sound,&fx,&sub,&sample);
    assert(push_note(&a,MELLO_EVENT_NOTE_ON,0u));assert(push_note(&a,MELLO_EVENT_NOTE_OFF,18000u));
    for(i=0u;i<18000u;++i){out[0]=out[1]=0;mello_audio_render(&a,out,1u,i*23u,80u);if(i>3000u&&out[0]!=out[1])diff++;}
    assert(diff>5000u);
}

int main(void)
{
    double dark,bright;
    structural_and_stereo_contract();
    dark=tone_roughness(150u);bright=tone_roughness(900u);
    assert(dark>0.0&&bright>dark*1.6); /* TONE changes feedback spectrum, not only output EQ. */
    printf("Reverb V3 feedback tone roughness: dark=%.5f bright=%.5f\n",dark,bright);
    rendered_stereo_contract();
    puts("PASS: WP-E3 Reverb V3 dense 8-line stereo FDN contract");
    return 0;
}
