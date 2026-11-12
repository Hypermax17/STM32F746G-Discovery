#ifndef UI_DESIGN_H
#define UI_DESIGN_H

#include "lvgl.h"

/* Ein "Design" ist eine Sammlung aus Farben, Schriften, Rundungen und dem Navigationsstil.
 * Alle Seiten und Widgets werden ueber ein ui_design_t aufgebaut; ein anderes Design aendert das Aussehen
 * ohne Aenderung am Seitencode. Die Farben stehen als 0xRRGGBB, damit die Tabellen konstant bleiben. */

typedef enum {
    UI_NAV_TOP,       /* Reiter oben */
    UI_NAV_BOTTOM,    /* Leiste unten mit Schaltflaechen */
    UI_NAV_TEXT       /* Textzeile unten, Terminal-Stil: "[1] SYSTEM  [2] ..." */
} ui_nav_style_t;

typedef struct {
    const char *name;                 /* z. B. "Dashboard dunkel" */
    char        letter;               /* Kurzzeichen fuer die Anzeige ('A', 'B', 'C') */

    uint32_t bg;                      /* Seitenhintergrund */
    uint32_t panel;                   /* Karten / Felder */
    uint32_t border;                  /* Rahmen, Trennlinien, Gitter */
    uint32_t text;                    /* Haupttext */
    uint32_t text_dim;                /* Nebentext */
    uint32_t accent;                  /* Akzent (aktiver Reiter, Hauptkurve) */
    uint32_t accent_text;             /* Text auf Akzentflaechen */
    uint32_t ok, warn, bad;           /* Statusfarben (gut / Achtung / Fehler); warn ist auch die 2. Kurve */
    uint32_t nav_bg;                  /* Hintergrund der Navigation */

    uint8_t radius;                   /* Eckenradius der Karten/Buttons */
    uint8_t border_w;                 /* Rahmenstaerke */
    uint8_t pad;                      /* Standardabstand */

    const lv_font_t *font_small;      /* Tabellen, Details */
    const lv_font_t *font;            /* Standardtext, Buttons */
    const lv_font_t *font_big;        /* grosse Messwerte */
    const lv_font_t *font_title;      /* Ueberschriften */

    ui_nav_style_t nav;
} ui_design_t;

extern const ui_design_t ui_design_dark;       /* A: dunkles Dashboard, Reiter oben */
extern const ui_design_t ui_design_light;      /* B: helles, flaches Design, Leiste unten */
extern const ui_design_t ui_design_terminal;   /* C: Terminal (gruen auf schwarz), Textzeile unten */

#define UI_DESIGN_COUNT 3U
extern const ui_design_t *const ui_designs[UI_DESIGN_COUNT];

static inline lv_color_t ui_col(uint32_t rgb) { return lv_color_hex(rgb); }

#endif
