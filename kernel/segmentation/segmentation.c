#include "segmentation.h"
#include "../system/gdt/gdt.h"
#include "../drivers/display/display.h"
#include "../helpers/memory/memory.h"

#define E820_MAP_ADDR 0x8000U

struct e820_entry
{
    unsigned int base_lo;
    unsigned int base_hi;
    unsigned int len_lo;
    unsigned int len_hi;
    unsigned int type;
    unsigned int acpi;
};

static unsigned int user_code_limit = SEGMENT_ALIGN;
static unsigned int user_data_limit = SEGMENT_ALIGN;
static unsigned int last_user_region_start = 0;
static unsigned int last_user_region_size = 0;

static unsigned int get_e820_count(void)
{
    return *((unsigned int *)E820_MAP_ADDR);
}

static struct e820_entry *get_e820_entries(void)
{
    return (struct e820_entry *)(E820_MAP_ADDR + 4);
}

static unsigned long long get_total_detected_memory(void)
{
    unsigned long long total = 0;
    unsigned int count = get_e820_count();
    struct e820_entry *entries = get_e820_entries();
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        if (entries[i].type != 1)
            continue;

        unsigned long long base = ((unsigned long long)entries[i].base_hi << 32) | entries[i].base_lo;
        unsigned long long len = ((unsigned long long)entries[i].len_hi << 32) | entries[i].len_lo;
        (void)base;
        total += len;
    }

    return total;
}

unsigned int user_logical_to_phys(unsigned int logical_addr)
{
    return USER_PHYS_BASE + logical_addr;
}

void *user_ptr(void *user_addr)
{
    if (!user_addr)
        return ((void *)0);

    return (void *)user_logical_to_phys((unsigned int)user_addr);
}

const char *user_cstr(const char *user_addr)
{
    return (const char *)user_ptr((void *)user_addr);
}

void user_copy_in(void *kernel_dst, const void *user_src, unsigned int size)
{
    if (!kernel_dst || !user_src || size == 0)
        return;

    mem_copy(kernel_dst, user_ptr((void *)user_src), size);
}

void user_copy_out(void *user_dst, const void *kernel_src, unsigned int size)
{
    if (!user_dst || !kernel_src || size == 0)
        return;

    mem_copy(user_ptr(user_dst), kernel_src, size);
}

int is_user_address(unsigned int logical_addr)
{
    if (logical_addr >= USER_POOL_SIZE)
        return 0;

    if (logical_addr < user_code_limit)
        return 1;

    if (logical_addr < user_data_limit)
        return 1;

    if (logical_addr >= USER_STACK_BOTTOM)
        return 1;

    return 0;
}

void expand_kernel_segment(unsigned int end_addr)
{
    if (end_addr > USER_PHYS_BASE)
    {
        write("\033[31mError: Kernel heap expansion into user region. Halting...\n\033[0m");
        for (;;)
            __asm__ volatile("hlt");
    }
}

int set_user_code_limit(unsigned int logical_end)
{
    unsigned int aligned_end = (logical_end + SEGMENT_ALIGN - 1U) & ~(SEGMENT_ALIGN - 1U);

    if (aligned_end == 0)
        aligned_end = SEGMENT_ALIGN;

    if (aligned_end > USER_STACK_BOTTOM)
        return -12;

    if (aligned_end > user_code_limit)
    {
        user_code_limit = aligned_end;
        gdt_set_user_code_limit_bytes(user_code_limit);
    }

    return 0;
}

int expand_user_segment(unsigned int logical_start, unsigned int size)
{
    unsigned int need_end = logical_start + size;
    unsigned int aligned_end = (need_end + SEGMENT_ALIGN - 1U) & ~(SEGMENT_ALIGN - 1U);

    if (aligned_end > USER_STACK_BOTTOM)
        return -12;

    if (aligned_end > user_data_limit)
    {
        user_data_limit = aligned_end;
        gdt_set_user_data_limit_bytes(user_data_limit);
    }

    return 0;
}

void reset_user_segment(void)
{
    mem_set((unsigned char *)USER_PHYS_BASE, 0, USER_POOL_SIZE);

    user_code_limit = SEGMENT_ALIGN;
    user_data_limit = SEGMENT_ALIGN;
    gdt_set_user_code_limit_bytes(user_code_limit);
    gdt_set_user_data_limit_bytes(user_data_limit);
    gdt_set_user_stack_limit_bytes(USER_POOL_SIZE);

    last_user_region_start = 0;
    last_user_region_size = 0;
}

void segmentation_track_user_region(unsigned int logical_start, unsigned int size)
{
    unsigned int aligned_start = logical_start & ~(SEGMENT_ALIGN - 1U);
    unsigned int aligned_end = (logical_start + size + SEGMENT_ALIGN - 1U) & ~(SEGMENT_ALIGN - 1U);

    if (aligned_end <= aligned_start)
        return;

    if (last_user_region_start == 0 && last_user_region_size == 0)
    {
        last_user_region_start = aligned_start;
        last_user_region_size = aligned_end - aligned_start;
        return;
    }

    unsigned int current_end = last_user_region_start + last_user_region_size;

    if (aligned_start < last_user_region_start)
        last_user_region_start = aligned_start;

    if (aligned_end > current_end)
        last_user_region_size = aligned_end - last_user_region_start;
    else
        last_user_region_size = current_end - last_user_region_start;
}

void segmentation_init(void)
{
    unsigned int cr0;

    write("Initializing segmentation...\n");

    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    if (cr0 & 0x80000000U)
    {
        write("\033[31mError: Paging is enabled. Halting...\n\033[0m");
        for (;;)
            __asm__ volatile("hlt");
    }

    gdt_init();

    write("Memory detected: ");
    {
        unsigned long long total = get_total_detected_memory();
        unsigned long long mb = total / (1024ULL * 1024ULL);
        write_dec(mb);
    }
    write(" MB\n");
    write("\033[32mSegmentation enabled (CS/DS/SS, kernel below 0x");
    write_hex(USER_PHYS_BASE);
    write(", user pool ");
    write_dec(USER_POOL_SIZE / (1024U * 1024U));
    write(" MB).\033[0m\n\n");
}
