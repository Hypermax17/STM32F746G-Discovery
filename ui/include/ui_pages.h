#ifndef UI_PAGES_H
#define UI_PAGES_H

#include "ui_design.h"

/* Seitenverwaltung: mehrere Seiten mit unterschiedlichen Aufgaben. Jede Seite wird durch zwei Funktionen
 * beschrieben. Der Zustand der Seite (Daten) gehoert nicht in die Widgets, sondern in eigene Module
 * (Datenquellen), die Seite kann so jederzeit neu aufgebaut werden. Alle Funktionen nur aus dem UI-Task aufrufen. */

typedef lv_obj_t *(*ui_page_create_fn)(const ui_design_t *design, unsigned page_idx);   /* liefert den Bildschirm */
typedef void (*ui_page_update_fn)(void);                                              /* periodisch, solange sichtbar */

typedef struct {
    const char       *name;       /* Text in der Navigation */
    const char       *icon;       /* LVGL-Symbol (z. B. LV_SYMBOL_LIST) fuer die Seitenleiste */
    ui_page_create_fn create;
    ui_page_update_fn update;     /* darf NULL sein */
} ui_page_def_t;

/* design: Design, mit dem alle Seiten aufgebaut werden (normalerweise &ui_design) */
void ui_pages_init(const ui_page_def_t *defs, unsigned count, const ui_design_t *design);

void ui_pages_show(unsigned idx);
void ui_pages_next(void);

unsigned    ui_pages_count(void);
unsigned    ui_pages_current(void);
const char *ui_pages_name(unsigned idx);
const char *ui_pages_icon(unsigned idx);
const ui_design_t *ui_pages_design(void);

#endif
