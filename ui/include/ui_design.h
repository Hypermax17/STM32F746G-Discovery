#ifndef UI_DESIGN_H
#define UI_DESIGN_H

#include "lvgl.h"

/* Das Basisdesign der Oberflaeche: Glas - Verlauf von tiefem Violett nach Nachtblau, durchscheinende Karten
 * mit hellem Rand und weichem Schatten, grosse Rundungen, Seitenleiste links.
 * Alle Seiten und Widgets werden ueber ein ui_design_t aufgebaut; Farben, Schriften und Rundungen lassen sich
 * dadurch an einer Stelle (ui/src/ui_design.c) aendern. Die Farben stehen als 0xRRGGBB. */

typedef struct {
    const char *name;

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
    uint32_t accent;                  /* Akzent (aktive Navigation, Hauptkurve, Hauptanzeige) */
    uint32_t accent_text;             /* Text auf Akzentflaechen */
    uint32_t ok, warn, bad;           /* Statusfarben (gut / Achtung / Fehler); warn ist auch die 2. Kurve */
    uint32_t nav_bg;                  /* Hintergrund der Seitenleiste */

    uint8_t radius;                   /* Eckenradius der Karten/Buttons */
    uint8_t border_w;                 /* Rahmenstaerke */

    const lv_font_t *font_small;      /* Tabellen, Details */
    const lv_font_t *font;            /* Standardtext, Buttons */
    const lv_font_t *font_big;        /* grosse Messwerte */
    const lv_font_t *font_title;      /* Ueberschriften */
} ui_design_t;

extern const ui_design_t ui_design;   /* das Basisdesign */

static inline lv_color_t ui_col(uint32_t rgb) { return lv_color_hex(rgb); }

#endif
