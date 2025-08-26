#include "paging.h"
#include "../helpers/memory/memory.h"
#include "../drivers/display/display.h"

uint32_t page_directory[PAGE_ENTRIES] __attribute__((aligned(4096)));
static uint32_t first_page_table[PAGE_ENTRIES] __attribute__((aligned(4096)));

#define PT_POOL_COUNT 16
static uint32_t page_table_pool[PT_POOL_COUNT][PAGE_ENTRIES] __attribute__((aligned(4096)));
static uint8_t page_table_used[PT_POOL_COUNT] = {0};

static uint32_t get_physical_addr(void *p) { return (uint32_t)p; }

static uint32_t alloc_page_table_phys()
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

void map_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t pd_index = (virt >> 22) & 0x3FF;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    uint32_t pde = page_directory[pd_index];
    uint32_t *page_table;
    if (!(pde & PAGE_PRESENT))
    {
        uint32_t new_pt_phys = alloc_page_table_phys();
        page_directory[pd_index] = (new_pt_phys & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW;
        page_table = (uint32_t *)(new_pt_phys);
    }
    else
    {
        page_table = (uint32_t *)(pde & 0xFFFFF000);
    }

    ((uint32_t *)page_table)[pt_index] = (phys & 0xFFFFF000) | (flags & 0xFFF) | PAGE_PRESENT;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void unmap_page(uint32_t virt)
{
    uint32_t pd_index = (virt >> 22) & 0x3FF;
    uint32_t pt_index = (virt >> 12) & 0x3FF;
    uint32_t pde = page_directory[pd_index];

    if (!(pde & PAGE_PRESENT))
        return;

    uint32_t *page_table = (uint32_t *)(pde & 0xFFFFF000);
    page_table[pt_index] = 0;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void set_user_pages(uint32_t phys_start, uint32_t size)
{
    uint32_t addr;
    uint32_t end = phys_start + size;
    for (addr = phys_start & 0xFFFFF000; addr < end; addr += PAGE_SIZE)
    {
        uint32_t pd_index = (addr >> 22) & 0x3FF;
        uint32_t pt_index = (addr >> 12) & 0x3FF;
        uint32_t pde = page_directory[pd_index];

        if (!(pde & PAGE_PRESENT))
        {
            uint32_t new_pt_phys = alloc_page_table_phys();
            page_directory[pd_index] = (new_pt_phys & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
            uint32_t *table = (uint32_t *)new_pt_phys;

            for (int i = 0; i < PAGE_ENTRIES; i++)
                table[i] = 0;

            table[pt_index] = (addr & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
        }
        else
        {
            page_directory[pd_index] |= PAGE_USER;

            uint32_t *table = (uint32_t *)(pde & 0xFFFFF000);
            table[pt_index] = (table[pt_index] & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
        }
        __asm__ volatile("invlpg (%0)" ::"r"(addr) : "memory");
    }
}

uint32_t get_cr2(void)
{
    uint32_t v;
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

    for (uint32_t i = 0; i < PAGE_ENTRIES; ++i)
        first_page_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW; // no PAGE_USER

    page_directory[0] = ((uint32_t)first_page_table & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW;

    for (int i = 0; i < PT_POOL_COUNT; i++)
        page_table_used[i] = 0;

    uint32_t pd_phys = (uint32_t)page_directory;
    __asm__ volatile("mov %0, %%cr3" ::"r"(pd_phys));

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // PG
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));

    write("Paging enabled (kernel protected, first 4MB)\n");
}

/* debug/dump.c */

typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t gran;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdtr_t;

typedef struct
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr_t;

extern uint32_t get_pte(uint32_t virt);

static void write_hex_local(uint32_t v)
{
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; i++)
    {
        uint8_t nib = (v >> ((7 - i) * 4)) & 0xF;
        buf[2 + i] = (nib < 10) ? ('0' + nib) : ('A' + (nib - 10));
    }
    buf[10] = '\0';
    write(buf);
}

static inline void read_gdtr(gdtr_t *out) { __asm__ volatile("sgdt %0" : "=m"(*out)); }
static inline uint16_t read_tr(void)
{
    uint16_t tr;
    __asm__ volatile("str %0" : "=r"(tr));
    return tr;
}
static inline void read_idtr(idtr_t *out) { __asm__ volatile("sidt %0" : "=m"(*out)); }

void dump_gdt_first6(void)
{
    gdtr_t gdtr;
    read_gdtr(&gdtr);
    write("GDTR base = ");
    write_hex_local(gdtr.base);
    write(" limit = ");
    write_hex_local(gdtr.limit);
    write("\n");
    for (int i = 0; i < 6; ++i)
    {
        uint8_t *g = (uint8_t *)(gdtr.base + i * 8);
        gdt_entry_t *d = (gdt_entry_t *)g;
        uint32_t base = (d->base_low) | (d->base_mid << 16) | (d->base_high << 24);
        uint32_t limit = (d->limit_low) | ((d->gran & 0x0F) << 16);
        write("GDT[");
        char idx = '0' + i;
        write(&idx);
        write("] base=");
        write_hex_local(base);
        write(" limit=");
        write_hex_local(limit);
        write(" access=");
        write_hex_local(d->access);
        write(" gran=");
        write_hex_local(d->gran);
        write("\n");
    }
    uint16_t tr = read_tr();
    write("Loaded TR = ");
    {
        char t[7];
        t[0] = '0';
        t[1] = 'x';
        uint16_t v = tr;
        for (int i = 0; i < 4; i++)
        {
            uint8_t n = (v >> ((3 - i) * 4)) & 0xF;
            t[2 + i] = (n < 10) ? ('0' + n) : ('A' + (n - 10));
        }
        t[6] = '\0';
        write(t);
        write("\n");
    }
}

void dump_idt_entry14(void)
{
    idtr_t idtr;
    read_idtr(&idtr);
    write("IDTR base = ");
    write_hex_local(idtr.base);
    write(" limit = ");
    write_hex_local(idtr.limit);
    write("\n");
    idt_entry_t *e = (idt_entry_t *)(idtr.base + 14 * sizeof(idt_entry_t));
    uint32_t offset = ((uint32_t)e->offset_high << 16) | e->offset_low;
    write("IDT[14] offset=");
    write_hex_local(offset);
    write(" selector=");
    write_hex_local(e->selector);
    write(" type_attr=");
    write_hex_local(e->type_attr);
    write("\n");
}

/* Checks PTE and PDE bits and prints them nicely */
void dump_pte_info(uint32_t virt)
{
    uint32_t pd_index = (virt >> 22) & 0x3FF;
    uint32_t pt_index = (virt >> 12) & 0x3FF;
    uint32_t pde = page_directory[pd_index];
    write("PDE for virt ");
    write_hex_local(virt);
    write(" = ");
    write_hex_local(pde);
    write("\n");
    if (!(pde & PAGE_PRESENT))
    {
        write("  PDE not present\n");
        return;
    }
    uint32_t *pt = (uint32_t *)(pde & 0xFFFFF000);
    uint32_t pte = pt[pt_index];
    write("PTE = ");
    write_hex_local(pte);
    write("\n");
    write("  present: ");
    write((pte & PAGE_PRESENT) ? "yes\n" : "no\n");
    write("  rw: ");
    write((pte & PAGE_RW) ? "yes\n" : "no\n");
    write("  user: ");
    write((pte & PAGE_USER) ? "yes\n" : "no\n");
    write("  phys addr: ");
    write_hex_local(pte & 0xFFFFF000);
    write("\n");
}

void dump_descriptors_and_ptes(uint32_t user_entry, uint32_t user_stack_top)
{
    /*write("=== DUMP START ===\n");
    dump_gdt_first6();
    dump_idt_entry14();
    write("\n-- USER entry PTE --\n");
    dump_pte_info(user_entry);
    write("\n-- USER stack PTE (top-4) --\n");
    dump_pte_info(user_stack_top - 4);
    write("\n-- KERNEL stack PTE (0x90000) --\n");
    dump_pte_info(0x00090000);
    write("=== DUMP END ===\n");*/
}

/* GDT entry (8 bytes) */

/* GDTR */

/* Minimal TSS we need */
typedef struct
{
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    /* rest omitted */
} __attribute__((packed)) tss_t;

/* Alociramo GDT i TSS statički u kernel prostoru (poravnati) */
#define GDT_ENTRIES 6
static gdt_entry_t kernel_gdt[GDT_ENTRIES] __attribute__((aligned(16)));
static gdtr_t kernel_gdtr __attribute__((aligned(8)));
static tss_t kernel_tss __attribute__((aligned(16)));

/* helper za popunjavanje GDT entry-ja */
static void set_gdt_entry(int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    kernel_gdt[idx].limit_low = (uint16_t)(limit & 0xFFFF);
    kernel_gdt[idx].base_low = (uint16_t)(base & 0xFFFF);
    kernel_gdt[idx].base_mid = (uint8_t)((base >> 16) & 0xFF);
    kernel_gdt[idx].access = access;
    kernel_gdt[idx].gran = (uint8_t)(((limit >> 16) & 0x0F) | (gran & 0xF0));
    kernel_gdt[idx].base_high = (uint8_t)((base >> 24) & 0xFF);
}

/* zapis TSS descriptor (special: not a normal code/data) */
static void set_tss_descriptor(int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    /* same layout as set_gdt_entry (TSS is a system segment descriptor) */
    set_gdt_entry(idx, base, limit, access, gran);
}

/* exportovana funkcija koju pozivaš iz kernel_main */
void setup_kernel_gdt_and_tss(void)
{
    write("Setting up kernel GDT and TSS...\n");

    /* 0: null descriptor */
    set_gdt_entry(0, 0, 0, 0, 0);

    /* 1: kernel code segment - base=0 limit=0xFFFFF (4GB with gran=4k), access=0x9A */
    /* access 0x9A = present(1) DPL=0 S=1 type=0xA (code, exec/read) */
    set_gdt_entry(1, 0x00000000, 0x000FFFFF, 0x9A, 0xCF);

    /* 2: kernel data segment - access 0x92 (present, DPL=0, data read/write) */
    set_gdt_entry(2, 0x00000000, 0x000FFFFF, 0x92, 0xCF);

    /* 3: user code segment - DPL=3 -> access = 0xFA */
    set_gdt_entry(3, 0x00000000, 0x000FFFFF, 0xFA, 0xCF);

    /* 4: user data segment - DPL=3 -> access = 0xF2 */
    set_gdt_entry(4, 0x00000000, 0x000FFFFF, 0xF2, 0xCF);

    /* 5: TSS descriptor - we'll fill base to kernel_tss */
    for (int i = 0; i < sizeof(kernel_tss) / 4; ++i)
        ((uint32_t *)&kernel_tss)[i] = 0;
    kernel_tss.esp0 = 0x00090000; /* ensure this is a valid kernel stack address you have */
    kernel_tss.ss0 = 0x10;        /* kernel data selector */
    /* TSS limit: size - 1 */
    set_tss_descriptor(5, (uint32_t)&kernel_tss, sizeof(kernel_tss) - 1, 0x89, 0x00);
    /* access 0x89 = present, DPL=0, type=9 (32-bit available TSS) */

    /* Prepare GDTR */
    kernel_gdtr.base = (uint32_t)&kernel_gdt;
    kernel_gdtr.limit = (uint16_t)(sizeof(kernel_gdt) - 1);

    /* Load GDT */
    asm volatile("lgdt (%0)" ::"r"(&kernel_gdtr));

    /* Far jump to reload CS to our new GDT's kernel code selector (0x08).
       After LGDT, CS still points to an entry of the old GDT until far-jumped. */
    asm volatile(
        "ljmp $0x08, $1f\n"
        "1:\n\t");

    /* Reload data segment registers with kernel data selector (0x10) */
    asm volatile(
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t" ::: "ax");

    /* Load TR for TSS (selector = index<<3 = 5*8 = 0x28) */
    asm volatile("ltr %%ax" ::"a"(0x28));

    write("GDT+TSS loaded from kernel memory.\n");
}