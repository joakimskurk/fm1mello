#include "mello_transport.h"
#include "mello_audio.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    MelloProject p;
    MelloTransport t;
    MelloAudio a;
    MelloSubConfig sub;
    MelloSampleConfig sample;
#if MELLO_FEATURE_CHORD_MACHINE
    MelloChordConfig chord;
#endif
    int32_t out[128];
    uint32_t now=0u;
    unsigned i;

    mello_project_defaults(&p);
    p.bpm=300u;
    p.seq_len=4u;
    for(i=0u;i<4u;++i){
        p.steps[i].pulses=4u;
        p.steps[i].gate=MELLO_GATE_MULTI;
        p.steps[i].ratchet=8u;
        p.steps[i].sub=true;
        p.steps[i].sample_slot=1u;
        p.steps[i].probability=65535u;
    }
    mello_transport_init(&t,&p,0x51A7u,0,0);
    sub=t.engine.sub;sub.on=true;sub.mode=MELLO_SUB_MODE_STEP;sub.gate_amount=950u;mello_engine_sub_set_config(&t.engine,&sub);
    sample=t.engine.sample;sample.on=true;sample.per_step=true;mello_engine_sample_set_config(&t.engine,&sample);
#if MELLO_FEATURE_CHORD_MACHINE
    chord=t.engine.chord;chord.on=true;chord.mode=MELLO_CHORD_PATTERN;chord.trigger=MELLO_CHORD_TRIGGER_STEP;chord.voices=4u;chord.pattern_length=4u;chord.pattern_masks[0]=15u;chord.pattern_masks[1]=5u;chord.pattern_masks[2]=10u;chord.pattern_masks[3]=15u;chord.gate_amount=1150u;mello_engine_chord_set_config(&t.engine,&chord);
#endif
    mello_audio_init(&a,0,0u,0,0u);
    mello_audio_set_config(&a,&t.engine.sound,&t.engine.fx,&t.engine.sub,&t.engine.sample);
#if MELLO_FEATURE_CHORD_MACHINE
    mello_audio_set_chord_config(&a,&t.engine.chord);
#endif
    mello_transport_set_event_sink(&t,mello_audio_event_sink,&a);
    mello_transport_start(&t,0u);

    /* 64-frame ISR blocks for ~8 seconds at deliberately extreme 300 BPM / 8 ratchets. */
    for(i=0u;i<5512u;++i){
        memset(out,0,sizeof out);
        mello_transport_poll(&t,now);
        mello_audio_render(&a,out,64u,now,300u);
        if(mello_audio_queue_overflowed(&a)){
            fprintf(stderr,"audio timing lanes overflow/order failure at %u us\n",(unsigned)now);
            return 1;
        }
        now+=1451u;
    }
    if(t.engine.queue_overflow){fprintf(stderr,"engine event queue overflow\n");return 2;}
    puts("PASS: 300 BPM ratchet/chord/sub/sample timing stress keeps ISR event lanes ordered");
    return 0;
}
