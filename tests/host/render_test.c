/* PC-Test der Darstellung: rendert Konsole und UI-Seiten in .rgb565-Dateien (Framebuffer-Dump). */
#include "bsp_lcd.h"
#include "bsp_console.h"
#include "bsp_gfx.h"
#include "ui_render.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void snap(const char *name)
{
    char path[128];
    snprintf(path, sizeof path, "%s/%s.rgb565", getenv("OUT") ? getenv("OUT") : ".", name);
    FILE *f = fopen(path, "wb");
    fwrite(bsp_lcd_framebuffer(), 2, BSP_LCD_WIDTH * BSP_LCD_HEIGHT, f);
    fclose(f);
}

static void log_line(int n)
{
    char b[64];
    snprintf(b, sizeof b, "%4d.%02d Zeile %d: Beispieltext\n", n / 100, n % 100, n);
    bsp_console_write(b, n % 7 == 3 ? BSP_COLOR_YELLOW : BSP_COLOR_WHITE);
}

int main(void)
{
    bsp_console_init();
    bsp_console_status("up 12s heap 71K            T 240,136");
    for (int i = 1; i <= 8; i++) log_line(i);
    ui_render_tabs(UI_PAGE_LOG);
    snap("1_log_8zeilen");

    for (int i = 9; i <= 40; i++) log_line(i);            /* scrollen */
    snap("2_log_gescrollt");

    /* Seite SYSTEM */
    bsp_console_set_visible(0);
    log_line(41); log_line(42);                            /* nur Ringpuffer, kein Zeichnen */
    ui_render_tabs(UI_PAGE_SYSTEM);
    ui_system_t s = {0};
    s.touch_ok = 1; s.qspi_ok = 1; s.audio_ok = 1; s.button_ok = 1; s.sdram_mb = 8; s.qspi_mb = 16;
    s.sensor_ok = 1; s.cpu_temp_c = 41.26f; s.vdda_v = 3.31f; s.uptime_s = 4711; s.heap_free = 71 * 1024; s.heap_min = 66 * 1024; s.heap_total = 96 * 1024; s.cpu_percent = 7;
    s.task_count = 7;
    const struct { const char *n; int p, st, c; } t[7] = {
        {"Tmr Svc", 6, 480, 0}, {"audio", 3, 700, 4}, {"touch", 2, 380, 1}, {"ui", 2, 650, 1},
        {"button", 2, 900, 0}, {"heartbeat", 1, 410, 0}, {"IDLE", 0, 112, 93}};
    for (int i = 0; i < 7; i++) {
        strcpy(s.task[i].name, t[i].n); s.task[i].prio = t[i].p; s.task[i].stack_free_words = t[i].st; s.task[i].cpu_percent = t[i].c;
    }
    ui_render_system(&s);
    snap("3_system");

    /* Peripherie-Fehler */
    s.audio_ok = 0; s.qspi_ok = 0; s.cpu_temp_c = 88.4f; s.cpu_percent = 91; s.has_sd = 1;
    ui_render_system(&s);
    snap("4_system_fehler");

    /* Seite TOUCH: zwei Striche */
    ui_render_tabs(UI_PAGE_TOUCH);
    ui_render_touch_background();
    for (int i = 0; i < 60; i++) {
        unsigned x = 40 + i * 6, y = 120 + (i % 20) * 3;
        ui_render_touch_point(0, x, y, i > 0, x - 6, 120 + ((i - 1) % 20) * 3);
    }
    ui_render_touch_point(1, 300, 180, 0, 0, 0);
    ui_render_touch_point(2, 400, 60, 0, 0, 0);
    snap("5_touch");

    /* Zurueck zum Log: aus dem Ringpuffer neu zeichnen */
    ui_render_tabs(UI_PAGE_LOG);
    bsp_console_set_visible(1);
    snap("6_log_wieder");

    /* Fehleranzeige ueberschreibt auch die Tab-Leiste */
    bsp_console_clear_unlocked();
    bsp_console_write_unlocked("*** HARD FAULT ***\nPC  08001234  LR  08001235\n", BSP_COLOR_RED);
    snap("7_fault");
    return 0;
}
