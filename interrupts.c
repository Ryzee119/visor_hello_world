#include "main.h"
#include <stdio.h>

static void print_idt_entries();

void interrupts_init()
{
    // For exceptions we cant use FreeRTOS API, so we need to install directly
    idt_install_interrupt(0, freertos_exception0, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(1, freertos_exception1, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(2, freertos_exception2, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(3, freertos_exception3, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(4, freertos_exception4, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(5, freertos_exception5, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(6, freertos_exception6, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(7, freertos_exception7, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(8, freertos_exception8, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(9, freertos_exception9, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(10, freertos_exception10, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(11, freertos_exception11, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(12, freertos_exception12, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(13, freertos_exception13, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(14, freertos_exception14, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(15, freertos_exception15, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(16, freertos_exception16, IDT_PRESENT | IDT_TYPE_INT_GATE);

    #define PIC_BASE(pic_irq) (pic_irq < 8 ? XBOX_PIC1_BASE_VECTOR_ADDRESS : XBOX_PIC2_BASE_VECTOR_ADDRESS)
    xPortInstallInterruptHandler(freertos_usb0_interrupt, PIC_BASE(PCI_USB0_IRQ) + PCI_USB0_IRQ);
    xPortInstallInterruptHandler(freertos_gpu_interrupt, PIC_BASE(PCI_GPU_IRQ) + PCI_GPU_IRQ);
    xPortInstallInterruptHandler(freertos_nic_interrupt, PIC_BASE(PCI_NIC_IRQ) + PCI_NIC_IRQ);
    xPortInstallInterruptHandler(freertos_apu_interrupt, PIC_BASE(PCI_APU_IRQ) + PCI_APU_IRQ);
    xPortInstallInterruptHandler(freertos_aci_interrupt, PIC_BASE(PCI_ACI_IRQ) + PCI_ACI_IRQ);
    xPortInstallInterruptHandler(freertos_usb1_interrupt, PIC_BASE(PCI_USB1_IRQ) + PCI_USB1_IRQ);
    xPortInstallInterruptHandler(freertos_ide_interrupt, PIC_BASE(PCI_IDE_IRQ) + PCI_IDE_IRQ);

    //print_idt_entries();
}

// Function to print IDT entries
static void print_idt_entries()
{
    IDTPointer_t idt_reg;

    __asm__ __volatile__("sidt %0" : "=m"(idt_reg));

    IDTEntry_t *idt = (IDTEntry_t *)idt_reg.ulTableBase;

    printf("IDT Base: 0x%x\n", idt_reg.ulTableBase);
    printf("IDT Limit: 0x%x\n", idt_reg.usTableLimit);

    for (int i = 0; i < (idt_reg.usTableLimit / sizeof(IDTEntry_t)); i++) {
        IDTEntry_t entry = idt[i];

        if (entry.usISRHigh == 0 && entry.usISRLow == 0) {
            continue;
        }
        printf("IDT Entry %02x:", i);
        printf("  Offset: 0x%x%04x ", entry.usISRHigh, entry.usISRLow);
        printf("  Selector: 0x%02x ", entry.usSegmentSelector);
        printf("  Type/Attr: 0x%02x\n", entry.ucFlags);
    }
}
