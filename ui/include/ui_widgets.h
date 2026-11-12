#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "ui_design.h"

/* Konfigurierbare Basiselemente. Alle Funktionen nehmen das Design als Parameter und liefern LVGL-Objekte,
 * die danach wie gewohnt angepasst werden koennen (Position, Groesse, Ereignisse). */

typedef enum {
    UI_TEXT_NORMAL,
    UI_TEXT_DIM,       /* Nebentext */
    UI_TEXT_SMALL,     /* kleine Schrift, Haupttext */
    UI_TEXT_SMALL_DIM,
    UI_TEXT_BIG,       /* grosser Messwert */
    UI_TEXT_TITLE,     /* Ueberschrift */
    UI_TEXT_HERO,      /* sehr grosse Zahl (Cockpit) */
    UI_TEXT_ACCENT,
    UI_TEXT_WARN,
    UI_TEXT_OK,
    UI_TEXT_BAD
} ui_text_kind_t;

/* Seitengeruest: Bildschirm mit Hintergrund, Navigation (gemaess Design.nav) und Inhaltsbereich */
typedef struct {
    lv_obj_t *screen;
    lv_obj_t *content;      /* Inhaltsbereich (ohne Scrollen/Rahmen) */
    int32_t   w, h;         /* Groesse des Inhaltsbereichs */
} ui_frame_t;

/* page_idx: Index dieser Seite (markiert den aktiven Reiter) */
ui_frame_t ui_frame_create(const ui_design_t *d, unsigned page_idx);

lv_obj_t *ui_card(lv_obj_t *parent, const ui_design_t *d, int32_t x, int32_t y, int32_t w, int32_t h);
lv_obj_t *ui_label(lv_obj_t *parent, const ui_design_t *d, ui_text_kind_t kind, const char *text);
lv_obj_t *ui_button(lv_obj_t *parent, const ui_design_t *d, const char *text,
                    int32_t x, int32_t y, int32_t w, int32_t h);
/* Farbiger Punkt (Status), Durchmesser dia */
lv_obj_t *ui_dot(lv_obj_t *parent, int32_t x, int32_t y, int32_t dia, uint32_t rgb);

/* Stil fuer Tabellen (ohne Kopfzeile; Spaltenueberschriften als Labels) und Diagramme */
lv_obj_t *ui_table(lv_obj_t *parent, const ui_design_t *d, int32_t x, int32_t y,
                   uint32_t rows, uint32_t cols, const int32_t *col_widths);
int32_t   ui_table_row_h(const ui_design_t *d);
/* Runde Anzeige (270-Grad-Bogen) mit Zahl in der Mitte und Titel darunter */
typedef struct {
    lv_obj_t *arc;
    lv_obj_t *value;
    lv_obj_t *title;
} ui_gauge_t;
ui_gauge_t ui_gauge(lv_obj_t *parent, const ui_design_t *d, int32_t x, int32_t y, int32_t size,
                    const char *title, int32_t min, int32_t max, uint32_t rgb);
void ui_gauge_set(const ui_gauge_t *g, int32_t value, const char *text);

lv_obj_t *ui_chart(lv_obj_t *parent, const ui_design_t *d, int32_t x, int32_t y, int32_t w, int32_t h,
                   uint32_t points);

void ui_label_set_kind_color(lv_obj_t *label, const ui_design_t *d, ui_text_kind_t kind);

#endif
