#include "paging.h"

unsigned int page_directory[PAGE_ENTRIES] __attribute__((aligned(4096)));
static unsigned int first_page_table[PAGE_ENTRIES] __attribute__((aligned(4096)));
static unsigned int page_table_pool[PT_POOL_COUNT][PAGE_ENTRIES] __attribute__((aligned(4096)));
static unsigned char page_table_used[PT_POOL_COUNT] = {0};
static unsigned int get_physical_addr(void *p) { return (unsigned int)p; }

static unsigned int alloc_page_table_phys()
{
    for (int i = 0; i < PT_POOL_COUNT; ++i)
    {
        if (!page_table_used[i])
        {
            page_table_used[i] = 1;
            for (int j = 0; j < PAGE_ENTRIES; ++j)
                page_table_pool[i][j] = 0;

            return get_physical_addr(page_table_pool[i]);
        }
    }

    write("Out of page-tables pool!\n");
    for (;;)
    {
        __asm__ volatile("hlt");
    }
}

void map_page(unsigned int virt, unsigned int phys, unsigned int flags)
{
    unsigned int pd_index = (virt >> 22) & 0x3FF;
    unsigned int pt_index = (virt >> 12) & 0x3FF;

    unsigned int pde = page_directory[pd_index];
    unsigned int *page_table;
    if (!(pde & PAGE_PRESENT))
    {
        unsigned int new_pt_phys = alloc_page_table_phys();
        page_directory[pd_index] = (new_pt_phys & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW;
        page_table = (unsigned int *)(new_pt_phys);
    }
    else
    {
        page_table = (unsigned int *)(pde & 0xFFFFF000);
    }

    ((unsigned int *)page_table)[pt_index] = (phys & 0xFFFFF000) | (flags & 0xFFF) | PAGE_PRESENT;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void unmap_page(unsigned int virt)
{
    unsigned int pd_index = (virt >> 22) & 0x3FF;
    unsigned int pt_index = (virt >> 12) & 0x3FF;
    unsigned int pde = page_directory[pd_index];

    if (!(pde & PAGE_PRESENT))
        return;

    unsigned int *page_table = (unsigned int *)(pde & 0xFFFFF000);
    page_table[pt_index] = 0;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void set_user_pages(unsigned int phys_start, unsigned int size)
{
    unsigned int addr;
    unsigned int end = phys_start + size;
    for (addr = phys_start & 0xFFFFF000; addr < end; addr += PAGE_SIZE)
    {
        unsigned int pd_index = (addr >> 22) & 0x3FF;
        unsigned int pt_index = (addr >> 12) & 0x3FF;
        unsigned int pde = page_directory[pd_index];

        if (!(pde & PAGE_PRESENT))
        {
            unsigned int new_pt_phys = alloc_page_table_phys();
            page_directory[pd_index] = (new_pt_phys & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
            unsigned int *table = (unsigned int *)new_pt_phys;

            for (int i = 0; i < PAGE_ENTRIES; i++)
                table[i] = 0;

            table[pt_index] = (addr & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
        }
        else
        {
            page_directory[pd_index] |= PAGE_USER;

            unsigned int *table = (unsigned int *)(pde & 0xFFFFF000);
            table[pt_index] = (table[pt_index] & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
        }
        __asm__ volatile("invlpg (%0)" ::"r"(addr) : "memory");
    }
}

unsigned int get_pte(unsigned int virt)
{
    unsigned int pd_index = (virt >> 22) & 0x3FF;
    unsigned int pt_index = (virt >> 12) & 0x3FF;
    unsigned int pde = page_directory[pd_index];
    if (!(pde & PAGE_PRESENT))
        return 0;
    unsigned int *pt = (unsigned int *)(pde & 0xFFFFF000);
    return pt[pt_index];
}

unsigned int get_cr2(void)
{
    unsigned int v;
    __asm__ volatile("mov %%cr2, %0" : "=r"(v));
    return v;
}

void paging_install(void)
{
    clear();
    write("About to enable paging...\n");

    for (int i = 0; i < PAGE_ENTRIES; ++i)
        first_page_table[i] = 0;
    for (int i = 0; i < PAGE_ENTRIES; ++i)
        page_directory[i] = 0;

    for (unsigned int i = 0; i < PAGE_ENTRIES; ++i)
        first_page_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW; // no PAGE_USER

    page_directory[0] = ((unsigned int)first_page_table & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW;

    for (int i = 0; i < PT_POOL_COUNT; i++)
        page_table_used[i] = 0;

    unsigned int pd_phys = (unsigned int)page_directory;
    __asm__ volatile("mov %0, %%cr3" ::"r"(pd_phys));

    unsigned int cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // PG
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));

    write("Paging enabled (kernel protected, first 4MB)\n");
}