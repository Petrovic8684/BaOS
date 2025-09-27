#include "idt.h"
#include "isr/isr_handlers.h"
#include "../../drivers/keyboard/keyboard.h"
#include "../../drivers/mouse/mouse.h"
#include "../../drivers/rtc/rtc.h"
#include "../../drivers/pit/pit.h"
#include "../../drivers/disk/ata.h"
#include "../../drivers/display/display.h"

static idt_entry_t idt[IDT_SIZE];
static idt_ptr_t idt_ptr;

extern void idt_flush(unsigned int);

typedef void (*irq_handler_t)(int irq);
static irq_handler_t irq_handlers[256];

typedef void (*isr_t)(unsigned int error_code, unsigned int *frame_ptr);
static isr_t isr_handlers[32];

static void register_irq_handler(int n, irq_handler_t h)
{
    irq_handlers[n] = h;
}

void irq_common_handler(int irq)
{
    if (irq >= 0 && irq < 256 && irq_handlers[irq])
        irq_handlers[irq](irq);
}

static void register_isr_handler(int vec, isr_t h)
{
    if (vec >= 0 && vec < 32)
        isr_handlers[vec] = h;
}

void isr_common_handler(int vector, unsigned int error_code, unsigned int *frame_ptr)
{
    if (vector >= 0 && vector < 32 && isr_handlers[vector])
        isr_handlers[vector](error_code, frame_ptr);
    else
        for (;;)
            __asm__ volatile("hlt");
}

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

extern void syscall_interrupt_handler();

static void set_idt_entry(int n, unsigned int handler, unsigned short selector, unsigned char type_attr)
{
    idt[n].offset_low = handler & 0xFFFF;
    idt[n].selector = selector;
    idt[n].zero = 0;
    idt[n].type_attr = type_attr;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

void idt_init(void)
{
    write("Initializing IDT...\n");

    idt_ptr.limit = sizeof(idt_entry_t) * IDT_SIZE - 1;
    idt_ptr.base = (unsigned int)&idt;

    for (int i = 0; i < IDT_SIZE; i++)
        set_idt_entry(i, 0, 0, 0);

    set_idt_entry(0x80, (unsigned int)syscall_interrupt_handler, 0x08, 0xEE);

    set_idt_entry(0, (unsigned int)isr0, 0x08, 0x8E);
    set_idt_entry(1, (unsigned int)isr1, 0x08, 0x8E);
    set_idt_entry(2, (unsigned int)isr2, 0x08, 0x8E);
    set_idt_entry(3, (unsigned int)isr3, 0x08, 0xEE);
    set_idt_entry(4, (unsigned int)isr4, 0x08, 0x8E);
    set_idt_entry(5, (unsigned int)isr5, 0x08, 0x8E);
    set_idt_entry(6, (unsigned int)isr6, 0x08, 0x8E);
    set_idt_entry(7, (unsigned int)isr7, 0x08, 0x8E);
    set_idt_entry(8, (unsigned int)isr8, 0x08, 0x8E);
    set_idt_entry(9, (unsigned int)isr9, 0x08, 0x8E);
    set_idt_entry(10, (unsigned int)isr10, 0x08, 0x8E);
    set_idt_entry(11, (unsigned int)isr11, 0x08, 0x8E);
    set_idt_entry(12, (unsigned int)isr12, 0x08, 0x8E);
    set_idt_entry(13, (unsigned int)isr13, 0x08, 0x8E);
    set_idt_entry(14, (unsigned int)isr14, 0x08, 0x8E);
    set_idt_entry(15, (unsigned int)isr15, 0x08, 0x8E);
    set_idt_entry(16, (unsigned int)isr16, 0x08, 0x8E);
    set_idt_entry(17, (unsigned int)isr17, 0x08, 0x8E);
    set_idt_entry(18, (unsigned int)isr18, 0x08, 0x8E);
    set_idt_entry(19, (unsigned int)isr19, 0x08, 0x8E);
    set_idt_entry(20, (unsigned int)isr20, 0x08, 0x8E);
    set_idt_entry(21, (unsigned int)isr21, 0x08, 0x8E);
    set_idt_entry(22, (unsigned int)isr22, 0x08, 0x8E);
    set_idt_entry(23, (unsigned int)isr23, 0x08, 0x8E);
    set_idt_entry(24, (unsigned int)isr24, 0x08, 0x8E);
    set_idt_entry(25, (unsigned int)isr25, 0x08, 0x8E);
    set_idt_entry(26, (unsigned int)isr26, 0x08, 0x8E);
    set_idt_entry(27, (unsigned int)isr27, 0x08, 0x8E);
    set_idt_entry(28, (unsigned int)isr28, 0x08, 0x8E);
    set_idt_entry(29, (unsigned int)isr29, 0x08, 0x8E);
    set_idt_entry(30, (unsigned int)isr30, 0x08, 0x8E);
    set_idt_entry(31, (unsigned int)isr31, 0x08, 0x8E);

    set_idt_entry(0x20, (unsigned int)irq0, 0x08, 0x8E);
    set_idt_entry(0x21, (unsigned int)irq1, 0x08, 0x8E);
    set_idt_entry(0x22, (unsigned int)irq2, 0x08, 0x8E);
    set_idt_entry(0x23, (unsigned int)irq3, 0x08, 0x8E);
    set_idt_entry(0x24, (unsigned int)irq4, 0x08, 0x8E);
    set_idt_entry(0x25, (unsigned int)irq5, 0x08, 0x8E);
    set_idt_entry(0x26, (unsigned int)irq6, 0x08, 0x8E);
    set_idt_entry(0x27, (unsigned int)irq7, 0x08, 0x8E);
    set_idt_entry(0x28, (unsigned int)irq8, 0x08, 0x8E);
    set_idt_entry(0x29, (unsigned int)irq9, 0x08, 0x8E);
    set_idt_entry(0x2A, (unsigned int)irq10, 0x08, 0x8E);
    set_idt_entry(0x2B, (unsigned int)irq11, 0x08, 0x8E);
    set_idt_entry(0x2C, (unsigned int)irq12, 0x08, 0x8E);
    set_idt_entry(0x2D, (unsigned int)irq13, 0x08, 0x8E);
    set_idt_entry(0x2E, (unsigned int)irq14, 0x08, 0x8E);
    set_idt_entry(0x2F, (unsigned int)irq15, 0x08, 0x8E);

    idt_flush((unsigned int)&idt_ptr);

    register_isr_handler(0, isr0_handler);
    register_isr_handler(1, isr1_handler);
    register_isr_handler(2, isr2_handler);
    register_isr_handler(3, isr3_handler);
    register_isr_handler(4, isr4_handler);
    register_isr_handler(5, isr5_handler);
    register_isr_handler(6, isr6_handler);
    register_isr_handler(7, isr7_handler);
    register_isr_handler(8, isr8_handler);
    register_isr_handler(9, isr9_handler);
    register_isr_handler(10, isr10_handler);
    register_isr_handler(11, isr11_handler);
    register_isr_handler(12, isr12_handler);
    register_isr_handler(13, isr13_handler);
    register_isr_handler(14, isr14_handler);
    register_isr_handler(15, isr15_handler);
    register_isr_handler(16, isr16_handler);
    register_isr_handler(17, isr17_handler);
    register_isr_handler(18, isr18_handler);
    register_isr_handler(19, isr19_handler);
    register_isr_handler(20, isr20_handler);
    register_isr_handler(21, isr21_handler);
    register_isr_handler(22, isr22_handler);
    register_isr_handler(23, isr23_handler);
    register_isr_handler(24, isr24_handler);
    register_isr_handler(25, isr25_handler);
    register_isr_handler(26, isr26_handler);
    register_isr_handler(27, isr27_handler);
    register_isr_handler(28, isr28_handler);
    register_isr_handler(29, isr29_handler);
    register_isr_handler(30, isr30_handler);
    register_isr_handler(31, isr31_handler);

    register_irq_handler(0x20, pit_irq_handler);
    register_irq_handler(0x21, keyboard_irq_handler);
    register_irq_handler(0x28, rtc_irq_handler);
    register_irq_handler(0x2C, mouse_irq_handler);
    register_irq_handler(0x2E, ata_irq_handler);
    register_irq_handler(0x2F, ata_irq_handler);

    __asm__ volatile("sti");

    write("\033[32mIDT initialized.\033[0m\n\n");
}
