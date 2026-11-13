#include "bsp_dht.h"
#include "stm32f7xx_hal.h"

#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define START_LOW_MS      22U               /* Startsignal: mindestens 18 ms Low */
#define MIN_INTERVAL_MS   1100U             /* Abstand zwischen zwei Messungen desselben Sensors */

void bsp_runtime_counter_init(void);        /* bsp_system.c: schaltet den DWT-Zyklenzaehler ein */

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    const char   *name;                     /* Arduino-Bezeichnung */
    uint32_t      last_ms;                  /* Zeitpunkt der letzten Messung */
} dht_sensor_t;

/* Datenpins (Arduino-Anschluss): Sensor 1 = D4 (PG7), Sensor 2 = D2 (PG6) */
static dht_sensor_t s_sensor[BSP_DHT_COUNT] = {
    { GPIOG, GPIO_PIN_7, "D4", 0 },
    { GPIOG, GPIO_PIN_6, "D2", 0 },
};

#define GLITCH_US 1U                        /* Stoerspitzen auf der Leitung unter 1 us ausblenden */

static struct { uint8_t raw[5]; unsigned bits; } s_diag[BSP_DHT_COUNT];
static dht_hold_t s_hold[BSP_DHT_COUNT];
static bsp_dht_stats_t s_stats[BSP_DHT_COUNT];
static SemaphoreHandle_t s_mutex;
static int s_ready;

static int io_pin(void *ctx)
{
    const dht_sensor_t *s = ctx;
    return (s->port->IDR & s->pin) != 0U;
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
    for (unsigned i = 0; i < BSP_DHT_COUNT; i++) {
        /* Open-Drain mit Pull-up: Schreiben von 0 zieht die Leitung nach Masse, 1 laesst sie los;
         * Lesen der Eingangsdaten funktioniert auch im Open-Drain-Betrieb. */
        GPIO_InitTypeDef g = {0};
        g.Pin   = s_sensor[i].pin;
        g.Mode  = GPIO_MODE_OUTPUT_OD;
        g.Pull  = GPIO_PULLUP;
        g.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(s_sensor[i].port, &g);
        HAL_GPIO_WritePin(s_sensor[i].port, s_sensor[i].pin, GPIO_PIN_SET);
        s_sensor[i].last_ms = HAL_GetTick() - MIN_INTERVAL_MS;   /* erste Messung sofort moeglich */
    }
    s_ready = 1;
    return 0;
}

const char *bsp_dht_pin_name(unsigned idx)
{
    return (idx < BSP_DHT_COUNT) ? s_sensor[idx].name : "?";
}

/* Eine Einzelmessung (der Aufrufer hat den Mutex). */
static int measure_once(unsigned idx, float *temp_c, float *hum_pct)
{
    dht_sensor_t *sn = &s_sensor[idx];

    const uint32_t since = HAL_GetTick() - sn->last_ms;
    if (since < MIN_INTERVAL_MS) {
        delay_ms(MIN_INTERVAL_MS - since);
    }

    /* Startsignal: Leitung nach Masse */
    sn->port->BSRR = (uint32_t)sn->pin << 16;
    delay_ms(START_LOW_MS);

    /* Loslassen und Telegramm erfassen. Loslassen und Erfassen liegen im selben kritischen Abschnitt: Interrupts mit
     * FreeRTOS-Prioritaet (>= 5, Audio, Taster, LED-DMA) und Task-Wechsel koennen die Messung nicht unterbrechen.
     * Offen bleibt nur der HAL-Tick (TIM6, Prioritaet 1, ca. 0,5 us je Millisekunde); er verschiebt die Pegelwechsel
     * um weniger als 1 us, die Bitschwelle liegt bei 42 us. */
    const dht_io_t io = { io_pin, io_cycles, SystemCoreClock / 1000000U, sn, GLITCH_US };
    uint8_t bytes[5];
    unsigned bits = 0;
    taskENTER_CRITICAL();
    sn->port->BSRR = sn->pin;
    int rc = dht_capture(&io, bytes, &bits);
    taskEXIT_CRITICAL();
    sn->last_ms = HAL_GetTick();
    for (unsigned i = 0; i < 5; i++) {
        s_diag[idx].raw[i] = bytes[i];
    }
    s_diag[idx].bits = bits;

    if (rc == DHT_OK) {
        rc = dht_decode(bytes, BSP_DHT_TYPE, temp_c, hum_pct);
    }
    return rc;
}

int bsp_dht_read(unsigned idx, float *temp_c, float *hum_pct)
{
    if (!s_ready || idx >= BSP_DHT_COUNT) {
        return DHT_ERR_NO_RESPONSE;
    }
    const int sched = (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
    if (sched) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
    }

    float t = 0.0f, h = 0.0f;
    int rc = DHT_ERR_NO_RESPONSE;
    for (int attempt = 0; attempt < BSP_DHT_ATTEMPTS; attempt++) {
        rc = measure_once(idx, &t, &h);
        if (rc == DHT_OK) {
            break;
        }
        s_stats[idx].failed_attempts++;
        s_stats[idx].last_error = rc;
        /* Kein Sensor angeschlossen (nie eine Antwort): nicht wiederholen. Wenn der Sensor sonst antwortet, kann auch
         * "keine Antwort" eine Stoerung sein. */
        if (rc == DHT_ERR_NO_RESPONSE && !s_hold[idx].valid) {
            break;
        }
    }

    rc = dht_hold_update(&s_hold[idx], rc, &t, &h, BSP_DHT_HOLD_READS);
    s_stats[idx].reads++;
    if (rc == DHT_HELD) {
        s_stats[idx].held++;
    } else if (rc < 0) {
        s_stats[idx].lost++;
    }
    if (rc >= 0) {
        *temp_c = t;
        *hum_pct = h;
    }
    if (sched) {
        xSemaphoreGive(s_mutex);
    }
    return rc;
}

void bsp_dht_stats(unsigned idx, bsp_dht_stats_t *out)
{
    if (idx < BSP_DHT_COUNT) {
        *out = s_stats[idx];
    } else {
        memset(out, 0, sizeof *out);
    }
}

void bsp_dht_last_diag(unsigned idx, uint8_t raw[5], unsigned *bits_done)
{
    for (unsigned i = 0; i < 5; i++) {
        raw[i] = (idx < BSP_DHT_COUNT) ? s_diag[idx].raw[i] : 0U;
    }
    if (bits_done != NULL) {
        *bits_done = (idx < BSP_DHT_COUNT) ? s_diag[idx].bits : 0U;
    }
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
