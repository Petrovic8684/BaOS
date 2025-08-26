#ifndef PAGING_H
#define PAGING_H

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

#define PAGE_SIZE 4096
#define PAGE_ENTRIES 1024

#define PAGE_PRESENT 0x1
#define PAGE_RW 0x2
#define PAGE_USER 0x4
#define PAGE_PWT 0x8
#define PAGE_PCD 0x10
#define PAGE_ACCESSED 0x20
#define PAGE_DIRTY 0x40
#define PAGE_SIZE_4MB 0x80
#define PAGE_GLOBAL 0x100

extern uint32_t page_directory[PAGE_ENTRIES];

void paging_install(void);
void map_page(uint32_t virt, uint32_t phys, uint32_t flags);
void unmap_page(uint32_t virt);
void set_user_pages(uint32_t phys_start, uint32_t size);
uint32_t get_cr2(void);

void dump_descriptors_and_ptes(uint32_t user_entry, uint32_t user_stack_top);
void setup_kernel_gdt_and_tss(void);

#endif
