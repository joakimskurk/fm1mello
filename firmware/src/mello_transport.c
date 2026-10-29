#include "mello_transport.h"

#define US_PER_MINUTE                  60000000u
#define US_PER_SIXTEENTH_AT_BPM1       15000000u
#define MELLO_MAX_CATCHUP_PULSES       8u
#define MELLO_MAX_MASTER_CLOCK_CATCHUP 24u

static bool time_due(uint32_t now, uint32_t due)
{
    return (int32_t)(now - due) >= 0;
}

static void send_now(MelloTransport *t, uint8_t st, uint8_t d1, uint8_t d2)
{
    if (t && t->midi_send) t->midi_send(t->midi_ctx, st, d1, d2);
}

static void recalc_timing(MelloTransport *t)
{
    uint32_t bpm, div, num;
    if (!t) return;
    bpm = t->engine.project.bpm;
    if (bpm < 20u) bpm = 20u;
    if (bpm > 300u) bpm = 300u;
    div = t->engine.project.clock_div;
    if (!(div == 1u || div == 2u || div == 4u)) div = 1u;

    num = US_PER_SIXTEENTH_AT_BPM1 * div; /* max 60,000,000: safe uint32 */
    t->pulse_den = bpm;
    t->pulse_base_us = num / bpm;
    t->pulse_rem = num % bpm;
    t->pulse_error = 0u;

    t->clock_den = bpm * MELLO_MIDI_CLOCKS_PER_QUARTER; /* <= 7200 */
    t->clock_base_us = US_PER_MINUTE / t->clock_den;
    t->clock_rem = US_PER_MINUTE % t->clock_den;
    t->clock_error = 0u;
}

static uint32_t advance_fractional_deadline(uint32_t due, uint32_t base, uint32_t rem, uint32_t den,
                                            uint32_t *error)
{
    uint32_t extra = 0u;
    if (den && rem) {
        uint32_t e = *error + rem;
        if (e >= den) {
            e -= den;
            extra = 1u;
        }
        *error = e;
    }
    return due + base + extra;
}

/* Convert a small engine-tick offset to microseconds without 64-bit arithmetic.
 * Scheduled offsets are bounded by current SkurkyMello gate clamps (<= 4 s). */
static uint32_t ticks_to_us_small(uint32_t ticks, uint16_t bpm)
{
    uint32_t q, r, quarter_us, base, rem;
    if (bpm < 20u) bpm = 20u;
    if (bpm > 300u) bpm = 300u;
    quarter_us = US_PER_MINUTE / bpm;
    q = ticks / MELLO_PPQN;
    r = ticks % MELLO_PPQN;
    base = quarter_us / MELLO_PPQN;
    rem = quarter_us % MELLO_PPQN;
    return q * quarter_us + r * base + (r * rem) / MELLO_PPQN;
}

static void remove_scheduled(MelloTransport *t, uint8_t idx)
{
    uint8_t i;
    if (!t || idx >= t->scheduled_count) return;
    for (i = idx; i + 1u < t->scheduled_count; ++i) t->scheduled[i] = t->scheduled[i + 1u];
    t->scheduled_count--;
}

static void schedule_midi(MelloTransport *t, uint32_t due_us, uint8_t status, uint8_t d1, uint8_t d2,
                          MelloVoice voice)
{
    uint8_t i;
    if (!t) return;
    if (t->scheduled_count >= MELLO_MIDI_SCHEDULE_CAP) {
        t->schedule_overflow = true;
        return;
    }
    i = t->scheduled_count++;
    t->scheduled[i].due_us = due_us;
    t->scheduled[i].status = status;
    t->scheduled[i].data1 = d1;
    t->scheduled[i].data2 = d2;
    t->scheduled[i].voice = voice;
}

static void dispatch_scheduled(MelloTransport *t, uint32_t now_us)
{
    uint8_t i = 0u;
    if (!t) return;
    while (i < t->scheduled_count) {
        MelloScheduledMidi ev = t->scheduled[i];
        if (!time_due(now_us, ev.due_us)) {
            i++;
            continue;
        }
        send_now(t, ev.status, ev.data1, ev.data2);
        if ((ev.status & 0xF0u) == 0x90u && ev.data2) {
            uint8_t *cnt = &t->active_note_count[ev.voice == MELLO_VOICE_SUB ? 1u : 0u][ev.data1 & 0x7Fu];
            if (*cnt != 255u) (*cnt)++;
        } else if ((ev.status & 0xF0u) == 0x80u || ((ev.status & 0xF0u) == 0x90u && !ev.data2)) {
            uint8_t *cnt = &t->active_note_count[ev.voice == MELLO_VOICE_SUB ? 1u : 0u][ev.data1 & 0x7Fu];
            if (*cnt) (*cnt)--;
        }
        remove_scheduled(t, i);
    }
}

static void all_notes_off(MelloTransport *t)
{
    uint16_t n;
    uint8_t lead_status, sub_status;
    if (!t) return;
    lead_status = (uint8_t)(0x80u | (t->midi_channel & 0x0Fu));
    sub_status = (uint8_t)(0x80u | (t->midi_sub_channel & 0x0Fu));
    for (n = 0u; n < 128u; ++n) {
        if (t->active_note_count[0][n]) send_now(t, lead_status, (uint8_t)n, 0u);
        if (t->active_note_count[1][n]) send_now(t, sub_status, (uint8_t)n, 0u);
        t->active_note_count[0][n] = 0u;
        t->active_note_count[1][n] = 0u;
    }
    t->scheduled_count = 0u;
}

static void consume_engine_events(MelloTransport *t, uint32_t pulse_tick, uint32_t pulse_due_us)
{
    MelloEvent ev;
    if (!t) return;
    while (mello_event_pop(&t->engine, &ev)) {
        if (ev.type == MELLO_EVENT_PULSE) {
            t->display_step = ev.step_index;
            t->display_pulse = ev.pulse_index;
            t->display_segment = ev.segment_index;
            if (t->event_sink) t->event_sink(t->event_sink_ctx, &ev, pulse_due_us);
        } else if (ev.type == MELLO_EVENT_PATTERN_WRAP) {
            t->pattern_wraps++;
        } else if (ev.type == MELLO_EVENT_SAMPLE_NOTE_ON || ev.type == MELLO_EVENT_SAMPLE_NOTE_OFF) {
            uint32_t delta_ticks = ev.tick - pulse_tick;
            uint32_t due = pulse_due_us + ticks_to_us_small(delta_ticks, t->engine.project.bpm);
            if (t->event_sink) t->event_sink(t->event_sink_ctx, &ev, due);
        } else if (ev.type == MELLO_EVENT_NOTE_ON || ev.type == MELLO_EVENT_NOTE_OFF ||
                   ev.type == MELLO_EVENT_SUB_NOTE_ON || ev.type == MELLO_EVENT_SUB_NOTE_OFF
#if MELLO_FEATURE_CHORD_MACHINE
                   || ev.type == MELLO_EVENT_CHORD_NOTE_ON || ev.type == MELLO_EVENT_CHORD_NOTE_OFF
#endif
        ) {
            uint32_t delta_ticks = ev.tick - pulse_tick;
            uint32_t due = pulse_due_us + ticks_to_us_small(delta_ticks, t->engine.project.bpm);
            bool is_sub = ev.type == MELLO_EVENT_SUB_NOTE_ON || ev.type == MELLO_EVENT_SUB_NOTE_OFF;
            bool is_on = ev.type == MELLO_EVENT_NOTE_ON || ev.type == MELLO_EVENT_SUB_NOTE_ON
#if MELLO_FEATURE_CHORD_MACHINE
                         || ev.type == MELLO_EVENT_CHORD_NOTE_ON
#endif
                ;
            uint8_t ch = is_sub ? t->midi_sub_channel : t->midi_channel;
            uint8_t status_on = (uint8_t)(0x90u | (ch & 0x0Fu));
            uint8_t status_off = (uint8_t)(0x80u | (ch & 0x0Fu));
            MelloVoice voice = is_sub ? MELLO_VOICE_SUB : MELLO_VOICE_LEAD;
#if MELLO_FEATURE_CHORD_MACHINE
            if (ev.type == MELLO_EVENT_CHORD_NOTE_ON || ev.type == MELLO_EVENT_CHORD_NOTE_OFF)
                voice = MELLO_VOICE_CHORD;
#endif
            /* Phase 9 audio sink always receives the source event, regardless of MIDI mode. */
            if (t->event_sink) t->event_sink(t->event_sink_ctx, &ev, due);
            /* Browser parity: sequencer note output is emitted only in MIDI MASTER and only
             * when NOTE OUT is enabled. SLAVE uses clock for transport but is not MIDI thru. */
            if (!t->midi_note_out_enabled || t->sync_mode != MELLO_SYNC_MASTER) continue;
            if (is_on) {
                schedule_midi(t, due, status_on, ev.note, ev.velocity, voice);
                if (t->midi_fixed_length)
                    schedule_midi(t, due + (uint32_t)t->midi_fixed_len_ms * 1000u, status_off, ev.note, 0u,
                                  voice);
            } else if (!t->midi_fixed_length) {
                schedule_midi(t, due, status_off, ev.note, 0u, voice);
            }
        }
    }
}

static void process_one_pulse(MelloTransport *t, uint32_t due_us)
{
    uint32_t pulse_tick;
    if (!t || !t->running) return;
    pulse_tick = t->engine.now_tick;
    if (!mello_engine_advance_pulse(&t->engine)) return;
    consume_engine_events(t, pulse_tick, due_us);
}

static void start_engine(MelloTransport *t, uint32_t now_us, bool from_external)
{
    if (!t) return;
    all_notes_off(t);
    mello_engine_start(&t->engine);
    t->running = true;
    recalc_timing(t);
    t->next_pulse_us = now_us;
    t->next_clock_us = now_us;
    t->slave_clock_phase = 0u;
    t->pattern_wraps = 0u;
    if (!from_external && t->sync_mode == MELLO_SYNC_MASTER) send_now(t, 0xFAu, 0u, 0u);
}

void mello_transport_init(MelloTransport *t, const MelloProject *project, uint32_t seed,
                          MelloMidiSendFn midi_send, void *midi_ctx)
{
    if (!t) return;
    memset(t, 0, sizeof(*t));
    mello_engine_init(&t->engine, project, seed);
    t->sync_mode = MELLO_SYNC_OFF;
    t->midi_send = midi_send;
    t->midi_ctx = midi_ctx;
    t->midi_channel = 0u;
    t->midi_sub_channel = 1u;
    t->midi_note_out_enabled = false;
    t->midi_fixed_length = false;
    t->midi_fixed_len_ms = 180u;
    t->display_step = 255u;
    t->display_segment = 255u;
    recalc_timing(t);
}

void mello_transport_factory_reset(MelloTransport *t, uint32_t seed)
{
    MelloMidiSendFn midi_send;
    void *midi_ctx;
    MelloEngineEventSinkFn event_sink;
    void *event_sink_ctx;
    if (!t) return;
    midi_send = t->midi_send;
    midi_ctx = t->midi_ctx;
    event_sink = t->event_sink;
    event_sink_ctx = t->event_sink_ctx;
    mello_transport_stop(t);
    mello_transport_init(t, 0, seed, midi_send, midi_ctx);
    mello_transport_set_event_sink(t, event_sink, event_sink_ctx);
}

void mello_transport_set_event_sink(MelloTransport *t, MelloEngineEventSinkFn sink, void *ctx)
{
    if (!t) return;
    t->event_sink = sink;
    t->event_sink_ctx = ctx;
}

void mello_transport_set_sync(MelloTransport *t, MelloSyncMode mode, uint32_t now_us)
{
    if (!t) return;
    if (mode > MELLO_SYNC_MASTER) mode = MELLO_SYNC_OFF;
    if (t->running) mello_transport_stop(t);
    t->sync_mode = mode;
    t->slave_clock_phase = 0u;
    t->slave_last_clock_us = 0u;
    t->slave_interval_sum = 0u;
    t->slave_interval_count = 0u;
    t->measured_bpm = 0u;
    recalc_timing(t);
    t->next_pulse_us = now_us;
    t->next_clock_us = now_us;
}

void mello_transport_set_midi_channel(MelloTransport *t, uint8_t channel_1_to_16)
{
    if (!t) return;
    if (channel_1_to_16 < 1u) channel_1_to_16 = 1u;
    if (channel_1_to_16 > 16u) channel_1_to_16 = 16u;
    t->midi_channel = (uint8_t)(channel_1_to_16 - 1u);
}

void mello_transport_set_midi_sub_channel(MelloTransport *t, uint8_t channel_1_to_16)
{
    if (!t) return;
    if (channel_1_to_16 < 1u) channel_1_to_16 = 1u;
    if (channel_1_to_16 > 16u) channel_1_to_16 = 16u;
    t->midi_sub_channel = (uint8_t)(channel_1_to_16 - 1u);
}

void mello_transport_set_midi_note_out(MelloTransport *t, bool enabled)
{
    if (!t) return;
    if (!enabled) all_notes_off(t);
    t->midi_note_out_enabled = enabled;
}

void mello_transport_set_midi_fixed_length(MelloTransport *t, bool fixed, uint16_t milliseconds)
{
    if (!t) return;
    if (milliseconds < 20u) milliseconds = 20u;
    if (milliseconds > 2000u) milliseconds = 2000u;
    t->midi_fixed_length = fixed;
    t->midi_fixed_len_ms = milliseconds;
}

void mello_transport_set_bpm(MelloTransport *t, uint16_t bpm)
{
    if (!t) return;
    if (bpm < 20u) bpm = 20u;
    if (bpm > 300u) bpm = 300u;
    t->engine.project.bpm = bpm;
    recalc_timing(t);
}

void mello_transport_start(MelloTransport *t, uint32_t now_us)
{
    if (!t || t->running) return;
    if (t->sync_mode == MELLO_SYNC_SLAVE) return; /* wait for MIDI Start/Continue */
    start_engine(t, now_us, false);
}

void mello_transport_stop(MelloTransport *t)
{
    if (!t) return;
    if (t->running && t->sync_mode == MELLO_SYNC_MASTER) send_now(t, 0xFCu, 0u, 0u);
    all_notes_off(t);
    mello_engine_stop(&t->engine);
    t->running = false;
    t->slave_clock_phase = 0u;
}

static void poll_internal_transport(MelloTransport *t, uint32_t now_us)
{
    uint8_t catchup = 0u;
    if (!t || !t->running || t->sync_mode == MELLO_SYNC_SLAVE) return;
    while (time_due(now_us, t->next_pulse_us) && catchup < MELLO_MAX_CATCHUP_PULSES) {
        uint32_t due = t->next_pulse_us;
        process_one_pulse(t, due);
        t->next_pulse_us = advance_fractional_deadline(t->next_pulse_us, t->pulse_base_us, t->pulse_rem,
                                                       t->pulse_den, &t->pulse_error);
        catchup++;
    }
}

static void poll_master_clock(MelloTransport *t, uint32_t now_us)
{
    uint8_t catchup = 0u;
    if (!t || !t->running || t->sync_mode != MELLO_SYNC_MASTER) return;
    while (time_due(now_us, t->next_clock_us) && catchup < MELLO_MAX_MASTER_CLOCK_CATCHUP) {
        send_now(t, 0xF8u, 0u, 0u);
        t->next_clock_us = advance_fractional_deadline(t->next_clock_us, t->clock_base_us, t->clock_rem,
                                                       t->clock_den, &t->clock_error);
        catchup++;
    }
}

void mello_transport_poll(MelloTransport *t, uint32_t now_us)
{
    if (!t) return;
    dispatch_scheduled(t, now_us);
    poll_master_clock(t, now_us);
    poll_internal_transport(t, now_us);
    dispatch_scheduled(t, now_us);
}

static void slave_clock_tick(MelloTransport *t, uint32_t now_us)
{
    uint8_t needed;
    if (!t) return;
    if (t->slave_last_clock_us) {
        uint32_t dt = now_us - t->slave_last_clock_us;
        if (dt > 1000u && dt < 100000u) {
            t->slave_interval_sum += dt;
            if (t->slave_interval_count < 24u) t->slave_interval_count++;
            if (t->slave_interval_count >= 8u) {
                uint32_t avg = t->slave_interval_sum / t->slave_interval_count;
                uint32_t den = avg * 24u;
                if (den) {
                    uint32_t b = US_PER_MINUTE / den;
                    if (b >= 30u && b <= 300u) {
                        t->measured_bpm = (uint16_t)b;
                        t->engine.project.bpm = (uint16_t)b;
                    }
                }
                if (t->slave_interval_count == 24u) {
                    t->slave_interval_sum = 0u;
                    t->slave_interval_count = 0u;
                }
            }
        } else {
            t->slave_interval_sum = 0u;
            t->slave_interval_count = 0u;
        }
    }
    t->slave_last_clock_us = now_us;
    if (!t->running) return;
    needed = (uint8_t)(6u * (t->engine.project.clock_div == 2u   ? 2u
                             : t->engine.project.clock_div == 4u ? 4u
                                                                 : 1u));
    t->slave_clock_phase++;
    if (t->slave_clock_phase >= needed) {
        t->slave_clock_phase = 0u;
        process_one_pulse(t, now_us);
    }
}

void mello_transport_midi_in(MelloTransport *t, uint8_t status, uint8_t d1, uint8_t d2, uint32_t now_us)
{
    (void)d1;
    (void)d2;
    if (!t || t->sync_mode != MELLO_SYNC_SLAVE) return;
    if (status == 0xF8u) {
        slave_clock_tick(t, now_us);
    } else if (status == 0xFAu || status == 0xFBu) {
        /* Browser source only calls handlePlay when currently stopped; repeated Start/Continue
         * while already running does not restart sequence phase. */
        if (!t->running) {
            start_engine(t, now_us, true);
            process_one_pulse(t, now_us); /* browser transport emits first sequence event at start */
        }
    } else if (status == 0xFCu) {
        all_notes_off(t);
        mello_engine_stop(&t->engine);
        t->running = false;
        t->slave_clock_phase = 0u;
    }
}

uint16_t mello_transport_effective_bpm(const MelloTransport *t)
{
    if (!t) return 0u;
    return (t->sync_mode == MELLO_SYNC_SLAVE && t->measured_bpm) ? t->measured_bpm : t->engine.project.bpm;
}

bool mello_transport_schedule_overflowed(const MelloTransport *t)
{
    return t ? t->schedule_overflow : false;
}
