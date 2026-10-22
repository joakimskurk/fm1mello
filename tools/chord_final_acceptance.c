#include "mello_engine.h"
#include "mello_audio.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if MELLO_FEATURE_CHORD_MACHINE

typedef struct {
    unsigned on;
    unsigned off;
    uint8_t seen[128];
} ArpStats;

static void drain_arp(MelloEngine *e,ArpStats *s,unsigned *pulse_on)
{
    MelloEvent ev;
    *pulse_on=0u;
    while(mello_event_pop(e,&ev)){
        if(ev.type==MELLO_EVENT_CHORD_NOTE_ON){
            ++s->on;
            ++*pulse_on;
            if(ev.note<128u)s->seen[ev.note]=1u;
        }else if(ev.type==MELLO_EVENT_CHORD_NOTE_OFF){
            ++s->off;
        }
    }
}

static int arp_acceptance(void)
{
    MelloProject p;
    MelloEngine e;
    MelloChordConfig c;
    ArpStats stats;
    unsigned pulse,pulse_on,distinct=0u;
    uint8_t note;

    memset(&stats,0,sizeof stats);
    mello_project_defaults(&p);
    p.root_note=60u;
    p.scale=MELLO_SCALE_MINOR;
    mello_engine_init(&e,&p,0x41525031u);
    mello_chord_defaults(&c);
    c.on=true;
    c.length=1u;
    c.voices=3u;
    c.mode=MELLO_CHORD_ARP;
    c.trigger=MELLO_CHORD_TRIGGER_STEP;
    c.steps[0].degree=0u;
    c.steps[0].shape=MELLO_CHORD_TRIAD;
    c.steps[0].inversion=0u;
    c.steps[0].octave=0;
    c.steps[0].spread=0u;
    c.steps[0].hold=1u;
    mello_engine_chord_set_config(&e,&c);
    mello_engine_start(&e);

    for(pulse=0u;pulse<12u;++pulse){
        if(!mello_engine_advance_pulse(&e)){
            fprintf(stderr,"ARP: transport failed to advance on pulse %u\n",pulse);
            return 0;
        }
        drain_arp(&e,&stats,&pulse_on);
        if(pulse_on>1u){
            fprintf(stderr,"ARP: emitted %u simultaneous note-ons on pulse %u\n",pulse_on,pulse);
            return 0;
        }
    }
    for(note=0u;note<128u;++note)if(stats.seen[note])++distinct;
    if(stats.on<3u||distinct<2u){
        fprintf(stderr,"ARP: insufficient traversal: ons=%u distinct=%u offs=%u\n",stats.on,distinct,stats.off);
        return 0;
    }
    printf("Chord ARP: ons=%u offs=%u distinct=%u, max one attack/pulse\n",stats.on,stats.off,distinct);
    return 1;
}

#define RV_CAP 24576u
#define DELAY_CAP 4096u
#define CHUNK 1024u
static int16_t delay_buf[DELAY_CAP];
static int16_t reverb_buf[RV_CAP];
static int32_t audio_out[CHUNK*2u];

static int push_chord(MelloAudio *a,MelloEventType type,uint8_t note)
{
    MelloEvent ev;
    memset(&ev,0,sizeof ev);
    ev.type=type;
    ev.voice=MELLO_VOICE_CHORD;
    ev.note=note;
    ev.velocity=92u;
    return mello_audio_push_engine_event(a,&ev,0u)?1:0;
}

static unsigned long long abs_energy(const int32_t *out,unsigned frames)
{
    unsigned i;
    unsigned long long sum=0u;
    for(i=0u;i<frames;++i){
        int64_t v=out[i*2u];
        if(v<0)v=-v;
        sum+=(unsigned long long)v;
    }
    return sum;
}

static unsigned long long render_late_tail(uint16_t reverb_wet,unsigned *nonzero_out)
{
    MelloAudio a;
    MelloSoundConfig sound;
    MelloFxConfig fx;
    MelloSubConfig sub;
    MelloSampleConfig sample;
    MelloChordConfig chord;
    const uint8_t notes[3]={48u,55u,64u};
    unsigned i,j,nonzero=0u;
    unsigned long long late=0u;

    memset(delay_buf,0,sizeof delay_buf);
    memset(reverb_buf,0,sizeof reverb_buf);
    memset(audio_out,0,sizeof audio_out);
    mello_sound_defaults(&sound);
    mello_fx_defaults(&fx);
    mello_sub_defaults(&sub);
    mello_sample_defaults(&sample);
    mello_chord_defaults(&chord);

    sound.osc_wave=MELLO_WAVE_OFF;
    sound.follower_sub_on=false;
    sub.on=false;
    sample.on=false;
    fx.dist_amount=0u;
    fx.delay_wet=0u;
    fx.delay_level=0u;
    fx.reverb_wet=reverb_wet;
    fx.reverb_level=1000u;
    fx.reverb_decay_ms=6000u;
    fx.reverb_pre_ms=0u;
    fx.reverb_tone=620u;
    chord.on=true;
    chord.voices=3u;
    chord.wave=MELLO_WAVE_SAW;
    chord.level=360u;
    chord.attack_ms=2u;
    chord.release_ms=20u;

    mello_audio_init(&a,delay_buf,DELAY_CAP,reverb_buf,RV_CAP);
    mello_audio_set_config(&a,&sound,&fx,&sub,&sample);
    mello_audio_set_chord_config(&a,&chord);
    for(i=0u;i<3u;++i)if(!push_chord(&a,MELLO_EVENT_CHORD_NOTE_ON,notes[i]))return 0u;

    for(i=0u;i<4u;++i){
        memset(audio_out,0,sizeof audio_out);
        mello_audio_render(&a,audio_out,CHUNK,i*CHUNK,80u);
    }
    for(i=0u;i<3u;++i)if(!push_chord(&a,MELLO_EVENT_CHORD_NOTE_OFF,notes[i]))return 0u;

    /* Four chunks are well beyond the 20 ms voice release. They also let the
       FDN become the sole source before the measured late-tail window. */
    for(i=0u;i<4u;++i){
        memset(audio_out,0,sizeof audio_out);
        mello_audio_render(&a,audio_out,CHUNK,(4u+i)*CHUNK,80u);
    }
    if(a.chord.active_count!=0u){
        fprintf(stderr,"Chord release did not reach idle: active=%u\n",(unsigned)a.chord.active_count);
        return 0u;
    }

    for(i=0u;i<4u;++i){
        memset(audio_out,0,sizeof audio_out);
        mello_audio_render(&a,audio_out,CHUNK,(8u+i)*CHUNK,80u);
        late+=abs_energy(audio_out,CHUNK);
        for(j=0u;j<CHUNK*2u;++j)if(audio_out[j]!=0)++nonzero;
    }
    if(nonzero_out)*nonzero_out=nonzero;
    return late;
}

static int shared_room_acceptance(void)
{
    unsigned dry_nz=0u,wet_nz=0u;
    unsigned long long dry=render_late_tail(0u,&dry_nz);
    unsigned long long wet=render_late_tail(1000u,&wet_nz);

    if(wet_nz<512u||wet<=dry+2000u){
        fprintf(stderr,"Chord/Reverb V2 shared-room tail failed: dry=%llu/%u wet=%llu/%u\n",
                dry,dry_nz,wet,wet_nz);
        return 0;
    }
    printf("Chord/Reverb V2 tail: dry=%llu/%u wet=%llu/%u\n",dry,dry_nz,wet,wet_nz);
    return 1;
}
#endif

int main(void)
{
#if MELLO_FEATURE_CHORD_MACHINE
    if(!arp_acceptance())return 1;
    if(!shared_room_acceptance())return 2;
    puts("PASS: final chord technical acceptance — ARP is monophonic/traversing and chord inhabits Reverb V2 shared room");
#else
    puts("PASS: final chord acceptance cleanly compiles out with MELLO_FEATURE_CHORD_MACHINE=0");
#endif
    return 0;
}
