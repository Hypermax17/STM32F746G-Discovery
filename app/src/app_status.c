#include "app_status.h"
#include "bsp.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"

static SemaphoreHandle_t s_mutex;
static char s_system[24] = "";
static char s_touch[16]  = "T -";

static void redraw(void)
{
    char line[BSP_CONSOLE_COLS + 1U];
    snprintf(line, sizeof line, "%-24.24s%s", s_system, s_touch);
    bsp_console_status(line);
}

void app_status_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
}

void app_status_set_system(uint32_t uptime_s, uint32_t heap_free_kb)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    snprintf(s_system, sizeof s_system, "up %lus heap %luK", (unsigned long)uptime_s,
             (unsigned long)heap_free_kb);
    redraw();
    xSemaphoreGive(s_mutex);
}

void app_status_set_touch(int active, unsigned x, unsigned y)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (active) {
        snprintf(s_touch, sizeof s_touch, "T %u,%u", x, y);
    } else {
        snprintf(s_touch, sizeof s_touch, "T -");
    }
    redraw();
    xSemaphoreGive(s_mutex);
}
