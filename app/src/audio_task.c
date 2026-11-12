#include "audio_task.h"
#include "bsp.h"

#include <math.h>

#include "FreeRTOS.h"
#include "task.h"

#define AUDIO_STACK_WORDS  1024
#define AUDIO_PRIORITY     3

#define TABLE_BITS   10U
#define TABLE_SIZE   (1U << TABLE_BITS)

#define BEEP_HZ      440U
#define BEEP_AMP     6000
#define BEEP_MS      300U
#define AMP_STEP     12           /* Aenderung je Sample (verhindert Knacken) */

static int16_t s_sine[TABLE_SIZE];
static volatile uint32_t s_freq_hz = BEEP_HZ;
static volatile int32_t  s_amp_target;

void audio_task_beep(int on)
{
    s_freq_hz = BEEP_HZ;
    s_amp_target = on ? BEEP_AMP : 0;
}

static void build_table(void)
{
    for (uint32_t i = 0; i < TABLE_SIZE; i++) {
        s_sine[i] = (int16_t)(32767.0f * sinf(6.2831853f * (float)i / (float)TABLE_SIZE));
    }
}

static void audio_task(void *arg)
{
    (void)arg;
    build_table();

    uint16_t id = 0;
    int rc = bsp_audio_init(&id);
    if (rc != 0) {
        BSP_LOGE("Audio: Init Fehler %d (ID %04X)", rc, (unsigned)id);
        vTaskDelete(NULL);
    }
    BSP_LOGI("Audio: WM8994 (ID %04X) %u Hz", (unsigned)id, (unsigned)BSP_AUDIO_SAMPLE_RATE);
    if (bsp_audio_start() != 0) {
        BSP_LOGE("Audio: Start fehlgeschlagen");
        vTaskDelete(NULL);
    }
    BSP_LOGI("Audio: Ton %u Hz", (unsigned)BEEP_HZ);

    uint32_t phase = 0;
    int32_t amp = 0;
    uint32_t beep_left = (BSP_AUDIO_SAMPLE_RATE / 1000U) * BEEP_MS;
    uint32_t last_underruns = 0, last_errors = 0;

    for (;;) {
        int16_t *buf;
        if (bsp_audio_next_buffer(&buf, 100) != 0) {
            continue;
        }

        uint32_t freq = beep_left ? BEEP_HZ : s_freq_hz;
        int32_t target = beep_left ? BEEP_AMP : s_amp_target;
        uint32_t inc = (uint32_t)(((uint64_t)freq << 32) / BSP_AUDIO_SAMPLE_RATE);

        for (uint32_t i = 0; i < BSP_AUDIO_HALF_FRAMES; i++) {
            if (amp < target)      { amp += AMP_STEP; if (amp > target) amp = target; }
            else if (amp > target) { amp -= AMP_STEP; if (amp < target) amp = target; }
            int16_t s = (int16_t)(((int32_t)s_sine[phase >> (32U - TABLE_BITS)] * amp) >> 15);
            buf[2U * i]      = s;       /* links */
            buf[2U * i + 1U] = s;       /* rechts */
            phase += inc;
        }
        bsp_audio_release_buffer(buf);

        if (beep_left) {
            beep_left = (beep_left > BSP_AUDIO_HALF_FRAMES) ? beep_left - BSP_AUDIO_HALF_FRAMES : 0;
        }

        uint32_t u = bsp_audio_underruns();
        uint32_t e = bsp_audio_errors();
        if (u != last_underruns) { last_underruns = u; BSP_LOGW("Audio: Unterbrechung (%lu)", (unsigned long)u); }
        if (e != last_errors)    { last_errors = e;    BSP_LOGW("Audio: SAI-Fehler (%lu)", (unsigned long)e); }
    }
}

void audio_task_start(void)
{
    if (xTaskCreate(audio_task, "audio", AUDIO_STACK_WORDS, NULL, AUDIO_PRIORITY, NULL) != pdPASS) {
        bsp_panic("Task 'audio' nicht erzeugt");
    }
}
