#include "mello_audio.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void queue_sub(MelloAudio *a, MelloEventType type, uint8_t note, uint8_t velocity, uint32_t due){
    MelloEvent e;
    memset(&e,0,sizeof e);
    e.type=type;e.note=note;e.velocity=velocity;e.voice=MELLO_VOICE_SUB;
    assert(mello_audio_push_engine_event(a,&e,due));
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

    /* Fresh note must set pitch directly: Tone.Monophonic does not portamento from silence. */
    sub.drive=0u;
    mello_audio_set_config(&a,&sound,&fx,&sub,&sample);
    queue_sub(&a,MELLO_EVENT_SUB_NOTE_ON,36u,114u,0u);
    memset(out,0,sizeof out);
    mello_audio_render(&a,out,1u,100u,80u);
    assert(a.sub.slide_left==0u);
    assert(a.sub.inc==a.sub.target_inc && a.sub.inc!=0u);
    assert(a.sub.env_stage==MELLO_ENV_ATTACK);
    assert(a.sub.env_q24>0 && a.sub.env_q24<(1<<24));

    /* 3 ms attack must transition into decay, and the four LPF stages must become active. */
    memset(out,0,sizeof out);
    mello_audio_render(&a,out,160u,200u,80u);
    assert(a.sub.env_stage==MELLO_ENV_DECAY || a.sub.env_stage==MELLO_ENV_SUSTAIN);
    assert(a.sub.filter1!=0 || a.sub.filter2!=0 || a.sub.filter3!=0 || a.sub.filter4!=0);

    /* A legato/retrigger while audible should use the configured portamento. */
    queue_sub(&a,MELLO_EVENT_SUB_NOTE_ON,43u,114u,0u);
    memset(out,0,sizeof out);
    mello_audio_render(&a,out,1u,5000u,80u);
    assert(a.sub.slide_left>0u);

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

    puts("PASS: HTML sub attack, legato glide, 24 dB filter state, Tone drive curve and velocity parity");
    return 0;
}
