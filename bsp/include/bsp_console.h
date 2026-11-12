#ifndef BSP_CONSOLE_H
#define BSP_CONSOLE_H

#include <stdint.h>
#include <stddef.h>

/* Text-Konsole auf dem LCD (kein UART!). Schrift 5x7 skaliert, Zelle 12x16 Pixel -> 40 Spalten x 17 Zeilen.
 *   Zeile 0        : Tab-Leiste der Applikation (wird nicht von der Konsole beschrieben)
 *   Zeilen 1..15   : scrollender Log-Bereich
 *   Zeile 16       : Statuszeile (invertiert)
 * Aller Text wird zusaetzlich in einem Ringpuffer (64 Zeilen) gespeichert. Ist die Konsole nicht
 * sichtbar (bsp_console_set_visible(0)), wird nur gespeichert; beim Wiedereinblenden wird der Log-Bereich
 * aus dem Ringpuffer neu gezeichnet. Aufrufe sind Thread-sicher, sobald der Scheduler laeuft. */

#define BSP_CONSOLE_FONT_SCALE 2U
#define BSP_CONSOLE_CELL_W (6U * BSP_CONSOLE_FONT_SCALE)
#define BSP_CONSOLE_CELL_H (8U * BSP_CONSOLE_FONT_SCALE)
#define BSP_CONSOLE_COLS (480U / BSP_CONSOLE_CELL_W)
#define BSP_CONSOLE_ROWS (272U / BSP_CONSOLE_CELL_H)

#define BSP_CONSOLE_FIRST_LOG_ROW 1U
#define BSP_CONSOLE_LAST_LOG_ROW  (BSP_CONSOLE_ROWS - 2U)
#define BSP_CONSOLE_STATUS_ROW    (BSP_CONSOLE_ROWS - 1U)

int  bsp_console_init(void);
int  bsp_console_ready(void);

/* Text in der Farbe 'color' (RGB565) ausgeben; '\n' beginnt eine neue Zeile. */
void bsp_console_write(const char *text, uint16_t color);

/* Statuszeile (unterste Zeile) ueberschreiben. */
void bsp_console_status(const char *text);

/* Log-Bereich ein-/ausblenden. Beim Einblenden wird der Bereich geloescht und neu gezeichnet; beim
 * Ausblenden bleibt der Bildschirm unveraendert (die Applikation darf die Zeilen 1..15 beschreiben). */
void bsp_console_set_visible(int visible);
int  bsp_console_is_visible(void);

/* Log-Zugriff fuer eine eigene Oberflaeche: Jede begonnene Zeile hat eine fortlaufende Nummer (die erste ist 0).
 * bsp_console_line_total() liefert die Nummer der aktuellen (letzten) Zeile. bsp_console_read_line() kopiert
 * Zeile 'serial' (Text + Farbe des ersten Zeichens, RGB565); Rueckgabe 0 = ok, -1 = Zeile nicht mehr im
 * Ringpuffer (die letzten 64 Zeilen sind verfuegbar). Mit bsp_console_set_visible(0) zeichnet die Konsole nicht
 * mehr, speichert aber weiter. */
uint32_t bsp_console_line_total(void);
int bsp_console_read_line(uint32_t serial, char *out, size_t out_size, uint16_t *color);

/* Ohne Sperre und ohne Scheduler-Abhaengigkeit: fuer Fehlerbehandlung (Panic, Fault-Handler). */
void bsp_console_write_unlocked(const char *text, uint16_t color);
void bsp_console_status_unlocked(const char *text);

/* Fehleranzeige: Konsole wird sichtbar und belegt die Zeilen 0..15 (Tab-Leiste ueberschrieben),
 * Ringpuffer und Bildschirm werden geloescht. */
void bsp_console_clear_unlocked(void);

#endif
