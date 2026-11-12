/* Seite SYSTEM (Cockpit-Anzeige): drei runde Anzeigen (CPU-Last, CPU-Temperatur, VDDA) in Glas-Karten,
 * darunter Status der Peripherie, Zusammenfassung und die vier aktivsten Tasks als Live-Tabelle. */
#include "pages.h"
#include "ui_widgets.h"
#include "sysinfo.h"
#include "audio_task.h"
#include "bsp_util.h"

#include <stdio.h>
#include <string.h>

#define NPERIPH 6U
#define TASK_ROWS 4U

static const char *const k_names[NPERIPH] = { "Display", "SDRAM", "Touch", "QSPI", "Audio", "Taster" };

static struct {
    const ui_design_t *d;
    lv_obj_t *dot[NPERIPH];
    lv_obj_t *summary;
    lv_obj_t *table;
    ui_gauge_t g_cpu, g_temp, g_vdda;
} w;

static void beep_cb(lv_event_t *e)
{
    audio_task_beep(lv_event_get_code(e) == LV_EVENT_PRESSED);
}

lv_obj_t *page_system_create(const ui_design_t *d, unsigned idx)
{
    memset(&w, 0, sizeof w);
    w.d = d;
    ui_frame_t f = ui_frame_create(d, idx);
    lv_obj_t *c = f.content;

    /* ---- drei runde Anzeigen, je in einer Karte ---- */
    const int32_t gap = 8;
    const int32_t cw = (f.w - 2 * gap - 2 * gap) / 3;     /* Kartenbreite */
    const int32_t gh = 104;                               /* Kartenhoehe */
    const int32_t gs = 92;                                /* Durchmesser der Anzeige */
    static const char *const titles[3] = { "CPU-Last %", "CPU-Temp C", "VDDA V" };
    ui_gauge_t *g[3] = { &w.g_cpu, &w.g_temp, &w.g_vdda };
    const int32_t lo[3] = { 0, 20, 3000 };
    const int32_t hi[3] = { 100, 90, 3600 };
    const uint32_t col[3] = { d->accent, d->warn, d->ok };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *card = ui_card(c, d, gap + i * (cw + gap), 4, cw, gh);
        *g[i] = ui_gauge(card, d, (cw - gs) / 2 - 1, 5, gs, titles[i], lo[i], hi[i], col[i]);
    }

    /* ---- untere Karte: Status, Zusammenfassung, Tasks ---- */
    const int32_t by = 4 + gh + gap;
    const int32_t bw = f.w - 2 * gap;
    const int32_t hh = (int32_t)lv_font_get_line_height(d->font_small);
    const int32_t trow = ui_table_row_h(d);
    lv_obj_t *card = ui_card(c, d, gap, by, bw, f.h - by - 4);

    const int32_t cell = (bw - 16) / (int32_t)NPERIPH;
    for (unsigned i = 0; i < NPERIPH; i++) {
        w.dot[i] = ui_dot(card, 8 + (int32_t)i * cell, 8 + 3, 9, d->text_dim);
        lv_obj_t *n = ui_label(card, d, UI_TEXT_SMALL, k_names[i]);
        lv_obj_set_pos(n, 8 + (int32_t)i * cell + 13, 8);
    }

    const int32_t sum_y = 8 + hh + 6;
    w.summary = ui_label(card, d, UI_TEXT_SMALL_DIM, "");
    lv_obj_set_pos(w.summary, 8, sum_y + 4);
    lv_obj_set_width(w.summary, bw - 16 - 96);
    lv_obj_t *beep = ui_button(card, d, "Test-Ton", bw - 8 - 84, sum_y, 84, 22);
    lv_obj_add_event_cb(beep, beep_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(beep, beep_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(beep, beep_cb, LV_EVENT_PRESS_LOST, NULL);

    const int32_t hdr_y = sum_y + 26;
    const int32_t tab_y = hdr_y + hh + 2;
    const int32_t col_w[4] = { bw - 16 - 40 - 100 - 100, 40, 100, 100 };
    static const char *const hdr[4] = { "Aktivste Tasks", "Pr", "Stack frei", "CPU" };
    int32_t x = 8;
    for (unsigned i = 0; i < 4; i++) {
        lv_obj_t *h = ui_label(card, d, UI_TEXT_SMALL_DIM, hdr[i]);
        lv_obj_set_pos(h, x + 2, hdr_y);
        x += col_w[i];
    }
    w.table = ui_table(card, d, 8, tab_y, TASK_ROWS, 4, col_w);
    (void)trow;

    page_system_update();
    return f.screen;
}

/* ---- Aktualisierung ------------------------------------------------------------------ */

static void fill_tasks(const sysinfo_t *s)
{
    /* die aktivsten Tasks zuerst (CPU-Anteil absteigend) */
    const sysinfo_task_t *list[SYSINFO_MAX_TASKS];
    const unsigned n = s->task_count;
    for (unsigned i = 0; i < n; i++) list[i] = &s->task[i];
    for (unsigned i = 1; i < n; i++) {
        const sysinfo_task_t *k = list[i];
        unsigned j = i;
        while (j > 0 && list[j - 1U]->cpu_percent < k->cpu_percent) { list[j] = list[j - 1U]; j--; }
        list[j] = k;
    }
    char buf[24];
    for (unsigned r = 0; r < TASK_ROWS; r++) {
        if (r < n) {
            const sysinfo_task_t *t = list[r];
            lv_table_set_cell_value(w.table, r, 0, t->name);
            snprintf(buf, sizeof buf, "%u", (unsigned)t->prio);
            lv_table_set_cell_value(w.table, r, 1, buf);
            snprintf(buf, sizeof buf, "%u", (unsigned)t->stack_free_words);
            lv_table_set_cell_value(w.table, r, 2, buf);
            snprintf(buf, sizeof buf, "%u%%", (unsigned)t->cpu_percent);
            lv_table_set_cell_value(w.table, r, 3, buf);
        } else {
            for (unsigned c = 0; c < 4; c++) lv_table_set_cell_value(w.table, r, c, "");
        }
    }
}

void page_system_update(void)
{
    if (w.table == NULL) {
        return;
    }
    const ui_design_t *d = w.d;
    sysinfo_t s;
    sysinfo_gather(&s);

    const uint8_t ok[NPERIPH] = { 1, s.sdram_mb != 0U, s.touch_ok, s.qspi_ok, s.audio_ok, s.button_ok };
    for (unsigned i = 0; i < NPERIPH; i++) {
        lv_obj_set_style_bg_color(w.dot[i], ui_col(ok[i] ? d->ok : d->bad), 0);
    }

    char buf[48], t[16], v[16];
    snprintf(buf, sizeof buf, "%u%%", (unsigned)s.cpu_percent);
    ui_gauge_set(&w.g_cpu, (int32_t)s.cpu_percent, buf);
    if (s.sensor_ok) {
        bsp_format_float(t, sizeof t, s.cpu_temp_c, 1);
        bsp_format_float(v, sizeof v, s.vdda_v, 2);
        ui_gauge_set(&w.g_temp, (int32_t)(s.cpu_temp_c + 0.5f), t);
        ui_gauge_set(&w.g_vdda, (int32_t)(s.vdda_v * 1000.0f + 0.5f), v);
    } else {
        ui_gauge_set(&w.g_temp, 20, "n/a");
        ui_gauge_set(&w.g_vdda, 3000, "n/a");
    }

    snprintf(buf, sizeof buf, "Heap %luK frei  up %lus", (unsigned long)(s.heap_free / 1024U),
             (unsigned long)s.uptime_s);
    lv_label_set_text(w.summary, buf);
    fill_tasks(&s);
}
