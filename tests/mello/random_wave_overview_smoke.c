#include "mello_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fills,texts,bounds,random_skyline_steps,pitch_bars,wave_label,intensity_label,ratchet_marks,event_marks,home_columns;
static int min_cell_w=999,max_cell_w=0,min_pitch_y=999,max_pitch_y=0;

static void reset_counts(void)
{
    fills=texts=bounds=random_skyline_steps=pitch_bars=wave_label=intensity_label=ratchet_marks=event_marks=home_columns=0;
    min_cell_w=999;max_cell_w=0;min_pitch_y=999;max_pitch_y=0;
}

static void fill_cb(void *ctx,int16_t x,int16_t y,int16_t w,int16_t h,uint16_t colour)
{
    (void)ctx;(void)colour;fills++;
    if(x<0||y<0||w<0||h<0||x+w>240||y+h>240)bounds=1;
    if(y==132&&h==27&&x>=8&&x<232){random_skyline_steps++;if(w<min_cell_w)min_cell_w=w;if(w>max_cell_w)max_cell_w=w;}
    if(y>=131&&y<=153&&h>=5&&h<=27&&x>=8&&x<232){pitch_bars++;if(y<min_pitch_y)min_pitch_y=y;if(y>max_pitch_y)max_pitch_y=y;}
    if(y==163&&h==1)ratchet_marks++;
    if((y==165||y==168)&&h==1)event_marks++;
    if(y==51&&h==93&&x>=8&&x<232){home_columns++;if(w>7)bounds=1;}
}

static void text_cb(void *ctx,int16_t x,int16_t y,int16_t w,uint8_t large,const char *text,uint16_t colour,uint8_t align)
{
    (void)ctx;(void)x;(void)y;(void)w;(void)large;(void)colour;(void)align;texts++;
    if(text&&strcmp(text,"WAVE")==0)wave_label=1;
    if(text&&(strcmp(text,"LOW")==0||strcmp(text,"MED")==0||strcmp(text,"HIGH")==0))intensity_label=1;
}

static void die(const char *s){fprintf(stderr,"FAIL: %s\n",s);exit(1);}

static unsigned long long step_distance(const MelloEngine *a,const MelloEngine *b)
{
    unsigned long long d=0;unsigned i;
    for(i=0;i<MELLO_MAX_STEPS;i++){
        const MelloStep *x=&a->project.steps[i],*y=&b->project.steps[i];
        d+=(unsigned long long)abs((int)x->pitch-(int)y->pitch)*8u;
        d+=(unsigned long long)abs((int)x->pulses-(int)y->pulses)*6u;
        d+=(unsigned long long)abs((int)x->ratchet-(int)y->ratchet)*6u;
        d+=(x->gate!=y->gate)*12u;
        d+=(unsigned long long)abs((int)x->octave-(int)y->octave)*8u;
        d+=(x->accent!=y->accent)*5u+(x->sub!=y->sub)*5u+(x->slide!=y->slide)*5u+(x->skip!=y->skip)*5u;
        d+=(unsigned long long)(x->probability>y->probability?x->probability-y->probability:y->probability-x->probability)/1024u;
        d+=(unsigned long long)abs((int)x->sample_slot-(int)y->sample_slot)*4u;
    }
    d+=(a->sound.osc_wave!=b->sound.osc_wave)*20u;
    d+=(unsigned long long)(a->sound.pwm_permille>b->sound.pwm_permille?a->sound.pwm_permille-b->sound.pwm_permille:b->sound.pwm_permille-a->sound.pwm_permille)/10u;
#if MELLO_FEATURE_CHORD_MACHINE
    for(i=0;i<a->chord.length&&i<MELLO_CHORD_MAX_STEPS;i++){
        const MelloChordStep *x=&a->chord.steps[i],*y=&b->chord.steps[i];
        d+=(unsigned long long)abs((int)x->degree-(int)y->degree)*6u+(x->shape!=y->shape)*8u+(unsigned long long)abs((int)x->inversion-(int)y->inversion)*4u+(unsigned long long)abs((int)x->octave-(int)y->octave)*6u+(unsigned long long)abs((int)x->spread-(int)y->spread)*4u+(unsigned long long)abs((int)x->hold-(int)y->hold)*2u;
    }
#endif
    return d;
}

static int steps_equal(const MelloProject *a,const MelloProject *b){return memcmp(a->steps,b->steps,sizeof a->steps)==0;}

static int unrelated_step_fields_equal(const MelloStep *a,const MelloStep *b,uint16_t target)
{
    if(target!=MELLO_RAND_PITCH&&a->pitch!=b->pitch)return 0;
    if(target!=MELLO_RAND_PULSES&&a->pulses!=b->pulses)return 0;
    if(target!=MELLO_RAND_RATCHET&&a->ratchet!=b->ratchet)return 0;
    if(target!=MELLO_RAND_GATE&&a->gate!=b->gate)return 0;
    if(target!=MELLO_RAND_OCTAVE&&a->octave!=b->octave)return 0;
    if(target!=MELLO_RAND_SLIDE&&a->slide!=b->slide)return 0;
    if(target!=MELLO_RAND_SKIP&&a->skip!=b->skip)return 0;
    if(target!=MELLO_RAND_ACCENT&&a->accent!=b->accent)return 0;
    if(target!=MELLO_RAND_SUB&&a->sub!=b->sub)return 0;
    if(target!=MELLO_RAND_SAMPLE&&a->sample_slot!=b->sample_slot)return 0;
    if(target!=MELLO_RAND_PROB&&a->probability!=b->probability)return 0;
    return 1;
}

static void check_legal(const MelloEngine *e)
{
    unsigned i;
    for(i=0;i<MELLO_MAX_STEPS;i++){
        const MelloStep *s=&e->project.steps[i];
        if(s->pulses<1||s->pulses>8||s->ratchet<1||s->ratchet>8||s->gate>MELLO_GATE_MULTI||s->octave<-2||s->octave>2||s->sample_slot>MELLO_SAMPLE_SLOT_COUNT)die("illegal randomized step");
    }
    if(e->sound.osc_wave>MELLO_WAVE_OFF||e->sound.pwm_permille<50||e->sound.pwm_permille>950)die("illegal randomized sound");
}

static void check_single_target_isolation(void)
{
    static const uint16_t step_targets[]={MELLO_RAND_PITCH,MELLO_RAND_PULSES,MELLO_RAND_RATCHET,MELLO_RAND_GATE,MELLO_RAND_OCTAVE,MELLO_RAND_ACCENT,MELLO_RAND_SUB,MELLO_RAND_SLIDE,MELLO_RAND_SKIP,MELLO_RAND_PROB,MELLO_RAND_SAMPLE};
    unsigned ti,intensity,i;
    for(ti=0;ti<sizeof step_targets/sizeof step_targets[0];++ti){
        for(intensity=0;intensity<3;++intensity){
            MelloEngine e,b;MelloSoundConfig snd;
            mello_engine_init(&e,0,0x510000u+(uint32_t)ti*17u+intensity);b=e;snd=e.sound;
            mello_engine_set_randomize_mask(&e,step_targets[ti]);
            mello_engine_randomize_steps_intensity(&e,(MelloRandomizeIntensity)intensity);
            if(e.randomize.mask!=step_targets[ti])die("single-target mask drift");
            for(i=0;i<MELLO_MAX_STEPS;++i)if(!unrelated_step_fields_equal(&b.project.steps[i],&e.project.steps[i],step_targets[ti]))die("single-target leaked into unrelated step field");
            if(memcmp(&snd,&e.sound,sizeof snd))die("step target leaked into sound");
#if MELLO_FEATURE_CHORD_MACHINE
            if(memcmp(&b.chord,&e.chord,sizeof b.chord))die("step target leaked into chord");
#endif
        }
    }
    for(intensity=0;intensity<3;++intensity){
        MelloEngine e,b;MelloSoundConfig before,after;
        mello_engine_init(&e,0,0x520000u+intensity);b=e;before=e.sound;
        mello_engine_set_randomize_mask(&e,MELLO_RAND_WAVE);mello_engine_randomize_steps_intensity(&e,(MelloRandomizeIntensity)intensity);
        if(!steps_equal(&e.project,&b.project))die("WAVE target modified steps");
        after=e.sound;before.osc_wave=after.osc_wave;before.pwm_permille=after.pwm_permille;
        if(memcmp(&before,&after,sizeof before))die("WAVE target leaked into unrelated sound field");
    }
#if MELLO_FEATURE_CHORD_MACHINE
    for(intensity=0;intensity<3;++intensity){
        MelloEngine e,b;mello_engine_init(&e,0,0x530000u+intensity);b=e;
        mello_engine_set_randomize_mask(&e,MELLO_RAND_CHORD);mello_engine_randomize_steps_intensity(&e,(MelloRandomizeIntensity)intensity);
        if(memcmp(&b.project,&e.project,sizeof b.project))die("CHORD target leaked into project");
        if(memcmp(&b.sound,&e.sound,sizeof b.sound))die("CHORD target leaked into sound");
    }
#endif
}

static void check_legacy_wave_isolation(void)
{
    MelloEngine e;MelloProject before;MelloSoundConfig snd_before,snd_after;unsigned i;int changed=0;
    mello_engine_init(&e,0,1234u);before=e.project;snd_before=e.sound;mello_engine_set_randomize_mask(&e,MELLO_RAND_WAVE);
    for(i=0;i<24u;++i){MelloWaveform w=e.sound.osc_wave;mello_engine_randomize_steps(&e);if(e.sound.osc_wave!=w)changed=1;}
    if(!changed)die("legacy MED WAVE never changed waveform");
    if(memcmp(&before,&e.project,sizeof before)!=0)die("legacy MED WAVE modified project");
    snd_after=e.sound;snd_before.osc_wave=snd_after.osc_wave;snd_before.pwm_permille=snd_after.pwm_permille;
    if(memcmp(&snd_before,&snd_after,sizeof snd_before)!=0)die("legacy MED WAVE changed unrelated synth parameter");
    if(e.sound.osc_wave==MELLO_WAVE_OFF)die("legacy MED WAVE selected OFF");
}

static void check_intensity_semantics(void)
{
    unsigned seed;unsigned long long dl=0,dm=0,dh=0;
    for(seed=1;seed<=256;seed++){
        MelloEngine base,old,med,lo,hi,lo2;
        mello_engine_init(&base,0,seed*7919u);old=base;med=base;lo=base;hi=base;lo2=base;
        mello_engine_set_randomize_mask(&old,MELLO_RAND_ALL);mello_engine_set_randomize_mask(&med,MELLO_RAND_ALL);mello_engine_set_randomize_mask(&lo,MELLO_RAND_ALL);mello_engine_set_randomize_mask(&hi,MELLO_RAND_ALL);mello_engine_set_randomize_mask(&lo2,MELLO_RAND_ALL);
        mello_engine_randomize_steps(&old);mello_engine_randomize_steps_intensity(&med,MELLO_RANDOMIZE_MED);mello_engine_randomize_steps_intensity(&lo,MELLO_RANDOMIZE_LOW);mello_engine_randomize_steps_intensity(&hi,MELLO_RANDOMIZE_HIGH);mello_engine_randomize_steps_intensity(&lo2,MELLO_RANDOMIZE_LOW);
        if(memcmp(&old.project,&med.project,sizeof old.project)||memcmp(&old.sound,&med.sound,sizeof old.sound)||old.gen_rng.state!=med.gen_rng.state)die("MED compatibility drift");
#if MELLO_FEATURE_CHORD_MACHINE
        if(memcmp(&old.chord,&med.chord,sizeof old.chord))die("MED chord compatibility drift");
#endif
        if(memcmp(&lo.project,&lo2.project,sizeof lo.project)||memcmp(&lo.sound,&lo2.sound,sizeof lo.sound)||lo.gen_rng.state!=lo2.gen_rng.state)die("LOW nondeterministic");
        if(lo.randomize.mask!=MELLO_RAND_ALL||med.randomize.mask!=MELLO_RAND_ALL||hi.randomize.mask!=MELLO_RAND_ALL)die("intensity changed target mask");
        check_legal(&lo);check_legal(&med);check_legal(&hi);
        dl+=step_distance(&base,&lo);dm+=step_distance(&base,&med);dh+=step_distance(&base,&hi);
    }
    printf("RANDOM intensity distance: LOW=%llu MED=%llu HIGH=%llu\n",dl,dm,dh);
    if(!(dl<dm&&dm<dh))die("aggregate intensity ordering");
    check_single_target_isolation();
}

static void render_random_length(unsigned n)
{
    MelloProject p;MelloTransport t;MelloStorage st;MelloUi ui;MelloUiDrawOps ops={fill_cb,text_cb,0,0,0};unsigned i;
    mello_project_defaults(&p);p.seq_len=(uint8_t)n;
    for(i=0;i<n;i++){p.steps[i].pitch=(int8_t)((i*3u)%14u);p.steps[i].pulses=(uint8_t)(1u+(i%8u));p.steps[i].ratchet=(uint8_t)(1u+(i%4u));p.steps[i].accent=(i%3u)==0;p.steps[i].sub=(i%5u)==0;p.steps[i].slide=(i%7u)==0;p.steps[i].skip=(i%11u)==0;}
    mello_transport_init(&t,&p,77u,0,0);mello_storage_init(&st,0,0);mello_ui_init(&ui,&t,&st);mello_ui_set_page(&ui,MELLO_UI_RANDOM);ui.dirty=MELLO_UI_DIRTY_ALL;
    reset_counts();mello_ui_render(&ui,&ops);
    if(bounds)die("RANDOM render out of bounds");
    if(random_skyline_steps!=(int)n)die("RANDOM skyline lost active step");
    if(pitch_bars<(int)n)die("RANDOM pitch contour incomplete");
    if(max_pitch_y-min_pitch_y<5)die("RANDOM pitch contour not recognizable");
    if(!wave_label||!intensity_label)die("RANDOM controls missing");
    if(n==16u&&min_cell_w<12)die("16-step skyline cells too narrow");
    if(n==32u&&min_cell_w<5)die("32-step skyline cells too narrow");
    if(ratchet_marks==0||event_marks==0)die("RANDOM structural cues missing");
}

static void check_ui_contract(void)
{
    MelloProject p;MelloTransport t;MelloStorage st;MelloUi ui;MelloUiDrawOps ops={fill_cb,text_cb,0,0,0};int16_t enc[MELLO_UI_ENCODER_COUNT]={0};MelloEngine expected;unsigned n;
    mello_project_defaults(&p);p.seq_len=32u;mello_transport_init(&t,&p,77u,0,0);mello_storage_init(&st,0,0);mello_ui_init(&ui,&t,&st);
    mello_ui_set_page(&ui,MELLO_UI_HOME);ui.dirty=MELLO_UI_DIRTY_ALL;reset_counts();mello_ui_render(&ui,&ops);
    if(bounds||home_columns<32)die("HOME lost 32-step overview");
    for(n=4;n<=32;n++)render_random_length(n);

    mello_project_defaults(&p);p.seq_len=16u;mello_transport_init(&t,&p,1234u,0,0);mello_storage_init(&st,0,0);mello_ui_init(&ui,&t,&st);mello_ui_set_page(&ui,MELLO_UI_RANDOM);
    if(ui.random_intensity!=MELLO_RANDOMIZE_MED)die("RANDOM default intensity is not MED");
    enc[MELLO_UI_ENC_KNOB1]=1;mello_ui_input(&ui,0,0,0,0,0,0,enc);
    if(ui.random_intensity!=MELLO_RANDOMIZE_HIGH)die("R1 did not select HIGH");
    expected=t.engine;mello_engine_randomize_steps_intensity(&expected,MELLO_RANDOMIZE_HIGH);
    mello_ui_input(&ui,0,1,0,0,0,1u<<3,0);
    if(memcmp(&expected.project,&t.engine.project,sizeof expected.project)||memcmp(&expected.sound,&t.engine.sound,sizeof expected.sound)||expected.gen_rng.state!=t.engine.gen_rng.state)die("B1 reroll did not use selected intensity");
    enc[MELLO_UI_ENC_KNOB1]=-1;mello_ui_input(&ui,0,2,0,0,0,0,enc);mello_ui_input(&ui,0,3,0,0,0,0,enc);
    if(ui.random_intensity!=MELLO_RANDOMIZE_LOW)die("R1 did not select LOW");
    ui.home_old_step=0;ui.home_new_step=1;t.display_step=1;ui.selected_step=0;ui.dirty=MELLO_UI_DIRTY_SEQ_PLAYHEAD;reset_counts();mello_ui_render(&ui,&ops);
    printf("RANDOM incremental redraw: fills=%d texts=%d\n",fills,texts);
    if(fills>9||texts>1)die("RANDOM redraw budget regression");
}

int main(void)
{
    check_legacy_wave_isolation();
    check_intensity_semantics();
    check_ui_contract();
    puts("PASS: LOW/MED/HIGH randomizer semantics + WAVE isolation + 4..32 RANDOM skyline + incremental redraw");
    return 0;
}
