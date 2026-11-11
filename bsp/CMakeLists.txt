# Board-Support-Package: Treiber fuer die Peripherie des STM32F746G-Discovery.
# Als OBJECT-Bibliothek, damit Interrupt-Handler (starke Symbole) sicher gelinkt werden.
add_library(bsp OBJECT
    src/bsp.c
    src/bsp_system.c
    src/bsp_led.c
    src/bsp_sdram.c
    src/bsp_lcd.c
    src/bsp_font.c
    src/bsp_console.c
    src/bsp_log.c
    src/bsp_fault.c
    src/bsp_i2c.c
    src/bsp_touch.c)

target_include_directories(bsp PUBLIC include)
target_link_libraries(bsp PUBLIC stm32_hal freertos)
target_compile_options(bsp PRIVATE -Wall -Wextra)
