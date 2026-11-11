#ifndef BSP_QSPI_H
#define BSP_QSPI_H

#include <stdint.h>

/* Externer Quad-SPI-Flash MT25QL128 (128 Mbit = 16 MB) am QUADSPI (Bank 1):
 *   CLK = PB2, NCS = PB6, IO0 = PD11, IO1 = PD12, IO2 = PE2, IO3 = PD13.
 * Betrieb im Standard-SPI-Protokoll (Befehl auf 1 Leitung), Lesen mit "Quad Output Fast Read"
 * (0x6B: Daten ueber 4 Leitungen), Takt 108 MHz.
 *
 * Zwei Zugriffsarten:
 *  - indirekt: bsp_qspi_read/write/erase (Adressen = Offset 0..16 MB-1)
 *  - memory-mapped: Flash erscheint lesbar ab BSP_QSPI_BASE (0x90000000), Zugriff wie auf RAM
 * Schreiben/Loeschen schaltet das Memory-Mapping selbststaendig kurz ab und danach wieder an.
 * Die Funktionen sind Thread-sicher (Mutex). Bereiche muessen vor dem Schreiben geloescht (0xFF) sein. */

#define BSP_QSPI_BASE            0x90000000UL
#define BSP_QSPI_SIZE            (16UL * 1024UL * 1024UL)
#define BSP_QSPI_PAGE_SIZE       256UL
#define BSP_QSPI_SUBSECTOR_SIZE  4096UL        /* kleinste Loescheinheit */
#define BSP_QSPI_SECTOR_SIZE     65536UL

typedef struct {
    uint8_t manufacturer;   /* 0x20 = Micron */
    uint8_t type;           /* 0xBA = 3 V */
    uint8_t capacity;       /* 0x18 = 128 Mbit */
} bsp_qspi_id_t;

/* Initialisiert QUADSPI, setzt den Chip zurueck und liest die JEDEC-ID.
 * Rueckgabe 0 = Chip erkannt (Hersteller 0x20 und Kapazitaet 0x18), sonst Fehler. *id wird immer gefuellt. */
int bsp_qspi_init(bsp_qspi_id_t *id);
int bsp_qspi_available(void);

int bsp_qspi_read(uint32_t addr, void *buf, uint32_t len);
int bsp_qspi_write(uint32_t addr, const void *buf, uint32_t len);
int bsp_qspi_erase_subsector(uint32_t addr);   /* 4 KB */
int bsp_qspi_erase_sector(uint32_t addr);      /* 64 KB */

int bsp_qspi_memory_mapped_enable(void);
int bsp_qspi_memory_mapped_disable(void);
int bsp_qspi_is_memory_mapped(void);

#endif
