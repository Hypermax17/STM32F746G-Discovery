/* Host-Test: WS2812-Kern (projects/terrarium/bsp/include/bsp_ws2812_core.h).
 * Simuliert Timer + DMA wie im Treiber (erste Periode low, danach je Update-Ereignis der naechste CCR-Wert,
 * der letzte Wert wird nie ausgegeben) und decodiert die Impulsfolge wie eine WS2812 (Toleranzen laut Datenblatt). */
#include "bsp_ws2812_core.h"

#include <stdio.h>
#include <string.h>

static int g_fail;
#define CHECK(c) do { if (!(c)) { printf("FEHLER Zeile %d: %s\n", __LINE__, #c); g_fail++; } } while (0)

#define N 7U

/* Ausgegebene Hochzeiten je Zeitfenster */
static uint32_t waveform(const uint16_t *buf, uint32_t len, uint16_t *out)
{
    uint32_t k = 0;
    out[k++] = 0;                              /* Periode 0: Schattenregister = 0 */
    for (uint32_t i = 0; i + 1U < len; i++) out[k++] = buf[i];
    return k;
}

/* Empfaenger: zaehlt Bits (high-Zeit -> 0/1), erkennt Reset (>= 40 Fenster low = 50 us), prueft Toleranzen */
static int receive(const uint16_t *wf, uint32_t len, const ws2812_timing_t *t, uint32_t tim_hz,
                   uint8_t *bytes, uint32_t *nbits, uint32_t *low_tail)
{
    const double tick_ns = 1e9 / (double)tim_hz;
    *nbits = 0;
    uint32_t last_data = 0;
    memset(bytes, 0, 3U * N);
    for (uint32_t i = 0; i < len; i++) {
        if (wf[i] == 0) continue;
        const double high = wf[i] * tick_ns;
        int bit;
        if (high >= 250 && high <= 550) bit = 0;           /* T0H 0,4 us +-0,15 */
        else if (high >= 650 && high <= 950) bit = 1;      /* T1H 0,8 us +-0,15 */
        else return -1;
        if (t->period * tick_ns < 1150 || t->period * tick_ns > 1350) return -2;   /* Bitzeit 1,25 us (+-0,1) */
        if (*nbits < 24U * N * 1U) {
            if (bit) bytes[*nbits / 8U] |= (uint8_t)(0x80U >> (*nbits % 8U));
        }
        (*nbits)++;
        last_data = i;
    }
    *low_tail = len - 1U - last_data;
    return 0;
}

static void run_case(uint32_t clk, const uint8_t rgb[N][3], uint8_t limit)
{
    ws2812_timing_t t = ws2812_timing(clk);
    uint16_t buf[WS2812_SLOTS(N) + 16U], wf[WS2812_SLOTS(N) + 16U];
    uint32_t n = ws2812_encode(rgb, N, limit, &t, buf);
    CHECK(n == WS2812_SLOTS(N));
    uint32_t len = waveform(buf, n, wf);
    uint8_t bytes[3U * N];
    uint32_t nbits, tail;
    int rc = receive(wf, len, &t, clk, bytes, &nbits, &tail);
    CHECK(rc == 0);
    CHECK(nbits == 24U * N);
    CHECK(tail >= 40U);                         /* >= 50 us low am Ende */
    for (unsigned i = 0; i < N; i++) {
        CHECK(bytes[3U * i + 0U] == ws2812_scale(rgb[i][1], limit));   /* G */
        CHECK(bytes[3U * i + 1U] == ws2812_scale(rgb[i][0], limit));   /* R */
        CHECK(bytes[3U * i + 2U] == ws2812_scale(rgb[i][2], limit));   /* B */
    }
}

int main(void)
{
    /* Zeiten bei 108 MHz (TIM3-Takt der Firmware) */
    ws2812_timing_t t = ws2812_timing(108000000UL);
    CHECK(t.period == 135 && t.t0 == 43 && t.t1 == 86);
    t = ws2812_timing(100000000UL);
    CHECK(t.period == 125 && t.t0 == 40 && t.t1 == 80);

    /* Skalierung */
    CHECK(ws2812_scale(255, 255) == 255);
    CHECK(ws2812_scale(0, 128) == 0);
    CHECK(ws2812_scale(255, 128) == 128);
    CHECK(ws2812_scale(1, 128) == 1);
    CHECK(ws2812_scale(100, 128) == 50);

    /* Bitfolge einer einzelnen LED: R=0x80 G=0x01 B=0xA5 -> gesendet G R B */
    {
        const uint8_t rgb[1][3] = {{0x80, 0x01, 0xA5}};
        uint16_t buf[WS2812_SLOTS(1)];
        ws2812_timing_t tt = ws2812_timing(108000000UL);
        uint32_t n = ws2812_encode(rgb, 1, 255, &tt, buf);
        CHECK(n == 24U + WS2812_RESET_SLOTS);
        const char *expect = "00000001" "10000000" "10100101";
        for (unsigned i = 0; i < 24; i++) CHECK(buf[i] == (expect[i] == '1' ? tt.t1 : tt.t0));
        for (unsigned i = 24; i < n; i++) CHECK(buf[i] == 0);
    }

    /* Verschiedene Farben, Grenzen und Taktfrequenzen */
    uint8_t rgb[N][3];
    const uint32_t clocks[] = { 108000000UL, 100000000UL, 90000000UL };
    for (unsigned c = 0; c < sizeof clocks / sizeof clocks[0]; c++) {
        for (unsigned i = 0; i < N; i++) { rgb[i][0] = 255; rgb[i][1] = 255; rgb[i][2] = 255; }
        run_case(clocks[c], rgb, 255);
        run_case(clocks[c], rgb, 128);
        for (unsigned i = 0; i < N; i++) { rgb[i][0] = (uint8_t)(i * 37); rgb[i][1] = (uint8_t)(255 - i * 41); rgb[i][2] = (uint8_t)(i * 3 + 1); }
        run_case(clocks[c], rgb, 128);
        memset(rgb, 0, sizeof rgb);
        run_case(clocks[c], rgb, 128);
    }

    printf(g_fail ? "ws2812_test: %d FEHLER\n" : "ws2812_test: alle Tests ok\n", g_fail);
    return g_fail ? 1 : 0;
}
