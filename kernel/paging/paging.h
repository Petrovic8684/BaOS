#ifndef PAGING_H
#define PAGING_H

#include "../drivers/display/display.h"

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

#define PT_POOL_COUNT 16

#ifndef PAGE_RW
#define PAGE_RW 0x2
#endif

void paging_install(void);
void map_page(unsigned int virt, unsigned int phys, unsigned int flags);
void unmap_page(unsigned int virt);
void set_user_pages(unsigned int virt_start, unsigned int size);
void ensure_phys_range_mapped(unsigned int phys_start, unsigned int size);
void unmap_user_range(unsigned int virt_start, unsigned int size);
unsigned int get_cr2(void);
unsigned int get_pte(unsigned int virt);

#endif
