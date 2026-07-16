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

unsigned int bytes_to_gdt_limit(unsigned int size_bytes)
{
    unsigned int pages = (size_bytes + SEGMENT_ALIGN - 1U) / SEGMENT_ALIGN;

    if (pages == 0)
        pages = 1;

    if (pages > 0x100000U)
        pages = 0x100000U;

    return pages - 1U;
}

void gdt_set_kernel_limit_bytes(unsigned int limit_bytes)
{
    unsigned int limit = bytes_to_gdt_limit(limit_bytes);

    set_gdt_entry(GDT_IDX_KERNEL_CODE, 0, limit, 0x9A, GDT_GRAN_4K);
    set_gdt_entry(GDT_IDX_KERNEL_DATA, 0, limit, 0x92, GDT_GRAN_4K);
}

void gdt_set_user_limit_bytes(unsigned int limit_bytes)
{
    unsigned int limit = bytes_to_gdt_limit(limit_bytes);

    set_gdt_entry(GDT_IDX_USER_CODE, USER_PHYS_BASE, limit, 0xFA, GDT_GRAN_4K);
    set_gdt_entry(GDT_IDX_USER_DATA, USER_PHYS_BASE, limit, 0xF2, GDT_GRAN_4K);
}

void reload_kernel_segments(void)
{
    __asm__ volatile(
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t" ::: "ax");
}

void reload_user_segments(void)
{
    __asm__ volatile(
        "mov $0x23, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t" ::: "ax");
}

void gdt_init(void)
{
    write("Initializing GDT...\n");

    set_gdt_entry(0, 0, 0, 0, 0);
    gdt_set_kernel_limit_bytes(KERNEL_SEGMENT_LIMIT);
    gdt_set_user_limit_bytes(SEGMENT_ALIGN);

    kernel_gdtr.base = (unsigned int)&kernel_gdt;
    kernel_gdtr.limit = (unsigned short)(sizeof(kernel_gdt) - 1);

    __asm__ volatile("lgdt (%0)" ::"r"(&kernel_gdtr));

    __asm__ volatile(
        "ljmp $0x08, $1f\n"
        "1:\n\t");

    reload_kernel_segments();

    write("\033[32mGDT initialized.\033[0m\n\n");
}
