#ifndef UI_H
#define UI_H

#include "bsp_touch.h"

/* Bedienoberflaeche: Tab-Leiste mit den Seiten SYSTEM, TOUCH und LOG.
 * Seitenwechsel durch Antippen der Tab-Leiste oder per User-Taster (kurzer Druck). */

void ui_start(void);

/* Aus dem Touch-Task: aktueller Touch-Zustand (nicht blockierend; bei voller Queue verworfen) */
void ui_post_touch(const bsp_touch_state_t *state);

/* Aus dem Button-Task: zur naechsten Seite wechseln */
void ui_next_page(void);

#endif
