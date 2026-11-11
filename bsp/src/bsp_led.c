#include "bsp_led.h"
#include "stm32f7xx_hal.h"

#define LED_PORT GPIOI
#define LED_PIN  GPIO_PIN_1

void bsp_led_init(void)
{
    __HAL_RCC_GPIOI_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &gpio);
    bsp_led_set(0);
}

void bsp_led_set(int on)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void bsp_led_toggle(void)
{
    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
}

/* Bewusst ohne Timer/Interrupts: funktioniert auch im Fehlerfall. */
static void busy_wait(uint32_t n)
{
    for (volatile uint32_t i = 0; i < n; i++) {
    }
}

void bsp_led_blink_forever(unsigned code)
{
    const uint32_t unit = 4000000U;   /* grob 100 ms bei 216 MHz */
    for (;;) {
        for (unsigned i = 0; i < code; i++) {
            bsp_led_set(1);
            busy_wait(unit);
            bsp_led_set(0);
            busy_wait(unit);
        }
        busy_wait(unit * 8U);
    }
}
