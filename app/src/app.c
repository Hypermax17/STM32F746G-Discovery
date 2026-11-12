#include "app.h"
#include "fault_test.h"
#include "audio_task.h"
#include "button_task.h"
#include "touch_task.h"
#include "data_hub.h"
#include "app_ui.h"
#include "ui_port.h"
#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif

#define HEARTBEAT_STACK_WORDS 256
#define HEARTBEAT_PRIORITY    1
#define BLINK_MS              500U

/* Heartbeat: LED LD1 blinkt (500 ms) */
static void heartbeat_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        bsp_led_toggle();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(BLINK_MS));
    }
}

void app_init(void)
{
    BSP_LOGI("STM32F746G-Discovery");
    BSP_LOGI("Version %s", APP_VERSION);
    BSP_LOGI("Build %s %s", __DATE__, __TIME__);
    BSP_LOGI("FreeRTOS %s", tskKERNEL_VERSION_NUMBER);
    BSP_LOGI("Heap: %lu KB", (unsigned long)(xPortGetFreeHeapSize() / 1024U));

    if (xTaskCreate(heartbeat_task, "heartbeat", HEARTBEAT_STACK_WORDS, NULL,
                    HEARTBEAT_PRIORITY, NULL) != pdPASS) {
        bsp_panic("Task 'heartbeat' nicht erzeugt");
    }
    data_hub_start();
    audio_task_start();
    button_task_start();
    touch_task_start();
    fault_test_start();
    BSP_LOGI("Tasks gestartet");

    ui_port_start(app_ui_init);      /* ab hier zeichnet LVGL; Log siehe Seite LOG */
}
