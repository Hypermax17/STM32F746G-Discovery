#include "bsp_touch.h"
#include "bsp_i2c.h"
#include "bsp_lcd.h"

#include "stm32f7xx_hal.h"

#define FT5336_ADDR        0x38U     /* 7-Bit; 8-Bit-Schreibadresse 0x70 */
#define FT5336_REG_TD_STAT 0x02U     /* Anzahl der Punkte (Bit 3:0) */
#define FT5336_REG_P1      0x03U     /* Punkt 1: XH XL YH YL Gewicht Misc, danach im 6-Byte-Raster */
#define FT5336_REG_VENDOR  0xA8U
#define FT5336_VENDOR_ID   0x51U
#define FT5336_EVENT_UP    1U        /* Ereignis "Abheben" in XH Bit 7:6 */

static int s_available;
static int s_swap_xy   = BSP_TOUCH_DEFAULT_SWAP_XY;
static int s_invert_x  = BSP_TOUCH_DEFAULT_INVERT_X;
static int s_invert_y  = BSP_TOUCH_DEFAULT_INVERT_Y;

void bsp_touch_set_orientation(int swap_xy, int invert_x, int invert_y)
{
    s_swap_xy  = swap_xy;
    s_invert_x = invert_x;
    s_invert_y = invert_y;
}

int bsp_touch_init(uint8_t *chip_id)
{
    uint8_t id = 0;
    /* Der Controller braucht nach dem Einschalten etwas Zeit */
    for (int tries = 0; tries < 5; tries++) {
        if (bsp_i2c3_mem_read(FT5336_ADDR, FT5336_REG_VENDOR, &id, 1) == 0 && id == FT5336_VENDOR_ID) {
            break;
        }
        HAL_Delay(50);
    }
    if (chip_id != NULL) {
        *chip_id = id;
    }
    s_available = (id == FT5336_VENDOR_ID);
    return s_available ? 0 : -1;
}

int bsp_touch_available(void)
{
    return s_available;
}

static uint16_t clamp(uint16_t v, uint16_t max)
{
    return (v > max) ? max : v;
}

int bsp_touch_read(bsp_touch_state_t *state)
{
    state->count = 0;
    if (!s_available) {
        return -1;
    }

    uint8_t n = 0;
    if (bsp_i2c3_mem_read(FT5336_ADDR, FT5336_REG_TD_STAT, &n, 1) != 0) {
        return -1;
    }
    n &= 0x0FU;
    if (n == 0U || n > BSP_TOUCH_MAX_POINTS) {
        return 0;                          /* keine (oder ungueltige) Beruehrung */
    }

    uint8_t buf[BSP_TOUCH_MAX_POINTS * 6U];
    if (bsp_i2c3_mem_read(FT5336_ADDR, FT5336_REG_P1, buf, (uint16_t)(n * 6U)) != 0) {
        return -1;
    }

    for (uint8_t i = 0; i < n; i++) {
        const uint8_t *p = &buf[i * 6U];
        if (((p[0] >> 6) & 0x3U) == FT5336_EVENT_UP) {
            continue;                      /* Finger wurde abgehoben */
        }
        uint16_t rx = (uint16_t)(((p[0] & 0x0FU) << 8) | p[1]);
        uint16_t ry = (uint16_t)(((p[2] & 0x0FU) << 8) | p[3]);

        uint16_t x = s_swap_xy ? ry : rx;
        uint16_t y = s_swap_xy ? rx : ry;
        x = clamp(x, BSP_LCD_WIDTH - 1U);
        y = clamp(y, BSP_LCD_HEIGHT - 1U);
        if (s_invert_x) x = (uint16_t)(BSP_LCD_WIDTH - 1U - x);
        if (s_invert_y) y = (uint16_t)(BSP_LCD_HEIGHT - 1U - y);

        bsp_touch_point_t *dst = &state->point[state->count++];
        dst->x = x;
        dst->y = y;
        dst->raw_x = rx;
        dst->raw_y = ry;
    }
    return 0;
}
