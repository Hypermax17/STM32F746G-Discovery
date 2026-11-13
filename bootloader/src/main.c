/* Bootloader (Schritt 0): prueft das Applikations-Image im Flash und startet es.
 *
 * Ablauf: Display/Konsole initialisieren -> Image bei 0x08040000 pruefen (Header, Groesse, CRC32) ->
 * bei gueltigem Image und nicht gedruecktem Taster B1 die Applikation starten, sonst im Bootloader bleiben.
 * Netzwerk-Update folgt in den naechsten Schritten (docs/bootloader.md). */
#include "bsp.h"
#include "bsp_system.h"
#include "bsp_led.h"
#include "bsp_lcd.h"
#include "bsp_sdram.h"
#include "bsp_console.h"
#include "bsp_log.h"

#include "boot_jump.h"
#include "image.h"

#include "stm32f7xx_hal.h"

#define START_DELAY_MS 1500U     /* Zeit, in der die Meldungen lesbar bleiben */

/* Taster B1 (PI11, aktiv high) direkt lesen; die Interrupt-/Queue-Logik des BSP braucht den Bootloader nicht. */
static int button_pressed(void)
{
    __HAL_RCC_GPIOI_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin  = GPIO_PIN_11;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOI, &g);
    return HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_11) == GPIO_PIN_SET;
}

int main(void)
{
    bsp_system_init_early();                 /* HAL, MPU, Caches */
    bsp_led_init();
    if (bsp_system_clock_config() != 0) {
        bsp_led_blink_forever(2);
    }
    if (bsp_sdram_init() != 0) {
        bsp_led_blink_forever(3);
    }
    if (bsp_lcd_init() != 0 || bsp_console_init() != 0) {
        bsp_led_blink_forever(5);
    }

    BSP_LOGI("Bootloader %s", BOOT_VERSION);

    image_info_t info;
    int rc = image_check((const void *)IMAGE_APP_BASE, IMAGE_APP_BASE, IMAGE_APP_MAX_SIZE, &info);
    int force = button_pressed();

    if (rc == IMAGE_OK) {
        BSP_LOGI("App %s, %lu KB, CRC %08lX ok", info.version, (unsigned long)((info.size + 1023U) / 1024U),
                 (unsigned long)info.crc32);
    } else {
        BSP_LOGE("App ungueltig: %s", image_error_text(rc));
    }

    if (rc == IMAGE_OK && !force) {
        BSP_LOGI("Starte App ...");
        HAL_Delay(START_DELAY_MS);
        boot_jump_to_app(IMAGE_APP_BASE);
    }

    if (force) {
        BSP_LOGW("Taster B1 gedrueckt: Bootloader bleibt aktiv");
    }
    BSP_LOGW("Update-Modus (Netzwerk folgt)");
    for (;;) {
        bsp_led_toggle();
        HAL_Delay(500);
    }
}
