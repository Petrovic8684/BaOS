#include "idt.h"

#define IDT_SIZE 256

idt_entry_t idt[IDT_SIZE];
idt_ptr_t idt_ptr;

extern void idt_flush(unsigned int);

void set_idt_entry(int n, unsigned int handler, unsigned short selector, unsigned char type_attr)
{
    idt[n].offset_low = handler & 0xFFFF;
    idt[n].selector = selector;
    idt[n].zero = 0;
    idt[n].type_attr = type_attr;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

void init_idt()
{
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_SIZE - 1;
    idt_ptr.base = (unsigned int)&idt;

    for (int i = 0; i < IDT_SIZE; i++)
        set_idt_entry(i, 0, 0, 0);

    extern void syscall_interrupt_handler();
    set_idt_entry(0x80, (unsigned int)syscall_interrupt_handler, 0x08, 0xEE);

    idt_flush((unsigned int)&idt_ptr);
}
