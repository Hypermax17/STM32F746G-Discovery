#ifndef BSP_SD_H
#define BSP_SD_H

#include <stdint.h>
#include "bsp_common.h"

/* microSD-Karte (SDMMC1, 4-Bit). GERUEST: Die Funktionen liefern BSP_ERR_NOT_IMPLEMENTED, bis der
 * Treiber (bsp/src/optional/bsp_sd.c) geschrieben und BSP_ENABLE_SD=ON gesetzt ist.
 * Siehe docs/optionale-module.md. BSP_HAS_SD ist 1, sobald der echte Treiber gebaut wird. */

typedef struct {
    uint32_t block_count;     /* Anzahl 512-Byte-Bloecke */
    uint32_t block_size;      /* Bytes pro Block (512) */
} bsp_sd_info_t;

int bsp_sd_init(void);                                   /* SDMMC, Pins, Karte initialisieren */
int bsp_sd_card_present(void);                           /* 1 = Karte steckt (Detect-Pin PC13) */
int bsp_sd_get_info(bsp_sd_info_t *info);
int bsp_sd_read_blocks(uint32_t block, void *buf, uint32_t count);
int bsp_sd_write_blocks(uint32_t block, const void *buf, uint32_t count);

#endif
