#include "mello_ui.h"
#include <stdint.h>
#include <stdio.h>

static unsigned fills, texts, step_cells, segment_cells;
static int bounds_error;

static void count_fill(void *ctx,int16_t x,int16_t y,int16_t w,int16_t h,uint16_t colour)
{
    (void)ctx;(void)colour;
    if(x<0||y<0||w<0||h<0||x+w>240||y+h>240)bounds_error=1;
    ++fills;
    if(y==36&&h==10&&x>=8&&x<232)++step_cells;
    if(y==44&&h==14&&x>=8&&x<232)++segment_cells;
}
static void count_text(void *ctx,int16_t x,int16_t y,int16_t w,uint8_t large,const char *text,uint16_t colour,uint8_t align)
{
    (void)ctx;(void)large;(void)text;(void)colour;(void)align;
    if(x<0||y<0||w<0||x+w>240||y>239)bounds_error=1;
    ++texts;
}
static void reset_counts(void){fills=texts=step_cells=segment_cells=0u;bounds_error=0;}
static void full_render(MelloUi *ui,const MelloUiDrawOps *ops,MelloUiPage page)
{
    mello_ui_set_page(ui,page);
    ui->dirty|=MELLO_UI_DIRTY_ALL;
    reset_counts();
    mello_ui_render(ui,ops);
}
static int delta_render(MelloUi *ui,const MelloUiDrawOps *ops,uint8_t step,uint8_t pulse)
{
    ui->transport->display_step=step;
    ui->transport->display_pulse=pulse;
    mello_ui_tick(ui);
    reset_counts();
    mello_ui_render(ui,ops);
    return bounds_error?0:1;
}

int main(void)
{
    MelloProject p;
    MelloTransport t;
    MelloStorage st;
    MelloUi ui;
    MelloUiDrawOps ops={count_fill,count_text,0,0,0};

    mello_project_defaults(&p);
    p.seq_len=32u;
    mello_transport_init(&t,&p,0x55495033u,0,0);
    mello_storage_init(&st,0,0);
    mello_ui_init(&ui,&t,&st);
    t.engine.project.seq_len=32u;
    t.display_step=0u;t.display_pulse=0u;t.display_segment=0u;
    mello_ui_tick(&ui); /* synchronize seen transport state */

    full_render(&ui,&ops,MELLO_UI_SEQ_STEP);
    if(bounds_error||step_cells<32u){
        fprintf(stderr,"STEP 32 overview failed: cells=%u bounds=%d\n",step_cells,bounds_error);
        return 1;
    }
    if(!delta_render(&ui,&ops,1u,1u))return 2;
    if(fills>=20u||texts>=4u){
        fprintf(stderr,"STEP playhead redraw too broad: fills=%u texts=%u\n",fills,texts);
        return 3;
    }
    printf("STEP delta: fills=%u texts=%u\n",fills,texts);

    /* RANDOM shares the universal 4..32 step strip and should only redraw the
       old/new playhead cells while a pattern is running. */
    full_render(&ui,&ops,MELLO_UI_RANDOM);
    if(!delta_render(&ui,&ops,2u,2u))return 4;
    if(fills>=20u||texts>=4u){
        fprintf(stderr,"RANDOM playhead redraw too broad: fills=%u texts=%u\n",fills,texts);
        return 5;
    }
    printf("RANDOM delta: fills=%u texts=%u\n",fills,texts);

    /* Segment view also uses the same 32-step geometry. It remains a dynamic
       body for now because segment/repeat state changes during playback. */
    full_render(&ui,&ops,MELLO_UI_SEQ_SEGMENT);
    if(bounds_error||segment_cells<32u){
        fprintf(stderr,"SEGMENT 32 overview failed: cells=%u bounds=%d\n",segment_cells,bounds_error);
        return 6;
    }

    /* FX is transport-independent. Advancing playback must update status only,
       never repaint the full 142 px body. */
    full_render(&ui,&ops,MELLO_UI_AUDIO_FX);
    if(!delta_render(&ui,&ops,3u,3u))return 7;
    if(fills>2u||texts>2u){
        fprintf(stderr,"static FX page redrew body on transport tick: fills=%u texts=%u\n",fills,texts);
        return 8;
    }
    printf("FX transport delta: fills=%u texts=%u\n",fills,texts);

    puts("PASS: universal 32-step overview + page-aware incremental playback redraws");
    return 0;
}
