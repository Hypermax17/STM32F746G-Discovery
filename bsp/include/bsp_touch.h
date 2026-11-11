#ifndef BSP_TOUCH_H
#define BSP_TOUCH_H

#include <stdint.h>

/* Kapazitiver Touchscreen FT5336 (I2C3, Adresse 0x38), bis zu 5 Beruehrungspunkte.
 * Die Koordinaten werden auf das Display (0..479, 0..271) abgebildet. Die
 * Ausrichtung des Controllers relativ zum Display ist einstellbar. */

#define BSP_TOUCH_MAX_POINTS 5U

/* Standard-Ausrichtung (ST-Referenz fuer dieses Board) */
#ifndef BSP_TOUCH_DEFAULT_SWAP_XY
#define BSP_TOUCH_DEFAULT_SWAP_XY 1
#endif
#ifndef BSP_TOUCH_DEFAULT_INVERT_X
#define BSP_TOUCH_DEFAULT_INVERT_X 0
#endif
#ifndef BSP_TOUCH_DEFAULT_INVERT_Y
#define BSP_TOUCH_DEFAULT_INVERT_Y 0
#endif

typedef struct {
    uint16_t x, y;          /* Display-Koordinaten */
    uint16_t raw_x, raw_y;  /* Rohwerte des Controllers */
} bsp_touch_point_t;

typedef struct {
    uint8_t count;                                  /* Anzahl aktiver Punkte (0 = keine Beruehrung) */
    bsp_touch_point_t point[BSP_TOUCH_MAX_POINTS];
} bsp_touch_state_t;

/* Voraussetzung: bsp_i2c3_init(). Rueckgabe 0 = FT5336 gefunden, *chip_id = Vendor-ID (0x51). */
int bsp_touch_init(uint8_t *chip_id);
int bsp_touch_available(void);

/* Aktuellen Zustand lesen (Polling, ca. 1 ms). Rueckgabe 0 = ok, sonst I2C-Fehler. */
int bsp_touch_read(bsp_touch_state_t *state);

void bsp_touch_set_orientation(int swap_xy, int invert_x, int invert_y);

#endif
