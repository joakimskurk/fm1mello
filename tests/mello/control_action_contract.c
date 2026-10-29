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
static void white_with_held(MelloUi *u,uint32_t ms,uint8_t pos,uint32_t held)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT]={0}; uint8_t note=mello_ui_white_key_note_index(pos);
    if(note==0xFFu) fail("WHITE","position missing");
    mello_ui_input(u,ms*1000u,ms,0u,0u,held,bit_u32(note),enc);
}

static void verify_rec_reserved(void)
{
    unsigned i;
    for(i=0u;i<sizeof(pages)/sizeof(pages[0]);++i){
        MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; MelloEngine before; MelloUiPage page;
        setup(&p,&t,&st,&u,pages[i].page); before=t.engine; page=u.page; st.dirty=false;
        buttons(&u,100u,bit_u32(MELLO_UI_BTN_REC),bit_u32(MELLO_UI_BTN_EDIT)|bit_u32(MELLO_UI_BTN_REC));
        if(u.page!=page || memcmp(&before,&t.engine,sizeof before) || st.dirty || u.tap_count) fail(pages[i].name,"REC is not reserved/inert");
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

static void verify_edit_semantics(void)
{
    unsigned i;
    for(i=0u;i<sizeof(pages)/sizeof(pages[0]);++i){
        MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; MelloEngine before; MelloUiPage page; bool b;
        setup(&p,&t,&st,&u,pages[i].page); page=u.page;
        if(page==MELLO_UI_TURING){
            b=t.engine.tm.locked; buttons(&u,100u,bit_u32(MELLO_UI_BTN_EDIT),0u);
            if(u.page!=page||t.engine.tm.locked==b) fail(pages[i].name,"EDIT did not toggle LOCK");
        } else if(page==MELLO_UI_SAMPLE_RESERVED){
            b=t.engine.sample.per_step; buttons(&u,100u,bit_u32(MELLO_UI_BTN_EDIT),0u);
            if(u.page!=page||t.engine.sample.per_step==b) fail(pages[i].name,"EDIT did not toggle per-step edit mode");
        } else {
            before=t.engine; buttons(&u,100u,bit_u32(MELLO_UI_BTN_EDIT),0u);
            if(u.page!=page||memcmp(&before,&t.engine,sizeof before)) fail(pages[i].name,"EDIT has unrelated page/action semantics");
        }
    }
}

static void verify_selected_position_actions(void)
{
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; bool b;

    setup(&p,&t,&st,&u,MELLO_UI_SEQ_STEP); b=t.engine.project.steps[3].skip;
    white_with_held(&u,100u,3u,bit_u32(MELLO_UI_BTN_EDIT));
    if(u.selected_step!=3u||t.engine.project.steps[3].skip!=b) fail("STEP","EDIT+white still performs hidden SKIP");
    black(&u,200u,4u); if(t.engine.project.steps[3].skip==b) fail("STEP B4","selected-step SKIP toggle missing");

    setup(&p,&t,&st,&u,MELLO_UI_SUB); b=t.engine.sub.x_steps[2].on;
    white_with_held(&u,100u,2u,bit_u32(MELLO_UI_BTN_EDIT));
    if(u.selected_sub_x_step!=2u||t.engine.sub.x_steps[2].on!=b) fail("SUB","EDIT+white still toggles selected SUB step");
    black(&u,200u,4u); if(t.engine.sub.x_steps[2].on==b) fail("SUB B4","selected SUB-step toggle missing");
}

static void verify_dot_reset_migration(void)
{
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; bool b;
    setup(&p,&t,&st,&u,MELLO_UI_DOT); b=t.engine.dot[u.selected_dot_target].reset;
    buttons(&u,100u,bit_u32(MELLO_UI_BTN_EDIT),0u);
    if(t.engine.dot[u.selected_dot_target].reset!=b) fail("DOT EDIT","reset toggle remained on EDIT");
    black(&u,200u,2u);
    if(t.engine.dot[u.selected_dot_target].reset==b) fail("DOT B2","reset toggle was not migrated");
}

static void verify_random_navigation(void)
{
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u;
    setup(&p,&t,&st,&u,MELLO_UI_HOME); black(&u,100u,5u);
    if(u.page!=MELLO_UI_RANDOM) fail("B5 RANDOM","RANDOM became inaccessible after EDIT cleanup");
    buttons(&u,200u,bit_u32(MELLO_UI_BTN_EDIT),0u);
    if(u.page!=MELLO_UI_RANDOM) fail("RANDOM EDIT","EDIT navigated away from RANDOM");
}

static void verify_manual_black_note_exception(void)
{
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; int16_t enc[MELLO_UI_ENCODER_COUNT]={0}; uint8_t note=black_note(5u);
    setup(&p,&t,&st,&u,MELLO_UI_SEQ_STEP); u.selected_step=0u; t.engine.project.steps[0].pitch=9;
    if(note==0xFFu) fail("STEP manual","B5 black key missing");
    mello_ui_input(&u,100000u,100u,0u,0u,bit_u32(MELLO_UI_BTN_SEL),bit_u32(note),enc);
    if(u.page!=MELLO_UI_SEQ_STEP) fail("STEP manual","context navigation beat SEL+black manual note entry");
    if(t.engine.project.steps[0].pitch==9) fail("STEP manual","black key stopped being musical under SEL");
}

int main(void)
{
    verify_rec_reserved(); verify_sample_navigation(); verify_context_actions(); verify_edit_semantics();
    verify_selected_position_actions(); verify_dot_reset_migration(); verify_random_navigation(); verify_manual_black_note_exception();
    printf("C3 CONTRACT: pages=%u REC=reserved EDIT=lock/edit-only B4=position-toggle B5=RANDOM\n",(unsigned)(sizeof(pages)/sizeof(pages[0])));
    puts("PASS: WP-C3 removes unrelated EDIT semantics while preserving C2 actions and manual-note priority");
    return 0;
}
