#include "mello_ui.h"
#include <stdio.h>
#include <string.h>

static int random_strip_steps;
static int home_columns;
static int bounds_error;
static int saw_wave_label;

static void fill_cb(void *ctx,int16_t x,int16_t y,int16_t w,int16_t h,uint16_t colour)
{
    (void)ctx;(void)colour;
    if(x<0||y<0||w<0||h<0||x+w>240||y+h>240) bounds_error=1;
    if(y==166&&h==4&&x>=8&&x<232) random_strip_steps++;
    if(y==51&&h==93&&x>=8&&x<232){home_columns++;if(w>7){fprintf(stderr,"HOME 32-step column too wide: %d\n",w);bounds_error=1;}}
}
static void text_cb(void *ctx,int16_t x,int16_t y,int16_t w,uint8_t large,const char *text,uint16_t colour,uint8_t align)
{
    (void)ctx;(void)x;(void)y;(void)w;(void)large;(void)colour;(void)align;
    if(text&&strcmp(text,"WAVE")==0)saw_wave_label=1;
}

int main(void)
{
    MelloEngine e;
    MelloProject before;
    MelloSoundConfig snd_before, snd_after;
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi ui;
    MelloUiDrawOps ops={fill_cb,text_cb,0,0,0};
    unsigned i;
    int changed=0;

    mello_engine_init(&e,0,1234u);
    before=e.project;snd_before=e.sound;
    mello_engine_set_randomize_mask(&e,MELLO_RAND_WAVE);
    for(i=0;i<24u;++i){MelloWaveform w=e.sound.osc_wave;mello_engine_randomize_steps(&e);if(e.sound.osc_wave!=w)changed=1;}
    if(!changed){fprintf(stderr,"wave randomizer never changed waveform\n");return 1;}
    if(memcmp(&before,&e.project,sizeof before)!=0){fprintf(stderr,"WAVE target modified sequence/project data\n");return 2;}
    snd_after=e.sound;
    snd_before.osc_wave=snd_after.osc_wave;
    snd_before.pwm_permille=snd_after.pwm_permille;
    if(memcmp(&snd_before,&snd_after,sizeof snd_before)!=0){fprintf(stderr,"WAVE target changed unrelated synth parameters\n");return 3;}
    if(e.sound.osc_wave==MELLO_WAVE_OFF){fprintf(stderr,"WAVE target selected OFF\n");return 4;}

    mello_project_defaults(&p);p.seq_len=32u;
    for(i=0;i<32u;++i){p.steps[i].pitch=(int8_t)(i%14u);p.steps[i].gate=MELLO_GATE_SINGLE;}
    mello_transport_init(&t,&p,77u,0,0);mello_storage_init(&st,0,0);mello_ui_init(&ui,&t,&st);

    mello_ui_set_page(&ui,MELLO_UI_HOME);ui.dirty|=MELLO_UI_DIRTY_ALL;mello_ui_render(&ui,&ops);
    if(bounds_error||home_columns<32){fprintf(stderr,"HOME did not render all 32 steps: %d\n",home_columns);return 5;}

    random_strip_steps=0;saw_wave_label=0;bounds_error=0;
    mello_ui_set_page(&ui,MELLO_UI_RANDOM);ui.dirty|=MELLO_UI_DIRTY_ALL;mello_ui_render(&ui,&ops);
    if(bounds_error||random_strip_steps<32){fprintf(stderr,"RANDOM did not render all 32 steps: %d\n",random_strip_steps);return 6;}
    if(!saw_wave_label){fprintf(stderr,"RANDOM missing WAVE target\n");return 7;}

    puts("PASS: WAVE randomization is isolated and HOME/RANDOM keep all 32 steps visible");
    return 0;
}
