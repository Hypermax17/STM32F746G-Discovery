#ifndef DATA_HUB_H
#define DATA_HUB_H

#include <stdint.h>

/* Messwert-Archiv (Projekt Terrarium): Ein Task misst alle DATA_HUB_PERIOD_S Sekunden die Chiptemperatur (MCU) und
 * den DHT-Sensor (Umgebungstemperatur und relative Luftfeuchte) - jeweils fertig in Grad Celsius bzw. Prozent -
 * und legt sie in einen Ringpuffer. Oberflaechen lesen daraus (Anzeigen, Diagramm, Tabelle); der Zustand liegt hier
 * und nicht in den Widgets, damit Seiten beliebig neu aufgebaut werden koennen. Die Versorgungsspannung (VDDA) wird
 * hier nicht aufgezeichnet. */

#define DATA_HUB_CAPACITY  128U         /* Anzahl gespeicherter Messwerte */
#define DATA_HUB_PERIOD_S  2U           /* Abstand der Messungen (DHT11: mindestens 1 s) */

typedef struct {
    uint32_t t_s;                       /* Zeit seit dem Start in Sekunden */
    float    mcu_c;                     /* Chiptemperatur in Grad Celsius (nur gueltig wenn mcu_ok) */
    float    temp_c;                    /* Umgebungstemperatur in Grad Celsius (nur gueltig wenn dht_ok) */
    float    hum_pct;                   /* relative Luftfeuchte in Prozent (nur gueltig wenn dht_ok) */
    uint8_t  mcu_ok;
    uint8_t  dht_ok;
} data_sample_t;

void data_hub_start(void);

/* Anzahl der bisher aufgenommenen Messwerte (fortlaufende Nummer des naechsten Messwerts) */
uint32_t data_hub_count(void);

/* Messwert mit der fortlaufenden Nummer 'serial' (0 = erster); 0 = ok, -1 = nicht (mehr) vorhanden */
int data_hub_get(uint32_t serial, data_sample_t *out);

#endif
