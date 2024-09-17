/* Base addresses and sizes */
ROM_BASE = 0xFFFC0000;
RAM_BASE = 0x00000000;
STACK_SIZE = 32K;
ROM_VISOR_ENTRY_OFFSET = 0x00001000; /* See 2bxcodes.nasm */

/* Memory regions */
MEMORY {
    ROM   (rx) : ORIGIN = ROM_BASE, LENGTH = 256K
    RAM   (rwx) : ORIGIN = RAM_BASE, LENGTH = 64M
}

/* Sections */
SECTIONS {
    .rom_headers (ROM_BASE) : AT (0x00000000) {
        KEEP(*(.header00))
        . = 0x40;
        KEEP(*(.header40))
        . = 0x6C;
        KEEP(*(.header6C))
        . = 0x70;
        KEEP(*(.header70))
        . = 0x80;
        KEEP(*(.xcodes))
    } > ROM

    .visor_entry (ROM_BASE + ROM_VISOR_ENTRY_OFFSET) : AT (ROM_VISOR_ENTRY_OFFSET) {
        _visor_start = .;
        KEEP(*(.visor_entry))
        *(.visor_entry);
        _visor_end = .;
    } > ROM

    .boot_code (0x00100000) : AT (LOADADDR(.visor_entry) + SIZEOF(.visor_entry)) {
        __boot_code_base = .;
        KEEP(*(.boot_code));
        __boot_code_end = .;
    } > RAM

    .text : AT (LOADADDR(.boot_code) + SIZEOF(.boot_code)) {
        __user_text_base = .;
        . = ALIGN(32);
        *(.text)
        *(.text.*)
        KEEP (*(.fini .fini.*))

        . = ALIGN(4);
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

    .data : AT (LOADADDR(.text) + SIZEOF(.text)) {
        __user_data_base = .;
        . = ALIGN(32);
        *(.data)
        *(.data.*)
    } > RAM

    .rodata : AT (LOADADDR(.data) + SIZEOF(.data)) {
        __user_rodata_base = .;
        . = ALIGN(32);
        *(.rdata)
        *(.rdata.*)
        *(.rodata)
        *(.rodata.*)
    } > RAM

    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        __bss_end = .;
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

    __boot_code_source = ROM_BASE + LOADADDR(.boot_code);
    __boot_code_size = SIZEOF(.boot_code);

    __user_text_source = (ROM_BASE + LOADADDR(.text));
    __user_text_size = SIZEOF(.text);

    __user_data_source = (ROM_BASE + LOADADDR(.data));
    __user_data_size = SIZEOF(.data);

    __user_rodata_source = (ROM_BASE + LOADADDR(.rodata));
    __user_rodata_size = SIZEOF(.rodata);

    __bss_size = __bss_end - __bss_start;
    __bss_dsize = __bss_size / 4;
}
