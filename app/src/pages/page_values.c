/* Seite MESSWERTE (Projekt Terrarium): vier Cockpit-Kacheln mit den beiden Temperaturen und den beiden
 * Luftfeuchten der DHT-Sensoren. Jede Kachel zeigt links die runde Anzeige mit dem aktuellen Wert und rechts ein
 * Mini-Diagramm des Verlaufs (letzte MINI_POINTS Messungen) mit dem Wertebereich darunter.
 * Alle Werte kommen fertig in Grad Celsius bzw. Prozent aus dem data_hub. */
#include "pages.h"
#include "ui_widgets.h"
#include "data_hub.h"
#include "bsp_dht.h"
#include "bsp_util.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define TILES       4U
#define MINI_POINTS 60U            /* Verlauf im Mini-Diagramm: 60 Messungen (bei 2 s = 2 min) */

typedef struct {
    unsigned sensor;               /* 0/1: Sensor */
    int      is_hum;               /* 0 = Temperatur, 1 = Luftfeuchte */
    const char *title;
} tile_def_t;

/* Reihenfolge: obere Zeile die Temperaturen, untere Zeile die Luftfeuchten */
static const tile_def_t k_tiles[TILES] = {
    { 0, 0, "Temp 1 C" }, { 1, 0, "Temp 2 C" },
    { 0, 1, "Hum 1 %"  }, { 1, 1, "Hum 2 %"  },
};

static uint32_t s_next;            /* naechste noch nicht ins Mini-Diagramm uebernommene Messung (bleibt beim Neuaufbau) */

static struct {
    const ui_design_t *d;
    struct {
        ui_gauge_t gauge;
        lv_obj_t *chart;
        lv_chart_series_t *ser;
        lv_obj_t *range;
    } t[TILES];
} w;

static int32_t to_tenth(float v) { return (int32_t)floorf(v * 10.0f + 0.5f); }
static int32_t to_int(float v)   { return (int32_t)floorf(v + 0.5f); }

static float value_of(const data_sample_t *s, unsigned tile)
{
    return k_tiles[tile].is_hum ? s->hum_pct[k_tiles[tile].sensor] : s->temp_c[k_tiles[tile].sensor];
}

static int valid(const data_sample_t *s, unsigned tile)
{
    return s->ok[k_tiles[tile].sensor] != 0U;
}

static uint32_t window_start(uint32_t count)
{
    return (count > MINI_POINTS) ? count - MINI_POINTS : 0U;
}

/* Achsenbereich der Mini-Diagramme an den sichtbaren Verlauf anpassen und den Wertebereich anzeigen */
static void rescale(unsigned tile, uint32_t count)
{
    float lo = 1e9f, hi = -1e9f;
    data_sample_t s;
    for (uint32_t i = window_start(count); i < count; i++) {
        if (data_hub_get(i, &s) != 0 || !valid(&s, tile)) continue;
        const float v = value_of(&s, tile);
        if (v < lo) lo = v;
        if (v > hi) hi = v;
    }
    const int is_hum = k_tiles[tile].is_hum;
    char text[48], a[16], b[16];
    float axlo, axhi;
    if (hi < lo) {                               /* noch keine gueltigen Werte */
        axlo = is_hum ? 40.0f : 15.0f;
        axhi = is_hum ? 60.0f : 25.0f;
        snprintf(text, sizeof text, "--");
    } else {
        const float pad = is_hum ? 2.0f : 1.0f;
        const float span = is_hum ? 10.0f : 4.0f;
        axlo = floorf(lo - pad);
        axhi = ceilf(hi + pad);
        if (axhi - axlo < span) { const float m = (axhi + axlo) / 2.0f; axlo = m - span / 2.0f; axhi = m + span / 2.0f; }
        if (is_hum) { if (axlo < 0.0f) axlo = 0.0f; if (axhi > 100.0f) axhi = 100.0f; }
        bsp_format_float(a, sizeof a, lo, BSP_DHT_DECIMALS);
        bsp_format_float(b, sizeof b, hi, BSP_DHT_DECIMALS);
        snprintf(text, sizeof text, "%s..%s %s", a, b, is_hum ? "%" : "C");
    }
    lv_chart_set_axis_range(w.t[tile].chart, LV_CHART_AXIS_PRIMARY_Y, to_tenth(axlo), to_tenth(axhi));
    lv_label_set_text(w.t[tile].range, text);
}

static void clear_charts(void)
{
    for (unsigned i = 0; i < TILES; i++) {
        lv_chart_set_all_values(w.t[i].chart, w.t[i].ser, LV_CHART_POINT_NONE);
    }
}

/* Runde Anzeigen mit der neuesten Messung (ungueltig: "--") */
static void update_gauges(uint32_t count)
{
    data_sample_t s;
    const int have = (count > 0U) && (data_hub_get(count - 1U, &s) == 0);
    char b[16];
    for (unsigned i = 0; i < TILES; i++) {
        const int is_hum = k_tiles[i].is_hum;
        if (have && valid(&s, i)) {
            bsp_format_float(b, sizeof b, value_of(&s, i), BSP_DHT_DECIMALS);
            ui_gauge_set(&w.t[i].gauge, to_int(value_of(&s, i)), b);
        } else {
            ui_gauge_set(&w.t[i].gauge, is_hum ? 0 : BSP_DHT_TEMP_MIN, "--");
        }
    }
}

/* Neue Messungen seit s_next in die Mini-Diagramme schieben und alle Anzeigen aktualisieren */
static void sync(void)
{
    const uint32_t count = data_hub_count();
    if (s_next < window_start(count)) {
        s_next = window_start(count);
        clear_charts();
    }
    data_sample_t s;
    while (s_next < count) {
        if (data_hub_get(s_next, &s) == 0) {
            for (unsigned i = 0; i < TILES; i++) {
                lv_chart_set_next_value(w.t[i].chart, w.t[i].ser,
                                        valid(&s, i) ? to_tenth(value_of(&s, i)) : LV_CHART_POINT_NONE);
            }
        }
        s_next++;
    }
    for (unsigned i = 0; i < TILES; i++) {
        rescale(i, count);
        lv_chart_refresh(w.t[i].chart);
    }
    update_gauges(count);
}

lv_obj_t *page_values_create(const ui_design_t *d, unsigned idx)
{
    memset(&w, 0, sizeof w);
    w.d = d;
    ui_frame_t f = ui_frame_create(d, idx);
    lv_obj_t *c = f.content;

    /* 2 x 2 Kacheln */
    const int32_t gap = 8;
    const int32_t tw = (f.w - 3 * gap) / 2;
    const int32_t th = (f.h - 8 - gap) / 2 - 0;      /* oberer Rand 4, unterer Rand 4 */
    const int32_t gs = 96;                           /* Durchmesser der runden Anzeige */
    const int32_t lh = lv_font_get_line_height(d->font_small);

    for (unsigned i = 0; i < TILES; i++) {
        const int32_t x = gap + (int32_t)(i % 2U) * (tw + gap);
        const int32_t y = 4 + (int32_t)(i / 2U) * (th + gap - 4);
        lv_obj_t *card = ui_card(c, d, x, y, tw, th);
        const int is_hum = k_tiles[i].is_hum;
        const uint32_t color = is_hum ? d->ok : d->warn;

        w.t[i].gauge = ui_gauge(card, d, 4, (th - gs) / 2 - 1, gs, k_tiles[i].title,
                                is_hum ? 0 : BSP_DHT_TEMP_MIN, is_hum ? 100 : BSP_DHT_TEMP_MAX, color);

        /* Mini-Diagramm rechts neben der Anzeige, Wertebereich darunter */
        const int32_t cx = 4 + gs + 4;
        const int32_t cw = tw - cx - 8;
        const int32_t ch = th - 2 * 10 - lh - 6;
        w.t[i].chart = ui_chart(card, d, cx, 10, cw, ch, MINI_POINTS);
        lv_chart_set_div_line_count(w.t[i].chart, 0, 0);          /* Sparkline ohne Gitter */
        w.t[i].ser = lv_chart_add_series(w.t[i].chart, ui_col(color), LV_CHART_AXIS_PRIMARY_Y);
        lv_chart_set_all_values(w.t[i].chart, w.t[i].ser, LV_CHART_POINT_NONE);
        w.t[i].range = ui_label(card, d, UI_TEXT_SMALL_DIM, "--");
        lv_obj_set_pos(w.t[i].range, cx, 10 + ch + 4);
        lv_obj_set_width(w.t[i].range, cw);
    }

    /* Verlauf aus dem Archiv nachladen */
    s_next = window_start(data_hub_count());
    sync();
    return f.screen;
}

void page_values_update(void)
{
    if (w.t[0].chart == NULL) {
        return;
    }
    sync();
}
