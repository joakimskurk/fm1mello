/* SPDX-License-Identifier: GPL-3.0-only */
/* Dinghy's platform: everything an FM-1 firmware needs before it can be fun, taken unchanged from
 * SLOOP / Felucca / sloopDX and wired up here. You should not need to touch this file.
 *
 *   boot + boot guard   a firmware that crashes at boot falls back to the USB rescue, then to the ROM
 *   USB rescue          OCT- held at power-on: no audio, only the updater (the web installer still works)
 *   updater             the M-UPGRADE SysEx path: the web installer / the hub can always reach the FM-1
 *   MIDI                USB in and out, the FM-1's TRS jack in (midi_get: 1 USB, 2 TRS; midi_send)
 *   USB audio           the output as a class-compliant 16-bit stereo input, 44.1 or 48 kHz
 *   keys, knobs, LEDs   fm1_in.notes / buttons, fm1_input_edges, fm1_enc_take, fm1_led_key (hal/fm1_input.h)
 *   screen              240 x 240, lcd_fill, draw_text_box with FONT_S (16 px) / FONT_L (32 px) (gfx.c)
 *   settings            one 4 KiB flash sector with a CRC (settings_load / settings_save)
 *   second core         2ND CORE ON: started at power-on, proved by a self-test (cpu1_boot); cpu1_run / cpu1_wait
 *   crash screen        the PC of a fault, then a reboot
 *
 * The app (dinghy.c) includes this file first and supplies:
 *   static void app_audio(int32_t *out, uint32_t n)   audio ISR: n stereo frames, L R L R, Q15 (+-32767), out is zero
 *   static void app_main(void)                        never returns; calls platform_poll() often (every few ms)
 */
/* the panel by name: buttons (fm1_in.buttons, fm1_input_edges, fm1_led_key: bit / id), the encoders
 * (fm1_enc_take); the 27 note keys are fm1_in.notes bit 0 (F3) .. 26 (G5), LED id 14 + key */
enum { BTN_OCT_DOWN = 0, BTN_OCT_UP = 1, BTN_FX = 2, BTN_SEL = 3, BTN_ENV = 4, BTN_LFO = 5, BTN_EDIT = 6,
       BTN_GLO = 7, BTN_HOME = 8, BTN_SAVE = 9, BTN_ARP = 10, BTN_SEQ = 11, BTN_PLAY = 12, BTN_REC = 13 };
enum { ENC_SELECT = 0, ENC_ALGORITHM = 1, ENC_KNOB1 = 2, ENC_KNOB2 = 3, ENC_KNOB3 = 4, ENC_KNOB4 = 5, ENC_PRESETS = 6 };
#define KEY_LED(k) (14u + (k))

#if defined(DINGHY_HOST) && (defined(__APPLE__) || defined(DINGHY_WEB))
#define FM1_SECTION(s)                         /* host mode on macOS (Mach-O names need a segment) and in a browser
                                                * (WebAssembly has none): none needed */
#else
#define FM1_SECTION(s) __attribute__((section(s)))
#endif
#include "mirror.h"                             /* Big screen: the LCD's writes mirrored over USB MIDI (mirror.c) */
#ifdef DINGHY_HOST
#ifdef DINGHY_WEB
#include "../../web/app/web_host.c"            /* in a browser: the page's FM-1 (web/app/), WebAssembly */
#else
#include "../../tests/host.c"                  /* on a computer: a fake FM-1 (tests/host.c) */
#endif
#else
#include <stdint.h>
#include "fm1_time.h"
#include "fm1_rand.h"                             /* the chip's random register: SEED AUTO (dinghy.c) */
#include "fm1_sys.h"
#include "fm1_irq.h"
#include "fm1_guard.h"
#include "fm1_cpu1.h"                             /* the second core (X0X, as SLOOP 2.8.3 has it) */
#define FM1_SETTLE_US 100u /* the boot scan (OCT- for the rescue): 10 us read no key on an FM-1 (Dinghy 0.4) */
#include "fm1_input.h"
static void fm1_led_key_glow(uint32_t id, int on)   /* as fm1_led_key, the dim layer (~1/24): "there is something here" */
{
    uint32_t p, r;
    for (p = 0; p < FM1_NCOL; p++)
        for (r = 1; r < 5u; r++)
            if (FM1_KEYMAP[r][p] == (int8_t)id) {
                if (on) fm1_led_dim[p] |= (uint8_t)(1u << r);
                else fm1_led_dim[p] &= (uint8_t)~(1u << r);
            }
}
#include "fm1_timer.h"
#include "fm1_lcd_hw.h"
#include "fm1_audio.h"
#include "fm1_adc.h"
#include "bootguard.h"

#ifndef APP_NAME
#define APP_NAME "DINGHY"
#endif
#ifndef FELUCCA_ID
#define FELUCCA_ID "FM-1_990"                   /* package identity: 99N = Dinghy build N (tools/build.py) */
#endif
#ifndef APP_VER
#define APP_VER "0.0"
#endif

bootguard_t bootguard FM1_SECTION(".noinit");
static volatile uint32_t fm1_ms;                /* milliseconds since power-on (TIMER5) */

#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#include "libc.c"
#define SM_BUFFER_ATTR FM1_SECTION(".pool")     /* the mirror's 16 KB queue in the pool, beside the delay and the reverb */
#define MIRROR_NOW() fm1_ms
#include "screen_mirror.c"                      /* MIT, Patrick F. Kellogg: the rows as SysEx */
#include "mirror.c"
#include "lcd.c"
#define CV_MAX (240u * 32u)                     /* Mello: FONT_L is the tallest canvas user */
#include "gfx.c"

#define FS 44100u                               /* the sample rate */
#define HALF_FRAMES 256u                        /* I2S half buffer: 5.8 ms (as sloopDX: the USB audio band is set for it) */
#define HALF_WORDS (HALF_FRAMES * 2u)
#define FELUCCA_OTA 1
#define FELUCCA_CDC 0
#define FELUCCA_UAC 1
#define FELUCCA_UART 1                          /* the FM-1's TRS MIDI IN (Dinghy 0.4) */
#include "usb.c"
#if FELUCCA_UART
#include "midi_uart.c"                          /* (Dinghy 0.4: sloopDX's, after Felucca Salt: the ring read by content; cable 1) */
#endif

/* ---- flash: read anywhere; erase / program only the update's staging area and the settings sector */
#define SETTINGS_AT 0x000FC000u                 /* one 4 KiB sector of Felucca's globals area */
#define MELLO_STORAGE_LO 0x00098000u
#define MELLO_STORAGE_HI 0x000E0000u
#define FL_RANGE_OK(off, n) (FL_IN(off, n, FL_OTA_LO, FL_OTA_HI) || \
                             FL_IN(off, n, SETTINGS_AT, SETTINGS_AT + 0x1000u) || \
                             FL_IN(off, n, MELLO_STORAGE_LO, MELLO_STORAGE_HI))
#include "fm1_flash.h"
static uint8_t flash_ok;
static int st_read(uint32_t off, void *dst, uint32_t n)
{
    uint8_t *d = dst;
    while (n) {
        uint32_t k = n > 256u ? 256u : n, f = irq_save();
        int rc = FL_FAR(fl_read_ram)(off, d, k);
        irq_restore(f);
        if (rc)
            return rc;
        off += k;
        d += k;
        n -= k;
    }
    return 0;
}

static uint8_t recovery_active;
#define OTA_IDENTITY (recovery_active ? "FM-1_000" : FELUCCA_ID)
#include "ota.c"
static void recovery_poll(void);
static uint32_t ota_now_ms(void) { return fm1_ms; }
static void ota_idle(void)
{
    fm1_wdt_feed();
    if (recovery_active) recovery_poll();
}
static int ota_in_area(uint32_t off, uint32_t n) { return FL_IN(off, n, OTA_AREA, OTA_AREA + OTA_AREA_LEN); }
static int ota_erase(uint32_t off)
{
    uint32_t took;
    if (!ota_in_area(off, 0x1000u) || (off & 0xFFFu))
        return -8;
    return fl_erase4k(off, &took);
}
static int ota_prog(uint32_t off, const void *p, uint32_t n)
{
    if (!ota_in_area(off, n))
        return -8;
    return fl_write(off, p, n);
}
static int ota_fread(uint32_t off, void *p, uint32_t n) { return st_read(off, p, n); }
static void ota_show(uint32_t step, int32_t code)
{
    static const char *const STEP[] = {"", "PACKAGE", "CHECK HEAD", "LOADER", "CONFIRM", "RESTART"};
    if (recovery_active) return;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(0, 92, 240, &FONT_S, "UPDATE", C_WHITE, 1);
    if (step < 9u)
        draw_text_box(0, 124, 240, &FONT_S, STEP[step < 6u ? step : 0], C_WHITE, 1);
    else
        draw_text_box(0, 124, 240, &FONT_S, code == 1 ? "DRY RUN OK" : "FAILED", C_WHITE, 1);
}
static void ota_commit(const uint8_t *parm)
{
    bootguard_clear(&bootguard);
    usb_detach();
    fm1_delay_ms(30);
    fm1_enter_update(parm);
}
#include "recovery.c"

/* ---- settings: [magic][length][crc32] then the bytes; settings_save erases the sector (~50 ms with
 * the interrupts off: the sound drops out), so save when nothing plays, not on every knob step */
#define SETTINGS_MAGIC 0x59474E44u              /* "DNGY" */
static uint32_t crc32(const uint8_t *p, uint32_t n)
{
    uint32_t c = 0xFFFFFFFFu, k;
    while (n--) {
        c ^= *p++;
        for (k = 0; k < 8u; k++)
            c = c & 1u ? (c >> 1) ^ 0xEDB88320u : c >> 1;
    }
    return ~c;
}
static int settings_load(void *p, uint32_t n)   /* 0: loaded; else p is untouched */
{
    static uint8_t buf[256];
    uint32_t h[3];
    if (!flash_ok || n > sizeof buf || st_read(SETTINGS_AT, h, sizeof h) || h[0] != SETTINGS_MAGIC || h[1] != n)
        return -1;
    if (st_read(SETTINGS_AT + sizeof h, buf, n) || crc32(buf, n) != h[2])
        return -1;
    memcpy(p, buf, n);
    return 0;
}
static int settings_save(const void *p, uint32_t n)   /* p in RAM, n <= 256 */
{
    uint32_t h[3] = {SETTINGS_MAGIC, n, 0}, took;
    int rc;
    if (!flash_ok || n > 256u)
        return -1;
    h[2] = crc32(p, n);
    if ((rc = fl_erase4k(SETTINGS_AT, &took)) != 0)
        return rc;
    if ((rc = fl_write(SETTINGS_AT + sizeof h, p, n)) != 0)
        return rc;
    return fl_write(SETTINGS_AT, (const uint8_t *)h, sizeof h);   /* the header last: a cut save stays invalid */
}

/* ---- the second core (2ND CORE). After SLOOP's cpu1.c (Copyright (C) 2026 Andre Rezemini, GPL-3.0-only: the SLOOP
 * dualcore branch, github.com/andrerezemini/sloop-lab) over X0X's HAL (hal/fm1_cpu1.h). Started at power-on, before
 * the audio and the timers (X0X: started later, core 1 never reached its entry), and only when the platform's
 * settings say 2ND CORE ON: OFF (the default) holds it, and the boot is as before. Proved by a self-test job that
 * runs from the flash, as the app's will; one that does not report in, answers wrong or comes back late leaves the
 * FM-1 on one core (fm1_cpu1_wait: then until power-off). The app hands it work through cpu1_run / cpu1_wait. */
enum { CPU1_OFF, CPU1_NOSTART, CPU1_FAILED, CPU1_OK }; /* switched off / did not report in / wrong, late, barred / on */
typedef void (*cpu1_job_t)(uint32_t);
#ifndef MELLO_CPU1_DEFAULT
#define MELLO_CPU1_DEFAULT 0 /* 2ND CORE while the sector holds no settings (MELLO_CFLAGS=-DMELLO_CPU1_DEFAULT=1: ON) */
#endif
#define PLAT_SETTINGS_MAGIC 0x534B4D31u /* "SKM1": the platform's own settings, in the sector above */
static struct {
    uint32_t magic;
    uint8_t second_core, spare;
    uint16_t seed; /* GLOBAL > SYSTEM's SEED (dinghy.c): 0 AUTO, else that number (was spare, 0: AUTO) */
} plat_settings;
static uint8_t cpu1_state = CPU1_OFF;
static uint32_t cpu1_timeouts;                 /* jobs core 1 did not finish in time, or read wrong */
static volatile uint32_t cpu1_test_out;

static int cpu1_on(void) { return cpu1_state == CPU1_OK && fm1_c1_on; }
static void cpu1_sync(void) { fm1_cpu1_sync(); } /* between the cores' writes and reads (the shared caches) */
static uint32_t cpu1_micros(void) { return fm1_micros(); } /* TIMER4: both cores read it */
/* hand core 1 a job: 1 = it took it (cpu1_wait before touching what it works on); 0 = run it yourself */
static int cpu1_run(cpu1_job_t fn, uint32_t arg) { return cpu1_state == CPU1_OK ? fm1_cpu1_run(fn, arg) : 0; }
/* 0 = done; -1 = read wrong or late: core 1 is held until power-off, and the caller does the job itself */
static int cpu1_wait(void)
{
    if (fm1_cpu1_wait() == 0)
        return 0;
    cpu1_timeouts++;
    cpu1_state = CPU1_FAILED;
    return -1;
}

static uint32_t cpu1_lcg(uint32_t x)           /* 64 steps of an LCG: the self-test's work */
{
    uint32_t i;
    for (i = 0; i < 64u; i++)
        x = x * 1664525u + 1013904223u;
    return x;
}
static void cpu1_selftest(uint32_t arg) { cpu1_test_out = cpu1_lcg(arg); }   /* on core 1, from the flash */

/* at power-on, with the flash up (the settings) and before the audio, the timers and fm1_guard_lock_top (the entry
 * word is in the reserved top of RAM). The PC limits are open while it starts (it boots through the chip's ROM) */
static void cpu1_boot(void)
{
    if (settings_load(&plat_settings, sizeof plat_settings) || plat_settings.magic != PLAT_SETTINGS_MAGIC) {
        memset(&plat_settings, 0, sizeof plat_settings);
        plat_settings.magic = PLAT_SETTINGS_MAGIC;
        plat_settings.second_core = MELLO_CPU1_DEFAULT;
    }
    if (!plat_settings.second_core)
        return;
    fm1_supplies_stock();                       /* VDC14 1.60 V, SYSVDD 1.38 V, as M-VAVE's own (fm1_sys.h) */
    fm1_guard_pc_open();
    if (fm1_cpu1_start() != 0) {
        fm1_guard_enable(FM1_GUARD_PC);
        cpu1_state = fm1_c1_gave_up == 3u ? CPU1_FAILED : CPU1_NOSTART;   /* (3: given up since power-on) */
        return;
    }
    fm1_guard_enable(FM1_GUARD_PC);
    cpu1_test_out = 0;
    if (!fm1_cpu1_run(cpu1_selftest, 20261110u) || fm1_cpu1_wait() != 0 || cpu1_test_out != cpu1_lcg(20261110u)) {
        fm1_cpu1_hold();                        /* answered, but wrong or late: one core from here on */
        cpu1_state = CPU1_FAILED;
        return;
    }
    cpu1_state = CPU1_OK;
}

/* ---- MIDI: USB and the TRS jack share one ring of 4-byte USB-MIDI packets (USB cable 0, the jack cable 1) */
static int midi_get(uint8_t *st, uint8_t *d1, uint8_t *d2)   /* 1 (USB) or 2 (TRS): a message; 0: none */
{
    uint32_t pk;
    if (mi_r == mi_w)
        return 0;
    pk = midi_in_q[mi_r % MQ];
    mi_r++;
    *st = (uint8_t)(pk >> 8);
    *d1 = (uint8_t)(pk >> 16);
    *d2 = (uint8_t)(pk >> 24);
    return pk & 0xF0u ? 2 : 1;
}
static void midi_send(uint32_t st, uint32_t d1, uint32_t d2)   /* USB out, from the main loop only */
{
    midi_out_event((st >> 4) | st << 8 | (d1 & 0x7Fu) << 16 | (d2 & 0x7Fu) << 24);
}

/* ---- the timer: 10 kHz key scan, milliseconds, USB at 2 kHz. It outranks the audio ISR; nested in a
 * render it only scans, counts and feeds the USB audio stream (as sloopDX main.c) */
static volatile uint8_t in_audio;
void fm1_timer5_irq(void)
{
    static uint32_t sub, owed, last, acc;
    uint32_t t0 = fm1_ticks(), usb_due = sub % 5u == 0u;
    fm1_timer5_ack();
    fm1_input_tick();
    acc += t0 - last;
    last = t0;
    while (acc >= 1000u * FM1_TICKS_PER_US) {
        acc -= 1000u * FM1_TICKS_PER_US;
        fm1_ms++;
    }
    if (usb_due)
        owed |= 1u;
#if FELUCCA_UART
    if (sub % 5u == 2u)
        owed |= 2u;                             /* 2 kHz: <= ~7 bytes per call at 31250 baud */
#endif
    if (++sub == 10u)
        sub = 0;
    if (in_audio) {
        if (usb_due)
            uac_service();
        return;
    }
    if (owed & 1u)
        usb_poll();
#if FELUCCA_UART
    if (owed & 2u)
        uart_midi_poll();
#endif
    owed = 0;
}
extern void isr_timer5(void);

/* ---- the sound: ALNK0 I2S, two halves; the app renders, the MASTER knob scales, Q15 -> 24 bit (-6 dBFS ceiling) */
static int32_t abuf[2u * HALF_WORDS] __attribute__((aligned(4)));
static volatile int32_t master_q12 = 2048;
static void app_audio(int32_t *out, uint32_t n);
static void app_main(void);

void fm1_alnk0_irq(void)                       /* via isr_alnk0 (hal/fm1_isr.S) */
{
    uint8_t p;
    in_audio = 1;
    p = fm1_audio_pending();
    fm1_audio_ack_aux(p);
    if (p & FM1_AUDIO_HALF) {
        int32_t *o = &abuf[fm1_audio_free_half() * HALF_WORDS];
        uint32_t i, b;
        for (i = 0; i < HALF_WORDS; i++)
            o[i] = 0;
        uac_render_start();
        app_audio(o, HALF_FRAMES);
        for (i = 0; i < HALF_WORDS; i++) {
            int32_t v = (o[i] * master_q12) >> 12;
            o[i] = v > 32767 ? 32767 : v < -32767 ? -32767 : v;
        }
        for (b = 0; b < HALF_FRAMES; b += 32u)
            uac_tap(o + 2u * b, 32u);           /* the computer records what the speaker plays */
        for (i = 0; i < HALF_WORDS; i++)
            o[i] <<= 7;
        fm1_audio_ack_half();
    }
    in_audio = 0;
}
extern void isr_alnk0(void);

/* ---- the main loop's duties: watchdog, USB, the installer's update request, the MASTER knob */
static void platform_poll(void)
{
    static int32_t knob_avg = 512 * 16;
    int32_t a;
    fm1_wdt_feed();
    usb_retry(fm1_ms);
    ota_service();
    if (usb.mirror_hello) {                     /* Big screen: the app asked for the screen */
        uint8_t h = usb.mirror_hello;
        usb.mirror_hello = 0;
        mirror_hello(h == 2u);
    }
    mirror_poll(fm1_ms, usb_mirror_send);
    if (usb.ota_req) {
        usb.ota_req = 0;
        if (flash_ok)
            ota_session();                      /* returns only if nothing was committed */
    }
    if (usb.uboot_req) {
        fm1_audio_stop();
        lcd_fill(0, 0, 240, 240, C_BLACK);
        draw_text_box(0, 110, 240, &FONT_S, "UBOOT (USB)", C_WHITE, 1);
        usb_detach();
        fm1_delay_ms(30);
        bootguard.pending = 0;
        fm1_enter_uboot();
    }
    if (fm1_ms >= 30000u && bootguard.pending)
        bootguard_clear(&bootguard);            /* 30 s up: this firmware boots */
    if ((a = fm1_adc_read(FM1_ADC_MASTER)) >= 0) {
        uint32_t k10;
        knob_avg += (a * 16 - knob_avg) / 8;
        k10 = (uint32_t)(knob_avg / 16);
        master_q12 = (int32_t)((k10 * k10) >> 8);
    }
}

static void fm1_fault(const fm1_crash_t *c)
{
    char b[12];
    uint32_t i, t0, v[3] = {c->vec, c->pc, c->rets};
    if (fm1_c1_on)      /* a crash beside the second core: one core until power-off (SLOOP 2.8.3, X0X ed35a45) */
        fm1_cpu1_bar();
    fm1_audio_stop();
    lcd_fill(0, 0, 240, 240, RGB(160, 0, 0));
    draw_text_box(0, 20, 240, &FONT_S, APP_NAME " CRASH", C_WHITE, 1);
    for (i = 0; i < 3u; i++) {
        uint32_t k;
        for (k = 0; k < 8u; k++) b[k] = "0123456789ABCDEF"[(v[i] >> (28u - 4u * k)) & 15u];
        b[8] = 0;
        draw_text_box(10, 60 + i * 20, 220, &FONT_S, b, C_WHITE, 0);
    }
    t0 = fm1_ticks();
    while ((uint32_t)(fm1_ticks() - t0) < 8000u * 1000u * FM1_TICKS_PER_US)
        fm1_wdt_feed();
    fm1_reboot();
}

void fm1_cstart(void)
{
    extern uint32_t _data_start[], _data_end[], _data_load[], _bss_start[], _bss_end[];
    extern uint32_t _pool_start[], _pool_end[], _rt_start[], _rt_end[], _rt_load[], _c1_start[], _c1_end[], _c1_load[];
    uint32_t *s, *d, p3, boot_mode, jedec;
    fm1_cpu1_halt(); /* a second core left running by a reset would read the RAM cleared below (SLOOP 2.8.3) */
    fm1_time_init();
    fm1_reset_reason();
    p3 = fm1_boot.p3_rst;
    fm1_wdt_arm(0x0D);
    if ((p3 & 1u) && !(p3 & (4u | 0x40u)))
        bootguard_clear(&bootguard);
    boot_mode = bootguard_begin(&bootguard);
    if (boot_mode == BOOT_ROM)
        fm1_enter_uboot();
    fm1_irq_init();
    for (d = _bss_start; d < _bss_end; d++) *d = 0;
    for (d = _pool_start; d < _pool_end; d++) *d = 0;
    for (s = _data_load, d = _data_start; d < _data_end; s++, d++) *d = *s;
    for (s = _rt_load, d = _rt_start; d < _rt_end; s++, d++) *d = *s;
    for (s = _c1_load, d = _c1_start; d < _c1_end; s++, d++) *d = *s; /* the second core's loop (fm1_cpu1.h) */
    fm1_mailbox_clear();
    fm1_guard_enable(FM1_GUARD_STACK | FM1_GUARD_WRITE | FM1_GUARD_BUS | FM1_GUARD_PC);
    fm1_boot.p3_rst = (uint8_t)p3;
    if (boot_mode == BOOT_RECOVERY || recovery_key())
        recovery_main();                        /* OCT- at power-on: the USB rescue */
    jedec = FL_FAR(fl_jedec_ram)();             /* (one read: FL_JEDEC_OK names its argument twice) */
    flash_ok = FL_JEDEC_OK(jedec);
    ota_boot_cleanup();
    lcd_init();
    lcd_fill(0, 0, 240, 240, C_BLACK);
    fm1_input_init();
    fm1_adc_init();
    cpu1_boot();                                /* 2ND CORE ON: the second core, before the audio and the timers */
    fm1_audio_init(abuf, HALF_WORDS, isr_alnk0, 3);   /* (abuf is zero: .bss) */
    usb_start();
#if FELUCCA_UART
    uart_midi_init();                           /* before TIMER5: PORTH read-modify-write */
#endif
    fm1_timer5_start(isr_timer5, 4);
    fm1_guard_lock_top();
    fm1_irq_enable_all();
    app_main();
    for (;;)
        platform_poll();
}
#endif
