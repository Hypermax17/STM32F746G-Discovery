#include "app.h"
#include "fault_test.h"
#include "app_status.h"
#include "touch_task.h"
#include "button_task.h"
#include "audio_task.h"
#include "ui.h"
#include "bsp.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif

#define HEARTBEAT_STACK_WORDS 512
#define HEARTBEAT_PRIORITY    1
#define BLINK_MS              500U

/* Heartbeat: LED LD1 blinkt (500 ms), die Statuszeile zeigt Laufzeit und freien Heap. */
static void heartbeat_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    TickType_t last_status = last - pdMS_TO_TICKS(1000);

    for (;;) {
        bsp_led_toggle();
        TickType_t now = xTaskGetTickCount();
        if ((TickType_t)(now - last_status) >= pdMS_TO_TICKS(1000)) {
            last_status = now;
            app_status_set_system(now / configTICK_RATE_HZ,
                                  (uint32_t)(xPortGetFreeHeapSize() / 1024U));
        }
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

    app_status_init();
    if (xTaskCreate(heartbeat_task, "heartbeat", HEARTBEAT_STACK_WORDS, NULL,
                    HEARTBEAT_PRIORITY, NULL) != pdPASS) {
        bsp_panic("Task 'heartbeat' nicht erzeugt");
    }
    ui_start();
    audio_task_start();
    button_task_start();
    touch_task_start();
    fault_test_start();
    BSP_LOGI("Tasks gestartet");
}
