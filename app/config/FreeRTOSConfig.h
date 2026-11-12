#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
#include <stdint.h>
extern uint32_t SystemCoreClock;
#endif

/* ---- Scheduler ---------------------------------------------------------- */
#define configUSE_PREEMPTION                     1
#define configUSE_TIME_SLICING                   1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1
#define configCPU_CLOCK_HZ                       (SystemCoreClock)
#define configTICK_RATE_HZ                       ((TickType_t)1000)
#define configMAX_PRIORITIES                     7
#define configMINIMAL_STACK_SIZE                 ((uint16_t)256)     /* Worte */
#define configMAX_TASK_NAME_LEN                  16
#define configUSE_16_BIT_TICKS                   0
#define configIDLE_SHOULD_YIELD                  1
#define configUSE_TICKLESS_IDLE                  0

/* ---- Speicher ----------------------------------------------------------- */
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configTOTAL_HEAP_SIZE                    ((size_t)(96 * 1024))
#define configAPPLICATION_ALLOCATED_HEAP         0

/* ---- Funktionen / Features ---------------------------------------------- */
#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1
#define configQUEUE_REGISTRY_SIZE                8
#define configUSE_TASK_NOTIFICATIONS             1
#define configUSE_TRACE_FACILITY                 1
#define configUSE_STATS_FORMATTING_FUNCTIONS     0
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configUSE_MALLOC_FAILED_HOOK             1
#define configCHECK_FOR_STACK_OVERFLOW           2
#define configGENERATE_RUN_TIME_STATS            1
/* Laufzeitzaehler: Zyklenzaehler (DWT) / 256, siehe bsp_system.c */
void bsp_runtime_counter_init(void);
uint32_t bsp_runtime_counter(void);
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() bsp_runtime_counter_init()
#define portGET_RUN_TIME_COUNTER_VALUE()         bsp_runtime_counter()

/* ---- Software-Timer ----------------------------------------------------- */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                 8
#define configTIMER_TASK_STACK_DEPTH             (configMINIMAL_STACK_SIZE * 2)

/* ---- API-Auswahl -------------------------------------------------------- */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_vTaskDelayUntil                  1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_uxTaskGetStackHighWaterMark      1
#define INCLUDE_xTimerPendFunctionCall           1

/* ---- Interrupt-Prioritaeten (Cortex-M7: 4 Prioritaetsbits) -------------- *
 * Interrupts mit Prioritaet 0..4 werden nie von FreeRTOS maskiert, duerfen
 * aber keine FreeRTOS-API aufrufen. Prioritaet 5..15: FromISR-Aufrufe erlaubt. */
#define configPRIO_BITS                          4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY  15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY          (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ---- Assert ------------------------------------------------------------- */
void app_assert_failed(const char *file, int line);
#define configASSERT(x) do { if ((x) == 0) { app_assert_failed(__FILE__, __LINE__); } } while (0)

/* ---- Handler-Namen (Vektortabelle aus dem CMSIS-Startup) ---------------- */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
