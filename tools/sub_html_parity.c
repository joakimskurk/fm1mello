#include "mello_audio.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define Q24_ONE_TEST (32767 << 8)

static void queue_sub(MelloAudio *a, MelloEventType type, uint8_t note, uint8_t velocity, uint32_t due){
    MelloEvent e;
    memset(&e,0,sizeof e);
    e.type=type;e.note=note;e.velocity=velocity;e.voice=MELLO_VOICE_SUB;
    assert(mello_audio_push_engine_event(a,&e,due));
}

static void render_frames(MelloAudio *a, uint32_t frames, uint32_t start_us){
    int32_t out[2*512];
    uint32_t done=0;
    while(done<frames){
        uint32_t n=frames-done;
        if(n>512u)n=512u;
        memset(out,0,sizeof out);
        mello_audio_render(a,out,n,start_us+(done*1000000u)/44100u,80u);
        done+=n;
    }
}

int main(void){
    MelloAudio a;
    MelloSoundConfig sound;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sample;
    int32_t out[2*256];
    MelloProject p;
    MelloEngine engine;
    MelloEvent ev;
    int found=0;

    mello_sound_defaults(&sound);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sample);
    sound.osc_wave=MELLO_WAVE_OFF;
    sound.follower_sub_on=false;
    fx.delay_wet=0u;fx.reverb_wet=0u;fx.dist_amount=0u;
    sub.glide_ms=20u;
    mello_audio_init(&a,0,0u,0,0u);
    mello_audio_set_config(&a,&sound,&fx,&sub,&sample);

    /* Tone.Distortion curve at drive=1.0 peaks around 0.349 full scale. */
    sub.drive=1000u;
    mello_audio_set_config(&a,&sound,&fx,&sub,&sample);
    memset(out,0,sizeof out);
    mello_audio_render(&a,out,1u,0u,80u);
    assert(a.sub_dist_curve[256] > 11000 && a.sub_dist_curve[256] < 12000);

    /* Fresh note sets pitch directly; Tone.Monophonic does not portamento from silence. */
    sub.drive=0u;
    mello_audio_set_config(&a,&sound,&fx,&sub,&sample);
    queue_sub(&a,MELLO_EVENT_SUB_NOTE_ON,36u,114u,0u);
    memset(out,0,sizeof out);
    mello_audio_render(&a,out,1u,100u,80u);
    assert(a.sub.slide_left==0u);
    assert(a.sub.inc==a.sub.target_inc && a.sub.inc!=0u);
    assert(a.sub.env_stage==MELLO_ENV_ATTACK);
    assert(a.sub.env_q24>0 && a.sub.env_q24<Q24_ONE_TEST);

    /* 3 ms linear attack enters Tone's exponential decay. Two biquad sections must be alive. */
    render_frames(&a,160u,200u);
    assert(a.sub.env_stage==MELLO_ENV_DECAY || a.sub.env_stage==MELLO_ENV_SUSTAIN);
    assert(a.sub.biquad1_y1!=0 || a.sub.biquad1_y2!=0);
    assert(a.sub.biquad2_y1!=0 || a.sub.biquad2_y2!=0);

    /* Halfway through the 160 ms decay, exponentialApproach is already near sustain (.24),
       unlike the old linear ramp which would still be around .62. */
    render_frames(&a,3528u,5000u);
    assert(a.sub.env_q24 > (Q24_ONE_TEST*25)/100);
    assert(a.sub.env_q24 < (Q24_ONE_TEST*36)/100);

    /* A retrigger while audible uses portamento. */
    queue_sub(&a,MELLO_EVENT_SUB_NOTE_ON,43u,114u,0u);
    memset(out,0,sizeof out);
    mello_audio_render(&a,out,1u,90000u,80u);
    assert(a.sub.slide_left>0u);

    /* Let it settle, then verify Tone-style exponential release is already very low halfway in. */
    render_frames(&a,7200u,91000u);
    queue_sub(&a,MELLO_EVENT_SUB_NOTE_OFF,43u,0u,0u);
    mello_audio_render(&a,out,1u,255000u,80u);
    render_frames(&a,2646u,256000u);
    assert(a.sub.env_q24 < (Q24_ONE_TEST*5)/100);
    assert(a.sub.env_stage==MELLO_ENV_RELEASE || a.sub.env_stage==MELLO_ENV_OFF);

    /* Engine-side dedicated sub velocity matches HTML triggerSub(..., 0.9) / MIDI export 114. */
    mello_project_defaults(&p);
    p.steps[0].sub=true;
    mello_engine_init(&engine,&p,0x12345678u);
    mello_engine_start(&engine);
    assert(mello_engine_advance_pulse(&engine));
    while(mello_event_pop(&engine,&ev)){
        if(ev.type==MELLO_EVENT_SUB_NOTE_ON){assert(ev.velocity==114u);found=1;break;}
    }
    assert(found);

    puts("PASS: HTML sub biquads, exponential envelope, legato glide, Tone drive and velocity parity");
    return 0;
}
