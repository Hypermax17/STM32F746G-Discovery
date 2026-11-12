#include "bsp_dht.h"
#include "stm32f7xx_hal.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define DHT_PORT GPIOG
#define DHT_PIN  GPIO_PIN_7                 /* Arduino D4 */

#define START_LOW_MS      22U               /* Startsignal: mindestens 18 ms Low */
#define MIN_INTERVAL_MS   1100U             /* Abstand zwischen zwei Messungen */

void bsp_runtime_counter_init(void);        /* bsp_system.c: schaltet den DWT-Zyklenzaehler ein */

static SemaphoreHandle_t s_mutex;
static uint32_t s_last_ms;
static int s_ready;

static int io_pin(void)
{
    return (DHT_PORT->IDR & DHT_PIN) != 0U;
}

static uint32_t io_cycles(void)
{
    return DWT->CYCCNT;
}

static void delay_ms(uint32_t ms)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    } else {
        HAL_Delay(ms);
    }
}

int bsp_dht_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        return -1;
    }
    bsp_runtime_counter_init();

    __HAL_RCC_GPIOG_CLK_ENABLE();
    /* Open-Drain mit Pull-up: Schreiben von 0 zieht die Leitung nach Masse, 1 laesst sie los;
     * Lesen der Eingangsdaten funktioniert auch im Open-Drain-Betrieb. */
    GPIO_InitTypeDef g = {0};
    g.Pin   = DHT_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_OD;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT_PORT, &g);
    HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_SET);

    s_ready = 1;
    s_last_ms = HAL_GetTick() - MIN_INTERVAL_MS;   /* erste Messung sofort moeglich (Sensor ist beim Start bereit) */
    return 0;
}

int bsp_dht_read(float *temp_c, float *hum_pct)
{
    if (!s_ready) {
        return DHT_ERR_NO_RESPONSE;
    }
    const int sched = (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
    if (sched) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
    }

    const uint32_t since = HAL_GetTick() - s_last_ms;
    if (since < MIN_INTERVAL_MS) {
        delay_ms(MIN_INTERVAL_MS - since);
    }

    /* Startsignal: Leitung nach Masse */
    DHT_PORT->BSRR = (uint32_t)DHT_PIN << 16;
    delay_ms(START_LOW_MS);

    /* Loslassen und Telegramm erfassen; Interrupts mit FreeRTOS-Prioritaet duerfen die Messung nicht stoeren */
    const dht_io_t io = { io_pin, io_cycles, SystemCoreClock / 1000000U };

    uint8_t bytes[5];
    taskENTER_CRITICAL();
    DHT_PORT->BSRR = DHT_PIN;
    int rc = dht_capture(&io, bytes);
    taskEXIT_CRITICAL();
    s_last_ms = HAL_GetTick();

    if (rc == DHT_OK) {
        float t = 0.0f, h = 0.0f;
        rc = dht_decode(bytes, BSP_DHT_TYPE, &t, &h);
        if (rc == DHT_OK) {
            *temp_c = t;
            *hum_pct = h;
        }
    }
    if (sched) {
        xSemaphoreGive(s_mutex);
    }
    return rc;
}

const char *bsp_dht_error_text(int rc)
{
    switch (rc) {
    case DHT_OK:              return "ok";
    case DHT_ERR_NO_RESPONSE: return "keine Antwort (Verdrahtung?)";
    case DHT_ERR_TIMEOUT:     return "Signal abgebrochen";
    case DHT_ERR_CHECKSUM:    return "Pruefsumme falsch";
    case DHT_ERR_RANGE:       return "Wert ausser Bereich";
    default:                  return "Fehler";
    }
}
