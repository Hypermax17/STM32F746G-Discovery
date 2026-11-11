#include "bsp_button.h"
#include "stm32f7xx_hal.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define BTN_PORT GPIOI
#define BTN_PIN  GPIO_PIN_11
#define BTN_IRQ  EXTI15_10_IRQn
#define BTN_IRQ_PRIORITY 6U            /* >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5) */
#define DEBOUNCE_MS 20U
#define QUEUE_LEN   8U

static QueueHandle_t s_queue;
static volatile int      s_level;      /* zuletzt akzeptierter Pegel */
static volatile uint32_t s_last_ms;    /* Zeitpunkt der letzten akzeptierten Flanke */

int bsp_button_init(void)
{
    s_queue = xQueueCreate(QUEUE_LEN, sizeof(bsp_button_event_t));
    if (s_queue == NULL) {
        return -1;
    }

    __HAL_RCC_GPIOI_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin   = BTN_PIN;
    g.Mode  = GPIO_MODE_IT_RISING_FALLING;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BTN_PORT, &g);

    s_level   = bsp_button_pressed();
    s_last_ms = HAL_GetTick();

    HAL_NVIC_SetPriority(BTN_IRQ, BTN_IRQ_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(BTN_IRQ);
    return 0;
}

int bsp_button_pressed(void)
{
    return HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN) == GPIO_PIN_SET;
}

int bsp_button_get_event(bsp_button_event_t *ev, uint32_t timeout_ms)
{
    TickType_t ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xQueueReceive(s_queue, ev, ticks) == pdTRUE) ? 0 : -1;
}

void EXTI15_10_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(BTN_PIN) == 0U) {
        return;
    }
    __HAL_GPIO_EXTI_CLEAR_IT(BTN_PIN);

    uint32_t now = HAL_GetTick();
    int level = bsp_button_pressed();
    /* Nur echte Pegelwechsel nach Ablauf der Entprellzeit */
    if (level != s_level && (now - s_last_ms) >= DEBOUNCE_MS) {
        s_level   = level;
        s_last_ms = now;

        bsp_button_event_t ev = {
            .type    = level ? BSP_BUTTON_PRESSED : BSP_BUTTON_RELEASED,
            .tick_ms = now,
        };
        BaseType_t woken = pdFALSE;
        xQueueSendFromISR(s_queue, &ev, &woken);   /* bei voller Queue: Ereignis verwerfen */
        portYIELD_FROM_ISR(woken);
    }
}
