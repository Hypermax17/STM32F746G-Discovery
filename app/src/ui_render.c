#include "ui_render.h"
#include "bsp_gfx.h"
#include "bsp_lcd.h"
#include "bsp_console.h"

#include <stdio.h>
#include <string.h>

#define SCALE   BSP_CONSOLE_FONT_SCALE
#define CELL_W  BSP_CONSOLE_CELL_W
#define CELL_H  BSP_CONSOLE_CELL_H
#define COLS    BSP_CONSOLE_COLS

#define COL_BG        0x0000U
#define COL_TEXT      BSP_COLOR_WHITE
#define COL_DIM       BSP_COLOR_GRAY
#define COL_HEAD      BSP_COLOR_CYAN
#define COL_OK        BSP_COLOR_GREEN
#define COL_BAD       BSP_COLOR_RED
#define COL_TAB_BG    BSP_RGB565(40, 40, 40)
#define COL_TAB_ACT   BSP_RGB565(0, 200, 230)
#define COL_GRID      BSP_RGB565(28, 28, 28)

/* ---- Textzeile mit Farbe je Zeichen -------------------------------------- */

typedef struct {
    char     ch[COLS + 1U];
    uint16_t color[COLS];
} line_t;

static void line_init(line_t *l)
{
    memset(l->ch, ' ', COLS);
    l->ch[COLS] = '\0';
    for (unsigned i = 0; i < COLS; i++) {
        l->color[i] = COL_TEXT;
    }
}

static void line_put(line_t *l, unsigned col, const char *s, uint16_t color)
{
    for (; *s != '\0' && col < COLS; s++, col++) {
        l->ch[col] = *s;
        l->color[col] = color;
    }
}

/* Zeile komplett zeichnen (alle 40 Zellen), damit alter Inhalt ohne Flackern ueberschrieben wird */
static void line_draw(const line_t *l, unsigned row)
{
    for (unsigned i = 0; i < COLS; i++) {
        bsp_gfx_draw_char(i * CELL_W, row * CELL_H, l->ch[i], l->color[i], COL_BG, SCALE);
    }
}

/* ---- Tab-Leiste ----------------------------------------------------------- */

void ui_render_tabs(ui_page_t active)
{
    static const char *const names[UI_TAB_COUNT] = { "SYSTEM", "TOUCH", "LOG" };
    const uint32_t tab_w = BSP_LCD_WIDTH / UI_TAB_COUNT;

    for (unsigned i = 0; i < UI_TAB_COUNT; i++) {
        const int is_active = ((unsigned)active == i);
        const uint16_t bg = is_active ? COL_TAB_ACT : COL_TAB_BG;
        const uint16_t fg = is_active ? BSP_COLOR_BLACK : COL_TEXT;
        bsp_lcd_fill_rect(i * tab_w, 0, tab_w, UI_TAB_H, bg);
        const uint32_t w = bsp_gfx_text_width(names[i], SCALE);
        bsp_gfx_draw_text(i * tab_w + (tab_w - w) / 2U, 0, names[i], fg, bg, SCALE);
    }
}

/* ---- Systemseite ---------------------------------------------------------- */

/* "Name   OK  Detail" in einer Haelfte (20 Spalten) */
static void put_status(line_t *l, unsigned col, const char *label, int ok, const char *detail)
{
    line_put(l, col, label, COL_TEXT);
    line_put(l, col + 8U, ok ? "OK" : "FEHLT", ok ? COL_OK : COL_BAD);
    if (ok && detail != NULL) {
        line_put(l, col + 11U, detail, COL_DIM);
    }
}

void ui_render_system(const ui_system_t *s)
{
    line_t l;
    char buf[48];
    unsigned row = 1;

    line_init(&l);
    line_put(&l, 0, "PERIPHERIE", COL_HEAD);
    snprintf(buf, sizeof buf, "up %lus", (unsigned long)s->uptime_s);
    line_put(&l, COLS - (unsigned)strlen(buf), buf, COL_DIM);
    line_draw(&l, row++);

    line_init(&l);
    put_status(&l, 0, "Display", 1, "480x272");
    snprintf(buf, sizeof buf, "%u MB", (unsigned)s->sdram_mb);
    put_status(&l, 20, "SDRAM", s->sdram_mb != 0U, buf);
    line_draw(&l, row++);

    line_init(&l);
    put_status(&l, 0, "Touch", s->touch_ok, "FT5336");
    snprintf(buf, sizeof buf, "%u MB", (unsigned)s->qspi_mb);
    put_status(&l, 20, "QSPI", s->qspi_ok, buf);
    line_draw(&l, row++);

    line_init(&l);
    put_status(&l, 0, "Audio", s->audio_ok, "WM8994");
    put_status(&l, 20, "Taster", s->button_ok, "B1");
    line_draw(&l, row++);

    /* Optionale Module: Geruest oder echter Treiber */
    line_init(&l);
    {
        static const char *const names[4] = { "SD", "USB", "ETH", "CAM" };
        const uint8_t has[4] = { s->has_sd, s->has_usb, s->has_eth, s->has_cam };
        unsigned col = 0;
        unsigned any = 0;
        for (unsigned i = 0; i < 4; i++) {
            snprintf(buf, sizeof buf, "%s:%s", names[i], has[i] ? "ok" : "-");
            line_put(&l, col, buf, has[i] ? COL_OK : COL_DIM);
            col += (unsigned)strlen(buf) + 1U;
            any |= has[i];
        }
        if (!any) {
            line_put(&l, col + 1U, "(nur Geruest)", COL_DIM);
        }
    }
    line_draw(&l, row++);

    line_init(&l);
    line_draw(&l, row++);

    line_init(&l);
    line_put(&l, 0, "TASKS", COL_HEAD);
    snprintf(buf, sizeof buf, "CPU %u%%", (unsigned)s->cpu_percent);
    line_put(&l, 8, buf, s->cpu_percent > 80U ? COL_BAD : COL_TEXT);
    snprintf(buf, sizeof buf, "Heap %luK frei (min %luK)", (unsigned long)(s->heap_free / 1024U),
             (unsigned long)(s->heap_min / 1024U));
    line_put(&l, 17, buf, COL_TEXT);
    line_draw(&l, row++);

    line_init(&l);
    line_put(&l, 0, "Name         Pr  Stack  CPU", COL_DIM);
    line_draw(&l, row++);

    for (unsigned i = 0; i < UI_MAX_TASKS && row <= BSP_CONSOLE_LAST_LOG_ROW; i++) {
        line_init(&l);
        if (i < s->task_count) {
            const ui_task_info_t *t = &s->task[i];
            snprintf(buf, sizeof buf, "%-12.12s %2u  %5u  %3u%%", t->name, (unsigned)t->prio,
                     (unsigned)t->stack_free_words, (unsigned)t->cpu_percent);
            line_put(&l, 0, buf, COL_TEXT);
        }
        line_draw(&l, row++);
    }
    for (; row <= BSP_CONSOLE_LAST_LOG_ROW; row++) {
        line_init(&l);
        line_draw(&l, row);
    }
}

/* ---- Touch-Seite ---------------------------------------------------------- */

#define TOUCH_RADIUS 6

void ui_render_touch_background(void)
{
    bsp_lcd_fill_rect(0, UI_CONTENT_Y, BSP_LCD_WIDTH, UI_CONTENT_H, COL_BG);
    for (uint32_t x = 40; x < BSP_LCD_WIDTH; x += 40U) {
        bsp_lcd_fill_rect(x, UI_CONTENT_Y, 1, UI_CONTENT_H, COL_GRID);
    }
    for (uint32_t y = UI_CONTENT_Y + 40U; y < UI_CONTENT_Y + UI_CONTENT_H; y += 40U) {
        bsp_lcd_fill_rect(0, y, BSP_LCD_WIDTH, 1, COL_GRID);
    }
    bsp_gfx_draw_text(8, UI_CONTENT_Y + 8U, "Beruehren zum Zeichnen", COL_DIM, COL_BG, SCALE);
    bsp_gfx_draw_text(8, UI_CONTENT_Y + UI_CONTENT_H - 20U, "Tab TOUCH: loeschen", COL_GRID, COL_BG, SCALE);
}

void ui_render_touch_point(unsigned slot, unsigned x, unsigned y, int has_prev, unsigned px, unsigned py)
{
    static const uint16_t colors[5] = {
        BSP_COLOR_RED, BSP_COLOR_GREEN, BSP_COLOR_CYAN, BSP_COLOR_YELLOW, BSP_RGB565(255, 100, 255)
    };
    const uint16_t c = colors[slot % 5U];
    if (has_prev) {
        bsp_gfx_line((int32_t)px, (int32_t)py, (int32_t)x, (int32_t)y, c);
    }
    bsp_gfx_fill_circle((int32_t)x, (int32_t)y, TOUCH_RADIUS, c);
}
