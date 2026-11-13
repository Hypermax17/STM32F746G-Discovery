# Projekt Terrarium: projektabhaengiges BSP (Treiber fuer zusaetzlich angeschlossene Hardware).
# Die Treiber des Boards selbst liegen in bsp/ (gemeinsam fuer alle Projekte, Branch main).
add_library(terrarium_bsp OBJECT
    bsp/src/bsp_dht.c
    bsp/src/bsp_ws2812.c)

target_include_directories(terrarium_bsp PUBLIC bsp/include)
target_link_libraries(terrarium_bsp PUBLIC bsp stm32_hal freertos)
target_compile_options(terrarium_bsp PRIVATE -Wall -Wextra)
