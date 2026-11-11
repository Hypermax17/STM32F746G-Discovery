#include "bsp_sdram.h"
#include "stm32f7xx_hal.h"

/* Chip: 128 Mbit, 12 Zeilen-, 8 Spaltenadressen, 4 Banks, 16 Bit. */
#define SDRAM_MODEREG_BURST_LENGTH_1          0x0000U
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   0x0000U
#define SDRAM_MODEREG_CAS_LATENCY_2           0x0020U
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD 0x0000U
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE  0x0200U

/* Refresh: 64 ms / 4096 Zeilen = 15.62 us -> bei 108 MHz SDCLK 1539 (inkl. Sicherheitsreserve) */
#define SDRAM_REFRESH_COUNT 1539U

static SDRAM_HandleTypeDef hsdram;

static void sdram_gpio_init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF12_FMC;

    g.Pin = GPIO_PIN_3;                                   /* PC3: SDCKE0 */
    HAL_GPIO_Init(GPIOC, &g);

    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
            GPIO_PIN_14 | GPIO_PIN_15;                    /* PD: D2 D3 D13 D14 D15 D0 D1 */
    HAL_GPIO_Init(GPIOD, &g);

    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 |
            GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 |
            GPIO_PIN_14 | GPIO_PIN_15;                    /* PE: NBL0 NBL1 D4..D12 */
    HAL_GPIO_Init(GPIOE, &g);

    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |
            GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 |
            GPIO_PIN_14 | GPIO_PIN_15;                    /* PF: A0..A5 SDNRAS A6..A9 */
    HAL_GPIO_Init(GPIOF, &g);

    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 |
            GPIO_PIN_15;                                  /* PG: A10 A11 BA0 BA1 SDCLK SDNCAS */
    HAL_GPIO_Init(GPIOG, &g);

    g.Pin = GPIO_PIN_3 | GPIO_PIN_5;                      /* PH: SDNE0 SDNWE */
    HAL_GPIO_Init(GPIOH, &g);
}

static int sdram_send(uint32_t mode, uint32_t refresh_n, uint32_t mode_reg)
{
    FMC_SDRAM_CommandTypeDef cmd;
    cmd.CommandMode            = mode;
    cmd.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
    cmd.AutoRefreshNumber      = refresh_n;
    cmd.ModeRegisterDefinition = mode_reg;
    return (HAL_SDRAM_SendCommand(&hsdram, &cmd, 0xFFFFU) == HAL_OK) ? 0 : -1;
}

int bsp_sdram_init(void)
{
    sdram_gpio_init();
    __HAL_RCC_FMC_CLK_ENABLE();

    FMC_SDRAM_TimingTypeDef timing;
    timing.LoadToActiveDelay    = 2;
    timing.ExitSelfRefreshDelay = 7;
    timing.SelfRefreshTime      = 4;
    timing.RowCycleDelay        = 7;
    timing.WriteRecoveryTime    = 2;
    timing.RPDelay              = 2;
    timing.RCDDelay             = 2;

    hsdram.Instance                = FMC_SDRAM_DEVICE;
    hsdram.Init.SDBank             = FMC_SDRAM_BANK1;
    hsdram.Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_8;
    hsdram.Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12;
    hsdram.Init.MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_16;
    hsdram.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
    hsdram.Init.CASLatency         = FMC_SDRAM_CAS_LATENCY_2;
    hsdram.Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    hsdram.Init.SDClockPeriod      = FMC_SDRAM_CLOCK_PERIOD_2;   /* 216 MHz / 2 = 108 MHz */
    hsdram.Init.ReadBurst          = FMC_SDRAM_RBURST_ENABLE;
    hsdram.Init.ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_0;

    if (HAL_SDRAM_Init(&hsdram, &timing) != HAL_OK) {
        return -1;
    }

    /* Initialisierungssequenz laut Datenblatt des SDRAM-Chips */
    if (sdram_send(FMC_SDRAM_CMD_CLK_ENABLE, 1, 0) != 0) return -1;
    HAL_Delay(1);
    if (sdram_send(FMC_SDRAM_CMD_PALL, 1, 0) != 0) return -1;
    if (sdram_send(FMC_SDRAM_CMD_AUTOREFRESH_MODE, 8, 0) != 0) return -1;

    uint32_t mode_reg = SDRAM_MODEREG_BURST_LENGTH_1 | SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |
                        SDRAM_MODEREG_CAS_LATENCY_2 | SDRAM_MODEREG_OPERATING_MODE_STANDARD |
                        SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;
    if (sdram_send(FMC_SDRAM_CMD_LOAD_MODE, 1, mode_reg) != 0) return -1;

    if (HAL_SDRAM_ProgramRefreshRate(&hsdram, SDRAM_REFRESH_COUNT) != HAL_OK) return -1;
    return 0;
}

/* Test in 4-KB-Bloecken. Der SDRAM-Bereich ist per MPU "write-through": nach dem
 * Schreiben wird der D-Cache fuer den Block verworfen, damit wirklich der
 * Chip gelesen wird und nicht der Cache. */
#define TEST_CHUNK_WORDS 1024U

uint32_t bsp_sdram_selftest(uint32_t *first_fail_addr)
{
    volatile uint32_t *base = (volatile uint32_t *)BSP_SDRAM_BASE;
    const uint32_t words = BSP_SDRAM_SIZE / 4U;
    uint32_t errors = 0;

    /* Pass 1: Muster abhaengig von der Adresse, Pass 2: invertiert (testet Daten- und Adressleitungen) */
    for (uint32_t pass = 0; pass < 2; pass++) {
        for (uint32_t off = 0; off < words; off += TEST_CHUNK_WORDS) {
            for (uint32_t i = 0; i < TEST_CHUNK_WORDS; i++) {
                uint32_t addr = (uint32_t)(uintptr_t)&base[off + i];
                uint32_t val = (addr * 2654435761U) ^ 0xA5A5A5A5U;
                base[off + i] = pass ? ~val : val;
            }
            SCB_InvalidateDCache_by_Addr((uint32_t *)&base[off], (int32_t)(TEST_CHUNK_WORDS * 4U));
            for (uint32_t i = 0; i < TEST_CHUNK_WORDS; i++) {
                uint32_t addr = (uint32_t)(uintptr_t)&base[off + i];
                uint32_t val = (addr * 2654435761U) ^ 0xA5A5A5A5U;
                uint32_t exp = pass ? ~val : val;
                if (base[off + i] != exp) {
                    if (errors == 0 && first_fail_addr != NULL) {
                        *first_fail_addr = addr;
                    }
                    errors++;
                }
            }
        }
    }
    return errors;
}
