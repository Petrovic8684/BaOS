#ifndef BAOS_STDLIB_HEAP_INTERNAL_H
#define BAOS_STDLIB_HEAP_INTERNAL_H

#include <stdlib.h>

extern unsigned int _end;

typedef struct free_hdr
{
    unsigned int size;
    struct free_hdr *next;
} free_hdr_t;

typedef struct alloc_hdr
{
    unsigned int size;
} alloc_hdr_t;

#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define ALLOC_ALIGN 8
#define PAGE_SIZE_LOCAL 4096
#define USER_STACK_TOP 0x02100000U
#define USER_STACK_PAGES 4U

extern free_hdr_t *free_list;

void *heap_expand(unsigned int bytes);
void free_list_insert_and_coalesce(free_hdr_t *blk);
void heap_init_once(void);

#endif
