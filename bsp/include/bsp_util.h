#ifndef BSP_UTIL_H
#define BSP_UTIL_H

#include <stddef.h>

/* Gleitkommazahl als Text mit fester Nachkommastellenzahl (0..6), z. B. 41.3 -> "41.3".
 * Noetig, weil das printf der newlib-nano kein %f kann (und die Float-Ausgabe malloc braucht).
 * Rundet kaufmaennisch, "-0.0" wird zu "0.0". Rueckgabe: Laenge des Textes (wie snprintf). */
int bsp_format_float(char *buf, size_t size, float value, unsigned decimals);

#endif
