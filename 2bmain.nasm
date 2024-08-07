; https://github.com/XboxDev/cromwell/blob/master/boot_rom/2bBootStartup.S

extern RAM_CODE_BASE
extern RAM_CODE_BASE_IN_ROM
extern RAM_CODE_SIZE
extern BSS_SIZE_L
extern STACK_END
extern BSS_BASE

extern main

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
    or eax, 0x60000000
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

    ; Copy everything into RAM
    mov edi, RAM_CODE_BASE
    mov esi, RAM_CODE_BASE_IN_ROM
    mov ecx, RAM_CODE_SIZE
    shr ecx, 2
    rep movsd

    jmp  jump_to_ram

jump_to_ram:
    ; Set the stack pointer
    mov esp, STACK_END

    ; Clear out .bss
    xor eax, eax
    mov ecx, BSS_SIZE_L
    mov edi, BSS_BASE
    rep stosd

    ; Jump to c code
    jmp main
