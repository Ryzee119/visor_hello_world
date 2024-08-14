#include <stdio.h>
#include "xbox.h"

void main(void);

#ifndef XBOX_DISABLE_STDIO_HOOK
// Hook printf to output to serial port
static int _putc(char c, FILE *file)
{
	(void) file;
    serial_putchar(c);
	return c;
}

static FILE __stdio = FDEV_SETUP_STREAM(_putc, NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const stdin = &__stdio; __strong_reference(stdin, stdout); __strong_reference(stdin, stderr);
#endif

void boot(void) {
    serial_init();

    main();
}