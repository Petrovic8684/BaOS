#ifndef IDT_H
#define IDT_H

typedef struct
{
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) idt_ptr_t;

void idt_init(void);
void set_idt_entry(int n, unsigned int handler, unsigned short selector, unsigned char type_attr);

#endif
