set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER gcc) #FreeRTOS assembly files need GCC
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
    -nostdlib -fno-builtin -nodefaultlibs -static -Wno-c23-extensions -Wno-ignored-optimization-argument \
")

set(CMAKE_ASM_FLAGS "\
    -m32 -march=pentium \
")

# Use our linker script
set(CMAKE_EXE_LINKER_FLAGS "-T${CMAKE_SOURCE_DIR}/rom.ld")

# Build picolibc
set(_FORMAT_DEFAULT_DOUBLE ON)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/picolibc)

# Build FreeRTOS
set(FREERTOS_PORT "GCC_IA32_FLAT")
set(FREERTOS_HEAP "3")
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE config_freertos)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/freertos_kernel freertos_kernel)

# Build TinyUSB
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
target_link_libraries(${OUTPUT_NAME} PRIVATE c gcc xbox pc freertos_kernel tinyusb xinput_host fatfs)

# Create a binary from the elf
add_custom_command(TARGET ${OUTPUT_NAME} POST_BUILD
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/calculate_usage.py ${CMAKE_BINARY_DIR}/${OUTPUT_NAME}
    COMMAND ${CMAKE_OBJCOPY} --output-target=binary --pad-to 0x40000 --gap-fill 0xFF $<TARGET_FILE:${OUTPUT_NAME}> ${CMAKE_BINARY_DIR}/rom.bin
    COMMENT "Converting ELF to binary"
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
)
