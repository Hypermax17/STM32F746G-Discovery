#ifndef BSP_CONSOLE_H
#define BSP_CONSOLE_H

#include <stdint.h>

/* Text-Konsole auf dem LCD (kein UART!). Schrift 5x7 skaliert, Zelle 12x16 Pixel
 * -> 40 Spalten x 17 Zeilen. Die unterste Zeile ist eine Statuszeile (invertiert),
 * die uebrigen 16 Zeilen scrollen. Aufrufe sind Thread-sicher, sobald der
 * Scheduler laeuft. */

#define BSP_CONSOLE_FONT_SCALE 2U
#define BSP_CONSOLE_COLS (480U / (6U * BSP_CONSOLE_FONT_SCALE))
#define BSP_CONSOLE_ROWS (272U / (8U * BSP_CONSOLE_FONT_SCALE))

int  bsp_console_init(void);
int  bsp_console_ready(void);

/* Text in der Farbe 'color' (RGB565) ausgeben; '\n' beginnt eine neue Zeile. */
void bsp_console_write(const char *text, uint16_t color);

/* Statuszeile (unterste Zeile) ueberschreiben. */
void bsp_console_status(const char *text);

/* Wie bsp_console_write, aber ohne Sperre und ohne Scheduler-Abhaengigkeit:
 * fuer Fehlerbehandlung (Panic, Fault-Handler). */
void bsp_console_write_unlocked(const char *text, uint16_t color);

/* Ohne Sperre (fuer Fehlerbehandlung): Statuszeile setzen bzw. Logbereich loeschen. */
void bsp_console_status_unlocked(const char *text);
void bsp_console_clear_unlocked(void);

#endif
