#include "bsp_cpu_sensor.h"
#include "bsp_sensor_math.h"
#include "stm32f7xx_hal.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define SAMPLES 16U

static ADC_HandleTypeDef hadc;
static SemaphoreHandle_t s_mutex;
static int s_ok;

static uint16_t cal_ts1(void)   { return *TEMPSENSOR_CAL1_ADDR_CMSIS; }
static uint16_t cal_ts2(void)   { return *TEMPSENSOR_CAL2_ADDR_CMSIS; }
static uint16_t cal_vref(void)  { return *VREFINT_CAL_ADDR_CMSIS; }

int bsp_cpu_sensor_init(void)
{
    /* Kalibrierwerte pruefen: nicht programmiert (0 / 0xFFFF) oder verdreht -> kein brauchbarer Messwert */
    const uint16_t c1 = cal_ts1(), c2 = cal_ts2(), cv = cal_vref();
    if (c1 == 0U || c1 == 0xFFFFU || c2 <= c1 || c2 == 0xFFFFU || cv == 0U || cv == 0xFFFFU) {
        return BSP_CPU_SENSOR_ERR_CAL;
    }

    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        return -2;
    }

    __HAL_RCC_ADC1_CLK_ENABLE();
    hadc.Instance                   = ADC1;
    hadc.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;     /* 108 MHz / 4 = 27 MHz */
    hadc.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc.Init.ScanConvMode          = DISABLE;
    hadc.Init.ContinuousConvMode    = DISABLE;
    hadc.Init.DiscontinuousConvMode = DISABLE;
    hadc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc.Init.NbrOfConversion       = 1;
    hadc.Init.DMAContinuousRequests = DISABLE;
    hadc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc) != HAL_OK) {
        return -1;
    }
    s_ok = 1;
    return 0;
}

int bsp_cpu_sensor_available(void)
{
    return s_ok;
}

/* Mittelwert aus SAMPLES Messungen eines internen Kanals; Aufrufer haelt den Mutex */
static int sample_avg(uint32_t channel, float *avg)
{
    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = channel;
    ch.Rank         = 1;
    ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;     /* 17,8 us bei 27 MHz (Sensor braucht >= 10 us) */
    if (HAL_ADC_ConfigChannel(&hadc, &ch) != HAL_OK) {
        return -1;
    }

    uint32_t sum = 0;
    for (unsigned i = 0; i < SAMPLES; i++) {
        if (HAL_ADC_Start(&hadc) != HAL_OK ||
            HAL_ADC_PollForConversion(&hadc, 10) != HAL_OK) {
            (void)HAL_ADC_Stop(&hadc);
            return -1;
        }
        sum += HAL_ADC_GetValue(&hadc);
        (void)HAL_ADC_Stop(&hadc);
    }
    *avg = (float)sum / (float)SAMPLES;
    return 0;
}

static int locking(void)
{
    return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

/* VDDA (immer messen: auch die Temperaturkorrektur braucht sie) */
static int read_vdda_locked(float *volts)
{
    float raw;
    if (sample_avg(ADC_CHANNEL_VREFINT, &raw) != 0 || raw < 1.0f) {
        return -1;
    }
    *volts = bsp_sensor_vdda_volts(raw, cal_vref());
    return 0;
}

int bsp_supply_voltage(float *volts)
{
    if (!s_ok || volts == NULL) return -1;
    if (locking()) xSemaphoreTake(s_mutex, portMAX_DELAY);
    float v = 0;
    int rc = read_vdda_locked(&v);
    if (locking()) xSemaphoreGive(s_mutex);
    if (rc == 0) *volts = v;
    return rc;
}

int bsp_cpu_temperature(float *celsius)
{
    if (!s_ok || celsius == NULL) return -1;
    if (locking()) xSemaphoreTake(s_mutex, portMAX_DELAY);
    float vdda = 0, ts = 0;
    int rc = read_vdda_locked(&vdda);
    if (rc == 0) rc = sample_avg(ADC_CHANNEL_TEMPSENSOR, &ts);
    if (locking()) xSemaphoreGive(s_mutex);
    if (rc == 0) *celsius = bsp_sensor_temp_celsius(ts, vdda, cal_ts1(), cal_ts2());
    return rc;
}
