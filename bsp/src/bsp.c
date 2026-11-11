#include "bsp.h"
#include "bsp_system.h"

#include "stm32f7xx_hal.h"

static void log_reset_cause(void)
{
    uint32_t csr = RCC->CSR;
    const char *cause = "unbekannt";
    if (csr & RCC_CSR_LPWRRSTF)        cause = "Low-Power";
    else if (csr & RCC_CSR_WWDGRSTF)   cause = "Window-Watchdog";
    else if (csr & RCC_CSR_IWDGRSTF)   cause = "Watchdog";
    else if (csr & RCC_CSR_SFTRSTF)    cause = "Software-Reset";
    else if (csr & RCC_CSR_PORRSTF)    cause = "Power-On";
    else if (csr & RCC_CSR_PINRSTF)    cause = "Reset-Taster";
    else if (csr & RCC_CSR_BORRSTF)    cause = "Brown-Out";
    BSP_LOGI("Reset: %s", cause);
    RCC->CSR |= RCC_CSR_RMVF;
}

void bsp_init(void)
{
    bsp_system_init_early();         /* HAL, MPU, Caches */
    bsp_led_init();

    if (bsp_system_clock_config() != 0) {
        bsp_led_blink_forever(2);
    }

    if (bsp_sdram_init() != 0) {
        bsp_led_blink_forever(3);
    }
    uint32_t t0 = HAL_GetTick();
    uint32_t fail_addr = 0;
    uint32_t sdram_errors = bsp_sdram_selftest(&fail_addr);
    uint32_t sdram_ms = HAL_GetTick() - t0;
    if (sdram_errors != 0) {
        bsp_led_blink_forever(4);
    }

    if (bsp_lcd_init() != 0) {
        bsp_led_blink_forever(5);
    }
    if (bsp_console_init() != 0) {
        bsp_led_blink_forever(5);
    }

    BSP_LOGI("Display OK (%ux%u RGB565)", (unsigned)BSP_LCD_WIDTH, (unsigned)BSP_LCD_HEIGHT);
    BSP_LOGI("Takt: %lu MHz", (unsigned long)(SystemCoreClock / 1000000UL));
    BSP_LOGI("SDRAM: %lu KB, Test OK (%lu ms)", (unsigned long)(BSP_SDRAM_SIZE / 1024UL),
             (unsigned long)sdram_ms);
    log_reset_cause();
}
