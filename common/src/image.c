#include "image.h"

#include <string.h>

/* Gueltige Stack-Anfangswerte: DTCM (64 KB) und SRAM (256 KB) */
#define RAM_START 0x20000000UL
#define RAM_END   0x20050000UL

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int image_check(const void *data, uint32_t load_addr, uint32_t max_size, image_info_t *info)
{
    const uint8_t *b = (const uint8_t *)data;
    image_header_t h;

    if (max_size < IMAGE_HEADER_OFFSET + sizeof h + 4U) {
        return IMAGE_ERR_SIZE;
    }
    memcpy(&h, b + IMAGE_HEADER_OFFSET, sizeof h);
    if (h.magic != IMAGE_MAGIC || h.header_version != IMAGE_HEADER_VERSION) {
        return IMAGE_ERR_NO_HEADER;
    }
    if (h.image_size < IMAGE_HEADER_OFFSET + sizeof h + 4U || h.image_size > max_size ||
        (h.image_size & 3U) != 0U) {
        return IMAGE_ERR_SIZE;
    }
    if (h.load_addr != load_addr) {
        return IMAGE_ERR_ADDR;
    }

    uint32_t sp    = rd32(b);
    uint32_t reset = rd32(b + 4);
    if (sp <= RAM_START || sp > RAM_END || (sp & 7U) != 0U ||
        (reset & 1U) == 0U || reset < load_addr || reset >= load_addr + h.image_size) {
        return IMAGE_ERR_VECTORS;
    }

    uint32_t calc   = crc32_update(0, b, h.image_size - 4U);
    uint32_t stored = rd32(b + h.image_size - 4U);
    if (calc != stored) {
        return IMAGE_ERR_CRC;
    }

    if (info != NULL) {
        info->size  = h.image_size;
        info->crc32 = stored;
        memcpy(info->version, h.version, sizeof h.version);
        info->version[sizeof h.version] = '\0';
    }
    return IMAGE_OK;
}

const char *image_error_text(int rc)
{
    switch (rc) {
    case IMAGE_OK:            return "ok";
    case IMAGE_ERR_NO_HEADER: return "kein Image-Header (leer/fremd)";
    case IMAGE_ERR_SIZE:      return "Groesse unplausibel";
    case IMAGE_ERR_ADDR:      return "falsche Linkadresse";
    case IMAGE_ERR_VECTORS:   return "Vektortabelle ungueltig";
    case IMAGE_ERR_CRC:       return "Pruefsumme falsch";
    default:                  return "unbekannter Fehler";
    }
}
