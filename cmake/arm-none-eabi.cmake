# Toolchain-Datei fuer arm-none-eabi-gcc (Cortex-M7, FPU fpv5-d16, hard-float)
set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Optional: ARM_TOOLCHAIN_PATH=/pfad/zu/bin setzen, falls der Compiler nicht im PATH liegt.
if(DEFINED ENV{ARM_TOOLCHAIN_PATH})
    set(_tc_prefix "$ENV{ARM_TOOLCHAIN_PATH}/arm-none-eabi-")
else()
    set(_tc_prefix "arm-none-eabi-")
endif()

set(CMAKE_C_COMPILER   ${_tc_prefix}gcc)
set(CMAKE_ASM_COMPILER ${_tc_prefix}gcc)
set(CMAKE_OBJCOPY      ${_tc_prefix}objcopy CACHE INTERNAL "")
set(CMAKE_SIZE         ${_tc_prefix}size    CACHE INTERNAL "")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(MCU_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS_INIT   "${MCU_FLAGS} -ffunction-sections -fdata-sections -fno-common")
set(CMAKE_ASM_FLAGS_INIT "${MCU_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${MCU_FLAGS} --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
