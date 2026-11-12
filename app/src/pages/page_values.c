/* Seite MESSWERTE: Live-Diagramm und Live-Tabelle von Chiptemperatur (Grad Celsius) und VDDA (Volt),
 * dazu Schaltflaechen Pause / Loeschen. Die Daten liegen im data_hub, die Seite kann beliebig neu aufgebaut werden. */
#include "pages.h"
#include "ui_widgets.h"
#include "data_hub.h"
#include "bsp_util.h"

#include <stdio.h>
#include <string.h>

#define POINTS         DATA_HUB_CAPACITY
#define TABLE_ROWS_MAX 8U

/* Zustand (bleibt beim Neuaufbau der Seite erhalten) */
static int      s_paused;
static uint32_t s_base;          /* Messwerte vor dieser Nummer gelten als geloescht */
static uint32_t s_next;          /* naechster noch nicht ins Diagramm uebernommener Messwert */

static struct {
    lv_obj_t *chart;
    lv_chart_series_t *ser_t, *ser_v;
    lv_obj_t *big_t, *big_v;
    lv_obj_t *t_max, *t_min, *v_max, *v_min;
    lv_obj_t *table;
    lv_obj_t *pause_label;
    lv_obj_t *info;
    const ui_design_t *d;
    int cockpit;              /* Layout: grosse Zahlen statt Kopfzeile */
    uint32_t rows;            /* Zeilen der Tabelle */
} w;

static int32_t to_tenth(float v)  { return (int32_t)(v * 10.0f + (v >= 0.0f ? 0.5f : -0.5f)); }
static int32_t to_milli(float v)  { return (int32_t)(v * 1000.0f + (v >= 0.0f ? 0.5f : -0.5f)); }

static uint32_t window_start(uint32_t count)
{
    uint32_t start = (count > POINTS) ? count - POINTS : 0U;
    return (start < s_base) ? s_base : start;
}

/* Achsenbereiche an die sichtbaren Messwerte anpassen und Achsenbeschriftung setzen */
static void rescale(uint32_t count)
{
    float tmin = 1e9f, tmax = -1e9f, vmin = 1e9f, vmax = -1e9f;
    data_sample_t s;
    for (uint32_t i = window_start(count); i < count; i++) {
        if (data_hub_get(i, &s) != 0) continue;
        if (s.temp_c < tmin) tmin = s.temp_c;
        if (s.temp_c > tmax) tmax = s.temp_c;
        if (s.vdda_v < vmin) vmin = s.vdda_v;
        if (s.vdda_v > vmax) vmax = s.vdda_v;
    }
    if (tmax < tmin) {                      /* noch keine Daten */
        tmin = 20.0f; tmax = 40.0f; vmin = 3.2f; vmax = 3.4f;
    }
    float tlo = (float)(int32_t)(tmin - 1.0f), thi = (float)(int32_t)(tmax + 2.0f);
    if (thi - tlo < 4.0f) { thi = tlo + 4.0f; }
    float vlo = vmin - 0.02f, vhi = vmax + 0.02f;
    if (vhi - vlo < 0.08f) { float m = (vhi + vlo) / 2.0f; vlo = m - 0.04f; vhi = m + 0.04f; }

    lv_chart_set_axis_range(w.chart, LV_CHART_AXIS_PRIMARY_Y, to_tenth(tlo), to_tenth(thi));
    lv_chart_set_axis_range(w.chart, LV_CHART_AXIS_SECONDARY_Y, to_milli(vlo), to_milli(vhi));

    char b[16];
    bsp_format_float(b, sizeof b, thi, 0); lv_label_set_text(w.t_max, b);
    bsp_format_float(b, sizeof b, tlo, 0); lv_label_set_text(w.t_min, b);
    bsp_format_float(b, sizeof b, vhi, 2); lv_label_set_text(w.v_max, b);
    bsp_format_float(b, sizeof b, vlo, 2); lv_label_set_text(w.v_min, b);
}

static void fill_table(uint32_t count)
{
    char b[24], t[16], v[16];
    data_sample_t s;
    const uint32_t start = window_start(count);
    for (uint32_t r = 0; r < w.rows; r++) {
        if (count > start + r && data_hub_get(count - 1U - r, &s) == 0) {
            snprintf(b, sizeof b, "%02lu:%02lu", (unsigned long)(s.t_s / 60U), (unsigned long)(s.t_s % 60U));
            lv_table_set_cell_value(w.table, r, 0, b);
            bsp_format_float(t, sizeof t, s.temp_c, 1);
            lv_table_set_cell_value(w.table, r, 1, t);
            bsp_format_float(v, sizeof v, s.vdda_v, 2);
            lv_table_set_cell_value(w.table, r, 2, v);
        } else {
            for (unsigned c = 0; c < 3; c++) lv_table_set_cell_value(w.table, r, c, "");
        }
    }
}

static void clear_chart(void)
{
    lv_chart_set_all_values(w.chart, w.ser_t, LV_CHART_POINT_NONE);
    lv_chart_set_all_values(w.chart, w.ser_v, LV_CHART_POINT_NONE);
}

/* Neue Messwerte seit s_next ins Diagramm schieben und Anzeigen aktualisieren */
static void sync(void)
{
    const uint32_t count = data_hub_count();
    if (s_next < window_start(count)) {
        s_next = window_start(count);
        clear_chart();
    }
    data_sample_t s;
    while (s_next < count) {
        if (data_hub_get(s_next, &s) == 0) {
            lv_chart_set_next_value(w.chart, w.ser_t, to_tenth(s.temp_c));
            lv_chart_set_next_value(w.chart, w.ser_v, to_milli(s.vdda_v));
        }
        s_next++;
    }
    rescale(count);
    lv_chart_refresh(w.chart);
    fill_table(count);

    char b[24], big[24], t[16], v[16];
    if (count > s_base && data_hub_get(count - 1U, &s) == 0) {
        bsp_format_float(t, sizeof t, s.temp_c, 1);
        bsp_format_float(v, sizeof v, s.vdda_v, 2);
        snprintf(big, sizeof big, w.cockpit ? "%s" : "%s C", t);
        lv_label_set_text(w.big_t, big);
        snprintf(big, sizeof big, w.cockpit ? "%s" : "%s V", v);
        lv_label_set_text(w.big_v, big);
    } else {
        lv_label_set_text(w.big_t, "-");
        lv_label_set_text(w.big_v, "-");
    }
    snprintf(b, sizeof b, "%lu Punkte, 1 s", (unsigned long)(count - window_start(count)));
    lv_label_set_text(w.info, b);
}

static void pause_cb(lv_event_t *e)
{
    (void)e;
    s_paused = !s_paused;
    lv_label_set_text(w.pause_label, s_paused ? "Weiter" : "Pause");
    if (!s_paused) {
        sync();
    }
}

static void clear_cb(lv_event_t *e)
{
    (void)e;
    s_base = data_hub_count();
    s_next = s_base;
    clear_chart();
    sync();
}

static void axis_labels(lv_obj_t *card, const ui_design_t *d, int32_t ch, int32_t cw)
{
    const int32_t lh = lv_font_get_line_height(d->font_small);
    w.t_max = ui_label(card, d, UI_TEXT_SMALL, "");  lv_obj_set_pos(w.t_max, 4, 6);
    w.t_min = ui_label(card, d, UI_TEXT_SMALL, "");  lv_obj_set_pos(w.t_min, 4, ch - 8 - lh);
    w.v_max = ui_label(card, d, UI_TEXT_SMALL, "");  lv_obj_set_pos(w.v_max, cw - 34, 6);
    w.v_min = ui_label(card, d, UI_TEXT_SMALL, "");  lv_obj_set_pos(w.v_min, cw - 34, ch - 8 - lh);
    ui_label_set_kind_color(w.t_max, d, UI_TEXT_ACCENT);
    ui_label_set_kind_color(w.t_min, d, UI_TEXT_ACCENT);
    ui_label_set_kind_color(w.v_max, d, UI_TEXT_WARN);
    ui_label_set_kind_color(w.v_min, d, UI_TEXT_WARN);
    lv_obj_t *l[4] = { w.t_max, w.t_min, w.v_max, w.v_min };
    for (unsigned i = 0; i < 4; i++) lv_obj_set_style_text_font(l[i], d->font_small, 0);
}

/* Diagramm mit zwei Kurven (Temperatur links, Spannung rechts) in einer Karte */
static void build_chart(lv_obj_t *c, const ui_design_t *d, int32_t x, int32_t y, int32_t cw, int32_t ch)
{
    lv_obj_t *cc = ui_card(c, d, x, y, cw, ch);
    w.chart = ui_chart(cc, d, 36, 8, cw - 72, ch - 16, POINTS);
    lv_chart_set_axis_range(w.chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_axis_range(w.chart, LV_CHART_AXIS_SECONDARY_Y, 0, 100);
    w.ser_t = lv_chart_add_series(w.chart, ui_col(d->accent), LV_CHART_AXIS_PRIMARY_Y);
    w.ser_v = lv_chart_add_series(w.chart, ui_col(d->warn), LV_CHART_AXIS_SECONDARY_Y);
    clear_chart();
    axis_labels(cc, d, ch, cw);
}

/* Tabelle der letzten Messwerte (neueste oben) in einer Karte */
static void build_table(lv_obj_t *c, const ui_design_t *d, int32_t x, int32_t y, int32_t tw, int32_t th)
{
    lv_obj_t *tc = ui_card(c, d, x, y, tw, th);
    const int32_t hh = (int32_t)lv_font_get_line_height(d->font_small);
    const int32_t rowh = ui_table_row_h(d);
    w.rows = (uint32_t)((th - hh - 12) / rowh);
    if (w.rows > TABLE_ROWS_MAX) w.rows = TABLE_ROWS_MAX;
    if (w.rows < 3U) w.rows = 3U;
    const int32_t inner = tw - 8;
    const int32_t col_w[3] = { inner * 38 / 100, inner * 31 / 100, inner - inner * 38 / 100 - inner * 31 / 100 };
    static const char *const hdr[3] = { "Zeit", "C", "V" };
    int32_t cx = 4;
    for (unsigned i = 0; i < 3; i++) {
        lv_obj_t *h = ui_label(tc, d, UI_TEXT_SMALL_DIM, hdr[i]);
        lv_obj_set_pos(h, cx + 2, 4);
        cx += col_w[i];
    }
    w.table = ui_table(tc, d, 4, 6 + hh, w.rows, 3, col_w);
}

static void build_buttons(lv_obj_t *c, const ui_design_t *d, int32_t x, int32_t y, int32_t bw, int32_t gap, int vertical)
{
    lv_obj_t *bp = ui_button(c, d, s_paused ? "Weiter" : "Pause", x, y, bw, 30);
    w.pause_label = lv_obj_get_child(bp, 0);
    lv_obj_add_event_cb(bp, pause_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bc = ui_button(c, d, "Loeschen", vertical ? x : x + bw + gap, vertical ? y + 30 + gap : y, bw, 30);
    lv_obj_add_event_cb(bc, clear_cb, LV_EVENT_CLICKED, NULL);
}

/* Layout CARDS: Kopfzeile mit Titel und aktuellen Werten, Diagramm links, Tabelle rechts */
static void build_cards(const ui_design_t *d, ui_frame_t *f)
{
    lv_obj_t *c = f->content;
    lv_obj_t *title = ui_label(c, d, UI_TEXT_TITLE, "Messwerte");
    lv_obj_set_pos(title, 12, 10);

    lv_obj_t *row = lv_obj_create(c);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 24, 0);
    lv_obj_align(row, LV_ALIGN_TOP_RIGHT, -12, 4);
    w.big_t = ui_label(row, d, UI_TEXT_BIG, "-");
    w.big_v = ui_label(row, d, UI_TEXT_BIG, "-");
    ui_label_set_kind_color(w.big_t, d, UI_TEXT_ACCENT);
    ui_label_set_kind_color(w.big_v, d, UI_TEXT_WARN);
    lv_obj_set_style_text_font(w.big_t, d->font_big, 0);
    lv_obj_set_style_text_font(w.big_v, d->font_big, 0);

    const int32_t top = 44;
    const int32_t ch = f->h - top - 46;
    const int32_t tw = 132;
    const int32_t tx = f->w - 8 - tw;
    build_chart(c, d, 8, top, tx - 8 - 8, ch);
    build_table(c, d, tx, top, tw, ch);

    build_buttons(c, d, 8, f->h - 38, 100, 8, 0);
    w.info = ui_label(c, d, UI_TEXT_SMALL_DIM, "");
    lv_obj_set_pos(w.info, 228, f->h - 38 + 8);
    lv_obj_set_width(w.info, f->w - 228 - 8);
}

/* Layout COCKPIT: grosse Zahlen links, Diagramm und kleine Tabelle rechts */
static void build_cockpit(const ui_design_t *d, ui_frame_t *f)
{
    lv_obj_t *c = f->content;
    const int32_t lw = 148;
    const int32_t bh = 80;

    lv_obj_t *b1 = ui_card(c, d, 8, 4, lw, bh);
    ui_label(b1, d, UI_TEXT_SMALL_DIM, "CPU-TEMP");
    w.big_t = ui_label(b1, d, UI_TEXT_HERO, "-");
    lv_obj_set_pos(w.big_t, 0, 14);
    lv_obj_set_style_text_color(w.big_t, ui_col(d->accent), 0);
    lv_obj_t *u1 = ui_label(b1, d, UI_TEXT_TITLE, "C");
    lv_obj_align(u1, LV_ALIGN_BOTTOM_RIGHT, 0, -6);

    lv_obj_t *b2 = ui_card(c, d, 8, 4 + bh + 6, lw, bh);
    ui_label(b2, d, UI_TEXT_SMALL_DIM, "VDDA");
    w.big_v = ui_label(b2, d, UI_TEXT_HERO, "-");
    lv_obj_set_pos(w.big_v, 0, 14);
    lv_obj_set_style_text_color(w.big_v, ui_col(d->warn), 0);
    lv_obj_t *u2 = ui_label(b2, d, UI_TEXT_TITLE, "V");
    lv_obj_align(u2, LV_ALIGN_BOTTOM_RIGHT, 0, -6);
    lv_obj_set_style_pad_all(b1, 8, 0);
    lv_obj_set_style_pad_all(b2, 8, 0);

    const int32_t by = 4 + 2 * (bh + 6);
    build_buttons(c, d, 8, by + 4, lw, 6, 1);
    w.info = ui_label(c, d, UI_TEXT_SMALL_DIM, "");
    lv_obj_set_pos(w.info, 10, by + 4 + 2 * 30 + 6 + 4);
    lv_obj_set_width(w.info, lw);

    const int32_t rx = 8 + lw + 8;
    const int32_t rw = f->w - rx - 8;
    const int32_t chart_h = 140;
    build_chart(c, d, rx, 4, rw, chart_h);
    build_table(c, d, rx, 4 + chart_h + 6, rw, f->h - 4 - chart_h - 6 - 8);
}

lv_obj_t *page_values_create(const ui_design_t *d, unsigned idx)
{
    memset(&w, 0, sizeof w);
    w.d = d;
    w.cockpit = (d->layout == UI_LAYOUT_COCKPIT);
    ui_frame_t f = ui_frame_create(d, idx);
    if (w.cockpit) {
        build_cockpit(d, &f);
    } else {
        build_cards(d, &f);
    }

    /* Verlauf aus dem Archiv nachladen */
    const uint32_t count = data_hub_count();
    s_next = window_start(count);
    sync();
    return f.screen;
}

void page_values_update(void)
{
    if (w.chart == NULL || s_paused) {
        return;
    }
    sync();
}
