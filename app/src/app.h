#ifndef APP_H
#define APP_H

void app_init(void);

/* Wechselt das Blinktempo der LED LD1 zwischen langsam (500 ms) und schnell (100 ms). */
void app_heartbeat_toggle_rate(void);

#endif
