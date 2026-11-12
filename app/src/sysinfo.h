#ifndef SYSINFO_H
#define SYSINFO_H

#include <stdint.h>

/* Systemzustand fuer die Anzeige: Peripherie-Status, Heap, Tasks (Prioritaet, freier Stack, CPU-Anteil)
 * und die interne Chiptemperatur/Versorgungsspannung in Grad Celsius bzw. Volt. */

#define SYSINFO_MAX_TASKS 8U

typedef struct {
    char     name[16];
    uint8_t  prio;
    uint16_t stack_free_words;     /* kleinste je freie Stackgroesse */
    uint8_t  cpu_percent;          /* Anteil seit dem letzten Aufruf von sysinfo_gather() */
} sysinfo_task_t;

typedef struct {
    uint8_t  touch_ok, qspi_ok, audio_ok, button_ok;
    uint8_t  sdram_mb, qspi_mb;
    uint8_t  has_sd, has_usb, has_eth, has_cam;      /* 1 = echter Treiber eingebunden (sonst nur Geruest) */
    uint32_t uptime_s;
    uint32_t heap_free, heap_min, heap_total;        /* Bytes */
    uint8_t  cpu_percent;                            /* Auslastung gesamt (100 - Idle) */
    uint8_t  sensor_ok;                              /* 1 = cpu_temp_c und vdda_v gueltig */
    float    cpu_temp_c;                             /* Grad Celsius */
    float    vdda_v;                                 /* Volt */
    uint8_t  task_count;
    sysinfo_task_t task[SYSINFO_MAX_TASKS];
} sysinfo_t;

void sysinfo_gather(sysinfo_t *s);

#endif
