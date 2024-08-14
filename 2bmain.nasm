; https://github.com/XboxDev/cromwell/blob/master/boot_rom/2bBootStartup.S

extern __ram_data_base
extern __ram_data_source
extern __ram_data_size
extern __bss_dsize
extern __bss_start
extern __stack
extern __libc_init_array
extern boot

%macro WR_MSR 3 ; addr, hi, lo
    mov ecx, %1
    mov edx, %2
    mov eax, %3
    wrmsr
%endmacro

; MCPX Magic Values
section .header00
    dd 0xff000009
    dd 0xff000008
    dd 0x2b16d065
    dd 0x3346322d
    dd 0x01010101
    dd 0x08080808
    dd 0x00000801

%ifdef MCPXREVD5
    dd 0xc8b4588a   ; MCPX 1.6 > D5
    dd 0x00100000
    dd 0x00050aa7
    dd 0xf0000000
%else
    dd 0xc8fc7c8a   ; MCPX =< D5
    dd 0x44290213
    dd 0x90004998
    dd 0x00000000
%endif

    dd 0xffffffff
    dd 0xffffffff

section .header40
    dd 0x00000000

section .header6C
    dd 0x00000107

; MCPX Config Area 
section .header70 
    dd 0x0000000f
    dd 0x40004400

%ifdef MCPXREVD5
    dd 0x16ce0090
    dd 0x00000dc0
%else
    dd 0x12d10070
    dd 0x00000c90
%endif

section .xcodes
    %include '2bxcodes.nasm'

; This is our entry point after visor roll over
section .visor_entry
  
    ; Clear Intel Interrupts in Processor Register
    mov ecx, 0x1b
    xor eax, eax
    xor edx, edx
    wrmsr

    ; Clear registers
    xor eax, eax
    xor edx, edx
    xor ecx, ecx

    ; Disable cache
    mov eax, cr0
    or eax, (0x60000000)
    mov cr0, eax
    wbinvd

    ; Clear CR3
    mov eax, eax
    mov cr3, eax

    ; Initialize MTRRs
    WR_MSR 0x2ff, 0, 0

    ; 0 +128M WB (RAM)
    WR_MSR 0x200, 0x00000000, 0x00000006 ; Base
    WR_MSR 0x201, 0x0000000f, 0xf8000800 ; Mask

    ; 0xf0000000 +128M WC (VRAM)
    WR_MSR 0x202, 0x00000000, 0xf0000001 ; Base
    WR_MSR 0x203, 0x0000000f, 0xf8000800 ; Mask

    ; 0xfff00000 +1M WP (Flash)
    WR_MSR 0x204, 0x00000000, 0xfff00005 ; Base
    WR_MSR 0x205, 0x0000000f, 0xfff00800 ; Mask

    ; Clear other MTRRs
    WR_MSR 0x204, 0x00000000, 0x00000000
    WR_MSR 0x205, 0x00000000, 0x00000000
    WR_MSR 0x206, 0x00000000, 0x00000000
    WR_MSR 0x207, 0x00000000, 0x00000000
    WR_MSR 0x208, 0x00000000, 0x00000000
    WR_MSR 0x209, 0x00000000, 0x00000000
    WR_MSR 0x20A, 0x00000000, 0x00000000
    WR_MSR 0x20B, 0x00000000, 0x00000000
    WR_MSR 0x20C, 0x00000000, 0x00000000
    WR_MSR 0x20D, 0x00000000, 0x00000000
    WR_MSR 0x20E, 0x00000000, 0x00000000
    WR_MSR 0x20F, 0x00000000, 0x00000000

    ; Enable MTRRs
    WR_MSR 0x2ff, 0x00000000, 0x00000800

    ; Enable Caching
    mov eax, cr0
    and eax, 0x9fffffff
    mov cr0, eax

    ; Clear out .bss
    xor eax, eax
    mov ecx, __bss_dsize
    mov edi, __bss_start
    rep stosd

    ; Enable Floating Point
    mov eax, cr4
    or  eax, (1<<9 | 1<<10)
    mov cr4, eax
    clts
    fninit

    ; Copy text into RAM
    mov edi, __ram_data_base
    mov esi, __ram_data_source
    mov ecx, __ram_data_size
    shr ecx, 2
    rep movsd

    ; Jump to c code
    jmp ram_entry;

section .text
ram_entry:

    ; Set the stack pointer
    mov esp, __stack

    ; Set the Interrupt Descriptor Table
    lidt [cs:idt_desc]

    ; Set the Global Descriptor Table
    lgdt [cs:gdt_desc]

    ; Far jump to reload CS with segment selector 0x0010
    jmp 0x0010:reload_segment_selector

align 16
reload_segment_selector:
    mov ax, 0x0018         ; Move 0x0018 into AX register
    mov ss, ax             ; Move AX into SS register
    mov ds, ax             ; Move AX into DS register
    mov es, ax             ; Move AX into ES register

    ; Clear FS and GS
    xor eax, eax           ; Clear EAX register
    mov fs, eax            ; Move EAX into FS register
    mov gs, eax            ; Move EAX into GS register

    call __libc_init_array

    jmp boot

section .data
align 0x10
gdt_table:
    dq 0x0000000000000000  ; Dummy
    dq 0x00CF9B000000FFFF  ; 0x0008 code32
    dq 0x00CF9B000000FFFF  ; 0x0010 code32
    dq 0x00CF93000000FFFF  ; 0x0018 data32
    dq 0x008F9B000000FFFF  ; 0x0020 code16 (8F indicates 4K granularity, huge limit)
    dq 0x008F93000000FFFF  ; 0x0028 data16
    dq 0x0000000000000000  ; Dummy

idt_table:
    times 0x5000 db 0    ; Allocate 0x5000 bytes (each initialized to 0)

gdt_desc:
    dw 0x30                ; Limit (size of GDT - 1)
    dd gdt_table           ; Base address of GDT
    dw 0x00;

idt_desc:
    dw 2048                ; 16-bit word (0x800 in hex, alignment padding for 16-bit descriptors)
    dd idt_table           ; 32-bit double word (address or other 32-bit value)
    dw 0x00;
