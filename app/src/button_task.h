#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

/* Wertet den User-Taster aus: kurzer Druck wechselt zur naechsten Seite der Oberflaeche,
 * ein langer Druck (>= 1 s) gibt eine Audio-Diagnose aus,
 * ein sehr langer Druck (>= 3 s) startet den QSPI-Schreibtest (nur in leeren Sektoren). */
void button_task_start(void);

#endif
