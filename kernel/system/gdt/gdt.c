#include "gdt.h"
#include "../../drivers/display/display.h"

static gdt_entry_t kernel_gdt[GDT_ENTRIES] __attribute__((aligned(16)));
static gdtr_t kernel_gdtr __attribute__((aligned(8)));

void set_gdt_entry(int idx, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran)
{
    kernel_gdt[idx].limit_low = (unsigned short)(limit & 0xFFFF);
    kernel_gdt[idx].base_low = (unsigned short)(base & 0xFFFF);
    kernel_gdt[idx].base_mid = (unsigned char)((base >> 16) & 0xFF);
    kernel_gdt[idx].access = access;
    kernel_gdt[idx].gran = (unsigned char)(((limit >> 16) & 0x0F) | (gran & 0xF0));
    kernel_gdt[idx].base_high = (unsigned char)((base >> 24) & 0xFF);
}

void gdt_init(void)
{
    write("Setting up kernel GDT...\n");

    set_gdt_entry(0, 0, 0, 0, 0);
    set_gdt_entry(1, 0x00000000, 0x000FFFFF, 0x9A, 0xCF);
    set_gdt_entry(2, 0x00000000, 0x000FFFFF, 0x92, 0xCF);
    set_gdt_entry(3, 0x00000000, 0x000FFFFF, 0xFA, 0xCF);
    set_gdt_entry(4, 0x00000000, 0x000FFFFF, 0xF2, 0xCF);

    kernel_gdtr.base = (unsigned int)&kernel_gdt;
    kernel_gdtr.limit = (unsigned short)(sizeof(kernel_gdt) - 1);

    __asm__ volatile("lgdt (%0)" ::"r"(&kernel_gdtr));

    __asm__ volatile(
        "ljmp $0x08, $1f\n"
        "1:\n\t");

    __asm__ volatile(
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t" ::: "ax");

    write("\033[32mGDT loaded.\033[0m\n\n");
}
