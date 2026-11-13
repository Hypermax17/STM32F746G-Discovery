#include "ui_widgets.h"
#include "ui_pages.h"

#include <stdint.h>

#define SCREEN_W 480
#define SCREEN_H 272

#define SIDE_RAIL_W 76

/* ---- Hilfen ------------------------------------------------------------------ */

static void plain(lv_obj_t *o)
{
    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

void ui_label_set_kind_color(lv_obj_t *label, const ui_design_t *d, ui_text_kind_t kind)
{
    uint32_t c = d->text;
    const lv_font_t *f = d->font;
    switch (kind) {
    case UI_TEXT_NORMAL:    break;
    case UI_TEXT_DIM:       c = d->text_dim; break;
    case UI_TEXT_SMALL:     f = d->font_small; break;
    case UI_TEXT_SMALL_DIM: f = d->font_small; c = d->text_dim; break;
    case UI_TEXT_BIG:       f = d->font_big; break;
    case UI_TEXT_TITLE:     f = d->font_title; break;
    case UI_TEXT_ACCENT:    c = d->accent; break;
    case UI_TEXT_WARN:      c = d->warn; break;
    case UI_TEXT_OK:        c = d->ok; break;
    case UI_TEXT_BAD:       c = d->bad; break;
    }
    lv_obj_set_style_text_color(label, ui_col(c), 0);
    lv_obj_set_style_text_font(label, f, 0);
}

lv_obj_t *ui_card(lv_obj_t *parent, const ui_design_t *d, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *o = lv_obj_create(parent);
    plain(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, ui_col(d->panel), 0);
    lv_obj_set_style_bg_opa(o, d->panel_opa, 0);
    lv_obj_set_style_radius(o, d->radius, 0);
    lv_obj_set_style_border_width(o, d->border_w, 0);
    lv_obj_set_style_border_color(o, ui_col(d->border), 0);
    lv_obj_set_style_border_opa(o, d->border_opa, 0);
    if (d->shadow_w > 0U) {
        lv_obj_set_style_shadow_width(o, d->shadow_w, 0);
        lv_obj_set_style_shadow_offset_y(o, 4, 0);
        lv_obj_set_style_shadow_color(o, lv_color_black(), 0);
        lv_obj_set_style_shadow_opa(o, LV_OPA_40, 0);
    }
    return o;
}

lv_obj_t *ui_label(lv_obj_t *parent, const ui_design_t *d, ui_text_kind_t kind, const char *text)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
    ui_label_set_kind_color(l, d, kind);
    return l;
}

lv_obj_t *ui_dot(lv_obj_t *parent, int32_t x, int32_t y, int32_t dia, uint32_t rgb)
{
    lv_obj_t *o = lv_obj_create(parent);
    plain(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, dia, dia);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(o, ui_col(rgb), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    return o;
}

lv_obj_t *ui_button(lv_obj_t *parent, const ui_design_t *d, const char *text,
                    int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *b = lv_button_create(parent);
    plain(b);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_radius(b, d->radius, 0);
    lv_obj_set_style_border_width(b, d->border_w, 0);
    lv_obj_set_style_border_color(b, ui_col(d->border), 0);
    lv_obj_set_style_border_opa(b, d->border_opa, 0);
    lv_obj_set_style_bg_color(b, ui_col(d->panel), 0);
    lv_obj_set_style_bg_opa(b, d->panel_opa, 0);
    lv_obj_set_style_text_color(b, ui_col(d->text), 0);
    lv_obj_set_style_text_font(b, d->font, 0);
    /* gedrueckt: Akzentflaeche */
    lv_obj_set_style_bg_color(b, ui_col(d->accent), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(b, ui_col(d->accent), LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(b, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_set_style_text_color(b, ui_col(d->accent_text), LV_STATE_PRESSED);

    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_center(l);
    return b;
}

lv_obj_t *ui_slider(lv_obj_t *parent, const ui_design_t *d, int32_t x, int32_t y, int32_t w,
                    int32_t min, int32_t max, uint32_t rgb)
{
    lv_obj_t *s = lv_slider_create(parent);
    lv_obj_remove_style_all(s);
    lv_obj_set_pos(s, x, y);
    lv_obj_set_size(s, w, 12);
    lv_slider_set_range(s, min, max);
    lv_obj_set_ext_click_area(s, 14);                         /* Treffflaeche groesser als der Balken */

    lv_obj_set_style_radius(s, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s, ui_col(d->text), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_20, LV_PART_MAIN);       /* Schiene: durchscheinend */

    lv_obj_set_style_radius(s, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s, ui_col(rgb), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_INDICATOR);

    lv_obj_set_style_radius(s, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_bg_color(s, ui_col(d->text), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_pad_all(s, 8, LV_PART_KNOB);              /* Griff ca. 28 px */
    lv_obj_set_style_shadow_width(s, d->shadow_w ? 8 : 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_opa(s, LV_OPA_40, LV_PART_KNOB);
    lv_obj_set_style_shadow_color(s, lv_color_black(), LV_PART_KNOB);
    return s;
}

/* ---- Tabelle / Diagramm / Rundanzeige --------------------------------------- */

int32_t ui_table_row_h(const ui_design_t *d)
{
    return (int32_t)lv_font_get_line_height(d->font_small) + 5;     /* 2 + 2 Abstand + 1 Linie */
}

lv_obj_t *ui_table(lv_obj_t *parent, const ui_design_t *d, int32_t x, int32_t y,
                   uint32_t rows, uint32_t cols, const int32_t *col_widths)
{
    lv_obj_t *t = lv_table_create(parent);
    plain(t);
    lv_obj_set_pos(t, x, y);
    lv_table_set_column_count(t, cols);
    lv_table_set_row_count(t, rows);
    for (uint32_t c = 0; c < cols; c++) {
        lv_table_set_column_width(t, c, col_widths[c]);
    }
    lv_obj_set_style_bg_opa(t, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_text_font(t, d->font_small, LV_PART_ITEMS);
    lv_obj_set_style_text_color(t, ui_col(d->text), LV_PART_ITEMS);
    lv_obj_set_style_pad_all(t, 2, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(t, LV_OPA_TRANSP, LV_PART_ITEMS);
    lv_obj_set_style_border_width(t, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_side(t, LV_BORDER_SIDE_BOTTOM, LV_PART_ITEMS);
    lv_obj_set_style_border_color(t, ui_col(d->border), LV_PART_ITEMS);
    lv_obj_set_style_border_opa(t, (lv_opa_t)(d->border_opa / 2U), LV_PART_ITEMS);
    return t;
}

lv_obj_t *ui_chart(lv_obj_t *parent, const ui_design_t *d, int32_t x, int32_t y, int32_t w, int32_t h,
                   uint32_t points)
{
    lv_obj_t *c = lv_chart_create(parent);
    plain(c);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    lv_chart_set_type(c, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(c, points);
    lv_chart_set_div_line_count(c, 4, 6);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_line_color(c, ui_col(d->border), LV_PART_MAIN);       /* Gitterlinien */
    lv_obj_set_style_line_opa(c, (lv_opa_t)(d->border_opa / 2U + 20U), LV_PART_MAIN);
    lv_obj_set_style_line_width(c, 1, LV_PART_MAIN);
    lv_obj_set_style_line_width(c, 2, LV_PART_ITEMS);                      /* Kurven */
    lv_obj_set_style_size(c, 0, 0, LV_PART_INDICATOR);                     /* keine Punktmarken */
    return c;
}

ui_gauge_t ui_gauge(lv_obj_t *parent, const ui_design_t *d, int32_t x, int32_t y, int32_t size,
                    const char *title, int32_t min, int32_t max, uint32_t rgb)
{
    ui_gauge_t g;
    g.arc = lv_arc_create(parent);
    lv_obj_remove_style_all(g.arc);                     /* ohne Theme: kein Knopf, keine Standardfarben */
    lv_obj_remove_flag(g.arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(g.arc, x, y);
    lv_obj_set_size(g.arc, size, size);
    lv_arc_set_rotation(g.arc, 135);
    lv_arc_set_bg_angles(g.arc, 0, 270);
    lv_arc_set_range(g.arc, min, max);
    lv_arc_set_value(g.arc, min);
    const int32_t aw = size / 9;
    lv_obj_set_style_arc_width(g.arc, aw, LV_PART_MAIN);
    lv_obj_set_style_arc_color(g.arc, ui_col(d->border), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(g.arc, (lv_opa_t)(d->border_opa / 3U), LV_PART_MAIN);   /* dezente Spur */
    lv_obj_set_style_arc_rounded(g.arc, true, LV_PART_MAIN);
    lv_obj_set_style_arc_width(g.arc, aw, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(g.arc, ui_col(rgb), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(g.arc, true, LV_PART_INDICATOR);

    g.value = ui_label(g.arc, d, UI_TEXT_BIG, "-");
    lv_obj_align(g.value, LV_ALIGN_CENTER, 0, -4);
    g.title = ui_label(g.arc, d, UI_TEXT_SMALL_DIM, title);
    lv_obj_align(g.title, LV_ALIGN_BOTTOM_MID, 0, 2);
    return g;
}

void ui_gauge_set(const ui_gauge_t *g, int32_t value, const char *text)
{
    lv_arc_set_value(g->arc, value);
    lv_label_set_text(g->value, text);
    lv_obj_align(g->value, LV_ALIGN_CENTER, 0, -4);
}

/* ---- Navigation ----------------------------------------------------------------- */

static void nav_click_cb(lv_event_t *e)
{
    ui_pages_show((unsigned)(uintptr_t)lv_event_get_user_data(e));
}

static void style_nav_button(lv_obj_t *b, const ui_design_t *d, int active)
{
    if (active) {
        lv_obj_set_style_bg_color(b, ui_col(d->accent), 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(b, ui_col(d->accent_text), 0);
    } else {
        lv_obj_set_style_bg_opa(b, LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(b, ui_col(d->text_dim), 0);
    }
    lv_obj_set_style_border_width(b, 0, 0);
    lv_obj_set_style_bg_color(b, ui_col(d->accent), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_set_style_text_color(b, ui_col(d->accent_text), LV_STATE_PRESSED);
}

static lv_obj_t *nav_button(lv_obj_t *parent, const ui_design_t *d, const char *text,
                            int32_t x, int32_t y, int32_t w, int32_t h, uintptr_t id, int active)
{
    lv_obj_t *b = ui_button(parent, d, text, x, y, w, h);
    style_nav_button(b, d, active);
    lv_obj_add_event_cb(b, nav_click_cb, LV_EVENT_CLICKED, (void *)id);
    return b;
}

/* Seitenleiste: Symbol (gross) und Name (klein) uebereinander */
static void rail_button(lv_obj_t *parent, const ui_design_t *d, const char *icon, const char *name,
                        int32_t y, int32_t h, uintptr_t id, int active)
{
    lv_obj_t *b = nav_button(parent, d, "", 4, y, SIDE_RAIL_W - 8, h, id, active);
    lv_obj_t *old = lv_obj_get_child(b, 0);
    lv_obj_delete(old);
    lv_obj_t *i = lv_label_create(b);
    lv_label_set_text(i, icon);
    lv_obj_set_style_text_font(i, d->font_title, 0);
    lv_obj_align(i, LV_ALIGN_TOP_MID, 0, 7);
    lv_obj_t *n = lv_label_create(b);
    lv_label_set_text(n, name);
    lv_obj_set_style_text_font(n, d->font_small, 0);
    lv_obj_align(n, LV_ALIGN_BOTTOM_MID, 0, -5);
}

static lv_obj_t *bar(lv_obj_t *scr, const ui_design_t *d, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *b = lv_obj_create(scr);
    plain(b);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_bg_color(b, ui_col(d->nav_bg), 0);
    lv_obj_set_style_bg_opa(b, d->bg_gradient ? LV_OPA_70 : LV_OPA_COVER, 0);
    return b;
}

static void line(lv_obj_t *scr, const ui_design_t *d, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t *l = lv_obj_create(scr);
    plain(l);
    lv_obj_set_pos(l, x, y);
    lv_obj_set_size(l, w, h);
    lv_obj_set_style_bg_color(l, ui_col(d->border), 0);
    lv_obj_set_style_bg_opa(l, d->border_opa, 0);
}

ui_frame_t ui_frame_create(const ui_design_t *d, unsigned page_idx)
{
    ui_frame_t f;
    f.screen = lv_obj_create(NULL);
    plain(f.screen);
    lv_obj_set_style_bg_color(f.screen, ui_col(d->bg), 0);
    lv_obj_set_style_bg_opa(f.screen, LV_OPA_COVER, 0);
    if (d->bg_gradient) {
        lv_obj_set_style_bg_grad_color(f.screen, ui_col(d->bg2), 0);
        lv_obj_set_style_bg_grad_dir(f.screen, LV_GRAD_DIR_VER, 0);
    }

    /* Seitenleiste links: Symbol und Name je Seite */
    const unsigned n = ui_pages_count();
    lv_obj_t *b = bar(f.screen, d, 0, 0, SIDE_RAIL_W, SCREEN_H);
    const int32_t item_h = 58;
    for (unsigned i = 0; i < n; i++) {
        rail_button(b, d, ui_pages_icon(i), ui_pages_name(i), 6 + (int32_t)i * (item_h + 4), item_h, i, i == page_idx);
    }
    line(f.screen, d, SIDE_RAIL_W - 1, 0, 1, SCREEN_H);

    f.w = SCREEN_W - SIDE_RAIL_W;
    f.h = SCREEN_H;
    f.content = lv_obj_create(f.screen);
    plain(f.content);
    lv_obj_set_pos(f.content, SIDE_RAIL_W, 0);
    lv_obj_set_size(f.content, f.w, f.h);
    return f;
}
