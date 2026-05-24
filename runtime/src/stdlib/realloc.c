#include <stdlib.h>
#include <string.h>
#include "stdlib/heap_internal.h"

void *realloc(void *ptr, unsigned int size)
{
    if (ptr == NULL)
        return malloc(size);

    if (size == 0)
    {
        free(ptr);
        return NULL;
    }

    alloc_hdr_t *ah = (alloc_hdr_t *)((unsigned int)ptr - sizeof(alloc_hdr_t));
    unsigned int old_total = ah->size;
    unsigned int old_payload = old_total - sizeof(alloc_hdr_t);

    if (old_payload >= size)
        return ptr;

    void *newp = malloc(size);
    if (!newp)
        return NULL;

    unsigned int to_copy = (old_payload < size) ? old_payload : size;
    memcpy(newp, ptr, to_copy);
    free(ptr);
    return newp;
}
