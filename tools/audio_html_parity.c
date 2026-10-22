#include "mello_audio.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int16_t delay_buf[4096];
static int16_t reverb_buf[8192];

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
    mello_audio_init(&a,delay_buf,4096u,reverb_buf,8192u);

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

    puts("PASS: HTML audio defaults, nonlinear distortion, delay topology and long reverb parity");
    return 0;
}
