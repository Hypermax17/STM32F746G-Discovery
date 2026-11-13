#include "ui_port.h"
#include "ui_touch_pack.h"
#include "ui_pages.h"
#include "bsp.h"

#include "lvgl.h"

#include "FreeRTOS.h"
#include "task.h"

#define UI_TASK_STACK_WORDS 3072
#define UI_TASK_PRIORITY    2

static void (*s_app_init)(void);

/* ---- Touch: geteilter Zustand (32 Bit, damit Lesen/Schreiben atomar ist) ----------------------- */

static volatile uint32_t s_touch_packed;       /* Bit 31 = beruehrt, Bit 16..26 = y, Bit 0..10 = x */
static volatile uint8_t  s_next_page_request;

void ui_port_touch_update(const bsp_touch_state_t *st)
{
    s_touch_packed = ui_touch_pack(s_touch_packed, st->count > 0U,
                                   st->count > 0U ? st->point[0].x : 0U, st->count > 0U ? st->point[0].y : 0U);
}

void ui_port_request_next_page(void)
{
    s_next_page_request = 1;
}

static void indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    int pressed;
    ui_touch_unpack(s_touch_packed, &data->point.x, &data->point.y, &pressed);
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

/* ---- Display: DIRECT-Modus mit zwei Vollbild-Puffern, Umschalten im Austastintervall ---------- */

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    (void)area;
    if (lv_display_flush_is_last(disp)) {
        /* px_map liegt in einem der beiden Puffer -> daran den Puffer erkennen */
        const uint32_t p = (uint32_t)(uintptr_t)px_map;
        const uint32_t fb = (p >= BSP_LCD_FB1_ADDR) ? BSP_LCD_FB1_ADDR : BSP_LCD_FB0_ADDR;
        (void)bsp_lcd_show_buffer(fb);
    }
    lv_display_flush_ready(disp);
}

/* ---- Log / Assert ------------------------------------------------------------------------------ */

static void lv_log_cb(lv_log_level_t level, const char *buf)
{
    if (level >= LV_LOG_LEVEL_ERROR) {
        BSP_LOGE("LVGL: %s", buf);
    } else {
        BSP_LOGW("LVGL: %s", buf);
    }
}

void ui_assert_halt(void)
{
    bsp_panic("LVGL: Assert (Speicher/NULL)");
}

/* ---- Zeitgeber: Anfrage "naechste Seite" aus anderen Tasks ---------------------------------------- */

static void request_poll_cb(lv_timer_t *t)
{
    (void)t;
    if (s_next_page_request) {
        s_next_page_request = 0;
        ui_pages_next();
    }
}

/* ---- UI-Task ------------------------------------------------------------------------------------- */

static uint32_t tick_cb(void)
{
    return (uint32_t)(xTaskGetTickCount() * (1000U / configTICK_RATE_HZ));
}

static void ui_task(void *arg)
{
    (void)arg;
    lv_init();
    lv_tick_set_cb(tick_cb);
    lv_log_register_print_cb(lv_log_cb);

    lv_display_t *disp = lv_display_create(BSP_LCD_WIDTH, BSP_LCD_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, (void *)BSP_LCD_FB0_ADDR, (void *)BSP_LCD_FB1_ADDR,
                           BSP_LCD_WIDTH * BSP_LCD_HEIGHT * 2U, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(disp, flush_cb);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, indev_read_cb);

    lv_timer_create(request_poll_cb, 30, NULL);

    /* Ab hier zeichnet nur noch LVGL; die Konsole speichert Meldungen weiter im Ringpuffer */
    bsp_console_set_visible(0);

    if (s_app_init != NULL) {
        s_app_init();
    }

    for (;;) {
        uint32_t wait = lv_timer_handler();
        if (wait < 5U)  wait = 5U;
        if (wait > 30U) wait = 30U;
        vTaskDelay(pdMS_TO_TICKS(wait));
    }
}

void ui_port_start(void (*app_init)(void))
{
    s_app_init = app_init;
    if (xTaskCreate(ui_task, "ui", UI_TASK_STACK_WORDS, NULL, UI_TASK_PRIORITY, NULL) != pdPASS) {
        bsp_panic("Task 'ui' nicht erzeugt");
    }
}
