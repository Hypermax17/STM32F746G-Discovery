#include "sysinfo.h"
#include "bsp.h"

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#define MAX_SAMPLE_TASKS 12U

static struct { UBaseType_t number; uint32_t runtime; } s_prev_rt[MAX_SAMPLE_TASKS];
static uint32_t s_prev_total;

static int task_before(const sysinfo_task_t *a, const sysinfo_task_t *b)
{
    if (a->prio != b->prio) return a->prio > b->prio;
    return strcmp(a->name, b->name) < 0;
}

void sysinfo_gather(sysinfo_t *s)
{
    memset(s, 0, sizeof *s);
    s->touch_ok  = (uint8_t)bsp_touch_available();
    s->qspi_ok   = (uint8_t)bsp_qspi_available();
    s->audio_ok  = (uint8_t)bsp_audio_is_ready();
    s->button_ok = 1;
    s->sdram_mb  = (uint8_t)(BSP_SDRAM_SIZE >> 20);
    s->qspi_mb   = (uint8_t)(BSP_QSPI_SIZE >> 20);
    s->has_sd = BSP_HAS_SD;  s->has_usb = BSP_HAS_USB;  s->has_eth = BSP_HAS_ETH;  s->has_cam = BSP_HAS_CAMERA;

    /* Fertige physikalische Werte aus dem BSP (Grad Celsius, Volt) */
    s->sensor_ok = (bsp_cpu_temperature(&s->cpu_temp_c) == 0) && (bsp_supply_voltage(&s->vdda_v) == 0);

    s->uptime_s   = xTaskGetTickCount() / configTICK_RATE_HZ;
    s->heap_free  = (uint32_t)xPortGetFreeHeapSize();
    s->heap_min   = (uint32_t)xPortGetMinimumEverFreeHeapSize();
    s->heap_total = (uint32_t)configTOTAL_HEAP_SIZE;

    TaskStatus_t st[MAX_SAMPLE_TASKS];
    uint32_t total = 0;
    UBaseType_t n = uxTaskGetSystemState(st, MAX_SAMPLE_TASKS, &total);
    uint32_t dtotal = total - s_prev_total;     /* vorzeichenlose Differenz: wrap-sicher */
    if (dtotal == 0U) dtotal = 1U;

    uint32_t idle_pct = 0;
    sysinfo_task_t list[MAX_SAMPLE_TASKS];
    unsigned cnt = 0;
    for (UBaseType_t i = 0; i < n && i < MAX_SAMPLE_TASKS; i++) {
        uint32_t prev = 0;
        for (unsigned k = 0; k < MAX_SAMPLE_TASKS; k++) {
            if (s_prev_rt[k].number == st[i].xTaskNumber) { prev = s_prev_rt[k].runtime; break; }
        }
        uint32_t pct = (uint32_t)(((uint64_t)(st[i].ulRunTimeCounter - prev) * 100U) / dtotal);
        if (pct > 100U) pct = 100U;
        if (strcmp(st[i].pcTaskName, "IDLE") == 0) idle_pct = pct;

        sysinfo_task_t *t = &list[cnt++];
        strncpy(t->name, st[i].pcTaskName, sizeof t->name - 1U);
        t->name[sizeof t->name - 1U] = '\0';
        t->prio = (uint8_t)st[i].uxCurrentPriority;
        t->stack_free_words = (uint16_t)st[i].usStackHighWaterMark;
        t->cpu_percent = (uint8_t)pct;
    }
    for (UBaseType_t i = 0; i < n && i < MAX_SAMPLE_TASKS; i++) {
        s_prev_rt[i].number = st[i].xTaskNumber;
        s_prev_rt[i].runtime = st[i].ulRunTimeCounter;
    }
    s_prev_total = total;
    s->cpu_percent = (uint8_t)(100U - ((idle_pct > 100U) ? 100U : idle_pct));

    /* nach Prioritaet (absteigend), dann Name sortieren */
    for (unsigned i = 1; i < cnt; i++) {
        sysinfo_task_t key = list[i];
        unsigned j = i;
        while (j > 0 && task_before(&key, &list[j - 1U])) { list[j] = list[j - 1U]; j--; }
        list[j] = key;
    }
    s->task_count = (uint8_t)((cnt < SYSINFO_MAX_TASKS) ? cnt : SYSINFO_MAX_TASKS);
    memcpy(s->task, list, s->task_count * sizeof list[0]);
}

