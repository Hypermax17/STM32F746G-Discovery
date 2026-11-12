#include "data_hub.h"
#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#define SENSOR_STACK_WORDS 512
#define SENSOR_PRIORITY    1
#define SAMPLE_PERIOD_MS   1000U

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

static void sensor_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        data_sample_t s;
        if (bsp_cpu_temperature(&s.temp_c) == 0 && bsp_supply_voltage(&s.vdda_v) == 0) {
            s.t_s = xTaskGetTickCount() / configTICK_RATE_HZ;
            taskENTER_CRITICAL();
            s_ring[s_count % DATA_HUB_CAPACITY] = s;
            s_count++;
            taskEXIT_CRITICAL();
        }
        vTaskDelayUntil(&last, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}

void data_hub_start(void)
{
    if (!bsp_cpu_sensor_available()) {
        BSP_LOGW("Messwert-Archiv: kein CPU-Sensor");
        return;
    }
    if (xTaskCreate(sensor_task, "sensor", SENSOR_STACK_WORDS, NULL, SENSOR_PRIORITY, NULL) != pdPASS) {
        bsp_panic("Task 'sensor' nicht erzeugt");
    }
}
