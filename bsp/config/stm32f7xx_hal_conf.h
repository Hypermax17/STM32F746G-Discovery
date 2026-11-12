/**
 * HAL-Konfiguration fuer das STM32F746G-Discovery-Board.
 * Nur die benoetigten HAL-Module sind aktiviert. Wird ein neuer Treiber im
 * BSP ergaenzt, muss hier das passende Modul eingeschaltet werden (und in
 * third_party/CMakeLists.txt in HAL_MODULES aufgenommen werden).
 */
#ifndef STM32F7XX_HAL_CONF_H
#define STM32F7XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Modulauswahl ------------------------------------------------------- */
#define HAL_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_LTDC_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_QSPI_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_SAI_MODULE_ENABLED
#define HAL_SDRAM_MODULE_ENABLED

/* ---- Oszillatoren ------------------------------------------------------- */
#if !defined(HSE_VALUE)
#define HSE_VALUE 25000000U            /* 25-MHz-Quarz X2 auf dem Board */
#endif
#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT 100U
#endif
#if !defined(HSI_VALUE)
#define HSI_VALUE 16000000U
#endif
#if !defined(LSI_VALUE)
#define LSI_VALUE 32000U
#endif
#if !defined(LSE_VALUE)
#define LSE_VALUE 32768U
#endif
#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT 5000U
#endif
#if !defined(EXTERNAL_CLOCK_VALUE)
#define EXTERNAL_CLOCK_VALUE 12288000U
#endif

/* ---- Systemkonfiguration ------------------------------------------------ */
#define VDD_VALUE                3300U
#define TICK_INT_PRIORITY        1U    /* HAL-Tick (TIM6): ausserhalb der FreeRTOS-Maskierung */
#define USE_RTOS                 0U
#define PREFETCH_ENABLE          1U
#define ART_ACCELERATOR_ENABLE   1U

#define USE_HAL_ADC_REGISTER_CALLBACKS    0U
#define USE_HAL_I2C_REGISTER_CALLBACKS    0U
#define USE_HAL_LTDC_REGISTER_CALLBACKS   0U
#define USE_HAL_QSPI_REGISTER_CALLBACKS   0U
#define USE_HAL_SAI_REGISTER_CALLBACKS    0U
#define USE_HAL_SDRAM_REGISTER_CALLBACKS  0U

/* #define USE_FULL_ASSERT 1U */

/* ---- Includes ----------------------------------------------------------- */
#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32f7xx_hal_rcc.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32f7xx_hal_gpio.h"
#endif
#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32f7xx_hal_dma.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32f7xx_hal_cortex.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32f7xx_hal_flash.h"
#endif
#ifdef HAL_ADC_MODULE_ENABLED
#include "stm32f7xx_hal_adc.h"
#endif
#ifdef HAL_SDRAM_MODULE_ENABLED
#include "stm32f7xx_hal_sdram.h"
#endif
#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32f7xx_hal_i2c.h"
#endif
#ifdef HAL_LTDC_MODULE_ENABLED
#include "stm32f7xx_hal_ltdc.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32f7xx_hal_pwr.h"
#endif
#ifdef HAL_QSPI_MODULE_ENABLED
#include "stm32f7xx_hal_qspi.h"
#endif
#ifdef HAL_SAI_MODULE_ENABLED
#include "stm32f7xx_hal_sai.h"
#endif

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line);
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
#else
#define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* STM32F7XX_HAL_CONF_H */
