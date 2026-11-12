# UI-Bibliothek: Anbindung von LVGL an das Board, Designs, Widget-Helfer und Seitenverwaltung.
# Allgemeiner Teil (liegt in main); die eigentlichen Seiten gehoeren zur Applikation (app/).
add_library(ui OBJECT
    src/ui_design.c
    src/ui_widgets.c
    src/ui_pages.c
    src/ui_port.c)

target_include_directories(ui PUBLIC include)
target_link_libraries(ui PUBLIC lvgl bsp freertos)
target_compile_options(ui PRIVATE -Wall -Wextra)
