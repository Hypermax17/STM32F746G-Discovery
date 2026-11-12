/* Platzhalter fuer Kamera: wird gebaut, solange BSP_ENABLE_CAMERA=OFF ist.
 * Der echte Treiber gehoert nach bsp/src/optional/bsp_camera.c (siehe docs/optionale-module.md). */
#include "bsp_camera.h"

int bsp_camera_init(uint16_t width, uint16_t height)
{
    (void)width;
    (void)height;
    return BSP_ERR_NOT_IMPLEMENTED;
}

int bsp_camera_start_continuous(uint16_t *frame_buffer)
{
    (void)frame_buffer;
    return BSP_ERR_NOT_IMPLEMENTED;
}

int bsp_camera_stop(void)
{
    return BSP_ERR_NOT_IMPLEMENTED;
}
