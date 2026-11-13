#ifndef BSP_WS2812_H
#define BSP_WS2812_H

#include <stdint.h>

#include "bsp_ws2812_core.h"

/* Projekt Terrarium: Beleuchtung mit WS2812-LEDs (Modul CJMCU-2812-7, 7 LEDs).
 *
 *   Datenleitung (DIN) an Arduino D3 = PB4 (TIM3_CH1), Masse gemeinsam mit dem Board, LED-Versorgung 5 V.
 *   Wegen des 3,3-V-Pegels (Eingang verlangt 0,7 x VDD = 3,5 V) empfiehlt sich ein Pegelwandler (74AHCT125)
 *   oder eine Schottky-Diode in der 5-V-Leitung; ausserdem ein Widerstand von ca. 330 Ohm in der Datenleitung.
 *
 * Das Telegramm erzeugt TIM3 im PWM-Modus, die Bits liefert DMA1 Stream 2 (Kanal 5, Update-Anforderung des Timers).
 * Die CPU ist waehrend der Uebertragung (ca. 0,3 ms) nicht belastet, Interrupt-Sperren (z. B. DHT-Messung) stoeren nicht.
 *
 * Farben sind sRGB-Werte 0..255 je Kanal. Zum Schutz der USB-Stromversorgung wird die Helligkeit auf
 * BSP_WS2812_MAX_LEVEL / 255 begrenzt (255 -> volle Leistung nie; 7 LEDs in Weiss ziehen sonst bis ca. 420 mA).
 * Mehrere Tasks duerfen den Treiber benutzen. */

#define BSP_WS2812_COUNT 7U

/* Obergrenze der Kanalwerte nach der Skalierung (128 = 50 %). Mit -DBSP_WS2812_MAX_LEVEL=255 abschaltbar. */
#ifndef BSP_WS2812_MAX_LEVEL
#define BSP_WS2812_MAX_LEVEL 128U
#endif

/* Rueckgabe 0 = ok; schaltet alle LEDs aus */
int  bsp_ws2812_init(void);

/* Farbe im Speicher setzen (wird erst mit bsp_ws2812_show() sichtbar) */
void bsp_ws2812_set_pixel(unsigned index, uint8_t r, uint8_t g, uint8_t b);
void bsp_ws2812_set_all(uint8_t r, uint8_t g, uint8_t b);

/* Sendet die Farben an die LEDs und wartet auf das Ende (ca. 0,3 ms). Rueckgabe 0 = ok, -1 = Fehler/Timeout. */
int  bsp_ws2812_show(void);

/* Alle LEDs auf dieselbe Farbe setzen und senden */
int  bsp_ws2812_fill(uint8_t r, uint8_t g, uint8_t b);

#endif
