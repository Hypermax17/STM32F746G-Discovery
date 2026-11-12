#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stdint.h>

/* I2C3 (PH7 = SCL, PH8 = SDA): gemeinsamer Bus von Touch-Controller FT5336 (7-Bit-Adresse 0x38)
 * und Audio-Codec WM8994 (7-Bit-Adresse 0x1A). Zugriffe sind Thread-sicher
 * (Mutex, sobald der Scheduler laeuft). 100 kHz, Polling. */

/* Rueckgabe 0 = ok */
int bsp_i2c3_init(void);

/* Register lesen/schreiben (8-Bit-Registeradresse). Rueckgabe 0 = ok, sonst Fehler. */
int bsp_i2c3_mem_read(uint8_t addr7, uint8_t reg, uint8_t *buf, uint16_t len);
int bsp_i2c3_mem_write(uint8_t addr7, uint8_t reg, const uint8_t *buf, uint16_t len);

/* Geraete mit 16-Bit-Registeradresse und 16-Bit-Wert (z. B. Audio-Codec WM8994, MSB zuerst) */
int bsp_i2c3_reg16_read(uint8_t addr7, uint16_t reg, uint16_t *value);
int bsp_i2c3_reg16_write(uint8_t addr7, uint16_t reg, uint16_t value);

/* 0 = Geraet antwortet */
int bsp_i2c3_probe(uint8_t addr7);

#endif
