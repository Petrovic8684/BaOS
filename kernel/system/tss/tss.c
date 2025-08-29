#include "tss.h"

static tss_t kernel_tss __attribute__((aligned(16)));

#define KERNEL_STACK_SIZE (16 * 1024)
static unsigned char kernel_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));

static inline unsigned int kernel_stack_top(void)
{
    return (unsigned int)(kernel_stack + KERNEL_STACK_SIZE);
}

static void set_tss_descriptor(int idx, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran)
{
    set_gdt_entry(idx, base, limit, access, gran);
}

void tss_init(void)
{
    write("Setting up kernel TSS...\n");

    for (int i = 0; i < (int)(sizeof(kernel_tss) / 4); ++i)
        ((unsigned int *)&kernel_tss)[i] = 0;

    kernel_tss.esp0 = kernel_stack_top();
    kernel_tss.ss0 = 0x10;

    set_tss_descriptor(5, (unsigned int)&kernel_tss,
                       sizeof(kernel_tss) - 1, 0x89, 0x00);

    asm volatile("ltr %%ax" ::"a"(0x28));

    write_colored("TSS loaded.\n", 0x02);
}