/* PC-Test der Display-Konsole (Boot-Log, Ringpuffer, Fehleranzeige): rendert in .rgb565-Dateien. */
#include "bsp_lcd.h"
#include "bsp_console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void snap(const char *name)
{
    char path[256];
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
    for (int i = 1; i <= 40; i++) log_line(i);            /* scrollen */
    snap("c1_log_gescrollt");

    /* Ausblenden (UI uebernimmt), weiter loggen, Ring auslesen */
    bsp_console_set_visible(0);
    for (int i = 41; i <= 50; i++) log_line(i);
    char text[80]; uint16_t color = 0;
    int ok = bsp_console_read_line(bsp_console_line_total() - 1U, text, sizeof text, &color);
    printf("letzte fertige Zeile (%d): \"%s\"\n", ok, text);

    /* Wieder einblenden: aus dem Ringpuffer neu zeichnen */
    bsp_console_set_visible(1);
    snap("c2_log_wieder");

    /* Fehleranzeige: ganzer Bildschirm */
    bsp_console_clear_unlocked();
    bsp_console_write_unlocked("*** HARD FAULT ***\nPC  08001234  LR  08001235\n", BSP_COLOR_RED);
    bsp_console_status_unlocked("Reset-Taster druecken / neu flashen");
    snap("c3_fault");
    return 0;
}
