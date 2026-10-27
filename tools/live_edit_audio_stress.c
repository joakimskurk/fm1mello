#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mello_engine.h"
#include "mello_transport.h"
#include "mello_audio.h"

#define FRAMES 128u
#define BLOCK_US 2902u

static MelloAudio a;
static MelloTransport t;
static int16_t delay_pool[MELLO_AUDIO_DELAY_SAMPLES];
static int16_t reverb_pool[MELLO_AUDIO_REVERB_SAMPLES];
static int32_t out[FRAMES * 2u];
static int sink_failed;

static void fail(const char *msg){ fprintf(stderr,"FAIL: %s\n",msg); exit(1); }
static void midi_sink(void *ctx,uint8_t st,uint8_t d1,uint8_t d2){(void)ctx;(void)st;(void)d1;(void)d2;}
static void event_sink(void *ctx,const MelloEvent *e,uint32_t due){if(!mello_audio_push_engine_event((MelloAudio*)ctx,e,due))sink_failed=1;}
static uint64_t block_energy(void){uint64_t e=0;for(uint32_t i=0;i<FRAMES*2u;++i){int64_t v=out[i];if(v<0)v=-v;e+=(uint64_t)v;}return e;}

static void publish(void){
    mello_audio_set_config(&a,&t.engine.sound,&t.engine.fx,&t.engine.sub,&t.engine.sample);
#if MELLO_FEATURE_CHORD_MACHINE
    mello_audio_set_chord_config(&a,&t.engine.chord);
#endif
    if(a.config_generation&1u)fail("config publication left odd generation");
}

static void render_once(uint32_t now){
    mello_audio_render(&a,out,FRAMES,now,mello_transport_effective_bpm(&t));
    if(a.applied_generation!=a.config_generation)fail("audio did not apply latest stable config");
}

static void test_invalidation(void){
    uint32_t rv0=a.test_reverb_reconfigs,build0=a.test_sub_curve_builds,copy0=a.test_sub_curve_applies,apply0=a.test_config_applies;
    for(unsigned i=0;i<64u;++i){
        MelloSoundConfig s=t.engine.sound;
        s.filter_cut=(uint16_t)(200u+(i*11u)%701u);
        mello_engine_sound_set_config(&t.engine,&s);
        publish();
        render_once(i*BLOCK_US);
    }
    if(a.test_reverb_reconfigs!=rv0)fail("sound edit rebuilt reverb");
    if(a.test_sub_curve_builds!=build0||a.test_sub_curve_applies!=copy0)fail("sound edit rebuilt sub distortion curve");
    if(a.test_config_applies!=apply0+64u)fail("sound edits did not apply exactly once each");

    MelloSubConfig sub=t.engine.sub;
    sub.drive=730u;
    mello_engine_sub_set_config(&t.engine,&sub);
    publish();
    if(a.test_sub_curve_builds!=build0+1u)fail("sub drive curve was not precomputed in main context");
    if(a.test_sub_curve_applies!=copy0)fail("sub drive curve changed before audio apply");
    render_once(70u*BLOCK_US);
    if(a.test_sub_curve_applies!=copy0+1u)fail("precomputed sub drive curve was not atomically applied");
    if(a.test_reverb_reconfigs!=rv0)fail("sub drive edit rebuilt reverb");

    MelloFxConfig fx=t.engine.fx;
    fx.reverb_size=333u;
    mello_engine_fx_set_config(&t.engine,&fx);
    publish();
    render_once(71u*BLOCK_US);
    if(a.test_reverb_reconfigs!=rv0+1u)fail("reverb size edit did not reconfigure reverb exactly once");

    apply0=a.test_config_applies;
    for(unsigned i=0;i<64u;++i){
        MelloSoundConfig s=t.engine.sound;
        s.master_level=(uint16_t)(500u+(i%40u)*10u);
        mello_engine_sound_set_config(&t.engine,&s);
        publish();
    }
    if(a.config_generation&1u)fail("coalesced updates left unstable generation");
    render_once(72u*BLOCK_US);
    if(a.test_config_applies!=apply0+1u)fail("rapid pre-render edits were not coalesced into one stable apply");
    if(a.config.master_level!=t.engine.sound.master_level)fail("coalesced apply did not use newest config");
}

static void test_generation_distance(void){
    uint32_t before=a.config_generation;
    uint32_t apply0=a.test_config_applies;
    for(unsigned i=0;i<32768u;++i){
        MelloSoundConfig s=t.engine.sound;
        s.master_level=(uint16_t)(500u+(i%40u)*10u);
        mello_engine_sound_set_config(&t.engine,&s);
        publish();
    }
    if(a.config_generation&1u)fail("long coalesced publish left unstable generation");
    if(a.config_generation==before)fail("config generation aliased after 32768 queued publications");
    render_once(73u*BLOCK_US);
    if(a.test_config_applies!=apply0+1u)fail("long coalesced publication did not apply exactly once");
    if(a.config.master_level!=t.engine.sound.master_level)fail("long coalesced publication lost newest config");
}

static void mutate_live(unsigned i){
    MelloSoundConfig s=t.engine.sound;
    MelloFxConfig f=t.engine.fx;
    MelloSubConfig sub=t.engine.sub;
    MelloSampleConfig sample=t.engine.sample;
    unsigned phase=i%36u;
    switch(phase){
    case 0:s.pwm_permille=(uint16_t)(100u+(i*17u)%801u);break;
    case 1:s.filter_cut=(uint16_t)(100u+(i*29u)%901u);break;
    case 2:s.filter_res=(uint16_t)(50u+(i*31u)%851u);break;
    case 3:s.attack_ms=(uint16_t)(5u+(i*7u)%450u);break;
    case 4:s.decay_ms=(uint16_t)(20u+(i*13u)%900u);break;
    case 5:s.sustain_permille=(uint16_t)(200u+(i*19u)%701u);break;
    case 6:s.release_ms=(uint16_t)(40u+(i*23u)%1500u);break;
    case 7:s.slide_ms=(uint16_t)((i*3u)%250u);break;
    case 8:s.lfo_rate_millihz=(uint16_t)(100u+(i*97u)%15000u);break;
    case 9:s.lfo_depth=(uint16_t)((i*37u)%801u);break;
    case 10:s.follower_sub_level=(uint16_t)(100u+(i*41u)%701u);break;
    case 11:s.master_level=(uint16_t)(500u+(i*11u)%451u);break;
    case 12:f.dist_amount=(uint16_t)((i*43u)%901u);break;
    case 13:f.dist_mix=(uint16_t)(100u+(i*47u)%901u);break;
    case 14:f.dist_character=(uint16_t)((i*53u)%1001u);break;
    case 15:f.dist_tone=(uint16_t)((i*59u)%1001u);break;
    case 16:f.dist_level=(uint16_t)(500u+(i*17u)%701u);break;
    case 17:f.delay_wet=(uint16_t)((i*61u)%701u);break;
    case 18:f.delay_time_ms=(uint16_t)(40u+(i*67u)%1100u);break;
    case 19:f.delay_feedback=(uint16_t)((i*71u)%801u);break;
    case 20:f.delay_tone=(uint16_t)((i*73u)%1001u);break;
    case 21:f.delay_level=(uint16_t)(300u+(i*79u)%901u);break;
    case 22:f.reverb_wet=(uint16_t)((i*83u)%701u);break;
    case 23:f.reverb_size=(uint16_t)(100u+(i*89u)%801u);break;
    case 24:f.reverb_decay_ms=(uint16_t)(600u+(i*211u)%24000u);break;
    case 25:f.reverb_pre_ms=(uint16_t)((i*5u)%100u);break;
    case 26:f.reverb_tone=(uint16_t)((i*97u)%1001u);break;
    case 27:f.reverb_level=(uint16_t)(500u+(i*101u)%1501u);break;
    case 28:sub.gate_amount=(uint16_t)(300u+(i*13u)%801u);break;
    case 29:sub.level=(uint16_t)(200u+(i*17u)%701u);break;
    case 30:sub.decay_ms=(uint16_t)(40u+(i*19u)%600u);break;
    case 31:sub.release_ms=(uint16_t)(50u+(i*23u)%1000u);break;
    case 32:sub.glide_ms=(uint16_t)((i*3u)%180u);break;
    case 33:sub.cut=(uint16_t)(100u+(i*29u)%801u);break;
    case 34:sub.drive=(uint16_t)((i*31u)%901u);break;
    default:sample.slots[0].level=(uint16_t)(400u+(i*37u)%801u);sample.slots[0].dist=(uint16_t)((i*41u)%801u);break;
    }
    mello_engine_sound_set_config(&t.engine,&s);
    mello_engine_fx_set_config(&t.engine,&f);
    mello_engine_sub_set_config(&t.engine,&sub);
    mello_engine_sample_set_config(&t.engine,&sample);
    publish();
}

static void test_running_stress(void){
    uint32_t now=100000u,start_tick=t.engine.now_tick;
    uint64_t window=0,total=0;
    unsigned silent_windows=0;
    for(unsigned i=0;i<2400u;++i){
        mutate_live(i);
        mello_transport_poll(&t,now);
        render_once(now);
        {uint64_t e=block_energy();window+=e;total+=e;}
        if((i%150u)==149u){if(window==0u)silent_windows++;window=0u;}
        now+=BLOCK_US;
    }
    if(sink_failed||mello_audio_queue_overflowed(&a))fail("audio event queue overflowed during live edits");
    if(!t.running)fail("transport stopped during live edits");
    if(t.engine.now_tick==start_tick)fail("transport phase did not advance during live edits");
    if(silent_windows)fail("audio froze for a stress window during live edits");
    if(total==0u)fail("live-edit stress produced no audio");
    printf("live-edit energy=%llu ticks=%lu applies=%lu sub-builds=%lu reverb-reconfigs=%lu\n",
        (unsigned long long)total,(unsigned long)t.engine.now_tick,(unsigned long)a.test_config_applies,
        (unsigned long)a.test_sub_curve_builds,(unsigned long)a.test_reverb_reconfigs);
}

int main(void){
    MelloProject p;
    mello_project_defaults(&p);
    for(unsigned i=0;i<MELLO_MAX_STEPS;++i){p.steps[i].sub=true;p.steps[i].sample_slot=0u;}
    mello_transport_init(&t,&p,0x425731u,midi_sink,0);
    mello_audio_init(&a,delay_pool,MELLO_AUDIO_DELAY_SAMPLES,reverb_pool,MELLO_AUDIO_REVERB_SAMPLES);
    mello_transport_set_event_sink(&t,event_sink,&a);
    publish();
    render_once(0u);
    test_invalidation();
    test_generation_distance();
    mello_transport_start(&t,100000u);
    test_running_stress();
    puts("PASS: live rotary config publication stays stable; heavy DSP derivation is invalidated selectively and audio/transport remain live");
    return 0;
}
