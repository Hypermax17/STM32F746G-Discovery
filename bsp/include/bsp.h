#ifndef BSP_H
#define BSP_H

/* Board-Support-Package STM32F746G-Discovery.
 * bsp_init() bringt das Board bis zur Display-Konsole hoch:
 *   HAL, MPU/Caches, Takt 216 MHz, LED, SDRAM (+Test), LCD, Konsole.
 * Danach koennen BSP_LOGx()-Meldungen auf dem Display ausgegeben werden. */

#include "bsp_led.h"
#include "bsp_sdram.h"
#include "bsp_lcd.h"
#include "bsp_console.h"
#include "bsp_log.h"
#include "bsp_fault.h"

void bsp_init(void);

#endif
