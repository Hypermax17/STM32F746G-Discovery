/* PC-Test der LVGL-Oberflaeche: rendert jede Seite in .rgb565-Dateien (Framebuffer-Dump).
 * Die Daten (Messwerte, Log, Systemzustand) sind synthetisch; die Seiten, Widgets, Designs und die
 * Seitenverwaltung sind dieselben Quelldateien wie in der Firmware. */
#include "lvgl.h"
#include "ui_pages.h"
#include "pages.h"
#include "app_ui.h"
#include "data_hub.h"
#include "bsp_dht.h"
#include "sysinfo.h"
#include "bsp_console.h"
#include "bsp_lcd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Attrappen fuer Firmware-Funktionen ------------------------------------------------ */

void ui_assert_halt(void) { fprintf(stderr, "LVGL assert\n"); abort(); }
void audio_task_beep(int on) { (void)on; }

#define NSAMPLES 100U
static data_sample_t g_hub[NSAMPLES];
static unsigned g_count = NSAMPLES;
uint32_t data_hub_count(void) { return g_count; }
int data_hub_get(uint32_t serial, data_sample_t *out)
{
    if (serial >= g_count) return -1;
    *out = g_hub[serial];
    return 0;
}

static const char *const k_log[] = {
    "  0.01 STM32F746G-Discovery", "  0.01 Version v0.1.0-14-gabc1234", "  0.01 Build Nov 12 2026 10:11:12",
    "  0.01 FreeRTOS V11.3.3", "  0.20 Display OK (480x272 RGB565)", "  0.20 Takt: 216 MHz",
    "  0.45 SDRAM: 8192 KB, Test OK (212 ms)", "  0.45 Reset: Power-On", "  0.50 Touch: FT5336 OK (ID 51)",
    "  0.50 Taster B1 bereit", "  0.52 QSPI: 16 MB ID 20 BA 18", "  0.52 QSPI: Mapping 0x90000000 ok",
    "  0.53 CPU: 41.3 C, VDDA 3.31 V", "  0.60 Audio: WM8994 (ID 8994) 48000 Hz", "  1.20 Audio: Unterbrechung (1)",
    "  2.00 Touch: I2C-Fehler", "  3.00 Taste: kurz (120 ms)",
};
#define NLOG (sizeof k_log / sizeof k_log[0])
uint32_t bsp_console_line_total(void) { return NLOG; }
int bsp_console_read_line(uint32_t serial, char *out, size_t sz, uint16_t *color)
{
    if (serial >= NLOG) return -1;
    snprintf(out, sz, "%s", k_log[serial]);
    *color = BSP_COLOR_WHITE;
    if (strstr(k_log[serial], "Unterbrechung")) *color = BSP_COLOR_YELLOW;
    if (strstr(k_log[serial], "Fehler"))        *color = BSP_COLOR_RED;
    if (strstr(k_log[serial], "Version"))       *color = BSP_COLOR_GRAY;
    return 0;
}

static int g_sensor_ok = 1;
void sysinfo_gather(sysinfo_t *s)
{
    memset(s, 0, sizeof *s);
    s->touch_ok = 1; s->qspi_ok = 1; s->audio_ok = g_sensor_ok; s->button_ok = 1;
    s->sdram_mb = 8; s->qspi_mb = 16; s->uptime_s = 4711; s->heap_free = 71 * 1024; s->heap_min = 66 * 1024;
    s->heap_total = 96 * 1024; s->cpu_percent = 7; s->sensor_ok = 1; s->cpu_temp_c = 41.26f; s->vdda_v = 3.31f;
    static const struct { const char *n; int p, st, c; } t[8] = {
        {"Tmr Svc", 6, 480, 0}, {"audio", 3, 700, 4}, {"touch", 2, 380, 1}, {"ui", 2, 650, 6},
        {"button", 2, 900, 0}, {"sensor", 1, 410, 0}, {"heartbeat", 1, 190, 0}, {"IDLE", 0, 112, 83}};
    for (int i = 0; i < 8; i++) {
        strcpy(s->task[i].name, t[i].n); s->task[i].prio = (uint8_t)t[i].p;
        s->task[i].stack_free_words = (uint16_t)t[i].st; s->task[i].cpu_percent = (uint8_t)t[i].c;
    }
    s->task_count = 8;
}

/* ---- Display-Treiber: Framebuffer im RAM ------------------------------------------------------- */

static uint16_t g_fb[BSP_LCD_WIDTH * BSP_LCD_HEIGHT];
static uint32_t g_now;
static uint32_t tick_cb(void) { return g_now; }
static void flush_cb(lv_display_t *d, const lv_area_t *a, uint8_t *p) { (void)a; (void)p; lv_display_flush_ready(d); }

static void run_ms(uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += 20) { g_now += 20; lv_timer_handler(); }
}

static void snap(const char *name)
{
    char path[256];
    snprintf(path, sizeof path, "%s/%s.rgb565", getenv("OUT") ? getenv("OUT") : ".", name);
    FILE *f = fopen(path, "wb");
    fwrite(g_fb, 2, BSP_LCD_WIDTH * BSP_LCD_HEIGHT, f);
    fclose(f);
}

int main(void)
{
    for (unsigned i = 0; i < NSAMPLES; i++) {
        g_hub[i].t_s = 600 + 2 * i;
        for (unsigned k = 0; k < DATA_HUB_SENSORS; k++) {
            g_hub[i].ok[k] = (k == 0U) ? ((i % 17U) != 5U) : 1U;       /* Sensor 1: gelegentlich eine fehlgeschlagene Messung */
            g_hub[i].temp_c[k] = (float)(int)(24.0f + 3.0f * (float)k + 2.0f * sinf((float)i * (0.12f + 0.05f * (float)k)));
            g_hub[i].hum_pct[k] = (float)(int)(55.0f - 10.0f * (float)k + 8.0f * sinf((float)i * 0.09f + (float)k));
        }
    }

    lv_init();
    lv_tick_set_cb(tick_cb);
    lv_display_t *disp = lv_display_create(BSP_LCD_WIDTH, BSP_LCD_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, g_fb, NULL, sizeof g_fb, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(disp, flush_cb);

    static const ui_page_def_t pages[] = {
        { "System", LV_SYMBOL_SETTINGS, page_system_create, page_system_update },
        { "Messwerte", LV_SYMBOL_CHARGE, page_values_create, page_values_update },
        { "Log", LV_SYMBOL_LIST, page_log_create, page_log_update },
    };
    static const char *const page_name[] = { "system", "messwerte", "log" };

    ui_pages_init(pages, 3, &ui_design);
    for (unsigned p = 0; p < 3; p++) {
        ui_pages_show(p);
        run_ms(400);
        char name[64];
        snprintf(name, sizeof name, "%u_%s", p + 1, page_name[p]);
        snap(name);
    }

    /* Auslieferungszustand (wie die Firmware) */
    app_ui_init();
    run_ms(400);
    snap("0_start");

    /* Sensor 2 nicht angeschlossen */
    for (unsigned i = 0; i < NSAMPLES; i++) g_hub[i].ok[1] = 0;
    ui_pages_init(pages, 3, &ui_design);
    ui_pages_show(1);
    run_ms(400);
    snap("2_messwerte_sensor2_fehlt");

    /* beide Sensoren nicht angeschlossen */
    for (unsigned i = 0; i < NSAMPLES; i++) g_hub[i].ok[0] = 0;
    ui_pages_init(pages, 3, &ui_design);
    ui_pages_show(1);
    run_ms(400);
    snap("2_messwerte_ohne_sensor");

    /* Noch keine Messwerte (direkt nach dem Start) */
    g_count = 0;
    ui_pages_init(pages, 3, &ui_design);
    ui_pages_show(1);
    run_ms(400);
    snap("2_messwerte_leer");
    printf("fertig\n");
    return 0;
}
