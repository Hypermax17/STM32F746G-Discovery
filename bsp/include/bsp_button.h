#ifndef BSP_BUTTON_H
#define BSP_BUTTON_H

#include <stdint.h>

/* User-Taster B1 (PI11, aktiv high, externer Pull-Down). Flanken loesen einen
 * Interrupt aus (EXTI15_10, Prioritaet 6), werden zeitlich entprellt (20 ms)
 * und als Ereignisse in eine Queue gelegt. */

typedef enum {
    BSP_BUTTON_PRESSED = 0,
    BSP_BUTTON_RELEASED
} bsp_button_event_type_t;

typedef struct {
    bsp_button_event_type_t type;
    uint32_t tick_ms;          /* Zeitpunkt (HAL_GetTick) */
} bsp_button_event_t;

/* Rueckgabe 0 = ok */
int bsp_button_init(void);

/* Aktueller Pegel (1 = gedrueckt), direkt vom Pin */
int bsp_button_pressed(void);

/* Wartet bis zu timeout_ms auf ein Ereignis. Rueckgabe 0 = Ereignis in *ev, -1 = Timeout.
 * Nur aus Tasks aufrufbar (Scheduler muss laufen). */
int bsp_button_get_event(bsp_button_event_t *ev, uint32_t timeout_ms);

#endif
