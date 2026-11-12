#ifndef UI_RENDER_H
#define UI_RENDER_H

#include <stdint.h>

/* Zeichnen der Seiten (reine Darstellung, keine Betriebssystem-Aufrufe). Layout (Zellen 12x16 Pixel):
 *   Zeile 0 = Tab-Leiste, Zeilen 1..15 = Inhalt, Zeile 16 = Statuszeile (gehoert der Konsole). */

#define UI_TAB_H        16U
#define UI_CONTENT_Y    16U
#define UI_CONTENT_H    240U           /* bis y = 255 */
#define UI_TAB_COUNT    3U

typedef enum {
    UI_PAGE_SYSTEM = 0,
    UI_PAGE_TOUCH,
    UI_PAGE_LOG
} ui_page_t;

#define UI_MAX_TASKS 8U

typedef struct {
    char     name[16];
    uint8_t  prio;
    uint16_t stack_free_words;     /* kleinste je freie Stackgroesse */
    uint8_t  cpu_percent;
} ui_task_info_t;

typedef struct {
    uint8_t  touch_ok, qspi_ok, audio_ok, button_ok;
    uint8_t  sdram_mb, qspi_mb;
    uint8_t  has_sd, has_usb, has_eth, has_cam;      /* 1 = echter Treiber eingebunden (sonst nur Geruest) */
    uint32_t uptime_s;
    uint32_t heap_free, heap_min, heap_total;        /* Bytes */
    uint8_t  cpu_percent;                            /* Auslastung gesamt (100 - Idle) */
    uint8_t  sensor_ok;                              /* 1 = cpu_temp_c und vdda_v sind gueltig */
    float    cpu_temp_c;                             /* Chiptemperatur in Grad Celsius */
    float    vdda_v;                                 /* Versorgungsspannung VDDA in Volt */
    uint8_t  task_count;
    ui_task_info_t task[UI_MAX_TASKS];
} ui_system_t;

void ui_render_tabs(ui_page_t active);
void ui_render_system(const ui_system_t *s);

void ui_render_touch_background(void);
/* Zeichnet einen Beruehrungspunkt (Slot 0..4); mit has_prev wird eine Linie vom letzten Punkt gezogen. */
void ui_render_touch_point(unsigned slot, unsigned x, unsigned y, int has_prev, unsigned px, unsigned py);

#endif
