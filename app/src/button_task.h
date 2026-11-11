#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

/* Wertet den User-Taster aus: kurzer Druck wechselt das Blinktempo der LED,
 * ein langer Druck (>= 1 s) wird nur protokolliert. */
void button_task_start(void);

#endif
