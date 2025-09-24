#include "pic.h"
#include "../../helpers/ports/ports.h"
#include "../../drivers/display/display.h"

void pic_init(void)
{
    clear();
    write("Initializing PIC...\n");

    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    outb(0x21, 0xF9);
    outb(0xA1, 0x3E);

    write("\033[32mPIC initialized.\n\033[0m");
}