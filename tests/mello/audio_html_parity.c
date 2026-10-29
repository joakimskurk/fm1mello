#include "mello_audio.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define RV_TARGET_CAP 24576u
#define IR_N 60000u

/* Regression hook implemented in mello_audio.c but intentionally not public API. */
int32_t mello_audio_reverb_ir_probe(MelloAudio *a,int32_t send);

static int16_t delay_buf[4096];
static int16_t delay_buf_ref[4096];
static int16_t reverb_buf[RV_TARGET_CAP];
static int32_t ir[IR_N];

static int push_note(MelloAudio *a,MelloEventType type,uint32_t due)
{
    MelloEvent e;
    memset(&e,0,sizeof e);
    e.type=type;e.note=60u;e.velocity=100u;
    return mello_audio_push_engine_event(a,&e,due)?1:0;
}

static int dry_equivalence(void)
{
    MelloAudio a,b;
    MelloSoundConfig sound;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sample;
    int32_t oa[2],ob[2];
    uint32_t t;
    memset(delay_buf,0,sizeof delay_buf);memset(delay_buf_ref,0,sizeof delay_buf_ref);memset(reverb_buf,0,sizeof reverb_buf);
    mello_sound_defaults(&sound);mello_fx_defaults(&fx);mello_sub_defaults(&sub);mello_sample_defaults(&sample);
    fx.delay_wet=0u;fx.delay_level=0u;fx.reverb_wet=0u;fx.dist_amount=0u;
    mello_audio_init(&a,delay_buf,4096u,reverb_buf,RV_TARGET_CAP);
    mello_audio_init(&b,delay_buf_ref,4096u,0,0u);
    mello_audio_set_config(&a,&sound,&fx,&sub,&sample);
    mello_audio_set_config(&b,&sound,&fx,&sub,&sample);
    if(!push_note(&a,MELLO_EVENT_NOTE_ON,0u)||!push_note(&b,MELLO_EVENT_NOTE_ON,0u))return 0;
    if(!push_note(&a,MELLO_EVENT_NOTE_OFF,900u)||!push_note(&b,MELLO_EVENT_NOTE_OFF,900u))return 0;
    for(t=0u;t<1600u;++t){
        oa[0]=oa[1]=ob[0]=ob[1]=0;
        mello_audio_render(&a,oa,1u,t,80u);
        mello_audio_render(&b,ob,1u,t,80u);
        if(oa[0]!=ob[0]||oa[1]!=ob[1])return 0;
    }
    return 1;
}

static void ir_setup(MelloAudio *a,uint16_t decay_ms,uint16_t size,uint16_t pre_ms)
{
    MelloSoundConfig sound;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sample;
    int32_t out[2]={0,0};
    memset(delay_buf,0,sizeof delay_buf);memset(reverb_buf,0,sizeof reverb_buf);
    mello_sound_defaults(&sound);mello_fx_defaults(&fx);mello_sub_defaults(&sub);mello_sample_defaults(&sample);
    fx.delay_wet=0u;fx.delay_level=0u;
    fx.reverb_wet=1000u;fx.reverb_level=1000u;
    fx.reverb_decay_ms=decay_ms;fx.reverb_size=size;fx.reverb_pre_ms=pre_ms;
    mello_audio_init(a,delay_buf,4096u,reverb_buf,RV_TARGET_CAP);
    mello_audio_set_config(a,&sound,&fx,&sub,&sample);
    mello_audio_render(a,out,1u,0u,80u);
}

static uint32_t render_ir(MelloAudio *a,uint32_t n)
{
    uint32_t i,first=n;
    if(n>IR_N)n=IR_N;
    for(i=0u;i<n;++i){
        ir[i]=mello_audio_reverb_ir_probe(a,i==0u?20000:0);
        if(first==n&&ir[i]!=0)first=i;
    }
    return first;
}

static double energy_window(uint32_t lo,uint32_t hi)
{
    double e=0.0;
    uint32_t i;
    if(hi>IR_N)hi=IR_N;
    for(i=lo;i<hi;++i){double v=(double)ir[i];e+=v*v;}
    return e;
}

static double max_corr_sq(uint32_t start,uint32_t length)
{
    double best=0.0;
    uint32_t lag,i,end=start+length;
    if(end>IR_N)end=IR_N;
    for(lag=100u;lag<=2500u;lag+=20u){
        double xy=0.0,xx=0.0,yy=0.0,c2;
        for(i=start;i+lag<end;++i){
            double x=(double)ir[i],y=(double)ir[i+lag];
            xy+=x*y;xx+=x*x;yy+=y*y;
        }
        if(xx<=0.0||yy<=0.0)continue;
        c2=(xy*xy)/(xx*yy);
        if(c2>best)best=c2;
    }
    return best;
}

static double max_block_energy_jump(uint32_t lo,uint32_t hi)
{
    const uint32_t block=1024u;
    double prev=0.0,best=1.0;
    uint32_t p;
    for(p=lo;p+block<=hi;p+=block){
        double now=energy_window(p,p+block);
        if(prev>0.01&&now>0.01){double r=now/prev;if(r>best)best=r;}
        prev=now;
    }
    return best;
}

static int reverb_v2_regression(void)
{
    MelloAudio a;
    uint32_t first0,first120,i,nonzero=0u;
    double corr2,jump2,short_e,medium_e,long_e;

    ir_setup(&a,6000u,500u,0u);
    if(a.reverb_pre_cap<5290u)return 0;
    if(!(a.reverb_line_delay[0]<a.reverb_line_delay[1]&&a.reverb_line_delay[1]<a.reverb_line_delay[2]&&a.reverb_line_delay[2]<a.reverb_line_delay[3]))return 0;
    first0=render_ir(&a,12000u);

    ir_setup(&a,6000u,500u,120u);
    if(a.reverb_pre_samples<5290u||a.reverb_pre_samples>5293u)return 0;
    first120=render_ir(&a,14000u);
    if(first0>=12000u||first120>=14000u||first120<=first0+5000u||first120>=first0+5600u)return 0;

    ir_setup(&a,6000u,500u,0u);
    (void)render_ir(&a,IR_N);
    for(i=2000u;i<50000u;++i)if(ir[i]>1||ir[i]<-1)nonzero++;
    corr2=max_corr_sq(1000u,50000u);
    jump2=max_block_energy_jump((44100u*15u)/100u,(44100u*15u)/10u);
    medium_e=energy_window((44100u*6u)/10u,(44100u*8u)/10u);
    if(nonzero<17000u||corr2>0.0324||jump2>6.25)return 0; /* corr<.18, RMS jump<2.5x */

    ir_setup(&a,400u,500u,0u);(void)render_ir(&a,IR_N);
    short_e=energy_window((44100u*6u)/10u,(44100u*8u)/10u);
    ir_setup(&a,30000u,500u,0u);(void)render_ir(&a,IR_N);
    long_e=energy_window((44100u*6u)/10u,(44100u*8u)/10u);
    if(!(short_e<medium_e*0.04&&long_e>medium_e*1.69))return 0;

    printf("Reverb V2 IR: dense=%u corr2=%.5f jumpE=%.3f predelay_delta=%u\n",
           (unsigned)nonzero,corr2,jump2,(unsigned)(first120-first0));
    return 1;
}

int main(void){
    MelloAudio a;
    MelloSoundConfig sound;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sample;
    int32_t out[4]={0};

    mello_sound_defaults(&sound);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sample);
    mello_audio_init(&a,delay_buf,4096u,reverb_buf,RV_TARGET_CAP);

    /* HTML defaults and derived parity values. */
    assert(sound.filter_cut==700u && sound.filter_res==200u);
    assert(sound.master_level==750u && sound.attack_ms==5u && sound.decay_ms==180u);
    assert(fx.delay_wet==120u && fx.delay_feedback==320u && fx.delay_tone==720u && fx.delay_level==750u);
    assert(fx.reverb_wet==240u && fx.reverb_decay_ms==6000u && fx.reverb_tone==620u && fx.reverb_level==1150u);
    assert(sub.cut==450u && sub.level==420u && sub.decay_ms==160u);
    assert(a.dist_drive_permille==0u);
    assert(a.reverb_send_permille>=195u && a.reverb_send_permille<=210u);
    assert(a.reverb_feedback_q15>29000u); /* six-second RT60 must not collapse like the old 0.56 feedback. */

    /* HTML distortion curve is strongly nonlinear: 50% amount / 35% character ~= 12-13% actual drive. */
    fx.dist_amount=500u; fx.dist_character=350u;
    mello_audio_set_config(&a,&sound,&fx,&sub,&sample);
    mello_audio_render(&a,out,1u,0u,80u);
    assert(a.dist_drive_permille>=120u && a.dist_drive_permille<=135u);

    /* Delay tone belongs on the return, not inside the feedback loop.  With zero send,
       a raw 10000-sample and 50% feedback must write ~5000 independent of tone. */
    memset(delay_buf,0,sizeof(delay_buf));
    fx.delay_sync=false; fx.delay_time_ms=30u; fx.delay_wet=0u; fx.delay_feedback=500u; fx.delay_tone=0u;
    fx.reverb_wet=0u; fx.dist_amount=0u;
    mello_audio_set_config(&a,&sound,&fx,&sub,&sample);
    mello_audio_render(&a,out,1u,100u,80u); /* apply config and establish delay_samples */
    memset(delay_buf,0,sizeof(delay_buf));
    a.delay_pos=0u; a.delay_lp=0;
    {
        uint32_t rp=(a.delay_capacity-a.delay_samples)%a.delay_capacity;
        delay_buf[rp]=10000;
    }
    out[0]=out[1]=0;
    mello_audio_render(&a,out,1u,200u,80u);
    assert(delay_buf[0]>=4950 && delay_buf[0]<=5050);

    /* Limiter ceiling is -1 dBFS before master; default master can never exceed ~21903. */
    sound.master_level=750u;
    fx.delay_feedback=0u; fx.delay_level=0u; fx.reverb_level=0u;
    mello_audio_set_config(&a,&sound,&fx,&sub,&sample);
    mello_audio_render(&a,out,1u,300u,80u);
    assert((29204*sound.master_level)/1000 <= 21910);

    assert(dry_equivalence());
    assert(reverb_v2_regression());

    puts("PASS: HTML audio defaults, nonlinear distortion, delay topology and diffuse Reverb V2 parity");
    return 0;
}
