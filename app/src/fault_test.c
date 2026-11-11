/* Gezielte Fehler zum Testen der Fehleranzeige auf dem Display.
 * Aktivierung beim Konfigurieren:  cmake --preset release -DAPP_FAULT_TEST=<art>
 * Arten: nullptr | div0 | undef | execsdram | stackoverflow | assert | malloc
 * Ohne APP_FAULT_TEST ist dieser Code nicht im Programm enthalten. */
#include "fault_test.h"

#ifdef APP_FAULT_TEST_ENABLED

#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"

__attribute__((unused)) static int zero(void)
{
    static volatile int z;
    return z;
}

static int (*volatile recurse_ptr)(int);

__attribute__((unused)) static int recurse(int depth)
{
    volatile char pad[64];
    pad[0] = (char)depth;
    return recurse_ptr(depth + 1) + pad[0];
}

static void fault_test_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(3000));
    BSP_LOGW("Fault-Test: %s", APP_FAULT_TEST_NAME);

    #if defined(APP_FAULT_TEST_NULLPTR)
    *(volatile uint32_t *)0 = 1;
    #elif defined(APP_FAULT_TEST_DIV0)
    volatile int r = 1 / zero();
    (void)r;
    #elif defined(APP_FAULT_TEST_UNDEF)
    __asm volatile("udf #0");
    #elif defined(APP_FAULT_TEST_EXECSDRAM)
    ((void (*)(void))(0xC0080001UL))();
    #elif defined(APP_FAULT_TEST_STACKOVERFLOW)
    recurse_ptr = recurse;
    (void)recurse(0);
    #elif defined(APP_FAULT_TEST_ASSERT)
    configASSERT(zero() != 0);
    #elif defined(APP_FAULT_TEST_MALLOC)
    (void)pvPortMalloc(64U * 1024U * 1024U);
    #else
    #error "Unbekannte APP_FAULT_TEST-Art"
    #endif

    vTaskDelete(NULL);
}

void fault_test_start(void)
{
    BSP_LOGW("Fault-Test aktiv: %s", APP_FAULT_TEST_NAME);
    xTaskCreate(fault_test_task, "faulttest", 512, NULL, 1, NULL);
}

#else
void fault_test_start(void) {}
#endif
