# Board-Support-Package: Treiber fuer die Peripherie des STM32F746G-Discovery.
# Als OBJECT-Bibliothek, damit Interrupt-Handler (starke Symbole) sicher gelinkt werden.
add_library(bsp OBJECT
    src/bsp.c
    src/bsp_system.c
    src/bsp_led.c
    src/bsp_sdram.c
    src/bsp_lcd.c
    src/bsp_font.c
    src/bsp_gfx.c
    src/bsp_console.c
    src/bsp_log.c
    src/bsp_fault.c
    src/bsp_i2c.c
    src/bsp_touch.c
    src/bsp_button.c
    src/bsp_qspi.c
    src/bsp_audio.c
    src/bsp_cpu_sensor.c
    src/bsp_util.c)

target_include_directories(bsp PUBLIC include)
target_link_libraries(bsp PUBLIC stm32_hal freertos)
target_compile_options(bsp PRIVATE -Wall -Wextra)

# ---- Optionale Module (Geruest) ------------------------------------------------
# Jedes Modul hat eine API (include/bsp_<name>.h) und einen Platzhalter (src/optional/bsp_<name>_stub.c),
# der BSP_ERR_NOT_IMPLEMENTED liefert. Mit -DBSP_ENABLE_<MODUL>=ON wird stattdessen der echte Treiber
# src/optional/bsp_<name>.c gebaut - sobald er geschrieben ist (docs/optionale-module.md).
# BSP_HAS_<MODUL> (0/1) steht der Applikation als Compile-Definition zur Verfuegung.
foreach(mod SD USB ETH CAMERA)
    string(TOLOWER ${mod} lmod)
    option(BSP_ENABLE_${mod} "Echten ${mod}-Treiber bauen (statt Platzhalter)" OFF)
    if(BSP_ENABLE_${mod})
        set(real ${CMAKE_CURRENT_SOURCE_DIR}/src/optional/bsp_${lmod}.c)
        if(NOT EXISTS ${real})
            message(FATAL_ERROR
                "BSP_ENABLE_${mod}=ON, aber ${real} existiert noch nicht.\n"
                "Der Treiber ist bisher nur ein Geruest - Schritte siehe docs/optionale-module.md.")
        endif()
        target_sources(bsp PRIVATE ${real})
        target_compile_definitions(bsp PUBLIC BSP_HAS_${mod}=1)
    else()
        target_sources(bsp PRIVATE src/optional/bsp_${lmod}_stub.c)
        target_compile_definitions(bsp PUBLIC BSP_HAS_${mod}=0)
    endif()
endforeach()
