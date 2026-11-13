#ifndef BSP_WS2812_CORE_H
#define BSP_WS2812_CORE_H

/* Hardwarefreier Kern des WS2812-Treibers: wandelt Farbwerte in die Folge von Timer-Vergleichswerten (CCR) um.
 * Kann auf dem PC getestet werden (tests/host/ws2812_test.c).
 *
 * Protokoll: 800 kHz, 24 Bit je LED in der Reihenfolge G, R, B, jeweils MSB zuerst. Ein Bit ist ein Impuls in einem
 * Zeitfenster von 1,25 us: "0" = 0,4 us high, "1" = 0,8 us high (Toleranz je +-0,15 us). Danach muss die Leitung
 * mindestens 50 us low bleiben, damit die LEDs die Daten uebernehmen ("Reset"). */

#include <stdint.h>

#define WS2812_BITS_PER_LED  24U
#define WS2812_RESET_SLOTS   64U       /* 64 x 1,25 us = 80 us low am Ende des Telegramms */

/* Anzahl Zeitfenster (= CCR-Werte) fuer n LEDs */
#define WS2812_SLOTS(n)      ((n) * WS2812_BITS_PER_LED + WS2812_RESET_SLOTS)

typedef struct {
    uint16_t period;     /* Timer-Takte je Bit (ARR + 1), 1,25 us */
    uint16_t t0;         /* Takte high fuer ein 0-Bit (0,4 us) */
    uint16_t t1;         /* Takte high fuer ein 1-Bit (0,8 us) */
} ws2812_timing_t;

/* Zeiten aus dem Timer-Takt (Hz) berechnen */
static inline ws2812_timing_t ws2812_timing(uint32_t tim_clk_hz)
{
    ws2812_timing_t t;
    t.period = (uint16_t)((tim_clk_hz + 400000UL) / 800000UL);          /* 1,25 us */
    t.t0     = (uint16_t)((tim_clk_hz + 1250000UL) / 2500000UL);        /* 0,40 us */
    t.t1     = (uint16_t)((tim_clk_hz + 625000UL) / 1250000UL);         /* 0,80 us */
    return t;
}

/* Wert v (0..255) auf die Helligkeitsgrenze limit (0..255) skalieren: limit 255 = unveraendert */
static inline uint8_t ws2812_scale(uint8_t v, uint8_t limit)
{
    return (uint8_t)(((uint32_t)v * limit + 127U) / 255U);
}

/* Schreibt WS2812_SLOTS(n) Werte nach out und liefert deren Anzahl. rgb[i] = {R, G, B} der LED i. */
static inline uint32_t ws2812_encode(const uint8_t (*rgb)[3], uint32_t n, uint8_t limit,
                                     const ws2812_timing_t *t, uint16_t *out)
{
    uint32_t k = 0;
    for (uint32_t i = 0; i < n; i++) {
        const uint8_t grb[3] = {
            ws2812_scale(rgb[i][1], limit),
            ws2812_scale(rgb[i][0], limit),
            ws2812_scale(rgb[i][2], limit),
        };
        for (unsigned c = 0; c < 3; c++) {
            for (int bit = 7; bit >= 0; bit--) {
                out[k++] = ((grb[c] >> bit) & 1U) ? t->t1 : t->t0;
            }
        }
    }
    for (uint32_t r = 0; r < WS2812_RESET_SLOTS; r++) {
        out[k++] = 0;
    }
    return k;
}

#endif
