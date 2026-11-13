#include "bsp_ws2812.h"

#include "stm32f7xx_hal.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <string.h>

#define WS_GPIO      GPIOB
#define WS_PIN       GPIO_PIN_4           /* Arduino D3, TIM3_CH1 (AF2) */
#define WS_PIN_POS   4U
#define WS_AF        2U
#define WS_TIM       TIM3
#define WS_DMA       DMA1_Stream2         /* TIM3_UP: DMA1, Stream 2, Kanal 5 */
#define WS_DMA_CH    5U
#define WS_DMA_IRQ   DMA1_Stream2_IRQn
#define WS_IRQ_PRIO  6U                   /* >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5) */
#define WS_TIMEOUT_MS 20U

#define SLOTS        WS2812_SLOTS(BSP_WS2812_COUNT)
#define BUF_ENTRIES  ((SLOTS + 15U) & ~15U)          /* auf 32 Byte (Cache-Zeile) aufgerundet */

/* DMA-Puffer im normalen SRAM; 32-Byte-ausgerichtet, damit der Cache gezielt geleert werden kann */
static uint16_t s_buf[BUF_ENTRIES] __attribute__((aligned(32)));
static uint8_t  s_rgb[BSP_WS2812_COUNT][3];
static ws2812_timing_t s_timing;

static SemaphoreHandle_t s_mutex;
static SemaphoreHandle_t s_done;
static volatile int s_error;
static volatile int s_busy;      /* 1 = Uebertragung laeuft (fuer das Warten vor dem Start des Schedulers) */

static void pin_idle(void)       /* Pin als Ausgang, low (Ruhepegel; das Reset-Signal der LEDs) */
{
    WS_GPIO->BSRR = (uint32_t)WS_PIN << 16;
    WS_GPIO->MODER = (WS_GPIO->MODER & ~(3UL << (2U * WS_PIN_POS))) | (1UL << (2U * WS_PIN_POS));
}

static void pin_timer(void)      /* Pin gehoert dem Timer */
{
    WS_GPIO->MODER = (WS_GPIO->MODER & ~(3UL << (2U * WS_PIN_POS))) | (2UL << (2U * WS_PIN_POS));
}

/* Wartet auf das Ende der Uebertragung. Vor dem Start des Schedulers (z. B. app_init) kann nicht auf eine
 * Semaphore gewartet werden; dann wird das Flag s_busy abgefragt. */
static int wait_done(void)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        const uint32_t t0 = HAL_GetTick();
        while (s_busy) {
            if (HAL_GetTick() - t0 > WS_TIMEOUT_MS) {
                return -1;
            }
        }
        return 0;
    }
    return (xSemaphoreTake(s_done, pdMS_TO_TICKS(WS_TIMEOUT_MS)) == pdTRUE) ? 0 : -1;
}

static void transfer_stop(void)
{
    WS_TIM->CR1  &= ~TIM_CR1_CEN;
    WS_TIM->DIER  = 0;
    pin_idle();
}

int bsp_ws2812_init(void)
{
    if (s_mutex != NULL) {
        return 0;
    }
    s_mutex = xSemaphoreCreateMutex();
    s_done  = xSemaphoreCreateBinary();
    if (s_mutex == NULL || s_done == NULL) {
        return -1;
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* Pin: zuerst Ausgang low (PB4 ist nach dem Reset JTAG-NJTRST mit Pull-up), Alternativfunktion TIM3 vorbereiten */
    WS_GPIO->OTYPER  &= ~WS_PIN;
    WS_GPIO->OSPEEDR |= 3UL << (2U * WS_PIN_POS);
    WS_GPIO->PUPDR   &= ~(3UL << (2U * WS_PIN_POS));
    WS_GPIO->AFR[0]   = (WS_GPIO->AFR[0] & ~(0xFUL << (4U * WS_PIN_POS))) | ((uint32_t)WS_AF << (4U * WS_PIN_POS));
    pin_idle();

    /* TIM3: Takt = 2 x PCLK1 (APB1-Teiler 4), Periode 1,25 us, PWM-Modus 1 mit Preload */
    const uint32_t tim_clk = ((RCC->CFGR & RCC_CFGR_PPRE1_2) != 0U) ? 2U * HAL_RCC_GetPCLK1Freq() : HAL_RCC_GetPCLK1Freq();
    s_timing = ws2812_timing(tim_clk);
    WS_TIM->CR1   = 0;
    WS_TIM->PSC   = 0;
    WS_TIM->ARR   = (uint32_t)s_timing.period - 1U;
    WS_TIM->CCMR1 = (6UL << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    WS_TIM->CCER  = TIM_CCER_CC1E;
    WS_TIM->DIER  = 0;

    HAL_NVIC_SetPriority(WS_DMA_IRQ, WS_IRQ_PRIO, 0);
    HAL_NVIC_EnableIRQ(WS_DMA_IRQ);

    memset(s_rgb, 0, sizeof s_rgb);
    return bsp_ws2812_show();
}

void bsp_ws2812_set_pixel(unsigned index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= BSP_WS2812_COUNT || s_mutex == NULL) {
        return;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_rgb[index][0] = r;
    s_rgb[index][1] = g;
    s_rgb[index][2] = b;
    xSemaphoreGive(s_mutex);
}

void bsp_ws2812_set_all(uint8_t r, uint8_t g, uint8_t b)
{
    if (s_mutex == NULL) {
        return;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (unsigned i = 0; i < BSP_WS2812_COUNT; i++) {
        s_rgb[i][0] = r;
        s_rgb[i][1] = g;
        s_rgb[i][2] = b;
    }
    xSemaphoreGive(s_mutex);
}

int bsp_ws2812_show(void)
{
    if (s_mutex == NULL) {
        return -1;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);

    const uint32_t n = ws2812_encode((const uint8_t (*)[3])s_rgb, BSP_WS2812_COUNT, BSP_WS2812_MAX_LEVEL, &s_timing, s_buf);
    SCB_CleanDCache_by_Addr((uint32_t *)s_buf, (int32_t)sizeof s_buf);      /* DMA liest direkt aus dem SRAM */

    /* Timer vorbereiten: Zaehler 0, Schattenregister laden (CCR1 = 0 -> erste Periode low), danach CCR1 mit dem
     * ersten Wert vorladen; der wird mit dem ersten Update-Ereignis uebernommen. Die weiteren n-1 Werte holt
     * bei jedem Update-Ereignis die DMA. */
    WS_TIM->CR1  = 0;
    WS_TIM->DIER = 0;
    WS_TIM->CCR1 = 0;
    WS_TIM->CNT  = 0;
    WS_TIM->EGR  = TIM_EGR_UG;
    WS_TIM->SR   = 0;
    WS_TIM->CCR1 = s_buf[0];

    WS_DMA->CR = 0;
    while ((WS_DMA->CR & DMA_SxCR_EN) != 0U) { }
    DMA1->LIFCR = DMA_LIFCR_CFEIF2 | DMA_LIFCR_CDMEIF2 | DMA_LIFCR_CTEIF2 | DMA_LIFCR_CHTIF2 | DMA_LIFCR_CTCIF2;
    WS_DMA->PAR  = (uint32_t)&WS_TIM->CCR1;
    WS_DMA->M0AR = (uint32_t)&s_buf[1];
    WS_DMA->NDTR = n - 1U;
    WS_DMA->FCR  = 0;                                          /* Direktmodus */
    WS_DMA->CR   = ((uint32_t)WS_DMA_CH << DMA_SxCR_CHSEL_Pos) | (2UL << DMA_SxCR_PL_Pos) |   /* Prioritaet hoch */
                   (1UL << DMA_SxCR_MSIZE_Pos) | (1UL << DMA_SxCR_PSIZE_Pos) |                 /* 16 Bit */
                   DMA_SxCR_MINC | (1UL << DMA_SxCR_DIR_Pos) |                                 /* Speicher -> Peripherie */
                   DMA_SxCR_TCIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE;

    (void)xSemaphoreTake(s_done, 0);
    s_error = 0;
    s_busy = 1;
    WS_DMA->CR |= DMA_SxCR_EN;
    pin_timer();
    WS_TIM->DIER = TIM_DIER_UDE;
    WS_TIM->CR1  = TIM_CR1_CEN;

    int rc = 0;
    if (wait_done() != 0) {
        WS_DMA->CR = 0;
        transfer_stop();
        s_busy = 0;
        rc = -1;
    } else if (s_error) {
        rc = -1;
    }
    xSemaphoreGive(s_mutex);
    return rc;
}

int bsp_ws2812_fill(uint8_t r, uint8_t g, uint8_t b)
{
    bsp_ws2812_set_all(r, g, b);
    return bsp_ws2812_show();
}

void DMA1_Stream2_IRQHandler(void)
{
    const uint32_t isr = DMA1->LISR;
    BaseType_t woken = pdFALSE;

    if ((isr & (DMA_LISR_TEIF2 | DMA_LISR_DMEIF2)) != 0U) {
        DMA1->LIFCR = DMA_LIFCR_CTEIF2 | DMA_LIFCR_CDMEIF2;
        s_error = 1;
    }
    if ((isr & (DMA_LISR_TCIF2 | DMA_LISR_TEIF2 | DMA_LISR_DMEIF2)) != 0U) {
        DMA1->LIFCR = DMA_LIFCR_CTCIF2;
        transfer_stop();
        s_busy = 0;
        xSemaphoreGiveFromISR(s_done, &woken);
    }
    portYIELD_FROM_ISR(woken);
}
