/* PC-Test des DHT-Treiberkerns: simulierter Sensor (Einzeldraht-Protokoll mit virtueller Zeit) treibt dht_capture()
 * und dht_decode() ueber die gleichen Zeitgrenzen wie auf dem Mikrocontroller. */
#include "bsp_dht_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CYC_PER_US 216U

/* ---- Simulierter Sensor -------------------------------------------------------------------------- */

typedef struct { double dur_us; int level; } seg_t;
typedef struct {
    seg_t   seg[200];
    int     nseg;
    double  start_us;                   /* Zeitpunkt, an dem die Leitung losgelassen wurde */
} sim_t;

static double g_rise_us = 4.0;          /* Anstiegszeit der Leitung nach dem Loslassen (Pull-up, Leitungskapazitaet) */
static double g_clock_us;               /* gemeinsame virtuelle Zeit (der Zyklenzaehler ist fuer alle Sensoren gleich) */

static void seg_reset(sim_t *s) { s->nseg = 0; s->start_us = g_clock_us; }
static void seg_add(sim_t *s, double dur, int level) { s->seg[s->nseg].dur_us = dur; s->seg[s->nseg].level = level; s->nseg++; }

static int level_at(const sim_t *s, double t)
{
    double acc = 0;
    for (int i = 0; i < s->nseg; i++) {
        acc += s->seg[i].dur_us;
        if (t < acc) return s->seg[i].level;
    }
    return 1;                           /* danach: Pull-up */
}

/* Jeder Zugriff kostet virtuelle Zeit (entspricht einer Abfrageschleife von ca. 0,1 us) */
/* Stoerspitzen (Einkopplung auf der Leitung): kurze Bereiche, in denen der Pegel kippt */
static double g_gl_at[16];                         /* Beginn relativ zum Loslassen (us) */
static int    g_gl_n;
static double g_gl_len = 0.3;                      /* Dauer (us) */
static int sim_pin(void *ctx)
{
    g_clock_us += 0.08;
    const double rel = g_clock_us - ((sim_t *)ctx)->start_us;
    int lvl = level_at(ctx, rel);
    for (int i = 0; i < g_gl_n; i++) {
        if (rel >= g_gl_at[i] && rel < g_gl_at[i] + g_gl_len) lvl = !lvl;
    }
    return lvl;
}
static uint32_t sim_cycles(void)    { g_clock_us += 0.02; return (uint32_t)(g_clock_us * CYC_PER_US); }

static double jitter(double j) { return ((double)rand() / RAND_MAX * 2.0 - 1.0) * j; }

/* Sensor antwortet nach resp_us mit dem Telegramm; stop_bits < 40: Telegramm bricht nach so vielen Bits ab */
static void build_frame(sim_t *s, const uint8_t b[5], double resp_us, double j, int stop_bits)
{
    seg_reset(s);
    seg_add(s, g_rise_us, 0);                           /* nach dem Loslassen liegt die Leitung noch Low (Anstiegszeit) */
    seg_add(s, resp_us > g_rise_us ? resp_us - g_rise_us : 0.0, 1);   /* Pull-up, Sensor noch still */
    seg_add(s, 80 + jitter(j), 0);
    seg_add(s, 80 + jitter(j), 1);
    for (int i = 0; i < 40; i++) {
        if (i == stop_bits) { seg_add(s, 1e9, 1); return; }
        const int bit = (b[i / 8] >> (7 - (i % 8))) & 1;
        seg_add(s, 50 + jitter(j), 0);
        seg_add(s, (bit ? 70 : 27) + jitter(j), 1);
    }
    seg_add(s, 50, 0);
}

static sim_t S1, S2;
static const dht_io_t IO1 = { sim_pin, sim_cycles, CYC_PER_US, &S1 };
static const dht_io_t IO2 = { sim_pin, sim_cycles, CYC_PER_US, &S2 };
#define IO IO1

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
    build_frame(&S1, f, 30, 0, 40);
    CHECK(dht_capture(&IO, got, NULL) == DHT_OK && memcmp(f, got, 5) == 0, "DHT11 Telegramm erfasst");
    CHECK(dht_decode(got, 11, &t, &h) == DHT_OK && t == 24.0f && h == 55.0f, "DHT11 decodiert: 24 C, 55 %");

    /* DHT11 mit Nachkommastelle (neuere Module): 31,4 C, 47,0 % */
    frame(f, 47, 0, 31, 4);
    build_frame(&S1, f, 25, 0, 40);
    dht_capture(&IO, got, NULL);
    CHECK(dht_decode(got, 11, &t, &h) == DHT_OK && t > 31.39f && t < 31.41f && h == 47.0f, "DHT11 mit Dezimalstelle: 31.4 C");

    /* DHT22: 65,2 % rF, 35,1 Grad Celsius */
    frame(f, 0x02, 0x8C, 0x01, 0x5F);
    build_frame(&S1, f, 35, 0, 40);
    dht_capture(&IO, got, NULL);
    CHECK(dht_decode(got, 22, &t, &h) == DHT_OK && t > 35.09f && t < 35.11f && h > 65.19f && h < 65.21f, "DHT22 decodiert: 35.1 C, 65.2 %");

    /* DHT22 negative Temperatur: -10,1 C */
    frame(f, 0x02, 0x8C, 0x80, 0x65);
    build_frame(&S1, f, 30, 0, 40);
    dht_capture(&IO, got, NULL);
    CHECK(dht_decode(got, 22, &t, &h) == DHT_OK && t > -10.11f && t < -10.09f, "DHT22 negative Temperatur: -10.1 C");

    /* Robustheit: 2000 zufaellige Telegramme mit +-6 us Zeitabweichung und verschiedenen Antwortzeiten */
    srand(1234);
    int bad = 0;
    for (int n = 0; n < 2000; n++) {
        frame(f, (uint8_t)(rand() % 100), 0, (uint8_t)(rand() % 50), 0);
        build_frame(&S1, f, 20 + rand() % 21, 6, 40);
        if (dht_capture(&IO, got, NULL) != DHT_OK || memcmp(f, got, 5) != 0) bad++;
    }
    CHECK(bad == 0, "2000 zufaellige Telegramme mit Zeitjitter +-6 us fehlerfrei");

    /* schwacher Pull-up / lange Leitung: Anstiegszeit 15 us */
    g_rise_us = 15.0;
    frame(f, 55, 0, 24, 0);
    build_frame(&S1, f, 35, 0, 40);
    CHECK(dht_capture(&IO, got, NULL) == DHT_OK && memcmp(f, got, 5) == 0, "langsamer Anstieg (15 us): Telegramm korrekt erfasst");
    g_rise_us = 4.0;

    /* Zwei Sensoren an zwei Pins: jeder Kontext liefert sein eigenes Telegramm, ein stummer Pin stoert den anderen nicht */
    uint8_t fa[5], fb[5];
    frame(fa, 55, 0, 24, 0);
    frame(fb, 40, 0, 31, 0);
    build_frame(&S1, fa, 30, 3, 40);
    build_frame(&S2, fb, 28, 3, 40);
    uint8_t ga[5], gb[5];
    int ra = dht_capture(&IO1, ga, NULL);
    build_frame(&S2, fb, 28, 3, 40);                    /* Sensor 2 wird danach (anderer Zeitpunkt) abgefragt */
    int rb = dht_capture(&IO2, gb, NULL);
    CHECK(ra == DHT_OK && rb == DHT_OK && memcmp(fa, ga, 5) == 0 && memcmp(fb, gb, 5) == 0,
          "zwei Sensoren (zwei Pins) liefern jeweils ihr eigenes Telegramm");
    seg_reset(&S1); seg_add(&S1, g_rise_us, 0); seg_add(&S1, 1e9, 1);   /* Sensor 1 stumm */
    const int r1 = dht_capture(&IO1, ga, NULL);
    build_frame(&S2, fb, 30, 3, 40);                    /* Sensor 2 wird danach abgefragt */
    const int r2 = dht_capture(&IO2, gb, NULL);
    CHECK(r1 == DHT_ERR_NO_RESPONSE && r2 == DHT_OK && memcmp(fb, gb, 5) == 0,
          "Sensor 1 stumm: Sensor 2 wird trotzdem korrekt gelesen");

    /* Fehlerfaelle */
    seg_reset(&S1); seg_add(&S1, g_rise_us, 0); seg_add(&S1, 1e9, 1);
    CHECK(dht_capture(&IO, got, NULL) == DHT_ERR_NO_RESPONSE, "kein Sensor: DHT_ERR_NO_RESPONSE");
    seg_reset(&S1); seg_add(&S1, 1e9, 0);
    CHECK(dht_capture(&IO, got, NULL) == DHT_ERR_TIMEOUT, "Leitung dauerhaft Low: DHT_ERR_TIMEOUT");
    frame(f, 55, 0, 24, 0);
    build_frame(&S1, f, 30, 0, 17);
    unsigned nb = 99;
    CHECK(dht_capture(&IO, got, &nb) == DHT_ERR_TIMEOUT && nb == 16, "Telegramm bricht im 17. Bit ab: DHT_ERR_TIMEOUT, 16 vollstaendige Bits gemeldet");
    build_frame(&S1, f, 150, 0, 40);
    CHECK(dht_capture(&IO, got, NULL) == DHT_ERR_NO_RESPONSE, "Antwort erst nach 150 us: zu spaet");

    frame(f, 55, 0, 24, 0); f[4] ^= 1;
    CHECK(dht_decode(f, 11, &t, &h) == DHT_ERR_CHECKSUM, "falsche Pruefsumme erkannt");
    frame(f, 130, 0, 24, 0);
    CHECK(dht_decode(f, 11, &t, &h) == DHT_ERR_RANGE, "Feuchte > 100 % abgelehnt");
    frame(f, 55, 0, 90, 0);
    CHECK(dht_decode(f, 11, &t, &h) == DHT_ERR_RANGE, "Temperatur > 85 C abgelehnt");

    /* Stoerspitzen: ohne Entprellung gehen Telegramme kaputt, mit 1 us Entprellung nicht */
    {
        srand(7);
        const dht_io_t io_raw  = { sim_pin, sim_cycles, CYC_PER_US, &S1, 0 };
        const dht_io_t io_filt = { sim_pin, sim_cycles, CYC_PER_US, &S1, 1 };
        int bad_raw = 0, bad_filt = 0;
        const int frames = 2000;
        for (int n = 0; n < frames; n++) {
            uint8_t fr[5], g2[5];
            frame(fr, (uint8_t)(30 + n % 60), 0, (uint8_t)(10 + n % 30), 0);
            g_gl_n = 4;
            for (int k = 0; k < g_gl_n; k++) g_gl_at[k] = 5.0 + (double)rand() / RAND_MAX * 4300.0;   /* im Telegramm */
            build_frame(&S1, fr, 30, 3, 40);
            if (dht_capture(&io_raw, g2, NULL) != DHT_OK || memcmp(fr, g2, 5) != 0) bad_raw++;
            build_frame(&S1, fr, 30, 3, 40);
            if (dht_capture(&io_filt, g2, NULL) != DHT_OK || memcmp(fr, g2, 5) != 0) bad_filt++;
        }
        g_gl_n = 0;
        printf("       Stoerspitzen (4 je Telegramm, 0,3 us): ohne Entprellung %d von %d fehlerhaft, mit %d\n",
               bad_raw, frames, bad_filt);
        CHECK(bad_raw > 100, "Simulation: Stoerspitzen stoeren ohne Entprellung deutlich");
        CHECK(bad_filt == 0, "mit Entprellung (1 us) sind alle Telegramme fehlerfrei");

        /* Spitzen duerfen die Messung auch nicht bei der Antwort/zwischen den Impulsen verfaelschen */
        g_gl_n = 1; g_gl_at[0] = 33.0;      /* mitten in der Pause vor der Antwort */
        frame(f, 55, 0, 24, 0);
        build_frame(&S1, f, 30, 0, 40);
        CHECK(dht_capture(&io_filt, got, NULL) == DHT_OK && memcmp(f, got, 5) == 0, "Spitze in der Antwortpause ausgeblendet");
        g_gl_n = 0;
    }

    /* Letzten Wert halten */
    {
        dht_hold_t hs = {0};
        float tt = 0, hh = 0;
        const unsigned HOLD = 3;
        tt = 22; hh = 51;
        CHECK(dht_hold_update(&hs, DHT_ERR_CHECKSUM, &tt, &hh, HOLD) == DHT_ERR_CHECKSUM,
              "Halten: ohne vorherigen Wert wird der Fehler gemeldet");
        CHECK(dht_hold_update(&hs, DHT_OK, &tt, &hh, HOLD) == DHT_OK && tt == 22 && hh == 51, "Halten: gueltiger Wert wird durchgereicht");
        int ok = 1;
        for (unsigned i = 0; i < HOLD; i++) {
            tt = 0; hh = 0;
            ok &= (dht_hold_update(&hs, DHT_ERR_TIMEOUT, &tt, &hh, HOLD) == DHT_HELD && tt == 22 && hh == 51);
        }
        CHECK(ok, "Halten: 3 ausgefallene Perioden liefern den letzten Wert (DHT_HELD)");
        tt = 0; hh = 0;
        CHECK(dht_hold_update(&hs, DHT_ERR_TIMEOUT, &tt, &hh, HOLD) == DHT_ERR_TIMEOUT, "Halten: danach wird der Fehler gemeldet");
        CHECK(dht_hold_update(&hs, DHT_ERR_TIMEOUT, &tt, &hh, HOLD) == DHT_ERR_TIMEOUT, "Halten: der alte Wert kommt nicht wieder");
        tt = 23; hh = 49;
        CHECK(dht_hold_update(&hs, DHT_OK, &tt, &hh, HOLD) == DHT_OK && hs.fails == 0, "Halten: neuer gueltiger Wert beendet den Ausfall");
        tt = 0; hh = 0;
        CHECK(dht_hold_update(&hs, DHT_ERR_CHECKSUM, &tt, &hh, HOLD) == DHT_HELD && tt == 23 && hh == 49,
              "Halten: nach Erholung liefert der neue Wert");
        /* Fehler dazwischen zaehlen nicht, wenn zwischendurch eine Messung gelingt */
        dht_hold_t h2 = {0};
        tt = 20; hh = 40;
        dht_hold_update(&h2, DHT_OK, &tt, &hh, HOLD);
        int never_lost = 1;
        for (int n = 0; n < 50; n++) {
            float a = 0, b = 0;
            if (n % 3 == 2) { a = 20; b = 40; never_lost &= (dht_hold_update(&h2, DHT_OK, &a, &b, HOLD) == DHT_OK); }
            else            { never_lost &= (dht_hold_update(&h2, DHT_ERR_CHECKSUM, &a, &b, HOLD) == DHT_HELD); }
        }
        CHECK(never_lost, "Halten: jede dritte Messung gueltig -> nie ein Fehler nach aussen");
    }

    printf(failures ? "\n%d FEHLER\n" : "\nalle DHT-Tests bestanden\n", failures);
    return failures ? 1 : 0;
}
