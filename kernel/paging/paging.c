#include "paging.h"

unsigned int page_directory[PAGE_ENTRIES] __attribute__((aligned(4096)));
static unsigned int first_page_table[PAGE_ENTRIES] __attribute__((aligned(4096)));
static unsigned int page_table_pool[PT_POOL_COUNT][PAGE_ENTRIES] __attribute__((aligned(4096)));
static unsigned char page_table_used[PT_POOL_COUNT] = {0};

static unsigned int get_physical_addr(void *p) { return (unsigned int)p; }

static unsigned int alloc_page_table_phys()
{
    unsigned int i, j;
    for (i = 0; i < PT_POOL_COUNT; ++i)
    {
        if (!page_table_used[i])
        {
            page_table_used[i] = 1;
            for (j = 0; j < PAGE_ENTRIES; ++j)
                page_table_pool[i][j] = 0;

            return get_physical_addr(&page_table_pool[i][0]);
        }
    }

    write_colored("Out of page-tables pool. Halting...\n", 0x04);
    for (;;)
        __asm__ volatile("hlt");
}

static void free_page_table_phys(unsigned int phys)
{
    if ((phys & 0xFFFFF000) == ((unsigned int)first_page_table & 0xFFFFF000))
        return;

    unsigned int i, j;
    for (i = 0; i < PT_POOL_COUNT; ++i)
    {
        if (((unsigned int)&page_table_pool[i][0] & 0xFFFFF000) == (phys & 0xFFFFF000))
        {
            for (j = 0; j < PAGE_ENTRIES; ++j)
                page_table_pool[i][j] = 0;
            page_table_used[i] = 0;
            return;
        }
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
        page_table = (unsigned int *)(pde & 0xFFFFF000);

    page_table[pt_index] = (phys & 0xFFFFF000) | (flags & 0xFFF) | PAGE_PRESENT;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void set_user_pages(unsigned int virt_start, unsigned int size)
{
    unsigned int addr;
    unsigned int end = virt_start + size;
    for (addr = virt_start & 0xFFFFF000; addr < end; addr += PAGE_SIZE)
    {
        unsigned int pd_index = (addr >> 22) & 0x3FF;
        unsigned int pt_index = (addr >> 12) & 0x3FF;
        unsigned int pde = page_directory[pd_index];

        unsigned int *table;
        unsigned int table_phys;

        if (!(pde & PAGE_PRESENT))
        {
            unsigned int new_pt_phys = alloc_page_table_phys();
            page_directory[pd_index] = (new_pt_phys & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
            table = (unsigned int *)new_pt_phys;
            table_phys = new_pt_phys;
        }
        else
        {
            table_phys = pde & 0xFFFFF000;

            if (table_phys == ((unsigned int)first_page_table & 0xFFFFF000))
            {
                unsigned int new_pt_phys = alloc_page_table_phys();

                unsigned int *old_table = (unsigned int *)table_phys;
                unsigned int *new_table = (unsigned int *)new_pt_phys;

                unsigned int i;
                for (i = 0; i < PAGE_ENTRIES; ++i)
                {
                    unsigned int ent = old_table[i];
                    if (ent & PAGE_PRESENT)
                        new_table[i] = ent;
                    else
                        new_table[i] = 0;
                }

                page_directory[pd_index] = (new_pt_phys & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW | PAGE_USER;

                table = new_table;
                table_phys = new_pt_phys;
            }
            else
            {
                table = (unsigned int *)table_phys;
                page_directory[pd_index] = (page_directory[pd_index] | PAGE_USER);
            }
        }

        table[pt_index] = (addr & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
        __asm__ volatile("invlpg (%0)" ::"r"(addr) : "memory");
    }
}

void ensure_phys_range_mapped(unsigned int phys_start, unsigned int size)
{
    unsigned int start = phys_start & 0xFFFFF000u;
    unsigned int end = (phys_start + size + 0xFFFu) & ~0xFFFu;
    unsigned int addr;
    for (addr = start; addr < end; addr += 0x1000u)
        if (get_pte(addr) == 0)
            map_page(addr, addr, PAGE_RW);
}

void unmap_page(unsigned int virt)
{
    unsigned int pd_index = (virt >> 22) & 0x3FF;
    unsigned int pt_index = (virt >> 12) & 0x3FF;
    unsigned int pde = page_directory[pd_index];

    if (!(pde & PAGE_PRESENT))
        return;

    if (!(pde & PAGE_USER))
        return;

    unsigned int table_phys = pde & 0xFFFFF000;
    unsigned int *page_table = (unsigned int *)table_phys;

    page_table[pt_index] = 0;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");

    unsigned int i;
    int empty = 1;
    for (i = 0; i < PAGE_ENTRIES; ++i)
    {
        unsigned int ent = page_table[i];
        if (ent & PAGE_PRESENT)
        {
            empty = 0;
            break;
        }
    }

    if (empty)
    {
        page_directory[pd_index] = 0;
        free_page_table_phys(table_phys);
    }
}

void unmap_user_range(unsigned int virt_start, unsigned int size)
{
    if (size == 0)
        return;

    unsigned int start = virt_start & 0xFFFFF000u;
    unsigned int end = (virt_start + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    unsigned int addr;
    for (addr = start; addr < end; addr += PAGE_SIZE)
        unmap_page(addr);
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

    unsigned int i;
    for (i = 0; i < PAGE_ENTRIES; ++i)
        first_page_table[i] = 0;
    for (i = 0; i < PAGE_ENTRIES; ++i)
        page_directory[i] = 0;

    for (i = 0; i < PAGE_ENTRIES; ++i)
        first_page_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW;

    page_directory[0] = ((unsigned int)first_page_table & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW;

    for (i = 0; i < PT_POOL_COUNT; i++)
        page_table_used[i] = 0;

    unsigned int pd_phys = (unsigned int)page_directory;
    __asm__ volatile("mov %0, %%cr3" ::"r"(pd_phys));

    unsigned int cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));

    write_colored("Paging enabled (kernel protected, first 4MB).\n", 0x02);
}
