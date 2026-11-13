#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

/* Flash-Aufteilung (STM32F746: 1 MB, eine Bank, Sektoren 4x32K, 1x128K, 3x256K)
 *
 *   0x08000000  256 KB  Bootloader  (Sektor 0..4)  - wird im Betrieb nie ueberschrieben
 *   0x08040000  768 KB  Applikation (Sektor 5..7)
 *
 * Die Werte stehen auch in den Linker-Skripten (linker/bootloader.ld, linker/app.ld). */
#define IMAGE_BOOT_BASE        0x08000000UL
#define IMAGE_BOOT_SIZE        0x00040000UL
#define IMAGE_APP_BASE         0x08040000UL
#define IMAGE_APP_MAX_SIZE     0x000C0000UL

/* Der Header liegt im Image direkt hinter der Vektortabelle (feste Position, siehe linker/app.ld). */
#define IMAGE_HEADER_OFFSET    0x200UL
#define IMAGE_MAGIC            0x31474D49UL      /* "IMG1" */
#define IMAGE_HEADER_VERSION   1UL

/* Image-Aufbau (Datei = Flash-Inhalt ab IMAGE_APP_BASE):
 *   [0x000] Vektortabelle der Applikation
 *   [0x200] image_header_t (64 Byte)
 *   ...     Programm
 *   [size-4] CRC32 (little endian) ueber alle Bytes davor
 * image_size im Header zaehlt das CRC-Wort mit. Es wird beim Bauen von scripts/make_image.py
 * eingetragen (die Applikation selbst kennt ihre Groesse nicht). */
typedef struct {
    uint32_t magic;
    uint32_t header_version;
    uint32_t image_size;          /* Bytes inkl. abschliessendem CRC32; 0 = noch nicht fertiggestellt */
    uint32_t load_addr;           /* Linkadresse der Vektortabelle */
    char     version[32];         /* git describe */
    uint32_t reserved[4];
} image_header_t;

typedef struct {
    uint32_t size;                /* Bytes inkl. CRC */
    uint32_t crc32;
    char     version[33];
} image_info_t;

enum {
    IMAGE_OK             = 0,
    IMAGE_ERR_NO_HEADER  = -1,    /* Magic/Header-Version stimmt nicht (leerer Flash, fremde Firmware) */
    IMAGE_ERR_SIZE       = -2,    /* Groesse unplausibel oder zu gross */
    IMAGE_ERR_ADDR       = -3,    /* fuer eine andere Adresse gelinkt */
    IMAGE_ERR_VECTORS    = -4,    /* Stackpointer/Reset-Vektor unplausibel */
    IMAGE_ERR_CRC        = -5     /* Pruefsumme falsch */
};

/* CRC-32 (IEEE 802.3, wie zlib.crc32); fortsetzbar: crc = crc32_update(crc, ...), Start mit 0 */
uint32_t crc32_update(uint32_t crc, const void *data, uint32_t len);

/* Prueft ein Image im Speicher. data = Zeiger auf das erste Byte, load_addr = Adresse, an der das
 * Image im Flash liegen soll, max_size = verfuegbare Bytes. info darf NULL sein. */
int image_check(const void *data, uint32_t load_addr, uint32_t max_size, image_info_t *info);

const char *image_error_text(int rc);

#endif
