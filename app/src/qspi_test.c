#include "qspi_test.h"
#include "bsp.h"

#include <string.h>

#include "stm32f7xx_hal.h"

#define SEARCH_SECTORS 256U          /* von hinten bis zu 1 MB absuchen */

static uint8_t pattern_byte(uint32_t addr, uint32_t i)
{
    return (uint8_t)((i * 7U + (addr >> 12) + 0x5AU) & 0xFFU);
}

/* 1 = Sektor komplett 0xFF, 0 = nicht leer, -1 = Lesefehler */
static int sector_blank(uint32_t addr)
{
    uint8_t buf[256];
    for (uint32_t off = 0; off < BSP_QSPI_SUBSECTOR_SIZE; off += sizeof buf) {
        if (bsp_qspi_read(addr + off, buf, sizeof buf) != 0) return -1;
        for (unsigned i = 0; i < sizeof buf; i++) {
            if (buf[i] != 0xFFU) return 0;
        }
    }
    return 1;
}

void qspi_write_test(void)
{
    if (!bsp_qspi_available()) {
        BSP_LOGW("QSPI-Test: kein Flash");
        return;
    }

    uint32_t addr = 0;
    int found = 0;
    for (uint32_t k = 1; k <= SEARCH_SECTORS; k++) {
        uint32_t a = BSP_QSPI_SIZE - k * BSP_QSPI_SUBSECTOR_SIZE;
        int b = sector_blank(a);
        if (b < 0) { BSP_LOGE("QSPI-Test: Lesefehler"); return; }
        if (b == 1) { addr = a; found = 1; break; }
    }
    if (!found) {
        BSP_LOGW("QSPI-Test: kein leerer Sektor");
        return;
    }
    BSP_LOGW("QSPI-Test: Sektor 0x%06lX", (unsigned long)addr);

    uint8_t buf[BSP_QSPI_PAGE_SIZE];
    uint32_t t0 = HAL_GetTick();
    for (uint32_t off = 0; off < BSP_QSPI_SUBSECTOR_SIZE; off += BSP_QSPI_PAGE_SIZE) {
        for (uint32_t i = 0; i < BSP_QSPI_PAGE_SIZE; i++) buf[i] = pattern_byte(addr, off + i);
        if (bsp_qspi_write(addr + off, buf, BSP_QSPI_PAGE_SIZE) != 0) {
            BSP_LOGE("QSPI-Test: Schreibfehler @%lX", (unsigned long)(addr + off));
            goto cleanup;
        }
    }
    uint32_t t_prog = HAL_GetTick() - t0;

    /* Pruefen: indirekt und (falls aktiv) per Memory-Mapping */
    unsigned bad_ind = 0, bad_map = 0;
    for (uint32_t off = 0; off < BSP_QSPI_SUBSECTOR_SIZE; off += BSP_QSPI_PAGE_SIZE) {
        if (bsp_qspi_read(addr + off, buf, BSP_QSPI_PAGE_SIZE) != 0) { bad_ind++; continue; }
        for (uint32_t i = 0; i < BSP_QSPI_PAGE_SIZE; i++) {
            if (buf[i] != pattern_byte(addr, off + i)) bad_ind++;
        }
    }
    if (bsp_qspi_is_memory_mapped()) {
        const volatile uint8_t *map = (const volatile uint8_t *)(BSP_QSPI_BASE + addr);
        for (uint32_t i = 0; i < BSP_QSPI_SUBSECTOR_SIZE; i++) {
            if (map[i] != pattern_byte(addr, i)) bad_map++;
        }
    }
    BSP_LOGI("Schreiben 4K: %lu ms", (unsigned long)t_prog);
    BSP_LOGI("Pruefung: indirekt %u, mapped %u Fehler", bad_ind, bad_map);

cleanup:;
    uint32_t t1 = HAL_GetTick();
    int er = bsp_qspi_erase_subsector(addr);
    uint32_t t_erase = HAL_GetTick() - t1;
    int blank = (er == 0) ? sector_blank(addr) : -1;
    BSP_LOGI("Loeschen: %lu ms, leer: %s", (unsigned long)t_erase, blank == 1 ? "ja" : "NEIN");
    if (blank != 1) {
        BSP_LOGE("QSPI-Test: Sektor 0x%06lX NICHT leer", (unsigned long)addr);
    } else {
        BSP_LOGI("QSPI-Test beendet");
    }
}
