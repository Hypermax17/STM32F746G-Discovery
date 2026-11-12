#ifndef DATA_HUB_H
#define DATA_HUB_H

#include <stdint.h>
#include "bsp_dht.h"

/* Messwert-Archiv (Projekt Terrarium): Ein Task misst alle DATA_HUB_PERIOD_S Sekunden die beiden DHT-Sensoren
 * (Temperatur und relative Luftfeuchte, fertig in Grad Celsius bzw. Prozent) und legt sie in einen Ringpuffer.
 * Oberflaechen lesen daraus (Anzeigen, Mini-Diagramme); der Zustand liegt hier und nicht in den Widgets, damit Seiten
 * beliebig neu aufgebaut werden koennen. Die Chiptemperatur und die Versorgungsspannung werden hier nicht
 * aufgezeichnet (sie stehen live auf der Seite System). */

#define DATA_HUB_SENSORS   BSP_DHT_COUNT    /* Anzahl der DHT-Sensoren */
#define DATA_HUB_CAPACITY  128U             /* Anzahl gespeicherter Messungen */
#define DATA_HUB_PERIOD_S  2U               /* Abstand der Messungen (DHT11: mindestens 1 s) */

typedef struct {
    uint32_t t_s;                           /* Zeit seit dem Start in Sekunden */
    float    temp_c[DATA_HUB_SENSORS];      /* Temperatur in Grad Celsius (nur gueltig wenn ok[i]) */
    float    hum_pct[DATA_HUB_SENSORS];     /* relative Luftfeuchte in Prozent (nur gueltig wenn ok[i]) */
    uint8_t  ok[DATA_HUB_SENSORS];          /* 1 = Messung von Sensor i erfolgreich */
} data_sample_t;

void data_hub_start(void);

/* Anzahl der bisher aufgenommenen Messungen (fortlaufende Nummer der naechsten Messung) */
uint32_t data_hub_count(void);

/* Messung mit der fortlaufenden Nummer 'serial' (0 = erste); 0 = ok, -1 = nicht (mehr) vorhanden */
int data_hub_get(uint32_t serial, data_sample_t *out);

#endif
