#include "app.h"
#include "bsp.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif

#define HEARTBEAT_STACK_WORDS 512
#define HEARTBEAT_PRIORITY    1

/* Heartbeat: LED LD1 blinkt (500 ms), die Statuszeile zeigt Laufzeit und Speicher. */
static void heartbeat_task(void *arg)
{
    (void)arg;
    char status[BSP_CONSOLE_COLS + 1U];
    TickType_t last = xTaskGetTickCount();
    unsigned beat = 0;

    for (;;) {
        bsp_led_toggle();
        if ((++beat & 1U) == 0U) {
            unsigned long s = xTaskGetTickCount() / configTICK_RATE_HZ;
            snprintf(status, sizeof status, "up %lus  heap %luK frei  tasks %u",
                     s, (unsigned long)(xPortGetFreeHeapSize() / 1024U),
                     (unsigned)uxTaskGetNumberOfTasks());
            bsp_console_status(status);
        }
        vTaskDelayUntil(&last, pdMS_TO_TICKS(500));
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
    BSP_LOGI("Tasks gestartet");
}
