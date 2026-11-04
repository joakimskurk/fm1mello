/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* The overview (DECISIONS "An overview"): ARP held a moment shows its pages; white key 2 goes to DRUMS; letting go
 * stays there and the overview is gone. A tap of ARP is as before. */
#include <stdio.h>
#include <string.h>

#include "mello_ui.h"

static int fails;
#define CHECK(c, ...)                                                                                        \
    do {                                                                                                     \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "FAIL: " __VA_ARGS__);                                                           \
            fprintf(stderr, "\n");                                                                           \
            fails++;                                                                                         \
        }                                                                                                    \
    } while (0)

int main(void)
{
    static MelloProject p;
    static MelloTransport t;
    static MelloStorage st;
    static MelloUi u;
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    uint32_t arp = 1u << MELLO_UI_BTN_ARP, w2 = 0u;
    uint8_t i;
    for (i = 0u; i < 27u; ++i)
        if (mello_ui_white_key_position(i) == 1) w2 = 1u << i;
    mello_project_defaults(&p);
    mello_transport_init(&t, &p, 5u, 0, 0);
    mello_storage_init(&st, 0, 0);
    mello_ui_init(&u, &t, &st);
    mello_ui_set_page(&u, MELLO_UI_HOME);

    mello_ui_input(&u, 1000000u, 1000u, arp, 0u, arp, 0u, enc);
    mello_ui_input(&u, 1500000u, 1500u, 0u, 0u, arp, 0u, enc);
    CHECK(u.sect_menu, "ARP held did not show its pages");
    mello_ui_set_held_notes(&u, w2);
    mello_ui_input(&u, 1600000u, 1600u, 0u, 0u, arp, w2, enc);
    mello_ui_set_held_notes(&u, 0u);
    mello_ui_input(&u, 1700000u, 1700u, 0u, arp, 0u, 0u, enc);
    CHECK(u.page == MELLO_UI_DRUMS && !u.sect_menu && u.sect_btn == 0xFFu,
          "ARP + white key 2 is not DRUMS, or the overview stayed (page %d)", (int)u.page);

    mello_ui_input(&u, 2000000u, 2000u, arp, 0u, arp, 0u, enc); /* a tap: the next page, no overview */
    mello_ui_input(&u, 2100000u, 2100u, 0u, arp, 0u, 0u, enc);
    CHECK(u.page != MELLO_UI_DRUMS && !u.sect_menu, "a tap of ARP did not go on, or showed the overview");

    if (fails) return 1;
    puts("PASS: section_overview");
    return 0;
}
