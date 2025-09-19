#include "heap.h"
#include "../paging.h"
#include "../../helpers/memory/memory.h"
#include "../../drivers/display/display.h"

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

static free_hdr_t *free_list = ((void *)0);

extern unsigned int _end;

static unsigned int heap_start = 0;
static unsigned int heap_end = 0;
static unsigned int heap_max = 0;

static void *heap_expand(unsigned int bytes)
{
    if (bytes == 0)
        return (void *)heap_end;

    unsigned int need_end = ALIGN_UP(heap_end + bytes, PAGE_SIZE_LOCAL);

    if (need_end > heap_max)
    {
        write("\033[31mError: Out of heap space. Halting...\n\033[0m");
        for (;;)
            __asm__ volatile("hlt");
    }

    ensure_phys_range_mapped(heap_end, need_end - heap_end);

    void *old = (void *)heap_end;
    heap_end = need_end;
    return old;
}

static void free_list_insert_and_coalesce(free_hdr_t *blk)
{
    free_hdr_t **p = &free_list;
    while (*p && (unsigned int)(*p) < (unsigned int)blk)
        p = &(*p)->next;

    blk->next = *p;
    *p = blk;

    if (blk->next)
    {
        unsigned int blk_end = (unsigned int)blk + blk->size;

        if (blk_end == (unsigned int)blk->next)
        {
            blk->size += blk->next->size;
            blk->next = blk->next->next;
        }
    }

    if (p != &free_list)
    {
        free_hdr_t *prev = free_list;

        while (prev && prev->next != blk)
            prev = prev->next;

        if (prev)
        {
            unsigned int prev_end = (unsigned int)prev + prev->size;

            if (prev_end == (unsigned int)blk)
            {
                prev->size += blk->size;
                prev->next = blk->next;
            }
        }
    }
}

void heap_init(void)
{
    write("Initializing heap...\n");
    heap_start = ALIGN_UP((unsigned int)&_end, PAGE_SIZE_LOCAL);
    heap_end = heap_start;

    heap_max = heap_start + (16 * 1024 * 1024U);

    free_list = ((void *)0);

    write("\033[32mHeap initialized: start=");
    write_hex(heap_start);
    write(", max=");
    write_hex(heap_max);
    write("\n\033[0m");
}

void *kmalloc(unsigned int size)
{
    if (size == 0)
        return ((void *)0);

    unsigned int payload_sz = (unsigned int)ALIGN_UP(size, ALLOC_ALIGN);
    unsigned int total_sz = payload_sz + sizeof(alloc_hdr_t);

    free_hdr_t **p = &free_list;
    while (*p)
    {
        if ((*p)->size >= total_sz)
        {
            free_hdr_t *found = *p;

            if (found->size >= total_sz + (unsigned int)sizeof(free_hdr_t) + ALLOC_ALIGN)
            {
                unsigned int orig_size = found->size;
                unsigned int leftover_size = orig_size - total_sz;
                unsigned int alloc_addr = (unsigned int)found;
                unsigned int new_free_addr = alloc_addr + total_sz;

                free_hdr_t *new_free = (free_hdr_t *)new_free_addr;
                new_free->size = leftover_size;
                new_free->next = found->next;

                *p = new_free;

                alloc_hdr_t *ah = (alloc_hdr_t *)alloc_addr;
                ah->size = total_sz;
                return (void *)(alloc_addr + sizeof(alloc_hdr_t));
            }
            else
            {
                *p = found->next;
                alloc_hdr_t *ah = (alloc_hdr_t *)found;
                ah->size = found->size;
                return (void *)((unsigned int)found + sizeof(alloc_hdr_t));
            }
        }
        p = &(*p)->next;
    }

    void *addr = heap_expand(total_sz);
    alloc_hdr_t *ah = (alloc_hdr_t *)addr;
    ah->size = total_sz;

    return (void *)((unsigned int)addr + sizeof(alloc_hdr_t));
}

void kfree(void *ptr)
{
    if (!ptr)
        return;

    alloc_hdr_t *ah = (alloc_hdr_t *)((unsigned int)ptr - sizeof(alloc_hdr_t));
    free_hdr_t *blk = (free_hdr_t *)ah;
    blk->size = ah->size;

    free_list_insert_and_coalesce(blk);
}
