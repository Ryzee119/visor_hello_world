ROM_BASE = 0xFFFC0000;
RAM_BASE = 0x00000000;

/* The offset in the BIN file that user code begins */
BIN_CODE_BASE = 0x2000;

STACK_SIZE = 4K;
VISOR_ENTRY_OFFSET = 0x00001000; /* See 2bxcodes.nasm */
MAIN_ENTRY_OFFSET = 0x00100000;

MEMORY
{
    ROM   (rx) : ORIGIN = ROM_BASE, LENGTH = 256K
    RAM   (rwx) : ORIGIN = RAM_BASE, LENGTH = 64M
}

SECTIONS
{
    .header00 ROM_BASE + 0x0C : AT (0x00) { KEEP(*(.header00)) } > ROM
    .header40 ROM_BASE + 0x6C : AT (0x40) { KEEP(*(.header40)) } > ROM
    .header6C ROM_BASE + 0x6C : AT (0x6C) { KEEP(*(.header6C)) } > ROM
    .header70 ROM_BASE + 0x70 : AT (0x70) { KEEP(*(.header70)) } > ROM

    .xcodes   ROM_BASE + 0x80 : AT (0x80) { KEEP(*(.xcodes)) } > ROM

    .visor_entry (ROM_BASE + VISOR_ENTRY_OFFSET) : AT (VISOR_ENTRY_OFFSET) {
        KEEP(*(.visor_entry))
        *(.visor_entry);

        ROM_CODE_BASE = .;
    } > ROM

    .text (RAM_BASE + MAIN_ENTRY_OFFSET): AT (BIN_CODE_BASE) {
        . = ALIGN(4K);
        RAM_CODE_BASE = .;
        *(.text)
        *(.text.*)
        _text_end = .;
    } > RAM

    .data : AT (BIN_CODE_BASE + SIZEOF(.text)) {
        . = ALIGN(4K);
        _data_start = .;
        *(.data)
        *(.data.*)
        _data_end = .;
    } > RAM

    .rodata : AT (BIN_CODE_BASE + SIZEOF(.text) + SIZEOF(.data)) {
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
        . += STACK_SIZE;
        STACK_END = .;
    } > RAM

    .heap (NOLOAD) : {
        . = ALIGN(4K);
        HEAP_BASE = .;
        . += 32M;
        /*. += LENGTH(RAM) - RAM_CODE_BASE - STACK_END;*/
        HEAP_END = .;
    } > RAM

    .pad_to_256k 0x20000 : {
        . = 0x20000 - 1;
        BYTE(0xFF);
    }

    /DISCARD/ : {
        *(.comment)
        *(.note.GNU-stack)
        *(.eh_frame)
        *(.rela.eh_frame)
        *(.shstrtab)
        *(.symtab)
        *(.strtab)
        *(.got.plt)
    }

    /* Used when copying code from ROM to RAM */
    RAM_CODE_SIZE = SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.rodata);

    /* Used when clearing BSS section on init */
    BSS_SIZE = BSS_END - BSS_BASE;
    BSS_DWORDS = BSS_SIZE / 4;

    /* Used if you need to setup malloc etc */
    HEAP_SIZE = HEAP_END - HEAP_BASE;
}
