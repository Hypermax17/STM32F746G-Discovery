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

/* Ausfallsicherheit: bsp_dht_read() wiederholt eine fehlgeschlagene Messung bis zu BSP_DHT_ATTEMPTS mal (mit dem
 * Mindestabstand von 1,1 s). Faellt die Messung trotzdem aus, liefert es fuer BSP_DHT_HOLD_READS weitere Aufrufe
 * den letzten gueltigen Wert (Rueckgabe DHT_HELD = 1) und erst danach einen Fehlercode (< 0). */
#ifndef BSP_DHT_ATTEMPTS
#define BSP_DHT_ATTEMPTS 3
#endif
#ifndef BSP_DHT_HOLD_READS
#define BSP_DHT_HOLD_READS 5U
#endif

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

/* Misst Temperatur und Luftfeuchte von Sensor idx. Rueckgabe:
 *   DHT_OK (0)      frischer Messwert
 *   DHT_HELD (1)    Messung ausgefallen, *temp_c/*hum_pct sind der letzte gueltige Wert (hoechstens BSP_DHT_HOLD_READS
 *                   Aufrufe in Folge); der Wert ist brauchbar, aber nicht aktuell
 *   < 0             DHT_ERR_*: kein gueltiger Wert (nie einer gemessen oder zu lange ausgefallen)
 * *temp_c und *hum_pct sind bei DHT_OK und DHT_HELD gesetzt. */
int bsp_dht_read(unsigned idx, float *temp_c, float *hum_pct);

/* Zaehler seit dem Start, zur Beurteilung der Leitungsqualitaet (auch Fehlversuche, die durch Wiederholen oder
 * Halten nicht sichtbar werden). */
typedef struct {
    uint32_t reads;              /* Aufrufe von bsp_dht_read() */
    uint32_t failed_attempts;    /* fehlgeschlagene Einzelmessungen (auch wenn die Wiederholung gelang) */
    uint32_t held;               /* Aufrufe, die den gehaltenen Wert lieferten */
    uint32_t lost;               /* Aufrufe ohne gueltigen Wert */
    int      last_error;         /* DHT_ERR_* der letzten fehlgeschlagenen Einzelmessung (0 = noch keine) */
} bsp_dht_stats_t;
void bsp_dht_stats(unsigned idx, bsp_dht_stats_t *out);

/* Diagnose des letzten Leseversuchs von Sensor idx: die 5 empfangenen Rohbytes und die Anzahl vollstaendig
 * erfasster Bits (40 = vollstaendig). Hilft bei Pruefsummen- und Abbruchfehlern. */
void bsp_dht_last_diag(unsigned idx, uint8_t raw[5], unsigned *bits_done);

/* Fehlertext zu einem Rueckgabewert */
const char *bsp_dht_error_text(int rc);

#endif
