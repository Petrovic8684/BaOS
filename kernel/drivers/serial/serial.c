#include "serial.h"
#include "../../helpers/ports/ports.h"
#include "../display/display.h"

#define COM1_PORT 0x3F8

static int serial_is_transmit_empty()
{
    return inb(COM1_PORT + 5) & 0x20;
}

static void serial_write_char(char a)
{
    while (serial_is_transmit_empty() == 0)
        ;
    outb(COM1_PORT, a);
}

void serial_write(const char *s)
{
    while (*s)
    {
        if (*s == '\n')
            serial_write_char('\r');
        serial_write_char(*s++);
    }
}

void serial_init()
{
    write("Initializing serial DEBUG driver...\n");

    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x01);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);

    write("\033[32mSerial DEBUG driver initialized.\033[0m\n\n");
}
