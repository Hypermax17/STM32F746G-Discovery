#include "touch_task.h"
#include "app_status.h"
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
    uint8_t prev_count = 0;
    unsigned errors = 0;

    for (;;) {
        bsp_touch_state_t st;
        if (bsp_touch_read(&st) == 0) {
            if (errors != 0U) {
                BSP_LOGI("Touch: I2C wieder ok");
                errors = 0;
            }
            if (st.count != prev_count) {
                if (st.count == 0U) {
                    BSP_LOGI("Touch: losgelassen");
                } else {
                    BSP_LOGI("Touch(%u) %u,%u raw %u,%u", (unsigned)st.count,
                             (unsigned)st.point[0].x, (unsigned)st.point[0].y,
                             (unsigned)st.point[0].raw_x, (unsigned)st.point[0].raw_y);
                }
                prev_count = st.count;
            }
            if (st.count != 0U) {
                app_status_set_touch(1, st.point[0].x, st.point[0].y);
            } else {
                app_status_set_touch(0, 0, 0);
            }
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
