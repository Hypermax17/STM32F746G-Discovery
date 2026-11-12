#include "bsp_i2c.h"
#include "stm32f7xx_hal.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* Timing 100 kHz bei 216 MHz Kernel-Takt (SYSCLK als I2C3-Taktquelle), Wert aus dem ST-BSP */
#define I2C3_TIMING  0x40912732UL
#define I2C3_TIMEOUT 50U

static I2C_HandleTypeDef hi2c3;
static SemaphoreHandle_t s_mutex;

static int i2c_hw_init(void)
{
    hi2c3.Instance              = I2C3;
    hi2c3.Init.Timing           = I2C3_TIMING;
    hi2c3.Init.OwnAddress1      = 0;
    hi2c3.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    hi2c3.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    hi2c3.Init.OwnAddress2      = 0;
    hi2c3.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    hi2c3.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c3) != HAL_OK) {
        return -1;
    }
    HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE);
    HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0);
    return 0;
}

int bsp_i2c3_init(void)
{
    RCC_PeriphCLKInitTypeDef clk = {0};
    clk.PeriphClockSelection = RCC_PERIPHCLK_I2C3;
    clk.I2c3ClockSelection   = RCC_I2C3CLKSOURCE_SYSCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&clk) != HAL_OK) {
        return -1;
    }

    __HAL_RCC_GPIOH_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_7 | GPIO_PIN_8;     /* PH7 SCL, PH8 SDA */
    g.Mode      = GPIO_MODE_AF_OD;
    g.Pull      = GPIO_PULLUP;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOH, &g);

    __HAL_RCC_I2C3_CLK_ENABLE();
    __HAL_RCC_I2C3_FORCE_RESET();
    __HAL_RCC_I2C3_RELEASE_RESET();

    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        return -2;
    }
    return i2c_hw_init();
}

static int locking(void)
{
    return s_mutex != NULL && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

static void lock(void)
{
    if (locking()) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
    }
}

static void unlock(void)
{
    if (locking()) {
        xSemaphoreGive(s_mutex);
    }
}

/* Nach einem Fehler die Peripherie neu starten, damit der Bus nicht blockiert bleibt. */
static void recover(void)
{
    HAL_I2C_DeInit(&hi2c3);
    (void)i2c_hw_init();
}

int bsp_i2c3_mem_read(uint8_t addr7, uint8_t reg, uint8_t *buf, uint16_t len)
{
    lock();
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(&hi2c3, (uint16_t)(addr7 << 1), reg,
                                            I2C_MEMADD_SIZE_8BIT, buf, len, I2C3_TIMEOUT);
    if (st != HAL_OK) {
        recover();
    }
    unlock();
    return (st == HAL_OK) ? 0 : -1;
}

int bsp_i2c3_mem_write(uint8_t addr7, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    lock();
    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(&hi2c3, (uint16_t)(addr7 << 1), reg,
                                             I2C_MEMADD_SIZE_8BIT, (uint8_t *)buf, len, I2C3_TIMEOUT);
    if (st != HAL_OK) {
        recover();
    }
    unlock();
    return (st == HAL_OK) ? 0 : -1;
}

int bsp_i2c3_reg16_read(uint8_t addr7, uint16_t reg, uint16_t *value)
{
    uint8_t d[2] = {0, 0};
    lock();
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(&hi2c3, (uint16_t)(addr7 << 1), reg,
                                            I2C_MEMADD_SIZE_16BIT, d, 2, I2C3_TIMEOUT);
    if (st != HAL_OK) {
        recover();
    }
    unlock();
    if (st != HAL_OK) {
        return -1;
    }
    *value = (uint16_t)((d[0] << 8) | d[1]);
    return 0;
}

int bsp_i2c3_reg16_write(uint8_t addr7, uint16_t reg, uint16_t value)
{
    uint8_t d[2] = {(uint8_t)(value >> 8), (uint8_t)(value & 0xFFU)};
    lock();
    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(&hi2c3, (uint16_t)(addr7 << 1), reg,
                                             I2C_MEMADD_SIZE_16BIT, d, 2, I2C3_TIMEOUT);
    if (st != HAL_OK) {
        recover();
    }
    unlock();
    return (st == HAL_OK) ? 0 : -1;
}

int bsp_i2c3_probe(uint8_t addr7)
{
    lock();
    HAL_StatusTypeDef st = HAL_I2C_IsDeviceReady(&hi2c3, (uint16_t)(addr7 << 1), 1, I2C3_TIMEOUT);
    if (st != HAL_OK) {
        recover();
    }
    unlock();
    return (st == HAL_OK) ? 0 : -1;
}
