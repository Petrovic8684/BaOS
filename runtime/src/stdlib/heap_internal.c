#include "stdlib/heap_internal.h"
#include "internal/syscalls.h"
#include <errno.h>
#include <string.h>

free_hdr_t *free_list = NULL;

static unsigned int heap_start = 0;
static unsigned int heap_end = 0;
static unsigned int heap_max = 0;

static int sys_set_user_pages(unsigned int virt_start, unsigned int size)
{
    struct map_args
    {
        unsigned int virt_start;
        unsigned int size;
    } args;
    args.virt_start = virt_start;
    args.size = size;

    int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(ret)
        : [num] "i"(SYS_SET_USER_PAGES), [arg] "r"(&args)
        : "eax", "ebx", "memory");

    if (ret < 0)
    {
        errno = -ret;
        return -1;
    }

    return 0;
}

void *heap_expand(unsigned int bytes)
{
    if (bytes == 0)
        return (void *)heap_end;

    unsigned int need_end = ALIGN_UP(heap_end + bytes, PAGE_SIZE_LOCAL);

    if (need_end > heap_max)
    {
        errno = ENOMEM;
        return NULL;
    }

    unsigned int map_start = heap_end;
    unsigned int map_size = need_end - heap_end;

    if (sys_set_user_pages(map_start, map_size) != 0)
        return NULL;

    void *old = (void *)heap_end;
    heap_end = need_end;
    return old;
}

void free_list_insert_and_coalesce(free_hdr_t *blk)
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

void heap_init_once(void)
{
    if (heap_start != 0)
        return;

    heap_start = ALIGN_UP((unsigned int)&_end, PAGE_SIZE_LOCAL);
    heap_end = heap_start;

    heap_max = USER_STACK_TOP - (USER_STACK_PAGES * PAGE_SIZE_LOCAL);

    free_list = NULL;
}
