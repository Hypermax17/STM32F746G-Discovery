#include "button_task.h"
#include "ui.h"
#include "bsp.h"
#include "qspi_test.h"

#include "FreeRTOS.h"
#include "task.h"

#define BUTTON_STACK_WORDS 1024
#define BUTTON_PRIORITY    2
#define LONG_PRESS_MS      1000U
#define TEST_PRESS_MS      3000U      /* >= 3 s: QSPI-Schreibtest */

static void button_task(void *arg)
{
    (void)arg;
    uint32_t pressed_at = 0;

    for (;;) {
        bsp_button_event_t ev;
        if (bsp_button_get_event(&ev, UINT32_MAX) != 0) {
            continue;
        }
        if (ev.type == BSP_BUTTON_PRESSED) {
            pressed_at = ev.tick_ms;
            BSP_LOGI("Taste: gedrueckt");
        } else {
            uint32_t held = ev.tick_ms - pressed_at;
            if (held >= TEST_PRESS_MS) {
                BSP_LOGI("Taste: %lu ms -> QSPI-Test", (unsigned long)held);
                qspi_write_test();
            } else if (held >= LONG_PRESS_MS) {
                BSP_LOGI("Taste: lang (%lu ms) -> Audio-Diagnose", (unsigned long)held);
                bsp_audio_dump();
            } else {
                BSP_LOGI("Taste: kurz (%lu ms)", (unsigned long)held);
                ui_next_page();
            }
        }
    }
}

void button_task_start(void)
{
    if (xTaskCreate(button_task, "button", BUTTON_STACK_WORDS, NULL, BUTTON_PRIORITY, NULL) != pdPASS) {
        bsp_panic("Task 'button' nicht erzeugt");
    }
}
