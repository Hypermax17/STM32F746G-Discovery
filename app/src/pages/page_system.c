/* Seite SYSTEM: Peripherie-Status, Chiptemperatur/VDDA, Tasks als Live-Tabelle. */
#include "pages.h"
#include "ui_widgets.h"
#include "sysinfo.h"
#include "audio_task.h"
#include "bsp_util.h"

#include <stdio.h>
#include <string.h>

#define NPERIPH 6U

static struct {
    lv_obj_t *dot[NPERIPH];
    lv_obj_t *detail[NPERIPH];
    lv_obj_t *opt;
    lv_obj_t *temp, *vdda;
    lv_obj_t *summary;
    lv_obj_t *table;
    const ui_design_t *d;
} w;

static const char *const k_names[NPERIPH] = { "Display", "SDRAM", "Touch", "QSPI", "Audio", "Taster" };

static void beep_cb(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        audio_task_beep(1);
    } else {
        audio_task_beep(0);
    }
}

lv_obj_t *page_system_create(const ui_design_t *d, unsigned idx)
{
    memset(&w, 0, sizeof w);
    w.d = d;
    ui_frame_t f = ui_frame_create(d, idx);
    lv_obj_t *c = f.content;

    /* ---- linke Spalte: Peripherie ---- */
    const int32_t lw = 176;
    const int32_t rowh = (int32_t)lv_font_get_line_height(d->font_small) + 6;
    lv_obj_t *card = ui_card(c, d, 8, 4, lw, (int32_t)NPERIPH * rowh + 12);
    for (unsigned i = 0; i < NPERIPH; i++) {
        const int32_t y = 6 + (int32_t)i * rowh;
        w.dot[i] = ui_dot(card, 8, y + (rowh - 10) / 2, 10, d->text_dim);
        lv_obj_t *n = ui_label(card, d, UI_TEXT_SMALL, k_names[i]);
        lv_obj_set_pos(n, 24, y + 2);
        w.detail[i] = ui_label(card, d, UI_TEXT_SMALL_DIM, "");
        lv_obj_set_pos(w.detail[i], 24 + 64, y + 2);
        lv_obj_set_width(w.detail[i], lw - 24 - 64 - 4);
    }
    w.opt = ui_label(c, d, UI_TEXT_SMALL_DIM, "");
    lv_obj_set_pos(w.opt, 10, 4 + (int32_t)NPERIPH * rowh + 12 + 4);
    lv_obj_set_width(w.opt, lw);

    /* Messwert-Kacheln unten */
    const int32_t tw = (lw - 8) / 2;
    const int32_t ty = f.h - 52;
    lv_obj_t *t1 = ui_card(c, d, 8, ty, tw, 44);
    ui_label(t1, d, UI_TEXT_SMALL_DIM, "CPU-Temp");
    w.temp = ui_label(t1, d, UI_TEXT_ACCENT, "-");
    lv_obj_set_pos(w.temp, 0, 20);
    lv_obj_set_pos(lv_obj_get_child(t1, 0), 0, 0);
    lv_obj_t *t2 = ui_card(c, d, 8 + tw + 8, ty, tw, 44);
    ui_label(t2, d, UI_TEXT_SMALL_DIM, "VDDA");
    w.vdda = ui_label(t2, d, UI_TEXT_WARN, "-");
    lv_obj_set_pos(w.vdda, 0, 20);
    lv_obj_set_style_pad_all(t1, 6, 0);
    lv_obj_set_style_pad_all(t2, 6, 0);

    /* ---- rechte Spalte: Tasks ---- */
    const int32_t rx = 192, rw = 280;
    const int32_t hh = (int32_t)lv_font_get_line_height(d->font_small);
    const int32_t trow = ui_table_row_h(d);
    const int32_t sum_y = 28;
    const int32_t hdr_y = sum_y + hh + 2;
    const int32_t tab_y = hdr_y + hh + 2;
    const int32_t card_h = tab_y + (int32_t)SYSINFO_MAX_TASKS * trow + 8;
    lv_obj_t *rc = ui_card(c, d, rx, 4, rw, card_h);

    lv_obj_t *title = ui_label(rc, d, UI_TEXT_TITLE, "Tasks");
    lv_obj_set_pos(title, 8, 3);
    lv_obj_t *beep = ui_button(rc, d, "Test-Ton", rw - 8 - 84, 2, 84, 22);
    lv_obj_add_event_cb(beep, beep_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(beep, beep_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(beep, beep_cb, LV_EVENT_PRESS_LOST, NULL);

    w.summary = ui_label(rc, d, UI_TEXT_SMALL_DIM, "");
    lv_obj_set_pos(w.summary, 8, sum_y);
    lv_obj_set_width(w.summary, rw - 16);

    static const int32_t col_w[4] = { 112, 32, 56, 52 };
    static const char *const hdr[4] = { "Name", "Pr", "Stack", "CPU" };
    int32_t x = 8;
    for (unsigned i = 0; i < 4; i++) {
        lv_obj_t *h = ui_label(rc, d, UI_TEXT_SMALL_DIM, hdr[i]);
        lv_obj_set_pos(h, x + 2, hdr_y);
        x += col_w[i];
    }
    w.table = ui_table(rc, d, 8, tab_y, SYSINFO_MAX_TASKS, 4, col_w);

    page_system_update();
    return f.screen;
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
    char buf[48];
    snprintf(buf, sizeof buf, "%ux%u", 480U, 272U);
    const char *detail_text[NPERIPH];
    char sd[8], qs[8];
    snprintf(sd, sizeof sd, "%u MB", (unsigned)s.sdram_mb);
    snprintf(qs, sizeof qs, "%u MB", (unsigned)s.qspi_mb);
    detail_text[0] = buf; detail_text[1] = sd; detail_text[2] = "FT5336"; detail_text[3] = qs;
    detail_text[4] = "WM8994"; detail_text[5] = "B1";
    for (unsigned i = 0; i < NPERIPH; i++) {
        lv_obj_set_style_bg_color(w.dot[i], ui_col(ok[i] ? d->ok : d->bad), 0);
        lv_label_set_text(w.detail[i], ok[i] ? detail_text[i] : "FEHLT");
        ui_label_set_kind_color(w.detail[i], d, ok[i] ? UI_TEXT_SMALL_DIM : UI_TEXT_BAD);
    }

    char opt[40] = "";
    const struct { const char *n; uint8_t on; } mods[4] = {
        { "SD", s.has_sd }, { "USB", s.has_usb }, { "ETH", s.has_eth }, { "CAM", s.has_cam } };
    size_t len = 0;
    for (unsigned i = 0; i < 4; i++) {
        if (mods[i].on) {
            len += (size_t)snprintf(opt + len, sizeof opt - len, "%s ", mods[i].n);
        }
    }
    lv_label_set_text(w.opt, len ? opt : "Optional: nur Geruest");

    if (s.sensor_ok) {
        char t[16], v[16];
        bsp_format_float(t, sizeof t, s.cpu_temp_c, 1);
        bsp_format_float(v, sizeof v, s.vdda_v, 2);
        snprintf(buf, sizeof buf, "%s C", t);
        lv_label_set_text(w.temp, buf);
        snprintf(buf, sizeof buf, "%s V", v);
        lv_label_set_text(w.vdda, buf);
    } else {
        lv_label_set_text(w.temp, "n/a");
        lv_label_set_text(w.vdda, "n/a");
    }

    snprintf(buf, sizeof buf, "CPU %u%%  Heap %luK frei  up %lus", (unsigned)s.cpu_percent,
             (unsigned long)(s.heap_free / 1024U), (unsigned long)s.uptime_s);
    lv_label_set_text(w.summary, buf);

    for (unsigned r = 0; r < SYSINFO_MAX_TASKS; r++) {
        if (r < s.task_count) {
            const sysinfo_task_t *t = &s.task[r];
            lv_table_set_cell_value(w.table, r, 0, t->name);
            snprintf(buf, sizeof buf, "%u", (unsigned)t->prio);
            lv_table_set_cell_value(w.table, r, 1, buf);
            snprintf(buf, sizeof buf, "%u", (unsigned)t->stack_free_words);
            lv_table_set_cell_value(w.table, r, 2, buf);
            snprintf(buf, sizeof buf, "%u%%", (unsigned)t->cpu_percent);
            lv_table_set_cell_value(w.table, r, 3, buf);
        } else {
            for (unsigned c = 0; c < 4; c++) {
                lv_table_set_cell_value(w.table, r, c, "");
            }
        }
    }
}
