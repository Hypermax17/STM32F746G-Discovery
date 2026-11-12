#ifndef BSP_DHT_H
#define BSP_DHT_H

#include "bsp_dht_core.h"

/* Projekt Terrarium: Temperatur-/Feuchtesensor DHT11 (optional DHT22) an einem Pin des Arduino-Anschlusses.
 *
 *   Datenleitung:  Arduino-Anschluss D4  =  PG7   (3,3 V und GND vom selben Anschluss)
 *
 * Die Messwerte kommen fertig in Grad Celsius und Prozent relativer Luftfeuchte. Der Treiber ist Thread-sicher
 * und haelt den Mindestabstand von 1,1 s zwischen zwei Messungen selbst ein (kann daher bis zu ~1 s blockieren).
 * Waehrend des ca. 4 ms langen Telegramms sind Interrupts mit FreeRTOS-Prioritaet (>= 5) gesperrt.
 *
 * Sensortyp: -DBSP_DHT_TYPE=22 fuer DHT22/AM2302, Standard ist DHT11. */

#ifndef BSP_DHT_TYPE
#define BSP_DHT_TYPE 11
#endif

#if BSP_DHT_TYPE == 22
#define BSP_DHT_DECIMALS   1          /* Aufloesung 0,1 */
#define BSP_DHT_TEMP_MIN   (-20)      /* sinnvolle Skala fuer Anzeigen */
#define BSP_DHT_TEMP_MAX   60
#define BSP_DHT_NAME       "DHT22"
#else
#define BSP_DHT_DECIMALS   0          /* Aufloesung 1 (Grad Celsius / % rF) */
#define BSP_DHT_TEMP_MIN   0          /* Messbereich 0..50 Grad Celsius */
#define BSP_DHT_TEMP_MAX   50
#define BSP_DHT_NAME       "DHT11"
#endif

/* Rueckgabe 0 = Pin eingerichtet (sagt nichts darueber, ob ein Sensor angeschlossen ist) */
int bsp_dht_init(void);

/* Misst Temperatur und Luftfeuchte. Rueckgabe DHT_OK (0) oder ein DHT_ERR_*-Wert; *temp_c und *hum_pct
 * nur bei Erfolg gesetzt. */
int bsp_dht_read(float *temp_c, float *hum_pct);

/* Fehlertext zu einem Rueckgabewert */
const char *bsp_dht_error_text(int rc);

#endif
