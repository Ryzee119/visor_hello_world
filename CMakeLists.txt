set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_OBJCOPY objcopy)

cmake_minimum_required (VERSION 3.13)
cmake_policy(SET CMP0079 NEW)
project(visor C ASM ASM_NASM)

find_package (Python COMPONENTS Interpreter)
message(STATUS "Python found: ${Python_Interpreter_FOUND}")
message(STATUS "PYTHON_EXECUTABLE: ${Python_EXECUTABLE}")

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(OUTPUT_NAME "rom.elf")

# Set the compiler flags for clang
set(CMAKE_C_FLAGS "\
    -Wall -target i386-elf \
    -m32 -march=pentium -mfpmath=sse -msse \
    -fno-stack-protector -ffreestanding -fno-pic -fno-common \
    -nostdlib -fno-builtin -nodefaultlibs -static -Wno-c23-extensions \
    -Wno-ignored-optimization-argument -Wno-implicit-function-declaration \
")

set(CMAKE_ASM_FLAGS "\
    -m32 -march=pentium \
")

# Use our linker script
set(CMAKE_EXE_LINKER_FLAGS "-T${CMAKE_SOURCE_DIR}/rom.ld")

# picolibc
set(_FORMAT_DEFAULT_INTEGER ON)
# set(_FORMAT_DEFAULT_DOUBLE ON)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/picolibc)

#arith64 - prevents having to link libgcc for a couple functions
add_library(arith64 INTERFACE)
set(ARITH64_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/arith64)
target_sources(arith64 INTERFACE ${ARITH64_DIR}/arith64.c)

# FreeRTOS
set(FREERTOS_PORT "GCC_IA32_FLAT")
set(FREERTOS_HEAP "3") # Wraps malloc/free
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE config_freertos)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/freertos_kernel freertos_kernel)

# TinyUSB
add_library(tinyusb_config INTERFACE)
target_compile_definitions(tinyusb_config INTERFACE TUH_OPT_RHPORT=0 CFG_TUSB_MCU=0 CFG_TUSB_DEBUG_PRINTF=printf_r)
target_include_directories(tinyusb_config SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/config_tinyusb)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb/src tinyusb_host)
target_sources(tinyusb PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb/src/portable/ohci/ohci.c)
target_compile_definitions(tinyusb PRIVATE portARMV8M_MINOR_VERSION=0) # Why FreeRTOS needs this?
target_link_libraries(tinyusb PRIVATE c freertos_kernel)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/tusb_xinput xinput_host)

# FATFS
add_library(fatfs_config INTERFACE)
target_include_directories(fatfs_config SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/config_fatfs)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/fatfs fatfs)

# LZ4 - called during boot, so no libc functions. Make freestanding and use our own memcpy/memset/memmove
set (LZ4_BUNDLED_MODE ON)
set (LZ4_BUILD_CLI OFF)
set (LZ4_FREESTANDING ON)
add_compile_definitions(LZ4_STATIC_LINKING_ONLY_DISABLE_MEMORY_ALLOCATION
                        LZ4_memcpy=boot_memcpy
                        LZ4_memset=boot_memset
                        LZ4_memmove=boot_memmove)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/lz4/build/cmake lz4)

# Build our xbox specific code
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/xbox xbox)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/pc pc)

# NASM elf32 i386. Note we set CMAKE_ASM_NASM_COMPILE_OBJECT otherwise it doesnt compile elf32 properly. FIXME?
set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_FLAGS "-I${CMAKE_SOURCE_DIR}")
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <DEFINES> \
    <INCLUDES> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

# Create the executable
add_executable(${OUTPUT_NAME})

# Set the linker flags
set_target_properties(${OUTPUT_NAME} PROPERTIES
    LINK_FLAGS "-Wl,--gc-sections -Wl,--build-id=none -target i386-elf"
)

# Link the libraries
target_link_libraries(${OUTPUT_NAME} PRIVATE c arith64 freertos_kernel pc xbox tinyusb xinput_host fatfs lz4)

# Create a binary from the elf
add_custom_command(TARGET ${OUTPUT_NAME} POST_BUILD
    COMMAND echo "Compressing ROM sections..."
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/compress_rom.py
                                 ${CMAKE_BINARY_DIR}/${OUTPUT_NAME} ${CMAKE_BINARY_DIR}/compressed.bin

    # Push the compressed blob back into the elf
    COMMAND ${CMAKE_OBJCOPY} --update-section .compressed=${CMAKE_BINARY_DIR}/compressed.bin ${OUTPUT_NAME}

    # These sections are now compressed, so we can remove them from the final binary as they are decompressed at runtime
    COMMAND ${CMAKE_OBJCOPY} --remove-section=.text --remove-section=.data --remove-section=.rodata ${OUTPUT_NAME}

    # Calculate the final size ROM usage
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/calculate_usage.py ${CMAKE_BINARY_DIR}/${OUTPUT_NAME}

    # Convert the elf to a binary 0x100000000 is 256kB from ROM base which makes 256Kb output size
    COMMAND ${CMAKE_OBJCOPY} -O binary --pad-to 0x100000000 --gap-fill 0xFF ${OUTPUT_NAME} ${CMAKE_BINARY_DIR}/rom.bin
)

#add_custom_target(apply_patch ALL
#    COMMAND git apply ${CMAKE_SOURCE_DIR}/config_freertos/pic_ack.patch
#    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib/freertos_kernel
#)
#add_dependencies(${OUTPUT_NAME} apply_patch)

# Include directories
target_include_directories(${OUTPUT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib
)

# Source files
target_sources(${OUTPUT_NAME} PRIVATE
    config_freertos/irq.S
    config_freertos/irq.c
    config_fatfs/ffsystem.c
    main.c
    usb.c
    debug.c
    interrupts.c
    fileio_usb.c
    doom.c
)
