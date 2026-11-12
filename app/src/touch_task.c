#include "touch_task.h"
#include "ui_port.h"
#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

#define TOUCH_STACK_WORDS 512
#define TOUCH_PRIORITY    2
#define TOUCH_PERIOD_MS   20

static void touch_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    unsigned errors = 0;

    for (;;) {
        bsp_touch_state_t st;
        if (bsp_touch_read(&st) == 0) {
            if (errors != 0U) {
                BSP_LOGI("Touch: I2C wieder ok");
                errors = 0;
            }
            ui_port_touch_update(&st);
        } else if (errors++ == 0U) {
            BSP_LOGE("Touch: I2C-Fehler");
        }
        vTaskDelayUntil(&last, pdMS_TO_TICKS(TOUCH_PERIOD_MS));
    }
}

void touch_task_start(void)
{
    if (!bsp_touch_available()) {
        BSP_LOGW("Touch-Task nicht gestartet");
        return;
    }
    if (xTaskCreate(touch_task, "touch", TOUCH_STACK_WORDS, NULL, TOUCH_PRIORITY, NULL) != pdPASS) {
        bsp_panic("Task 'touch' nicht erzeugt");
    }
}
