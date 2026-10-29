#include "mello_ui.h"

#include <stdio.h>

static void turn(MelloUi *ui, uint8_t encoder, int16_t delta)
{
    int16_t enc[MELLO_UI_ENCODER_COUNT] = {0};
    enc[encoder] = delta;
    mello_ui_input(ui, 0u, 0u, 0u, 0u, 0u, 0u, enc);
}

int main(void)
{
    MelloProject project;
    MelloTransport transport;
    MelloStorage storage;
    MelloUi ui;

    mello_project_defaults(&project);
    mello_transport_init(&transport, &project, 123u, 0, 0);
    mello_storage_init(&storage, 0, 0);
    mello_ui_init(&ui, &transport, &storage);

    mello_ui_set_page(&ui, MELLO_UI_SUB);
    ui.sub_subpage = 3u;
    transport.engine.sub.wave = MELLO_WAVE_SINE;
    turn(&ui, MELLO_UI_ENC_KNOB2, 1);
    if (transport.engine.sub.wave != MELLO_WAVE_TRIANGLE) return 1;
    turn(&ui, MELLO_UI_ENC_KNOB2, 1);
    if (transport.engine.sub.wave != MELLO_WAVE_SQUARE) return 2;
    turn(&ui, MELLO_UI_ENC_KNOB2, 1);
    if (transport.engine.sub.wave != MELLO_WAVE_SAW) return 3;
    turn(&ui, MELLO_UI_ENC_KNOB2, 1);
    if (transport.engine.sub.wave != MELLO_WAVE_SINE) return 4;
    turn(&ui, MELLO_UI_ENC_KNOB2, -1);
    if (transport.engine.sub.wave != MELLO_WAVE_SAW) return 5;
    transport.engine.sub.wave = MELLO_WAVE_PULSE;
    turn(&ui, MELLO_UI_ENC_KNOB2, 1);
    if (transport.engine.sub.wave != MELLO_WAVE_SINE) return 6;

    mello_ui_set_page(&ui, MELLO_UI_SOUND_RESERVED);
    ui.sound_subpage = 3u;
    transport.engine.sound.follower_sub_wave = MELLO_WAVE_SINE;
    turn(&ui, MELLO_UI_ENC_KNOB4, 1);
    if (transport.engine.sound.follower_sub_wave != MELLO_WAVE_TRIANGLE) return 7;
    turn(&ui, MELLO_UI_ENC_KNOB4, 1);
    if (transport.engine.sound.follower_sub_wave != MELLO_WAVE_SQUARE) return 8;
    turn(&ui, MELLO_UI_ENC_KNOB4, 1);
    if (transport.engine.sound.follower_sub_wave != MELLO_WAVE_SAW) return 9;
    turn(&ui, MELLO_UI_ENC_KNOB4, 1);
    if (transport.engine.sound.follower_sub_wave != MELLO_WAVE_SINE) return 10;
    turn(&ui, MELLO_UI_ENC_KNOB4, -1);
    if (transport.engine.sound.follower_sub_wave != MELLO_WAVE_SAW) return 11;

    ui.sound_subpage = 0u;
    transport.engine.sound.osc_wave = MELLO_WAVE_SINE;
    turn(&ui, MELLO_UI_ENC_KNOB1, 1);
    if (transport.engine.sound.osc_wave != MELLO_WAVE_PULSE) {
        fprintf(stderr, "main oscillator lost PULSE while restricting sub oscillators\n");
        return 12;
    }

    puts("PASS: SUB BASS and follower SUB OSC use SINE/TRI/SQUARE/SAW only; main oscillator retains PULSE");
    return 0;
}
