#include "bsp_console.h"
#include "bsp_lcd.h"
#include "bsp_gfx.h"

#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define SCALE    BSP_CONSOLE_FONT_SCALE
#define CELL_W   BSP_CONSOLE_CELL_W
#define CELL_H   BSP_CONSOLE_CELL_H
#define COLS     BSP_CONSOLE_COLS
#define ROWS     BSP_CONSOLE_ROWS
#define LAST_ROW BSP_CONSOLE_LAST_LOG_ROW
#define STATUS_ROW BSP_CONSOLE_STATUS_ROW

#define RING_LINES 64U
#define COLOR_BG   0x0000U

static SemaphoreHandle_t s_mutex;
static int s_ready;
static int s_visible = 1;

/* Ringpuffer der Textzeilen (Zeichen + Farbe je Zeichen) */
static char     s_ch[RING_LINES][COLS];
static uint16_t s_cl[RING_LINES][COLS];
static uint8_t  s_len[RING_LINES];
static uint32_t s_head;        /* Index der aktuellen Zeile */
static uint32_t s_count;       /* gespeicherte Zeilen (>= 1) */
static uint32_t s_col;         /* Cursor-Spalte */
static uint32_t s_row;         /* Bildschirmzeile der aktuellen Zeile (nur gueltig, wenn sichtbar) */
static uint32_t s_first = BSP_CONSOLE_FIRST_LOG_ROW;

static void draw_cell(uint32_t col, uint32_t row, char c, uint16_t fg, uint16_t bg)
{
    bsp_gfx_draw_char(col * CELL_W, row * CELL_H, c, fg, bg, SCALE);
}

static void clear_rows(uint32_t first, uint32_t last)
{
    bsp_lcd_fill_rect(0, first * CELL_H, BSP_LCD_WIDTH, (last - first + 1U) * CELL_H, COLOR_BG);
}

static void ring_reset(void)
{
    s_head = 0;
    s_count = 1;
    s_len[0] = 0;
    s_col = 0;
    s_row = s_first;
}

static void scroll_up(void)
{
    uint16_t *fb = bsp_lcd_framebuffer();
    memmove(fb + s_first * CELL_H * BSP_LCD_WIDTH, fb + (s_first + 1U) * CELL_H * BSP_LCD_WIDTH,
            (LAST_ROW - s_first) * CELL_H * BSP_LCD_WIDTH * sizeof(uint16_t));
    clear_rows(LAST_ROW, LAST_ROW);
}

static void newline(void)
{
    s_head = (s_head + 1U) % RING_LINES;
    s_len[s_head] = 0;
    if (s_count < RING_LINES) {
        s_count++;
    }
    s_col = 0;
    if (s_visible) {
        if (s_row < LAST_ROW) {
            s_row++;
        } else {
            scroll_up();
        }
    }
}

/* Log-Bereich aus dem Ringpuffer neu zeichnen: die juengsten Zeilen, die aktuelle Zeile ganz unten. */
static void redraw(void)
{
    clear_rows(s_first, LAST_ROW);
    const uint32_t avail = LAST_ROW - s_first + 1U;
    const uint32_t n = (s_count < avail) ? s_count : avail;
    for (uint32_t k = 0; k < n; k++) {
        const uint32_t idx = (s_head + RING_LINES - (n - 1U - k)) % RING_LINES;
        for (uint32_t i = 0; i < s_len[idx]; i++) {
            draw_cell(i, s_first + k, s_ch[idx][i], s_cl[idx][i], COLOR_BG);
        }
    }
    s_row = s_first + n - 1U;
}

void bsp_console_write_unlocked(const char *text, uint16_t color)
{
    if (!s_ready) {
        return;
    }
    for (; *text != '\0'; text++) {
        const char c = *text;
        if (c == '\n') {
            newline();
        } else if (c == '\r') {
            s_col = 0;
        } else {
            if (s_col >= COLS) {
                newline();
            }
            s_ch[s_head][s_col] = c;
            s_cl[s_head][s_col] = color;
            if (s_col >= s_len[s_head]) {
                s_len[s_head] = (uint8_t)(s_col + 1U);
            }
            if (s_visible) {
                draw_cell(s_col, s_row, c, color, COLOR_BG);
            }
            s_col++;
        }
    }
}

static int locking_possible(void)
{
    return s_mutex != NULL && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

static void lock(void)   { if (locking_possible()) xSemaphoreTake(s_mutex, portMAX_DELAY); }
static void unlock(void) { if (locking_possible()) xSemaphoreGive(s_mutex); }

void bsp_console_write(const char *text, uint16_t color)
{
    lock();
    bsp_console_write_unlocked(text, color);
    unlock();
}

static void status_draw(const char *text)
{
    for (uint32_t i = 0; i < COLS; i++) {
        const char c = (*text != '\0') ? *text++ : ' ';
        draw_cell(i, STATUS_ROW, c, BSP_COLOR_BLACK, BSP_COLOR_GRAY);
    }
}

void bsp_console_status_unlocked(const char *text)
{
    if (s_ready) {
        status_draw(text);
    }
}

void bsp_console_status(const char *text)
{
    if (!s_ready) {
        return;
    }
    lock();
    status_draw(text);
    unlock();
}

void bsp_console_set_visible(int visible)
{
    if (!s_ready) {
        return;
    }
    lock();
    s_visible = visible ? 1 : 0;
    if (s_visible) {
        redraw();
    }
    unlock();
}

int bsp_console_is_visible(void)
{
    return s_visible;
}

void bsp_console_clear_unlocked(void)
{
    if (!s_ready) {
        return;
    }
    s_first = 0;                 /* Fehleranzeige nutzt auch die Zeile der Tab-Leiste */
    s_visible = 1;
    ring_reset();
    clear_rows(0, LAST_ROW);
}

int bsp_console_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        return -1;
    }
    s_first = BSP_CONSOLE_FIRST_LOG_ROW;
    s_visible = 1;
    ring_reset();
    bsp_lcd_fill(COLOR_BG);
    s_ready = 1;
    status_draw("");
    return 0;
}

int bsp_console_ready(void)
{
    return s_ready;
}
