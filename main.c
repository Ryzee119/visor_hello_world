#define SERIAL_PORT 0x3f8
#define SERIAL_THR 0
#define SERIAL_LSR 5

#if (1)

static __inline void IoOutputByte(unsigned short address, unsigned char value) {
    __asm__ __volatile__ ("outb %b0,%w1": :"a" (value), "Nd" (address));
}

static __inline unsigned char IoInputByte(unsigned short address) {
  unsigned char v;

  __asm__ __volatile__ ("inb %w1,%0":"=a" (v):"Nd" (address));
  return v;
}

void serial_init()
{
    IoOutputByte(0x2e, 0x55);
    IoOutputByte(0x2e, 0x07);
    IoOutputByte(0x2f, 0x04);
    IoOutputByte(0x2e, 0x30);
    IoOutputByte(0x2f, 0x01);
    IoOutputByte(0x2e, 0x61);
    IoOutputByte(0x2f, SERIAL_PORT & 0xff);
    IoOutputByte(0x2e, 0x60);
    IoOutputByte(0x2f, SERIAL_PORT >> 8);
    IoOutputByte(0x2e, 0xAA);
}

int serial_putchar(int ch)
{
    /* Wait for THRE (bit 5) to be high */
    while ((IoInputByte(SERIAL_PORT + SERIAL_LSR) & (1<<5)) == 0);
    IoOutputByte(SERIAL_PORT + SERIAL_THR, ch);
    return ch;
}
#endif

extern void main(void) {
    serial_init();

    char *str = "Hello, World!\n";
    for (int i = 0; str[i] != '\0'; i++) {
        serial_putchar(str[i]);
    }
    while(1);

    return;
}