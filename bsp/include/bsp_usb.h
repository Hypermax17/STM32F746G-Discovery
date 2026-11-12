#ifndef BSP_USB_H
#define BSP_USB_H

#include <stdint.h>
#include "bsp_common.h"

/* USB OTG (Full-Speed an CN12, High-Speed mit ULPI-PHY an CN13). GERUEST: Die Funktionen liefern
 * BSP_ERR_NOT_IMPLEMENTED, bis der Treiber (bsp/src/optional/bsp_usb.c) geschrieben und
 * BSP_ENABLE_USB=ON gesetzt ist. Der USB-Protokollstapel (Device-Klassen oder Host) kommt als
 * Middleware oberhalb dieses Treibers. Siehe docs/optionale-module.md. */

typedef enum {
    BSP_USB_FS_DEVICE = 0,
    BSP_USB_FS_HOST,
    BSP_USB_HS_DEVICE,
    BSP_USB_HS_HOST
} bsp_usb_mode_t;

int bsp_usb_init(bsp_usb_mode_t mode);
int bsp_usb_start(void);
int bsp_usb_stop(void);
int bsp_usb_connected(void);      /* 1 = VBUS/Verbindung erkannt */

#endif
