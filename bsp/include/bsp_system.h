#ifndef BSP_SYSTEM_H
#define BSP_SYSTEM_H

/* Interne Funktionen von bsp_init() */

/* HAL_Init, MPU (SDRAM write-through), I- und D-Cache */
void bsp_system_init_early(void);

/* HSE 25 MHz -> PLL -> 216 MHz (Over-Drive), AHB 216, APB1 54, APB2 108 MHz.
 * Rueckgabe 0 = ok. */
int bsp_system_clock_config(void);

#endif
