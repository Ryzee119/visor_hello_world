MEMORY
{
    ROM   (rx) : ORIGIN = 0xFFFC0000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x00000000, LENGTH = 64M
}

ROM_BASE = 0xFFFC0000;
RAM_BASE = 0x00000000;
RAM_CODE = RAM_BASE + 0x0001000;

SECTIONS
{
    .header00 ROM_BASE + 0x0C : AT (0x00) { KEEP(*(.header00)) } > ROM
    .header40 ROM_BASE + 0x6C : AT (0x40) { KEEP(*(.header40)) } > ROM
    .header6C ROM_BASE + 0x6C : AT (0x6C) { KEEP(*(.header6C)) } > ROM
    .header70 ROM_BASE + 0x70 : AT (0x70) { KEEP(*(.header70)) } > ROM

    .xcodes ROM_BASE + 0x80 : AT (0x80) { KEEP(*(.xcodes)) } > ROM

    .visor_entry ROM_BASE + 0x1000 : AT (0x1000) {
        KEEP(*(.visor_entry))
        *(.visor_entry);
        END_OF_ROM_CODE = .;
    } > ROM

    CODE_OFFSET_IN_ROM = END_OF_ROM_CODE - ROM_BASE;

    .text RAM_CODE : AT (CODE_OFFSET_IN_ROM) {
        . = ALIGN(4K);
        RAM_CODE_BASE = .;
        RAM_CODE_BASE_IN_ROM = END_OF_ROM_CODE;
        *(.text)
        *(.text.*)
        _text_end = .;
    } > RAM

    .data : AT (CODE_OFFSET_IN_ROM + SIZEOF(.text)) {
        . = ALIGN(4K);
        _data_start = .;
        *(.data)
        *(.data.*)
        _data_end = .;
    } > RAM

    .rodata : AT (CODE_OFFSET_IN_ROM + SIZEOF(.text) + SIZEOF(.data)) {
        . = ALIGN(4K);
        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        _rodata_end = .;
    } > RAM

    .bss (NOLOAD) : {
        . = ALIGN(4K);
        BSS_BASE = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        BSS_END = .;
    } > RAM

    .stack (NOLOAD) : {
        . = ALIGN(4K);
        STACK_BASE = .;
        . += 0x4000;
        STACK_END = .;
    } > RAM

    .heap (NOLOAD) : {
        HEAP_BASE = .;
        . += ORIGIN(RAM) + LENGTH(RAM) - 4M - SIZEOF(.text) - SIZEOF(.data) - SIZEOF(.rodata) - SIZEOF(.bss) - SIZEOF(.stack); /* Remaining RAM minus stack size */
        HEAP_END = .;
    } > RAM

    .pad_to_256k 0x20000 : {
        . = 0x20000 - 1;
        BYTE(0xFF)
    }

    RAM_CODE_SIZE = SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.rodata);

    BSS_SIZE = BSS_END - BSS_BASE;
    BSS_SIZE_L = BSS_SIZE / 4;
}
