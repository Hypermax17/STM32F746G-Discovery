#ifndef BSP_CAMERA_H
#define BSP_CAMERA_H

#include <stdint.h>
#include "bsp_common.h"

/* Kamera-Anschluss (DCMI, 8-Bit parallel, Sensor-Steuerung ueber I2C/SCCB). GERUEST: Die Funktionen
 * liefern BSP_ERR_NOT_IMPLEMENTED, bis der Treiber (bsp/src/optional/bsp_camera.c) geschrieben und
 * BSP_ENABLE_CAMERA=ON gesetzt ist. Siehe docs/optionale-module.md. */

int bsp_camera_init(uint16_t width, uint16_t height);
int bsp_camera_start_continuous(uint16_t *frame_buffer);        /* DMA in einen RGB565-Framebuffer */
int bsp_camera_stop(void);

#endif
