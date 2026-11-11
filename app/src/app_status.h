#ifndef APP_STATUS_H
#define APP_STATUS_H

#include <stdint.h>

/* Statuszeile (unterste Displayzeile): "<System>   <Touch>".
 * Mehrere Tasks aktualisieren je ihren Teil; die Zeile wird zusammengesetzt. */

void app_status_init(void);
void app_status_set_system(uint32_t uptime_s, uint32_t heap_free_kb);
void app_status_set_touch(int active, unsigned x, unsigned y);

#endif
