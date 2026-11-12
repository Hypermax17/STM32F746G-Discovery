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
static void log_dht_state(unsigned idx, int rc, float t, float h)
{
    static int last[DATA_HUB_SENSORS] = { 1, 1 };    /* 1 = noch nicht bekannt (DHT_OK ist 0, Fehler sind negativ) */
    if (rc == last[idx]) {
        return;
    }
    last[idx] = rc;
    if (rc == DHT_OK) {
        char ts[16], hs[16];
        bsp_format_float(ts, sizeof ts, t, BSP_DHT_DECIMALS);
        bsp_format_float(hs, sizeof hs, h, BSP_DHT_DECIMALS);
        BSP_LOGI("%s %u (%s): %s C, %s %%", BSP_DHT_NAME, idx + 1U, bsp_dht_pin_name(idx), ts, hs);
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

static void sensor_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        data_sample_t s = {0};
        for (unsigned i = 0; i < DATA_HUB_SENSORS; i++) {
            int rc = bsp_dht_read(i, &s.temp_c[i], &s.hum_pct[i]);        /* blockiert bis zu ~1,1 s (Mindestabstand) */
            if (rc == DHT_ERR_TIMEOUT || rc == DHT_ERR_CHECKSUM || rc == DHT_ERR_RANGE) {
                rc = bsp_dht_read(i, &s.temp_c[i], &s.hum_pct[i]);        /* gelegentliche Bitfehler: einmal wiederholen */
            }
            s.ok[i] = (uint8_t)(rc == DHT_OK);
            log_dht_state(i, rc, s.temp_c[i], s.hum_pct[i]);
        }
        s.t_s = xTaskGetTickCount() / configTICK_RATE_HZ;

        taskENTER_CRITICAL();
        s_ring[s_count % DATA_HUB_CAPACITY] = s;
        s_count++;
        taskEXIT_CRITICAL();
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
