#include "mello_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    MelloUiPage page;
    uint8_t layers;
} LayerCase;

#define L(n,p,c) {n,p,c}
static const LayerCase cases[] = {
    L("HOME",MELLO_UI_HOME,1u),
    L("STEP",MELLO_UI_SEQ_STEP,4u),
    L("SEGMENT",MELLO_UI_SEQ_SEGMENT,2u),
    L("LOOP",MELLO_UI_SEQ_LOOP,3u),
    L("SLOTSEQ",MELLO_UI_SEQ_SLOTSEQ,1u),
    L("RANDOM",MELLO_UI_RANDOM,1u),
    L("KEY/SCALE",MELLO_UI_KEY_SCALE,1u),
    L("SLOTS",MELLO_UI_SLOTS,2u),
    L("DOT",MELLO_UI_DOT,4u),
    L("TURING",MELLO_UI_TURING,3u),
    L("SUB",MELLO_UI_SUB,5u),
#if MELLO_FEATURE_CHORD_MACHINE
    L("CHORD",MELLO_UI_CHORD,5u),
#endif
    L("GLOBAL",MELLO_UI_GLOBAL,2u),
    L("SOUND",MELLO_UI_SOUND_RESERVED,5u),
    L("AUDIO FX",MELLO_UI_AUDIO_FX,5u),
    L("SAMPLE",MELLO_UI_SAMPLE_RESERVED,4u),
};

static void fail(const char *where, const char *what)
{
    fprintf(stderr, "FAIL: %s: %s\n", where, what);
    exit(1);
}

static void setup(MelloProject *p, MelloTransport *t, MelloStorage *st, MelloUi *u, MelloUiPage page)
{
    mello_project_defaults(p);
    mello_transport_init(t,p,123u,0,0);
    mello_storage_init(st,0,0);
    mello_ui_init(u,t,st);
    mello_ui_set_page(u,page);
}

static void turn(MelloUi *u, uint8_t enc_id, int16_t delta)
{
    int16_t e[MELLO_UI_ENCODER_COUNT]={0};
    e[enc_id]=delta;
    mello_ui_input(u,100000u,100u,0u,0u,0u,0u,e);
}

static uint8_t layer(const MelloUi *u, MelloUiPage p)
{
    switch(p){
    case MELLO_UI_SEQ_STEP:return u->step_subpage;
    case MELLO_UI_SEQ_SEGMENT:return u->segment_subpage;
    case MELLO_UI_SEQ_LOOP:return u->loop_subpage;
    case MELLO_UI_SLOTS:return u->slots_subpage;
    case MELLO_UI_DOT:return u->selected_dot_target;
    case MELLO_UI_TURING:return u->tm_subpage;
    case MELLO_UI_SUB:return u->sub_subpage;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD:return u->chord_subpage;
#endif
    case MELLO_UI_GLOBAL:return u->global_subpage;
    case MELLO_UI_SOUND_RESERVED:return u->sound_subpage;
    case MELLO_UI_AUDIO_FX:return u->fx_subpage;
    case MELLO_UI_SAMPLE_RESERVED:return u->sample_subpage;
    default:return 0u;
    }
}

static void verify_algorithm_duplicates_select(void)
{
    unsigned i;
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
        MelloProject pa,pb; MelloTransport ta,tb; MelloStorage sa,sb; MelloUi a,b;
        setup(&pa,&ta,&sa,&a,cases[i].page);
        setup(&pb,&tb,&sb,&b,cases[i].page);
        turn(&a,MELLO_UI_ENC_SELECT,+1);
        turn(&b,MELLO_UI_ENC_ALGORITHM,+1);
        if(layer(&a,cases[i].page)!=layer(&b,cases[i].page)) fail(cases[i].name,"ALGORITHM no longer duplicates SELECT layer movement");
        if(cases[i].layers>1u && layer(&a,cases[i].page)!=1u) fail(cases[i].name,"layer encoder baseline mismatch");
        if(cases[i].layers==1u && layer(&a,cases[i].page)!=0u) fail(cases[i].name,"single-layer page unexpectedly moved");
    }
}

static void verify_presets_global_slot_cursor(void)
{
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u; MelloEngine before;
    setup(&p,&t,&st,&u,MELLO_UI_HOME);
    u.selected_slot=0u; u.slot_bank=0u; before=t.engine;
    turn(&u,MELLO_UI_ENC_PRESETS,+17);
    if(u.page!=MELLO_UI_HOME || u.selected_slot!=17u || u.slot_bank!=1u) fail("PRESETS","global slot cursor baseline mismatch");
    if(memcmp(&before,&t.engine,sizeof before)) fail("PRESETS","global slot cursor unexpectedly changed engine state");
}

static void verify_loop_k4_reset_action(void)
{
    MelloProject p; MelloTransport t; MelloStorage st; MelloUi u;
    setup(&p,&t,&st,&u,MELLO_UI_SEQ_LOOP);
    u.loop_subpage=2u;
    t.engine.loop_cycle_pos=5u;
    t.engine.loop_cycle_tick=9u;
    turn(&u,MELLO_UI_ENC_KNOB4,+1);
    if(t.engine.loop_cycle_pos!=0u || t.engine.loop_cycle_tick!=0u) fail("LOOP K4","RESET page-action baseline mismatch");
}

int main(void)
{
    verify_algorithm_duplicates_select();
    verify_presets_global_slot_cursor();
    verify_loop_k4_reset_action();
    printf("C1 ENCODER MAP: page/layers=%u ALGORITHM=SELECT-duplicate PRESETS=global-slot-cursor LOOP-K4=RESET-action\n",(unsigned)(sizeof(cases)/sizeof(cases[0])));
    puts("PASS: deterministic WP-C C1 encoder/action debt baseline matches reconstructed pre-remap behavior");
    return 0;
}
