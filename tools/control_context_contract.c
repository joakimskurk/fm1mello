#include "mello_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char hint[64];
    unsigned buttons[14];
} Capture;

typedef struct {
    const char *name;
    MelloUiPage page;
    const char *hint;
} HintCase;

static const HintCase hints[] = {
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
static void led_btn(void *ctx,uint8_t i,bool on){ Capture *cap=(Capture*)ctx; if(i<14u)cap->buttons[i]=on?1u:0u; }
static void midi(void *ctx,uint8_t a,uint8_t b,uint8_t c){(void)ctx;(void)a;(void)b;(void)c;}

static void setup(MelloProject *p,MelloTransport *t,MelloStorage *st,MelloUi *u,MelloUiPage page){
    mello_project_defaults(p); mello_transport_init(t,p,123u,midi,0); mello_storage_init(st,0,0); mello_ui_init(u,t,st); mello_ui_set_page(u,page);
}
static void press(MelloUi *u,uint32_t ms,uint32_t pressed,uint32_t held){
    int16_t enc[MELLO_UI_ENCODER_COUNT]={0}; mello_ui_input(u,ms*1000u,ms,pressed,0u,held,0u,enc);
}
static uint8_t black_note(uint8_t pos){
    uint8_t i,n=0u; for(i=0u;i<27u;++i){ if(mello_ui_white_key_position(i)>=0)continue; if(n==pos)return i; ++n; } return 0xFFu;
}
static void black(MelloUi *u,uint32_t ms,uint8_t pos){
    int16_t enc[MELLO_UI_ENCODER_COUNT]={0}; uint8_t note=black_note(pos); if(note==0xFFu)die("BLACK","missing key");
    mello_ui_input(u,ms*1000u,ms,0u,0u,0u,bit_u32(note),enc);
}
static void turn_k4(MelloUi *u){ int16_t enc[MELLO_UI_ENCODER_COUNT]={0}; enc[MELLO_UI_ENC_KNOB4]=1; mello_ui_input(u,100000u,100u,0,0,0,0,enc); }
static void render(MelloUi *u,Capture *cap){
    MelloUiDrawOps ops={fill,text,led_key,led_btn,cap};
    memset(cap,0,sizeof *cap);
    u->last_key_led_mask=0u;
    u->last_button_led_mask=0u;
    u->dirty=MELLO_UI_DIRTY_ALL;
    mello_ui_render(u,&ops);
    mello_ui_sync_leds(u,&ops);
}

static void verify_hints(void){
    unsigned i;
    for(i=0u;i<sizeof(hints)/sizeof(hints[0]);++i){
        MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; Capture cap;
        setup(&p,&t,&st,&u,hints[i].page); render(&u,&cap);
        if(strcmp(cap.hint,hints[i].hint)){ fprintf(stderr,"FAIL: %s hint '%s' != '%s'\n",hints[i].name,cap.hint,hints[i].hint); exit(1); }
        if(strlen(cap.hint)>29u) die(hints[i].name,"hint exceeds 232 px small-font row");
    }
}

static void verify_random_mask_and_oct(void){
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; uint32_t mask=MELLO_RAND_PITCH|MELLO_RAND_GATE;
    setup(&p,&t,&st,&u,MELLO_UI_RANDOM); mello_engine_set_randomize_mask(&t.engine,mask); u.dirty=0u;
    press(&u,100u,bit_u32(MELLO_UI_BTN_OCT_DOWN),0u);
    if(t.engine.randomize.mask!=mask||u.dirty) die("RANDOM OCT-","still changes RANDOM state");
    press(&u,200u,bit_u32(MELLO_UI_BTN_OCT_UP),0u);
    if(t.engine.randomize.mask!=mask||u.dirty) die("RANDOM OCT+","still changes RANDOM state");
    black(&u,300u,6u); if(t.engine.randomize.mask!=0u) die("RANDOM B6","mask NONE missing");
    black(&u,400u,7u); if(t.engine.randomize.mask!=MELLO_RAND_ALL) die("RANDOM B7","mask ALL missing");
}

static void verify_global_reset(void){
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; uint32_t both=bit_u32(MELLO_UI_BTN_OCT_DOWN)|bit_u32(MELLO_UI_BTN_OCT_UP);
    setup(&p,&t,&st,&u,MELLO_UI_GLOBAL); t.engine.project.bpm=199u; t.engine.project.seq_len=24u; u.dirty=0u;
    press(&u,100u,both,both);
    if(u.reset_armed||t.engine.project.bpm!=199u||t.engine.project.seq_len!=24u) die("GLOBAL OCT chord","hidden RESET ALL survived");
    black(&u,200u,3u);
    if(!u.reset_armed||t.engine.project.bpm!=199u) die("GLOBAL B3","first press did not arm only");
    black(&u,500u,3u);
    if(u.reset_armed||u.page!=MELLO_UI_HOME||t.engine.project.bpm!=80u||t.engine.project.seq_len!=16u) die("GLOBAL B3","confirmed factory reset failed");
}

static void verify_loop_reset(void){
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u;
    setup(&p,&t,&st,&u,MELLO_UI_SEQ_LOOP); u.loop_subpage=2u; t.engine.loop_cycle_pos=5u; t.engine.loop_cycle_tick=9u; u.dirty=0u;
    turn_k4(&u); if(t.engine.loop_cycle_pos!=5u||t.engine.loop_cycle_tick!=9u||u.dirty) die("LOOP K4","still acts as RESET");
    black(&u,200u,2u); if(t.engine.loop_cycle_pos||t.engine.loop_cycle_tick) die("LOOP B2","context reset missing");
}

static void verify_mod_navigation(void){
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; bool on;
    setup(&p,&t,&st,&u,MELLO_UI_HOME); on=t.engine.tm.on;
    press(&u,100u,bit_u32(MELLO_UI_BTN_LFO),0u);
    if(u.page!=MELLO_UI_SOUND_RESERVED||u.sound_subpage!=2u||t.engine.tm.on!=on) die("MOD 1","did not open Audio LFO cleanly");
    press(&u,200u,bit_u32(MELLO_UI_BTN_LFO),0u); if(u.page!=MELLO_UI_DOT||t.engine.tm.on!=on) die("MOD 2","did not advance to DOT");
    press(&u,300u,bit_u32(MELLO_UI_BTN_LFO),0u); if(u.page!=MELLO_UI_TURING||t.engine.tm.on!=on) die("MOD 3","did not advance to EVOLVE");
    black(&u,350u,0u); if(t.engine.tm.on==on) die("TURING B0","state toggle missing"); on=t.engine.tm.on;
    press(&u,400u,bit_u32(MELLO_UI_BTN_LFO),0u);
    if(u.page!=MELLO_UI_SOUND_RESERVED||u.sound_subpage!=2u||t.engine.tm.on!=on) die("MOD wrap","physical MOD toggled Turing or failed wrap");

    mello_ui_set_page(&u,MELLO_UI_HOME); press(&u,500u,bit_u32(MELLO_UI_BTN_FX),0u);
    if(u.page!=MELLO_UI_AUDIO_FX) die("FX","did not open Audio FX");
    press(&u,600u,bit_u32(MELLO_UI_BTN_FX),0u); if(u.page!=MELLO_UI_AUDIO_FX) die("FX","still routes to DOT");
}

static void verify_led_family(void){
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; Capture cap;
    setup(&p,&t,&st,&u,MELLO_UI_DOT); render(&u,&cap);
    if(!cap.buttons[MELLO_UI_BTN_LFO]||cap.buttons[MELLO_UI_BTN_FX]) die("DOT LED","not in MOD family");
    mello_ui_set_page(&u,MELLO_UI_TURING); render(&u,&cap);
    if(!cap.buttons[MELLO_UI_BTN_LFO]) die("TURING LED","MOD family not lit");
    mello_ui_set_page(&u,MELLO_UI_SOUND_RESERVED); u.sound_subpage=2u; render(&u,&cap);
    if(!cap.buttons[MELLO_UI_BTN_LFO]||cap.buttons[MELLO_UI_BTN_ENV]) die("LFO LED","Audio LFO not owned by MOD");
    u.sound_subpage=0u; render(&u,&cap);
    if(!cap.buttons[MELLO_UI_BTN_ENV]||cap.buttons[MELLO_UI_BTN_LFO]) die("SOUND LED","non-LFO SOUND lost ENV ownership");
    mello_ui_set_page(&u,MELLO_UI_RANDOM); render(&u,&cap);
    if(cap.buttons[MELLO_UI_BTN_EDIT]) die("RANDOM LED","still falsely owned by EDIT");
}

static void verify_inactive_and_chord(void){
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; MelloEngine before; bool on;
    setup(&p,&t,&st,&u,MELLO_UI_HOME); before=t.engine; u.dirty=0u; black(&u,100u,0u);
    if(u.page!=MELLO_UI_HOME||memcmp(&before,&t.engine,sizeof before)||u.dirty) die("HOME B0","inactive rail slot is not inert");
    black(&u,200u,5u); if(u.page!=MELLO_UI_RANDOM) die("HOME B5","RANDOM access missing");
#if MELLO_FEATURE_CHORD_MACHINE
    setup(&p,&t,&st,&u,MELLO_UI_CHORD); on=t.engine.chord.on; black(&u,100u,0u); if(t.engine.chord.on==on) die("CHORD B0","toggle missing");
#else
    (void)on;
#endif
}

int main(void){
    verify_hints(); verify_random_mask_and_oct(); verify_global_reset(); verify_loop_reset();
    verify_mod_navigation(); verify_led_family(); verify_inactive_and_chord();
    printf("C4 CONTRACT: hints=%u MOD=LFO>DOT>EVOLVE OCT=nav-only LOOP-K4=inert\n",(unsigned)(sizeof(hints)/sizeof(hints[0])));
    puts("PASS: WP-C4 contextual rail, visible guidance and MOD-family navigation stay aligned");
    return 0;
}
