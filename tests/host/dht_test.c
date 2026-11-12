/* PC-Test des DHT-Treiberkerns: simulierter Sensor (Einzeldraht-Protokoll mit virtueller Zeit) treibt dht_capture()
 * und dht_decode() ueber die gleichen Zeitgrenzen wie auf dem Mikrocontroller. */
#include "bsp_dht_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CYC_PER_US 216U

/* ---- Simulierter Sensor -------------------------------------------------------------------------- */

typedef struct { double dur_us; int level; } seg_t;
static seg_t g_seg[200];
static int g_nseg;
static double g_now_us;                 /* virtuelle Zeit seit dem Loslassen der Leitung */

static void seg_reset(void) { g_nseg = 0; g_now_us = 0; }
static void seg_add(double dur, int level) { g_seg[g_nseg].dur_us = dur; g_seg[g_nseg].level = level; g_nseg++; }

static int level_at(double t)
{
    double acc = 0;
    for (int i = 0; i < g_nseg; i++) {
        acc += g_seg[i].dur_us;
        if (t < acc) return g_seg[i].level;
    }
    return 1;                           /* danach: Pull-up */
}

/* Jeder Zugriff kostet virtuelle Zeit (entspricht einer Abfrageschleife von ca. 0,1 us) */
static int      sim_pin(void)    { g_now_us += 0.08; return level_at(g_now_us); }
static uint32_t sim_cycles(void) { g_now_us += 0.02; return (uint32_t)(g_now_us * CYC_PER_US); }

static double jitter(double j) { return ((double)rand() / RAND_MAX * 2.0 - 1.0) * j; }

/* Sensor antwortet nach resp_us mit dem Telegramm; stop_bits < 40: Telegramm bricht nach so vielen Bits ab */
static void build_frame(const uint8_t b[5], double resp_us, double j, int stop_bits)
{
    seg_reset();
    seg_add(resp_us, 1);                                /* Pull-up, Sensor noch still */
    seg_add(80 + jitter(j), 0);
    seg_add(80 + jitter(j), 1);
    for (int i = 0; i < 40; i++) {
        if (i == stop_bits) { seg_add(1e9, 1); return; }
        const int bit = (b[i / 8] >> (7 - (i % 8))) & 1;
        seg_add(50 + jitter(j), 0);
        seg_add((bit ? 70 : 27) + jitter(j), 1);
    }
    seg_add(50, 0);
}

static const dht_io_t IO = { sim_pin, sim_cycles, CYC_PER_US };

static int failures;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FEHLER %s\n", msg); failures++; } else printf("ok     %s\n", msg); } while (0)

static void frame(uint8_t out[5], uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    out[0] = a; out[1] = b; out[2] = c; out[3] = d; out[4] = (uint8_t)(a + b + c + d);
}

int main(void)
{
    uint8_t f[5], got[5];
    float t = 0, h = 0;

    /* DHT11: 55 % rF, 24 Grad Celsius */
    frame(f, 55, 0, 24, 0);
    build_frame(f, 30, 0, 40);
    CHECK(dht_capture(&IO, got) == DHT_OK && memcmp(f, got, 5) == 0, "DHT11 Telegramm erfasst");
    CHECK(dht_decode(got, 11, &t, &h) == DHT_OK && t == 24.0f && h == 55.0f, "DHT11 decodiert: 24 C, 55 %");

    /* DHT11 mit Nachkommastelle (neuere Module): 31,4 C, 47,0 % */
    frame(f, 47, 0, 31, 4);
    build_frame(f, 25, 0, 40);
    dht_capture(&IO, got);
    CHECK(dht_decode(got, 11, &t, &h) == DHT_OK && t > 31.39f && t < 31.41f && h == 47.0f, "DHT11 mit Dezimalstelle: 31.4 C");

    /* DHT22: 65,2 % rF, 35,1 Grad Celsius */
    frame(f, 0x02, 0x8C, 0x01, 0x5F);
    build_frame(f, 35, 0, 40);
    dht_capture(&IO, got);
    CHECK(dht_decode(got, 22, &t, &h) == DHT_OK && t > 35.09f && t < 35.11f && h > 65.19f && h < 65.21f, "DHT22 decodiert: 35.1 C, 65.2 %");

    /* DHT22 negative Temperatur: -10,1 C */
    frame(f, 0x02, 0x8C, 0x80, 0x65);
    build_frame(f, 30, 0, 40);
    dht_capture(&IO, got);
    CHECK(dht_decode(got, 22, &t, &h) == DHT_OK && t > -10.11f && t < -10.09f, "DHT22 negative Temperatur: -10.1 C");

    /* Robustheit: 2000 zufaellige Telegramme mit +-6 us Zeitabweichung und verschiedenen Antwortzeiten */
    srand(1234);
    int bad = 0;
    for (int n = 0; n < 2000; n++) {
        frame(f, (uint8_t)(rand() % 100), 0, (uint8_t)(rand() % 50), 0);
        build_frame(f, 20 + rand() % 21, 6, 40);
        if (dht_capture(&IO, got) != DHT_OK || memcmp(f, got, 5) != 0) bad++;
    }
    CHECK(bad == 0, "2000 zufaellige Telegramme mit Zeitjitter +-6 us fehlerfrei");

    /* Fehlerfaelle */
    seg_reset(); seg_add(1e9, 1);
    CHECK(dht_capture(&IO, got) == DHT_ERR_NO_RESPONSE, "kein Sensor: DHT_ERR_NO_RESPONSE");
    seg_reset(); seg_add(1e9, 0);
    CHECK(dht_capture(&IO, got) == DHT_ERR_TIMEOUT, "Leitung dauerhaft Low: DHT_ERR_TIMEOUT");
    frame(f, 55, 0, 24, 0);
    build_frame(f, 30, 0, 17);
    CHECK(dht_capture(&IO, got) == DHT_ERR_TIMEOUT, "Telegramm bricht nach 17 Bits ab: DHT_ERR_TIMEOUT");
    build_frame(f, 150, 0, 40);
    CHECK(dht_capture(&IO, got) == DHT_ERR_NO_RESPONSE, "Antwort erst nach 150 us: zu spaet");

    frame(f, 55, 0, 24, 0); f[4] ^= 1;
    CHECK(dht_decode(f, 11, &t, &h) == DHT_ERR_CHECKSUM, "falsche Pruefsumme erkannt");
    frame(f, 130, 0, 24, 0);
    CHECK(dht_decode(f, 11, &t, &h) == DHT_ERR_RANGE, "Feuchte > 100 % abgelehnt");
    frame(f, 55, 0, 90, 0);
    CHECK(dht_decode(f, 11, &t, &h) == DHT_ERR_RANGE, "Temperatur > 85 C abgelehnt");

    printf(failures ? "\n%d FEHLER\n" : "\nalle DHT-Tests bestanden\n", failures);
    return failures ? 1 : 0;
}
