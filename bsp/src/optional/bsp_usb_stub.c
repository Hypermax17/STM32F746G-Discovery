/* Platzhalter fuer USB: wird gebaut, solange BSP_ENABLE_USB=OFF ist.
 * Der echte Treiber gehoert nach bsp/src/optional/bsp_usb.c (siehe docs/optionale-module.md). */
#include "bsp_usb.h"

int bsp_usb_init(bsp_usb_mode_t mode)
{
    (void)mode;
    return BSP_ERR_NOT_IMPLEMENTED;
}

int bsp_usb_start(void)
{
    return BSP_ERR_NOT_IMPLEMENTED;
}

int bsp_usb_stop(void)
{
    return BSP_ERR_NOT_IMPLEMENTED;
}

int bsp_usb_connected(void)
{
    return 0;   /* Abfrage: "nein" (kein Fehlercode, damit if(...) nicht wahr wird) */
}
