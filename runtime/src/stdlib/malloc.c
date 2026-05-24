#include <stdlib.h>
#include "stdlib/heap_internal.h"

void *malloc(unsigned int size)
{
    if (size == 0)
        return NULL;

    heap_init_once();

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
    if (!addr)
        return NULL;

    alloc_hdr_t *ah = (alloc_hdr_t *)addr;
    ah->size = total_sz;
    return (void *)((unsigned int)addr + sizeof(alloc_hdr_t));
}
