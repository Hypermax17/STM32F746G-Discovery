/* Platzhalter fuer SD: wird gebaut, solange BSP_ENABLE_SD=OFF ist.
 * Der echte Treiber gehoert nach bsp/src/optional/bsp_sd.c (siehe docs/optionale-module.md). */
#include "bsp_sd.h"

int bsp_sd_init(void)
{
    return BSP_ERR_NOT_IMPLEMENTED;
}

int bsp_sd_card_present(void)
{
    return 0;   /* Abfrage: "nein" (kein Fehlercode, damit if(...) nicht wahr wird) */
}

int bsp_sd_get_info(bsp_sd_info_t *info)
{
    (void)info;
    return BSP_ERR_NOT_IMPLEMENTED;
}

int bsp_sd_read_blocks(uint32_t block, void *buf, uint32_t count)
{
    (void)block;
    (void)buf;
    (void)count;
    return BSP_ERR_NOT_IMPLEMENTED;
}

int bsp_sd_write_blocks(uint32_t block, const void *buf, uint32_t count)
{
    (void)block;
    (void)buf;
    (void)count;
    return BSP_ERR_NOT_IMPLEMENTED;
}
