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

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void bsp_system_init_early(void)
{
    HAL_Init();
    mpu_config();
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
