#ifndef BSP_GFX_H
#define BSP_GFX_H

#include <stdint.h>

/* Einfache Zeichenfunktionen auf dem Framebuffer (RGB565). Text: Font 5x7, skaliert; eine Zelle
 * ist (6*scale) x (8*scale) Pixel. Alle Funktionen schneiden an den Displayraendern ab. */

void bsp_gfx_pixel(uint32_t x, uint32_t y, uint16_t color);
void bsp_gfx_draw_char(uint32_t x, uint32_t y, char c, uint16_t fg, uint16_t bg, uint32_t scale);
void bsp_gfx_draw_text(uint32_t x, uint32_t y, const char *s, uint16_t fg, uint16_t bg, uint32_t scale);
uint32_t bsp_gfx_text_width(const char *s, uint32_t scale);

void bsp_gfx_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color);
void bsp_gfx_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color);   /* Rahmen */
void bsp_gfx_fill_circle(int32_t cx, int32_t cy, int32_t r, uint16_t color);

#endif
