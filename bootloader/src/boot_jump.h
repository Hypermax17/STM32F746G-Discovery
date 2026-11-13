#ifndef BOOT_JUMP_H
#define BOOT_JUMP_H

#include <stdint.h>

/* Versetzt den Controller in einen reset-aehnlichen Zustand (Takt auf HSI, Peripherie zurueckgesetzt,
 * Caches/MPU/Interrupts aus) und springt in die Applikation bei base. Kehrt nicht zurueck. */
void boot_jump_to_app(uint32_t base) __attribute__((noreturn));

#endif
