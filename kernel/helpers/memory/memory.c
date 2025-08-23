#include "memory.h"

void mem_copy(void *dst, const void *src, unsigned int n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    for (unsigned int i = 0; i < n; i++)
        d[i] = s[i];
}

void mem_set(void *dst, unsigned char val, unsigned int n)
{
    unsigned char *d = (unsigned char *)dst;

    for (unsigned int i = 0; i < n; i++)
        d[i] = val;
}