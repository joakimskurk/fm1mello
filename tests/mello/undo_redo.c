/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* UNDO / REDO (feedback 2026-10-30): the key toggles, says which it will do, and an edit after an UNDO leaves nothing
 * to redo. */
#include <stdio.h>
#include <string.h>

#include "mello_engine.h"

static MelloEngine e;

int main(void)
{
    MelloProject p, rolled;
    int fails = 0;
    mello_project_defaults(&p);
    mello_engine_init(&e, &p, 3u);
#define CHECK(c, m)                                                                                          \
    do {                                                                                                     \
        if (!(c)) fprintf(stderr, "FAIL: %s\n", m), fails++;                                                 \
    } while (0)
    CHECK(mello_engine_undo_state(&e) == MELLO_UNDO_NONE, "a new project has something to undo");
    mello_engine_randomize_steps_intensity(&e, MELLO_RANDOMIZE_HIGH);
    rolled = e.project;
    CHECK(mello_engine_undo_state(&e) == MELLO_UNDO_UNDO, "after a roll: not UNDO");
    CHECK(mello_engine_undo_swap(&e) && !memcmp(&e.project, &p, sizeof p),
          "UNDO did not bring the pattern back");
    CHECK(mello_engine_undo_state(&e) == MELLO_UNDO_REDO, "after UNDO: not REDO");
    CHECK(mello_engine_undo_swap(&e) && !memcmp(&e.project, &rolled, sizeof rolled),
          "REDO did not bring the roll");
    CHECK(mello_engine_undo_state(&e) == MELLO_UNDO_UNDO, "after REDO: not UNDO");
    (void)mello_engine_undo_swap(&e); /* UNDO, then an edit */
    e.project.steps[3].pitch = (int8_t)(e.project.steps[3].pitch + 1);
    CHECK(mello_engine_undo_state(&e) == MELLO_UNDO_NONE, "an edit after UNDO still offers REDO");
    CHECK(!mello_engine_undo_swap(&e), "the key still swapped after an edit");
    mello_engine_randomize_steps_intensity(&e, MELLO_RANDOMIZE_LOW);
    e.project.steps[5].pitch =
        (int8_t)(e.project.steps[5].pitch + 1); /* an edit after a roll: UNDO still undoes */
    CHECK(mello_engine_undo_state(&e) == MELLO_UNDO_UNDO, "an edit after a roll took UNDO away");
    if (fails) return 1;
    puts("PASS: UNDO, REDO, UNDO; an edit after UNDO ends it; an edit after a roll keeps UNDO");
    return 0;
}
