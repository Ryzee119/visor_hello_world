set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER gcc) #FreeRTOS assembly files need GCC
set(CMAKE_CXX_COMPILER clang++)

cmake_minimum_required(VERSION 3.10)
project(visor C ASM ASM_NASM)

set(OUTPUT_NAME "rom.elf")

# Set the compiler flags for clang
set(CMAKE_C_FLAGS "\
    -Wall -target i386-elf \
    -m32 -march=pentium -mfpmath=sse -msse \
    -fno-stack-protector -ffreestanding -fno-pic -fno-common \
    -nostdlib -fno-builtin -nodefaultlibs -static \
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
target_include_directories(freertos_config SYSTEM INTERFACE freertos_configuration)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/freertos_kernel freertos_kernel)

# NASM elf32 i386. Note we set CMAKE_ASM_NASM_COMPILE_OBJECT otherwise it doesnt compile elf32 properly. FIXME?
set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_FLAGS "-I${CMAKE_SOURCE_DIR}")
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <DEFINES> \
    <INCLUDES> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

# Build our xbox specific code
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/xbox xbox)

# Create the executable
add_executable(${OUTPUT_NAME})

# We link gcc as we need some gcc functions. FIXME?
target_link_libraries(${OUTPUT_NAME} PRIVATE c gcc freertos_kernel xbox )

# Set the linker flags
set_target_properties(${OUTPUT_NAME} PROPERTIES
    LINK_FLAGS "-Wl,--gc-sections -Wl,--build-id=none -target i386-elf"
)

# Create a binary from the elf
add_custom_command(TARGET ${OUTPUT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --output-target=binary $<TARGET_FILE:${OUTPUT_NAME}> ${CMAKE_BINARY_DIR}/rom.bin
    COMMENT "Converting ELF to binary"
)

# Include directories
target_include_directories(${OUTPUT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Source files
target_sources(${OUTPUT_NAME} PRIVATE
    main.c
)
