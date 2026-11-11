/* FreeRTOS-Hooks und Assert: Meldungen erscheinen auf dem Display. */
#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

void vApplicationMallocFailedHook(void)
{
    bsp_panic("FreeRTOS: Heap erschoepft");
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    bsp_panic("Stack-Ueberlauf in Task '%s'", name);
}

void app_assert_failed(const char *file, int line)
{
    bsp_panic("ASSERT %s:%d", file, line);
}
