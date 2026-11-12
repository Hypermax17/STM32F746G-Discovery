#ifndef UI_PORT_H
#define UI_PORT_H

#include "bsp_touch.h"

/* Anbindung von LVGL an das Board: Display (LTDC, zwei Framebuffer im SDRAM, Umschalten im vertikalen
 * Austastintervall), Touch (Zeigereingabe), Zeitbasis und Log. LVGL ist nicht thread-sicher: alle LVGL-Aufrufe
 * erfolgen im UI-Task, andere Tasks nutzen nur die Funktionen dieser Datei. */

/* Erzeugt den UI-Task. app_init wird dort einmal aufgerufen, nachdem LVGL bereit ist (hier Seiten anlegen
 * und anzeigen). Blendet die Konsole aus; Meldungen bleiben im Ringpuffer (bsp_console_read_line). */
void ui_port_start(void (*app_init)(void));

/* Aus dem Touch-Task: aktueller Zustand (nur der erste Punkt steuert LVGL) */
void ui_port_touch_update(const bsp_touch_state_t *state);

/* Aus beliebiger Task (z. B. Taster): naechste Seite anzeigen */
void ui_port_request_next_page(void);

#endif
