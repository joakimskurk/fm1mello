#include "mello_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char hint[64]; } Capture;
typedef struct { const char *name; MelloUiPage page; const char *hint; } MapCase;

static const MapCase cases[] = {
    {"HOME",MELLO_UI_HOME,"B5:RANDOM"},
    {"STEP",MELLO_UI_SEQ_STEP,"B4:SKIP B5:RANDOM"},
    {"SEGMENT",MELLO_UI_SEQ_SEGMENT,"B5:RANDOM"},
    {"LOOP",MELLO_UI_SEQ_LOOP,"B2:RST B5:RANDOM"},
    {"SLOTSEQ",MELLO_UI_SEQ_SLOTSEQ,"B5:RANDOM"},
    {"RANDOM",MELLO_UI_RANDOM,"B1:RND B3:CLR2 B6:NONE B7:ALL"},
    {"KEY/SCALE",MELLO_UI_KEY_SCALE,"B5:RANDOM"},
    {"SLOTS",MELLO_UI_SLOTS,"B5:RANDOM"},
    {"DOT",MELLO_UI_DOT,"B1:RND B2:RST B5:RANDOM"},
    {"TURING",MELLO_UI_TURING,"B0:ON B1:RND B5:RANDOM"},
    {"SUB",MELLO_UI_SUB,"B0:ON B4:STEP B5:RANDOM"},
#if MELLO_FEATURE_CHORD_MACHINE
    {"CHORD",MELLO_UI_CHORD,"B0:ON B5:RANDOM"},
#endif
    {"GLOBAL",MELLO_UI_GLOBAL,"B1:TAP B3:RST2 B5:RANDOM"},
    {"SOUND",MELLO_UI_SOUND_RESERVED,"B5:RANDOM"},
    {"AUDIO FX",MELLO_UI_AUDIO_FX,"B5:RANDOM"},
    {"SAMPLE",MELLO_UI_SAMPLE_RESERVED,"B0:ON B5:RANDOM"},
};

static uint32_t bit_u32(unsigned n){ return 1u<<n; }
static void die(const char *where,const char *what){ fprintf(stderr,"FAIL: %s: %s\n",where,what); exit(1); }
static void fill(void *ctx,int16_t x,int16_t y,int16_t w,int16_t h,uint16_t c){(void)ctx;(void)x;(void)y;(void)w;(void)h;(void)c;}
static void text(void *ctx,int16_t x,int16_t y,int16_t w,uint8_t large,const char *s,uint16_t c,uint8_t align){
    Capture *cap=(Capture*)ctx; (void)x;(void)w;(void)large;(void)c;(void)align;
    if(y==172) snprintf(cap->hint,sizeof cap->hint,"%s",s?s:"");
}
static void led_key(void *ctx,uint8_t i,bool on){(void)ctx;(void)i;(void)on;}
static void led_btn(void *ctx,uint8_t i,bool on){(void)ctx;(void)i;(void)on;}
static void midi(void *ctx,uint8_t a,uint8_t b,uint8_t c){(void)ctx;(void)a;(void)b;(void)c;}
static void setup(MelloProject *p,MelloTransport *t,MelloStorage *st,MelloUi *u,MelloUiPage page){
    mello_project_defaults(p); mello_transport_init(t,p,123u,midi,0); mello_storage_init(st,0,0); mello_ui_init(u,t,st); mello_ui_set_page(u,page);
}
static uint8_t black_note(uint8_t pos){
    uint8_t i,n=0u; for(i=0u;i<27u;++i){ if(mello_ui_white_key_position(i)>=0)continue; if(n==pos)return i; ++n; } return 0xFFu;
}
static void black(MelloUi *u,uint32_t ms,uint8_t pos){
    int16_t enc[MELLO_UI_ENCODER_COUNT]={0}; uint8_t note=black_note(pos); if(note==0xFFu)die("BLACK","missing key");
    mello_ui_input(u,ms*1000u,ms,0u,0u,0u,bit_u32(note),enc);
}
static void render(MelloUi *u,Capture *cap){
    MelloUiDrawOps ops={fill,text,led_key,led_btn,cap}; memset(cap,0,sizeof *cap); u->dirty=MELLO_UI_DIRTY_ALL; mello_ui_render(u,&ops);
}
static int has_slot(const char *hint,uint8_t slot){ char k[4]={'B',(char)('0'+slot),':',0}; return strstr(hint,k)!=0; }

static void verify_every_page_and_inactive_slot(void){
    unsigned i; unsigned active=0u, inactive=0u;
    for(i=0u;i<sizeof(cases)/sizeof(cases[0]);++i){
        MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; Capture cap; uint8_t slot;
        setup(&p,&t,&st,&u,cases[i].page); render(&u,&cap);
        if(strcmp(cap.hint,cases[i].hint)) die(cases[i].name,"rendered descriptor hint drift");
        for(slot=0u;slot<8u;++slot){
            if(has_slot(cases[i].hint,slot)){ ++active; continue; }
            MelloEngine before_engine; MelloUi before_ui;
            setup(&p,&t,&st,&u,cases[i].page); before_engine=t.engine; before_ui=u;
            black(&u,(uint32_t)(100u+slot),slot);
            if(memcmp(&before_engine,&t.engine,sizeof before_engine)||memcmp(&before_ui,&u,sizeof before_ui))
                die(cases[i].name,"inactive contextual slot changed state");
            ++inactive;
        }
    }
#if MELLO_FEATURE_CHORD_MACHINE
    if(active!=31u) die("MAP","active slot count != 31 with Chord");
#else
    if(active!=29u) die("MAP","active slot count != 29 without Chord");
#endif
    printf("C5 MAP COVERAGE: pages=%u active=%u inactive=%u\n",(unsigned)(sizeof(cases)/sizeof(cases[0])),active,inactive);
}

static void verify_b5_from_every_advertised_page(void){
    unsigned i, count=0u;
    for(i=0u;i<sizeof(cases)/sizeof(cases[0]);++i){
        MelloProject p; MelloTransport t; MelloStorage st; MelloUi u;
        if(!has_slot(cases[i].hint,5u)) continue;
        setup(&p,&t,&st,&u,cases[i].page); black(&u,100u,5u);
        if(u.page!=MELLO_UI_RANDOM) die(cases[i].name,"advertised B5 did not open RANDOM");
        ++count;
    }
#if MELLO_FEATURE_CHORD_MACHINE
    if(count!=15u) die("B5","advertised page count != 15 with Chord");
#else
    if(count!=14u) die("B5","advertised page count != 14 without Chord");
#endif
}

static void verify_manual_note_priority(void){
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; int16_t enc[MELLO_UI_ENCODER_COUNT]={0}; uint8_t note=black_note(4u); int8_t before;
    setup(&p,&t,&st,&u,MELLO_UI_SEQ_STEP); u.selected_step=0u; before=t.engine.project.steps[0].pitch;
    mello_ui_input(&u,100000u,100u,0u,0u,bit_u32(MELLO_UI_BTN_SEL),bit_u32(note),enc);
    if(t.engine.project.steps[0].pitch==before) die("STEP SEL+black","manual note priority lost");
    if(u.page!=MELLO_UI_SEQ_STEP) die("STEP SEL+black","context navigation stole musical key");
}

int main(void){
    verify_every_page_and_inactive_slot(); verify_b5_from_every_advertised_page(); verify_manual_note_priority();
    puts("PASS: WP-C5 all page/slot guidance is covered; inactive slots inert and B5/manual-note contracts hold");
    return 0;
}
