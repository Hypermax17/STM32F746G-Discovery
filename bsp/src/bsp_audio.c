#include "bsp_audio.h"
#include "bsp_i2c.h"
#include "stm32f7xx_hal.h"

#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define CODEC_ADDR7  0x1AU      /* WM8994: 8-Bit-Adresse 0x34 */

/* WM8994-Register */
#define WM_ID                 0x000U   /* erwartet 0x8994 */
#define WM_PWR_MGMT_1         0x001U
#define WM_PWR_MGMT_5         0x005U
#define WM_HPOUT1L_VOL        0x01CU
#define WM_HPOUT1R_VOL        0x01DU
#define WM_OUTPUT_MIXER_1     0x02DU
#define WM_OUTPUT_MIXER_2     0x02EU
#define WM_BIAS_ANTIPOP       0x039U
#define WM_WSEQ_CTRL_1        0x110U
#define WM_AIF1_CLOCKING_1    0x200U
#define WM_CLOCKING_1         0x208U
#define WM_AIF1_RATE          0x210U
#define WM_AIF1_CONTROL_1     0x300U
#define WM_AIF1_MASTER_SLAVE  0x302U
#define WM_AIF1DAC1_FILTERS_1 0x420U
#define WM_AIF1DAC2_FILTERS_1 0x422U
#define WM_DAC1_LEFT_MIXER    0x601U
#define WM_DAC1_RIGHT_MIXER   0x602U
#define WM_DAC2_LEFT_MIXER    0x604U
#define WM_DAC2_RIGHT_MIXER   0x605U
#define WM_ERRATA_102         0x102U
#define WM_ERRATA_817         0x817U

#define HP_VOL_MAX_STEP   57U       /* 0x39 = 0 dB (0x3F waere +6 dB) */

#define BUF_SAMPLES (BSP_AUDIO_HALF_FRAMES * 2U * 2U)   /* zwei Haelften, je Frame 2 Samples */

static SAI_HandleTypeDef hsai;
static DMA_HandleTypeDef hdma;
static QueueHandle_t s_free_q;
static volatile uint32_t s_underruns;
static volatile uint32_t s_errors;
static uint8_t s_volume = BSP_AUDIO_DEFAULT_VOLUME;
static int s_ready;

/* DMA-Puffer: 32-Byte-ausgerichtet (Cache-Zeilen) */
static int16_t s_dma_buf[BUF_SAMPLES] __attribute__((aligned(32)));

static void delay_ms(uint32_t ms)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    } else {
        HAL_Delay(ms);
    }
}

static int wm_write(uint16_t reg, uint16_t val)
{
    return bsp_i2c3_reg16_write(CODEC_ADDR7, reg, val);
}

/* ---- Codec -------------------------------------------------------------- */

static int codec_init(uint16_t *id_out)
{
    uint16_t id = 0;
    if (bsp_i2c3_reg16_read(CODEC_ADDR7, WM_ID, &id) != 0) {
        *id_out = 0;
        return -1;
    }
    *id_out = id;
    if (id != 0x8994U) {
        return -1;
    }

    int err = 0;
    /* Errata-Workaround des WM8994 */
    err |= wm_write(WM_ERRATA_102, 0x0003);
    err |= wm_write(WM_ERRATA_817, 0x0000);
    err |= wm_write(WM_ERRATA_102, 0x0000);

    /* VMID Soft-Start, Bias-Strom; Bias-Generator und VMID einschalten */
    err |= wm_write(WM_BIAS_ANTIPOP, 0x006C);
    err |= wm_write(WM_PWR_MGMT_1, 0x0003);
    delay_ms(50);

    /* Signalweg: AIF1 Timeslot 0 -> DAC1 (L/R); DAC1 einschalten; DAC2-Pfade aus */
    err |= wm_write(WM_PWR_MGMT_5, 0x0303);
    err |= wm_write(WM_DAC1_LEFT_MIXER, 0x0001);
    err |= wm_write(WM_DAC1_RIGHT_MIXER, 0x0001);
    err |= wm_write(WM_DAC2_LEFT_MIXER, 0x0000);
    err |= wm_write(WM_DAC2_RIGHT_MIXER, 0x0000);

    /* Takt: AIF1 48 kHz, MCLK/fs = 256; 16 Bit, I2S, Slave; Core- und AIF1-Takt ein (Quelle MCLK1) */
    err |= wm_write(WM_AIF1_RATE, 0x0083);
    err |= wm_write(WM_AIF1_CONTROL_1, 0x4010);
    err |= wm_write(WM_AIF1_MASTER_SLAVE, 0x0000);
    err |= wm_write(WM_CLOCKING_1, 0x000A);
    err |= wm_write(WM_AIF1_CLOCKING_1, 0x0001);

    /* DAC1 -> Kopfhoerer-Ausgangsstufe */
    err |= wm_write(WM_OUTPUT_MIXER_1, 0x0100);
    err |= wm_write(WM_OUTPUT_MIXER_2, 0x0100);

    /* Kopfhoerer-Kaltstart ueber die eingebaute Write-Sequenz (Index 0) */
    err |= wm_write(WM_WSEQ_CTRL_1, 0x8100);
    delay_ms(300);

    /* Zunaechst stumm */
    err |= wm_write(WM_AIF1DAC1_FILTERS_1, 0x0200);
    err |= wm_write(WM_AIF1DAC2_FILTERS_1, 0x0200);
    return err ? -1 : 0;
}

int bsp_audio_set_volume(uint8_t percent)
{
    if (percent > 100U) percent = 100U;
    s_volume = percent;
    uint16_t step = (uint16_t)((percent * HP_VOL_MAX_STEP) / 100U);
    uint16_t val = (uint16_t)(step | 0x140U);       /* HPOUT1_VU (Bit 8) + MUTE_N (Bit 6, 1 = nicht stumm) */
    int err = wm_write(WM_HPOUT1L_VOL, val);
    err |= wm_write(WM_HPOUT1R_VOL, val);
    return err ? -1 : 0;
}

int bsp_audio_set_mute(int mute)
{
    uint16_t v = mute ? 0x0200U : 0x0010U;           /* Soft-Mute bzw. Unmute mit Rampe */
    int err = wm_write(WM_AIF1DAC1_FILTERS_1, v);
    err |= wm_write(WM_AIF1DAC2_FILTERS_1, v);
    return err ? -1 : 0;
}

/* ---- SAI2 + DMA --------------------------------------------------------- */

static int clock_config(void)
{
    /* PLLI2S: VCO = 1 MHz * 344 = 344 MHz; /Q (7) = 49,14 MHz; DivQ = 1 -> SAI2-Kernel-Takt.
     * (Die LCD-Pixeltaktquelle PLLSAI bleibt unberuehrt.) Das HAL teilt daraus MCLK = 256 * fs. */
    RCC_PeriphCLKInitTypeDef clk = {0};
    clk.PeriphClockSelection = RCC_PERIPHCLK_SAI2;
    clk.Sai2ClockSelection   = RCC_SAI2CLKSOURCE_PLLI2S;
    clk.PLLI2S.PLLI2SN = 344;
    clk.PLLI2S.PLLI2SP = RCC_PLLP_DIV8;
    clk.PLLI2S.PLLI2SQ = 7;
    clk.PLLI2S.PLLI2SR = 2;
    clk.PLLI2SDivQ     = 1;
    return (HAL_RCCEx_PeriphCLKConfig(&clk) == HAL_OK) ? 0 : -1;
}

static void gpio_init(void)
{
    __HAL_RCC_GPIOI_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;   /* MCLK_A, SCK_A, SD_A, FS_A */
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF10_SAI2;
    HAL_GPIO_Init(GPIOI, &g);
}

static int sai_init(void)
{
    __HAL_RCC_SAI2_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    hdma.Instance                 = DMA2_Stream4;
    hdma.Init.Channel             = DMA_CHANNEL_3;
    hdma.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma.Init.MemInc              = DMA_MINC_ENABLE;
    hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma.Init.Mode                = DMA_CIRCULAR;
    hdma.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    hdma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma.Init.MemBurst            = DMA_MBURST_SINGLE;
    hdma.Init.PeriphBurst         = DMA_PBURST_SINGLE;
    HAL_DMA_DeInit(&hdma);
    if (HAL_DMA_Init(&hdma) != HAL_OK) return -1;

    hsai.Instance = SAI2_Block_A;
    __HAL_SAI_DISABLE(&hsai);
    HAL_SAI_DeInit(&hsai);
    __HAL_LINKDMA(&hsai, hdmatx, hdma);

    hsai.Init.AudioMode      = SAI_MODEMASTER_TX;
    hsai.Init.Synchro        = SAI_ASYNCHRONOUS;
    hsai.Init.SynchroExt     = SAI_SYNCEXT_DISABLE;
    hsai.Init.OutputDrive    = SAI_OUTPUTDRIVE_ENABLE;
    hsai.Init.NoDivider      = SAI_MASTERDIVIDER_ENABLE;      /* MCLK-Ausgabe, fs = MCLK/256 */
    hsai.Init.FIFOThreshold  = SAI_FIFOTHRESHOLD_1QF;
    hsai.Init.AudioFrequency = BSP_AUDIO_SAMPLE_RATE;
    hsai.Init.Mckdiv         = 0;
    hsai.Init.MonoStereoMode = SAI_STEREOMODE;
    hsai.Init.CompandingMode = SAI_NOCOMPANDING;
    hsai.Init.TriState       = SAI_OUTPUT_NOTRELEASED;
    hsai.Init.Protocol       = SAI_FREE_PROTOCOL;
    hsai.Init.DataSize       = SAI_DATASIZE_16;
    hsai.Init.FirstBit       = SAI_FIRSTBIT_MSB;
    hsai.Init.ClockStrobing  = SAI_CLOCKSTROBING_RISINGEDGE;

    /* I2S: 32 Bit je Frame (16 Bit links, 16 Bit rechts), FS niedrig = linker Kanal, ein Bit vor dem MSB */
    hsai.FrameInit.FrameLength       = 32;
    hsai.FrameInit.ActiveFrameLength = 16;
    hsai.FrameInit.FSDefinition      = SAI_FS_CHANNEL_IDENTIFICATION;
    hsai.FrameInit.FSPolarity        = SAI_FS_ACTIVE_LOW;
    hsai.FrameInit.FSOffset          = SAI_FS_BEFOREFIRSTBIT;

    hsai.SlotInit.FirstBitOffset = 0;
    hsai.SlotInit.SlotSize       = SAI_SLOTSIZE_DATASIZE;
    hsai.SlotInit.SlotNumber     = 2;
    hsai.SlotInit.SlotActive     = SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1;

    if (HAL_SAI_Init(&hsai) != HAL_OK) return -2;

    HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
    HAL_NVIC_SetPriority(SAI2_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(SAI2_IRQn);

    /* Block einschalten: erzeugt MCLK, den der Codec fuer seine Konfiguration braucht */
    __HAL_SAI_ENABLE(&hsai);
    return 0;
}

int bsp_audio_init(uint16_t *codec_id)
{
    uint16_t id = 0;
    if (codec_id == NULL) codec_id = &id;

    s_free_q = xQueueCreate(2, sizeof(int16_t *));
    if (s_free_q == NULL) return -2;

    if (clock_config() != 0) return -2;
    gpio_init();
    if (sai_init() != 0) return -2;

    if (codec_init(codec_id) != 0) return -1;
    if (bsp_audio_set_volume(s_volume) != 0) return -1;
    s_ready = 1;
    return 0;
}

int bsp_audio_start(void)
{
    if (!s_ready) return -1;
    memset(s_dma_buf, 0, sizeof s_dma_buf);
    SCB_CleanDCache_by_Addr((uint32_t *)s_dma_buf, (int32_t)sizeof s_dma_buf);
    xQueueReset(s_free_q);

    if (HAL_SAI_Transmit_DMA(&hsai, (uint8_t *)s_dma_buf, BUF_SAMPLES) != HAL_OK) return -2;
    return bsp_audio_set_mute(0);
}

void bsp_audio_stop(void)
{
    if (!s_ready) return;
    (void)bsp_audio_set_mute(1);
    (void)HAL_SAI_DMAStop(&hsai);
    __HAL_SAI_ENABLE(&hsai);            /* MCLK weiter ausgeben */
    xQueueReset(s_free_q);
}

int bsp_audio_next_buffer(int16_t **buf, uint32_t timeout_ms)
{
    return (xQueueReceive(s_free_q, buf, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) ? 0 : -1;
}

void bsp_audio_release_buffer(int16_t *buf)
{
    SCB_CleanDCache_by_Addr((uint32_t *)buf, (int32_t)(BSP_AUDIO_HALF_FRAMES * 2U * sizeof(int16_t)));
}

uint32_t bsp_audio_underruns(void) { return s_underruns; }
uint32_t bsp_audio_errors(void)    { return s_errors; }

/* ---- Interrupts ---------------------------------------------------------- */

static void half_done(int16_t *half)
{
    BaseType_t woken = pdFALSE;
    if (xQueueSendFromISR(s_free_q, &half, &woken) != pdTRUE) {
        s_underruns++;
    }
    portYIELD_FROM_ISR(woken);
}

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *h)
{
    if (h == &hsai) half_done(&s_dma_buf[0]);
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *h)
{
    if (h == &hsai) half_done(&s_dma_buf[BUF_SAMPLES / 2U]);
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *h)
{
    if (h == &hsai) s_errors++;
}

void DMA2_Stream4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma);
}

void SAI2_IRQHandler(void)
{
    HAL_SAI_IRQHandler(&hsai);
}
