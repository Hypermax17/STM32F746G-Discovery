# Applikation: eigentliche Anwendung mit Logik. Hardware-Zugriff nur ueber das BSP.
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/STM32F746NGHx_FLASH.ld)

add_executable(app
    src/main.c
    src/app.c
    src/app_hooks.c
    src/fault_test.c
    src/app_status.c
    src/touch_task.c
    src/button_task.c
    $<TARGET_OBJECTS:stm32_startup>)

set_target_properties(app PROPERTIES OUTPUT_NAME stm32f746g_app SUFFIX .elf)

target_include_directories(app PRIVATE src)
target_compile_definitions(app PRIVATE APP_VERSION="${APP_VERSION}")
target_compile_options(app PRIVATE -Wall -Wextra)

# Optional: Fehler provozieren, um die Fehleranzeige zu testen
#   -DAPP_FAULT_TEST=nullptr|div0|undef|execsdram|stackoverflow|assert|malloc
set(APP_FAULT_TEST "" CACHE STRING "Testfehler 3 s nach dem Start ausloesen (leer = aus)")
if(APP_FAULT_TEST)
    string(TOUPPER "${APP_FAULT_TEST}" _ft)
    target_compile_definitions(app PRIVATE
        APP_FAULT_TEST_ENABLED=1
        APP_FAULT_TEST_${_ft}=1
        APP_FAULT_TEST_NAME="${APP_FAULT_TEST}")
endif()
target_link_libraries(app PRIVATE bsp stm32_hal freertos)
target_link_options(app PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,-Map=$<TARGET_FILE_DIR:app>/stm32f746g_app.map
    -Wl,--print-memory-usage)
set_target_properties(app PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

# Binaries fuer STM32CubeProgrammer: .hex und .bin (Startadresse 0x08000000)
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:app> $<TARGET_FILE_DIR:app>/stm32f746g_app.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:app> $<TARGET_FILE_DIR:app>/stm32f746g_app.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:app>
    COMMENT "Erzeuge .hex/.bin")
