/* Seite MESSWERTE (Projekt Terrarium): drei Cockpit-Anzeigen (MCU-Temperatur, Luftfeuchte, Umgebungstemperatur),
 * darunter Live-Diagramm und Live-Tabelle der aufgezeichneten Werte sowie Pause/Loeschen.
 * Alle Werte kommen fertig in Grad Celsius bzw. Prozent aus dem data_hub; die Versorgungsspannung wird hier nicht
 * angezeigt oder aufgezeichnet. */
#include "pages.h"
#include "ui_widgets.h"
#include "data_hub.h"
#include "bsp_dht.h"
#include "bsp_util.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define POINTS         DATA_HUB_CAPACITY
#define TABLE_ROWS_MAX 6U
#define MCU_MIN        20          /* Skala der MCU-Anzeige in Grad Celsius */
#define MCU_MAX        90

/* Zustand (bleibt beim Neuaufbau der Seite erhalten) */
static int      s_paused;
static uint32_t s_base;          /* Messwerte vor dieser Nummer gelten als geloescht */
static uint32_t s_next;          /* naechster noch nicht ins Diagramm uebernommener Messwert */

static struct {
    const ui_design_t *d;
    ui_gauge_t g_mcu, g_hum, g_temp;
    lv_obj_t *chart;
    lv_chart_series_t *ser_mcu, *ser_temp, *ser_hum;
    lv_obj_t *t_max, *t_min, *h_max, *h_min;
    lv_obj_t *table;
    lv_obj_t *pause_label;
    lv_obj_t *info;
    uint32_t rows;
} w;

static int32_t to_tenth(float v) { return (int32_t)floorf(v * 10.0f + 0.5f); }
static int32_t to_int(float v)   { return (int32_t)floorf(v + 0.5f); }

static uint32_t window_start(uint32_t count)
{
    uint32_t start = (count > POINTS) ? count - POINTS : 0U;
    return (start < s_base) ? s_base : start;
}

/* Diagrammachsen an die sichtbaren Werte anpassen: links Grad Celsius (MCU und Umgebung), rechts Prozent */
static void rescale(uint32_t count)
{
    float tmin = 1e9f, tmax = -1e9f, hmin = 1e9f, hmax = -1e9f;
    data_sample_t s;
    for (uint32_t i = window_start(count); i < count; i++) {
        if (data_hub_get(i, &s) != 0) continue;
        if (s.mcu_ok) { if (s.mcu_c < tmin) tmin = s.mcu_c; if (s.mcu_c > tmax) tmax = s.mcu_c; }
        if (s.dht_ok) {
            if (s.temp_c < tmin) tmin = s.temp_c;
            if (s.temp_c > tmax) tmax = s.temp_c;
            if (s.hum_pct < hmin) hmin = s.hum_pct;
            if (s.hum_pct > hmax) hmax = s.hum_pct;
        }
    }
    if (tmax < tmin) { tmin = 20.0f; tmax = 40.0f; }
    if (hmax < hmin) { hmin = 30.0f; hmax = 70.0f; }

    float tlo = floorf(tmin - 1.0f), thi = ceilf(tmax + 1.0f);
    if (thi - tlo < 4.0f) { thi = tlo + 4.0f; }
    float hlo = floorf(hmin) - 3.0f, hhi = ceilf(hmax) + 3.0f;
    if (hhi - hlo < 10.0f) { float m = (hhi + hlo) / 2.0f; hlo = m - 5.0f; hhi = m + 5.0f; }
    if (hlo < 0.0f) hlo = 0.0f;
    if (hhi > 100.0f) hhi = 100.0f;

    lv_chart_set_axis_range(w.chart, LV_CHART_AXIS_PRIMARY_Y, to_tenth(tlo), to_tenth(thi));
    lv_chart_set_axis_range(w.chart, LV_CHART_AXIS_SECONDARY_Y, to_tenth(hlo), to_tenth(hhi));

    char b[16];
    bsp_format_float(b, sizeof b, thi, 0); lv_label_set_text(w.t_max, b);
    bsp_format_float(b, sizeof b, tlo, 0); lv_label_set_text(w.t_min, b);
    bsp_format_float(b, sizeof b, hhi, 0); lv_label_set_text(w.h_max, b);
    bsp_format_float(b, sizeof b, hlo, 0); lv_label_set_text(w.h_min, b);
}

static void value_text(char *out, size_t n, int ok, float v, unsigned decimals)
{
    if (ok) {
        bsp_format_float(out, n, v, decimals);
    } else {
        snprintf(out, n, "--");
    }
}

static void fill_table(uint32_t count)
{
    char b[24], m[16], t[16], h[16];
    data_sample_t s;
    const uint32_t start = window_start(count);
    for (uint32_t r = 0; r < w.rows; r++) {
        if (count > start + r && data_hub_get(count - 1U - r, &s) == 0) {
            snprintf(b, sizeof b, "%02lu:%02lu", (unsigned long)(s.t_s / 60U), (unsigned long)(s.t_s % 60U));
            value_text(m, sizeof m, s.mcu_ok, s.mcu_c, 1);
            value_text(t, sizeof t, s.dht_ok, s.temp_c, BSP_DHT_DECIMALS);
            value_text(h, sizeof h, s.dht_ok, s.hum_pct, BSP_DHT_DECIMALS);
            lv_table_set_cell_value(w.table, r, 0, b);
            lv_table_set_cell_value(w.table, r, 1, m);
            lv_table_set_cell_value(w.table, r, 2, t);
            lv_table_set_cell_value(w.table, r, 3, h);
        } else {
            for (unsigned c = 0; c < 4; c++) lv_table_set_cell_value(w.table, r, c, "");
        }
    }
}

static void clear_chart(void)
{
    lv_chart_set_all_values(w.chart, w.ser_mcu, LV_CHART_POINT_NONE);
    lv_chart_set_all_values(w.chart, w.ser_temp, LV_CHART_POINT_NONE);
    lv_chart_set_all_values(w.chart, w.ser_hum, LV_CHART_POINT_NONE);
}

/* Die drei runden Anzeigen mit dem neuesten Messwert aktualisieren (ungueltig: "--") */
static void update_gauges(uint32_t count)
{
    data_sample_t s;
    const int have = (count > s_base) && (data_hub_get(count - 1U, &s) == 0);
    char b[16];
    if (have && s.mcu_ok) {
        bsp_format_float(b, sizeof b, s.mcu_c, 1);
        ui_gauge_set(&w.g_mcu, to_int(s.mcu_c), b);
    } else {
        ui_gauge_set(&w.g_mcu, MCU_MIN, "--");
    }
    if (have && s.dht_ok) {
        bsp_format_float(b, sizeof b, s.hum_pct, BSP_DHT_DECIMALS);
        ui_gauge_set(&w.g_hum, to_int(s.hum_pct), b);
        bsp_format_float(b, sizeof b, s.temp_c, BSP_DHT_DECIMALS);
        ui_gauge_set(&w.g_temp, to_int(s.temp_c), b);
    } else {
        ui_gauge_set(&w.g_hum, 0, "--");
        ui_gauge_set(&w.g_temp, BSP_DHT_TEMP_MIN, "--");
    }
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
            lv_chart_set_next_value(w.chart, w.ser_mcu, s.mcu_ok ? to_tenth(s.mcu_c) : LV_CHART_POINT_NONE);
            lv_chart_set_next_value(w.chart, w.ser_temp, s.dht_ok ? to_tenth(s.temp_c) : LV_CHART_POINT_NONE);
            lv_chart_set_next_value(w.chart, w.ser_hum, s.dht_ok ? to_tenth(s.hum_pct) : LV_CHART_POINT_NONE);
        }
        s_next++;
    }
    rescale(count);
    lv_chart_refresh(w.chart);
    fill_table(count);
    update_gauges(count);

    char b[40];
    snprintf(b, sizeof b, "%lu Punkte, alle %u s", (unsigned long)(count - window_start(count)),
             (unsigned)DATA_HUB_PERIOD_S);
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

lv_obj_t *page_values_create(const ui_design_t *d, unsigned idx)
{
    memset(&w, 0, sizeof w);
    w.d = d;
    ui_frame_t f = ui_frame_create(d, idx);
    lv_obj_t *c = f.content;

    /* ---- drei Cockpit-Anzeigen: MCU, Hum, Temp ---- */
    const int32_t gap = 8;
    const int32_t cw = (f.w - 4 * gap) / 3;
    const int32_t gh = 104;
    const int32_t gs = 92;
    static const char *const titles[3] = { "MCU C", "Hum %", "Temp C" };
    ui_gauge_t *g[3] = { &w.g_mcu, &w.g_hum, &w.g_temp };
    const int32_t lo[3] = { MCU_MIN, 0, BSP_DHT_TEMP_MIN };
    const int32_t hi[3] = { MCU_MAX, 100, BSP_DHT_TEMP_MAX };
    const uint32_t col[3] = { d->accent, d->ok, d->warn };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *card = ui_card(c, d, gap + i * (cw + gap), 4, cw, gh);
        *g[i] = ui_gauge(card, d, (cw - gs) / 2 - 1, 5, gs, titles[i], lo[i], hi[i], col[i]);
    }

    /* ---- Diagramm (links) und Tabelle (rechts) ---- */
    const int32_t ry = 4 + gh + gap;
    const int32_t rh = 120;
    const int32_t tw = 158;
    const int32_t tx = f.w - gap - tw;
    const int32_t chw = tx - gap - gap;

    lv_obj_t *cc = ui_card(c, d, gap, ry, chw, rh);
    w.chart = ui_chart(cc, d, 34, 8, chw - 34 - 40, rh - 16, POINTS);
    lv_chart_set_axis_range(w.chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_axis_range(w.chart, LV_CHART_AXIS_SECONDARY_Y, 0, 100);
    w.ser_mcu  = lv_chart_add_series(w.chart, ui_col(d->accent), LV_CHART_AXIS_PRIMARY_Y);
    w.ser_temp = lv_chart_add_series(w.chart, ui_col(d->warn), LV_CHART_AXIS_PRIMARY_Y);
    w.ser_hum  = lv_chart_add_series(w.chart, ui_col(d->ok), LV_CHART_AXIS_SECONDARY_Y);
    clear_chart();
    const int32_t lh = lv_font_get_line_height(d->font_small);
    w.t_max = ui_label(cc, d, UI_TEXT_SMALL_DIM, "");  lv_obj_set_pos(w.t_max, 4, 6);
    w.t_min = ui_label(cc, d, UI_TEXT_SMALL_DIM, "");  lv_obj_set_pos(w.t_min, 4, rh - 8 - lh);
    w.h_max = ui_label(cc, d, UI_TEXT_SMALL, "");      lv_obj_set_pos(w.h_max, chw - 32, 6);
    w.h_min = ui_label(cc, d, UI_TEXT_SMALL, "");      lv_obj_set_pos(w.h_min, chw - 32, rh - 8 - lh);
    ui_label_set_kind_color(w.h_max, d, UI_TEXT_OK);
    ui_label_set_kind_color(w.h_min, d, UI_TEXT_OK);
    lv_obj_set_style_text_font(w.h_max, d->font_small, 0);
    lv_obj_set_style_text_font(w.h_min, d->font_small, 0);

    lv_obj_t *tc = ui_card(c, d, tx, ry, tw, rh);
    const int32_t hh = lv_font_get_line_height(d->font_small);
    const int32_t rowh = ui_table_row_h(d);
    w.rows = (uint32_t)((rh - hh - 12) / rowh);
    if (w.rows > TABLE_ROWS_MAX) w.rows = TABLE_ROWS_MAX;
    const int32_t inner = tw - 8;
    const int32_t col_w[4] = { 38, 36, 38, inner - 38 - 36 - 38 };
    static const char *const hdr[4] = { "Zeit", "MCU", "Temp", "Hum" };
    int32_t x = 4;
    for (unsigned i = 0; i < 4; i++) {
        lv_obj_t *h = ui_label(tc, d, UI_TEXT_SMALL_DIM, hdr[i]);
        lv_obj_set_pos(h, x + 2, 4);
        x += col_w[i];
    }
    w.table = ui_table(tc, d, 4, 6 + hh, w.rows, 4, col_w);

    /* ---- Schaltflaechen und Info ---- */
    const int32_t by = ry + rh + 6;
    lv_obj_t *bp = ui_button(c, d, s_paused ? "Weiter" : "Pause", gap, by, 100, 26);
    w.pause_label = lv_obj_get_child(bp, 0);
    lv_obj_add_event_cb(bp, pause_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bc = ui_button(c, d, "Loeschen", gap + 100 + gap, by, 100, 26);
    lv_obj_add_event_cb(bc, clear_cb, LV_EVENT_CLICKED, NULL);
    w.info = ui_label(c, d, UI_TEXT_SMALL_DIM, "");
    lv_obj_set_pos(w.info, 2 * gap + 2 * 100 + 12, by + 6);
    lv_obj_set_width(w.info, f.w - (2 * gap + 2 * 100 + 12) - gap);

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
