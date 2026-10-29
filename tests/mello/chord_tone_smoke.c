#include "mello_audio.h"
#include <stdio.h>
#include <string.h>

#if MELLO_FEATURE_CHORD_MACHINE
static void note_on(MelloAudio *a,uint8_t note)
{
    MelloEvent e;memset(&e,0,sizeof e);e.type=MELLO_EVENT_CHORD_NOTE_ON;e.note=note;e.velocity=88u;
    if(!mello_audio_push_engine_event(a,&e,0u))fprintf(stderr,"chord queue push failed\n");
}
static long long energy(const int32_t *out,unsigned frames)
{
    unsigned i;long long sum=0;for(i=0;i<frames;++i){int32_t v=out[i*2u];if(v<0)v=-v;sum+=v;}return sum;
}
static void render_chord(uint16_t cut,uint16_t dist,int32_t *out,unsigned frames)
{
    MelloAudio a;MelloSoundConfig s;MelloFxConfig f;MelloSubConfig sub;MelloSampleConfig sm;MelloChordConfig c;
    mello_sound_defaults(&s);mello_fx_defaults(&f);mello_sub_defaults(&sub);mello_sample_defaults(&sm);mello_chord_defaults(&c);
    s.osc_wave=MELLO_WAVE_OFF;s.follower_sub_on=false;s.filter_cut=cut;sub.on=false;sm.on=false;
    f.delay_wet=0u;f.reverb_wet=0u;f.dist_amount=dist;f.dist_mix=dist;f.dist_level=1000u;c.on=true;
    mello_audio_init(&a,0,0u,0,0u);mello_audio_set_config(&a,&s,&f,&sub,&sm);mello_audio_set_chord_config(&a,&c);
    note_on(&a,48u);note_on(&a,55u);note_on(&a,64u);
    memset(out,0,(size_t)frames*2u*sizeof(*out));mello_audio_render(&a,out,frames,0u,120u);
}
#endif

int main(void)
{
#if MELLO_FEATURE_CHORD_MACHINE
    enum{N=4096};static int32_t low[N*2],high[N*2];MelloChordConfig c;long long e;
    mello_chord_defaults(&c);
    if(c.voices!=3u||c.wave!=MELLO_WAVE_SAW||c.steps[0].shape!=MELLO_CHORD_TRIAD||c.steps[0].octave!=-1||c.steps[0].spread!=1u){
        fprintf(stderr,"chord bed defaults regressed\n");return 1;
    }
    render_chord(0u,0u,low,N);render_chord(1000u,1000u,high,N);
    if(memcmp(low,high,sizeof low)!=0){fprintf(stderr,"main FILTER/DIST still changes chord timbre\n");return 2;}
    e=energy(low,N);if(e<800000LL){fprintf(stderr,"chord bed too weak: energy=%lld\n",e);return 3;}
    puts("PASS: chord defaults form an open mid-bed and remain spectrally independent of main FILTER/DIST");
#else
    puts("PASS: chord tone smoke skipped with chord feature compiled out");
#endif
    return 0;
}
