#include "mello_ui.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SCREEN_W 240
#define SCREEN_H 240
#define MAX_TEXT_RECTS 512

/* Pinned Dinghy / Felucca metrics at d44df7d: FONT_S=8x16 advance/cell,
 * FONT_L=16x32 advance/cell. The target draw_text_box clears the entire box
 * before blitting, so overlapping text boxes and later graphics are destructive. */
enum { FONT_S_ADV = 8, FONT_S_H = 16, FONT_L_ADV = 16, FONT_L_H = 32 };

typedef struct {
    int16_t x, y, w, h;
    char text[64];
} TextRect;

typedef struct {
    TextRect text[MAX_TEXT_RECTS];
    unsigned text_count;
    unsigned errors;
    unsigned clips;
    unsigned overlaps;
    unsigned fixtures;
    unsigned saw_home_single;
    const char *fixture;
} Audit;

static int rect_overlap(int16_t ax, int16_t ay, int16_t aw, int16_t ah,
                        int16_t bx, int16_t by, int16_t bw, int16_t bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static void audit_fail(Audit *a, const char *kind, const char *detail)
{
    ++a->errors;
    fprintf(stderr, "FAIL %-10s %-18s %s\n", kind, a->fixture ? a->fixture : "?", detail);
}

static void audit_fill(void *ctx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour)
{
    Audit *a = (Audit *)ctx;
    unsigned i;
    char detail[180];
    (void)colour;
    if (w <= 0 || h <= 0) return;
    if (x < 0 || y < 0 || x + w > SCREEN_W || y + h > SCREEN_H) {
        snprintf(detail, sizeof(detail), "fill outside screen: %d,%d %dx%d", x, y, w, h);
        audit_fail(a, "BOUNDS", detail);
    }
    for (i = 0u; i < a->text_count; ++i) {
        const TextRect *t = &a->text[i];
        if (t->text[0] && rect_overlap(t->x,t->y,t->w,t->h,x,y,w,h)) {
            snprintf(detail, sizeof(detail), "later fill %d,%d %dx%d overwrites text '%s' at %d,%d %dx%d",
                     x,y,w,h,t->text,t->x,t->y,t->w,t->h);
            ++a->overlaps;
            audit_fail(a, "OVERDRAW", detail);
            return;
        }
    }
}

static void audit_text(void *ctx, int16_t x, int16_t y, int16_t w, uint8_t large,
                       const char *text, uint16_t colour, uint8_t align)
{
    Audit *a = (Audit *)ctx;
    const char *s = text ? text : "";
    size_t n = strlen(s);
    int16_t h = large ? FONT_L_H : FONT_S_H;
    int32_t text_w = (int32_t)n * (large ? FONT_L_ADV : FONT_S_ADV);
    unsigned i;
    char detail[180];
    (void)colour; (void)align;

    if (text_w > w) {
        snprintf(detail, sizeof(detail), "text '%s' width %ld exceeds box %d at %d,%d",
                 s, (long)text_w, w, x, y);
        ++a->clips;
        audit_fail(a, "CLIP", detail);
    }
    if (x < 0 || y < 0 || w <= 0 || x + w > SCREEN_W || y + h > SCREEN_H) {
        snprintf(detail, sizeof(detail), "text '%s' box outside screen: %d,%d %dx%d", s,x,y,w,h);
        audit_fail(a, "BOUNDS", detail);
    }
    for (i = 0u; i < a->text_count; ++i) {
        const TextRect *t = &a->text[i];
        if (s[0] && t->text[0] && rect_overlap(t->x,t->y,t->w,t->h,x,y,w,h)) {
            snprintf(detail, sizeof(detail), "text '%s' at %d,%d %dx%d overlaps prior '%s' at %d,%d %dx%d",
                     s,x,y,w,h,t->text,t->x,t->y,t->w,t->h);
            ++a->overlaps;
            audit_fail(a, "TEXT-OVER", detail);
        }
    }
    if (a->text_count < MAX_TEXT_RECTS) {
        TextRect *t = &a->text[a->text_count++];
        t->x=x; t->y=y; t->w=w; t->h=h;
        snprintf(t->text,sizeof(t->text),"%s",s);
    } else {
        audit_fail(a, "INTERNAL", "text rectangle capacity exhausted");
    }
    if (a->fixture && !strncmp(a->fixture,"HOME/",5) && y < 172 && strstr(s,"SGL"))
        a->saw_home_single = 1u;
}

static int subcount(MelloUiPage p)
{
    switch (p) {
    case MELLO_UI_SEQ_STEP: return 4;
    case MELLO_UI_SEQ_SEGMENT: return 2;
    case MELLO_UI_SEQ_LOOP: return 3;
    case MELLO_UI_SLOTS: return 2;
    case MELLO_UI_TURING: return 3;
    case MELLO_UI_SUB: return 5;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: return 5;
#endif
    case MELLO_UI_GLOBAL: return 2;
    case MELLO_UI_SOUND_RESERVED: return 5;
    case MELLO_UI_AUDIO_FX: return 5;
    case MELLO_UI_SAMPLE_RESERVED: return 4;
    default: return 1;
    }
}

static void set_fixture(MelloUi *ui, MelloUiPage page, int sub)
{
    ui->step_subpage=0u; ui->segment_subpage=0u; ui->loop_subpage=0u;
    ui->slots_subpage=0u; ui->tm_subpage=0u; ui->sub_subpage=0u;
#if MELLO_FEATURE_CHORD_MACHINE
    ui->chord_subpage=0u;
#endif
    ui->global_subpage=0u; ui->sound_subpage=0u; ui->fx_subpage=0u; ui->sample_subpage=0u;
    mello_ui_set_page(ui,page);
    switch (page) {
    case MELLO_UI_SEQ_STEP: ui->step_subpage=(uint8_t)sub; break;
    case MELLO_UI_SEQ_SEGMENT: ui->segment_subpage=(uint8_t)sub; break;
    case MELLO_UI_SEQ_LOOP: ui->loop_subpage=(uint8_t)sub; break;
    case MELLO_UI_SLOTS: ui->slots_subpage=(uint8_t)sub; break;
    case MELLO_UI_TURING: ui->tm_subpage=(uint8_t)sub; break;
    case MELLO_UI_SUB: ui->sub_subpage=(uint8_t)sub; break;
#if MELLO_FEATURE_CHORD_MACHINE
    case MELLO_UI_CHORD: ui->chord_subpage=(uint8_t)sub; break;
#endif
    case MELLO_UI_GLOBAL: ui->global_subpage=(uint8_t)sub; break;
    case MELLO_UI_SOUND_RESERVED: ui->sound_subpage=(uint8_t)sub; break;
    case MELLO_UI_AUDIO_FX: ui->fx_subpage=(uint8_t)sub; break;
    case MELLO_UI_SAMPLE_RESERVED: ui->sample_subpage=(uint8_t)sub; break;
    default: break;
    }
}

static void run_fixture(Audit *audit, MelloUiPage page, int sub, int direction, int variant)
{
    MelloProject p;
    MelloTransport tr;
    MelloStorage st;
    MelloUi ui;
    MelloUiDrawOps draw = {audit_fill,audit_text,0,0,audit};
    int16_t d[MELLO_UI_ENCODER_COUNT] = {0};
    char name[64];
    int k;

    mello_project_defaults(&p);
    mello_transport_init(&tr,&p,123u,0,0);
    mello_storage_init(&st,0,0);
    mello_ui_init(&ui,&tr,&st);
    set_fixture(&ui,page,sub);

    if (page == MELLO_UI_DOT) ui.selected_dot_target = (uint8_t)variant;
    if (page == MELLO_UI_RANDOM) ui.random_intensity = (MelloRandomizeIntensity)variant;

    if (direction != 0) {
        for (k=0;k<4;++k) {
            memset(d,0,sizeof(d));
            d[MELLO_UI_ENC_KNOB1+k]=(int16_t)(direction > 0 ? 30000 : -30000);
            mello_ui_input(&ui,1000u,1000u,0u,0u,0u,0u,d);
        }
    }

    snprintf(name,sizeof(name),"%s/%d/%c/v%d",mello_ui_page_name(page),sub,
             direction>0?'+':direction<0?'-':'0',variant);
    audit->fixture=name;
    audit->text_count=0u;
    ++audit->fixtures;
    ui.dirty |= MELLO_UI_DIRTY_ALL;
    mello_ui_render(&ui,&draw);
}

typedef struct { uint16_t px[SCREEN_H][SCREEN_W]; } Framebuffer;

static void fb_fill(void *ctx,int16_t x,int16_t y,int16_t w,int16_t h,uint16_t colour)
{
    Framebuffer *fb=(Framebuffer *)ctx;
    int yy,xx;
    if(x<0){w=(int16_t)(w+x);x=0;} if(y<0){h=(int16_t)(h+y);y=0;}
    if(x+w>SCREEN_W) w=(int16_t)(SCREEN_W-x);
    if(y+h>SCREEN_H) h=(int16_t)(SCREEN_H-y);
    if(w<=0||h<=0)return;
    for(yy=y;yy<y+h;++yy)for(xx=x;xx<x+w;++xx)fb->px[yy][xx]=colour;
}

static uint16_t text_mark(const char *s)
{
    uint32_t h=2166136261u;
    while(s&&*s){h^=(uint8_t)*s++;h*=16777619u;}
    return (uint16_t)((h&0x7fffu)|0x8000u);
}

static void fb_text(void *ctx,int16_t x,int16_t y,int16_t w,uint8_t large,const char *text,uint16_t colour,uint8_t align)
{
    Framebuffer *fb=(Framebuffer *)ctx;
    int16_t h=(int16_t)(large?FONT_L_H:FONT_S_H), adv=(int16_t)(large?FONT_L_ADV:FONT_S_ADV);
    int16_t tw=(int16_t)(strlen(text?text:"")*(size_t)adv), tx=x;
    int i;
    (void)colour;
    fb_fill(ctx,x,y,w,h,0u);
    if(align==1)tx=(int16_t)(x+(w-tw)/2); else if(align==2)tx=(int16_t)(x+w-tw);
    for(i=0;text&&text[i];++i){int16_t gx=(int16_t)(tx+i*adv); if(gx>=x&&gx<x+w)fb_fill(fb,gx,(int16_t)(y+3),adv>2?2:1,h>6?3:1,text_mark(text));}
}

static int home_incremental_equivalence(void)
{
    MelloProject p;
    MelloTransport tr;
    MelloStorage st;
    MelloUi ui;
    Framebuffer inc,full;
    MelloUiDrawOps inc_ops={fb_fill,fb_text,0,0,&inc};
    MelloUiDrawOps full_ops={fb_fill,fb_text,0,0,&full};

    memset(&inc,0,sizeof(inc)); memset(&full,0,sizeof(full));
    mello_project_defaults(&p);
    mello_transport_init(&tr,&p,123u,0,0);
    mello_storage_init(&st,0,0);
    mello_ui_init(&ui,&tr,&st);
    mello_ui_set_page(&ui,MELLO_UI_HOME);
    tr.display_step=0u;
    ui.dirty=MELLO_UI_DIRTY_ALL;
    mello_ui_render(&ui,&inc_ops);

    tr.display_step=1u;
    ui.home_old_step=0u;
    ui.home_new_step=1u;
    ui.dirty=(uint16_t)(MELLO_UI_DIRTY_HOME_PLAYHEAD|MELLO_UI_DIRTY_STATUS);
    mello_ui_render(&ui,&inc_ops);

    ui.dirty=MELLO_UI_DIRTY_ALL;
    mello_ui_render(&ui,&full_ops);
    return memcmp(&inc,&full,sizeof(inc))==0;
}

int main(void)
{
    Audit audit;
    int page,sub,dir,variant,variants;
    memset(&audit,0,sizeof(audit));

    for(page=MELLO_UI_HOME;page<=MELLO_UI_SAMPLE_RESERVED;++page){
        for(sub=0;sub<subcount((MelloUiPage)page);++sub){
            variants = page==MELLO_UI_DOT ? 4 : page==MELLO_UI_RANDOM ? 3 : 1;
            for(variant=0;variant<variants;++variant)
                for(dir=-1;dir<=1;++dir)
                    run_fixture(&audit,(MelloUiPage)page,sub,dir,variant);
        }
    }

    if(!audit.saw_home_single){
        audit.fixture="HOME/default";
        audit_fail(&audit,"COVERAGE","HOME default SINGLE/SGL summary was not rendered");
    }
    if(!home_incremental_equivalence()){
        audit.fixture="HOME/incremental";
        audit_fail(&audit,"DIRTY","incremental HOME playhead+status redraw differs from clean redraw");
    }

    if(audit.errors){
        fprintf(stderr,"FAIL: display integrity fixtures=%u clips=%u overlaps=%u errors=%u\n",
                audit.fixtures,audit.clips,audit.overlaps,audit.errors);
        return 1;
    }
    printf("PASS: display integrity fixtures=%u clips=0 overlaps=0; HOME SINGLE and incremental redraw clean\n",audit.fixtures);
    return 0;
}
