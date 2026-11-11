#include "bsp_qspi.h"
#include "stm32f7xx_hal.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* Befehle MT25QL128 */
#define CMD_RESET_ENABLE   0x66U
#define CMD_RESET_MEMORY   0x99U
#define CMD_READ_ID        0x9FU
#define CMD_WRITE_ENABLE   0x06U
#define CMD_READ_STATUS    0x05U
#define CMD_READ_FLAGSTAT  0x70U
#define CMD_CLEAR_FLAGSTAT 0x50U
#define CMD_QUAD_OUT_READ  0x6BU      /* 1-1-4, 8 Dummy-Takte (Chip-Standard) */
#define CMD_PAGE_PROGRAM   0x02U
#define CMD_ERASE_4K       0x20U
#define CMD_ERASE_64K      0xD8U

#define READ_DUMMY_CYCLES  8U
#define SR_WIP             0x01U
#define SR_WEL             0x02U
#define FSR_READY          0x80U
#define FSR_ERASE_ERR      0x20U
#define FSR_PROGRAM_ERR    0x10U
#define FSR_PROTECT_ERR    0x02U

#define TIMEOUT_CMD_MS     100U
#define TIMEOUT_PROGRAM_MS 10U
#define TIMEOUT_ERASE4K_MS 800U
#define TIMEOUT_ERASE64_MS 3000U

#define QSPI_MPU_REGION    MPU_REGION_NUMBER2

static QSPI_HandleTypeDef hqspi;
static SemaphoreHandle_t  s_mutex;
static int s_available;
static int s_mapped;

/* ---- Hilfsfunktionen ---------------------------------------------------- */

static int sched_running(void)
{
    return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

static void lock(void)
{
    if (s_mutex != NULL && sched_running()) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
    }
}

static void unlock(void)
{
    if (s_mutex != NULL && sched_running()) {
        xSemaphoreGive(s_mutex);
    }
}

static void gpio_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_AF_PP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    g.Pin = GPIO_PIN_6;                         /* PB6: NCS */
    g.Pull = GPIO_PULLUP;
    g.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOB, &g);

    g.Pin = GPIO_PIN_2;                         /* PB2: CLK */
    g.Pull = GPIO_NOPULL;
    g.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOB, &g);

    g.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;   /* PD11 IO0, PD12 IO1, PD13 IO3 */
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &g);

    g.Pin = GPIO_PIN_2;                         /* PE2: IO2 */
    HAL_GPIO_Init(GPIOE, &g);
}

/* Basis-Befehl; Aufrufer setzt die uebrigen Felder */
static QSPI_CommandTypeDef base_cmd(uint8_t instr)
{
    QSPI_CommandTypeDef c = {0};
    c.Instruction       = instr;
    c.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    c.AddressMode       = QSPI_ADDRESS_NONE;
    c.AddressSize       = QSPI_ADDRESS_24_BITS;
    c.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    c.DataMode          = QSPI_DATA_NONE;
    c.DummyCycles       = 0;
    c.DdrMode           = QSPI_DDR_MODE_DISABLE;
    c.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    c.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    return c;
}

static int cmd_only(uint8_t instr, uint32_t instr_mode)
{
    QSPI_CommandTypeDef c = base_cmd(instr);
    c.InstructionMode = instr_mode;
    return (HAL_QSPI_Command(&hqspi, &c, TIMEOUT_CMD_MS) == HAL_OK) ? 0 : -1;
}

static int read_reg(uint8_t instr, uint8_t *buf, uint32_t len)
{
    QSPI_CommandTypeDef c = base_cmd(instr);
    c.DataMode = QSPI_DATA_1_LINE;
    c.NbData   = len;
    if (HAL_QSPI_Command(&hqspi, &c, TIMEOUT_CMD_MS) != HAL_OK) return -1;
    return (HAL_QSPI_Receive(&hqspi, buf, TIMEOUT_CMD_MS) == HAL_OK) ? 0 : -1;
}

static int write_enable(void)
{
    if (cmd_only(CMD_WRITE_ENABLE, QSPI_INSTRUCTION_1_LINE) != 0) return -1;
    uint8_t sr = 0;
    if (read_reg(CMD_READ_STATUS, &sr, 1) != 0) return -1;
    return (sr & SR_WEL) ? 0 : -1;
}

/* Wartet, bis der Chip nicht mehr beschaeftigt ist (WIP = 0). Gibt die CPU ab, wenn es laenger dauert. */
static int wait_ready(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    for (unsigned spins = 0;; spins++) {
        uint8_t sr = 0xFF;
        if (read_reg(CMD_READ_STATUS, &sr, 1) != 0) return -1;
        if ((sr & SR_WIP) == 0U) return 0;
        if (HAL_GetTick() - start > timeout_ms) return -2;
        if (spins > 20U) {
            if (sched_running()) vTaskDelay(1); else HAL_Delay(1);
        }
    }
}

/* Nach Programmieren/Loeschen: Fehlerflags des Chips pruefen */
static int check_flags(void)
{
    uint8_t fsr = 0;
    if (read_reg(CMD_READ_FLAGSTAT, &fsr, 1) != 0) return -1;
    if (fsr & (FSR_ERASE_ERR | FSR_PROGRAM_ERR | FSR_PROTECT_ERR)) {
        (void)cmd_only(CMD_CLEAR_FLAGSTAT, QSPI_INSTRUCTION_1_LINE);
        return -3;
    }
    return 0;
}

/* ---- MPU / Cache fuer den Memory-Mapped-Bereich ------------------------- *
 * Solange das Memory-Mapping aus ist, ist der Bereich 0x90000000 per MPU gesperrt:
 * Der Cortex-M7 darf nicht spekulativ darauf zugreifen (haengt sonst den Bus auf). */
static void mpu_qspi(int mapped)
{
    MPU_Region_InitTypeDef r = {0};
    r.Enable           = MPU_REGION_ENABLE;
    r.Number           = QSPI_MPU_REGION;
    r.BaseAddress      = BSP_QSPI_BASE;
    r.Size             = MPU_REGION_SIZE_16MB;
    r.SubRegionDisable = 0x00;
    r.TypeExtField     = MPU_TEX_LEVEL0;
    r.AccessPermission = mapped ? MPU_REGION_PRIV_RO : MPU_REGION_NO_ACCESS;
    r.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
    r.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
    r.IsCacheable      = mapped ? MPU_ACCESS_CACHEABLE : MPU_ACCESS_NOT_CACHEABLE;   /* write-through */
    r.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;

    /* Waehrend die MPU aus ist, gilt die Standard-Speicherkarte (SDRAM = Device):
     * keine Unterbrechung zulassen. */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    SCB_CleanInvalidateDCache();
    HAL_MPU_Disable();
    HAL_MPU_ConfigRegion(&r);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
    SCB_CleanInvalidateDCache();
    __set_PRIMASK(primask);
}

static int mapped_on(void)
{
    QSPI_CommandTypeDef c = base_cmd(CMD_QUAD_OUT_READ);
    c.AddressMode = QSPI_ADDRESS_1_LINE;
    c.DataMode    = QSPI_DATA_4_LINES;
    c.DummyCycles = READ_DUMMY_CYCLES;

    QSPI_MemoryMappedTypeDef cfg = {0};
    cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    cfg.TimeOutPeriod     = 0;

    mpu_qspi(1);
    if (HAL_QSPI_MemoryMapped(&hqspi, &c, &cfg) != HAL_OK) {
        mpu_qspi(0);
        return -1;
    }
    s_mapped = 1;
    return 0;
}

static int mapped_off(void)
{
    if (!s_mapped) return 0;
    int rc = (HAL_QSPI_Abort(&hqspi) == HAL_OK) ? 0 : -1;
    s_mapped = 0;
    mpu_qspi(0);
    return rc;
}

/* ---- Oeffentliche API ---------------------------------------------------- */

int bsp_qspi_init(bsp_qspi_id_t *id)
{
    bsp_qspi_id_t local = {0};
    if (id == NULL) id = &local;
    id->manufacturer = id->type = id->capacity = 0;

    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) return -2;

    mpu_qspi(0);                                /* Bereich sperren, bis das Mapping aktiv ist */
    gpio_init();

    __HAL_RCC_QSPI_CLK_ENABLE();
    __HAL_RCC_QSPI_FORCE_RESET();
    __HAL_RCC_QSPI_RELEASE_RESET();

    hqspi.Instance                = QUADSPI;
    hqspi.Init.ClockPrescaler     = 1;                               /* 216 MHz / 2 = 108 MHz */
    hqspi.Init.FifoThreshold      = 4;
    hqspi.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    hqspi.Init.FlashSize          = 23;                              /* 2^(23+1) = 16 MB */
    hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_2_CYCLE;
    hqspi.Init.ClockMode          = QSPI_CLOCK_MODE_0;
    hqspi.Init.FlashID            = QSPI_FLASH_ID_1;
    hqspi.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;
    if (HAL_QSPI_Init(&hqspi) != HAL_OK) return -3;

    /* Reset: erst im 4-Leitungs-Protokoll (falls ein frueheres Programm den Chip in QPI
     * gelassen hat), dann im normalen SPI-Protokoll. */
    (void)cmd_only(CMD_RESET_ENABLE, QSPI_INSTRUCTION_4_LINES);
    (void)cmd_only(CMD_RESET_MEMORY, QSPI_INSTRUCTION_4_LINES);
    (void)cmd_only(CMD_RESET_ENABLE, QSPI_INSTRUCTION_1_LINE);
    (void)cmd_only(CMD_RESET_MEMORY, QSPI_INSTRUCTION_1_LINE);
    HAL_Delay(5);

    uint8_t raw[3] = {0};
    if (read_reg(CMD_READ_ID, raw, 3) != 0) return -4;
    id->manufacturer = raw[0];
    id->type         = raw[1];
    id->capacity     = raw[2];
    if (raw[0] != 0x20U || raw[2] != 0x18U) return -5;

    if (wait_ready(TIMEOUT_ERASE64_MS) != 0) return -6;
    s_available = 1;
    return 0;
}

int bsp_qspi_available(void)
{
    return s_available;
}

int bsp_qspi_is_memory_mapped(void)
{
    return s_mapped;
}

int bsp_qspi_memory_mapped_enable(void)
{
    if (!s_available) return -1;
    lock();
    int rc = s_mapped ? 0 : mapped_on();
    unlock();
    return rc;
}

int bsp_qspi_memory_mapped_disable(void)
{
    if (!s_available) return -1;
    lock();
    int rc = mapped_off();
    unlock();
    return rc;
}

int bsp_qspi_read(uint32_t addr, void *buf, uint32_t len)
{
    if (!s_available || addr > BSP_QSPI_SIZE || len > BSP_QSPI_SIZE - addr) return -1;
    uint8_t *dst = buf;
    int rc = 0;

    lock();
    int was_mapped = s_mapped;
    if (mapped_off() != 0) { unlock(); return -2; }

    while (len > 0U && rc == 0) {
        uint32_t n = (len > 65536U) ? 65536U : len;
        QSPI_CommandTypeDef c = base_cmd(CMD_QUAD_OUT_READ);
        c.AddressMode = QSPI_ADDRESS_1_LINE;
        c.Address     = addr;
        c.DataMode    = QSPI_DATA_4_LINES;
        c.DummyCycles = READ_DUMMY_CYCLES;
        c.NbData      = n;
        if (HAL_QSPI_Command(&hqspi, &c, TIMEOUT_CMD_MS) != HAL_OK ||
            HAL_QSPI_Receive(&hqspi, dst, TIMEOUT_CMD_MS + n / 100U) != HAL_OK) {
            rc = -3;
        }
        addr += n; dst += n; len -= n;
    }

    if (was_mapped && mapped_on() != 0 && rc == 0) rc = -4;
    unlock();
    return rc;
}

static int program_page(uint32_t addr, const uint8_t *src, uint32_t n)
{
    if (write_enable() != 0) return -1;
    QSPI_CommandTypeDef c = base_cmd(CMD_PAGE_PROGRAM);
    c.AddressMode = QSPI_ADDRESS_1_LINE;
    c.Address     = addr;
    c.DataMode    = QSPI_DATA_1_LINE;
    c.NbData      = n;
    if (HAL_QSPI_Command(&hqspi, &c, TIMEOUT_CMD_MS) != HAL_OK) return -2;
    if (HAL_QSPI_Transmit(&hqspi, (uint8_t *)src, TIMEOUT_CMD_MS) != HAL_OK) return -3;
    if (wait_ready(TIMEOUT_PROGRAM_MS) != 0) return -4;
    return check_flags();
}

int bsp_qspi_write(uint32_t addr, const void *buf, uint32_t len)
{
    if (!s_available || addr > BSP_QSPI_SIZE || len > BSP_QSPI_SIZE - addr) return -1;
    const uint8_t *src = buf;
    int rc = 0;

    lock();
    int was_mapped = s_mapped;
    if (mapped_off() != 0) { unlock(); return -2; }

    while (len > 0U && rc == 0) {
        uint32_t room = BSP_QSPI_PAGE_SIZE - (addr % BSP_QSPI_PAGE_SIZE);   /* bis zur Seitengrenze */
        uint32_t n = (len < room) ? len : room;
        rc = program_page(addr, src, n);
        addr += n; src += n; len -= n;
    }

    if (was_mapped && mapped_on() != 0 && rc == 0) rc = -5;
    unlock();
    return rc;
}

static int erase(uint8_t instr, uint32_t addr, uint32_t unit, uint32_t timeout)
{
    if (!s_available || addr >= BSP_QSPI_SIZE) return -1;
    addr &= ~(unit - 1U);

    lock();
    int was_mapped = s_mapped;
    int rc = mapped_off();
    if (rc == 0) rc = write_enable();
    if (rc == 0) {
        QSPI_CommandTypeDef c = base_cmd(instr);
        c.AddressMode = QSPI_ADDRESS_1_LINE;
        c.Address     = addr;
        rc = (HAL_QSPI_Command(&hqspi, &c, TIMEOUT_CMD_MS) == HAL_OK) ? 0 : -2;
    }
    if (rc == 0) rc = wait_ready(timeout);
    if (rc == 0) rc = check_flags();
    if (was_mapped && mapped_on() != 0 && rc == 0) rc = -5;
    unlock();
    return rc;
}

int bsp_qspi_erase_subsector(uint32_t addr)
{
    return erase(CMD_ERASE_4K, addr, BSP_QSPI_SUBSECTOR_SIZE, TIMEOUT_ERASE4K_MS);
}

int bsp_qspi_erase_sector(uint32_t addr)
{
    return erase(CMD_ERASE_64K, addr, BSP_QSPI_SECTOR_SIZE, TIMEOUT_ERASE64_MS);
}
