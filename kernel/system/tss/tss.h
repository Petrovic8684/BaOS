#ifndef TSS_H
#define TSS_H

#include "../../drivers/display/display.h"
#include "../gdt/gdt.h"

typedef struct
{
    unsigned int prev_tss;
    unsigned int esp0;
    unsigned int ss0;
    unsigned int esp1, ss1;
    unsigned int esp2, ss2;
    unsigned int cr3;
    unsigned int eip;
    unsigned int eflags;
} __attribute__((packed)) tss_t;

void tss_init(void);
void tss_set_esp0(unsigned int esp0);

#endif