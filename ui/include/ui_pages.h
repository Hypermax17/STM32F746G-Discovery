#ifndef UI_PAGES_H
#define UI_PAGES_H

#include "ui_design.h"

/* Seitenverwaltung: mehrere Seiten mit unterschiedlichen Aufgaben. Jede Seite wird durch zwei Funktionen
 * beschrieben. Der Zustand der Seite (Daten) gehoert nicht in die Widgets, damit eine Seite jederzeit mit einem
 * anderen Design neu aufgebaut werden kann. Alle Funktionen nur aus dem UI-Task aufrufen. */

typedef lv_obj_t *(*ui_page_create_fn)(const ui_design_t *design, unsigned page_idx);   /* liefert den Bildschirm */
typedef void (*ui_page_update_fn)(void);                                              /* periodisch, solange sichtbar */

typedef struct {
    const char       *name;       /* Text in der Navigation */
    const char       *icon;       /* LVGL-Symbol (z. B. LV_SYMBOL_LIST) fuer die Seitenleiste */
    ui_page_create_fn create;
    ui_page_update_fn update;     /* darf NULL sein */
} ui_page_def_t;

/* design_idx: Startdesign je Seite (Index in ui_designs[]) */
void ui_pages_init(const ui_page_def_t *defs, const uint8_t *design_idx, unsigned count);

void ui_pages_show(unsigned idx);
void ui_pages_next(void);
void ui_pages_next_design(unsigned idx);      /* Seite idx mit dem naechsten Design neu aufbauen */

unsigned    ui_pages_count(void);
unsigned    ui_pages_current(void);
const char *ui_pages_name(unsigned idx);
const char *ui_pages_icon(unsigned idx);
const ui_design_t *ui_pages_design(unsigned idx);

#endif
