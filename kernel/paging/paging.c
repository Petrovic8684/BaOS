#include "paging.h"
#include "../drivers/display/display.h"

#define PT_POOL_COUNT 128
#define PAGE_RW 0x2
#define E820_MAP_ADDR 0x8000U

static unsigned int page_directory[PAGE_ENTRIES] __attribute__((aligned(4096)));
static unsigned int first_page_table[PAGE_ENTRIES] __attribute__((aligned(4096)));
static unsigned int page_table_pool[PT_POOL_COUNT][PAGE_ENTRIES] __attribute__((aligned(4096))) __attribute__((section(".page_tables")));
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

    write("\033[31mOut of page-tables pool. Halting...\n\033[0m");
    for (;;)
        __asm__ volatile("hlt");
}

struct e820_entry
{
    unsigned int base_lo;
    unsigned int base_hi;
    unsigned int len_lo;
    unsigned int len_hi;
    unsigned int type;
    unsigned int acpi;
};

static unsigned int get_e820_count(void)
{
    return *((unsigned int *)E820_MAP_ADDR);
}

static struct e820_entry *get_e820_entries(void)
{
    return (struct e820_entry *)(E820_MAP_ADDR + 4);
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

static int range_fully_usable(unsigned long long start, unsigned long long size)
{
    unsigned long long end = start + size;
    unsigned int count = get_e820_count();
    struct e820_entry *entries = get_e820_entries();
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        if (entries[i].type != 1)
            continue;

        unsigned long long base = ((unsigned long long)entries[i].base_hi << 32) | entries[i].base_lo;
        unsigned long long len = ((unsigned long long)entries[i].len_hi << 32) | entries[i].len_lo;
        unsigned long long eend = base + len;

        if (base <= start && eend >= end)
            return 1;
    }
    return 0;
}

static int addr_is_usable(unsigned long long addr)
{
    unsigned int count = get_e820_count();
    struct e820_entry *entries = get_e820_entries();
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        if (entries[i].type != 1)
            continue;

        unsigned long long base = ((unsigned long long)entries[i].base_hi << 32) | entries[i].base_lo;
        unsigned long long len = ((unsigned long long)entries[i].len_hi << 32) | entries[i].len_lo;
        if (addr >= base && addr < base + len)
            return 1;
    }
    return 0;
}

static void split_4mb_pde(unsigned int pd_index)
{
    unsigned int pde = page_directory[pd_index];
    if (!(pde & PAGE_PRESENT) || !(pde & PAGE_SIZE_4MB))
        return;

    unsigned int phys_base = pde & 0xFFC00000u;

    unsigned int new_pt_phys = alloc_page_table_phys();
    unsigned int *new_table = (unsigned int *)new_pt_phys;

    unsigned int i;
    for (i = 0; i < PAGE_ENTRIES; ++i)
    {
        unsigned int page_phys = phys_base + (i << 12);
        new_table[i] = (page_phys & 0xFFFFF000u) | PAGE_PRESENT | PAGE_RW;
    }

    page_directory[pd_index] = (new_pt_phys & 0xFFFFF000u) | PAGE_PRESENT | PAGE_RW;
}

static void map_page(unsigned int virt, unsigned int phys, unsigned int flags)
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
        if (pde & PAGE_SIZE_4MB)
        {
            split_4mb_pde(pd_index);
            pde = page_directory[pd_index];
        }
        page_table = (unsigned int *)(pde & 0xFFFFF000);
    }

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

            if (pde & PAGE_SIZE_4MB)
            {
                split_4mb_pde(pd_index);
                pde = page_directory[pd_index];
                table_phys = pde & 0xFFFFF000;
            }

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

static void unmap_page(unsigned int virt)
{
    unsigned int pd_index = (virt >> 22) & 0x3FF;
    unsigned int pt_index = (virt >> 12) & 0x3FF;
    unsigned int pde = page_directory[pd_index];

    if (!(pde & PAGE_PRESENT))
        return;

    if (!(pde & PAGE_USER))
        return;

    if (pde & PAGE_SIZE_4MB)
    {
        split_4mb_pde(pd_index);
        pde = page_directory[pd_index];
    }

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

void unmap_all_user_pages(void)
{
    unsigned int pd_index;
    for (pd_index = 0; pd_index < PAGE_ENTRIES; ++pd_index)
    {
        unsigned int pde = page_directory[pd_index];

        if (!(pde & PAGE_PRESENT))
            continue;

        if (pde & PAGE_SIZE_4MB)
        {
            if (pde & PAGE_USER)
            {
                page_directory[pd_index] = 0;

                unsigned int base = pd_index << 22;
                unsigned int addr;
                for (addr = base; addr < (base + (4U * 1024U * 1024U)); addr += PAGE_SIZE)
                    __asm__ volatile("invlpg (%0)" ::"r"(addr) : "memory");
            }
            continue;
        }

        unsigned int table_phys = pde & 0xFFFFF000u;
        unsigned int *table = (unsigned int *)table_phys;
        if (!table)
            continue;

        unsigned int i;
        int any_user_left = 0;

        for (i = 0; i < PAGE_ENTRIES; ++i)
        {
            unsigned int ent = table[i];
            if ((ent & PAGE_PRESENT) && (ent & PAGE_USER))
            {
                table[i] = 0;

                unsigned int addr = (pd_index << 22) | (i << 12);
                __asm__ volatile("invlpg (%0)" ::"r"(addr) : "memory");
            }
        }

        int empty = 1;
        for (i = 0; i < PAGE_ENTRIES; ++i)
        {
            if (table[i] & PAGE_PRESENT)
            {
                empty = 0;
                break;
            }
        }

        if (empty)
        {
            if (((unsigned int)table_phys & 0xFFFFF000u) != ((unsigned int)first_page_table & 0xFFFFF000u))
            {
                page_directory[pd_index] = 0;
                free_page_table_phys(table_phys);
            }
            else
                page_directory[pd_index] &= ~PAGE_USER;
        }
        else
            page_directory[pd_index] &= ~PAGE_USER;
    }
}

unsigned int get_pte(unsigned int virt)
{
    unsigned int pd_index = (virt >> 22) & 0x3FF;
    unsigned int pt_index = (virt >> 12) & 0x3FF;
    unsigned int pde = page_directory[pd_index];

    if (!(pde & PAGE_PRESENT))
        return 0;

    if (pde & PAGE_SIZE_4MB)
        return pde;

    unsigned int *pt = (unsigned int *)(pde & 0xFFFFF000);
    return pt[pt_index];
}

static unsigned int get_cr2(void)
{
    unsigned int v;
    __asm__ volatile("mov %%cr2, %0" : "=r"(v));
    return v;
}

void paging_init(void)
{
    write("About to enable paging...\n");

    unsigned long long total_detected = 0;
    unsigned long long total_mapped = 0;

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

    unsigned int count = get_e820_count();
    struct e820_entry *entries = get_e820_entries();

    unsigned int ei;
    for (ei = 0; ei < count; ++ei)
        if (entries[ei].type == 1)
        {
            unsigned long long base = ((unsigned long long)entries[ei].base_hi << 32) | entries[ei].base_lo;
            unsigned long long len = ((unsigned long long)entries[ei].len_hi << 32) | entries[ei].len_lo;

            total_detected += len;
        }

    unsigned int pd_index;
    for (pd_index = 1; pd_index < PAGE_ENTRIES; ++pd_index)
    {
        unsigned long long region_phys = ((unsigned long long)pd_index) << 22;
        unsigned long long region_size = 4ULL * 1024ULL * 1024ULL;

        if (range_fully_usable(region_phys, region_size))
        {
            unsigned int pde = ((unsigned int)(region_phys & 0xFFC00000u)) | PAGE_PRESENT | PAGE_RW | PAGE_SIZE_4MB;
            page_directory[pd_index] = pde;
            total_mapped += region_size;
        }
        else
        {
            unsigned int any_mapped = 0;
            unsigned int table_phys = 0;
            unsigned int *table = ((void *)0);
            unsigned long long page_addr;

            for (page_addr = region_phys; page_addr < region_phys + region_size; page_addr += PAGE_SIZE)
            {
                if (page_addr > 0xFFFFFFFFULL)
                    break;

                if (addr_is_usable(page_addr))
                {
                    if (!any_mapped)
                    {
                        table_phys = alloc_page_table_phys();
                        page_directory[pd_index] = (table_phys & 0xFFFFF000u) | PAGE_PRESENT | PAGE_RW;
                        table = (unsigned int *)table_phys;
                        any_mapped = 1;
                    }

                    unsigned int virt = (unsigned int)page_addr;
                    unsigned int phys = (unsigned int)page_addr;
                    unsigned int pt_index = (virt >> 12) & 0x3FF;

                    table[pt_index] = (phys & 0xFFFFF000u) | PAGE_PRESENT | PAGE_RW;
                    total_mapped += PAGE_SIZE;
                }
            }
        }
    }

    unsigned int cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 4);
    __asm__ volatile("mov %0, %%cr4" ::"r"(cr4));

    unsigned int pd_phys = (unsigned int)page_directory;
    __asm__ volatile("mov %0, %%cr3" ::"r"(pd_phys));

    unsigned int cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));

    write("Memory detected: ");
    {
        unsigned long long mb = total_detected / (1024ULL * 1024ULL);
        write_dec(mb);
    }
    write(" MB\n");
    write("\033[32mPaging enabled (Kernel protected, ");
    {
        unsigned long long mb = total_mapped / (1024ULL * 1024ULL);
        write_dec(mb);
    }
    write(" MB).\n\033[0m");
}
