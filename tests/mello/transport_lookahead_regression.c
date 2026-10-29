/* Phase 2 regression (review 2026-10, timing): the main loop saw a pulse only after the audio block holding
 * it had been rendered, so every step started 0-5.8 ms late, at the next block. With
 * mello_transport_poll_ahead every note reaches the audio queue before its block is rendered. The loop
 * below runs the main loop every 2 ms and the audio callback every 256 frames, as on the FM-1. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mello_transport.h"

#define BLOCK_US 5805u /* 256 frames at 44.1 kHz */

static uint32_t rendered_until; /* the audio has rendered everything before this time */
static unsigned notes, late;
static uint32_t worst_late_us;

static void sink(void *ctx, const MelloEvent *e, uint32_t due_us)
{
    (void)ctx;
    if (e->type != MELLO_EVENT_NOTE_ON && e->type != MELLO_EVENT_SUB_NOTE_ON) return;
    notes++;
    if ((int32_t)(due_us - rendered_until) < 0) { /* its block is already out: it plays late */
        late++;
        if (rendered_until - due_us > worst_late_us) worst_late_us = rendered_until - due_us;
    }
}

static void run(uint32_t ahead_us)
{
    static MelloTransport t;
    MelloProject p;
    uint32_t now, next_block = 0u;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 0x51A7u, 0, 0);
    mello_transport_set_event_sink(&t, sink, NULL);
    notes = late = 0u;
    worst_late_us = 0u;
    rendered_until = 0u;
    mello_transport_start(&t, 1000u + ahead_us);       /* as mello_ui does on PLAY */
    for (now = 1000u; now < 20000000u; now += 2000u) { /* 20 s */
        while ((int32_t)(now - next_block) >= 0) {     /* the audio ISR renders [next_block, +BLOCK_US) */
            next_block += BLOCK_US;
            rendered_until = next_block;
        }
        mello_transport_poll_ahead(&t, now, ahead_us);
    }
}

int main(void)
{
    run(0u);
    printf("no lookahead:      %u notes, %u late (worst %u us)\n", notes, late, (unsigned)worst_late_us);
    if (late == 0u) {
        printf("FAIL: the harness does not reproduce the late notes\n");
        return 1;
    }
    run(MELLO_TRANSPORT_LOOKAHEAD_US);
    printf("15 ms lookahead:   %u notes, %u late (worst %u us)\n", notes, late, (unsigned)worst_late_us);
    if (late || notes < 100u) {
        printf("FAIL: notes still reach the audio after their block\n");
        return 1;
    }
    printf("PASS: every note reaches the audio before its block is rendered\n");
    return 0;
}
