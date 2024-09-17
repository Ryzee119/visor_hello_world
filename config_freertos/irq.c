#include <stdint.h>
#include <stdio.h>

static const char *exception_fmt = "EIP: %08X CS: %04X EFLAGS: %08X\n";

void __attribute__((weak)) exception_handler0(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("DIV0 EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler1(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("DEBUG EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler2(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("NMI EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler3(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("BREAKPOINT EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler4(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("OVERFLOW EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler5(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("BOUND RANGE EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler6(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("INVALID OPCODE EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler7(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("DEVICE NOT AVAILABLE EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler8(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("DOUBLE FAULT EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler9(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("COPROCESSOR SEGMENT OVERRUN EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler10(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("INVALID TSS EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler11(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("SEGMENT NOT PRESENT EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler12(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("STACK SEGMENT FAULT EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler13(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("GENERAL PROTECTION FAULT EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler14(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("PAGE FAULT EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler15(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("RESERVED EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler16(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("x87 FLOATING POINT ERROR EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) exception_handler17(unsigned int eip, unsigned int cs, unsigned int eflags)
{
    printf("ALIGNMENT CHECK EXCEPTION\n");
    printf(exception_fmt, eip, cs, eflags);
}

void __attribute__((weak)) timer_handler()
{
    printf("TIMER\n");
}

void __attribute__((weak)) usb0_handler()
{
    printf("USB0\n");
}

void __attribute__((weak)) gpu_handler()
{
    printf("GPU\n");
}

void __attribute__((weak)) nic_handler()
{
    printf("NIC\n");
}

void __attribute__((weak)) apu_handler()
{
    printf("APU\n");
}

void __attribute__((weak)) aci_handler()
{
    printf("NIC\n");
}

void __attribute__((weak)) usb1_handler()
{
    printf("USB1\n");
}

void __attribute__((weak)) ide_handler()
{
    printf("IDE\n");
}
