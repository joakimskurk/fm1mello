#ifndef MELLO_TRANSPORT_H
#define MELLO_TRANSPORT_H

#include "mello_engine.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MELLO_MIDI_SCHEDULE_CAP 96u
#define MELLO_MIDI_CLOCKS_PER_QUARTER 24u

typedef enum {
    MELLO_SYNC_OFF = 0,
    MELLO_SYNC_SLAVE = 1,
    MELLO_SYNC_MASTER = 2
} MelloSyncMode;

typedef void (*MelloMidiSendFn)(void *ctx, uint8_t status, uint8_t data1, uint8_t data2);
typedef void (*MelloEngineEventSinkFn)(void *ctx, const MelloEvent *event, uint32_t due_us);

typedef struct {
    uint32_t due_us;
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    MelloVoice voice;
} MelloScheduledMidi;

typedef struct {
    MelloEngine engine;
    MelloSyncMode sync_mode;
    MelloMidiSendFn midi_send;
    void *midi_ctx;
    MelloEngineEventSinkFn event_sink;
    void *event_sink_ctx;

    bool running;
    bool midi_note_out_enabled;
    bool midi_fixed_length;
    uint16_t midi_fixed_len_ms;
    uint8_t midi_channel;     /* lead, 0..15 */
    uint8_t midi_sub_channel; /* sub, 0..15 */

    uint32_t next_pulse_us;
    uint32_t pulse_base_us;
    uint32_t pulse_rem;
    uint32_t pulse_den;
    uint32_t pulse_error;

    uint32_t next_clock_us;
    uint32_t clock_base_us;
    uint32_t clock_rem;
    uint32_t clock_den;
    uint32_t clock_error;

    uint8_t slave_clock_phase;
    uint32_t slave_last_clock_us;
    uint32_t slave_interval_sum;
    uint8_t slave_interval_count;
    uint16_t measured_bpm;

    MelloScheduledMidi scheduled[MELLO_MIDI_SCHEDULE_CAP];
    uint8_t scheduled_count;
    bool schedule_overflow;
    uint8_t active_note_count[2][128];

    uint8_t display_step;
    uint8_t display_pulse;
    uint8_t display_segment;
    uint32_t pattern_wraps;
} MelloTransport;

void mello_transport_init(MelloTransport *t, const MelloProject *project, uint32_t seed,
                          MelloMidiSendFn midi_send, void *midi_ctx);
void mello_transport_factory_reset(MelloTransport *t, uint32_t seed);
void mello_transport_set_sync(MelloTransport *t, MelloSyncMode mode, uint32_t now_us);
void mello_transport_set_event_sink(MelloTransport *t, MelloEngineEventSinkFn sink, void *ctx);
void mello_transport_set_midi_channel(MelloTransport *t, uint8_t channel_1_to_16);
void mello_transport_set_midi_sub_channel(MelloTransport *t, uint8_t channel_1_to_16);
void mello_transport_set_midi_note_out(MelloTransport *t, bool enabled);
void mello_transport_set_midi_fixed_length(MelloTransport *t, bool fixed, uint16_t milliseconds);
void mello_transport_set_bpm(MelloTransport *t, uint16_t bpm);
void mello_transport_start(MelloTransport *t, uint32_t now_us);
void mello_transport_stop(MelloTransport *t);
void mello_transport_poll(MelloTransport *t, uint32_t now_us);
void mello_transport_midi_in(MelloTransport *t, uint8_t status, uint8_t d1, uint8_t d2, uint32_t now_us);

uint16_t mello_transport_effective_bpm(const MelloTransport *t);
bool mello_transport_schedule_overflowed(const MelloTransport *t);

#ifdef __cplusplus
}
#endif
#endif
