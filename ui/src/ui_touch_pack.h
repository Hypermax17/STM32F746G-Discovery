#ifndef UI_TOUCH_PACK_H
#define UI_TOUCH_PACK_H

#include <stdint.h>

/* Beruehrungszustand in einem 32-Bit-Wort (Lesen/Schreiben ist atomar):
 *   Bit 31 = beruehrt, Bit 16..26 = y, Bit 0..10 = x
 * Beim Loslassen bleibt die letzte Position erhalten (nur Bit 31 wird geloescht). LVGL wertet beim Loslassen
 * noch einmal die Position aus; wuerde hier 0/0 stehen, spraengen z. B. Schieberegler beim Loslassen auf 0. */

static inline uint32_t ui_touch_pack(uint32_t prev, int touched, uint32_t x, uint32_t y)
{
    if (touched) {
        return 0x80000000UL | ((y & 0x7FFU) << 16) | (x & 0x7FFU);
    }
    return prev & 0x7FFFFFFFUL;
}

static inline void ui_touch_unpack(uint32_t v, int32_t *x, int32_t *y, int *pressed)
{
    *x = (int32_t)(v & 0x7FFU);
    *y = (int32_t)((v >> 16) & 0x7FFU);
    *pressed = (v & 0x80000000UL) != 0U;
}

#endif
