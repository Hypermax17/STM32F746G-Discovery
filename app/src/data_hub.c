#include "data_hub.h"
#include "bsp.h"
#include "bsp_dht.h"

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

/* Statusmeldungen nur bei Aenderung des DHT-Zustands (kein Dauerfeuer im Log) */
static void log_dht_state(int rc, float t, float h)
{
    static int last = 1;                         /* 1 = noch nicht bekannt (DHT_OK ist 0, Fehler sind negativ) */
    if (rc == last) {
        return;
    }
    last = rc;
    if (rc == DHT_OK) {
        char ts[16], hs[16];
        bsp_format_float(ts, sizeof ts, t, BSP_DHT_DECIMALS);
        bsp_format_float(hs, sizeof hs, h, BSP_DHT_DECIMALS);
        BSP_LOGI("%s: %s C, %s %%", BSP_DHT_NAME, ts, hs);
    } else {
        BSP_LOGW("%s: %s", BSP_DHT_NAME, bsp_dht_error_text(rc));
    }
}

static void sensor_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        data_sample_t s = {0};
        s.mcu_ok = (uint8_t)(bsp_cpu_sensor_available() && bsp_cpu_temperature(&s.mcu_c) == 0);

        const int rc = bsp_dht_read(&s.temp_c, &s.hum_pct);    /* blockiert bis zu ~1,1 s (Mindestabstand) */
        s.dht_ok = (uint8_t)(rc == DHT_OK);
        log_dht_state(rc, s.temp_c, s.hum_pct);

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
        BSP_LOGI("%s an PG7 (Arduino D4)", BSP_DHT_NAME);
    }
    if (xTaskCreate(sensor_task, "sensor", SENSOR_STACK_WORDS, NULL, SENSOR_PRIORITY, NULL) != pdPASS) {
        bsp_panic("Task 'sensor' nicht erzeugt");
    }
}
