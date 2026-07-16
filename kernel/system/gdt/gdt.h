#ifndef GDT_H
#define GDT_H

#define GDT_ENTRIES 7

#define SEGMENT_ALIGN 4096U
#define USER_PHYS_BASE 0x02000000U
#define USER_POOL_SIZE 0x00100000U
#define USER_STACK_PAGES 4U
#define USER_STACK_TOP USER_POOL_SIZE
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_PAGES * SEGMENT_ALIGN)
#define KERNEL_SEGMENT_LIMIT (USER_PHYS_BASE + USER_POOL_SIZE)
#define KERNEL_HEAP_MAX (16U * 1024U * 1024U)

#define GDT_IDX_KERNEL_CODE 1
#define GDT_IDX_KERNEL_DATA 2
#define GDT_IDX_USER_CODE 3
#define GDT_IDX_USER_DATA 4
#define GDT_IDX_USER_STACK 5
#define GDT_IDX_TSS 6

#define GDT_GRAN_4K 0xCF

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
unsigned int bytes_to_gdt_limit(unsigned int size_bytes);
void gdt_set_kernel_limit_bytes(unsigned int limit_bytes);
void gdt_set_user_code_limit_bytes(unsigned int limit_bytes);
void gdt_set_user_data_limit_bytes(unsigned int limit_bytes);
void gdt_set_user_stack_limit_bytes(unsigned int limit_bytes);
void reload_kernel_segments(void);
void reload_user_segments(void);

#endif
