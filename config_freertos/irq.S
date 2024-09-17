.file "FreeRTOSIRQ.S"

/* The correcly handle IRQs with FreeRTOS, the IRQ need to wrap portFREERTOS_INTERRUPT_ENTRY and portINTERRUPT_EPILOGUE (or Exit)
   when calling the user handler. This file provide FreeRTOS friendly IRQs for  all the interrupts in the system.
*/

#include "FreeRTOSConfig.h"
#include "freertos_kernel/portable/GCC/IA32_flat/ISR_Support.h"

// create 3 dwords to store EFLAGS, CS, and EIP
.section .data
exception_frame: .long 0, 0, 0


.global freertos_usb0_interrupt
.text

.macro EXCEPTION_WRAPPER name, function
    .align 4
    .global \name
    .func \name
    \name :
        cld
        // Save the contents of eax, then store the exception frame
        pushl %eax
        movl 0x4(%esp), %eax
        movl %eax, exception_frame

        movl 0x8(%esp), %eax
        movl %eax, exception_frame+4

        movl 0x0C(%esp), %eax
        movl %eax, exception_frame+8
        popl %eax 

        pusha
        pushl exception_frame + 8    // EFLAGS
        pushl exception_frame + 4    // CS
        pushl exception_frame + 0    // EIP
        call \function
        popa

        hlt
        iret
    .endfunc
.endm

.macro INTERRUPT_WRAPPER name, function, source
    .align 4
    .global \name
    .func \name
    \name :
        portFREERTOS_INTERRUPT_ENTRY
        cld
        call \function
        portINTERRUPT_EPILOGUE

        movb $0x20, %al
        outb %al, $0x20

        .if \source == "pic2"
            movb $0x20, %al
            outb %al, $0xA0
        .endif

        popa
        iret
    .endfunc
.endm

EXCEPTION_WRAPPER freertos_exception0, exception_handler0
EXCEPTION_WRAPPER freertos_exception1, exception_handler1
EXCEPTION_WRAPPER freertos_exception2, exception_handler2
EXCEPTION_WRAPPER freertos_exception3, exception_handler3
EXCEPTION_WRAPPER freertos_exception4, exception_handler4
EXCEPTION_WRAPPER freertos_exception5, exception_handler5
EXCEPTION_WRAPPER freertos_exception6, exception_handler6
EXCEPTION_WRAPPER freertos_exception7, exception_handler7
EXCEPTION_WRAPPER freertos_exception8, exception_handler8
EXCEPTION_WRAPPER freertos_exception9, exception_handler9
EXCEPTION_WRAPPER freertos_exception10, exception_handler10
EXCEPTION_WRAPPER freertos_exception11, exception_handler11
EXCEPTION_WRAPPER freertos_exception12, exception_handler12
EXCEPTION_WRAPPER freertos_exception13, exception_handler13
EXCEPTION_WRAPPER freertos_exception14, exception_handler14
EXCEPTION_WRAPPER freertos_exception15, exception_handler15
EXCEPTION_WRAPPER freertos_exception16, exception_handler16


INTERRUPT_WRAPPER freertos_usb0_interrupt, usb0_handler, "pic1"
INTERRUPT_WRAPPER freertos_gpu_interrupt, gpu_handler, "pic1"
INTERRUPT_WRAPPER freertos_nic_interrupt, nic_handler, "pic1"
INTERRUPT_WRAPPER freertos_apu_interrupt, apu_handler, "pic1"
INTERRUPT_WRAPPER freertos_aci_interrupt, aci_handler, "pic1"
INTERRUPT_WRAPPER freertos_usb1_interrupt, usb1_handler, "pic2"
INTERRUPT_WRAPPER freertos_ide_interrupt, ide_handler, "pic2"

.end
