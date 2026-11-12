#ifndef BSP_H
#define BSP_H

/* Board-Support-Package STM32F746G-Discovery.
 * bsp_init() bringt das Board bis zur Display-Konsole hoch:
 *   HAL, MPU/Caches, Takt 216 MHz, LED, SDRAM (+Test), LCD, Konsole, I2C3, Touch, User-Taster, QSPI-Flash.
 * Danach koennen BSP_LOGx()-Meldungen auf dem Display ausgegeben werden. */

#include "bsp_led.h"
#include "bsp_sdram.h"
#include "bsp_lcd.h"
#include "bsp_console.h"
#include "bsp_log.h"
#include "bsp_fault.h"
#include "bsp_i2c.h"
#include "bsp_touch.h"
#include "bsp_button.h"
#include "bsp_qspi.h"
#include "bsp_audio.h"
#include "bsp_common.h"
#include "bsp_sd.h"
#include "bsp_usb.h"
#include "bsp_eth.h"
#include "bsp_camera.h"

void bsp_init(void);

#endif
