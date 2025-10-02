#ifndef PAGING_H
#define PAGING_H

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

void paging_init(void);
unsigned int get_pte(unsigned int virt);
void unmap_all_user_pages(void);
void ensure_phys_range_mapped(unsigned int phys_start, unsigned int size);
int set_user_pages(unsigned int virt_start, unsigned int size);

#endif
