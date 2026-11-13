/* Host-Test: CRC32 und Image-Pruefung des Bootloaders (common/src). Optional wird ein echtes Image
 * (build/.../stm32f746g_app.bin) geprueft:  image_test [pfad/zur/app.bin] */
#include "image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail;
#define CHECK(c) do { if (!(c)) { printf("FEHLER Zeile %d: %s\n", __LINE__, #c); g_fail++; } } while (0)

#define SZ 4096U
static uint8_t g_img[SZ];

static void put32(uint32_t off, uint32_t v) { memcpy(g_img + off, &v, 4); }

/* Baut ein gueltiges Image wie make_image.py */
static void make_good(void)
{
    memset(g_img, 0xFF, sizeof g_img);
    for (uint32_t i = 0x240; i < SZ; i++) g_img[i] = (uint8_t)(i * 7);
    put32(0, 0x20010000UL);                    /* SP */
    put32(4, IMAGE_APP_BASE + 0x241UL);        /* Reset (Thumb) */
    image_header_t h = { IMAGE_MAGIC, IMAGE_HEADER_VERSION, SZ, IMAGE_APP_BASE, "v1.2.3", {0} };
    memcpy(g_img + IMAGE_HEADER_OFFSET, &h, sizeof h);
    put32(SZ - 4, crc32_update(0, g_img, SZ - 4));
}

int main(int argc, char **argv)
{
    /* CRC-32: Standard-Pruefwert und Fortsetzbarkeit */
    CHECK(crc32_update(0, "123456789", 9) == 0xCBF43926UL);
    CHECK(crc32_update(crc32_update(0, "12345", 5), "6789", 4) == 0xCBF43926UL);
    CHECK(crc32_update(0, "", 0) == 0);

    image_info_t info;
    make_good();
    CHECK(sizeof(image_header_t) == 64);
    CHECK(image_check(g_img, IMAGE_APP_BASE, SZ, &info) == IMAGE_OK);
    CHECK(info.size == SZ && strcmp(info.version, "v1.2.3") == 0);
    CHECK(image_check(g_img, IMAGE_APP_BASE, SZ, NULL) == IMAGE_OK);

    /* Jedes einzelne gekippte Bit im Image muss auffallen (Stichprobe ueber alle Bytes) */
    for (uint32_t i = 0; i < SZ; i++) {
        make_good();
        g_img[i] ^= 0x10;
        CHECK(image_check(g_img, IMAGE_APP_BASE, SZ, NULL) != IMAGE_OK);
    }

    make_good(); CHECK(image_check(g_img, IMAGE_APP_BASE + 0x1000, SZ, NULL) == IMAGE_ERR_ADDR);   /* falsche Adresse */
    make_good(); CHECK(image_check(g_img, IMAGE_APP_BASE, SZ - 4, NULL) == IMAGE_ERR_SIZE);        /* Bereich zu klein */
    make_good(); CHECK(image_check(g_img, IMAGE_APP_BASE, 100, NULL) == IMAGE_ERR_SIZE);
    memset(g_img, 0xFF, sizeof g_img); CHECK(image_check(g_img, IMAGE_APP_BASE, SZ, NULL) == IMAGE_ERR_NO_HEADER); /* leerer Flash */
    memset(g_img, 0x00, sizeof g_img); CHECK(image_check(g_img, IMAGE_APP_BASE, SZ, NULL) == IMAGE_ERR_NO_HEADER);

    make_good(); put32(0, 0x08000000UL); put32(SZ - 4, crc32_update(0, g_img, SZ - 4));            /* SP im Flash */
    CHECK(image_check(g_img, IMAGE_APP_BASE, SZ, NULL) == IMAGE_ERR_VECTORS);
    make_good(); put32(4, IMAGE_APP_BASE + 0x240UL); put32(SZ - 4, crc32_update(0, g_img, SZ - 4)); /* kein Thumb-Bit */
    CHECK(image_check(g_img, IMAGE_APP_BASE, SZ, NULL) == IMAGE_ERR_VECTORS);
    make_good(); put32(4, 0x08000001UL); put32(SZ - 4, crc32_update(0, g_img, SZ - 4));            /* Reset ausserhalb */
    CHECK(image_check(g_img, IMAGE_APP_BASE, SZ, NULL) == IMAGE_ERR_VECTORS);

    make_good(); put32(IMAGE_HEADER_OFFSET + 8, SZ + 4);                                            /* Groesse > verfuegbar */
    CHECK(image_check(g_img, IMAGE_APP_BASE, SZ, NULL) == IMAGE_ERR_SIZE);
    make_good(); put32(IMAGE_HEADER_OFFSET + 8, SZ - 2);                                            /* nicht durch 4 teilbar */
    CHECK(image_check(g_img, IMAGE_APP_BASE, SZ, NULL) == IMAGE_ERR_SIZE);

    /* Echtes Image aus dem Build */
    if (argc > 1) {
        FILE *f = fopen(argv[1], "rb");
        if (!f) { printf("FEHLER: %s nicht lesbar\n", argv[1]); return 1; }
        static uint8_t buf[IMAGE_APP_MAX_SIZE];
        size_t n = fread(buf, 1, sizeof buf, f);
        fclose(f);
        int rc = image_check(buf, IMAGE_APP_BASE, (uint32_t)n, &info);
        printf("%s: %zu Byte, %s, Version %s, CRC %08lX\n", argv[1], n, image_error_text(rc), info.version, (unsigned long)info.crc32);
        CHECK(rc == IMAGE_OK);
        CHECK(info.size == n);
        buf[n / 2] ^= 1;
        CHECK(image_check(buf, IMAGE_APP_BASE, (uint32_t)n, NULL) == IMAGE_ERR_CRC);
    }

    printf(g_fail ? "image_test: %d FEHLER\n" : "image_test: alle Tests ok\n", g_fail);
    return g_fail ? 1 : 0;
}
