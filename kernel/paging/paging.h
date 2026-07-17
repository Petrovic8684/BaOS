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

#define USER_PHYS_BASE 0x02000000U
#define USER_POOL_SIZE 0x00100000U
#define USER_STACK_PAGES 4U
#define USER_STACK_TOP (USER_PHYS_BASE + USER_POOL_SIZE)
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_PAGES * PAGE_SIZE)

void paging_init(void);
unsigned int get_pte(unsigned int virt);
void unmap_all_user_pages(void);
void ensure_phys_range_mapped(unsigned int phys_start, unsigned int size);
int set_user_pages(unsigned int virt_start, unsigned int size);
int is_user_address(unsigned int virt_addr);
void *user_ptr(void *user_addr);
const char *user_cstr(const char *user_addr);
void user_copy_in(void *kernel_dst, const void *user_src, unsigned int size);
void user_copy_out(void *user_dst, const void *kernel_src, unsigned int size);

#endif
