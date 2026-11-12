#include "bsp_system.h"
#include "bsp_sdram.h"
#include "stm32f7xx_hal.h"

static void mpu_config(void)
{
    HAL_MPU_Disable();

    /* SDRAM: Normal-Speicher, cachebar, write-through (LTDC liest per DMA direkt aus
     * dem SDRAM, daher darf der Cache keine "schmutzigen" Zeilen halten). Nicht
     * ausfuehrbar. Ohne diese Region waere der Bereich 0xC0000000 "Device"-Speicher:
     * langsam und ohne unaligned Zugriffe. */
    MPU_Region_InitTypeDef r = {0};
    r.Enable           = MPU_REGION_ENABLE;
    r.Number           = MPU_REGION_NUMBER0;
    r.BaseAddress      = BSP_SDRAM_BASE;
    r.Size             = MPU_REGION_SIZE_8MB;
    r.SubRegionDisable = 0x00;
    r.TypeExtField     = MPU_TEX_LEVEL0;
    r.AccessPermission = MPU_REGION_FULL_ACCESS;
    r.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
    r.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
    r.IsCacheable      = MPU_ACCESS_CACHEABLE;
    r.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&r);

    /* Null-Zeiger-Falle: die ersten 32 Bytes (Adresse 0) sind nicht zugreifbar ->
     * MemManage-Fault statt stillschweigend ITCM zu lesen/schreiben. */
    MPU_Region_InitTypeDef n = {0};
    n.Enable           = MPU_REGION_ENABLE;
    n.Number           = MPU_REGION_NUMBER1;
    n.BaseAddress      = 0x00000000UL;
    n.Size             = MPU_REGION_SIZE_32B;
    n.SubRegionDisable = 0x00;
    n.TypeExtField     = MPU_TEX_LEVEL0;
    n.AccessPermission = MPU_REGION_NO_ACCESS;
    n.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
    n.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
    n.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    n.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&n);

    /* QSPI-Bereich: gesperrt, bis bsp_qspi das Memory-Mapping aktiviert (verhindert
     * spekulative Zugriffe des Cortex-M7 auf den abgeschalteten QUADSPI). */
    MPU_Region_InitTypeDef q = {0};
    q.Enable           = MPU_REGION_ENABLE;
    q.Number           = MPU_REGION_NUMBER2;
    q.BaseAddress      = 0x90000000UL;
    q.Size             = MPU_REGION_SIZE_16MB;
    q.SubRegionDisable = 0x00;
    q.TypeExtField     = MPU_TEX_LEVEL0;
    q.AccessPermission = MPU_REGION_NO_ACCESS;
    q.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
    q.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
    q.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    q.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&q);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void bsp_system_init_early(void)
{
    HAL_Init();
    SCB->VTOR = FLASH_BASE;          /* Vektortabelle explizit im Flash (nicht ueber den Alias bei 0x0) */
    mpu_config();

    /* Eigene Fault-Handler (bsp_fault.c) statt HardFault-Eskalation; Division durch 0 trappen */
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk;
    SCB->CCR   |= SCB_CCR_DIV_0_TRP_Msk;
    SCB_EnableICache();
    SCB_EnableDCache();
}

int bsp_system_clock_config(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* VCO-Eingang 25 MHz / 25 = 1 MHz; VCO = 432 MHz; SYSCLK = 432/2 = 216 MHz; 48-MHz-Takt = 432/9 */
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM       = 25;
    osc.PLL.PLLN       = 432;
    osc.PLL.PLLP       = RCC_PLLP_DIV2;
    osc.PLL.PLLQ       = 9;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        return -1;
    }
    if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
        return -2;
    }

    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                         RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_7) != HAL_OK) {
        return -3;
    }
    return 0;
}

/* HAL-Zeitbasis auf TIM6 (1 kHz), damit SysTick exklusiv FreeRTOS gehoert. */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    __HAL_RCC_TIM6_CLK_ENABLE();

    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t tim_clk = (RCC->CFGR & RCC_CFGR_PPRE1_2) ? (2U * pclk1) : pclk1;

    TIM6->CR1 = 0;
    TIM6->PSC = (tim_clk / 1000000U) - 1U;    /* 1 MHz */
    TIM6->ARR = 1000U - 1U;                   /* 1 kHz */
    TIM6->EGR = TIM_EGR_UG;
    TIM6->SR  = 0;
    TIM6->DIER = TIM_DIER_UIE;

    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, TickPriority, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    TIM6->CR1 = TIM_CR1_CEN;
    return HAL_OK;
}

void TIM6_DAC_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF) {
        TIM6->SR = ~TIM_SR_UIF;
        HAL_IncTick();
    }
}

/* Zaehler fuer die FreeRTOS-Laufzeitstatistik: DWT-Zyklenzaehler (216 MHz) / 256 = 0,84 MHz */
void bsp_runtime_counter_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55U;               /* Cortex-M7: Debug-Register entsperren */
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t bsp_runtime_counter(void)
{
    return DWT->CYCCNT >> 8;
}
