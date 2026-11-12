#include "bsp_gfx.h"
#include "bsp_lcd.h"
#include "bsp_font.h"

#include <string.h>

void bsp_gfx_pixel(uint32_t x, uint32_t y, uint16_t color)
{
    if (x < BSP_LCD_WIDTH && y < BSP_LCD_HEIGHT) {
        bsp_lcd_framebuffer()[y * BSP_LCD_WIDTH + x] = color;
    }
}

void bsp_gfx_draw_char(uint32_t x, uint32_t y, char c, uint16_t fg, uint16_t bg, uint32_t scale)
{
    const uint32_t cell_w = 6U * scale;
    const uint32_t cell_h = 8U * scale;
    if (x + cell_w > BSP_LCD_WIDTH || y + cell_h > BSP_LCD_HEIGHT) {
        return;
    }
    const uint8_t *glyph = NULL;
    if ((uint8_t)c >= BSP_FONT_FIRST_CHAR && (uint8_t)c <= BSP_FONT_LAST_CHAR) {
        glyph = bsp_font5x7[(uint8_t)c - BSP_FONT_FIRST_CHAR];
    }

    uint16_t *fb = bsp_lcd_framebuffer();
    for (uint32_t cy = 0; cy < cell_h; cy++) {
        const uint32_t gy = cy / scale;                 /* Glyphenzeile 0..7 (7 = Abstand) */
        uint16_t *line = &fb[(y + cy) * BSP_LCD_WIDTH + x];
        for (uint32_t cx = 0; cx < cell_w; cx++) {
            const uint32_t gx = cx / scale;             /* Glyphenspalte 0..5 (5 = Abstand) */
            const int on = (glyph != NULL) && gx < 5U && gy < 7U && ((glyph[gx] >> gy) & 1U);
            line[cx] = on ? fg : bg;
        }
    }
}

void bsp_gfx_draw_text(uint32_t x, uint32_t y, const char *s, uint16_t fg, uint16_t bg, uint32_t scale)
{
    for (; *s != '\0'; s++) {
        bsp_gfx_draw_char(x, y, *s, fg, bg, scale);
        x += 6U * scale;
    }
}

uint32_t bsp_gfx_text_width(const char *s, uint32_t scale)
{
    return (uint32_t)strlen(s) * 6U * scale;
}

void bsp_gfx_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
{
    const int32_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    const int32_t dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);      /* negativ */
    const int32_t sx = (x0 < x1) ? 1 : -1;
    const int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;

    for (;;) {
        if (x0 >= 0 && y0 >= 0) {
            bsp_gfx_pixel((uint32_t)x0, (uint32_t)y0, color);
        }
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void bsp_gfx_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color)
{
    if (w == 0U || h == 0U) {
        return;
    }
    bsp_lcd_fill_rect(x, y, w, 1, color);
    bsp_lcd_fill_rect(x, y + h - 1U, w, 1, color);
    bsp_lcd_fill_rect(x, y, 1, h, color);
    bsp_lcd_fill_rect(x + w - 1U, y, 1, h, color);
}

void bsp_gfx_fill_circle(int32_t cx, int32_t cy, int32_t r, uint16_t color)
{
    for (int32_t dy = -r; dy <= r; dy++) {
        int32_t dx = 0;
        while ((dx + 1) * (dx + 1) + dy * dy <= r * r) {
            dx++;
        }
        const int32_t y = cy + dy;
        const int32_t x0 = cx - dx;
        if (y < 0 || x0 < 0) {
            continue;
        }
        bsp_lcd_fill_rect((uint32_t)x0, (uint32_t)y, (uint32_t)(2 * dx + 1), 1, color);
    }
}
