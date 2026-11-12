#ifndef DATA_HUB_H
#define DATA_HUB_H

#include <stdint.h>

/* Messwert-Archiv: Ein Task misst jede Sekunde Chiptemperatur und Versorgungsspannung (fertig umgerechnet,
 * Grad Celsius und Volt) und legt sie in einen Ringpuffer. Oberflaechen lesen daraus (Diagramm, Tabelle);
 * der Zustand liegt hier und nicht in den Widgets, damit Seiten beliebig neu aufgebaut werden koennen. */

#define DATA_HUB_CAPACITY 128U          /* Anzahl gespeicherter Messwerte (~2 min) */

typedef struct {
    uint32_t t_s;                       /* Zeit seit dem Start in Sekunden */
    float    temp_c;                    /* Grad Celsius */
    float    vdda_v;                    /* Volt */
} data_sample_t;

void data_hub_start(void);

/* Anzahl der bisher aufgenommenen Messwerte (fortlaufende Nummer des naechsten Messwerts) */
uint32_t data_hub_count(void);

/* Messwert mit der fortlaufenden Nummer 'serial' (0 = erster); 0 = ok, -1 = nicht (mehr) vorhanden */
int data_hub_get(uint32_t serial, data_sample_t *out);

#endif
