cmake_minimum_required(VERSION 3.21)

project(stm32f746g_discovery C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_FLAGS_DEBUG   "-Og -g3")
set(CMAKE_C_FLAGS_RELEASE "-O2 -g")

# Versionsstring (git describe) fuer das Boot-Banner
find_package(Git QUIET)
set(APP_VERSION "unknown")
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always --dirty --tags
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE APP_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(NOT APP_VERSION)
        set(APP_VERSION "unknown")
    endif()
endif()

add_subdirectory(third_party)   # CMSIS, HAL, FreeRTOS, LVGL
add_subdirectory(bsp)           # Board-Support-Package (Treiber)
add_subdirectory(ui)            # UI-Bibliothek (LVGL-Anbindung, Designs, Seitenverwaltung)
add_subdirectory(projects/terrarium)   # projektabhaengiges BSP (zusaetzliche Hardware dieses Projekts)
add_subdirectory(app)           # Applikation
