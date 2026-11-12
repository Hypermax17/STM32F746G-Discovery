/* Seite LOG: Systemmeldungen (aus dem Ringpuffer der BSP-Konsole), farbig nach Schwere. */
#include "pages.h"
#include "ui_widgets.h"
#include "bsp_console.h"
#include "bsp_lcd.h"

#include <stdio.h>
#include <string.h>

#define MAX_LINES  60U       /* sichtbare Zeilen im Verlauf */
#define LINE_CHARS 80U

static int      s_paused;
static uint32_t s_next;      /* naechste noch nicht angezeigte Zeilennummer */

static struct {
    lv_obj_t *list;
    lv_obj_t *count;
    lv_obj_t *pause_label;
    const ui_design_t *d;
} w;

static ui_text_kind_t kind_for(uint16_t rgb565)
{
    if (rgb565 == BSP_COLOR_RED)    return UI_TEXT_BAD;
    if (rgb565 == BSP_COLOR_YELLOW) return UI_TEXT_WARN;
    if (rgb565 == BSP_COLOR_GRAY)   return UI_TEXT_DIM;
    return UI_TEXT_NORMAL;
}

static void append_line(const char *text, uint16_t color)
{
    lv_obj_t *l = ui_label(w.list, w.d, kind_for(color), text);
    lv_obj_set_width(l, LV_PCT(100));
    lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(l, w.d->font_small, 0);
    while (lv_obj_get_child_count(w.list) > MAX_LINES) {
        lv_obj_delete(lv_obj_get_child(w.list, 0));
    }
}

/* Alle fertigen Zeilen (bis total-1) uebernehmen; die aktuell geschriebene Zeile folgt, sobald sie fertig ist */
static void sync(void)
{
    const uint32_t total = bsp_console_line_total();
    char text[LINE_CHARS];
    uint16_t color;
    int added = 0;

    /* zu viele Zeilen verpasst (Ringpuffer 64): ab der ersten noch vorhandenen weitermachen */
    if (total > s_next && total - s_next > 60U) {
        s_next = total - 60U;
    }
    for (; s_next < total; s_next++) {
        if (bsp_console_read_line(s_next, text, sizeof text, &color) == 0 && text[0] != '\0') {
            append_line(text, color);
            added = 1;
        }
    }
    if (added) {
        lv_obj_scroll_to_y(w.list, LV_COORD_MAX, LV_ANIM_OFF);
    }
    char b[24];
    snprintf(b, sizeof b, "%lu Zeilen", (unsigned long)total);
    lv_label_set_text(w.count, b);
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
    lv_obj_clean(w.list);
    s_next = bsp_console_line_total();
}

lv_obj_t *page_log_create(const ui_design_t *d, unsigned idx)
{
    memset(&w, 0, sizeof w);
    w.d = d;
    ui_frame_t f = ui_frame_create(d, idx);
    lv_obj_t *c = f.content;

    lv_obj_t *title = ui_label(c, d, UI_TEXT_TITLE, "Systemlog");
    ui_label_set_kind_color(title, d, UI_TEXT_ACCENT);
    lv_obj_set_style_text_font(title, d->font_title, 0);
    lv_obj_set_pos(title, 10, 6);
    w.count = ui_label(c, d, UI_TEXT_DIM, "");
    lv_label_set_long_mode(w.count, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(w.count, LV_SIZE_CONTENT);
    lv_obj_align(w.count, LV_ALIGN_TOP_RIGHT, -10, 10);

    const int32_t top = 6 + (int32_t)lv_font_get_line_height(d->font_title) + 6;
    const int32_t lh = f.h - top - 42;
    lv_obj_t *card = ui_card(c, d, 8, top, f.w - 16, lh);
    w.list = lv_obj_create(card);
    lv_obj_remove_style_all(w.list);
    lv_obj_set_pos(w.list, 0, 0);
    lv_obj_set_size(w.list, f.w - 16 - 2 * d->border_w, lh - 2 * d->border_w);
    lv_obj_set_flex_flow(w.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(w.list, 4, 0);
    lv_obj_set_style_pad_row(w.list, 0, 0);
    lv_obj_add_flag(w.list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(w.list, LV_SCROLLBAR_MODE_OFF);

    const int32_t by = f.h - 36;
    lv_obj_t *bp = ui_button(c, d, s_paused ? "Weiter" : "Pause", 8, by, 100, 30);
    w.pause_label = lv_obj_get_child(bp, 0);
    lv_obj_add_event_cb(bp, pause_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bc = ui_button(c, d, "Loeschen", 116, by, 100, 30);
    lv_obj_add_event_cb(bc, clear_cb, LV_EVENT_CLICKED, NULL);

    /* Verlauf aus dem Ringpuffer nachladen */
    const uint32_t total = bsp_console_line_total();
    s_next = (total > 60U) ? total - 60U : 0U;
    sync();
    return f.screen;
}

void page_log_update(void)
{
    if (w.list == NULL || s_paused) {
        return;
    }
    sync();
}
