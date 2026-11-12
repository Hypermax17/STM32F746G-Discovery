#ifndef BSP_DHT_H
#define BSP_DHT_H

#include "bsp_dht_core.h"

/* Projekt Terrarium: zwei Temperatur-/Feuchtesensoren DHT11 (optional DHT22) am Arduino-Anschluss.
 * Der DHT11 hat keine Adresse, daher hat jeder Sensor einen eigenen Datenpin:
 *
 *   Sensor 1:  Datenleitung an Arduino D4 = PG7
 *   Sensor 2:  Datenleitung an Arduino D2 = PG6
 *   (3,3 V und GND vom selben Anschluss, beide Sensoren koennen sich Versorgung und Masse teilen)
 *
 * Die Messwerte kommen fertig in Grad Celsius und Prozent relativer Luftfeuchte. Der Treiber ist Thread-sicher
 * (die Sensoren werden nacheinander gelesen) und haelt den Mindestabstand von 1,1 s zwischen zwei Messungen
 * desselben Sensors selbst ein (kann daher bis zu ~1 s blockieren). Waehrend des ca. 4 ms langen Telegramms sind
 * Interrupts mit FreeRTOS-Prioritaet (>= 5) gesperrt.
 *
 * Sensortyp: -DBSP_DHT_TYPE=22 fuer DHT22/AM2302 (beide Sensoren gleichen Typs), Standard ist DHT11. */

#define BSP_DHT_COUNT 2U

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

/* Richtet die Datenpins aller Sensoren ein. Rueckgabe 0 = ok (sagt nichts darueber, ob Sensoren angeschlossen sind) */
int bsp_dht_init(void);

/* Bezeichnung des Anschlusses von Sensor idx (0 oder 1), z. B. "D4" */
const char *bsp_dht_pin_name(unsigned idx);

/* Misst Temperatur und Luftfeuchte von Sensor idx. Rueckgabe DHT_OK (0) oder ein DHT_ERR_*-Wert;
 * *temp_c und *hum_pct nur bei Erfolg gesetzt. */
int bsp_dht_read(unsigned idx, float *temp_c, float *hum_pct);

/* Fehlertext zu einem Rueckgabewert */
const char *bsp_dht_error_text(int rc);

#endif
