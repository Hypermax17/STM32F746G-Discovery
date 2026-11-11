#ifndef BSP_LCD_H
#define BSP_LCD_H

#include <stdint.h>

/* 4,3"-Display RK043FN48H, 480x272, RGB565, Framebuffer im SDRAM (Layer 1) */
#define BSP_LCD_WIDTH  480U
#define BSP_LCD_HEIGHT 272U
#define BSP_LCD_FB_ADDR 0xC0000000UL   /* siehe MEMORY SDRAM_FB im Linker-Script */

/* RGB565-Farben */
#define BSP_RGB565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3)))
#define BSP_COLOR_BLACK  0x0000U
#define BSP_COLOR_WHITE  0xFFFFU
#define BSP_COLOR_RED    BSP_RGB565(255, 64, 64)
#define BSP_COLOR_GREEN  BSP_RGB565(64, 255, 64)
#define BSP_COLOR_YELLOW BSP_RGB565(255, 220, 0)
#define BSP_COLOR_GRAY   BSP_RGB565(150, 150, 150)
#define BSP_COLOR_CYAN   BSP_RGB565(0, 220, 255)

/* Voraussetzung: SDRAM ist initialisiert. Rueckgabe 0 = ok. */
int bsp_lcd_init(void);

uint16_t *bsp_lcd_framebuffer(void);
void bsp_lcd_fill(uint16_t color);
void bsp_lcd_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color);
void bsp_lcd_backlight(int on);

#endif
