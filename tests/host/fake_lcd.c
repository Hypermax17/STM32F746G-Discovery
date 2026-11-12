/* Ersatz fuer bsp_lcd.c: Framebuffer im normalen RAM */
#include "bsp_lcd.h"
static uint16_t fb[BSP_LCD_WIDTH * BSP_LCD_HEIGHT];
uint16_t *bsp_lcd_framebuffer(void) { return fb; }
void bsp_lcd_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color)
{
    if (x >= BSP_LCD_WIDTH || y >= BSP_LCD_HEIGHT) return;
    if (x + w > BSP_LCD_WIDTH) w = BSP_LCD_WIDTH - x;
    if (y + h > BSP_LCD_HEIGHT) h = BSP_LCD_HEIGHT - y;
    for (uint32_t r = y; r < y + h; r++)
        for (uint32_t i = 0; i < w; i++) fb[r * BSP_LCD_WIDTH + x + i] = color;
}
void bsp_lcd_fill(uint16_t color) { bsp_lcd_fill_rect(0, 0, BSP_LCD_WIDTH, BSP_LCD_HEIGHT, color); }
