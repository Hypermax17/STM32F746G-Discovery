/* FreeRTOS-Hooks: das BSP benutzt Mutexe/Queues, der Bootloader startet aber keinen Scheduler. */
#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

void vApplicationMallocFailedHook(void)
{
    bsp_panic("Bootloader: Heap erschoepft");
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    bsp_panic("Bootloader: Stack-Ueberlauf '%s'", name);
}

void app_assert_failed(const char *file, int line)
{
    bsp_panic("ASSERT %s:%d", file, line);
}
