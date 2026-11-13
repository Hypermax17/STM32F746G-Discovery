# Bootloader: liegt in Flash-Sektor 0..4 (0x08000000, 256 KB) und startet die Applikation bei 0x08040000.
# Er nutzt das BSP (Display-Konsole, LED) und wird nie vom Netzwerk-Update ueberschrieben.
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/bootloader.ld)

add_executable(bootloader
    src/main.c
    src/boot_jump.c
    src/hooks.c
    $<TARGET_OBJECTS:stm32_startup>)

set_target_properties(bootloader PROPERTIES OUTPUT_NAME stm32f746g_bootloader SUFFIX .elf)
target_include_directories(bootloader PRIVATE src)
target_compile_definitions(bootloader PRIVATE BOOT_VERSION="${APP_VERSION}")
target_compile_options(bootloader PRIVATE -Wall -Wextra)
target_link_libraries(bootloader PRIVATE boot_common bsp stm32_hal freertos m)
target_link_options(bootloader PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,-Map=$<TARGET_FILE_DIR:bootloader>/stm32f746g_bootloader.map
    -Wl,--print-memory-usage)
set_target_properties(bootloader PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

add_custom_command(TARGET bootloader POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:bootloader> $<TARGET_FILE_DIR:bootloader>/stm32f746g_bootloader.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:bootloader> $<TARGET_FILE_DIR:bootloader>/stm32f746g_bootloader.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:bootloader>
    COMMENT "Erzeuge Bootloader .hex/.bin")
