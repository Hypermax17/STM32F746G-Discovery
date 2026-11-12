#ifndef BSP_ETH_H
#define BSP_ETH_H

#include <stdint.h>
#include "bsp_common.h"

/* Ethernet (RMII, PHY LAN8742A). GERUEST: Die Funktionen liefern BSP_ERR_NOT_IMPLEMENTED, bis der
 * Treiber (bsp/src/optional/bsp_eth.c) geschrieben und BSP_ENABLE_ETH=ON gesetzt ist. Ein IP-Stack
 * (z. B. lwIP) setzt auf bsp_eth_send_frame/bsp_eth_recv_frame auf. Siehe docs/optionale-module.md. */

int bsp_eth_init(const uint8_t mac[6]);
int bsp_eth_link_up(void);                                       /* 1 = Link vorhanden */
int bsp_eth_send_frame(const void *data, uint32_t len);
int bsp_eth_recv_frame(void *buf, uint32_t max_len, uint32_t timeout_ms);   /* Laenge oder <0 */

#endif
