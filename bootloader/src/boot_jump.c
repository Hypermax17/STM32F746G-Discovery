#include "boot_jump.h"

#include "stm32f7xx_hal.h"

/* Wie HAL_RCC_DeInit(), aber ohne dessen Nebenwirkungen: das HAL setzt die Reset-Flags in RCC->CSR zurueck
 * (die Applikation will die Reset-Ursache noch lesen) und startet den HAL-Tick neu (Interrupt wieder frei). */
static void rcc_back_to_hsi(void)
{
    RCC->CR |= RCC_CR_HSION;
    for (uint32_t n = 0; n < 1000000U && !(RCC->CR & RCC_CR_HSIRDY); n++) { }
    RCC->CFGR = 0;                                          /* SYSCLK = HSI, alle Teiler 1 */
    for (uint32_t n = 0; n < 1000000U && (RCC->CFGR & RCC_CFGR_SWS) != 0U; n++) { }
    RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_PLLI2SON | RCC_CR_PLLSAION | RCC_CR_CSSON | RCC_CR_HSEON | RCC_CR_HSEBYP);
    RCC->PLLCFGR     = 0x24003010U;                         /* Reset-Werte */
    RCC->PLLI2SCFGR  = 0x24003000U;
    RCC->PLLSAICFGR  = 0x24003000U;
    RCC->CIR         = 0x00FF0000U;                         /* Interrupts aus, Flags loeschen */
}

void boot_jump_to_app(uint32_t base)
{
    const uint32_t sp    = *(const volatile uint32_t *)base;
    const uint32_t entry = *(const volatile uint32_t *)(base + 4U);

    __disable_irq();

    /* SysTick und alle Interrupts abschalten, anstehende Anforderungen verwerfen */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    for (unsigned i = 0; i < (sizeof NVIC->ICER / sizeof NVIC->ICER[0]); i++) {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }
    SCB->ICSR = SCB_ICSR_PENDSVCLR_Msk | SCB_ICSR_PENDSTCLR_Msk;

    /* Takt zurueck auf HSI, dann alle Peripheriemodule zuruecksetzen (LTDC, FMC, DMA, ...), damit die
     * Applikation wie nach einem Reset startet. */
    rcc_back_to_hsi();
    __HAL_RCC_AHB1_FORCE_RESET();
    __HAL_RCC_AHB1_RELEASE_RESET();
    __HAL_RCC_AHB2_FORCE_RESET();
    __HAL_RCC_AHB2_RELEASE_RESET();
    __HAL_RCC_AHB3_FORCE_RESET();
    __HAL_RCC_AHB3_RELEASE_RESET();
    __HAL_RCC_APB1_FORCE_RESET();
    __HAL_RCC_APB1_RELEASE_RESET();
    __HAL_RCC_APB2_FORCE_RESET();
    __HAL_RCC_APB2_RELEASE_RESET();

    /* Caches und MPU aus (die Applikation richtet sie selbst wieder ein) */
    SCB_DisableDCache();
    SCB_DisableICache();
    HAL_MPU_Disable();

    SCB->VTOR = base;
    __set_CONTROL(0);
    __set_MSP(sp);
    __DSB();
    __ISB();
    __enable_irq();            /* Reset-Zustand: PRIMASK = 0 (alle Interrupts sind im NVIC aus) */

    ((void (*)(void))entry)();
    for (;;) { }
}
