/* Seite DASHBOARD (Projekt Terrarium): Beleuchtung. Drei Schieberegler (R, G, B) stellen die Farbe fuer das ganze
 * WS2812-Modul ein; ein Farbfeld zeigt die gewaehlte Farbe, "Aus" setzt alles auf 0.
 * Der Zustand (Farbe) steht in s_rgb und nicht in den Widgets, damit die Seite beliebig neu aufgebaut werden kann.
 * Die LEDs folgen den Reglern sofort (der Treiber begrenzt die Helligkeit, siehe bsp_ws2812.h). */
#include "pages.h"
#include "ui_widgets.h"
#include "bsp_ws2812.h"

#include <stdio.h>
#include <string.h>

static uint8_t s_rgb[3];                         /* Sollfarbe R, G, B */

static struct {
    lv_obj_t *slider[3];
    lv_obj_t *value[3];
    lv_obj_t *swatch;
    lv_obj_t *hex;
} w;

static void refresh(void)
{
    char t[16];
    for (unsigned i = 0; i < 3; i++) {
        snprintf(t, sizeof t, "%u", (unsigned)s_rgb[i]);
        lv_label_set_text(w.value[i], t);
    }
    lv_obj_set_style_bg_color(w.swatch, lv_color_make(s_rgb[0], s_rgb[1], s_rgb[2]), 0);
    snprintf(t, sizeof t, "#%02X%02X%02X", s_rgb[0], s_rgb[1], s_rgb[2]);
    lv_label_set_text(w.hex, t);
}

static void apply(void)
{
    refresh();
    (void)bsp_ws2812_fill(s_rgb[0], s_rgb[1], s_rgb[2]);
}

static void slider_cb(lv_event_t *e)
{
    const unsigned ch = (unsigned)(uintptr_t)lv_event_get_user_data(e);
    s_rgb[ch] = (uint8_t)lv_slider_get_value(lv_event_get_target_obj(e));
    apply();
}

static void off_cb(lv_event_t *e)
{
    (void)e;
    memset(s_rgb, 0, sizeof s_rgb);
    for (unsigned i = 0; i < 3; i++) {
        lv_slider_set_value(w.slider[i], 0, LV_ANIM_OFF);
    }
    apply();
}

lv_obj_t *page_dashboard_create(const ui_design_t *d, unsigned idx)
{
    memset(&w, 0, sizeof w);
    ui_frame_t f = ui_frame_create(d, idx);
    lv_obj_t *c = f.content;

    lv_obj_set_pos(ui_label(c, d, UI_TEXT_TITLE, "Beleuchtung"), 16, 10);

    lv_obj_t *card = ui_card(c, d, 8, 46, f.w - 16, 218);

    static const char *const k_name[3] = { "R", "G", "B" };
    const uint32_t color[3] = { d->bad, d->ok, 0x6BA8FF };
    const int32_t sx = 44, sw = 190;
    for (unsigned i = 0; i < 3; i++) {
        const int32_t y = 26 + (int32_t)i * 56;
        lv_obj_t *n = ui_label(card, d, UI_TEXT_NORMAL, k_name[i]);
        lv_obj_set_style_text_color(n, ui_col(color[i]), 0);
        lv_obj_set_pos(n, 16, y - 6);

        w.slider[i] = ui_slider(card, d, sx, y, sw, 0, 255, color[i]);
        lv_slider_set_value(w.slider[i], s_rgb[i], LV_ANIM_OFF);
        lv_obj_add_event_cb(w.slider[i], slider_cb, LV_EVENT_VALUE_CHANGED, (void *)(uintptr_t)i);

        w.value[i] = ui_label(card, d, UI_TEXT_NORMAL, "0");
        lv_obj_set_pos(w.value[i], sx + sw + 28, y - 6);
        lv_obj_set_width(w.value[i], 44);
    }

    /* Farbfeld mit Hex-Wert rechts */
    const int32_t fx = f.w - 16 - 16 - 72;
    w.swatch = lv_obj_create(card);
    lv_obj_remove_style_all(w.swatch);
    lv_obj_set_pos(w.swatch, fx, 22);
    lv_obj_set_size(w.swatch, 72, 72);
    lv_obj_set_style_radius(w.swatch, d->radius, 0);
    lv_obj_set_style_bg_opa(w.swatch, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(w.swatch, d->border_w, 0);
    lv_obj_set_style_border_color(w.swatch, ui_col(d->border), 0);
    lv_obj_set_style_border_opa(w.swatch, d->border_opa, 0);
    w.hex = ui_label(card, d, UI_TEXT_SMALL_DIM, "#000000");
    lv_obj_set_width(w.hex, 72);
    lv_obj_set_style_text_align(w.hex, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(w.hex, fx, 100);

    lv_obj_t *off = ui_button(card, d, "Aus", 16, 166, 96, 36);
    lv_obj_add_event_cb(off, off_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *hint = ui_label(card, d, UI_TEXT_SMALL_DIM, "LED-Helligkeit auf 50 % begrenzt");
    lv_obj_set_pos(hint, 128, 176);

    refresh();
    return f.screen;
}

void page_dashboard_update(void)
{
    /* nichts zyklisch zu aktualisieren: die Seite zeigt nur den eingestellten Zustand */
}
