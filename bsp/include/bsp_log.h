#ifndef BSP_LOG_H
#define BSP_LOG_H

#include <stdarg.h>

/* Logging auf das Display. Format wie printf (newlib-nano: kein %f). */

typedef enum {
    BSP_LOG_DEBUG = 0,
    BSP_LOG_INFO,
    BSP_LOG_WARN,
    BSP_LOG_ERROR
} bsp_log_level_t;

void bsp_log(bsp_log_level_t level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#define BSP_LOGD(...) bsp_log(BSP_LOG_DEBUG, __VA_ARGS__)
#define BSP_LOGI(...) bsp_log(BSP_LOG_INFO,  __VA_ARGS__)
#define BSP_LOGW(...) bsp_log(BSP_LOG_WARN,  __VA_ARGS__)
#define BSP_LOGE(...) bsp_log(BSP_LOG_ERROR, __VA_ARGS__)

/* Fataler Fehler: Meldung rot auf das Display, danach blinkt die LED LD1 endlos.
 * Kehrt nie zurueck, auch aus Interrupts/Fehlerbehandlung aufrufbar. */
void bsp_panic(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));

#endif
