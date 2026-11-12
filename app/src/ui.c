#include "ui.h"
#include "ui_render.h"
#include "audio_task.h"
#include "bsp.h"

#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define UI_STACK_WORDS   1024
#define UI_PRIORITY      2
#define UI_QUEUE_LEN     8
#define SYSTEM_REFRESH_MS 500U

typedef enum { MSG_TOUCH, MSG_NEXT_PAGE } msg_type_t;

typedef struct {
    msg_type_t type;
    bsp_touch_state_t touch;
} ui_msg_t;

static QueueHandle_t s_queue;
static ui_page_t s_page = UI_PAGE_LOG;     /* beim Start sind die Boot-Meldungen sichtbar */

/* Touch-Seite: letzter Punkt je Finger-Slot */
static uint8_t  s_has_prev[BSP_TOUCH_MAX_POINTS];
static uint16_t s_prev_x[BSP_TOUCH_MAX_POINTS], s_prev_y[BSP_TOUCH_MAX_POINTS];
static uint8_t  s_prev_count;
static uint8_t  s_ignore_until_release;

/* ---- Systemdaten sammeln -------------------------------------------------- */

#define MAX_SAMPLE_TASKS 12U

static struct { UBaseType_t number; uint32_t runtime; } s_prev_rt[MAX_SAMPLE_TASKS];
static uint32_t s_prev_total;

static int task_before(const ui_task_info_t *a, const ui_task_info_t *b)
{
    if (a->prio != b->prio) return a->prio > b->prio;
    return strcmp(a->name, b->name) < 0;
}

static void gather_system(ui_system_t *s)
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
    ui_task_info_t list[MAX_SAMPLE_TASKS];
    unsigned cnt = 0;
    for (UBaseType_t i = 0; i < n && i < MAX_SAMPLE_TASKS; i++) {
        uint32_t prev = 0;
        for (unsigned k = 0; k < MAX_SAMPLE_TASKS; k++) {
            if (s_prev_rt[k].number == st[i].xTaskNumber) { prev = s_prev_rt[k].runtime; break; }
        }
        uint32_t pct = (uint32_t)(((uint64_t)(st[i].ulRunTimeCounter - prev) * 100U) / dtotal);
        if (pct > 100U) pct = 100U;
        if (strcmp(st[i].pcTaskName, "IDLE") == 0) idle_pct = pct;

        ui_task_info_t *t = &list[cnt++];
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
        ui_task_info_t key = list[i];
        unsigned j = i;
        while (j > 0 && task_before(&key, &list[j - 1U])) { list[j] = list[j - 1U]; j--; }
        list[j] = key;
    }
    s->task_count = (uint8_t)((cnt < UI_MAX_TASKS) ? cnt : UI_MAX_TASKS);
    memcpy(s->task, list, s->task_count * sizeof list[0]);
}

static void refresh_system(void)
{
    ui_system_t s;
    gather_system(&s);
    ui_render_system(&s);
}

/* ---- Seitenwechsel ---------------------------------------------------------- */

static void reset_touch_state(void)
{
    memset(s_has_prev, 0, sizeof s_has_prev);
}

static void switch_page(ui_page_t page)
{
    if (s_page == UI_PAGE_LOG && page != UI_PAGE_LOG) {
        bsp_console_set_visible(0);          /* ab jetzt darf die UI den Inhaltsbereich beschreiben */
    }
    s_page = page;
    audio_task_set_touch(0, 0, 0);
    reset_touch_state();
    ui_render_tabs(page);

    switch (page) {
    case UI_PAGE_LOG:
        bsp_console_set_visible(1);          /* zeichnet den Log-Bereich neu */
        break;
    case UI_PAGE_TOUCH:
        ui_render_touch_background();
        break;
    case UI_PAGE_SYSTEM:
    default:
        refresh_system();
        break;
    }
}

/* ---- Touch ------------------------------------------------------------------ */

static void handle_touch(const bsp_touch_state_t *t)
{
    const int down_edge = (t->count > 0U && s_prev_count == 0U);

    if (t->count == 0U) {
        s_ignore_until_release = 0;
        reset_touch_state();
    } else if (down_edge && t->point[0].y < UI_CONTENT_Y) {
        /* Tippen auf die Tab-Leiste */
        const ui_page_t tab = (ui_page_t)(t->point[0].x / (BSP_LCD_WIDTH / UI_TAB_COUNT));
        s_ignore_until_release = 1;
        if (tab == s_page && tab == UI_PAGE_TOUCH) {
            ui_render_touch_background();    /* erneutes Tippen auf TOUCH loescht die Zeichnung */
            reset_touch_state();
        } else if (tab < UI_TAB_COUNT && tab != s_page) {
            switch_page(tab);
        }
    }

    const int active = (t->count > 0U) && !s_ignore_until_release;

    if (s_page == UI_PAGE_TOUCH && active) {
        for (unsigned i = 0; i < t->count && i < BSP_TOUCH_MAX_POINTS; i++) {
            const unsigned x = t->point[i].x, y = t->point[i].y;
            /* nur im Inhaltsbereich zeichnen (nicht in Tab-Leiste/Statuszeile) */
            if (y >= UI_CONTENT_Y + 6U && y < UI_CONTENT_Y + UI_CONTENT_H - 6U) {
                ui_render_touch_point(i, x, y, s_has_prev[i], s_prev_x[i], s_prev_y[i]);
                s_has_prev[i] = 1;
                s_prev_x[i] = (uint16_t)x;
                s_prev_y[i] = (uint16_t)y;
            } else {
                s_has_prev[i] = 0;
            }
        }
        for (unsigned i = t->count; i < BSP_TOUCH_MAX_POINTS; i++) s_has_prev[i] = 0;
    }

    /* Ton nur auf der Touch-Seite */
    if (s_page == UI_PAGE_TOUCH && active && t->point[0].y >= UI_CONTENT_Y) {
        audio_task_set_touch(1, t->point[0].x, t->point[0].y);
    } else {
        audio_task_set_touch(0, 0, 0);
    }
    s_prev_count = t->count;
}

/* ---- Task --------------------------------------------------------------------- */

static void ui_task(void *arg)
{
    (void)arg;
    ui_render_tabs(s_page);
    TickType_t last_refresh = xTaskGetTickCount();

    for (;;) {
        ui_msg_t m;
        if (xQueueReceive(s_queue, &m, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (m.type == MSG_NEXT_PAGE) {
                switch_page((ui_page_t)(((unsigned)s_page + 1U) % UI_TAB_COUNT));
            } else {
                handle_touch(&m.touch);
            }
        }
        if (s_page == UI_PAGE_SYSTEM &&
            (TickType_t)(xTaskGetTickCount() - last_refresh) >= pdMS_TO_TICKS(SYSTEM_REFRESH_MS)) {
            last_refresh = xTaskGetTickCount();
            refresh_system();
        }
    }
}

void ui_post_touch(const bsp_touch_state_t *state)
{
    ui_msg_t m = { .type = MSG_TOUCH, .touch = *state };
    (void)xQueueSend(s_queue, &m, 0);
}

void ui_next_page(void)
{
    ui_msg_t m = { .type = MSG_NEXT_PAGE };
    (void)xQueueSend(s_queue, &m, 0);
}

void ui_start(void)
{
    s_queue = xQueueCreate(UI_QUEUE_LEN, sizeof(ui_msg_t));
    if (s_queue == NULL ||
        xTaskCreate(ui_task, "ui", UI_STACK_WORDS, NULL, UI_PRIORITY, NULL) != pdPASS) {
        bsp_panic("Task 'ui' nicht erzeugt");
    }
}
