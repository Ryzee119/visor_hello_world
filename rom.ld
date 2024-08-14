/* Base addresses and sizes */
ROM_BASE = 0xFFFC0000;
RAM_BASE = 0x00000000;
BIN_CODE_BASE = 0x2000;
STACK_SIZE = 4K;
ROM_VISOR_ENTRY_OFFSET = 0x00001000; /* See 2bxcodes.nasm */
RAM_MAIN_ENTRY_OFFSET = 0x00100000;

/* Memory regions */
MEMORY {
    ROM   (rx) : ORIGIN = ROM_BASE, LENGTH = 256K
    RAM   (rwx) : ORIGIN = RAM_BASE, LENGTH = 64M
}

/* Sections */
SECTIONS {
    .header00 ROM_BASE + 0x0C : AT (0x00) { KEEP(*(.header00)) } > ROM
    .header40 ROM_BASE + 0x6C : AT (0x40) { KEEP(*(.header40)) } > ROM
    .header6C ROM_BASE + 0x6C : AT (0x6C) { KEEP(*(.header6C)) } > ROM
    .header70 ROM_BASE + 0x70 : AT (0x70) { KEEP(*(.header70)) } > ROM
    .xcodes   ROM_BASE + 0x80 : AT (0x80) { KEEP(*(.xcodes)) } > ROM
    .pad_to_256k : AT (0x3FFFF) { BYTE(0x00) } > ROM

    .visor_entry (ROM_BASE + ROM_VISOR_ENTRY_OFFSET) : AT (ROM_VISOR_ENTRY_OFFSET) {
        _visor_start = .;
        KEEP(*(.visor_entry))
        *(.visor_entry);
        _visor_end = .;
        /* Pad out to BIN_CODE_BASE */
        . = . + BIN_CODE_BASE - ROM_VISOR_ENTRY_OFFSET - SIZEOF(.visor_entry) - 1;
        BYTE(0);
    } > ROM

    .ram_code (RAM_BASE + RAM_MAIN_ENTRY_OFFSET) : AT (BIN_CODE_BASE) {
        . = ALIGN(4K);
        __text_start = .;
        __ram_data_base = .;
        *(.text)
        *(.text.*)
        KEEP(*(.fini .fini.*))
        __text_end = .;

        . = ALIGN(16);
        __data_start = .;
        *(.data)
        *(.data.*)
        __data_end = .;

        . = ALIGN(16);
        __rodata_start = .;
        *(.rdata)
        *(.rdata.*)
        *(.rodata)
        *(.rodata.*)
        *(.srodata .srodata.*)
        __rodata_end = .;

        . = ALIGN(16);
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);

        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array .ctors))
        PROVIDE_HIDDEN(__init_array_end = .);

        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP(*(.fini_array .dtors))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > RAM

    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        KEEP(*(.bss))
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    } > RAM

    .preserve (NOLOAD) : {
        PROVIDE(__preserve_start__ = .);
        KEEP(*(SORT_BY_NAME(.preserve.*)))
        KEEP(*(.preserve))
        PROVIDE(__preserve_end__ = .);
    } > RAM

    .stack (NOLOAD) : {
        __stack_end = .;
        . += STACK_SIZE;
        . = ALIGN(16);
        __stack = .;
    } > RAM

    .heap (NOLOAD) : {
        . = ALIGN(16);
        __heap_start = .;
        . += LENGTH(RAM) - (__heap_start - ORIGIN(RAM));
        __heap_end = .;
    } > RAM

    /DISCARD/ : {
        *(.comment)
        *(.debug_info)
        *(.debug_abbrev)
        *(.debug_aranges)
        *(.debug_line)
        *(.debug_str)
        *(.debug_loclists)
        *(.debug_rnglists)
        *(.debug_line_str)
    }

    __ram_data_source = ROM_BASE + LOADADDR(.ram_code);
    __ram_data_size = SIZEOF(.ram_code);
    __bss_size = __bss_end - __bss_start;
    __bss_dsize = __bss_size / 4;
}
