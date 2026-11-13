#include "data_hub.h"
#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#define SENSOR_STACK_WORDS 768
#define SENSOR_PRIORITY    1

static data_sample_t s_ring[DATA_HUB_CAPACITY];
static volatile uint32_t s_count;

uint32_t data_hub_count(void)
{
    return s_count;
}

int data_hub_get(uint32_t serial, data_sample_t *out)
{
    int rc = -1;
    taskENTER_CRITICAL();
    const uint32_t n = s_count;
    if (serial < n && n - serial <= DATA_HUB_CAPACITY) {
        *out = s_ring[serial % DATA_HUB_CAPACITY];
        rc = 0;
    }
    taskEXIT_CRITICAL();
    return rc;
}

/* Statusmeldungen nur bei Aenderung des Zustands eines Sensors (kein Dauerfeuer im Log) */
#define STATE_UNKNOWN 99

static void log_dht_state(unsigned idx, int rc, float t, float h)
{
    static int last[DATA_HUB_SENSORS] = { STATE_UNKNOWN, STATE_UNKNOWN };
    if (rc == last[idx]) {
        return;
    }
    const int before = last[idx];
    last[idx] = rc;
    bsp_dht_stats_t st;
    bsp_dht_stats(idx, &st);
    if (rc == DHT_OK) {
        char ts[16], hs[16];
        bsp_format_float(ts, sizeof ts, t, BSP_DHT_DECIMALS);
        bsp_format_float(hs, sizeof hs, h, BSP_DHT_DECIMALS);
        if (before == DHT_HELD) {
            BSP_LOGI("%s %u (%s): wieder gueltig, %s C, %s %%", BSP_DHT_NAME, idx + 1U, bsp_dht_pin_name(idx), ts, hs);
        } else {
            BSP_LOGI("%s %u (%s): %s C, %s %%", BSP_DHT_NAME, idx + 1U, bsp_dht_pin_name(idx), ts, hs);
        }
    } else if (rc == DHT_HELD) {
        BSP_LOGW("%s %u (%s): Messung ausgefallen (%s), letzter Wert wird gehalten", BSP_DHT_NAME, idx + 1U,
                 bsp_dht_pin_name(idx), bsp_dht_error_text(st.last_error));
    } else {
        uint8_t raw[5];
        unsigned bits;
        bsp_dht_last_diag(idx, raw, &bits);
        if (rc == DHT_ERR_NO_RESPONSE) {
            BSP_LOGW("%s %u (%s): %s", BSP_DHT_NAME, idx + 1U, bsp_dht_pin_name(idx), bsp_dht_error_text(rc));
        } else {   /* mit Rohdaten: erfasste Bits und Bytes */
            BSP_LOGW("%s %u (%s): %s [%u Bit: %02X %02X %02X %02X %02X]", BSP_DHT_NAME, idx + 1U,
                     bsp_dht_pin_name(idx), bsp_dht_error_text(rc), bits, raw[0], raw[1], raw[2], raw[3], raw[4]);
        }
    }
}

/* Alle STATS_PERIODS Messperioden: Zusammenfassung, aber nur wenn etwas auffaellig war (Fehlversuche, auch wenn die
 * Wiederholung sie verdeckt hat). Hilft, die Leitungsqualitaet zu beurteilen, z. B. mit und ohne Beleuchtung. */
#define STATS_PERIODS 30U

static void log_dht_stats(void)
{
    static bsp_dht_stats_t prev[DATA_HUB_SENSORS];
    for (unsigned i = 0; i < DATA_HUB_SENSORS; i++) {
        bsp_dht_stats_t now;
        bsp_dht_stats(i, &now);
        const uint32_t reads = now.reads - prev[i].reads;
        const uint32_t failed = now.failed_attempts - prev[i].failed_attempts;
        const uint32_t held = now.held - prev[i].held;
        const uint32_t lost = now.lost - prev[i].lost;
        if (failed != 0U || held != 0U || lost != 0U) {
            BSP_LOGI("%s %u (%s): %lu Messungen, %lu Fehlversuche, %lu gehalten, %lu ausgefallen", BSP_DHT_NAME, i + 1U,
                     bsp_dht_pin_name(i), (unsigned long)reads, (unsigned long)failed, (unsigned long)held,
                     (unsigned long)lost);
        }
        prev[i] = now;
    }
}

static void sensor_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    unsigned period = 0;
    for (;;) {
        data_sample_t s = {0};
        for (unsigned i = 0; i < DATA_HUB_SENSORS; i++) {
            /* Wiederholen und Halten des letzten Werts erledigt der Treiber; blockiert bis zu ~1,1 s je Versuch */
            const int rc = bsp_dht_read(i, &s.temp_c[i], &s.hum_pct[i]);
            s.ok[i] = (uint8_t)(rc >= 0);                                 /* DHT_OK oder DHT_HELD */
            log_dht_state(i, rc, s.temp_c[i], s.hum_pct[i]);
        }
        s.t_s = xTaskGetTickCount() / configTICK_RATE_HZ;

        taskENTER_CRITICAL();
        s_ring[s_count % DATA_HUB_CAPACITY] = s;
        s_count++;
        taskEXIT_CRITICAL();
        if (++period >= STATS_PERIODS) {
            period = 0;
            log_dht_stats();
        }
        vTaskDelayUntil(&last, pdMS_TO_TICKS(DATA_HUB_PERIOD_S * 1000U));
    }
}

void data_hub_start(void)
{
    if (bsp_dht_init() != 0) {
        BSP_LOGE("%s: Init fehlgeschlagen", BSP_DHT_NAME);
    } else {
        BSP_LOGI("%u x %s an %s und %s", (unsigned)DATA_HUB_SENSORS, BSP_DHT_NAME, bsp_dht_pin_name(0),
                 bsp_dht_pin_name(1));
    }
    if (xTaskCreate(sensor_task, "sensor", SENSOR_STACK_WORDS, NULL, SENSOR_PRIORITY, NULL) != pdPASS) {
        bsp_panic("Task 'sensor' nicht erzeugt");
    }
}
