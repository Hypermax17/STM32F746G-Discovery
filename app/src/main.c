#include "bsp.h"
#include "app.h"

#include "FreeRTOS.h"
#include "task.h"

int main(void)
{
    bsp_init();        /* Display-Konsole steht danach bereit */
    app_init();        /* Tasks anlegen, Banner ausgeben */

    vTaskStartScheduler();

    bsp_panic("Scheduler beendet (zu wenig Heap?)");
}
