#ifndef BSP_SDRAM_H
#define BSP_SDRAM_H

#include <stdint.h>

/* Externes SDRAM: 128-Mbit-Chip, am F746 mit 16-Bit-Bus -> 8 MB nutzbar (FMC Bank 1) */
#define BSP_SDRAM_BASE 0xC0000000UL
#define BSP_SDRAM_SIZE (8UL * 1024UL * 1024UL)

/* Initialisiert FMC + SDRAM-Chip. Rueckgabe 0 = ok. */
int bsp_sdram_init(void);

/* Schreib-/Lese-Test ueber das gesamte SDRAM (zerstoert den Inhalt!).
 * Rueckgabe 0 = ok, sonst Anzahl Fehler; *first_fail_addr = erste fehlerhafte Adresse. */
uint32_t bsp_sdram_selftest(uint32_t *first_fail_addr);

#endif
