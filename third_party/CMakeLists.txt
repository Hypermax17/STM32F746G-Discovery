# Drittanbieter-Bibliotheken (als Git-Submodule eingebunden).
# Warnungen werden fuer diesen Code unterdrueckt.

set(TP ${CMAKE_CURRENT_SOURCE_DIR})

# ---- CMSIS (Core + Device) ---------------------------------------------------
add_library(cmsis INTERFACE)
target_include_directories(cmsis SYSTEM INTERFACE
    ${TP}/cmsis_core/Include
    ${TP}/cmsis_device_f7/Include)
target_compile_definitions(cmsis INTERFACE STM32F746xx)

# ---- STM32F7 HAL -------------------------------------------------------------
# Die Konfigurationsdatei stm32f7xx_hal_conf.h liegt im BSP (bsp/config).
set(HAL_DIR ${TP}/stm32f7xx_hal_driver)
set(HAL_MODULES
    hal hal_adc hal_adc_ex hal_cortex hal_dma hal_dma_ex hal_flash hal_flash_ex hal_gpio hal_i2c
    hal_i2c_ex hal_ltdc hal_ltdc_ex hal_pwr hal_pwr_ex hal_qspi hal_rcc hal_rcc_ex hal_sai hal_sai_ex
    hal_sdram ll_fmc)
set(HAL_SOURCES "")
foreach(m ${HAL_MODULES})
    list(APPEND HAL_SOURCES ${HAL_DIR}/Src/stm32f7xx_${m}.c)
endforeach()

add_library(stm32_hal STATIC
    ${HAL_SOURCES}
    ${TP}/cmsis_device_f7/Source/Templates/system_stm32f7xx.c)
target_include_directories(stm32_hal SYSTEM PUBLIC
    ${HAL_DIR}/Inc
    ${CMAKE_SOURCE_DIR}/bsp/config)
target_compile_definitions(stm32_hal PUBLIC USE_HAL_DRIVER)
target_link_libraries(stm32_hal PUBLIC cmsis)
target_compile_options(stm32_hal PRIVATE -w)

# Startup-Code als Objekt-Bibliothek (muss sicher gelinkt werden)
add_library(stm32_startup OBJECT
    ${TP}/cmsis_device_f7/Source/Templates/gcc/startup_stm32f746xx.s)

# ---- FreeRTOS-Kernel ---------------------------------------------------------
set(FR ${TP}/FreeRTOS-Kernel)
add_library(freertos STATIC
    ${FR}/tasks.c ${FR}/list.c ${FR}/queue.c ${FR}/timers.c
    ${FR}/event_groups.c ${FR}/stream_buffer.c
    ${FR}/portable/GCC/ARM_CM7/r0p1/port.c
    ${FR}/portable/MemMang/heap_4.c)
target_include_directories(freertos SYSTEM PUBLIC
    ${FR}/include
    ${FR}/portable/GCC/ARM_CM7/r0p1
    ${CMAKE_SOURCE_DIR}/app/config)       # FreeRTOSConfig.h
target_link_libraries(freertos PUBLIC cmsis)
target_compile_options(freertos PRIVATE -w)
