#ifndef BSP_DHT_CORE_H
#define BSP_DHT_CORE_H

#include <stddef.h>
#include <stdint.h>

/* Kern des DHT11/DHT22-Treibers: Erfassen der 40 Bits und Umrechnen in physikalische Werte.
 * Ohne Hardware-Zugriff (nur ueber dht_io_t), daher am PC mit einem simulierten Sensor testbar
 * (tests/host/dht_test.c). Protokoll (Einzeldraht): Der Mikrocontroller zieht die Leitung >= 18 ms nach Masse
 * und laesst los (Pull-up). Der Sensor antwortet mit 80 us Low + 80 us High, danach folgen 40 Bits:
 * je 50 us Low, dann High-Zeit ~26 us = 0 oder ~70 us = 1. */

#define DHT_OK                  0
#define DHT_ERR_NO_RESPONSE   (-1)    /* Sensor zieht die Leitung nach dem Startsignal nicht auf Low */
#define DHT_ERR_TIMEOUT       (-2)    /* Signal bricht mitten im Telegramm ab */
#define DHT_ERR_CHECKSUM      (-3)
#define DHT_ERR_RANGE         (-4)    /* Werte ausserhalb des Messbereichs (Stoerung) */
#define DHT_HELD                1     /* kein Fehler, aber die Messung ist ausgefallen: Rueckgabe ist der letzte gueltige Wert */

#define DHT_BIT_THRESHOLD_US  42U     /* High-Zeit darueber = 1, darunter = 0. Leicht unter der Mitte (26..28 / 70 us), weil ein
                                         langsamer Anstieg der Leitung die gemessene High-Zeit verkuerzt */

typedef struct {
    int      (*pin)(void *ctx);       /* aktueller Pegel der Datenleitung (0/1) */
    uint32_t (*cycles)(void);         /* frei laufender Zyklenzaehler */
    uint32_t cycles_per_us;           /* Zaehlerschritte pro Mikrosekunde */
    void     *ctx;                    /* welcher Sensor / welcher Pin (wird an pin() uebergeben) */
    uint32_t glitch_us;               /* Entprellung: ein Pegel gilt erst, wenn er so lange stabil bleibt (0 = aus) */
} dht_io_t;

/* Wartet, bis die Leitung 'level' hat. 0 = ok (elapsed_us = Wartezeit), -1 = Zeitueberschreitung.
 * Kurze Stoerspitzen (Einkopplung auf der offenen, hochohmigen Leitung) werden ausgeblendet: der Pegel muss
 * glitch_us lang ohne Unterbrechung anliegen. Die Zeit wird bis zur ersten Flanke gemessen, die Entprellung
 * verschiebt daher Anfang und Ende eines Impulses gleich und verfaelscht keine Impulsdauer. Die echten Impulse
 * des Sensors sind mindestens ca. 26 us lang. */
static inline int dht_wait(const dht_io_t *io, int level, uint32_t timeout_us, uint32_t *elapsed_us)
{
    const uint32_t t0 = io->cycles();
    const uint32_t limit = timeout_us * io->cycles_per_us;
    const uint32_t confirm = io->glitch_us * io->cycles_per_us;
    for (;;) {
        const int p = io->pin(io->ctx);
        const uint32_t now = io->cycles();
        const uint32_t dt = now - t0;
        if (p == level) {
            int stable = 1;
            while (stable && (io->cycles() - now) < confirm) {
                stable = (io->pin(io->ctx) == level);
            }
            if (stable) {
                if (elapsed_us != NULL) {
                    *elapsed_us = dt / io->cycles_per_us;
                }
                return 0;
            }
            continue;                         /* Stoerspitze: weiter warten (Zeitlimit bleibt bestehen) */
        }
        if (dt > limit) {
            return -1;
        }
    }
}

/* Erfasst die Antwort des Sensors; aufzurufen unmittelbar nach dem Loslassen der Leitung (Startsignal beendet).
 * Rueckgabe DHT_OK oder DHT_ERR_NO_RESPONSE / DHT_ERR_TIMEOUT; bytes = 5 Datenbytes (bei Fehlern soweit erfasst),
 * *bits_done (optional) = Anzahl vollstaendig erfasster Bits (0..40) fuer die Fehlerdiagnose. */
static inline int dht_capture(const dht_io_t *io, uint8_t bytes[5], unsigned *bits_done)
{
    if (bits_done != NULL) {
        *bits_done = 0;
    }
    uint32_t high_us = 0;
    /* Nach dem Loslassen liegt die Leitung noch auf Low (eigenes Startsignal) und steigt ueber den Pull-up erst
     * nach der Anstiegszeit an. Zuerst auf dieses High warten - sonst wird das eigene Low als Antwort des Sensors
     * missverstanden und alle folgenden Pulse sind um eine Flanke verschoben. */
    if (dht_wait(io, 1, 200, NULL) != 0) return DHT_ERR_TIMEOUT;        /* bleibt Low: Kurzschluss / kein Pull-up */
    if (dht_wait(io, 0, 100, NULL) != 0) return DHT_ERR_NO_RESPONSE;    /* Sensor antwortet nach 20..40 us */
    if (dht_wait(io, 1, 120, NULL) != 0) return DHT_ERR_TIMEOUT;        /* Ende der 80 us Low-Phase */
    if (dht_wait(io, 0, 120, NULL) != 0) return DHT_ERR_TIMEOUT;        /* Ende der 80 us High-Phase */

    for (unsigned i = 0; i < 5; i++) {
        bytes[i] = 0;
    }
    for (unsigned i = 0; i < 40; i++) {
        if (dht_wait(io, 1, 100, NULL) != 0) return DHT_ERR_TIMEOUT;    /* 50 us Low vor jedem Bit */
        if (dht_wait(io, 0, 120, &high_us) != 0) return DHT_ERR_TIMEOUT;
        bytes[i / 8U] = (uint8_t)(bytes[i / 8U] << 1);
        if (high_us > DHT_BIT_THRESHOLD_US) {
            bytes[i / 8U] |= 1U;
        }
        if (bits_done != NULL) {
            *bits_done = i + 1U;
        }
    }
    return DHT_OK;
}

/* Rechnet die 5 Bytes in Grad Celsius und % relative Feuchte um. type = 11 (DHT11) oder 22 (DHT22/AM2302). */
static inline int dht_decode(const uint8_t b[5], int type, float *temp_c, float *hum_pct)
{
    if ((uint8_t)(b[0] + b[1] + b[2] + b[3]) != b[4]) {
        return DHT_ERR_CHECKSUM;
    }
    float h, t;
    if (type == 22) {
        h = (float)(((unsigned)b[0] << 8) | b[1]) * 0.1f;
        t = (float)((((unsigned)b[2] & 0x7FU) << 8) | b[3]) * 0.1f;
        if (b[2] & 0x80U) t = -t;
    } else {
        h = (float)b[0] + (float)b[1] * 0.1f;
        t = (float)b[2] + (float)(b[3] & 0x7FU) * 0.1f;
        if (b[3] & 0x80U) t = -t;
    }
    if (h < 0.0f || h > 100.0f || t < -40.0f || t > 85.0f) {
        return DHT_ERR_RANGE;
    }
    *temp_c = t;
    *hum_pct = h;
    return DHT_OK;
}

/* ---- Letzten Wert halten ------------------------------------------------------------------------------
 * Faellt eine Messung aus, liefert der Treiber fuer hold Messperioden den letzten gueltigen Wert weiter (Rueckgabe
 * DHT_HELD), danach den Fehlercode. Eine gueltige Messung beendet das Halten sofort. */
typedef struct {
    float    temp_c, hum_pct;     /* letzter gueltiger Wert */
    int      valid;               /* 1 = es gibt einen gueltigen Wert, der noch gehalten werden darf */
    unsigned fails;               /* aufeinanderfolgende ausgefallene Messperioden */
} dht_hold_t;

/* rc = Ergebnis der Messung; *t/*h enthalten bei DHT_OK den neuen Wert. Liefert DHT_OK, DHT_HELD (dann sind *t/*h
 * der gehaltene Wert) oder rc (Fehler). */
static inline int dht_hold_update(dht_hold_t *s, int rc, float *t, float *h, unsigned hold)
{
    if (rc == DHT_OK) {
        s->temp_c = *t;
        s->hum_pct = *h;
        s->valid = 1;
        s->fails = 0;
        return DHT_OK;
    }
    s->fails++;
    if (s->valid && s->fails <= hold) {
        *t = s->temp_c;
        *h = s->hum_pct;
        return DHT_HELD;
    }
    s->valid = 0;                 /* zu lange ausgefallen: alten Wert nicht mehr ausgeben */
    return rc;
}

#endif
