#include "ui_widgets.h"
#include "ui_pages.h"

#include <stdint.h>

#define SCREEN_W 480
#define SCREEN_H 272

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
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, d->radius, 0);
    lv_obj_set_style_border_width(o, d->border_w, 0);
    lv_obj_set_style_border_color(o, ui_col(d->border), 0);
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
    lv_obj_set_style_bg_color(b, ui_col(d->panel), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(b, ui_col(d->text), 0);
    lv_obj_set_style_text_font(b, d->font, 0);
    /* gedrueckt: Akzentflaeche */
    lv_obj_set_style_bg_color(b, ui_col(d->accent), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(b, ui_col(d->accent), LV_STATE_PRESSED);
    lv_obj_set_style_text_color(b, ui_col(d->accent_text), LV_STATE_PRESSED);

    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_center(l);
    return b;
}

/* ---- Tabelle / Diagramm ----------------------------------------------------- */

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
    lv_obj_set_style_line_width(c, 1, LV_PART_MAIN);
    lv_obj_set_style_line_width(c, 2, LV_PART_ITEMS);                      /* Kurven */
    lv_obj_set_style_size(c, 0, 0, LV_PART_INDICATOR);                     /* keine Punktmarken */
    return c;
}

/* ---- Navigation ----------------------------------------------------------------- */

#define NAV_DESIGN_BASE 100U

static void nav_click_cb(lv_event_t *e)
{
    const uintptr_t id = (uintptr_t)lv_event_get_user_data(e);
    if (id >= NAV_DESIGN_BASE) {
        ui_pages_next_design((unsigned)(id - NAV_DESIGN_BASE));
    } else {
        ui_pages_show((unsigned)id);
    }
}

static void style_nav_button(lv_obj_t *b, const ui_design_t *d, int active, int flat)
{
    if (active) {
        lv_obj_set_style_bg_color(b, ui_col(d->accent), 0);
        lv_obj_set_style_bg_opa(b, flat ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(b, ui_col(flat ? d->accent : d->accent_text), 0);
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
                            int32_t x, int32_t y, int32_t w, int32_t h, uintptr_t id, int active, int flat)
{
    lv_obj_t *b = ui_button(parent, d, text, x, y, w, h);
    style_nav_button(b, d, active, flat);
    lv_obj_add_event_cb(b, nav_click_cb, LV_EVENT_CLICKED, (void *)id);
    return b;
}

static lv_obj_t *bar(lv_obj_t *scr, const ui_design_t *d, int32_t y, int32_t h)
{
    lv_obj_t *b = lv_obj_create(scr);
    plain(b);
    lv_obj_set_pos(b, 0, y);
    lv_obj_set_size(b, SCREEN_W, h);
    lv_obj_set_style_bg_color(b, ui_col(d->nav_bg), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    return b;
}

static void line(lv_obj_t *scr, const ui_design_t *d, int32_t y)
{
    lv_obj_t *l = lv_obj_create(scr);
    plain(l);
    lv_obj_set_pos(l, 0, y);
    lv_obj_set_size(l, SCREEN_W, 1);
    lv_obj_set_style_bg_color(l, ui_col(d->border), 0);
    lv_obj_set_style_bg_opa(l, LV_OPA_COVER, 0);
}

ui_frame_t ui_frame_create(const ui_design_t *d, unsigned page_idx)
{
    ui_frame_t f;
    f.screen = lv_obj_create(NULL);
    plain(f.screen);
    lv_obj_set_style_bg_color(f.screen, ui_col(d->bg), 0);
    lv_obj_set_style_bg_opa(f.screen, LV_OPA_COVER, 0);

    const unsigned n = ui_pages_count();
    int32_t cy = 0;
    char buf[24];

    switch (d->nav) {
    case UI_NAV_TOP: {
        const int32_t bh = 32;
        lv_obj_t *b = bar(f.screen, d, 0, bh);
        for (unsigned i = 0; i < n; i++) {
            nav_button(b, d, ui_pages_name(i), 8 + (int32_t)i * 104, 4, 100, 24, i, i == page_idx, 0);
        }
        lv_snprintf(buf, sizeof buf, "Design %c", d->letter);
        nav_button(b, d, buf, SCREEN_W - 8 - 84, 4, 84, 24, NAV_DESIGN_BASE + page_idx, 0, 0);
        line(f.screen, d, bh - 1);
        cy = bh;
        f.h = SCREEN_H - bh;
        break;
    }
    case UI_NAV_BOTTOM: {
        const int32_t bh = 40;
        const int32_t y0 = SCREEN_H - bh;
        lv_obj_t *b = bar(f.screen, d, y0, bh);
        const int32_t bw = SCREEN_W / (int32_t)(n + 1U);
        for (unsigned i = 0; i < n; i++) {
            lv_obj_t *nb = nav_button(b, d, ui_pages_name(i), (int32_t)i * bw, 0, bw, bh, i, i == page_idx, 1);
            if (i == page_idx) {                          /* Akzentstrich oben */
                lv_obj_t *s = lv_obj_create(b);
                plain(s);
                lv_obj_set_pos(s, (int32_t)i * bw + 12, 0);
                lv_obj_set_size(s, bw - 24, 3);
                lv_obj_set_style_bg_color(s, ui_col(d->accent), 0);
                lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
                (void)nb;
            }
        }
        lv_snprintf(buf, sizeof buf, "Design %c", d->letter);
        nav_button(b, d, buf, (int32_t)n * bw, 0, SCREEN_W - (int32_t)n * bw, bh, NAV_DESIGN_BASE + page_idx, 0, 1);
        line(f.screen, d, y0);
        cy = 0;
        f.h = SCREEN_H - bh;
        break;
    }
    case UI_NAV_TEXT:
    default: {
        const int32_t bh = 24;
        const int32_t y0 = SCREEN_H - bh;
        lv_obj_t *b = bar(f.screen, d, y0, bh);
        int32_t x = 4;
        for (unsigned i = 0; i < n; i++) {
            lv_snprintf(buf, sizeof buf, "[%u] %s", i + 1U, ui_pages_name(i));
            lv_point_t sz;
            lv_text_get_size(&sz, buf, d->font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
            const int32_t w = sz.x + 16;
            nav_button(b, d, buf, x, 0, w, bh, i, i == page_idx, 0);
            x += w + 4;
        }
        lv_snprintf(buf, sizeof buf, "[D] %c", d->letter);
        nav_button(b, d, buf, SCREEN_W - 84, 0, 80, bh, NAV_DESIGN_BASE + page_idx, 0, 0);
        line(f.screen, d, y0);
        cy = 0;
        f.h = SCREEN_H - bh;
        break;
    }
    }

    f.w = SCREEN_W;
    f.content = lv_obj_create(f.screen);
    plain(f.content);
    lv_obj_set_pos(f.content, 0, cy);
    lv_obj_set_size(f.content, f.w, f.h);
    return f;
}
