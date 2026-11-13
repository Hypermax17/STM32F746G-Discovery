# Gemeinsamer Code von Bootloader und Applikation (Image-Format, CRC32); hardwarefrei, per Host-Test pruefbar.
add_library(boot_common STATIC src/crc32.c src/image.c)
target_include_directories(boot_common PUBLIC include)
target_compile_options(boot_common PRIVATE -Wall -Wextra)
