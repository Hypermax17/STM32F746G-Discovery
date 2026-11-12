/* Platzhalter fuer Ethernet: wird gebaut, solange BSP_ENABLE_ETH=OFF ist.
 * Der echte Treiber gehoert nach bsp/src/optional/bsp_eth.c (siehe docs/optionale-module.md). */
#include "bsp_eth.h"

int bsp_eth_init(const uint8_t mac[6])
{
    (void)mac;
    return BSP_ERR_NOT_IMPLEMENTED;
}

int bsp_eth_link_up(void)
{
    return 0;   /* Abfrage: "nein" (kein Fehlercode, damit if(...) nicht wahr wird) */
}

int bsp_eth_send_frame(const void *data, uint32_t len)
{
    (void)data;
    (void)len;
    return BSP_ERR_NOT_IMPLEMENTED;
}

int bsp_eth_recv_frame(void *buf, uint32_t max_len, uint32_t timeout_ms)
{
    (void)buf;
    (void)max_len;
    (void)timeout_ms;
    return BSP_ERR_NOT_IMPLEMENTED;
}
