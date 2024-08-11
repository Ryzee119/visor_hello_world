cmake_minimum_required(VERSION 3.10)
project(visor C ASM_NASM)

set(OUTPUT_NAME "rom.elf")

# Use gcc as the compiler and ld as the linker, also strip everything out of the binary
set(CMAKE_C_COMPILER gcc)
set(CMAKE_LINKER ld)
set(CMAKE_C_FLAGS "-Wall -O2 -g0 -march=pentium -m32 -fno-stack-protector -ffreestanding -fno-pic -nostdlib -static")

# NASM elf32 i386. Note we set CMAKE_ASM_NASM_COMPILE_OBJECT otherwise it doesnt compile elf32 properly
set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_FLAGS "-I${CMAKE_SOURCE_DIR}")
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

add_executable(${OUTPUT_NAME} 2bmain.nasm 2bxcodes.nasm main.c)

# Use our custom linker script
set_target_properties(${OUTPUT_NAME} PROPERTIES NASM_OBJ_FORMAT elf32
    LINK_FLAGS "-Wl,--script=${CMAKE_SOURCE_DIR}/rom.ld -Wl,-m elf_i386 -Wl,--gc-sections -Wl,--build-id=none"
)

# Create a binary from the elf
add_custom_command(TARGET ${OUTPUT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --output-target=binary $<TARGET_FILE:${OUTPUT_NAME}> ${CMAKE_BINARY_DIR}/rom.bin
    COMMENT "Converting ELF to binary"
)

set_target_properties(${OUTPUT_NAME} PROPERTIES OUTPUT_NAME "rom.bin")
