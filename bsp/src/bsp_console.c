#include "bsp_console.h"
#include "bsp_lcd.h"
#include "bsp_font.h"

#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define SCALE  BSP_CONSOLE_FONT_SCALE
#define CELL_W (6U * SCALE)
#define CELL_H (8U * SCALE)
#define COLS   BSP_CONSOLE_COLS
#define ROWS   BSP_CONSOLE_ROWS
#define LOG_ROWS (ROWS - 1U)          /* letzte Zeile = Statuszeile */

#define COLOR_BG 0x0000U

static SemaphoreHandle_t s_mutex;
static int      s_ready;
static uint32_t s_col;
static uint32_t s_row;

static void draw_char(uint32_t col, uint32_t row, char c, uint16_t fg, uint16_t bg)
{
    uint16_t *fb = bsp_lcd_framebuffer();
    const uint8_t *glyph = NULL;
    if ((uint8_t)c >= BSP_FONT_FIRST_CHAR && (uint8_t)c <= BSP_FONT_LAST_CHAR) {
        glyph = bsp_font5x7[(uint8_t)c - BSP_FONT_FIRST_CHAR];
    }

    uint32_t x0 = col * CELL_W;
    uint32_t y0 = row * CELL_H;
    for (uint32_t cy = 0; cy < CELL_H; cy++) {
        uint32_t gy = cy / SCALE;                       /* Glyphenzeile 0..7 (7 = Abstand) */
        uint16_t *line = &fb[(y0 + cy) * BSP_LCD_WIDTH + x0];
        for (uint32_t cx = 0; cx < CELL_W; cx++) {
            uint32_t gx = cx / SCALE;                   /* Glyphenspalte 0..5 (5 = Abstand) */
            int on = (glyph != NULL) && gx < 5U && gy < 7U && ((glyph[gx] >> gy) & 1U);
            line[cx] = on ? fg : bg;
        }
    }
}

static void clear_log_row(uint32_t row)
{
    bsp_lcd_fill_rect(0, row * CELL_H, BSP_LCD_WIDTH, CELL_H, COLOR_BG);
}

static void scroll_up(void)
{
    uint16_t *fb = bsp_lcd_framebuffer();
    memmove(fb, fb + CELL_H * BSP_LCD_WIDTH, (LOG_ROWS - 1U) * CELL_H * BSP_LCD_WIDTH * sizeof(uint16_t));
    clear_log_row(LOG_ROWS - 1U);
}

static void newline(void)
{
    s_col = 0;
    if (s_row + 1U >= LOG_ROWS) {
        scroll_up();
    } else {
        s_row++;
    }
}

void bsp_console_write_unlocked(const char *text, uint16_t color)
{
    if (!s_ready) {
        return;
    }
    for (; *text != '\0'; text++) {
        char c = *text;
        if (c == '\n') {
            newline();
        } else if (c == '\r') {
            s_col = 0;
        } else {
            if (s_col >= COLS) {
                newline();
            }
            draw_char(s_col, s_row, c, color, COLOR_BG);
            s_col++;
        }
    }
}

static int locking_possible(void)
{
    return s_mutex != NULL && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

void bsp_console_write(const char *text, uint16_t color)
{
    if (locking_possible()) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        bsp_console_write_unlocked(text, color);
        xSemaphoreGive(s_mutex);
    } else {
        bsp_console_write_unlocked(text, color);
    }
}

static void status_draw(const char *text)
{
    for (uint32_t i = 0; i < COLS; i++) {
        char c = (*text != '\0') ? *text++ : ' ';
        draw_char(i, ROWS - 1U, c, BSP_COLOR_BLACK, BSP_COLOR_GRAY);
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
    if (locking_possible()) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        status_draw(text);
        xSemaphoreGive(s_mutex);
    } else {
        status_draw(text);
    }
}

void bsp_console_clear_unlocked(void)
{
    if (!s_ready) {
        return;
    }
    bsp_lcd_fill_rect(0, 0, BSP_LCD_WIDTH, LOG_ROWS * CELL_H, COLOR_BG);
    s_col = 0;
    s_row = 0;
}

int bsp_console_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        return -1;
    }
    bsp_lcd_fill(COLOR_BG);
    s_col = 0;
    s_row = 0;
    s_ready = 1;
    bsp_console_status("");
    return 0;
}

int bsp_console_ready(void)
{
    return s_ready;
}
