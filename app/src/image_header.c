/* Image-Header der Applikation (Format: common/include/image.h). Liegt per Linker-Skript bei
 * IMAGE_APP_BASE + 0x200; image_size und CRC32 traegt scripts/make_image.py nach dem Linken ein. */
#include "image.h"

#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif

__attribute__((section(".image_header"), used))
const image_header_t g_image_header = {
    .magic          = IMAGE_MAGIC,
    .header_version = IMAGE_HEADER_VERSION,
    .image_size     = 0,
    .load_addr      = IMAGE_APP_BASE,
    .version        = APP_VERSION,
};
