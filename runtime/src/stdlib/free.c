#include <stdlib.h>
#include "stdlib/heap_internal.h"

void free(void *ptr)
{
    if (!ptr)
        return;

    alloc_hdr_t *ah = (alloc_hdr_t *)((unsigned int)ptr - sizeof(alloc_hdr_t));
    free_hdr_t *blk = (free_hdr_t *)ah;
    blk->size = ah->size;

    free_list_insert_and_coalesce(blk);
}
