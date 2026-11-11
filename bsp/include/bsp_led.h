#ifndef BSP_LED_H
#define BSP_LED_H

/* Gruene User-LED LD1 (PI1) */

void bsp_led_init(void);
void bsp_led_set(int on);
void bsp_led_toggle(void);

/* Notanzeige, falls das Display noch nicht (oder nicht mehr) nutzbar ist:
 * blinkt 'code' mal kurz, macht eine Pause und wiederholt das endlos.
 *   2 = Takt-Konfiguration, 3 = SDRAM-Init, 4 = SDRAM-Test, 5 = LCD-Init,
 *   6 = Panic (Laufzeitfehler) */
void bsp_led_blink_forever(unsigned code) __attribute__((noreturn));

#endif
