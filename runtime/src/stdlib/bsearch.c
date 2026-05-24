#include <stdlib.h>

void *bsearch(const void *key, const void *base, size_t nitems, size_t size,
              int (*compar)(const void *, const void *))
{
    size_t left = 0;
    size_t right = nitems;
    while (left < right)
    {
        size_t mid = left + (right - left) / 2;
        const void *elem = (const char *)base + mid * size;
        int cmp = compar(key, elem);
        if (cmp < 0)
            right = mid;
        else if (cmp > 0)
            left = mid + 1;
        else
            return (void *)elem;
    }
    return NULL;
}
