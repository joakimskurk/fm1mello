#include "mello_engine.h"
#include "mello_audio.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if !MELLO_FEATURE_CHORD_MACHINE
#error chord_machine_smoke requires MELLO_FEATURE_CHORD_MACHINE=1
#endif

static void drain_counts(MelloEngine *e,unsigned *on,unsigned *off){
    MelloEvent ev; *on=*off=0;
    while(mello_event_pop(e,&ev)){
        if(ev.type==MELLO_EVENT_CHORD_NOTE_ON)++*on;
        if(ev.type==MELLO_EVENT_CHORD_NOTE_OFF)++*off;
    }
}

static void push_chord(MelloAudio *a,MelloEventType type,uint8_t note){
    MelloEvent ev; memset(&ev,0,sizeof ev); ev.type=type; ev.note=note; ev.voice=MELLO_VOICE_CHORD; ev.velocity=96u;
    assert(mello_audio_push_engine_event(a,&ev,0u));
}

int main(void){
    MelloProject p; MelloEngine e; MelloChordConfig c; uint8_t n0[4],n1[4]; unsigned on,off; MelloStep before;
    MelloAudio a; MelloSoundConfig snd; MelloFxConfig fx; MelloSubConfig sub; MelloSampleConfig samp; int32_t out[2*1024]; uint32_t i; int nonzero=0;

    mello_project_defaults(&p); p.root_note=60u; p.scale=MELLO_SCALE_MINOR;
    mello_engine_init(&e,&p,0xC0DEC0DEu);
    assert(!e.chord.on);
    c=e.chord; c.on=true; c.length=4u; c.voices=4u; c.mode=MELLO_CHORD_STAB; c.trigger=MELLO_CHORD_TRIGGER_STEP;
    c.steps[0].degree=0u; c.steps[0].shape=MELLO_CHORD_SEVENTH; c.steps[0].inversion=0u; c.steps[0].octave=0; c.steps[0].spread=0u; c.steps[0].hold=1u;
    mello_engine_chord_set_config(&e,&c);
    assert(mello_engine_resolve_chord_notes(&e,0u,n0)==4u);
    assert(n0[0]==60u && n0[1]==63u && n0[2]==67u && n0[3]==70u);

    c=e.chord; c.steps[0].inversion=1u; c.steps[0].spread=1u; mello_engine_chord_set_config(&e,&c);
    assert(mello_engine_resolve_chord_notes(&e,0u,n1)==4u);
    assert(n1[0]>n0[0] && n1[3]>n0[3]);

    c=e.chord; c.steps[0].inversion=0u; c.steps[0].spread=0u; c.mode=MELLO_CHORD_STAB; c.steps[0].hold=1u; mello_engine_chord_set_config(&e,&c);
    mello_engine_start(&e); assert(mello_engine_advance_pulse(&e)); drain_counts(&e,&on,&off); assert(on==4u && off==4u);

    mello_engine_stop(&e); mello_engine_chord_reset(&e); c=e.chord; c.mode=MELLO_CHORD_HOLD; c.steps[0].hold=2u; c.steps[1].hold=2u; mello_engine_chord_set_config(&e,&c);
    mello_engine_start(&e); assert(mello_engine_advance_pulse(&e)); drain_counts(&e,&on,&off); assert(on>=3u && off==0u);
    assert(mello_engine_advance_pulse(&e)); drain_counts(&e,&on,&off); assert(on==0u && off==0u);
    assert(mello_engine_advance_pulse(&e)); drain_counts(&e,&on,&off); assert(on>=3u && off>=3u);

    mello_engine_stop(&e); mello_engine_chord_reset(&e); c=e.chord; c.mode=MELLO_CHORD_STAB; c.trigger=MELLO_CHORD_TRIGGER_GRID; c.grid_mode=MELLO_DOT_HORIZONTAL; c.grid_length=4u; c.grid_cells=0x0001u; c.grid_amount=1000u; c.grid_follow=false; c.steps[0].hold=16u; mello_engine_chord_set_config(&e,&c);
    mello_engine_start(&e); assert(mello_engine_advance_pulse(&e)); drain_counts(&e,&on,&off); assert(on>0u);
    assert(mello_engine_advance_pulse(&e)); drain_counts(&e,&on,&off); assert(on==0u);

    mello_engine_stop(&e); mello_engine_chord_reset(&e); c=e.chord; c.trigger=MELLO_CHORD_TRIGGER_STEP; c.mode=MELLO_CHORD_PATTERN; c.pattern_length=2u; c.pattern_masks[0]=0x1u; c.pattern_masks[1]=0x7u; c.steps[0].hold=16u; mello_engine_chord_set_config(&e,&c);
    mello_engine_start(&e); assert(mello_engine_advance_pulse(&e)); drain_counts(&e,&on,&off); assert(on==1u && off==1u);
    assert(mello_engine_advance_pulse(&e)); drain_counts(&e,&on,&off); assert(on==3u && off==3u);

    before=e.project.steps[0]; c=e.chord; mello_engine_set_randomize_mask(&e,MELLO_RAND_CHORD); mello_engine_randomize_steps(&e);
    assert(memcmp(&before,&e.project.steps[0],sizeof before)==0);
    assert(memcmp(&c,&e.chord,sizeof c)!=0);

    mello_sound_defaults(&snd); mello_fx_defaults(&fx); mello_sub_defaults(&sub); mello_sample_defaults(&samp); mello_chord_defaults(&c);
    snd.osc_wave=MELLO_WAVE_OFF; snd.follower_sub_on=false; sub.on=false; samp.on=false; fx.delay_wet=0u; fx.reverb_wet=0u; fx.dist_amount=0u;
    c.on=true; c.voices=4u; c.wave=MELLO_WAVE_TRIANGLE; c.level=300u; c.attack_ms=2u; c.release_ms=80u;
    mello_audio_init(&a,0,0u,0,0u); mello_audio_set_config(&a,&snd,&fx,&sub,&samp); mello_audio_set_chord_config(&a,&c);
    push_chord(&a,MELLO_EVENT_CHORD_NOTE_ON,60u); push_chord(&a,MELLO_EVENT_CHORD_NOTE_ON,63u); push_chord(&a,MELLO_EVENT_CHORD_NOTE_ON,67u); push_chord(&a,MELLO_EVENT_CHORD_NOTE_ON,70u);
    memset(out,0,sizeof out); mello_audio_render(&a,out,1024u,0u,80u); assert(a.chord.active_count==4u);
    for(i=0u;i<2u*1024u;++i){if(out[i]){nonzero=1;break;}}
    assert(nonzero);
    for(i=0u;i<4u;++i)push_chord(&a,MELLO_EVENT_CHORD_NOTE_OFF,(uint8_t[]){60u,63u,67u,70u}[i]);
    memset(out,0,sizeof out); mello_audio_render(&a,out,4096u,30000u,80u); assert(a.chord.active_count==0u);

    assert(strcmp(mello_event_type_name(MELLO_EVENT_CHORD_NOTE_ON),"CHORD_ON")==0);
    puts("PASS: removable chord machine chain/voicing/grid/pattern/random/audio smoke");
    return 0;
}
