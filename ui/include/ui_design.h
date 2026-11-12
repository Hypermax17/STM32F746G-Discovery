#ifndef UI_DESIGN_H
#define UI_DESIGN_H

#include "lvgl.h"

/* Ein "Design" ist eine Sammlung aus Farben, Schriften, Rundungen, Navigationsstil und Seitenlayout.
 * Alle Seiten und Widgets werden ueber ein ui_design_t aufgebaut; ein anderes Design aendert das Aussehen
 * ohne Aenderung am Seitencode (ausser dort, wo ein Layout-Stil eine eigene Anordnung verlangt).
 * Die Farben stehen als 0xRRGGBB, damit die Tabellen konstant bleiben. */

typedef enum {
    UI_NAV_TOP,       /* Reiter oben (Hoehe 32 px) */
    UI_NAV_SIDE       /* Seitenleiste links (Breite 76 px) mit Symbol und Name */
} ui_nav_style_t;

typedef enum {
    UI_LAYOUT_CARDS,      /* Karten mit Tabellen und Diagrammen */
    UI_LAYOUT_COCKPIT     /* grosse Zahlen und runde Anzeigen */
} ui_layout_t;

typedef struct {
    const char *name;                 /* z. B. "Dashboard dunkel" */
    char        letter;               /* Kurzzeichen fuer die Anzeige ('A', 'B', 'C') */

    uint32_t bg;                      /* Seitenhintergrund (bei Verlauf: oberer Wert) */
    uint32_t bg2;                     /* unterer Wert des Hintergrundverlaufs */
    uint8_t  bg_gradient;             /* 1 = senkrechter Verlauf bg -> bg2 */
    uint32_t panel;                   /* Karten / Felder */
    uint8_t  panel_opa;               /* Deckkraft der Karten (255 = deckend, kleiner = durchscheinend) */
    uint32_t border;                  /* Rahmen, Trennlinien, Gitter */
    uint8_t  border_opa;
    uint8_t  shadow_w;                /* Schattenbreite der Karten in px (0 = kein Schatten) */
    uint32_t text;                    /* Haupttext */
    uint32_t text_dim;                /* Nebentext */
    uint32_t accent;                  /* Akzent (aktive Navigation, Hauptkurve) */
    uint32_t accent_text;             /* Text auf Akzentflaechen */
    uint32_t ok, warn, bad;           /* Statusfarben (gut / Achtung / Fehler); warn ist auch die 2. Kurve */
    uint32_t nav_bg;                  /* Hintergrund der Navigation */

    uint8_t radius;                   /* Eckenradius der Karten/Buttons */
    uint8_t border_w;                 /* Rahmenstaerke */

    const lv_font_t *font_small;      /* Tabellen, Details */
    const lv_font_t *font;            /* Standardtext, Buttons */
    const lv_font_t *font_big;        /* grosse Messwerte */
    const lv_font_t *font_title;      /* Ueberschriften */
    const lv_font_t *font_hero;       /* sehr grosse Zahlen (Cockpit) */

    ui_nav_style_t nav;
    ui_layout_t    layout;
} ui_design_t;

extern const ui_design_t ui_design_dark;       /* A: dunkles Dashboard, Reiter oben (blau/cyan) */
extern const ui_design_t ui_design_cockpit;    /* B: Cockpit, Graphit mit gruenen Akzenten, Seitenleiste */
extern const ui_design_t ui_design_glass;      /* C: Glas mit Verlauf (violett), Seitenleiste */

#define UI_DESIGN_COUNT 3U
extern const ui_design_t *const ui_designs[UI_DESIGN_COUNT];

static inline lv_color_t ui_col(uint32_t rgb) { return lv_color_hex(rgb); }

#endif
