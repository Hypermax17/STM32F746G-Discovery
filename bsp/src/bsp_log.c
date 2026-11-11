#include "bsp_log.h"
#include "bsp_console.h"
#include "bsp_lcd.h"
#include "bsp_led.h"

#include <stdio.h>
#include <string.h>
#include "stm32f7xx_hal.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define LINE_MAX_CHARS 128U   /* inkl. Zeitstempel und Zeilenende; Task-Stack >= 1 KB empfohlen */

static uint16_t level_color(bsp_log_level_t level)
{
    switch (level) {
    case BSP_LOG_DEBUG: return BSP_COLOR_GRAY;
    case BSP_LOG_WARN:  return BSP_COLOR_YELLOW;
    case BSP_LOG_ERROR: return BSP_COLOR_RED;
    case BSP_LOG_INFO:
    default:            return BSP_COLOR_WHITE;
    }
}

void bsp_log(bsp_log_level_t level, const char *fmt, ...)
{
    if (!bsp_console_ready()) {
        return;
    }

    char line[LINE_MAX_CHARS];
    uint32_t ms = HAL_GetTick();
    int n = snprintf(line, sizeof line, "%4lu.%02lu ", (unsigned long)(ms / 1000U),
                     (unsigned long)((ms % 1000U) / 10U));
    if (n < 0 || (size_t)n >= sizeof line - 2U) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(&line[n], sizeof line - (size_t)n - 1U, fmt, ap);
    va_end(ap);

    size_t len = strnlen(line, sizeof line - 2U);
    line[len] = '\n';
    line[len + 1U] = '\0';
    bsp_console_write(line, level_color(level));
}

void bsp_panic(const char *fmt, ...)
{
    __disable_irq();

    static char msg[LINE_MAX_CHARS];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    if (bsp_console_ready()) {
        bsp_console_write_unlocked("\n*** PANIC ***\n", BSP_COLOR_RED);
        bsp_console_write_unlocked(msg, BSP_COLOR_RED);
        bsp_console_write_unlocked("\n", BSP_COLOR_RED);
    }
    bsp_led_blink_forever(6);
}
