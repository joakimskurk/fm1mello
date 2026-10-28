#include "mello_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const char *name; MelloUiPage page; } PageCase;
static const PageCase pages[] = {
    {"HOME",MELLO_UI_HOME},{"STEP",MELLO_UI_SEQ_STEP},{"SEGMENT",MELLO_UI_SEQ_SEGMENT},
    {"LOOP",MELLO_UI_SEQ_LOOP},{"SLOTSEQ",MELLO_UI_SEQ_SLOTSEQ},{"RANDOM",MELLO_UI_RANDOM},
    {"KEY/SCALE",MELLO_UI_KEY_SCALE},{"SLOTS",MELLO_UI_SLOTS},{"DOT",MELLO_UI_DOT},
    {"TURING",MELLO_UI_TURING},{"SUB",MELLO_UI_SUB},
#if MELLO_FEATURE_CHORD_MACHINE
    {"CHORD",MELLO_UI_CHORD},
#endif
    {"GLOBAL",MELLO_UI_GLOBAL},{"SOUND",MELLO_UI_SOUND_RESERVED},
    {"AUDIO FX",MELLO_UI_AUDIO_FX},{"SAMPLE",MELLO_UI_SAMPLE_RESERVED},
};

static uint32_t bit_u32(unsigned n){ return 1u<<n; }
static void fail(const char *where,const char *what){ fprintf(stderr,"FAIL: %s: %s\n",where,what); exit(1); }
static void setup(MelloProject *p,MelloTransport *t,MelloStorage *st,MelloUi *u,MelloUiPage page)
{ mello_project_defaults(p); mello_transport_init(t,p,123u,0,0); mello_storage_init(st,0,0); mello_ui_init(u,t,st); mello_ui_set_page(u,page); }
static void buttons(MelloUi *u,uint32_t ms,uint32_t pressed,uint32_t held)
{ int16_t enc[MELLO_UI_ENCODER_COUNT]={0}; mello_ui_input(u,ms*1000u,ms,pressed,0u,held,0u,enc); }
static uint8_t black_note(uint8_t pos)
{
    uint8_t i,n=0u;
    for(i=0u;i<27u;++i){ if(mello_ui_white_key_position(i)>=0) continue; if(n==pos) return i; ++n; }
    return 0xFFu;
}
static void black(MelloUi *u,uint32_t ms,uint8_t pos)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT]={0}; uint8_t note=black_note(pos);
    if(note==0xFFu) fail("BLACK","position missing");
    mello_ui_input(u,ms*1000u,ms,0u,0u,0u,bit_u32(note),enc);
}

static void verify_rec_reserved(void)
{
    unsigned i;
    for(i=0u;i<sizeof(pages)/sizeof(pages[0]);++i){
        MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; MelloEngine before; MelloUiPage page;
        setup(&p,&t,&st,&u,pages[i].page); before=t.engine; page=u.page; st.dirty=false;
        buttons(&u,100u,bit_u32(MELLO_UI_BTN_REC),bit_u32(MELLO_UI_BTN_EDIT)|bit_u32(MELLO_UI_BTN_REC));
        if(u.page!=page || memcmp(&before,&t.engine,sizeof before) || st.dirty || u.tap_count) fail(pages[i].name,"REC is not reserved/inert in C2");
    }
}

static void verify_sample_navigation(void)
{
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u;
    setup(&p,&t,&st,&u,MELLO_UI_HOME);
    buttons(&u,100u,bit_u32(MELLO_UI_BTN_ENV),0u); if(u.page!=MELLO_UI_SOUND_RESERVED) fail("ENV","HOME did not open SOUND");
    buttons(&u,200u,bit_u32(MELLO_UI_BTN_ENV),0u); if(u.page!=MELLO_UI_SAMPLE_RESERVED) fail("ENV","SOUND did not cycle to SAMPLE");
    buttons(&u,300u,bit_u32(MELLO_UI_BTN_ENV),0u); if(u.page!=MELLO_UI_SOUND_RESERVED) fail("ENV","SAMPLE did not cycle to SOUND");
}

static void verify_context_actions(void)
{
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; MelloEngine before; bool b;

    setup(&p,&t,&st,&u,MELLO_UI_RANDOM); before=t.engine; st.dirty=false;
    black(&u,100u,1u); if(!memcmp(&before,&t.engine,sizeof before)||!st.dirty) fail("RANDOM B1","reroll not migrated");
    t.engine.project.steps[0].pitch=9; st.dirty=false;
    black(&u,200u,3u); if(!u.reset_armed||t.engine.project.steps[0].pitch!=9) fail("RANDOM B3","CLEAR did not arm safely");
    black(&u,500u,3u); if(u.reset_armed||t.engine.project.steps[0].pitch==9||!st.dirty) fail("RANDOM B3","confirmed CLEAR failed");

    setup(&p,&t,&st,&u,MELLO_UI_GLOBAL); black(&u,1000u,1u); black(&u,1500u,1u);
    if(t.engine.project.bpm!=120u||u.tap_count!=2u) fail("GLOBAL B1","TAP not migrated");

    setup(&p,&t,&st,&u,MELLO_UI_SEQ_LOOP); t.engine.loop_cycle_pos=5u; t.engine.loop_cycle_tick=9u; black(&u,100u,2u);
    if(t.engine.loop_cycle_pos||t.engine.loop_cycle_tick) fail("LOOP B2","reset not migrated");

    setup(&p,&t,&st,&u,MELLO_UI_DOT); before=t.engine; black(&u,100u,1u);
    if(!memcmp(&before,&t.engine,sizeof before)) fail("DOT B1","reroll not migrated");
    setup(&p,&t,&st,&u,MELLO_UI_TURING); before=t.engine; black(&u,100u,1u);
    if(!memcmp(&before,&t.engine,sizeof before)) fail("TURING B1","reroll not migrated");

    setup(&p,&t,&st,&u,MELLO_UI_SUB); b=t.engine.sub.on; black(&u,100u,0u); if(t.engine.sub.on==b) fail("SUB B0","toggle not migrated");
#if MELLO_FEATURE_CHORD_MACHINE
    setup(&p,&t,&st,&u,MELLO_UI_CHORD); b=t.engine.chord.on; black(&u,100u,0u); if(t.engine.chord.on==b) fail("CHORD B0","toggle not migrated");
#endif
    setup(&p,&t,&st,&u,MELLO_UI_SAMPLE_RESERVED); b=t.engine.sample.on; black(&u,100u,0u); if(t.engine.sample.on==b) fail("SAMPLE B0","toggle not migrated");
}

static void verify_c3_debt_still_explicit(void)
{
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; uint8_t note; bool b; int16_t enc[MELLO_UI_ENCODER_COUNT]={0};
    setup(&p,&t,&st,&u,MELLO_UI_HOME); buttons(&u,100u,bit_u32(MELLO_UI_BTN_EDIT),0u);
    if(u.page!=MELLO_UI_RANDOM) fail("C3 debt","EDIT navigation disappeared before C3");
    setup(&p,&t,&st,&u,MELLO_UI_SEQ_STEP); note=mello_ui_white_key_note_index(3u); b=t.engine.project.steps[3].skip;
    mello_ui_input(&u,100000u,100u,0u,0u,bit_u32(MELLO_UI_BTN_EDIT),bit_u32(note),enc);
    if(t.engine.project.steps[3].skip==b) fail("C3 debt","EDIT+STEP SKIP debt missing");
}

static void verify_manual_black_note_exception(void)
{
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; int16_t enc[MELLO_UI_ENCODER_COUNT]={0};
    setup(&p,&t,&st,&u,MELLO_UI_SEQ_STEP); u.selected_step=0u; t.engine.project.steps[0].pitch=9;
    mello_ui_input(&u,100000u,100u,0u,0u,bit_u32(MELLO_UI_BTN_SEL),bit_u32(10u),enc);
    if(t.engine.project.steps[0].pitch==9) fail("STEP manual","A# black key stopped being musical under SEL");
}

int main(void)
{
    verify_rec_reserved(); verify_sample_navigation(); verify_context_actions(); verify_c3_debt_still_explicit(); verify_manual_black_note_exception();
    printf("C2 CONTRACT: pages=%u REC=reserved B0=state B1=trigger B2=reset B3=destructive-confirm ENV=SOUND/SAMPLE\n",(unsigned)(sizeof(pages)/sizeof(pages[0])));
    puts("PASS: WP-C2 removes non-record REC semantics while preserving migrated actions and manual-note exception");
    return 0;
}
