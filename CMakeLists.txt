set(CMAKE_C_COMPILER gcc)

cmake_minimum_required(VERSION 3.10)
project(visor C ASM_NASM)

set(OUTPUT_NAME "rom.elf")

set(CMAKE_C_FLAGS "-Wall -O2 -g0 -march=pentium -m32 -fno-stack-protector -ffreestanding -fno-pic -nostdlib -static")

# NASM elf32 i386. Note we set CMAKE_ASM_NASM_COMPILE_OBJECT otherwise it doesnt compile elf32 properly. FIXME?
set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_FLAGS "-I${CMAKE_SOURCE_DIR}")
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

add_executable(${OUTPUT_NAME} 2bmain.nasm 2bxcodes.nasm main.c)

# Some linker flags change between clang and GCC
if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    SET(TARGET_FORMAT "-target i386-elf")
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    SET(TARGET_FORMAT "-Wl,-m elf_i386")
else()
    message(FATAL_ERROR "Unsupported compiler ${CMAKE_C_COMPILER_ID}")
endif()

# Use our custom linker script and pass in our flags
set_target_properties(${OUTPUT_NAME} PROPERTIES
    NASM_OBJ_FORMAT elf32
    LINK_FLAGS "-Wl,--script=${CMAKE_SOURCE_DIR}/rom.ld -Wl,--gc-sections -Wl,--build-id=none ${TARGET_FORMAT}"
)

# Create a binary from the elf
add_custom_command(TARGET ${OUTPUT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --output-target=binary $<TARGET_FILE:${OUTPUT_NAME}> ${CMAKE_BINARY_DIR}/rom.bin
    COMMENT "Converting ELF to binary"
)
