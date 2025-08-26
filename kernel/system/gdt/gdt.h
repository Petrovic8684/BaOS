#ifndef GDT_H
#define GDT_H

#define GDT_ENTRIES 6

#include "../../drivers/display/display.h"

typedef struct
{
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_mid;
    unsigned char access;
    unsigned char gran;
    unsigned char base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) gdtr_t;

void gdt_init(void);
void set_gdt_entry(int idx, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran);

#endif